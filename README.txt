This is a pod writer C++ API, using Assimp 3.1.1 as the model loader.

The project is written with Microsoft Visual Studio 2015.

The Assimp library is built into dll, x86. The static linking of Assimp is slow at run time, especially in the debug mode.

How to use the API:

VEEMEE::ModelConverter::getInstance().ConvertToPOD("filename", exportOptions);

E.g.
VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/model.fbx", PODWriter::Basic); // this will export the basic mesh without skinning data

VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/model.fbx", PODWriter::ExportSkinningData); // this will export the basic mesh in the bind pose

VEEMEE::ModelConverter::getInstance().ConvertToPOD("D:/model.fbx", PODWriter::ExportEverything); // this will export the model with the animation (currently not working)

Issues need to fix:
Skinning transformations matrixes are under work now.