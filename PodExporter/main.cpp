#include "ModelConverter.h"
void main()
{
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/Imagination/m005.DAE", PODWriter::ExportAnimation);
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/VeeMod Source/Final/Trump.fbx", PODWriter::ExportAnimation);
}
