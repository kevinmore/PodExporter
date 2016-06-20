#pragma once
#include "ModelLoader.h"
#include "PODDefines.h"
#include <fstream>
using std::vector;

namespace pvr {
namespace assets {
namespace assetWriters {

class PODWriter
{
public:
	enum ExportOptions
	{
		Basic = 0x00,
		ExportSkinningData = 0x01,
		ExportAnimation = 0x02,
		ExportEverything = 0x03
	};

	PODWriter(ModelLoader& loader);

	void exportModel(const std::string& path, ExportOptions options = ExportEverything);
	void setModels(vector<ModelDataPtr>& models) { m_modelDataVec = models; }

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
	ExportOptions m_exportOptions;
	uint32 m_numFrames;
	fstream m_fileStream;
};

}
}
}