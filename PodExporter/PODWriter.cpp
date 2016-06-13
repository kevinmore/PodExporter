/************************************************************************/
/* The reason using std::fstream instead of pvr::FileStream is that
pvr::FileStream is a text stream, on Windows Systems, it translate byte '\n'
to '\r\n', which produces one more byte than expected.                  */
/************************************************************************/

#include "PODWriter.h"
#include "PVRTBoneBatches.h"
#include <cstdio>
#include <algorithm>

#define  MAX_NUM_BONES_PER_BATCH 9
#define HISTORY_MESSAGE "Hello POD!" // Put your messages here...

namespace { // LOCAL FUNCTIONS
using namespace pvr;
using namespace assets;

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
	pvr::DataType::Enum type;
	switch (sizeof(T))
	{
	case 2:
		type = pvr::DataType::UInt16;
		break;
	case 4:
		type = pvr::DataType::UInt32;
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
void writeVertexData(fstream& stream, pvr::DataType::Enum type, uint32 numComponents, uint32 stride, std::vector<T>& data)
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

void writeVertexAttributeOffset(fstream& stream, pvr::DataType::Enum type, uint32 numComponents, uint32 stride, uint32 offset)
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
void addByteIntoVector(T& data, vector<char>& targetVector)
{
	char *temp;
	temp = reinterpret_cast<char*>(&data);
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
	, m_matrixMap(loader.getBoneOffsetMatrixMap())
{
}

void PODWriter::exportModel(const std::string& path, ExportOptions options)
{
	// determine exporting options
	m_exportSkinningData = (options == ExportEverything) || (options == ExportSkinningData);
	m_exportAnimations = m_modelLoader.getScene()->HasAnimations() && (options == ExportEverything || options == ExportAnimation) ;
	
	// validate if there is skinning data
	if (m_exportSkinningData)
	{
		for (uint i = 0; i < m_modelLoader.getScene()->mNumMeshes; ++i)
		{
			aiMesh* mesh = m_modelLoader.getScene()->mMeshes[i];
			m_exportSkinningData = m_exportSkinningData && m_modelLoader.getScene()->mMeshes[i]->HasBones();
		}
	}

	m_fileStream = fstream(path, ios::binary | ios::out | ios::trunc);

	if (m_fileStream.is_open())
	{
		// write pod version block
		writeStartTag(pod::PODFormatVersion, pod::c_PODFormatVersionLength);
		writeByteArray(m_fileStream, pod::c_PODFormatVersion, pod::c_PODFormatVersionLength);
		writeEndTag(pod::PODFormatVersion);

		// write history block
		std::string msg = HISTORY_MESSAGE;
		writeStartTag(pod::FileHistory, msg.length() + 1);
		writeByteArrayFromeString(m_fileStream, msg);
		writeEndTag(pod::FileHistory);

		// write scene block
		// a block that contains only further nested blocks between its Start and End tags 
		// will have a Length of zero. 
		writeStartTag(pod::Scene, 0);
		writeSceneBlock();
		writeEndTag(pod::Scene);

		m_fileStream.flush();
		m_fileStream.close();
	}
	else
	{
		cout << "\nCannot open file: " << path;
	}

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
	// Clear Color
	float clearColor[3] = {0.68f, 0.68f, 0.68f};
	writeStartTag(pod::e_sceneClearColor, 3 * 4);
	write4ByteArray(m_fileStream, clearColor, 3);
	writeEndTag(pod::e_sceneClearColor);

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

	uint32 numFrames = 0;
	if (m_exportAnimations)
	{
		aiAnimation* animation = m_modelLoader.getScene()->mAnimations[0];
		uint32 numPositionKeys(0), numRotationKeys(0), numScalingKeys(0);
		for (uint i = 0; i < animation->mNumChannels; ++i)
		{
			numPositionKeys = std::max(numPositionKeys, animation->mChannels[i]->mNumPositionKeys);
			numRotationKeys = std::max(numRotationKeys, animation->mChannels[i]->mNumRotationKeys);
			numScalingKeys = std::max(numScalingKeys, animation->mChannels[i]->mNumScalingKeys);
		}

		// Num. Frames
		numFrames = std::max(std::max(numPositionKeys, numRotationKeys), numScalingKeys);
		writeStartTag(pod::e_sceneNumFrames, 4);
		write4Bytes(m_fileStream, numFrames);
		writeEndTag(pod::e_sceneNumFrames);

		// FPS (30 fps by default)
		uint32 fps = numFrames / static_cast<uint32>(animation->mDuration);

		//special case
		if (fps == 0)
		{
			fps = animation->mTicksPerSecond != 0 ? uint32(animation->mTicksPerSecond) : 30;
		}

		writeStartTag(pod::e_sceneFPS, 4);
		write4Bytes(m_fileStream, fps);
		writeEndTag(pod::e_sceneFPS);
	}

	// Material Block
	cout << "\n\nExporting Materials..." << endl;
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMaterialBlock(i);
	}

	// Texture Block
	cout << "\n\nExporting Textures..." << endl;
	for (uint32 i = 0; i < numTextures; ++i)
	{
		writeTextureBlock(i);
	}

	// Mesh Block
	cout << "\n\nExporting Meshes..." << endl;
	for (uint i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMeshBlock(i);
	}

	// Node Block
	cout << "\n\nExporting Nodes..." << endl;
	//for (uint i = 0; i < numFrames; ++i)
	//{
	//	aiAnimation* anim = m_modelLoader.getScene()->mAnimations[0];
	//	calcFinalTransforms(anim, i, m_Nodes[numMeshNodes], glm::mat4(1.0f));
	//}

	for (uint32 i = 0; i < numNodes; ++i)
	{
		writeNodeBlock(i);
	}

	cout << "\n\nDone Exporting." << endl;
}

void PODWriter::writeMeshBlock(uint index)
{
	MeshData meshData = m_modelDataVec[index]->meshData;

	// write mesh block
	writeStartTag(pod::e_sceneMesh, 0);

	// Unpack Matrix
	//writeStartTag(pod::e_meshUnpackMatrix, 4 * 16);
	//writeBytes(m_fileStream, meshData.unpackMatrix.a1, 4 * 16);
	//writeEndTag(pod::e_meshUnpackMatrix);

	// Num. Faces
	writeStartTag(pod::e_meshNumFaces, 4);
	write4Bytes(m_fileStream, (uint32)meshData.numFaces);
	writeEndTag(pod::e_meshNumFaces);

	// Num. UVW channels (currently only support 1 UV channel)
	uint32 numUVW = 1;
	writeStartTag(pod::e_meshNumUVWChannels, 4);
	write4Bytes(m_fileStream, numUVW);
	writeEndTag(pod::e_meshNumUVWChannels);

	// Get vertex attributes buffers from the model loader
	vector<vec3> positionBuffer = meshData.positions;
	vector<vec3> normalBuffer = meshData.normals;
	vector<vec3> tangentBuffer = meshData.tangents;
	vector<vec3> bitangentBuffer = meshData.bitangents;
	vector<color4D> colorBuffer = meshData.colors;
	vector<vec2> uvBuffer = meshData.texCoords;
	vector<VertexBoneData> boneBuffer = meshData.bones;

	// Index buffer
	vector<uint32> indexBuffer = meshData.indices;

	if (m_exportSkinningData)
	{
		// construct the interleaved data list
		uint32 stride = sizeof(positionBuffer[0]) + sizeof(boneBuffer[0]);
		if (normalBuffer.size() > 0) stride += sizeof(normalBuffer[0]);
		if (tangentBuffer.size() > 0) stride += sizeof(tangentBuffer[0]);
		if (bitangentBuffer.size() > 0) stride += sizeof(bitangentBuffer[0]);
		if (uvBuffer.size() > 0) stride += sizeof(uvBuffer[0]);
		if (colorBuffer.size() > 0) stride += sizeof(colorBuffer[0]);

		vector<char> interleavedDataList;
		for (uint i = 0; i < meshData.numVertices; ++i)
		{
			addByteIntoVector(positionBuffer[i], interleavedDataList);

			if (normalBuffer.size() > 0)
				addByteIntoVector(normalBuffer[i], interleavedDataList);

			if (tangentBuffer.size() > 0)
				addByteIntoVector(tangentBuffer[i], interleavedDataList);

			if (bitangentBuffer.size() > 0)
				addByteIntoVector(bitangentBuffer[i], interleavedDataList);

			if (uvBuffer.size() > 0)
				addByteIntoVector(uvBuffer[i], interleavedDataList);

			if (colorBuffer.size() > 0)
				addByteIntoVector(colorBuffer[i], interleavedDataList);

			addByteIntoVector(boneBuffer[i], interleavedDataList);
		}

		CPVRTBoneBatches boneBatches;
		int    nVtxOut;
		char    *pVtxOut;

		int idOffset = stride - sizeof(boneBuffer[0]);
		boneBatches.Create(&nVtxOut, &pVtxOut, indexBuffer.data(), meshData.numVertices,
			interleavedDataList.data(), stride,
			idOffset + 4 * sizeof(uint16), EPODDataFloat,
			idOffset, EPODDataUnsignedShort,
			meshData.numFaces, MAX_NUM_BONES_PER_BATCH, NUM_BONES_PER_VEREX);

		// Num. Vertices
		writeStartTag(pod::e_meshNumVertices, 4);
		write4Bytes(m_fileStream, nVtxOut);
		writeEndTag(pod::e_meshNumVertices);

		// Max. Num. Bones per Batch 
		writeStartTag(pod::e_meshMaxNumBonesPerBatch, 4);
		write4Bytes(m_fileStream, boneBatches.nBatchBoneMax);
		writeEndTag(pod::e_meshMaxNumBonesPerBatch);

		// Num. Bone Batches 
		writeStartTag(pod::e_meshNumBoneBatches, 4);
		write4Bytes(m_fileStream, boneBatches.nBatchCnt);
		writeEndTag(pod::e_meshNumBoneBatches);

		// Num. Bone Indices per Batch 
		// A list of integers, each integer representing the number of indices in each batch in the "Bone Batch Index List"
		writeStartTag(pod::e_meshNumBoneIndicesPerBatch, 4 * boneBatches.nBatchCnt);
		write4ByteArray(m_fileStream, boneBatches.pnBatchBoneCnt, boneBatches.nBatchCnt);
		writeEndTag(pod::e_meshNumBoneIndicesPerBatch);

		// Bone Batch Index List 
		// A list of indices into the "Node" list, each indexed "Node" representing the transformations associated with a single bone. 
		// (Read via "Bone Index List"). 
		writeStartTag(pod::e_meshBoneBatchIndexList, 4 * boneBatches.nBatchBoneMax * boneBatches.nBatchCnt);
		write4ByteArray(m_fileStream, boneBatches.pnBatches, boneBatches.nBatchBoneMax * boneBatches.nBatchCnt);
		writeEndTag(pod::e_meshBoneBatchIndexList);

		// Bone Offset per Batch
		// A list of integers, each integer representing the offset into the "Vertex List", 
		// or "Vertex Index List" of the data is indexed, the batch starts at. 
		writeStartTag(pod::e_meshBoneOffsetPerBatch, 4 * boneBatches.nBatchCnt);
		write4ByteArray(m_fileStream, boneBatches.pnBatchOffset, boneBatches.nBatchCnt);
		writeEndTag(pod::e_meshBoneOffsetPerBatch);

		// Interleaved data list
		writeStartTag(pod::e_meshInterleavedDataList, stride * nVtxOut);
		writeByteArray(m_fileStream, pVtxOut, stride * nVtxOut);
		writeEndTag(pod::e_meshInterleavedDataList);
		FREE(pVtxOut);

		// Vertex Index List
		writeStartTag(pod::e_meshVertexIndexList, sizeof(uint32) * indexBuffer.size());
		writeVertexIndexData<uint32>(m_fileStream, indexBuffer);
		writeEndTag(pod::e_meshVertexIndexList);

		// Dummy Vertex Attribute Lists (as all the vertex data is in the interleaved data list)
		uint32 offset = 0;
		writeStartTag(pod::e_meshVertexList, 0);
		writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
		offset += DataType::size(DataType::Float32) * 3;
		writeEndTag(pod::e_meshVertexList);

		if (normalBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshNormalList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
			offset += DataType::size(DataType::Float32) * 3;
			writeEndTag(pod::e_meshNormalList);
		}

		if (tangentBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshTangentList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
			offset += DataType::size(DataType::Float32) * 3;
			writeEndTag(pod::e_meshTangentList);
		}

		if (bitangentBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshBinormalList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
			offset += DataType::size(DataType::Float32) * 3;
			writeEndTag(pod::e_meshBinormalList);
		}

		if (uvBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshUVWList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 2, stride, offset);
			offset += DataType::size(DataType::Float32) * 2;
			writeEndTag(pod::e_meshUVWList);
		}

		if (colorBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshVertexColorList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 4, stride, offset);
			offset += DataType::size(DataType::Float32) * 4;
			writeEndTag(pod::e_meshVertexColorList);
		}

		writeStartTag(pod::e_meshBoneIndexList, 0);
		writeVertexAttributeOffset(m_fileStream, DataType::UInt16, 4, stride, offset);
		offset += DataType::size(DataType::UInt16) * 4;
		writeEndTag(pod::e_meshBoneIndexList);

		writeStartTag(pod::e_meshBoneWeightList, 0);
		writeVertexAttributeOffset(m_fileStream, DataType::Float32, 4, stride, offset);
		offset += DataType::size(DataType::Float32) * 4;
		writeEndTag(pod::e_meshBoneWeightList);
	}
	else
	{
		// Num. Vertices
		writeStartTag(pod::e_meshNumVertices, 4);
		write4Bytes(m_fileStream, (uint32)meshData.numVertices);
		writeEndTag(pod::e_meshNumVertices);

		// Interleaved Data List
		// Structure: position.xyz + normal.xyz + tangetn.xyz + UV.xy
		uint32 stride = sizeof(positionBuffer[0]);

		if (normalBuffer.size() > 0) stride += sizeof(normalBuffer[0]);
		if (tangentBuffer.size() > 0) stride += sizeof(tangentBuffer[0]);
		if (bitangentBuffer.size() > 0) stride += sizeof(bitangentBuffer[0]);
		if (uvBuffer.size() > 0) stride += sizeof(uvBuffer[0]);
		if (colorBuffer.size() > 0) stride += sizeof(colorBuffer[0]);

		writeStartTag(pod::e_meshInterleavedDataList, stride * meshData.numVertices);
		for (uint i = 0; i < meshData.numVertices; ++i)
		{
			writeBytes(m_fileStream, positionBuffer[i]);

			if (normalBuffer.size() > 0)
				writeBytes(m_fileStream, normalBuffer[i]);

			if (tangentBuffer.size() > 0)
				writeBytes(m_fileStream, tangentBuffer[i]);

			if (bitangentBuffer.size() > 0)
				writeBytes(m_fileStream, bitangentBuffer[i]);

			if (uvBuffer.size() > 0)
				writeBytes(m_fileStream, uvBuffer[i]);

			if (colorBuffer.size() > 0)
				writeBytes(m_fileStream, colorBuffer[i]);
		}
		writeEndTag(pod::e_meshInterleavedDataList);

		// Vertex Index List
		writeStartTag(pod::e_meshVertexIndexList, sizeof(uint32) * indexBuffer.size());
		writeVertexIndexData<uint32>(m_fileStream, indexBuffer);
		writeEndTag(pod::e_meshVertexIndexList);

		// Dummy Vertex Attribute Lists (as all the vertex data is in the interleaved data list)
		uint32 offset = 0;
		writeStartTag(pod::e_meshVertexList, 0);
		writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
		offset += DataType::size(DataType::Float32) * 3;
		writeEndTag(pod::e_meshVertexList);

		if (normalBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshNormalList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
			offset += DataType::size(DataType::Float32) * 3;
			writeEndTag(pod::e_meshNormalList);
		}

		if (tangentBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshTangentList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
			offset += DataType::size(DataType::Float32) * 3;
			writeEndTag(pod::e_meshTangentList);
		}

		if (bitangentBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshBinormalList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 3, stride, offset);
			offset += DataType::size(DataType::Float32) * 3;
			writeEndTag(pod::e_meshBinormalList);
		}

		if (uvBuffer.size() > 0) 
		{
			writeStartTag(pod::e_meshUVWList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 2, stride, offset);
			offset += DataType::size(DataType::Float32) * 2;
			writeEndTag(pod::e_meshUVWList);
		}

		if (colorBuffer.size() > 0)
		{
			writeStartTag(pod::e_meshVertexColorList, 0);
			writeVertexAttributeOffset(m_fileStream, DataType::Float32, 4, stride, offset);
			offset += DataType::size(DataType::Float32) * 4;
			writeEndTag(pod::e_meshVertexColorList);
		}
	} // end if export skinning data

	writeEndTag(pod::e_sceneMesh);
}

void PODWriter::writeNodeBlock(uint index)
{
	aiNode* node = m_Nodes[index];
	cout << "\n" << index << " " << node->mName.C_Str();

	// write node block
	writeStartTag(pod::e_sceneNode, 0);

	// Node Index (only consider mesh, light and camera not supported yet)
	int32 objectIndex = node->mNumMeshes == 1 ? index : -1;
	writeStartTag(pod::e_nodeIndex, 4);
	write4Bytes(m_fileStream, objectIndex);
	writeEndTag(pod::e_nodeIndex);

	// Node Name
	std::string name(node->mName.C_Str());
	writeStartTag(pod::e_nodeName, name.length() + 1);
	writeByteArrayFromeString(m_fileStream, name);
	writeEndTag(pod::e_nodeName);

	// Material Index (if the node is a mesh)
	int32 matIndex = node->mNumMeshes == 1 ? node->mMeshes[0] : -1;
	writeStartTag(pod::e_nodeMaterialIndex, 4);
	write4Bytes(m_fileStream, matIndex);
	writeEndTag(pod::e_nodeMaterialIndex);

	// Parent Index 
	int32 parentIdx = -1;
	for (uint32 i = 0; i < m_Nodes.size(); ++i)
	{
		if (m_Nodes[i] == node->mParent)
		{
			parentIdx = i;
			break;
		}
	}
	writeStartTag(pod::e_nodeParentIndex, 4);
	write4Bytes(m_fileStream, parentIdx);
	writeEndTag(pod::e_nodeParentIndex);

	// early exit if animation exporting is not required
	if (!m_exportAnimations)
	{
		writeEndTag(pod::e_sceneNode);
		return;
	}

	// Node Animation
	aiAnimation* anim = m_modelLoader.getScene()->mAnimations[0];
	aiNodeAnim* animation = NULL;
	for (uint i = 0; i < anim->mNumChannels; ++i)
	{
		if (anim->mChannels[i]->mNodeName == node->mName)
		{
			animation = anim->mChannels[i];
			break;
		}
	}
	vector<glm::vec3> positions;
	vector<glm::quat> rotations;
	vector<glm::vec3> scalings;
	vector<glm::mat4> matrices;

	vector<float> plainRotations;

	uint32 flag = 0x00; // no animation
	if (animation)
 	{
 		flag = 0x08; // using matrix
		glm::mat4 boneOffset;// = toGLMMatrix4x4(m_matrixMap[node]);

		if (m_matrixMap.find(node) != m_matrixMap.end())
		{
			boneOffset = toGLMMatrix4x4(m_matrixMap[node]);
		}

		uint numFrames = std::max(animation->mNumPositionKeys,
 			std::max(animation->mNumRotationKeys, animation->mNumScalingKeys));
		
		for (uint i = 0; i < animation->mNumPositionKeys; ++i)
 		{
			positions.push_back(glm::vec3(animation->mPositionKeys[i].mValue.x,
				animation->mPositionKeys[i].mValue.y, animation->mPositionKeys[i].mValue.z));
 		}
 
 		for (uint i = 0; i < animation->mNumRotationKeys; ++i)
 		{
			rotations.push_back(glm::quat(animation->mRotationKeys[i].mValue.w, animation->mRotationKeys[i].mValue.x,
				animation->mRotationKeys[i].mValue.y, animation->mRotationKeys[i].mValue.z));
 		}
 
 		for (uint i = 0; i < animation->mNumScalingKeys; ++i)
 		{
			scalings.push_back(glm::vec3(animation->mScalingKeys[i].mValue.x,
				animation->mScalingKeys[i].mValue.y, animation->mScalingKeys[i].mValue.z));
 		}

 		for (uint i = 0; i < numFrames; ++i)
 		{
			glm::mat4 translationM, rotationM, scalingM;

			// position
			if (i < positions.size())
			{
				translationM = glm::translate(glm::mat4(1.0f), positions[i]);
			}
			else
			{
				// pick the last one
				translationM = glm::translate(glm::mat4(1.0f), positions[positions.size() - 1]);
			}

			// rotation
			if (i < rotations.size())
			{
				rotationM = glm::mat4_cast(rotations[i]);
			}
			else
			{
				// pick the last one
				rotationM = glm::mat4_cast(rotations[rotations.size() - 1]);
			}
 		
			// rotation
			if (i < scalings.size())
			{
				scalingM = glm::scale(glm::mat4(1.0f), scalings[i]);
			}
			else
			{
				// pick the last one
				scalingM = glm::scale(glm::mat4(1.0f), scalings[scalings.size() - 1]);
			}

			glm::mat4 nodeTransformation = translationM * rotationM * scalingM;

			glm::mat4 parentTransform;
			aiNode* pNode = node;
			while (pNode->mParent)
			{
				parentTransform = toGLMMatrix4x4(pNode->mParent->mTransformation) * parentTransform;
				pNode = pNode->mParent;
			}
			glm::mat4 inverseParent = glm::inverse(parentTransform);

			glm::mat4 rootTrans = toGLMMatrix4x4(m_modelLoader.getScene()->mRootNode->mTransformation);
			glm::mat4 myParentTrans = toGLMMatrix4x4(node->mParent->mTransformation);
			glm::mat4 selfTrans = toGLMMatrix4x4(node->mTransformation);

			glm::vec3 outScale, outTranslation, outSkew;
			glm::quat outOrientation;
			glm::vec4 outPerspective;
			glm::decompose(nodeTransformation * boneOffset, outScale, outOrientation, outTranslation, outSkew, outPerspective);

			glm::vec3 acurateAngles_Bip01 = glm::eulerAngles(glm::quat(-0.5, -0.5, -0.5, -0.5));
			glm::vec3 acurateAngles_Bip01_Footsteps = glm::eulerAngles(glm::quat(0, 0, 0.707, -0.707));
			glm::vec3 acurateAngles_Bip01_Pelvis = glm::eulerAngles(glm::quat(-0.48586, -0.48586, -0.51375, -0.51375));
			glm::vec3 acurateAngles_Bip01_Spine = glm::eulerAngles(glm::quat(0.0001, -0.0094, -0.0222, -0.9997));
			glm::vec3 acurateAngles_Bip01_Spine1 = glm::eulerAngles(glm::quat(-0.000266838324, -0.0184, -0.01368, -1));
			glm::vec3 acurateAngles_Bip01_Spine2 = glm::eulerAngles(glm::quat(-0.00019, -0.012878, 0.01556, -1));
			glm::vec3 acurateAngles_Bip01_Neck = glm::eulerAngles(glm::quat(0, 0, 0.16845, -0.9857));
			//outOrientation = glm::rotate(outOrientation, glm::pi<float>() * 0.5f, glm::vec3(1, 0, 0));
			glm::vec3 myAngles = glm::eulerAngles(outOrientation);
			glm::vec3 myRotatedAngles1 = glm::eulerAngles(glm::rotate(outOrientation, glm::pi<float>() * 0.5f, glm::vec3(1, 0, 0)));
			glm::vec3 myRotatedAngles2 = glm::eulerAngles(glm::rotate(outOrientation, -glm::pi<float>() * 0.5f, glm::vec3(1, 0, 0)));

			plainRotations.push_back(outOrientation.x);
			plainRotations.push_back(outOrientation.y);
			plainRotations.push_back(outOrientation.z);
			plainRotations.push_back(outOrientation.w);

			matrices.push_back(myParentTrans * nodeTransformation * boneOffset);
 		}
 	}
 	else
	{
		matrices.push_back(toGLMMatrix4x4(node->mTransformation));
	}

	// Animation Flag
	writeStartTag(pod::e_nodeAnimationFlags, 4);
	write4Bytes(m_fileStream, flag);
	writeEndTag(pod::e_nodeAnimationFlags);

	// Animation Matrix, 16 floats per frame of animation
	writeStartTag(pod::e_nodeAnimationMatrix, sizeof(matrices[0]) * matrices.size());
	for (uint i = 0; i < matrices.size(); ++i)
	{
		write4ByteArray(m_fileStream, &matrices[i][0][0], 16);
	}
	writeEndTag(pod::e_nodeAnimationMatrix);

	writeEndTag(pod::e_sceneNode);
}

void PODWriter::writeMaterialBlock(uint index)
{
	MaterialData matData = m_modelDataVec[index]->materialData;

	// write material block
	writeStartTag(pod::e_sceneMaterial, 0);

	// Material Flags (blending enabled/disabled)
	uint32	flags = matData.blendMode > 0 ? 0x01 : 0x00;
	writeStartTag(pod::e_materialFlags, 4);
	write4Bytes(m_fileStream, flags);
	writeEndTag(pod::e_materialFlags);

	// Material Name
	std::string name(matData.name);
	writeStartTag(pod::e_materialName, name.length() + 1);
	writeByteArrayFromeString(m_fileStream, name);
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
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialDiffuseTextureIndex);

	writeStartTag(pod::e_materialAmbientTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_AMBIENT].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialAmbientTextureIndex);

	writeStartTag(pod::e_materialSpecularColorTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_SPECULAR].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialSpecularColorTextureIndex);

	writeStartTag(pod::e_materialSpecularLevelTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_HEIGHT].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialSpecularLevelTextureIndex);

	writeStartTag(pod::e_materialBumpMapTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_NORMALS].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialBumpMapTextureIndex);

	writeStartTag(pod::e_materialEmissiveTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_EMISSIVE].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialEmissiveTextureIndex);

	writeStartTag(pod::e_materialGlossinessTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_SHININESS].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialGlossinessTextureIndex);

	writeStartTag(pod::e_materialOpacityTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_OPACITY].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
	writeEndTag(pod::e_materialOpacityTextureIndex);

	writeStartTag(pod::e_materialReflectionTextureIndex, 4);
	if (!matData.textureData.texturesMap[aiTextureType_REFLECTION].empty())
	{
		write4Bytes(m_fileStream, offset);
		++offset;
	}
	else
		write4Bytes(m_fileStream, emptyTextureIndex);
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

	writeEndTag(pod::e_sceneMaterial);
}

void PODWriter::writeTextureBlock(uint index)
{
	// write texture block
	writeStartTag(pod::e_sceneTexture, 0);

	// Texture Name (file path not included as stated in the document)
	std::string path = m_modelLoader.getTexture(index);
	const size_t last_slash_idx = path.rfind('/');
	if (std::string::npos != last_slash_idx)
	{
		path = path.substr(last_slash_idx + 1, path.length() - last_slash_idx);
	}
	else
	{
		const size_t last_backslash_idx = path.rfind('\\');
		if (std::string::npos != last_backslash_idx)
		{
			path = path.substr(last_backslash_idx + 1, path.length() - last_backslash_idx);
		}
	}

	std::string name(path);
	writeStartTag(pod::e_textureFilename, name.length() + 1);
	writeByteArrayFromeString(m_fileStream, name);
	writeEndTag(pod::e_textureFilename);

	writeEndTag(pod::e_sceneTexture);
}

/*
aiNodeAnim* PODWriter::findNodeAnimation(aiAnimation* pAnimation, aiString& nodeName)
{
	for (uint i = 0; i < pAnimation->mNumChannels; ++i)
	{
		aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
		if (pNodeAnim->mNodeName == nodeName)
		{
			return pNodeAnim;
		}
	}

	return NULL;
}

void PODWriter::calcFinalTransforms(aiAnimation* pAnimation, uint currentFrame, aiNode* pNode, glm::mat4& parentTransform)
{
	aiNodeAnim* pNodeAnim = findNodeAnimation(pAnimation, pNode->mName);

	glm::mat4 nodeTransformation = toGLMMatrix4x4(pNode->mTransformation);

	if (pNodeAnim)
	{
		// position
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(pNodeAnim->mPositionKeys[currentFrame].mValue.x,
			pNodeAnim->mPositionKeys[currentFrame].mValue.y, pNodeAnim->mPositionKeys[currentFrame].mValue.z));

		// rotation
		glm::mat4 rotation = glm::mat4_cast(glm::quat(pNodeAnim->mRotationKeys[currentFrame].mValue.w, pNodeAnim->mRotationKeys[currentFrame].mValue.x,
			pNodeAnim->mRotationKeys[currentFrame].mValue.y, pNodeAnim->mRotationKeys[currentFrame].mValue.z));

		// scale
		glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(pNodeAnim->mScalingKeys[currentFrame].mValue.x,
			pNodeAnim->mScalingKeys[currentFrame].mValue.y, pNodeAnim->mScalingKeys[currentFrame].mValue.z));

		nodeTransformation = translation * rotation * scaling;
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransformation;
	map<aiNode*, mat4> matrixMap = m_modelLoader.getBoneOffsetMatrixMap();
	mat4 offsetMatrix;
	if (matrixMap.find(std::string(pNode->mName.C_Str())) != matrixMap.end())
	{
		offsetMatrix = matrixMap[std::string(pNode->mName.C_Str())];
	}

	//m_nodeAnimationsList[pNode].push_back( globalTransformation * toGLMMatrix4x4(offsetMatrix));
	m_nodeAnimationsList[pNode].push_back(nodeTransformation * toGLMMatrix4x4(offsetMatrix));

	for (uint i = 0; i < pNode->mNumChildren; ++i)
	{
		calcFinalTransforms(pAnimation, currentFrame, pNode->mChildren[i], globalTransformation);
	}
}
*/
}
}
}

