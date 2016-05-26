#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

void ModelLoader::clear()
{
	m_positions.clear();
	m_colors.clear();
	m_texCoords.clear();
	m_normals.clear();
	m_tangents.clear();
	m_indices.clear();
	m_Bones.clear();
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

	Assimp::Importer importer;
	m_aiScene = importer.ReadFile(fileName, flags);
	
	if (!m_aiScene)
	{
		cout << importer.GetErrorString();
		vector<ModelDataPtr> empty;
		return empty;
	}

	// count nodes
	m_NumNodes = 0;
	countChildren(m_aiScene->mRootNode);

	m_GlobalInverseTransform = m_aiScene->mRootNode->mTransformation.Inverse();

	uint numVertices = 0, numIndices = 0, numFaces = 0;

	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		numVertices += m_aiScene->mMeshes[i]->mNumVertices;
		numIndices += m_aiScene->mMeshes[i]->mNumFaces * 3;
		numFaces += m_aiScene->mMeshes[i]->mNumFaces;
	}

	m_positions.reserve(numVertices);
	m_colors.reserve(numVertices);
	m_normals.reserve(numVertices);
	m_texCoords.reserve(numVertices);
	m_tangents.reserve(numVertices);
	m_indices.reserve(numIndices);
	m_Bones.resize(numVertices);

	m_animationDuration = 0;
	numVertices = 0;
	numIndices = 0;
	m_NumBones = 0;
	vector<ModelDataPtr> modelDataVector;
	modelDataVector.resize(m_aiScene->mNumMeshes);

	aiNode* node = m_aiScene->mRootNode->mChildren[1]->mChildren[0];

	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		ModelDataPtr md(new ModelData());

		md->meshData = loadMesh(i, numVertices, numIndices, m_aiScene->mMeshes[i]);

		md->materialData = loadMaterial(i, m_aiScene->mMaterials[m_aiScene->mMeshes[i]->mMaterialIndex]);

		md->hasAnimation = m_aiScene->HasAnimations();

		// calculate the animation duration in seconds
		if (m_aiScene->HasAnimations()) md->animationDuration = (float)m_aiScene->mAnimations[0]->mDuration;
		else md->animationDuration = 0.0f;

		numVertices += m_aiScene->mMeshes[i]->mNumVertices;
		numIndices += m_aiScene->mMeshes[i]->mNumFaces * 3;

		modelDataVector[i] = ModelDataPtr(md);
	}

	// generate the skeleton of the model
	// specify the root bone
	if (m_BoneMapping.size() > 0)
	{
		Bone* skeleton_root = new Bone();
		skeleton_root->m_ID = 9999;
		skeleton_root->m_name = "Skeleton ROOT";

		mat4 identity;
		generateSkeleton(m_aiScene->mRootNode, skeleton_root, identity);
		m_skeleton = new Skeleton(skeleton_root, m_GlobalInverseTransform);

		// print out the skeleton
		//m_skeleton->dumpSkeleton(skeleton_root, 0);
	}

	cout << "Loaded " << fileName << endl;
	cout << "Model has " << m_aiScene->mNumMeshes << " meshes, " << numVertices << " vertices, " << numFaces << " faces. ";
	if (m_NumBones)
		cout << "Contains " << m_NumBones << " bones. ";
	if (m_aiScene->HasAnimations())
	{
		m_animationDuration = m_aiScene->mAnimations[0]->mDuration;
		cout << "Contains " << m_animationDuration << " seconds animation. ";
	}

	/*
	// CONSTRUCT THE POD MODEL
	pvr::assets::Model::InternalData& modelInternalData = model.getInternalData();

	// camera
	for (uint i = 0; i < m_aiScene->mNumCameras; ++i)
	{
		aiCamera *cam = m_aiScene->mCameras[i];
		pvr::assets::Camera pvrCam;
		pvrCam.setFar(cam->mClipPlaneFar);
		pvrCam.setNear(cam->mClipPlaneNear);
		pvrCam.setFOV(cam->mHorizontalFOV);
		modelInternalData.cameras.push_back(pvrCam);
	}

	// lights
	for (uint i = 0; i < m_aiScene->mNumLights; ++i)
	{
		aiLight *l = m_aiScene->mLights[i];
		pvr::assets::Light pvrLight;
		pvrLight.setColor(l->mColorDiffuse.r, l->mColorDiffuse.g, l->mColorDiffuse.b);
		pvrLight.setConstantAttenuation(l->mAttenuationConstant);
		pvrLight.setLinearAttenuation(l->mAttenuationLinear);
		pvrLight.setQuadraticAttenuation(l->mAttenuationQuadratic);

		switch (l->mType)
		{
		case aiLightSource_DIRECTIONAL:
			pvrLight.setType(pvr::assets::Light::Directional);
			break;
		case aiLightSource_POINT:
			pvrLight.setType(pvr::assets::Light::Point);
			break;
		case aiLightSource_SPOT:
			pvrLight.setType(pvr::assets::Light::Spot);
			break;
		default:
			break;
		}

		modelInternalData.lights.push_back(pvrLight);
	}
	
	for (int i = 0; i < modelDataVector.size(); ++i)
	{
		ModelDataPtr md = modelDataVector[i];

		// meshes
		MeshData mesh = md->meshData;
		pvr::assets::Mesh pvrMesh;
		pvrMesh.setNumVertices(mesh.numVertices);
		pvrMesh.setNumFaces(mesh.numFaces);
	}
	*/
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
			result = string(pNode->mName.C_Str());

			// multiple meshes under a root, e.g. hair + head under the root head
			if (i > 0)
			{
				result += "_" + i;
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

void ModelLoader::prepareVertexContainers(unsigned int index, const aiMesh* mesh)
{
	const vec3 zero3D(0.0f, 0.0f, 0.0f);
	const color4D  zeroColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Populate the vertex attribute vectors
	for (uint i = 0; i < mesh->mNumVertices; ++i)
	{
		vec3 pos = (mesh->mVertices[i]);
		vec3 texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero3D;
		vec3 normal = mesh->HasNormals() ? mesh->mNormals[i] : zero3D;
		vec3 tangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[i] : zero3D;
		color4D color = mesh->HasVertexColors(0) ? mesh->mColors[0][i] : zeroColor;

		m_positions.push_back(pos);
		m_texCoords.push_back(vec2(texCoord.x, texCoord.y));
		m_normals.push_back(normal);
		m_tangents.push_back(tangent);
		m_colors.push_back(color);
	}

	if (mesh->HasBones()) loadBones(index, mesh);


	// Populate the index buffer
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		const aiFace& face = mesh->mFaces[i];

		if (face.mNumIndices != 3)
		{
			// Unsupported modes : GL_POINTS / GL_LINES / GL_POLYGON
			cout << "Unsupported number of indices per face " << face.mNumIndices;
			break;
		}
		else
		{
			m_indices.push_back((uint16)face.mIndices[0]);
			m_indices.push_back((uint16)face.mIndices[1]);
			m_indices.push_back((uint16)face.mIndices[2]);
		}
	}
}

void ModelLoader::loadBones(uint MeshIndex, const aiMesh* paiMesh)
{
	for (uint i = 0; i < paiMesh->mNumBones; ++i)
	{
		uint boneIndex = 0;
		string boneName(paiMesh->mBones[i]->mName.data);
		if (m_BoneMapping.find(boneName) == m_BoneMapping.end())
		{
			// Allocate an index for a new bone
			boneIndex = m_NumBones;
			m_NumBones++;
			Bone bi;
			m_BoneInfo.push_back(bi);
			m_BoneInfo[boneIndex].m_ID = boneIndex;
			m_BoneInfo[boneIndex].m_name = boneName;
			m_BoneInfo[boneIndex].m_offsetMatrix = paiMesh->mBones[i]->mOffsetMatrix;
			m_BoneMapping[boneName] = boneIndex;
		}
		else
		{
			boneIndex = m_BoneMapping[boneName];
		}

		uint offset = 0;
		for (uint k = 0; k < MeshIndex; ++k)
		{
			offset += m_aiScene->mMeshes[k]->mNumVertices;
		}

		for (uint j = 0; j < paiMesh->mBones[i]->mNumWeights; ++j)
		{
			uint VertexID = offset + paiMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = paiMesh->mBones[i]->mWeights[j].mWeight;
			m_Bones[VertexID].AddBoneData(boneIndex, Weight);
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

MeshData ModelLoader::loadMesh(unsigned int index, unsigned int numVertices, unsigned int numIndices, const aiMesh* mesh)
{
	MeshData data;

// 	data.name = mesh->mName.length > 0
// 		? m_fileName + "/mesh_" + mesh->mName.C_Str()
// 		: m_fileName + "/mesh_" + (char)index;

	data.name = getMeshNameFromNode(index, m_aiScene->mRootNode);
	data.numVertices = mesh->mNumVertices;
	data.numFaces = mesh->mNumFaces;
	data.numIndices = mesh->mNumFaces * 3;
	data.baseVertex = numVertices;
	data.baseIndex = numIndices;

	prepareVertexContainers(index, mesh);

	return data;
}

MaterialData ModelLoader::loadMaterial(unsigned int index, const aiMaterial* material)
{
	assert(material != nullptr);

	aiString name;
	material->Get(AI_MATKEY_NAME, name);

	MaterialData data;
	//data.name = m_fileName + "/material_" + string::number(index);
	data.name = name.C_Str();

	color4D ambientColor(0.1f, 0.1f, 0.1f, 1.0f);
	color4D diffuseColor(0.8f, 0.8f, 0.8f, 1.0f);
	color4D specularColor(0.0f, 0.0f, 0.0f, 1.0f);
	color4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
	data.ambientColor = ambientColor;
	data.diffuseColor = diffuseColor;
	data.specularColor = specularColor;
	data.emissiveColor = emissiveColor;

	int blendMode;
	data.blendMode = -1;

	int twoSided = 1;
	data.twoSided = 1;

	float opacity = 1.0f;

	float shininess = 10.0f;
	data.shininess = 10.0f;
	float shininessStrength = 0.0f;
	data.shininessStrength = 0.0f;

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

	if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
	{
		data.emissiveColor = emissiveColor;
	}

	if (material->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
	{
		data.twoSided = twoSided;
	}

	if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
	{
		data.ambientColor.a = opacity;
		data.diffuseColor.a = opacity;
		data.specularColor.a = opacity;
		data.emissiveColor.a = opacity;

		if (opacity < 1.0f)
		{
			data.alphaBlending = true;

			// Activate backface culling allows to avoid
			// cull artifacts when alpha blending is activated
			data.twoSided = 1;

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
			data.alphaBlending = false;
			data.blendMode = -1;
		}
	}

	if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
	{
		data.shininess = shininess;
	}

	if (material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength) == AI_SUCCESS)
	{
		data.shininessStrength = shininessStrength;
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
	if (material->GetTextureCount(aiTextureType_DIFFUSE)) textureTypes .push_back( aiTextureType_DIFFUSE);
	if (material->GetTextureCount(aiTextureType_SPECULAR)) textureTypes .push_back( aiTextureType_SPECULAR);
	if (material->GetTextureCount(aiTextureType_AMBIENT)) textureTypes .push_back( aiTextureType_AMBIENT);
	if (material->GetTextureCount(aiTextureType_EMISSIVE)) textureTypes .push_back( aiTextureType_EMISSIVE);
	if (material->GetTextureCount(aiTextureType_HEIGHT)) textureTypes .push_back( aiTextureType_HEIGHT);
	if (material->GetTextureCount(aiTextureType_NORMALS)) textureTypes .push_back( aiTextureType_NORMALS);
	if (material->GetTextureCount(aiTextureType_SHININESS)) textureTypes .push_back( aiTextureType_SHININESS);
	if (material->GetTextureCount(aiTextureType_OPACITY)) textureTypes .push_back( aiTextureType_OPACITY);
	if (material->GetTextureCount(aiTextureType_DISPLACEMENT)) textureTypes .push_back( aiTextureType_DISPLACEMENT);
	if (material->GetTextureCount(aiTextureType_LIGHTMAP)) textureTypes .push_back( aiTextureType_LIGHTMAP);
	if (material->GetTextureCount(aiTextureType_REFLECTION)) textureTypes .push_back( aiTextureType_REFLECTION);
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

			if (type == aiTextureType_DIFFUSE)
			{
				data.diffuseMap = texturePath;
				m_modelFeatures.hasColorMap = true;
			}
			else if (type == aiTextureType_NORMALS)
			{
				data.normalMap = texturePath;
				m_modelFeatures.hasNormalMap = true;
			}
			else if (type == aiTextureType_SPECULAR)
			{
				data.specularMap = texturePath;
			}
		}
	}

	return data;
}

void ModelLoader::countChildren(aiNode* pNode)
{
	if (!pNode) return;

	++m_NumNodes;

	for (size_t i = 0; i < pNode->mNumChildren; ++i)
	{
		countChildren(pNode->mChildren[i]);
	}
}
