#pragma once
#include "Common.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#define NUM_BONES_PER_VEREX 4
using namespace std;

struct VertexBoneData
{
	unsigned short IDs[NUM_BONES_PER_VEREX];
	float Weights[NUM_BONES_PER_VEREX];

	VertexBoneData()
	{
		Reset();
	};

	void Reset()
	{
		ZERO_MEM(IDs);
		ZERO_MEM(Weights);
	}

	void AddBoneData(unsigned short BoneID, float Weight)
	{
		for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); ++i)
		{
			if (Weights[i] == 0.0)
			{
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}
		}

		// should never get here - more bones than we have space for
		assert(0);
	}
};

struct MeshData
{
	string name;

	unsigned int numVertices;
	unsigned int numFaces;
	unsigned int numIndices;
	unsigned int baseVertex;
	unsigned int baseIndex;
	mat4 unpackMatrix;

	/*
	*	Vertex Data Containers
	*/
	vector<vec3> positions;
	vector<vec2> texCoords;
	vector<vec3> normals;
	vector<vec3> tangents;
	vector<vec3> bitangents;
	vector<color4D> colors;
	vector<unsigned int> indices;
	vector<VertexBoneData> bones;
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

	vector<ModelDataPtr> loadModel(const string& filename, LoadingQuality flag = MAX_QUALITY);

	string& getFileNmae() { return m_fileName; }
	vector<aiNode*>& getNodeList() { return m_Nodes; }
	uint getNumNodes() { return m_Nodes.size();  }
	uint getNumTextures() { return m_texturePaths.size(); }
	const aiScene* getScene() { return m_aiScene; }
	string& getTexture(uint index) { return m_texturePaths[index]; }
	map<string, unsigned short>& getBoneMap() { return m_BoneMapping; }
	map<string, mat4>& getBoneOffsetMatrixMap() { return m_BoneOffsetMatrixMapping; }
private:
	/*
	*	Methods to process the model file
	*/
	MeshData     loadMesh(unsigned int index, unsigned int baseVertex, unsigned int baseIndex);
	MaterialData loadMaterial(const aiMaterial* material);
	TextureData  loadTexture(const aiMaterial* material);
	void loadBones(const aiMesh* paiMesh, MeshData& data);
	void readVertexAttributes(unsigned int index, const aiMesh* mesh, MeshData& data);
	string getMeshNameFromNode(unsigned int meshIndex, aiNode* pNode);
	aiNode* getNode(const char* meshName, vector<aiNode*>& source);
	void parseNoneMeshNodes(aiNode* pNode);
	mat4 calculateGlobalTransform(aiNode* pNode);
	/*
	*	Clean up
	*/
	void clear();

	/*
	*	Members Variables
	*/
	string m_fileName;
	// make the importer as member valuable, so that it does not call FreeScene until the class is destructed
	Assimp::Importer m_importer;
	const aiScene* m_aiScene;
	mat4 m_GlobalInverseTransform;
	map<string, unsigned short> m_BoneMapping; // maps a bone name to its index
	map<string, mat4> m_BoneOffsetMatrixMapping; // maps a bone name to its offset matrix
	vector<aiNode*> m_Nodes;
	vector<aiNode*> m_subMeshNodes;
	vector<string> m_texturePaths;
};

