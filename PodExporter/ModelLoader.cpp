#include "ModelLoader.h"
#include "AnimationHelper.h"
#include <fstream>
#include <sstream>
#include <assimp/postprocess.h>

void ModelLoader::clear()
{
	m_texturePaths.clear();

	// delete the nodes that we created for the sub meshes
// 	for each (aiNode* var in m_Nodes)
// 	{
// 		std::string name = std::string(var->mName.C_Str());
// 		if (name.find("-submesh") != std::string::npos)
// 		{
// 			SAFE_DELETE(var);
// 		}
// 	}
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

	switch (flag)
	{
	case ModelLoader::SIMPLE:
		flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
		break;
	case ModelLoader::FAST:
		flags = aiProcessPreset_TargetRealtime_Fast;
		break;
	case ModelLoader::QUALITY:
		flags = aiProcessPreset_TargetRealtime_Quality;
		break;
	case ModelLoader::MAX_QUALITY:
		flags = aiProcessPreset_TargetRealtime_MaxQuality;
		break;
	default:
		break;
	}

	m_aiScene = m_importer.ReadFile(fileName, flags);
	if (!m_aiScene)
	{
		cout << m_importer.GetErrorString();
		vector<ModelDataPtr> empty;
		return empty;
	}

	// deal with the embedded textures
	extractEmbeddedTextures();

	//displaySceneGraph(m_aiScene->mRootNode);

	m_GlobalInverseTransform = m_aiScene->mRootNode->mTransformation;
	m_GlobalInverseTransform.Inverse();
	
	m_modelDataVector.resize(m_aiScene->mNumMeshes);

	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		ModelDataPtr md(new ModelData());
		m_modelDataVector[i] = ModelDataPtr(md);

		md->meshData = loadMesh(i);
		md->materialData = loadMaterial(m_aiScene->mMaterials[m_aiScene->mMeshes[i]->mMaterialIndex]);

		aiNode* meshNode = getNode(md->meshData.name.c_str(), m_subMeshNodes);
		
		if (!meshNode)
		{
			meshNode = m_aiScene->mRootNode->FindNode(md->meshData.name.c_str());
		}

		m_Nodes.push_back(meshNode);
	}

 	parseLightNodes();
	parseCameraNodes();

	// parse other nodes, this step makes sure the mesh nodes are in the front
	parseOtherNodes(m_aiScene->mRootNode);

	// load bones
	for (uint i = 0; i < m_aiScene->mNumMeshes; ++i)
	{
		aiMesh* mesh = m_aiScene->mMeshes[i];
		if (mesh->HasBones())
		{
			loadBones(i, m_modelDataVector[i]->meshData);
		}
	}

	// IMPORTANT: this step is essential, make sure the model is not in bind pose, otherwise the animation will be broken
	// apply the first frame pose
	applyPoseAtFrame(0);

	//parseExtraNodes(m_aiScene->mRootNode);
	//collectExtraNodeAnimations();


	return m_modelDataVector;
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
				meshNode->mParent = pNode;
				meshNode->mMeshes = new unsigned int[1]{ meshIndex };
				meshNode->mTransformation = pNode->mTransformation;
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
	// Populate the index buffer
	vector<unsigned int> unSupportedIndices;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		const aiFace& face = mesh->mFaces[i];

		if (face.mNumIndices != 3)
		{
			cout << "Unsupported number of indices per face " << face.mNumIndices << endl;

			for (unsigned int j = 0; j < face.mNumIndices; ++j)
			{
				unSupportedIndices.push_back(face.mIndices[j]);
			}
		}
		else
		{
			data.indices.push_back((unsigned int)face.mIndices[0]);
			data.indices.push_back((unsigned int)face.mIndices[1]);
			data.indices.push_back((unsigned int)face.mIndices[2]);
		}
	}

	// Populate the vertex attribute vectors
	for (uint i = 0; i < mesh->mNumVertices; ++i)
	{
		// pass the unsupported vertices
		bool skip = false;
		for (uint j = 0; j < unSupportedIndices.size(); ++j)
		{
			if (i == unSupportedIndices[j])
			{
				skip = true;
				break;
			}
		}

		if(skip) continue;

		data.positions.push_back(mesh->mVertices[i]);

		if (mesh->HasNormals())
		{
			data.normals.push_back(mesh->mNormals[i]);
		}

		if (mesh->HasTangentsAndBitangents())
		{
			data.tangents.push_back(mesh->mTangents[i]);
			data.bitangents.push_back(mesh->mBitangents[i]);
		}

		if (mesh->HasTextureCoords(0))
		{
			vec3 texCoord = mesh->mTextureCoords[0][i];
			data.texCoords.push_back(vec2(texCoord.x, texCoord.y));
		}

		if (mesh->HasVertexColors(0))
		{
			color4D color = mesh->mColors[0][i];
			data.colors.push_back(color);
		}
	}


	data.numIndices = data.indices.size();
	data.numFaces = data.indices.size() / 3;
	data.numVertices = data.positions.size();
}

void ModelLoader::loadBones(unsigned int index, MeshData& data)
{
	aiMesh* paiMesh = m_aiScene->mMeshes[index];
	data.bones.resize(m_modelDataVector[index]->meshData.numVertices);

	for (uint i = 0; i < paiMesh->mNumBones; ++i)
	{
		unsigned short boneIndex = 0;
		string boneName(paiMesh->mBones[i]->mName.data);
		if (m_BoneMapping.find(boneName) == m_BoneMapping.end())
		{
			// Allocate an index for a new bone
			aiNode* boneNode = getNode(boneName.c_str(), m_Nodes);
			auto it = std::find(m_Nodes.begin(), m_Nodes.end(), boneNode);
			boneIndex = std::distance(m_Nodes.begin(), it);
			m_BoneMapping[boneName] = boneIndex;

			m_BoneOffsetMatrixMapping[boneName] = paiMesh->mBones[i]->mOffsetMatrix;
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

MeshData ModelLoader::loadMesh(unsigned int index)
{
	MeshData data;
	aiMesh* mesh = m_aiScene->mMeshes[index];

	data.name = getMeshNameFromNode(index, m_aiScene->mRootNode);
	
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

	for each(aiTextureType type in textureTypes)
	{
		if (material->GetTexture(type, 0, &path) == AI_SUCCESS)
		{
			if (strlen(path.data) == 0) continue;

			// validate if the texture file exist
			string texturePath = path.data;
			fstream f(texturePath);
			if ((!f.good() || m_embeddedTexutreNames.size() > 0) && texturePath[0] == '*')
			{
				string textureIndex = texturePath.substr(1, texturePath.length());
				texturePath = m_embeddedTexutreNames[stoi(textureIndex)];
			}

			// absolute path
			//string texturePath = directory + "/" + path.data;

			data.texturesMap[type] = texturePath;
			m_texturePaths.push_back(texturePath);
		}
	}

	return data;
}

void ModelLoader::parseOtherNodes(aiNode* pNode, uint index/* = 0*/)
{
	if (!pNode) return;

	// if it's the root node, check if it holds an identity transformation matrix
	bool isUselessNode = false;
	if (pNode == m_aiScene->mRootNode && pNode->mTransformation.IsIdentity())
	{
		isUselessNode = true;
	}

	// ignore the node with 0 or multiple meshes
	if (!isUselessNode && pNode->mNumMeshes != 1)
	{
		if(std::find(m_Nodes.begin(), m_Nodes.end(), pNode) == m_Nodes.end())
			m_Nodes.push_back(pNode);
	}

	for (size_t i = 0; i < pNode->mNumChildren; ++i)
	{
		parseOtherNodes(pNode->mChildren[i], i);
	}
}

void ModelLoader::parseLightNodes()
{
	if (m_aiScene->HasLights())
	{
		for (uint i = 0; i < m_aiScene->mNumLights; ++i)
		{
			aiLight* pLight = m_aiScene->mLights[i];

			// skip the ambient light, because pod does not support it
			// store it as the scene ambient color
			if (pLight->mType == aiLightSource_AMBIENT)
			{
				m_sceneAmbientColor = pLight->mColorAmbient;
			}
			else if(pLight->mType == aiLightSource_POINT || pLight->mType == aiLightSource_DIRECTIONAL || pLight->mType == aiLightSource_SPOT)
			{
				// get the node
				aiNode* pLightNode = m_aiScene->mRootNode->FindNode(pLight->mName);
				m_Nodes.push_back(pLightNode);
			}
		}
	}
}

void ModelLoader::parseCameraNodes()
{
	if (m_aiScene->HasCameras())
	{
		for (uint i = 0; i < m_aiScene->mNumCameras; ++i)
		{
			aiCamera* pCamera = m_aiScene->mCameras[i];

			// get the node
			aiNode* pCameraNode = m_aiScene->mRootNode->FindNode(pCamera->mName);
			m_Nodes.push_back(pCameraNode);
		}
	}
}

void ModelLoader::parseExtraNodes(aiNode* pNode)
{
	if (!pNode) return;
	// if the node is an assimp generated intermediate nodes
	if (isExtraNode(pNode) && std::find(m_parsedExtraNodes.begin(), m_parsedExtraNodes.end(), pNode) == m_parsedExtraNodes.end())
	{
		aiNode* cleanNode = applyChildTransform(pNode);

		cout << cleanNode->mName.C_Str() << endl;
		DecomposeAndDisplayMat4(cleanNode->mTransformation);

		// remove the intermediate nodes
		//cleanNode->mParent = pNode->mParent;
		//pNode->mParent->mChildren[index] = cleanNode;
	}

	for (size_t i = 0; i < pNode->mNumChildren; ++i)
	{
		parseExtraNodes(pNode->mChildren[i]);
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
	mat4 result = pNode->mTransformation;
	while (pNode->mParent)
	{
		result = pNode->mParent->mTransformation * result;
		pNode = pNode->mParent;
	}

	return result;
}

aiNode* ModelLoader::applyChildTransform(aiNode* pNode, mat4& parentTransform)
{
	pNode->mTransformation = parentTransform * pNode->mTransformation;
	m_parsedExtraNodes.push_back(pNode);
	aiNode* result = NULL;

	if (isExtraNode(pNode))
	{
		for (uint i = 0; i < pNode->mNumChildren; ++i)
		{
			result = applyChildTransform(pNode->mChildren[i], pNode->mTransformation);
		}
	}
	else
	{
		result = pNode;
	}
	return result;
}

void ModelLoader::displaySceneGraph(aiNode* pNode, uint indent /*= 0*/)
{
	if(indent > 0)
		cout << "|";
	for (uint i = 0; i < indent; ++i)
		cout << "-";

	cout << pNode->mName.C_Str() << endl;

	for (uint i = 0; i < pNode->mNumChildren; ++i)
		displaySceneGraph(pNode->mChildren[i], indent + 1);
}

bool ModelLoader::isExtraNode(aiNode* pNode)
{
	std::string name = std::string(pNode->mName.C_Str());
	return isExtraNode(name);
}

bool ModelLoader::isExtraNode(string& nodeName)
{
	return nodeName.find("_$AssimpFbx$_") != std::string::npos;
}

aiNode* ModelLoader::getTrueParentNode(aiNode* pNode)
{
	aiNode* temp = pNode->mParent;
	while (isExtraNode(temp))
	{
		temp = temp->mParent;
	}
	return temp;
}

void ModelLoader::collectExtraNodeAnimations()
{
	if (!m_aiScene->HasAnimations())
		return;

	for (uint i = 0; i < m_aiScene->mNumAnimations; ++ i)
	{
		for (uint j = 0; j < m_aiScene->mAnimations[i]->mNumChannels; ++j)
		{
			aiNodeAnim* a = m_aiScene->mAnimations[i]->mChannels[j];
			std::string name = std::string(a->mNodeName.C_Str());
			if (isExtraNode(name))
			{
				m_extraNodeAnimation[name] = a;
			}
		}
	}

}

void ModelLoader::applyPoseAtFrame(uint frameIndex)
{
	if (m_aiScene->HasAnimations())
	{
		AnimationHelper helper(m_GlobalInverseTransform, m_BoneOffsetMatrixMapping);

		map<string, mat4> boneFinalTransforms = helper.getBoneFinalTransformsAtFrame(frameIndex, m_aiScene->mAnimations[0], m_aiScene->mRootNode);

		for (uint i = 0; i < m_modelDataVector.size(); ++i)
		{
			if (!m_aiScene->mMeshes[i]->HasBones()) continue;

			for (uint j = 0; j < m_modelDataVector[i]->meshData.numVertices; ++j)
			{
				VertexBoneData boneData = m_modelDataVector[i]->meshData.bones[j];

				glm::mat4 finalM(0.0f);
				for (uint k = 0; k < NUM_BONES_PER_VEREX; ++k)
				{
					float weight = boneData.Weights[k];

					if (weight == 0.0f)
						continue;

					std::string boneName(m_Nodes[boneData.IDs[k]]->mName.C_Str());
					glm::mat4 weightedMat = weight * toGLMMatrix4x4(boneFinalTransforms[boneName]);

					finalM += weightedMat;
				}

				vec3 oldPos = m_modelDataVector[i]->meshData.positions[j];
				glm::vec4 newPos = glm::vec4(oldPos.x, oldPos.y, oldPos.z, 1.0f) * finalM; // THE ORDER MATTERS!
				m_modelDataVector[i]->meshData.positions[j] = vec3(newPos.x, newPos.y, newPos.z);

				vec3 oldNormal = m_modelDataVector[i]->meshData.normals[j];
				glm::vec4 newNormal = glm::vec4(oldNormal.x, oldNormal.y, oldNormal.z, 0.0f) * finalM;
				m_modelDataVector[i]->meshData.normals[j] = vec3(newNormal.x, newNormal.y, newNormal.z);

				vec3 oldTagent = m_modelDataVector[i]->meshData.tangents[j];
				glm::vec4 newTagent = glm::vec4(oldTagent.x, oldTagent.y, oldTagent.z, 0.0f) * finalM;
				m_modelDataVector[i]->meshData.tangents[j] = vec3(newTagent.x, newTagent.y, newTagent.z);

				vec3 oldBiTagent = m_modelDataVector[i]->meshData.bitangents[j];
				glm::vec4 newBiTagent = glm::vec4(oldBiTagent.x, oldBiTagent.y, oldBiTagent.z, 0.0f) * finalM;
				m_modelDataVector[i]->meshData.bitangents[j] = vec3(newBiTagent.x, newBiTagent.y, newBiTagent.z);
			}
		}
	}
}

string ModelLoader::getEmbeddedTextureName(uint textureIndex)
{
	stringstream ss;
	ss << textureIndex;

	string nameWithoutExtension;
	const size_t last_idx = m_fileName.rfind('.');
	if (std::string::npos != last_idx)
	{
		nameWithoutExtension = m_fileName.substr(0, last_idx);
	}

	return nameWithoutExtension + "_EmbeddedTexture_" + ss.str() ;
}

string ModelLoader::getEmbeddedTextureName(string& textureIndex)
{
	string nameWithoutExtension;
	const size_t last_idx = m_fileName.rfind('.');
	if (std::string::npos != last_idx)
	{
		nameWithoutExtension = m_fileName.substr(0, last_idx);
	}

	return nameWithoutExtension + "_EmbeddedTexture_" + textureIndex;
}

void ModelLoader::extractEmbeddedTextures()
{
	if (m_aiScene->HasTextures())
	{
		for (uint i = 0; i < m_aiScene->mNumTextures; ++i)
		{
			aiTexture* tex = m_aiScene->mTextures[i];

			bool isCompressed = tex->mHeight == 0;

			// if the texture is compressed, (jpg, png...), the data is the raw file bytes
			if (isCompressed)
			{
				string textureName = getEmbeddedTextureName(i) + "." + string(tex->achFormatHint);
				m_embeddedTexutreNames.push_back(textureName);
				fstream f = fstream(textureName, ios::binary | ios::out | ios::trunc);
				if (f.is_open())
				{
					f.write(reinterpret_cast<const char *>(tex->pcData), tex->mWidth);
					f.flush();
					f.close();
				}
				else
				{
					cout << "\nCannot write out embedded texture";
				}
			}
			else
			{
				// use free image to create an image
				cout << "uncompressed embedded texture out put not implemented yet.";
			}
		}
	}
}
