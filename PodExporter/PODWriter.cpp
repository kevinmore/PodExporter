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
	size_t dataWritten;
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
	size_t dataWritten;
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
	size_t dataWritten;
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

bool writeVertexIndexData(Stream& stream, assets::Mesh& mesh)
{
	return true;
}

}

namespace pvr {
namespace assets {
namespace assetWriters {

PODWriter::PODWriter(ModelLoader& loader)
	: m_modelLoader(loader)
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
	for (size_t i = 0; i < m_modelDataVec.size(); ++i)
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
	for (size_t i = 0; i < m_modelDataVec.size(); ++i)
	{
		writeMaterialBlock();
	}

	// Node Block
	for (uint32 i = 0; i < numNodes; ++i)
	{
		writeNodeBlock();
	}
}

void PODWriter::writeMaterialBlock()
{
	// write material block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneMaterial, 0);


}

void PODWriter::writeMeshBlock()
{
	// write mesh block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneMesh, 0);
}

void PODWriter::writeNodeBlock()
{
	// write node block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneNode, 0);
}

void PODWriter::writeTextureBlock()
{
	// write texture block
	// a block that contains only further nested blocks between its Start and End tags 
	// will have a Length of zero. 
	writeStartTag(pod::e_sceneTexture, 0);
}

}
}
}

