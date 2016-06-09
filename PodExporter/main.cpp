#include "ModelConverter.h"
void main()
{
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/ninja/ninja.b3d", PODWriter::ExportEverything);
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/PCMinityGrey/basic.fbx", PODWriter::ExportEverything);
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/Imagination/m005.DAE", PODWriter::ExportEverything);
}
