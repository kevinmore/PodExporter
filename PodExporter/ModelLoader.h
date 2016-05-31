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
};


struct TextureData
{
	string diffuseMap, normalMap, specularMap;

	map<aiTextureType, string> textureMap;
};

struct MaterialData
{
	string name;

	color4D ambientColor;
	color4D diffuseColor;
	color4D specularColor;
	color4D emissiveColor;

	float shininess;
	float shininessStrength;

	int twoSided;
	int blendMode;
	bool alphaBlending;

	TextureData textureData;
};

struct ModelData
{
	MeshData     meshData;
	MaterialData materialData;
	bool hasAnimation;
	float animationDuration;
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
	double getAnimationDuration() { return m_animationDuration; }
	const aiScene* getScene() { return m_aiScene; }

private:
	/*
	*	Methods to process the model file
	*/
	MeshData     loadMesh(unsigned int index, unsigned int numVertices, unsigned int numIndices, const aiMesh* mesh);
	MaterialData loadMaterial(unsigned int index, const aiMaterial* material);
	TextureData  loadTexture(const aiMaterial* material);
	void loadBones(uint MeshIndex, const aiMesh* paiMesh);
	void prepareVertexContainers(unsigned int index, const aiMesh* mesh);
	void generateSkeleton(aiNode* pAiRootNode, Bone* pRootSkeleton, mat4& parentTransform);
	string getMeshNameFromNode(unsigned int meshIndex, aiNode* pNode);
	aiNode* getNodeFromMeshName(const char* meshName, vector<aiNode*>& source);
	void parseNoneMeshNodes(aiNode* pNode);

	/*
	*	Clean up
	*/
	void clear();

	/*
	*	Vertex Data Containers
	*/
	vector<vec3> m_positions;
	vector<color4D> m_colors;
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
	uint m_NumBones;
	vector<Bone> m_BoneInfo;
	mat4 m_GlobalInverseTransform;
	double m_animationDuration;
};

