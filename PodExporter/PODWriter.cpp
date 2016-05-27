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
bool writeBytes(Stream& stream, T& data)
{
	uint dataWritten;
	uint sizez = sizeof(T);
	return stream.write(sizeof(T), 1, &data, dataWritten);
}

template <typename T>
bool writeByteArray(Stream& stream, T* data, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		bool result = writeBytes(stream, data[i]);
		if (!result) { return result; }
	}
	return true;
}

template <typename T>
bool write4Bytes(Stream& stream, T& data)
{
	uint dataWritten;
	return stream.write(4, 1, &data, dataWritten);
}

template <typename T>
bool write4ByteArray(Stream& stream, T* data, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		if (!write4Bytes(stream, data[i])) { return false; }
	}
	return true;
}

template <typename T>
bool write2Bytes(Stream& stream, T& data)
{
	uint dataWritten;
	return stream.write(2, 1, &data, dataWritten);
}

template <typename T>
bool write2ByteArray(Stream& stream, T* data, uint32 count)
{
	for (uint32 i = 0; i < count; ++i)
	{
		if (!write2Bytes(stream, data[i])) { return false; }
	}
	return true;
}

template <typename T>
bool writeByteArrayFromVector(Stream& stream, vector<T>& data)
{
	return writeByteArray<T>(stream, data.data(), data.size());
}

template <typename T>
bool write2ByteArrayFromVector(Stream& stream, vector<T>& data)
{
	return write2ByteArray<T>(stream, data.data(), data.size());
}

template <typename T>
bool write4ByteArrayFromVector(Stream& stream, vector<T>& data)
{
	return write4ByteArray<T>(stream, data.data(), data.size());
}

bool writeByteArrayFromeString(Stream& stream, std::string& data)
{
	return writeByteArray(stream, data.data(), data.length());
}

bool writeByteArrayFromeStringHash(Stream& stream, StringHash& data)
{
	return writeByteArray(stream, data.c_str(), data.length());
}

bool writeTag(Stream& stream, uint32 tagMask, uint32 identifier, uint32 dataLength)
{
	uint32 halfTag = identifier | tagMask;
	if (!write4Bytes(stream, halfTag)) { return false; }
	if (!write4Bytes(stream, dataLength)) { return false; }
	return true;
}

template <typename T>
bool writeVertexIndexData(Stream& stream, std::vector<T>& data)
{
	bool result;

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
		result = write2ByteArrayFromVector<T>(stream, data);
		break;
	case 4:
		result = write4ByteArrayFromVector<T>(stream, data);
		break;
	}
	writeTag(stream, pod::c_endTagMask, pod::e_blockData, 0);

	return result;
}

template <typename T>
bool writeVertexData(Stream& stream, DataType::Enum type, uint32 numComponents, std::vector<T>& data)
{
	bool result;

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
	uint32 stride = DataType::size(type) * numComponents;
	write4Bytes(stream, stride); // need fix?
	writeTag(stream, pod::c_endTagMask, pod::e_blockStride, 0);

	// write the block data buffer
	writeTag(stream, pod::c_startTagMask, pod::e_blockData, data.size() * sizeof(T));

	switch (DataType::size(type))
	{
	case 1:
		result = writeByteArrayFromVector<T>(stream, data);
		break;
	case 2:
		result = write2ByteArrayFromVector<T>(stream, data);
		break;
	case 4:
		result = write4ByteArrayFromVector<T>(stream, data);
		break;
	}

	writeTag(stream, pod::c_endTagMask, pod::e_blockData, 0);

	return result;
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
{
}

void PODWriter::exportModel(const std::string& path)
{
	Stream::ptr_type assetStream(new FileStream(path, "w"));
	if (openAssetStream(assetStream))
	{
		writeAllAssets();
		m_assetStream->close();
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
	writeByteArray(*m_assetStream, pod::c_PODFormatVersion, pod::c_PODFormatVersionLength);
	writeEndTag(pod::PODFormatVersion);

	// write export options (mock up)
	writeStartTag(pod::ExportOptions, 0);
	writeEndTag(pod::ExportOptions);

	// write history (mock up)
	writeStartTag(pod::FileHistory, 0);
	writeEndTag(pod::FileHistory);

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

bool PODWriter::writeStartTag(uint32 identifier, uint32 dataLength)
{
	return writeTag(*m_assetStream, pod::c_startTagMask, identifier, dataLength);
}

bool PODWriter::writeEndTag(uint32 identifier)
{
	return writeTag(*m_assetStream, pod::c_endTagMask, identifier, 0);
}

void PODWriter::writeSceneBlock()
{
	// unit
	float32 units = 1.0f;
	writeStartTag(pod::e_sceneUnits, 4);
	write4Bytes(*m_assetStream, units);
	writeEndTag(pod::e_sceneUnits);

	// Clear Colour 
	float32	clearColor[3] = { 0.f, 0.f, 0.f };
	writeStartTag(pod::e_sceneClearColor, 3 * sizeof(float32));
	write4ByteArray(*m_assetStream, &clearColor[0], 3);
	writeEndTag(pod::e_sceneClearColor);

	// Ambient Colour 
	float32	ambientColor[3] = { 0.f, 0.f, 0.f };
	writeStartTag(pod::e_sceneAmbientColor, 3 * sizeof(float32));
	write4ByteArray(*m_assetStream, &ambientColor[0], 3);
	writeEndTag(pod::e_sceneAmbientColor);

	// Num. Cameras (not supported yet)
	uint32 numCam = 0;
	writeStartTag(pod::e_sceneNumCameras, 4);
	write4Bytes(*m_assetStream, numCam);
	writeEndTag(pod::e_sceneNumCameras);

	// Num. Lights (not supported yet)
	uint32 numLights = 0;
	writeStartTag(pod::e_sceneNumLights, 4);
	write4Bytes(*m_assetStream, numLights);
	writeEndTag(pod::e_sceneNumLights);

	// Num. Meshes
	uint32 numMeshes = m_modelDataVec.size();
	writeStartTag(pod::e_sceneNumMeshes, 4);
	write4Bytes(*m_assetStream, numMeshes);
	writeEndTag(pod::e_sceneNumMeshes);

	// Num. Nodes
	uint32 numNodes = m_modelLoader.getNumNodes();
	writeStartTag(pod::e_sceneNumNodes, 4);
	write4Bytes(*m_assetStream, numNodes);
	writeEndTag(pod::e_sceneNumNodes);

	// Num. Mesh Nodes
	uint32 numMeshNodes = m_modelDataVec.size();
	writeStartTag(pod::e_sceneNumMeshNodes, 4);
	write4Bytes(*m_assetStream, numMeshNodes);
	writeEndTag(pod::e_sceneNumMeshNodes);

	// Num. Textures
	uint32 numTextures = 0;
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		ModelDataPtr md = m_modelDataVec[i];

		if (!md->materialData.textureData.diffuseMap.empty())
			++numTextures;
		if (!md->materialData.textureData.normalMap.empty())
			++numTextures;
		if (!md->materialData.textureData.specularMap.empty())
			++numTextures;
	}
	writeStartTag(pod::e_sceneNumTextures, 4);
	write4Bytes(*m_assetStream, numTextures);
	writeEndTag(pod::e_sceneNumTextures);

	// Num. Materials (1 mesh 1 material)
	uint32 numMaterials = m_modelDataVec.size();
	writeStartTag(pod::e_sceneNumMaterials, 4);
	write4Bytes(*m_assetStream, numMaterials);
	writeEndTag(pod::e_sceneNumMaterials);

	// Num. Frames (30 fps by default)
	uint32 numFrames = static_cast<uint32>(m_modelLoader.getAnimationDuration() * 30);
	writeStartTag(pod::e_sceneNumFrames, 4);
	write4Bytes(*m_assetStream, numFrames);
	writeEndTag(pod::e_sceneNumFrames);

	// FPS (30 fps by default)
	uint32 fps = 30;
	writeStartTag(pod::e_sceneFPS, 4);
	write4Bytes(*m_assetStream, fps);
	writeEndTag(pod::e_sceneFPS);

	// Scene flags (0)
	uint32 flag = 0;
	writeStartTag(pod::e_sceneFlags, 4);
	write4Bytes(*m_assetStream, flag);
	writeEndTag(pod::e_sceneFlags);

	// Material Block
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMaterialBlock(i);
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

	// Mesh Block
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMeshBlock(i);
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
	write4Bytes(*m_assetStream, flags);
	writeEndTag(pod::e_materialFlags);

	// Material Name
	StringHash name(matData.name);
	writeStartTag(pod::e_materialName, name.length());
	writeByteArrayFromeStringHash(*m_assetStream, name);
	writeEndTag(pod::e_materialName);

	// Texture Index
	// calculate the index offset
	uint offset = 0;
	for (uint i = 0; i < index; ++i)
	{
		if (!m_modelDataVec[i]->materialData.textureData.diffuseMap.empty())
			++offset;
		if (!m_modelDataVec[i]->materialData.textureData.normalMap.empty())
			++offset;
		if (!m_modelDataVec[i]->materialData.textureData.specularMap.empty())
			++offset;
	}

	if (!matData.textureData.diffuseMap.empty())
	{
		writeStartTag(pod::e_materialDiffuseTextureIndex, 4);
		write4Bytes(*m_assetStream, offset);
		writeEndTag(pod::e_materialDiffuseTextureIndex);
		++offset;
	}

	if (!matData.textureData.normalMap.empty())
	{
		writeStartTag(pod::e_materialBumpMapTextureIndex, 4);
		write4Bytes(*m_assetStream, offset);
		writeEndTag(pod::e_materialBumpMapTextureIndex);
		++offset;
	}

	if (!matData.textureData.specularMap.empty())
	{
		writeStartTag(pod::e_materialSpecularColorTextureIndex, 4);
		write4Bytes(*m_assetStream, offset);
		writeEndTag(pod::e_materialSpecularColorTextureIndex);
		++offset;
	}

	// Ambient Color
	float32	ambientColor[3] = { matData.ambientColor.r, matData.ambientColor.g, matData.ambientColor.b };
	writeStartTag(pod::e_materialAmbientColor, 3 * sizeof(float32));
	write4ByteArray(*m_assetStream, &ambientColor[0], 3);
	writeEndTag(pod::e_materialAmbientColor);

	// Diffuse Color
	float32	diffuseColor[3] = { matData.diffuseColor.r, matData.diffuseColor.g, matData.diffuseColor.b };
	writeStartTag(pod::e_materialDiffuseColor, 3 * sizeof(float32));
	write4ByteArray(*m_assetStream, &diffuseColor[0], 3);
	writeEndTag(pod::e_materialDiffuseColor);

	// Specular Color
	float32	specularColor[3] = { matData.specularColor.r, matData.specularColor.g, matData.specularColor.b };
	writeStartTag(pod::e_materialSpecularColor, 3 * sizeof(float32));
	write4ByteArray(*m_assetStream, &specularColor[0], 3);
	writeEndTag(pod::e_materialSpecularColor);

	// Shininess
	writeStartTag(pod::e_materialShininess, 4);
	write4Bytes(*m_assetStream, (float32)matData.shininess);
	writeEndTag(pod::e_materialShininess);

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
	write4Bytes(*m_assetStream, (uint32)meshData.numVertices);
	writeEndTag(pod::e_meshNumVertices);

	// Num. Faces
	writeStartTag(pod::e_meshNumFaces, 4);
	write4Bytes(*m_assetStream, (uint32)meshData.numFaces);
	writeEndTag(pod::e_meshNumFaces);



	// Vertex Index List
// 	vector<uint16> entireIndexBuffer = m_modelLoader.geIndexBuffer();
// 	vector<uint16> currentIndexBuffer;
// 	for (uint i = meshData.baseIndex; i < meshData.baseIndex + meshData.numIndices; ++i)
// 	{
// 		currentIndexBuffer.push_back(entireIndexBuffer[i]);
// 	}
// 	writeStartTag(pod::e_meshVertexIndexList, sizeof(uint16) * currentIndexBuffer.size());
// 	writeVertexIndexData<uint16>(*m_assetStream, currentIndexBuffer);
// 	writeEndTag(pod::e_meshVertexIndexList);

	// Vertex List (Position)
	vector<vec3> positionBuffer = m_modelLoader.getPositionBuffer();

	// Normal List
	vector<vec3> normalBuffer = m_modelLoader.getNormalBuffer();

	// UVW List
	vector<vec2> uvBuffer = m_modelLoader.getUVBuffer();

	// Bone Weights List
	vector<VertexBoneData> boneBuffer = m_modelLoader.getBoneBuffer();

	// Interleaved Data List
	// Structure: position.xyz + normal.xyz + UV.xy + BoneWeight.xyzw + BoneIndex.xyzw (stride = 52 bytes)
	uint32 stride = sizeof(positionBuffer[0]) +sizeof(normalBuffer[0]) + sizeof(uvBuffer[0]) + sizeof(boneBuffer[0]);
	writeStartTag(pod::e_meshInterleavedDataList, stride * meshData.numVertices);
 	for (uint i = meshData.baseVertex; i < meshData.baseVertex + meshData.numVertices; ++i)
 	{
 		writeBytes(*m_assetStream, positionBuffer[i]);
 		writeBytes(*m_assetStream, normalBuffer[i]);
 		writeBytes(*m_assetStream, uvBuffer[i]);
 		writeBytes(*m_assetStream, boneBuffer[i].Weights);
 		writeBytes(*m_assetStream, boneBuffer[i].IDs);
 	}
	writeEndTag(pod::e_meshInterleavedDataList);

// 	writeStartTag(pod::e_meshVertexList, sizeof(vec3) * currentPositionBuffer.size());
// 	writeVertexData<vec3>(*m_assetStream, DataType::Float32, 3, currentPositionBuffer);
// 	writeEndTag(pod::e_meshVertexList);
// 	writeStartTag(pod::e_meshNormalList, sizeof(vec3) * currentNormalBuffer.size());
// 	writeVertexData<vec3>(*m_assetStream, DataType::Float32, 3, currentNormalBuffer);
// 	writeEndTag(pod::e_meshNormalList);
// 
// 	writeStartTag(pod::e_meshUVWList, sizeof(vec2) * currentUVBuffer.size());
// 	writeVertexData<vec2>(*m_assetStream, DataType::Float32, 2, currentUVBuffer);
// 	writeEndTag(pod::e_meshUVWList);

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
	writeEndTag(pod::e_nodeIndex);

	// Node Name
	//StringHash name(matData.name);
	//writeStartTag(pod::e_nodeName, name.length());
	//writeByteArrayFromeStringHash(*m_assetStream, name);
	//writeEndTag(pod::e_nodeName);

	// Material Index (if the node is a mesh)
	writeStartTag(pod::e_nodeMaterialIndex, 4);
	writeEndTag(pod::e_nodeMaterialIndex);

	// Parent Index 
	writeStartTag(pod::e_nodeParentIndex, 4);
	writeEndTag(pod::e_nodeParentIndex);

	// Animation Flag
	writeStartTag(pod::e_nodeAnimationFlags, 4);
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

	// Texture Index
	// calculate the index offset
	uint offset = 0;
	std::string path;
	for (uint i = 0; i <= index; ++i)
	{
		if (!m_modelDataVec[i]->materialData.textureData.diffuseMap.empty())
		{
			if (index == offset)
			{
				path = m_modelDataVec[i]->materialData.textureData.diffuseMap;
				break;
			}
			++offset;
		}
		if (!m_modelDataVec[i]->materialData.textureData.normalMap.empty())
		{
			if (index == offset)
			{
				path = m_modelDataVec[i]->materialData.textureData.normalMap;
				break;
			}
			++offset;
		}
		if (!m_modelDataVec[i]->materialData.textureData.specularMap.empty())
		{
			if (index == offset)
			{
				path = m_modelDataVec[i]->materialData.textureData.normalMap;
				break;
			}
			++offset;
		}
	}

	// Texture Name (file path not included as stated in the document)
	const size_t last_slash_idx = path.rfind('/');
	if (std::string::npos != last_slash_idx)
	{
		path = path.substr(last_slash_idx + 1, path.length() - last_slash_idx);
	}

	StringHash name(path);
	writeStartTag(pod::e_textureFilename, name.length());
	writeByteArrayFromeStringHash(*m_assetStream, name);
	writeEndTag(pod::e_textureFilename);

	writeEndTag(pod::e_sceneTexture);
}

}
}
}

