#include "ModelConverter.h"

void main()
{
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("../ModelExamples/Trump/Trump.fbx");
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("../ModelExamples/RocketBox_Models/m005.dae");
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("../ModelExamples/crytek_sponza/sponza.obj");
}
