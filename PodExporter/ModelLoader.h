#pragma once
#include "Skeleton.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

struct MeshData
{
	string name;

	unsigned int numVertices;
	unsigned int numFaces;
	unsigned int numIndices;
	unsigned int baseVertex;
	unsigned int baseIndex;
	mat4 unpackMatrix;
};


struct TextureData
{
	map<aiTextureType, string> texturesMap;
};

struct MaterialData
{
	string name;

	color4D ambientColor;
	color4D diffuseColor;
	color4D specularColor;

	float opacity;
	float shininess;

	int blendMode;

	TextureData textureData;
};

struct ModelData
{
	MeshData     meshData;
	MaterialData materialData;
};

typedef shared_ptr<MeshData> MeshDataPtr;
typedef shared_ptr<TextureData> TextureDataPtr;
typedef shared_ptr<MaterialData> MaterialDataPtr;
typedef shared_ptr<ModelData> ModelDataPtr;
typedef unsigned short uint16;

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();

	enum LoadingQuality
	{
		SIMPLE,
		FAST,
		QUALITY,
		MAX_QUALITY
	};

	/*
	*	This is the core functionality
	*/
	vector<ModelDataPtr> loadModel(const string& filename, LoadingQuality flag = MAX_QUALITY);
	string& getFileNmae() { return m_fileName; }
	const Skeleton* getSkeleton() { return m_skeleton; }
	vector<uint16>& geIndexBuffer() { return m_indices; }
	vector<vec3>& getPositionBuffer() { return m_positions; }
	vector<vec3>& getNormalBuffer() { return m_normals; }
	vector<vec3>& getTangetBuffer() { return m_tangents; }
	vector<vec2>& getUVBuffer() { return m_texCoords; }
	vector<VertexBoneData>& getBoneBuffer() { return m_Bones; }
	vector<Bone>& getBoneList() { return m_BoneInfo; }
	vector<aiNode*>& getNodeList() { return m_Nodes; }
	uint getNumNodes() { return m_Nodes.size();  }
	uint getNumTextures() { return m_texturePaths.size(); }
	const aiScene* getScene() { return m_aiScene; }
	string& getTexture(uint index) { return m_texturePaths[index]; }

private:
	/*
	*	Methods to process the model file
	*/
	MeshData     loadMesh(unsigned int index, unsigned int baseVertex, unsigned int baseIndex);
	MaterialData loadMaterial(const aiMaterial* material);
	TextureData  loadTexture(const aiMaterial* material);
	void loadBones(uint MeshIndex, const aiMesh* paiMesh);
	void prepareVertexContainers(unsigned int index, const aiMesh* mesh);
	void generateSkeleton(aiNode* pAiRootNode, Bone* pRootSkeleton, mat4& parentTransform);
	string getMeshNameFromNode(unsigned int meshIndex, aiNode* pNode);
	aiNode* getNodeFromMeshName(const char* meshName, vector<aiNode*>& source);
	void parseNoneMeshNodes(aiNode* pNode);
	mat4 calculateGlobalTransform(aiNode* pNode);

	/*
	*	Clean up
	*/
	void clear();

	/*
	*	Vertex Data Containers
	*/
	vector<vec3> m_positions;
	vector<vec2> m_texCoords;
	vector<vec3> m_normals;
	vector<vec3> m_tangents;
	vector<uint16> m_indices;

	/*
	*	Members Variables
	*/
	string m_fileName;
	// make the importer as member valuable, so that it does not call FreeScene until the class is destructed
	Assimp::Importer m_importer;
	const aiScene* m_aiScene;
	Skeleton* m_skeleton;
	map<string, uint> m_BoneMapping; // maps a bone name to its index
	vector<aiNode*> m_Nodes;
	vector<aiNode*> m_subMeshNodes;
	vector<VertexBoneData> m_Bones;
	vector<string> m_texturePaths;
	uint m_NumBones;
	vector<Bone> m_BoneInfo;
	mat4 m_GlobalInverseTransform;
};

