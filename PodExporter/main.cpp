#include "ModelConverter.h"

void main()
{
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("../ModelExamples/Trump/Trump2.fbx");
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/SDK/assimp-3.2/test/models-nonbsd/X/dwarf.x");
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("../ModelExamples/RocketBox_Models/m005.dae");
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("../ModelExamples/crytek_sponza/sponza.obj");
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/Naruto/Naruto.fbx", PODWriter::Basic);
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/ninja/ninja.b3d");
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/Thunder/hyperspace_madness_fbx_review_samples/Hyperspace_Madness_samples/Hyperspace_Madness_Killamari_Minion.fbx");
}
