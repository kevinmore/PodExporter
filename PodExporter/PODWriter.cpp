/************************************************************************/
/* The reason using std::fstream instead of pvr::FileStream is that
pvr::FileStream is a text stream, on Windows Systems, it translate byte '\n'
to '\r\n', which produces one more byte than expected.                  */
/************************************************************************/

#include "PODWriter.h"
#include "PVRAssets/FileIO/PODDefines.h"
#include "PVRAssets/Model.h"
#include "PVRCore/StringHash.h"
#include "PVRCore/PVRCore.h"
#include <cstdio>
#include <algorithm>

namespace { // LOCAL FUNCTIONS
using namespace pvr;
using namespace assets;
using namespace pvr::types;

template <typename T>
void writeBytes(fstream& stream, T& data, streamsize size = 0)
{
	if (size > 0)
		stream.write(reinterpret_cast<const char *>(&data), size);
	else
		stream.write(reinterpret_cast<const char *>(&data), sizeof(T));
}

template <typename T>
void writeByteArray(fstream& stream, T* data, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		writeBytes(stream, data[i]);
	}
}

template <typename T>
void write4Bytes(fstream& stream, T& data)
{
	writeBytes(stream, data, 4);
}

template <typename T>
void write4ByteArray(fstream& stream, T* data, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		write4Bytes(stream, data[i]);
	}
}

template <typename T>
void write2Bytes(fstream& stream, T& data)
{
	writeBytes(stream, data, 2);
}

template <typename T>
void write2ByteArray(fstream& stream, T* data, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		write2Bytes(stream, data[i]);
	}
}

template <typename T>
void writeByteArrayFromVector(fstream& stream, vector<T>& data)
{
	writeByteArray<T>(stream, data.data(), data.size());
}

template <typename T>
void write2ByteArrayFromVector(fstream& stream, vector<T>& data)
{
	write2ByteArray<T>(stream, data.data(), data.size());
}

template <typename T>
void write4ByteArrayFromVector(fstream& stream, vector<T>& data)
{
	write4ByteArray<T>(stream, data.data(), data.size());
}

void writeByteArrayFromeString(fstream& stream, std::string& data)
{
	writeByteArray(stream, data.c_str(), data.length());

	// write the null terminator
	const char* end = "\0";
	writeBytes(stream, *end, 1);
}

void writeByteArrayFromeStringHash(fstream& stream, StringHash& data)
{
	writeByteArray(stream, data.c_str(), data.length());

	// write the null terminator
	const char* end = "\0";
	writeBytes(stream, *end, 1);
}

void writeTag(fstream& stream, uint32 tagMask, uint32 identifier, uint32 dataLength)
{
	uint32 halfTag = identifier | tagMask;
	write4Bytes(stream, halfTag);
	write4Bytes(stream, dataLength);
}

template <typename T>
void writeVertexIndexData(fstream& stream, std::vector<T>& data)
{
	// write block data type (UInt32 or UInt16)
	writeTag(stream, pod::c_startTagMask, pod::e_blockDataType, 4);
	DataType::Enum type;
	switch (sizeof(T))
	{
	case 2:
		type = DataType::UInt16;
		break;
	case 4:
		type = DataType::UInt32;
		break;
	}
	write4Bytes(stream, type);
	writeTag(stream, pod::c_endTagMask, pod::e_blockDataType, 0);

	// write the block data buffer
	writeTag(stream, pod::c_startTagMask, pod::e_blockData, data.size() * sizeof(T));
	switch (sizeof(T))
	{
	case 2:
		write2ByteArrayFromVector<T>(stream, data);
		break;
	case 4:
		write4ByteArrayFromVector<T>(stream, data);
		break;
	}
	writeTag(stream, pod::c_endTagMask, pod::e_blockData, 0);
}

template <typename T>
void writeVertexData(fstream& stream, DataType::Enum type, uint32 numComponents, uint32 stride, std::vector<T>& data)
{
	// write block data type
	writeTag(stream, pod::c_startTagMask, pod::e_blockDataType, 4);
	write4Bytes(stream, type);
	writeTag(stream, pod::c_endTagMask, pod::e_blockDataType, 0);

	// write num components (e.g. position buffer, it has x, y, z, so numComponents = 3)
	writeTag(stream, pod::c_startTagMask, pod::e_blockNumComponents, 4);
	write4Bytes(stream, numComponents);
	writeTag(stream, pod::c_endTagMask, pod::e_blockNumComponents, 0);

	// write stride: The distance, in bytes, from one array member to the next. 
	writeTag(stream, pod::c_startTagMask, pod::e_blockStride, 4);
	write4Bytes(stream, stride);
	writeTag(stream, pod::c_endTagMask, pod::e_blockStride, 0);

	// write the block data buffer
	writeTag(stream, pod::c_startTagMask, pod::e_blockData, data.size() * sizeof(T));

	switch (DataType::size(type))
	{
	case 1:
		writeByteArrayFromVector<T>(stream, data);
		break;
	case 2:
		write2ByteArrayFromVector<T>(stream, data);
		break;
	case 4:
		write4ByteArrayFromVector<T>(stream, data);
		break;
	}

	writeTag(stream, pod::c_endTagMask, pod::e_blockData, 0);
}

void writeDummyVertexData(fstream& stream, DataType::Enum type, uint32 numComponents, uint32 stride, uint32 offset)
{
	// write block data type
	writeTag(stream, pod::c_startTagMask, pod::e_blockDataType, 4);
	write4Bytes(stream, type);
	writeTag(stream, pod::c_endTagMask, pod::e_blockDataType, 0);

	// write num components (e.g. position buffer, it has x, y, z, so numComponents = 3)
	writeTag(stream, pod::c_startTagMask, pod::e_blockNumComponents, 4);
	write4Bytes(stream, numComponents);
	writeTag(stream, pod::c_endTagMask, pod::e_blockNumComponents, 0);

	// write stride: The distance, in bytes, from one array member to the next. 
	writeTag(stream, pod::c_startTagMask, pod::e_blockStride, 4);
	write4Bytes(stream, stride);
	writeTag(stream, pod::c_endTagMask, pod::e_blockStride, 0);

	// write the block data buffer (here, we write the offset of this vertex attribute)
	writeTag(stream, pod::c_startTagMask, pod::e_blockData, 4);
	write4Bytes(stream, offset);
	writeTag(stream, pod::c_endTagMask, pod::e_blockData, 0);
}

template <typename T>
void addByteIntoVector(T& data, vector<byte>& targetVector)
{
	byte *temp;
	temp = reinterpret_cast<byte*>(&data);
	for (uint i = 0; i < sizeof(T); ++i)
		targetVector.push_back(temp[i]);
}

}

namespace pvr {
namespace assets {
namespace assetWriters {


PODWriter::PODWriter(ModelLoader& loader)
	: m_modelLoader(loader)
	, m_Nodes(loader.getNodeList())
	, m_exportAnimations(true)
{
}

void PODWriter::exportModel(const std::string& path, bool exportAnimations)
{
	m_exportAnimations = exportAnimations;

	m_fileStream = fstream(path, ios::binary | ios::out | ios::trunc);

	if (m_fileStream.is_open())
	{
		writeAllAssets();
		m_fileStream.flush();
		m_fileStream.close();
	}
	else
	{
		cout << "\nCannot open file: " << path;
	}

}

bool PODWriter::addAssetToWrite(const Model & asset)
{
	if (m_assetsToWrite.size() >= 1)
	{
		return false;
	}
	m_assetsToWrite.push_back(&asset);
	return true;
}

bool PODWriter::writeAllAssets()
{
	// write pod version block
	writeStartTag(pod::PODFormatVersion, pod::c_PODFormatVersionLength);
	writeByteArray(m_fileStream, pod::c_PODFormatVersion, pod::c_PODFormatVersionLength);
	writeEndTag(pod::PODFormatVersion);

	// write scene block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::Scene, 0);
	writeSceneBlock();
	writeEndTag(pod::Scene);

	return true;
}

uint32 PODWriter::assetsAddedSoFar()
{
	return (uint32)m_assetsToWrite.size();
}

bool PODWriter::supportsMultipleAssets()
{
	return false;
}

bool PODWriter::canWriteAsset(const Model& asset)
{
	return true;
}

vector<string> PODWriter::getSupportedFileExtensions()
{
	vector<string> extensions;
	extensions.push_back("pod");
	return vector<string>(extensions);
}

string PODWriter::getWriterName()
{
	return "PowerVR assets::Model Writer";
}

string PODWriter::getWriterVersion()
{
	return "1.0.0";
}

void PODWriter::writeStartTag(uint32 identifier, uint32 dataLength)
{
	writeTag(m_fileStream, pod::c_startTagMask, identifier, dataLength);
}

void PODWriter::writeEndTag(uint32 identifier)
{
	writeTag(m_fileStream, pod::c_endTagMask, identifier, 0);
}

void PODWriter::writeSceneBlock()
{
	// unit
	float32 units = 1.0f;
	writeStartTag(pod::e_sceneUnits, 4);
	write4Bytes(m_fileStream, units);
	writeEndTag(pod::e_sceneUnits);

	// Clear Colour 
	float32	clearColor[3] = { 0.f, 0.f, 0.f };
	writeStartTag(pod::e_sceneClearColor, 3 * sizeof(float32));
	write4ByteArray(m_fileStream, &clearColor[0], 3);
	writeEndTag(pod::e_sceneClearColor);

	// Ambient Colour 
	float32	ambientColor[3] = { 0.5f, 0.5f, 0.5f };
	writeStartTag(pod::e_sceneAmbientColor, 3 * sizeof(float32));
	write4ByteArray(m_fileStream, &ambientColor[0], 3);
	writeEndTag(pod::e_sceneAmbientColor);

	// Num. Cameras (not supported yet)
	uint32 numCam = 0;
	writeStartTag(pod::e_sceneNumCameras, 4);
	write4Bytes(m_fileStream, numCam);
	writeEndTag(pod::e_sceneNumCameras);

	// Num. Lights (not supported yet)
	uint32 numLights = 0;
	writeStartTag(pod::e_sceneNumLights, 4);
	write4Bytes(m_fileStream, numLights);
	writeEndTag(pod::e_sceneNumLights);

	// Num. Meshes
	uint32 numMeshes = m_modelDataVec.size();
	writeStartTag(pod::e_sceneNumMeshes, 4);
	write4Bytes(m_fileStream, numMeshes);
	writeEndTag(pod::e_sceneNumMeshes);

	// Num. Nodes
	uint32 numNodes = m_modelLoader.getNumNodes();
	writeStartTag(pod::e_sceneNumNodes, 4);
	write4Bytes(m_fileStream, numNodes);
	writeEndTag(pod::e_sceneNumNodes);

	// Num. Mesh Nodes
	uint32 numMeshNodes = m_modelDataVec.size();
	writeStartTag(pod::e_sceneNumMeshNodes, 4);
	write4Bytes(m_fileStream, numMeshNodes);
	writeEndTag(pod::e_sceneNumMeshNodes);

	// Num. Textures
	uint32 numTextures = m_modelLoader.getNumTextures();
	writeStartTag(pod::e_sceneNumTextures, 4);
	write4Bytes(m_fileStream, numTextures);
	writeEndTag(pod::e_sceneNumTextures);

	// Num. Materials (1 mesh 1 material)
	uint32 numMaterials = m_modelDataVec.size();
	writeStartTag(pod::e_sceneNumMaterials, 4);
	write4Bytes(m_fileStream, numMaterials);
	writeEndTag(pod::e_sceneNumMaterials);

	// Num. Frames
	uint32 numFrames = (m_modelLoader.getScene()->HasAnimations() && m_exportAnimations) ?
		std::max(std::max(m_modelLoader.getScene()->mAnimations[0]->mChannels[0]->mNumPositionKeys,
			m_modelLoader.getScene()->mAnimations[0]->mChannels[0]->mNumRotationKeys),
			m_modelLoader.getScene()->mAnimations[0]->mChannels[0]->mNumScalingKeys)
		: 0;
	writeStartTag(pod::e_sceneNumFrames, 4);
	write4Bytes(m_fileStream, numFrames);
	writeEndTag(pod::e_sceneNumFrames);

	if (m_modelLoader.getScene()->HasAnimations() && m_exportAnimations)
	{
		// FPS (30 fps by default)
		uint32 fps = 30;
		writeStartTag(pod::e_sceneFPS, 4);
		write4Bytes(m_fileStream, fps);
		writeEndTag(pod::e_sceneFPS);
	}

	// Scene flags (0)
	uint32 flag = 0;
	writeStartTag(pod::e_sceneFlags, 4);
	write4Bytes(m_fileStream, flag);
	writeEndTag(pod::e_sceneFlags);

	// Material Block
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMaterialBlock(i);
	}

	// Mesh Block
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMeshBlock(i);
	}

	// Node Block
	for (uint32 i = 0; i < numNodes; ++i)
	{
		writeNodeBlock(i);
	}

	// Texture Block
	for (uint32 i = 0; i < numTextures; ++i)
	{
		writeTextureBlock(i);
	}
}

void PODWriter::writeMaterialBlock(uint index)
{
	MaterialData matData = m_modelDataVec[index]->materialData;

	// write material block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneMaterial, 0);

	// Material Flags (blending disabled)
	uint32	flags = 0;
	writeStartTag(pod::e_materialFlags, 4);
	write4Bytes(m_fileStream, flags);
	writeEndTag(pod::e_materialFlags);

	// Material Name
	StringHash name(matData.name);
	writeStartTag(pod::e_materialName, name.length() + 1);
	writeByteArrayFromeStringHash(m_fileStream, name);
	writeEndTag(pod::e_materialName);

	// Texture Index
	// calculate the index offset
	uint offset = 0;
	for (uint i = 0; i < index; ++i)
	{
		offset += m_modelDataVec[i]->materialData.textureData.texturesMap.size();
	}

	int32 emptyTextureIndex = -1;

	writeStartTag(pod::e_materialDiffuseTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_DIFFUSE].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialDiffuseTextureIndex);

	writeStartTag(pod::e_materialAmbientTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_AMBIENT].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialAmbientTextureIndex);

	writeStartTag(pod::e_materialSpecularColorTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_SPECULAR].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialSpecularColorTextureIndex);

	writeStartTag(pod::e_materialSpecularLevelTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_HEIGHT].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialSpecularLevelTextureIndex);

	writeStartTag(pod::e_materialBumpMapTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_NORMALS].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialBumpMapTextureIndex);

	writeStartTag(pod::e_materialEmissiveTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_EMISSIVE].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialEmissiveTextureIndex);

	writeStartTag(pod::e_materialGlossinessTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_SHININESS].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialGlossinessTextureIndex);

	writeStartTag(pod::e_materialOpacityTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_OPACITY].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialOpacityTextureIndex);

	writeStartTag(pod::e_materialReflectionTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_REFLECTION].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
	{
		write4Bytes(m_fileStream, emptyTextureIndex);
	}
	writeEndTag(pod::e_materialReflectionTextureIndex);

	// refraction map not supported
	writeStartTag(pod::e_materialRefractionTextureIndex, 4);
	write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialRefractionTextureIndex);

	//Opacity
	writeStartTag(pod::e_materialOpacity, 4);
	write4Bytes(m_fileStream, matData.opacity);
	writeEndTag(pod::e_materialOpacity);

	// Ambient Color
	float32	ambientColor[3] = { matData.ambientColor.r, matData.ambientColor.g, matData.ambientColor.b };
	writeStartTag(pod::e_materialAmbientColor, 3 * sizeof(float32));
	write4ByteArray(m_fileStream, &ambientColor[0], 3);
	writeEndTag(pod::e_materialAmbientColor);

	// Diffuse Color
	float32	diffuseColor[3] = { matData.diffuseColor.r, matData.diffuseColor.g, matData.diffuseColor.b };
	writeStartTag(pod::e_materialDiffuseColor, 3 * sizeof(float32));
	write4ByteArray(m_fileStream, &diffuseColor[0], 3);
	writeEndTag(pod::e_materialDiffuseColor);

	// Specular Color
	float32	specularColor[3] = { matData.specularColor.r, matData.specularColor.g, matData.specularColor.b };
	writeStartTag(pod::e_materialSpecularColor, 3 * sizeof(float32));
	write4ByteArray(m_fileStream, &specularColor[0], 3);
	writeEndTag(pod::e_materialSpecularColor);

	// Shininess
	writeStartTag(pod::e_materialShininess, 4);
	write4Bytes(m_fileStream, matData.shininess);
	writeEndTag(pod::e_materialShininess);

	// Blend Function
	uint32 blendFuncSource, blendFuncDest, blendOperation;
	switch (matData.blendMode)
	{
	case aiBlendMode_Default:
		blendFuncSource = assets::Model::Material::BlendFuncSrcAlpha;
		blendFuncDest = assets::Model::Material::BlendFuncOneMinusDstAlpha;
		blendOperation = assets::Model::Material::BlendOpSubtract;
		break;

	case aiBlendMode_Additive:
		blendFuncSource = assets::Model::Material::BlendFuncOne;
		blendFuncDest = assets::Model::Material::BlendFuncOne;
		blendOperation = assets::Model::Material::BlendOpAdd;
		break;

	default:
		blendFuncSource = assets::Model::Material::BlendFuncOne;
		blendFuncDest = assets::Model::Material::BlendFuncZero;
		blendOperation = assets::Model::Material::BlendOpAdd;
		break;
	}

	// RGBA
	writeStartTag(pod::e_materialBlendingRGBSrc, 4);
	write4Bytes(m_fileStream, blendFuncSource);
	writeEndTag(pod::e_materialBlendingRGBSrc);

	writeStartTag(pod::e_materialBlendingAlphaSrc, 4);
	write4Bytes(m_fileStream, blendFuncSource);
	writeEndTag(pod::e_materialBlendingAlphaSrc);

	writeStartTag(pod::e_materialBlendingRGBDst, 4);
	write4Bytes(m_fileStream, blendFuncDest);
	writeEndTag(pod::e_materialBlendingRGBDst);

	writeStartTag(pod::e_materialBlendingAlphaDst, 4);
	write4Bytes(m_fileStream, blendFuncDest);
	writeEndTag(pod::e_materialBlendingAlphaDst);

	// Blend Operation
	writeStartTag(pod::e_materialBlendingRGBOperation, 4);
	write4Bytes(m_fileStream, blendOperation);
	writeEndTag(pod::e_materialBlendingRGBOperation);

	writeStartTag(pod::e_materialBlendingAlphaOperation, 4);
	write4Bytes(m_fileStream, blendOperation);
	writeEndTag(pod::e_materialBlendingAlphaOperation);

	float32	blendColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	writeStartTag(pod::e_materialBlendingRGBAColor, 4 * sizeof(float32));
	write4ByteArray(m_fileStream, &blendColor[0], 4);
	writeEndTag(pod::e_materialBlendingRGBAColor);

	float32	blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	writeStartTag(pod::e_materialBlendingFactorArray, 4 * sizeof(float32));
	write4ByteArray(m_fileStream, &blendFactor[0], 4);
	writeEndTag(pod::e_materialBlendingFactorArray);

	writeEndTag(pod::e_sceneMaterial);
}

void PODWriter::writeMeshBlock(uint index)
{
	MeshData meshData = m_modelDataVec[index]->meshData;

	// write mesh block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneMesh, 0);

	// Num. Vertices
	writeStartTag(pod::e_meshNumVertices, 4);
	write4Bytes(m_fileStream, (uint32)meshData.numVertices);
	writeEndTag(pod::e_meshNumVertices);

	// Num. Faces
	writeStartTag(pod::e_meshNumFaces, 4);
	write4Bytes(m_fileStream, (uint32)meshData.numFaces);
	writeEndTag(pod::e_meshNumFaces);

	// Num. UVW channels
	uint32 numUVW = 1;
	writeStartTag(pod::e_meshNumUVWChannels, 4);
	write4Bytes(m_fileStream, (uint32)numUVW);
	writeEndTag(pod::e_meshNumUVWChannels);

	// Vertex List (Position)
	vector<vec3> positionBuffer = m_modelLoader.getPositionBuffer();

	// Normal List
	vector<vec3> normalBuffer = m_modelLoader.getNormalBuffer();

	// Tangent List
	vector<vec3> tangentBuffer = m_modelLoader.getTangetBuffer();

	// UVW List
	vector<vec2> uvBuffer = m_modelLoader.getUVBuffer();

	// Bone Weights List
	vector<VertexBoneData> boneBuffer = m_modelLoader.getBoneBuffer();

	// Interleaved Data List
	// Structure: position.xyz + normal.xyz + tangetn.xyz + UV.xy + BoneWeight.xyzw + BoneIndex.xyzw (stride = 64 bytes)
	uint32 stride = (m_modelLoader.getScene()->HasAnimations() && m_exportAnimations) ?
		sizeof(positionBuffer[0]) + sizeof(normalBuffer[0]) + sizeof(tangentBuffer[0]) + sizeof(uvBuffer[0]) + sizeof(boneBuffer[0])
		: sizeof(positionBuffer[0]) + sizeof(normalBuffer[0]) + sizeof(tangentBuffer[0]) + sizeof(uvBuffer[0]);
	writeStartTag(pod::e_meshInterleavedDataList, stride * meshData.numVertices);
	for (uint i = meshData.baseVertex; i < meshData.baseVertex + meshData.numVertices; ++i)
	{
		writeBytes(m_fileStream, positionBuffer[i]);
		writeBytes(m_fileStream, normalBuffer[i]);
		writeBytes(m_fileStream, tangentBuffer[i]);
		writeBytes(m_fileStream, uvBuffer[i]);
		if (m_modelLoader.getScene()->HasAnimations() && m_exportAnimations)
		{
			writeBytes(m_fileStream, boneBuffer[i].Weights);
			writeBytes(m_fileStream, boneBuffer[i].IDs);
		}
	}
 	writeEndTag(pod::e_meshInterleavedDataList);

	if (m_modelLoader.getScene()->HasAnimations() && m_exportAnimations)
	{
		// Max. Num. Bones per Batch 
		//writeStartTag(pod::e_meshMaxNumBonesPerBatch, 4);
		//writeEndTag(pod::e_meshMaxNumBonesPerBatch);

		// Num. Bone Batches 
		//writeStartTag(pod::e_meshNumBoneBatches, 4);
		//writeEndTag(pod::e_meshNumBoneBatches);

		// Bone Batch Index List 
		// A list of indices into the "Node" list, each indexed "Node" representing the transformations associated with a single bone. 
		// (Read via £Bone Index List"). 
		//writeStartTag(pod::e_meshBoneBatchIndexList, 400);
		//writeEndTag(pod::e_meshBoneBatchIndexList);

		// Num. Bone Indices per Batch 
		// A list of integers, each integer representing the number of indices in each batch in the "Bone Batch Index List"
		//writeStartTag(pod::e_meshNumBoneIndicesPerBatch, 8);
		//writeEndTag(pod::e_meshNumBoneIndicesPerBatch);

		// Bone Offset per Batch
		// A list of integers, each integer representing the offset into the "Vertex List", 
		// or "Vertex Index List" of the data is indexed, the batch starts at. 
		//writeStartTag(pod::e_meshBoneOffsetPerBatch, 8);
		//writeEndTag(pod::e_meshBoneOffsetPerBatch);
	}

	// Vertex Index List
	vector<uint16> entireIndexBuffer = m_modelLoader.geIndexBuffer();
	vector<uint16> currentIndexBuffer;
	for (uint i = meshData.baseIndex; i < meshData.baseIndex + meshData.numIndices; ++i)
	{
		currentIndexBuffer.push_back(entireIndexBuffer[i]);
	}
	writeStartTag(pod::e_meshVertexIndexList, sizeof(uint16) * currentIndexBuffer.size());
	writeVertexIndexData<uint16>(m_fileStream, currentIndexBuffer);
	writeEndTag(pod::e_meshVertexIndexList);

	// Dummy Vertex Attribute Lists (as all the vertex data is in the interleaved data list)
	uint32 offset = 0;
	writeStartTag(pod::e_meshVertexList, 0);
	writeDummyVertexData(m_fileStream, DataType::Float32, 3, stride, offset);
	offset += DataType::size(DataType::Float32) * 3;
	writeEndTag(pod::e_meshVertexList);

	writeStartTag(pod::e_meshNormalList, 0);
	writeDummyVertexData(m_fileStream, DataType::Float32, 3, stride, offset);
	offset += DataType::size(DataType::Float32) * 3;
	writeEndTag(pod::e_meshNormalList);

	writeStartTag(pod::e_meshTangentList, 0);
	writeDummyVertexData(m_fileStream, DataType::Float32, 3, stride, offset);
	offset += DataType::size(DataType::Float32) * 3;
	writeEndTag(pod::e_meshTangentList);

	writeStartTag(pod::e_meshUVWList, 0);
	writeDummyVertexData(m_fileStream, DataType::Float32, 2, stride, offset);
	offset += DataType::size(DataType::Float32) * 2;
	writeEndTag(pod::e_meshUVWList);

	if (m_modelLoader.getScene()->HasAnimations() && m_exportAnimations)
	{
		writeStartTag(pod::e_meshBoneWeightList, 0);
		writeDummyVertexData(m_fileStream, DataType::Float32, 4, stride, offset);
		offset += DataType::size(DataType::Float32) * 4;
		writeEndTag(pod::e_meshBoneWeightList);

		writeStartTag(pod::e_meshBoneIndexList, 0);
		writeDummyVertexData(m_fileStream, DataType::UInt8, 4, stride, offset);
		offset += DataType::size(DataType::UInt8) * 4;
		writeEndTag(pod::e_meshBoneIndexList);
	}

	writeEndTag(pod::e_sceneMesh);
}

void PODWriter::writeNodeBlock(uint index)
{
	aiNode* node = m_Nodes[index];

	// write node block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneNode, 0);

	// Node Index
	writeStartTag(pod::e_nodeIndex, 4);
	int32 objectIndex = node->mNumMeshes > 0 ? index : -1;
	write4Bytes(m_fileStream, objectIndex);
	writeEndTag(pod::e_nodeIndex);

	// Node Name
	StringHash name(node->mName.C_Str());
	writeStartTag(pod::e_nodeName, name.length() + 1);
	writeByteArrayFromeStringHash(m_fileStream, name);
	writeEndTag(pod::e_nodeName);

	// Material Index (if the node is a mesh)
	if (node->mNumMeshes == 1)
	{
		writeStartTag(pod::e_nodeMaterialIndex, 4);
		write4Bytes(m_fileStream, node->mMeshes[0]); // need fix
		writeEndTag(pod::e_nodeMaterialIndex);
	}

	// Parent Index 
	aiNode* parent = node->mParent;
	int32 parentIdx = -1;
	for (uint32 i = 0; i < m_Nodes.size(); ++i)
	{
		if (m_Nodes[i] == parent)
		{
			parentIdx = i;
			break;
		}
	}
	writeStartTag(pod::e_nodeParentIndex, 4);
	write4Bytes(m_fileStream, parentIdx); // need fix
	writeEndTag(pod::e_nodeParentIndex);

	// Node Animation
	if (!m_modelLoader.getScene()->HasAnimations() || !m_exportAnimations)
	{
		writeEndTag(pod::e_sceneNode);
		return;
	}

	aiAnimation* anim = m_modelLoader.getScene()->mAnimations[0];
	aiNodeAnim* animation = NULL;
	for (uint i = 0; i <anim->mNumChannels; ++i)
	{
		if (anim->mChannels[i]->mNodeName == node->mName)
		{
			animation = anim->mChannels[i];
			break;
		}
	}

	// Animation transforms
	vector<float32> position, rotation, scale, matrices;

	// Animation Flag
	uint32 flag = 0;
	if (animation)
	{
		if (animation->mNumPositionKeys > 0)
		{
			flag |= 0x01;
			for (uint i = 0; i < animation->mNumPositionKeys; ++i)
			{
				// vec3
				position.push_back(animation->mPositionKeys[i].mValue.x);
				position.push_back(animation->mPositionKeys[i].mValue.y);
				position.push_back(animation->mPositionKeys[i].mValue.z);
			}
		}
		if (animation->mNumRotationKeys > 0)
		{
			flag |= 0x02;
			for (uint i = 0; i < animation->mNumRotationKeys; ++i)
			{
				// quaternion
				position.push_back(animation->mRotationKeys[i].mValue.x);
				position.push_back(animation->mRotationKeys[i].mValue.y);
				position.push_back(animation->mRotationKeys[i].mValue.z);
				position.push_back(animation->mRotationKeys[i].mValue.w); // may need to change order
			}
		}
		if (animation->mNumScalingKeys > 0)
			flag |= 0x04;
		// scaling not supported
	}
	writeStartTag(pod::e_nodeAnimationFlags, 4);
	write4Bytes(m_fileStream, flag);
	writeEndTag(pod::e_nodeAnimationFlags);

	// Animation Position, 3 floats per frame of animation
	writeStartTag(pod::e_nodeAnimationPosition, 4 * 3);
	writeEndTag(pod::e_nodeAnimationPosition);

	// Animation Rotation, 4 floats per frame of animation
	writeStartTag(pod::e_nodeAnimationRotation, 4 * 4);
	writeEndTag(pod::e_nodeAnimationRotation);

	// Animation Scale, 7 floats per frame of animation
	writeStartTag(pod::e_nodeAnimationScale, 4 * 7);
	writeEndTag(pod::e_nodeAnimationScale);

	writeEndTag(pod::e_sceneNode);
}

void PODWriter::writeTextureBlock(uint index)
{
	// write texture block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneTexture, 0);

	// Texture Name (file path not included as stated in the document)
	std::string path = m_modelLoader.getTexture(index);
	const size_t last_slash_idx = path.rfind('/');
	if (std::string::npos != last_slash_idx)
	{
		path = path.substr(last_slash_idx + 1, path.length() - last_slash_idx);
	}

	StringHash name(path);
	writeStartTag(pod::e_textureFilename, name.length() + 1);
	writeByteArrayFromeStringHash(m_fileStream, name);
	writeEndTag(pod::e_textureFilename);

	writeEndTag(pod::e_sceneTexture);
}

}
}
}

