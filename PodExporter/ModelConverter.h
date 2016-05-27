#pragma once
#include "ModelLoader.h"
#include "PODWriter.h"

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
			m_aiScene = loader.getScene();

			pvr::assets::assetWriters::PODWriter exporter(loader);
			exporter.setModels(models);

			string nameWithoutExtension;
			const size_t last_slash_idx = fileName.rfind('.');
			if (std::string::npos != last_slash_idx)
			{
				nameWithoutExtension = fileName.substr(0, last_slash_idx);
			}

			exporter.exportModel(nameWithoutExtension + ".pod");
		}

	private:
		ModelConverter() {};
		const aiScene* m_aiScene;

	};

}