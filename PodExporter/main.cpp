#include "ModelConverter.h"
void main()
{
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/ninja/ninja.b3d", PODWriter::ExportEverything);
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/PCMinityGrey/basic.fbx", PODWriter::ExportEverything);
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/Project-Nebula-master/Resource/Models/Final/NPC/f013_wave.DAE", PODWriter::ExportEverything);
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/Imagination/m005.DAE", PODWriter::Basic);
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/WorkSpace/Kevin/ogldev-source/Content/crytek_sponza/sponza.obj", PODWriter::Basic);
}
