/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_buf.h
 *
 * $Id: dx_buf.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/**
* @file dx_buf.h
* @brief Declare dx buffer related functions, dx buffer is a ring buffer
* @remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
*/

#ifndef DX_BUF_H
#define DX_BUF_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include <common/cm_trace.h>

typedef struct dxBufferObj* 	DxBuffer;

/**
* @fn DxBuffer	dxBufferNew(IN int size)
* @brief allocate memory for dx buffer
* @param size - size of data in dx buffer
* @retval DxBuffer a pointer to allocated memory; NULL for failure
*/
DxBuffer	dxBufferNew(IN int size);

/**
* @fn void dxBufferFree(IN OUT DxBuffer _this)
* @brief free memory for dx buffer
* @param _this - a pointer to dx buffer
*/
void dxBufferFree(IN OUT DxBuffer _this);

/**
* @fn DxBuffer dxBufferDup(IN DxBuffer _this)
* @brief Duplicate dx buffer
* @param _this - a pointer to the dx buffer which will be duplicated
* @retval DxBuffer a pointer to the duplicate; NULL for failure
*/
DxBuffer dxBufferDup(IN DxBuffer _this);

/**
* @fn void dxBufferClear(IN OUT DxBuffer _this)
* @brief clean the data in dx buffer
* @param _this - a pointer to the dx buffer which will be cleaned
*/
void	dxBufferClear(IN OUT DxBuffer _this);

/**
* @fn int dxBufferRead(IN OUT DxBuffer _this, OUT char* buff, IN int len)
* @brief read data from dx buffer
* @param _this - a pointer to the dx buffer from which data will be read
* @param buff - a pointer to allcated memory for putting readed data
* @param len - the length of wanted data
* @retval int the length of readed data
*/
int		dxBufferRead(IN OUT DxBuffer _this, OUT char* buff, IN int len);

/**
* @fn int		dxBufferWrite(IN OUT DxBuffer _this, OUT char* buff, IN int len)
* @brief write data to dx buffer
* @param _this - a pointer to the dx buffer into which data will be writen
* @param buff - a pointer to the data which will be writen
* @param len - the length of the data which will be writen
* @retval int the length of the data which is actually writen
*/
int		dxBufferWrite(IN OUT DxBuffer _this, OUT char* buff, IN int len);

/**
* @fn int		dxBufferGetCurrLen(IN DxBuffer _this)
* @brief get the length of data which is currently stored in dx buffer
* @param _this - a pointer to the dx buffer
* @retval int the length of data which is currently stored in dx buffer
*/
int		dxBufferGetCurrLen(IN DxBuffer _this);

/**
* @fn int		dxBufferGetMaxLen(IN DxBuffer _this)
* @brief get the MAX length of data which can be stored in dx buffer
* @param _this - a pointer to the dx buffer
* @retval int the MAX length of data which can be stored in dx buffer
*/
int		dxBufferGetMaxLen(IN DxBuffer _this);

/**
* @fn void		dxBufferSetTraceFlag(IN OUT DxBuffer _this, IN TraceFlag traceflag)
* @brief set this DxBuffer's trace flag ON or OFF
* @param _this - a pointer to dx buffer
* @param traceflag - a int to indicate whether trace flag is on or off
*/
void		dxBufferSetTraceFlag(IN OUT DxBuffer _this, IN TraceFlag traceflag);

#ifdef  __cplusplus
}
#endif


#endif /* DX_BUF_H */
