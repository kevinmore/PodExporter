/*!****************************************************************************

@file         PVRTVertex.h
@copyright    Copyright (c) Imagination Technologies Limited.
@brief        Utility functions which process vertices.

******************************************************************************/
#ifndef _PVRTVERTEX_H_
#define _PVRTVERTEX_H_

/****************************************************************************
** Enumerations
****************************************************************************/
enum EPVRTDataType {
	EPODDataNone,
	EPODDataFloat,
	EPODDataInt,
	EPODDataUnsignedShort,
	EPODDataRGBA,
	EPODDataARGB,
	EPODDataD3DCOLOR,
	EPODDataUBYTE4,
	EPODDataDEC3N,
	EPODDataFixed16_16,
	EPODDataUnsignedByte,
	EPODDataShort,
	EPODDataShortNorm,
	EPODDataByte,
	EPODDataByteNorm,
	EPODDataUnsignedByteNorm,
	EPODDataUnsignedShortNorm,
	EPODDataUnsignedInt,
	EPODDataABGR
};

/*****************************************************************************
** Functions
*****************************************************************************/

/*!***************************************************************************
@brief     3D floating point vector
*****************************************************************************/
typedef struct
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
	float z;	/*!< z coordinate */
} PVRTVECTOR3f;

/*!***************************************************************************
@brief     4D floating point vector
*****************************************************************************/
typedef struct
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
	float z;	/*!< z coordinate */
	float w;	/*!< w coordinate */
} PVRTVECTOR4f;

/*!***************************************************************************
@fn       			PVRTVertexRead
@param[out]			pV
@param[in]				pData
@param[in]				eType
@param[in]				nCnt
@brief      		Read a vector
*****************************************************************************/
void PVRTVertexRead(
	PVRTVECTOR4f		* const pV,
	const void			* const pData,
	const EPVRTDataType	eType,
	const int			nCnt);

/*!***************************************************************************
@fn       			PVRTVertexRead
@param[out]			pV
@param[in]				pData
@param[in]				eType
@brief      		Read an int
*****************************************************************************/
void PVRTVertexRead(
	unsigned int		* const pV,
	const void			* const pData,
	const EPVRTDataType	eType);

/*!***************************************************************************
@fn       			PVRTVertexWrite
@param[out]			pOut
@param[in]				eType
@param[in]				nCnt
@param[in]				pV
@brief      		Write a vector
*****************************************************************************/
void PVRTVertexWrite(
	void				* const pOut,
	const EPVRTDataType	eType,
	const int			nCnt,
	const PVRTVECTOR4f	* const pV);

/*!***************************************************************************
@fn       			PVRTVertexWrite
@param[out]			pOut
@param[in]				eType
@param[in]				V
@brief      		Write an int
*****************************************************************************/
void PVRTVertexWrite(
	void				* const pOut,
	const EPVRTDataType	eType,
	const unsigned int	V);

#endif /* _PVRTVERTEX_H_ */

/*****************************************************************************
End of file (PVRTVertex.h)
*****************************************************************************/

