#include "ModelLoader.h"
#include <sstream>
#include <assimp/postprocess.h>

void ModelLoader::clear()
{
	m_texturePaths.clear();

	// delete the nodes that we created for the sub meshes
	for each (aiNode* var in m_Nodes)
	{
		std::string name = std::string(var->mName.C_Str());
		if (name.find("-submesh") != std::string::npos)
		{
			SAFE_DELETE(var);
		}
	}
	m_subMeshNodes.clear();
	m_Nodes.clear();
}

ModelLoader::ModelLoader()
{
}

ModelLoader::~ModelLoader()
{
	clear();
}

vector<ModelDataPtr> ModelLoader::loadModel(const string& fileName, LoadingQuality flag)
{
	m_fileName = fileName;
	uint flags;

	if (flag == SIMPLE)
		flags = aiProcess_Triangulate | aiProcess_GenNormals;
	else if (flag == FAST)
		flags = aiProcessPreset_TargetRealtime_Fast;
	else if (flag == QUALITY)
		flags = aiProcessPreset_TargetRealtime_Quality;
	else if (flag == MAX_QUALITY)
		flags = aiProcessPreset_TargetRealtime_MaxQuality;

	m_aiScene = m_importer.ReadFile(fileName, flags);
	
	if (!m_aiScene)
	{
		cout << m_importer.GetErrorString();
		vector<ModelDataPtr> empty;
		return empty;
	}

	m_GlobalInverseTransform = m_aiScene->mRootNode->mTransformation.Inverse();

	uint numVertices = 0, numIndices = 0, numFaces = 0;
	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		numVertices += m_aiScene->mMeshes[i]->mNumVertices;
		numIndices += m_aiScene->mMeshes[i]->mNumFaces * 3;
		numFaces += m_aiScene->mMeshes[i]->mNumFaces;
	}

	numVertices = 0;
	numIndices = 0;
	vector<ModelDataPtr> modelDataVector;
	modelDataVector.resize(m_aiScene->mNumMeshes);

	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		ModelDataPtr md(new ModelData());

		md->meshData = loadMesh(i, numVertices, numIndices);
		md->materialData = loadMaterial(m_aiScene->mMaterials[m_aiScene->mMeshes[i]->mMaterialIndex]);

		numVertices += m_aiScene->mMeshes[i]->mNumVertices;
		numIndices += m_aiScene->mMeshes[i]->mNumFaces * 3;
		numFaces += m_aiScene->mMeshes[i]->mNumFaces;

		modelDataVector[i] = ModelDataPtr(md);

		aiNode* meshNode = getNode(md->meshData.name.c_str(), m_subMeshNodes);
		
		if (!meshNode)
		{
			meshNode = m_aiScene->mRootNode->FindNode(md->meshData.name.c_str());
		}

		md->meshData.unpackMatrix = calculateGlobalTransform(meshNode);

		m_Nodes.push_back(meshNode);
	}

	// parse other nodes, this step makes sure the mesh nodes are in the front
	parseNoneMeshNodes(m_aiScene->mRootNode);
	for (uint i = 0; i < m_Nodes.size(); ++i)
	{
		int parentIdx = -1;
		for (int j = 0; j < (int)m_Nodes.size(); ++j)
		{
			if (m_Nodes[j] == m_Nodes[i]->mParent)
			{
				parentIdx = j;
				break;
			}
		}

		cout << "\n" << i << " " /*<< parentIdx << " "*/ << m_Nodes[i]->mName.C_Str();
	}

	// load bones
	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		aiMesh* mesh = m_aiScene->mMeshes[i];
		if (mesh->HasBones())
			loadBones(mesh, modelDataVector[i]->meshData);
	}

	// generate the skeleton of the model
	// specify the root bone
 	if (m_BoneMapping.size() > 0)
// 	{
// 		Bone* skeleton_root = new Bone();
// 		skeleton_root->m_ID = 9999;
// 		skeleton_root->m_name = "Skeleton ROOT";
// 
// 		mat4 identity;
// 		generateSkeleton(m_aiScene->mRootNode, skeleton_root, identity);
// 		m_skeleton = new Skeleton(skeleton_root, m_GlobalInverseTransform);
// 
// 		// print out the skeleton
// 		//m_skeleton->dumpSkeleton(skeleton_root, 0);
// 	}

	cout << "Loaded " << fileName << endl;
	cout << "Model has " << m_aiScene->mNumMeshes << " meshes, " << numVertices << " vertices, " << numFaces << " faces. ";
	if (m_BoneMapping.size())
		cout << "Contains " << m_BoneMapping.size() << " bones. ";
	if (m_aiScene->HasAnimations())
	{
		cout << "Contains " << m_aiScene->mAnimations[0]->mDuration << " seconds animation. ";
	}

	return modelDataVector;
}

string ModelLoader::getMeshNameFromNode(unsigned int meshIndex, aiNode* pNode)
{
	// traverse the node structure to find a matching index
	string result;

	if (!pNode) return result;

	for (uint i = 0; i < pNode->mNumMeshes; ++i)
	{
		if (pNode->mMeshes[i] == meshIndex)
		{
			if (pNode->mNumMeshes > 1)
			{
				stringstream ss;
				ss << i;
				string suffix = "-submesh" + ss.str();
				result = string(pNode->mName.C_Str()) + suffix;

				// create mesh nodes
				aiNode* meshNode = new aiNode(result);
				meshNode->mNumMeshes = 1;
				aiNode* parent = pNode->mParent;
				meshNode->mParent = parent;
				meshNode->mMeshes = new unsigned int[1]{ meshIndex };

				// register this new node
				//++parent->mNumChildren;
				//parent->mChildren[parent->mNumChildren - 1] = meshNode;

				m_subMeshNodes.push_back(meshNode);
			}
			else
			{
				result = string(pNode->mName.C_Str());
			}

			break;
		}
	}

	if (result.empty())
	{
		for (uint i = 0; i < pNode->mNumChildren; ++i)
		{
			result = getMeshNameFromNode(meshIndex, pNode->mChildren[i]);
			if (!result.empty())
				break;
		}
	}

	return result;
}

void ModelLoader::readVertexAttributes(unsigned int index, const aiMesh* mesh, MeshData& data)
{
	const vec3 zero3D(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	for (uint i = 0; i < mesh->mNumVertices; ++i)
	{
		vec3 pos = (mesh->mVertices[i]);
		vec3 texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero3D;
		vec3 normal = mesh->HasNormals() ? mesh->mNormals[i] : zero3D;
		vec3 tangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[i] : zero3D;

		data.positions.push_back(pos);
		data.texCoords.push_back(vec2(texCoord.x, texCoord.y));
		data.normals.push_back(normal);
		data.tangents.push_back(tangent);
	}

	// Populate the index buffer
	uint numNotsupportedFaces = 0;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		const aiFace& face = mesh->mFaces[i];

		if (face.mNumIndices != 3)
		{
			cout << "Unsupported number of indices per face " << face.mNumIndices << endl;
			++numNotsupportedFaces;
		}
		else
		{
			data.indices.push_back((uint16)face.mIndices[0]);
			data.indices.push_back((uint16)face.mIndices[1]);
			data.indices.push_back((uint16)face.mIndices[2]);
		}
	}
}

void ModelLoader::loadBones(const aiMesh* paiMesh, MeshData& data)
{
	data.bones.resize(paiMesh->mNumVertices);
	for (uint i = 0; i < paiMesh->mNumBones; ++i)
	{
		uint8_t boneIndex = 0;
		string boneName(paiMesh->mBones[i]->mName.data);
		if (m_BoneMapping.find(boneName) == m_BoneMapping.end())
		{
			// Allocate an index for a new bone
			aiNode* boneNode = getNode(boneName.c_str(), m_Nodes);
			auto it = std::find(m_Nodes.begin(), m_Nodes.end(), boneNode);
			boneIndex = std::distance(m_Nodes.begin(), it);

			Bone bi;
			bi.m_ID = boneIndex;
			bi.m_name = boneName;
			bi.m_offsetMatrix = paiMesh->mBones[i]->mOffsetMatrix;
			m_BoneMapping[boneName] = boneIndex;
			m_BoneInfo.push_back(bi);
		}
		else
		{
			boneIndex = m_BoneMapping[boneName];
		}

		for (uint j = 0; j < paiMesh->mBones[i]->mNumWeights; ++j)
		{
			uint VertexID = paiMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = paiMesh->mBones[i]->mWeights[j].mWeight;
			data.bones[VertexID].AddBoneData(boneIndex, Weight);
		}
	}
}

void ModelLoader::generateSkeleton(aiNode* pAiRootNode, Bone* pRootSkeleton, mat4& parentTransform)
{
	// generate a skeleton from the existing bone map and BoneInfo vector
	Bone* pBone = NULL;

	string nodeName(pAiRootNode->mName.data);

	mat4 nodeTransformation(pAiRootNode->mTransformation);
	mat4 globalTransformation = parentTransform * nodeTransformation;

	// aiNode is not aiBone, aiBones are part of all the aiNodes
	if (m_BoneMapping.find(nodeName) != m_BoneMapping.end())
	{
		uint BoneIndex = m_BoneMapping[nodeName];
		m_BoneInfo[BoneIndex].m_boneSpaceTransform = pAiRootNode->mTransformation;

		Bone bi = m_BoneInfo[BoneIndex];
		pBone = new Bone(pRootSkeleton);
		pBone->m_ID = BoneIndex;
		pBone->m_name = bi.m_name;

		pBone->m_offsetMatrix = bi.m_offsetMatrix;
		pBone->m_boneSpaceTransform = pAiRootNode->mTransformation;
		pBone->m_modelSpaceTransform = globalTransformation;
		pBone->m_finalTransform = m_GlobalInverseTransform * pBone->m_modelSpaceTransform * pBone->m_offsetMatrix;
	}

	for (uint i = 0; i < pAiRootNode->mNumChildren; ++i)
	{
		if (pBone) generateSkeleton(pAiRootNode->mChildren[i], pBone, globalTransformation);
		else generateSkeleton(pAiRootNode->mChildren[i], pRootSkeleton, globalTransformation);
	}
}

MeshData ModelLoader::loadMesh(unsigned int index, unsigned int baseVertex, unsigned int baseIndex)
{
	MeshData data;
	aiMesh* mesh = m_aiScene->mMeshes[index];

	data.name = getMeshNameFromNode(index, m_aiScene->mRootNode);
	data.numVertices = mesh->mNumVertices;
	data.numFaces = mesh->mNumFaces;
	data.numIndices = mesh->mNumFaces * 3;
	data.baseVertex = baseVertex;
	data.baseIndex = baseIndex;

	readVertexAttributes(index, mesh, data);

	return data;
}

MaterialData ModelLoader::loadMaterial(const aiMaterial* material)
{
	assert(material != nullptr);

	aiString name;
	material->Get(AI_MATKEY_NAME, name);

	MaterialData data;
	data.name = string(name.C_Str());
	const size_t last_idx = data.name.rfind("-material");
	if (std::string::npos != last_idx)
	{
		data.name = data.name.substr(0, last_idx);
	}

	color4D ambientColor(0.1f, 0.1f, 0.1f, 1.0f);
	color4D diffuseColor(0.8f, 0.8f, 0.8f, 1.0f);
	color4D specularColor(0.0f, 0.0f, 0.0f, 1.0f);
	data.ambientColor = ambientColor;
	data.diffuseColor = diffuseColor;
	data.specularColor = specularColor;

	int blendMode;
	data.blendMode = -1;

	float opacity = 1.0f;
	data.opacity = 1.0f;

	float shininess = 10.0f;
	data.shininess = 10.0f;

	if (material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor) == AI_SUCCESS)
	{
		data.ambientColor = ambientColor;
	}

	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
	{
		data.diffuseColor = diffuseColor;
	}

	if (material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS)
	{
		data.specularColor = specularColor;
	}

	if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
	{
		data.ambientColor.a = opacity;
		data.diffuseColor.a = opacity;
		data.specularColor.a = opacity;
		data.opacity = opacity;

		if (opacity < 1.0f)
		{

			if (material->Get(AI_MATKEY_BLEND_FUNC, blendMode) == AI_SUCCESS)
			{
				if (blendMode == aiBlendMode_Additive)
					data.blendMode = aiBlendMode_Additive;
				else
					data.blendMode = aiBlendMode_Default;
			}
		}
		else
		{
			data.blendMode = -1;
		}
	}

	if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
	{
		data.shininess = shininess;
	}

	// process the textures
	data.textureData = loadTexture(material);

	return data;
}

TextureData ModelLoader::loadTexture(const aiMaterial* material)
{
	assert(material);

	// Extract the directory part from the file name
	TextureData data = TextureData();
	aiString path;

	string directory;
	const size_t last_slash_idx = m_fileName.rfind('/');
	if (std::string::npos != last_slash_idx)
	{
		directory = m_fileName.substr(0, last_slash_idx);
	}

	// process all texture types
	vector<aiTextureType> textureTypes;
	if (material->GetTextureCount(aiTextureType_DIFFUSE)) textureTypes.push_back(aiTextureType_DIFFUSE);
	if (material->GetTextureCount(aiTextureType_AMBIENT)) textureTypes.push_back(aiTextureType_AMBIENT);
	if (material->GetTextureCount(aiTextureType_SPECULAR)) textureTypes.push_back(aiTextureType_SPECULAR);
	if (material->GetTextureCount(aiTextureType_HEIGHT)) textureTypes.push_back(aiTextureType_HEIGHT);
	if (material->GetTextureCount(aiTextureType_NORMALS)) textureTypes.push_back(aiTextureType_NORMALS);
	if (material->GetTextureCount(aiTextureType_EMISSIVE)) textureTypes.push_back(aiTextureType_EMISSIVE);
	if (material->GetTextureCount(aiTextureType_SHININESS)) textureTypes.push_back(aiTextureType_SHININESS);
	if (material->GetTextureCount(aiTextureType_OPACITY)) textureTypes.push_back(aiTextureType_OPACITY);
	if (material->GetTextureCount(aiTextureType_REFLECTION)) textureTypes.push_back(aiTextureType_REFLECTION);
	//if (material->GetTextureCount(aiTextureType_DISPLACEMENT)) textureTypes.push_back(aiTextureType_DISPLACEMENT);
	//if (material->GetTextureCount(aiTextureType_LIGHTMAP)) textureTypes.push_back(aiTextureType_LIGHTMAP);
	//if(material->GetTextureCount(aiTextureType_UNKNOWN)     ) textureFeatures .push_back( aiTextureType_UNKNOWN);

	for each(aiTextureType type in textureTypes)
	{
		if (material->GetTexture(type, 0, &path) == AI_SUCCESS)
		{
			if (strlen(path.data) == 0) continue;

			// absolute path
			//string texturePath = directory + "/" + path.data;

			// relative path
			string texturePath = path.data;

			data.texturesMap[type] = texturePath;
			m_texturePaths.push_back(texturePath);
		}
	}

	return data;
}

void ModelLoader::parseNoneMeshNodes(aiNode* pNode)
{
	if (!pNode) return;

	if (pNode->mNumMeshes == 0)
		m_Nodes.push_back(pNode);

	for (size_t i = 0; i < pNode->mNumChildren; ++i)
	{
		parseNoneMeshNodes(pNode->mChildren[i]);
	}
}

aiNode* ModelLoader::getNode(const char* meshName, vector<aiNode*>& source)
{
	for each (aiNode* var in source)
	{
		if (!strcmp(meshName, var->mName.C_Str()))
		{
			return var;
		}
	}

	return NULL;
}

mat4 ModelLoader::calculateGlobalTransform(aiNode* pNode)
{
	mat4 result;
	while (pNode->mParent)
	{
		result = pNode->mParent->mTransformation * result;
		pNode = pNode->mParent;
	}

	return result;
}
