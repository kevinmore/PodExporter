/******************************************************************************

@File         PVRTVertex.cpp

@Title        PVRTVertex

@Version

@Copyright    Copyright (c) Imagination Technologies Limited.

@Platform     ANSI compatible

@Description  Utility functions which process vertices.

******************************************************************************/

/****************************************************************************
** Includes
****************************************************************************/
#include "Common.h"
#include "PVRTVertex.h"

/****************************************************************************
** Defines
****************************************************************************/

/****************************************************************************
** Macros
****************************************************************************/
#define MAX_VERTEX_OUT (3*nVtxNum)
#define PVRT_MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define PVRT_MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define PVRT_CLAMP(x, l, h)      (PVRT_MIN((h), PVRT_MAX((x), (l))))

/****************************************************************************
** Structures
****************************************************************************/

/****************************************************************************
** Constants
****************************************************************************/

/****************************************************************************
** Local function definitions
****************************************************************************/

/*****************************************************************************
** Functions
*****************************************************************************/

/*!***************************************************************************
@Function			PVRTVertexRead
@Output			pV
@Input				pData
@Input				eType
@Input				nCnt
@Description		Read a vector
*****************************************************************************/
void PVRTVertexRead(
	PVRTVECTOR4f		* const pV,
	const void			* const pData,
	const EPVRTDataType	eType,
	const int			nCnt)
{
	int		i;
	float	*pOut = (float*)pV;

	pV->x = 0;
	pV->y = 0;
	pV->z = 0;
	pV->w = 1;

	switch (eType)
	{
	default:
		_ASSERT(false);
		break;

	case EPODDataFloat:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = ((float*)pData)[i];
		break;

	case EPODDataFixed16_16:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = ((int*)pData)[i] * 1.0f / (float)(1 << 16);
		break;

	case EPODDataInt:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((int*)pData)[i];
		break;

	case EPODDataUnsignedInt:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((unsigned int*)pData)[i];
		break;

	case EPODDataByte:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((char*)pData)[i];
		break;

	case EPODDataByteNorm:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((char*)pData)[i] / (float)((1 << 7) - 1);
		break;

	case EPODDataUnsignedByte:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((unsigned char*)pData)[i];
		break;

	case EPODDataUnsignedByteNorm:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((unsigned char*)pData)[i] / (float)((1 << 8) - 1);
		break;

	case EPODDataShort:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((short*)pData)[i];
		break;

	case EPODDataShortNorm:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((short*)pData)[i] / (float)((1 << 15) - 1);
		break;

	case EPODDataUnsignedShort:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((unsigned short*)pData)[i];
		break;

	case EPODDataUnsignedShortNorm:
		for (i = 0; i < nCnt; ++i)
			pOut[i] = (float)((unsigned short*)pData)[i] / (float)((1 << 16) - 1);
		break;

	case EPODDataRGBA:
	{
		unsigned int dwVal = *(unsigned int*)pData;
		unsigned char v[4];

		v[0] = (unsigned char)(dwVal >> 24);
		v[1] = (unsigned char)(dwVal >> 16);
		v[2] = (unsigned char)(dwVal >> 8);
		v[3] = (unsigned char)(dwVal >> 0);

		for (i = 0; i < 4; ++i)
			pOut[i] = 1.0f / 255.0f * (float)v[i];
	}
	break;

	case EPODDataABGR:
	{
		unsigned int dwVal = *(unsigned int*)pData;
		unsigned char v[4];

		v[0] = (unsigned char)(dwVal >> 0);
		v[1] = (unsigned char)(dwVal >> 8);
		v[2] = (unsigned char)(dwVal >> 16);
		v[3] = (unsigned char)(dwVal >> 24);

		for (i = 0; i < 4; ++i)
			pOut[i] = 1.0f / 255.0f * (float)v[i];
	}
	break;

	case EPODDataARGB:
	case EPODDataD3DCOLOR:
	{
		unsigned int dwVal = *(unsigned int*)pData;
		unsigned char v[4];

		v[0] = (unsigned char)(dwVal >> 16);
		v[1] = (unsigned char)(dwVal >> 8);
		v[2] = (unsigned char)(dwVal >> 0);
		v[3] = (unsigned char)(dwVal >> 24);

		for (i = 0; i < 4; ++i)
			pOut[i] = 1.0f / 255.0f * (float)v[i];
	}
	break;

	case EPODDataUBYTE4:
	{
		unsigned int dwVal = *(unsigned int*)pData;
		unsigned char v[4];

		v[0] = (unsigned char)(dwVal >> 0);
		v[1] = (unsigned char)(dwVal >> 8);
		v[2] = (unsigned char)(dwVal >> 16);
		v[3] = (unsigned char)(dwVal >> 24);

		for (i = 0; i < 4; ++i)
			pOut[i] = v[i];
	}
	break;

	case EPODDataDEC3N:
	{
		int dwVal = *(int*)pData;
		int v[4];

		v[0] = (dwVal << 22) >> 22;
		v[1] = (dwVal << 12) >> 22;
		v[2] = (dwVal << 2) >> 22;
		v[3] = 0;

		for (i = 0; i < 3; ++i)
			pOut[i] = (float)v[i] * (1.0f / 511.0f);
	}
	break;
	}
}

/*!***************************************************************************
@Function			PVRTVertexRead
@Output			pV
@Input				pData
@Input				eType
@Description		Read an int
*****************************************************************************/
void PVRTVertexRead(
	unsigned int		* const pV,
	const void			* const pData,
	const EPVRTDataType	eType)
{
	switch (eType)
	{
	default:
		_ASSERT(false);
		break;

	case EPODDataUnsignedShort:
		*pV = *(unsigned short*)pData;
		break;

	case EPODDataUnsignedInt:
		*pV = *(unsigned int*)pData;
		break;
	}
}

/*!***************************************************************************
@Function			PVRTVertexWrite
@Output			pOut
@Input				eType
@Input				nCnt
@Input				pV
@Description		Write a vector
*****************************************************************************/
void PVRTVertexWrite(
	void				* const pOut,
	const EPVRTDataType	eType,
	const int			nCnt,
	const PVRTVECTOR4f	* const pV)
{
	int		i;
	float	*pData = (float*)pV;

	switch (eType)
	{
	default:
		_ASSERT(false);
		break;

	case EPODDataDEC3N:
	{
		int v[3];

		for (i = 0; i < nCnt; ++i)
		{
			v[i] = (int)(pData[i] * 511.0f);
			v[i] = PVRT_CLAMP(v[i], -511, 511);
			v[i] &= 0x000003ff;
		}

		for (; i < 3; ++i)
		{
			v[i] = 0;
		}

		*(unsigned int*)pOut = (v[0] << 0) | (v[1] << 10) | (v[2] << 20);
	}
	break;

	case EPODDataARGB:
	case EPODDataD3DCOLOR:
	{
		unsigned char v[4];

		for (i = 0; i < nCnt; ++i)
			v[i] = (unsigned char)PVRT_CLAMP(pData[i] * 255.0f, 0.0f, 255.0f);

		for (; i < 4; ++i)
			v[i] = 0;

		*(unsigned int*)pOut = (v[3] << 24) | (v[0] << 16) | (v[1] << 8) | v[2];
	}
	break;

	case EPODDataRGBA:
	{
		unsigned char v[4];

		for (i = 0; i < nCnt; ++i)
			v[i] = (unsigned char)PVRT_CLAMP(pData[i] * 255.0f, 0.0f, 255.0f);

		for (; i < 4; ++i)
			v[i] = 0;

		*(unsigned int*)pOut = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
	}
	break;

	case EPODDataABGR:
	{
		unsigned char v[4];

		for (i = 0; i < nCnt; ++i)
			v[i] = (unsigned char)PVRT_CLAMP(pData[i] * 255.0f, 0.0f, 255.0f);

		for (; i < 4; ++i)
			v[i] = 0;

		*(unsigned int*)pOut = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
	}
	break;

	case EPODDataUBYTE4:
	{
		unsigned char v[4];

		for (i = 0; i < nCnt; ++i)
			v[i] = (unsigned char)PVRT_CLAMP(pData[i], 0.0f, 255.0f);

		for (; i < 4; ++i)
			v[i] = 0;

		*(unsigned int*)pOut = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
	}
	break;

	case EPODDataFloat:
		for (i = 0; i < nCnt; ++i)
			((float*)pOut)[i] = pData[i];
		break;

	case EPODDataFixed16_16:
		for (i = 0; i < nCnt; ++i)
			((int*)pOut)[i] = (int)(pData[i] * (float)(1 << 16));
		break;

	case EPODDataInt:
		for (i = 0; i < nCnt; ++i)
			((int*)pOut)[i] = (int)pData[i];
		break;

	case EPODDataUnsignedInt:
		for (i = 0; i < nCnt; ++i)
			((unsigned int*)pOut)[i] = (unsigned int)pData[i];
		break;

	case EPODDataByte:
		for (i = 0; i < nCnt; ++i)
			((char*)pOut)[i] = (char)pData[i];
		break;

	case EPODDataByteNorm:
		for (i = 0; i < nCnt; ++i)
			((char*)pOut)[i] = (char)(pData[i] * (float)((1 << 7) - 1));
		break;

	case EPODDataUnsignedByte:
		for (i = 0; i < nCnt; ++i)
			((unsigned char*)pOut)[i] = (unsigned char)pData[i];
		break;

	case EPODDataUnsignedByteNorm:
		for (i = 0; i < nCnt; ++i)
			((char*)pOut)[i] = (unsigned char)(pData[i] * (float)((1 << 8) - 1));
		break;

	case EPODDataShort:
		for (i = 0; i < nCnt; ++i)
			((short*)pOut)[i] = (short)pData[i];
		break;

	case EPODDataShortNorm:
		for (i = 0; i < nCnt; ++i)
			((short*)pOut)[i] = (short)(pData[i] * (float)((1 << 15) - 1));
		break;

	case EPODDataUnsignedShort:
		for (i = 0; i < nCnt; ++i)
			((unsigned short*)pOut)[i] = (unsigned short)pData[i];
		break;

	case EPODDataUnsignedShortNorm:
		for (i = 0; i < nCnt; ++i)
			((unsigned short*)pOut)[i] = (unsigned short)(pData[i] * (float)((1 << 16) - 1));
		break;
	}
}

/*!***************************************************************************
@Function			PVRTVertexWrite
@Output			pOut
@Input				eType
@Input				V
@Description		Write an int
*****************************************************************************/
void PVRTVertexWrite(
	void				* const pOut,
	const EPVRTDataType	eType,
	const unsigned int	V)
{
	switch (eType)
	{
	default:
		_ASSERT(false);
		break;

	case EPODDataUnsignedShort:
		*(unsigned short*)pOut = (unsigned short)V;
		break;

	case EPODDataUnsignedInt:
		*(unsigned int*)pOut = V;
		break;
	}
}

/*****************************************************************************
End of file (PVRTVertex.cpp)
*****************************************************************************/

