#include "ModelConverter.h"
void main()
{
	VEEMEE::ModelConverter::getInstance().ConvertToPOD("C:/Users/Kevin/Downloads/PCMinityGrey/basic.fbx", PODWriter::Basic);
	//VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/Imagination/m005.DAE", PODWriter::ExportSkinningData);
}
