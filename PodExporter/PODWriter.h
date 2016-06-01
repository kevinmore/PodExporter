#pragma once
#include "PVRAssets/AssetWriter.h"
#include "ModelLoader.h"
#include <fstream>
using std::vector;

namespace pvr {
namespace assets {
namespace assetWriters {
class Model;

class PODWriter : public AssetWriter<Model>
{
public:
	PODWriter(ModelLoader& loader);

	void exportModel(const std::string& path, bool exportSkinningData = true, bool exportAnimations = true);
	void setModels(vector<ModelDataPtr>& models) { m_modelDataVec = models; }

	virtual bool addAssetToWrite(const Model& asset);
	virtual bool writeAllAssets();

	virtual uint32 assetsAddedSoFar();
	virtual bool supportsMultipleAssets();

	virtual bool canWriteAsset(const Model& asset);
	virtual vector<string> getSupportedFileExtensions();
	virtual string getWriterName();
	virtual string getWriterVersion();

private:
	void writeStartTag(uint32 identifier, uint32 dataLength);
	void writeEndTag(uint32 identifier);

	void writeSceneBlock();
	void writeMaterialBlock(uint index);
	void writeMeshBlock(uint index);
	void writeNodeBlock(uint index);
	void writeTextureBlock(uint index);

	ModelLoader m_modelLoader;
	vector<ModelDataPtr> m_modelDataVec;
	vector<aiNode*> m_Nodes;
	bool m_exportSkinningData;
	bool m_exportAnimations;

	fstream m_fileStream;
};

}
}
}