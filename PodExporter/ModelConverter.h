#pragma once
#include "ModelLoader.h"
#include "PODWriter.h"
using namespace pvr::assets::assetWriters;
namespace VEEMEE
{
	class ModelConverter
	{
	public:
		static ModelConverter& getInstance()
		{
			static ModelConverter instance;

			return instance;
		}

		ModelConverter(ModelConverter const&) = delete;
		void operator=(ModelConverter const&) = delete;

		void ConvertToPOD(const std::string& fileName)
		{
			ModelLoader loader;
			vector<ModelDataPtr> models = loader.loadModel(fileName);

			PODWriter exporter(loader);
			exporter.setModels(models);

			string nameWithoutExtension;
			const size_t last_idx = fileName.rfind('.');
			if (std::string::npos != last_idx)
			{
				nameWithoutExtension = fileName.substr(0, last_idx);
			}

			exporter.exportModel(nameWithoutExtension + ".pod", PODWriter::ExportSkinningData);
		}

	private:
		ModelConverter() {};
	};

}