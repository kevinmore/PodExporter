/*!*********************************************************************************************************************
\file         PVRAssets/FileIO/PODDefines.h
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief         Contains Enumerations and Defines necessary to read POD model files.
***********************************************************************************************************************/
#pragma once
#include <string>

//!\cond NO_DOXYGEN
namespace pvr {

	/*!*******************************************************************************************
	\brief  8-bit integer unsigned type.
	*********************************************************************************************/
	typedef unsigned char          byte;

	/*!*******************************************************************************************
	\brief  Character type. 8-bit integer signed type on all currently supported platforms.
	*********************************************************************************************/
	typedef char                   char8;

	/*!*******************************************************************************************
	\brief  Wide-character type. Platform dependent.
	*********************************************************************************************/
	typedef	wchar_t                wchar;

#if defined(_UNICODE)
	typedef wchar             tchar;
#else
	typedef char8             tchar;
#endif
	/*!*******************************************************************************************
	\brief  String of basic characters.
	*********************************************************************************************/
	typedef std::basic_string<char> string;

	//UTF types
	/*!*******************************************************************************************
	\brief  A UTF-8 (unsigned) character. 8-bit unsigned integer.
	*********************************************************************************************/
	typedef unsigned char          utf8;
	/*!*******************************************************************************************
	\brief  A UTF-16 (unsigned) character. 16-bit unsigned integer.
	*********************************************************************************************/
	typedef unsigned short         utf16;
	/*!*******************************************************************************************
	\brief  A UTF-32 (unsigned) character. 32-bit unsigned integer.
	*********************************************************************************************/
	typedef unsigned int           utf32;

	//Signed Integer Types
	/*!*******************************************************************************************
	\brief  8-bit signed integer.
	*********************************************************************************************/
	typedef signed char            int8;

	/*!*******************************************************************************************
	\brief  16-bit signed integer.
	*********************************************************************************************/
	typedef signed short           int16;

	/*!*******************************************************************************************
	\brief  32-bit signed integer.
	*********************************************************************************************/
	typedef signed int             int32;
#if defined(_WIN32)

	/*!*******************************************************************************************
	\brief  64-bit signed integer.
	*********************************************************************************************/
	typedef signed __int64         int64;
#elif defined(__GNUC__)
	__extension__
		/*!*******************************************************************************************
		\brief  64-bit signed integer.
		*********************************************************************************************/
		typedef signed long long       int64;
#else
	/*!*******************************************************************************************
	\brief  64-bit signed integer.
	*********************************************************************************************/
	typedef signed long long       int64;
#endif
	//Unsigned Integer Types
	/*!*******************************************************************************************
	\brief  8-bit unsigned integer.
	*********************************************************************************************/
	typedef unsigned char          uint8;

	/*!*******************************************************************************************
	\brief  16-bit unsigned integer.
	*********************************************************************************************/
	typedef unsigned short         uint16;

	/*!*******************************************************************************************
	\brief  32-bit unsigned integer.
	*********************************************************************************************/
	typedef unsigned int           uint32;
	//#if defined(_WIN32)
	//typedef unsigned __int64       uint64;
	//#else
	typedef unsigned long long     uint64;
	//#endif

	//Floating Point
	/*!*******************************************************************************************
	\brief  32-bit floating point number (single-precision float).
	*********************************************************************************************/
	typedef float                  float32;

	/*!*******************************************************************************************
	\brief  64-bit floating point number (double-precision float).
	*********************************************************************************************/
	typedef double                 float64;

	/*!*********************************************************************************************
	\brief  An enumeration that defines data types used throughout the Framework.
	Commonly used in places where raw data are used to define the types actually contained.
	***********************************************************************************************/
	namespace DataType {
		enum Enum
		{
			None = 0,//< none
			Float32,//< float 1
			Int32,//< integer 2
			UInt16, //< unsigned short 3
			RGBA,//< rgba 4
			ARGB,//< argb 5
			D3DCOLOR,//< d3d color 6
			UBYTE4,//< unsigned 4 byte 7
			DEC3N,
			Fixed16_16,
			UInt8,//< unsigned byte 10
			Int16,//< short 11
			Int16Norm,//< short normalized 12
			Int8,//< byte 13
			Int8Norm,//< byte normalized 14
			UInt8Norm,//< unsigned byte normalized 15
			UInt16Norm,//< unsigned short normalized
			UInt32,//< unsigned int
			ABGR,//< abgr

			Custom = 1000
		};

		/*!*********************************************************************************************************************
		\brief Return the Size of a DataType.
		\param[in] type The Data type
		\return The size of the Datatype in bytes.
		***********************************************************************************************************************/
		inline uint32 size(Enum type)
		{
			switch (type)
			{
			default:
				return 0;
			case DataType::Float32:
			case DataType::Int32:
			case DataType::UInt32:
			case DataType::RGBA:
			case DataType::ABGR:
			case DataType::ARGB:
			case DataType::D3DCOLOR:
			case DataType::UBYTE4:
			case DataType::DEC3N:
			case DataType::Fixed16_16:
				return 4;
			case DataType::Int16:
			case DataType::Int16Norm:
			case DataType::UInt16:
				return 2;
			case DataType::UInt8:
			case DataType::UInt8Norm:
			case DataType::Int8:
			case DataType::Int8Norm:
				return 1;
			}
		}

		/*!*********************************************************************************************************************
		\brief Return the number of components in a datatype.
		\param[in] type The datatype
		\return The number of components (e.g. float32 is 1, vec3 is 3)
		***********************************************************************************************************************/
		inline uint32 componentCount(Enum type)
		{
			switch (type)
			{
			default:
				return 0;

			case DataType::Float32:
			case DataType::Int32:
			case DataType::UInt32:
			case DataType::Int16:
			case DataType::Int16Norm:
			case DataType::UInt16:
			case DataType::Fixed16_16:
			case DataType::Int8:
			case DataType::Int8Norm:
			case DataType::UInt8:
			case DataType::UInt8Norm:
				return 1;

			case DataType::DEC3N:
				return 3;

			case DataType::RGBA:
			case DataType::ABGR:
			case DataType::ARGB:
			case DataType::D3DCOLOR:
			case DataType::UBYTE4:
				return 4;
			}
		}


		/*!*********************************************************************************************
		\brief       Return if the format is Normalized (represents a range between 0..1 for unsigned types
		or between -1..1 for signed types)
		\param       type The format to test.
		\return      True if the format is Normalised.
		\description A Normalised format is a value that is stored as an Integer, but that actually
		represents a value from 0..1 or -1..1 instead of the numeric value
		of the integer. For example, for a normalised unsigned byte value, the value
		0 represents 0.0, the value 127 represents 0.5 and the value 255 represents 1.0.
		***********************************************************************************************/
		inline bool isNormalised(Enum type)
		{
			return (type == DataType::Int8Norm || type == DataType::UInt8Norm
				|| type == DataType::Int16Norm
				|| type == DataType::UInt16Norm);
		}
	}// DataType


	namespace assets
	{
		namespace Model
		{
			namespace Material 
			{
				enum BlendFunction
				{
					BlendFuncZero = 0,              //!< BlendFunction (Zero)
					BlendFuncOne,                   //!< BlendFunction (One)
					BlendFuncFactor,                //!< BlendFunction (Factor)
					BlendFuncOneMinusBlendFactor,   //!< BlendFunction (One Minus Blend Factor)

					BlendFuncSrcColor = 0x0300,     //!< BlendFunction (source Color)
					BlendFuncOneMinusSrcColor,      //!< BlendFunction (One Minus Source Color)
					BlendFuncSrcAlpha,              //!< BlendFunction (Source Alpha)
					BlendFuncOneMinusSrcAlpha,      //!< BlendFunction (One Minus Source Alpha)
					BlendFuncDstAlpha,              //!< BlendFunction (Destination Alpha)
					BlendFuncOneMinusDstAlpha,      //!< BlendFunction (One Minus Destination Alpha)
					BlendFuncDstColor,              //!< BlendFunction (Destination Alpha)
					BlendFuncOneMinusDstColor,      //!< BlendFunction (One Minus Destination Color)
					BlendFuncSrcAlphaSaturate,      //!< BlendFunction (Source Alpha Saturate)

					BlendFuncConstantColor = 0x8001,//!< BlendFunction (Constant Color)
					BlendFuncOneMinusConstantColor, //!< BlendFunction (One Minus Constant Color)
					BlendFuncConstantAlpha,         //!< BlendFunction (Constant Alpha)
					BlendFuncOneMinusConstantAlpha  //!< BlendFunction (One Minus Constant Alpha)
				};

				enum BlendOperation
				{
					BlendOpAdd = 0x8006,            //!< Blend Operation (Add)
					BlendOpMin,                     //!< Blend Operation (Min)
					BlendOpMax,                     //!< Blend Operation (Max)
					BlendOpSubtract = 0x800A,       //!< Blend Operation (Subtract)
					BlendOpReverseSubtract          //!< Blend Operation (Reverse Subtract)
				};
			}
		}
	}

	namespace pod {

		enum PodTagConstants :uint32
		{
			c_startTagMask = 0x00000000,
			c_endTagMask = 0x80000000,
			c_TagMash = 0x80000000,
			c_PODFormatVersionLength = 11
		};
		static const char8* const c_PODFormatVersion = "AB.POD.2.0";

		/*!****************************************************************************
		\brief        Enum for the identifiers in the pod blocks.
		******************************************************************************/
		enum PODIdentifiers
		{
			PODFormatVersion = 1000,
			Scene,
			ExportOptions,
			FileHistory,
			EndiannessMismatch = -402456576,

			// Scene
			e_sceneClearColor = 2000,
			e_sceneAmbientColor,
			e_sceneNumCameras,
			e_sceneNumLights,
			e_sceneNumMeshes,
			e_sceneNumNodes,
			e_sceneNumMeshNodes,
			e_sceneNumTextures,
			e_sceneNumMaterials,
			e_sceneNumFrames,
			e_sceneCamera,		// Will come multiple times
			e_sceneLight,		// Will come multiple times
			e_sceneMesh,		// Will come multiple times
			e_sceneNode,		// Will come multiple times
			e_sceneTexture,	// Will come multiple times
			e_sceneMaterial,	// Will come multiple times
			e_sceneFlags,
			e_sceneFPS,
			e_sceneUserData,
			e_sceneUnits,

			// Materials
			e_materialName = 3000,
			e_materialDiffuseTextureIndex,
			e_materialOpacity,
			e_materialAmbientColor,
			e_materialDiffuseColor,
			e_materialSpecularColor,
			e_materialShininess,
			e_materialEffectFile,
			e_materialEffectName,
			e_materialAmbientTextureIndex,
			e_materialSpecularColorTextureIndex,
			e_materialSpecularLevelTextureIndex,
			e_materialBumpMapTextureIndex,
			e_materialEmissiveTextureIndex,
			e_materialGlossinessTextureIndex,
			e_materialOpacityTextureIndex,
			e_materialReflectionTextureIndex,
			e_materialRefractionTextureIndex,
			e_materialBlendingRGBSrc,
			e_materialBlendingAlphaSrc,
			e_materialBlendingRGBDst,
			e_materialBlendingAlphaDst,
			e_materialBlendingRGBOperation,
			e_materialBlendingAlphaOperation,
			e_materialBlendingRGBAColor,
			e_materialBlendingFactorArray,
			e_materialFlags,
			e_materialUserData,

			// Textures
			e_textureFilename = 4000,

			// Nodes
			e_nodeIndex = 5000,
			e_nodeName,
			e_nodeMaterialIndex,
			e_nodeParentIndex,
			e_nodePosition, // Deprecated
			e_nodeRotation, // Deprecated
			e_nodeScale,	// Deprecated
			e_nodeAnimationPosition,
			e_nodeAnimationRotation,
			e_nodeAnimationScale,
			e_nodeMatrix,	// Deprecated
			e_nodeAnimationMatrix,
			e_nodeAnimationFlags,
			e_nodeAnimationPositionIndex,
			e_nodeAnimationRotationIndex,
			e_nodeAnimationScaleIndex,
			e_nodeAnimationMatrixIndex,
			e_nodeUserData,

			// Mesh
			e_meshNumVertices = 6000,
			e_meshNumFaces,
			e_meshNumUVWChannels,
			e_meshVertexIndexList,
			e_meshStripLength,
			e_meshNumStrips,
			e_meshVertexList,
			e_meshNormalList,
			e_meshTangentList,
			e_meshBinormalList,
			e_meshUVWList,			// Will come multiple times
			e_meshVertexColorList,
			e_meshBoneIndexList,
			e_meshBoneWeightList,
			e_meshInterleavedDataList,
			e_meshBoneBatchIndexList,
			e_meshNumBoneIndicesPerBatch,
			e_meshBoneOffsetPerBatch,
			e_meshMaxNumBonesPerBatch,
			e_meshNumBoneBatches,
			e_meshUnpackMatrix,

			// Light
			e_lightTargetObjectIndex = 7000,
			e_lightColor,
			e_lightType,
			e_lightConstantAttenuation,
			e_lightLinearAttenuation,
			e_lightQuadraticAttenuation,
			e_lightFalloffAngle,
			e_lightFalloffExponent,

			// Camera
			e_cameraTargetObjectIndex = 8000,
			e_cameraFOV,
			e_cameraFarPlane,
			e_cameraNearPlane,
			e_cameraFOVAnimation,

			// Mesh data block
			e_blockDataType = 9000,
			e_blockNumComponents,
			e_blockStride,
			e_blockData
		};
	}
}
//!\endcond