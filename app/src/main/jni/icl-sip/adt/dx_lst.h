/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_lst.h
 *
 * $Id: dx_lst.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/**
* @file dx_lst.h
* @brief Declare dx list related functions
* @remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
*/
#include <common/cm_def.h>
#include <common/cm_trace.h>

#ifndef DX_LST_H
#define DX_LST_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	DX_LST_POINTER = 0,
	DX_LST_INTEGER,
	DX_LST_STRING
} DxLstType;

typedef struct dxLstObj* DxLst;

/**
* @fn DxLst     dxLstNew(IN DxLstType t)
* @brief create new dx link list
* @param t - type of list
* @retval DxLst - a pointer to dx link list
*/
DxLst     dxLstNew(IN DxLstType t);

/**
* @fn void      dxLstFree(IN OUT DxLst _this, void(*elementFreeCB)(void*))
* @brief free dx link list
* @param _this - a pointer to dx link list
* @param elementFreeCB - a function pointer which is used to free the element in link list
*/
void      dxLstFree(IN OUT DxLst _this, void(*elementFreeCB)(void*));

/**
* @fn DxLst     dxLstDup(IN OUT DxLst _this, void*(*elementDupCB)(void*))
* @brief duplicate dx link list
* @param _this - a pointer to dx link list which will be duplicated
* @param elementFreeCB - a function pointer which is used to duplicate the element in link list
* @retval DxLst - a pointer to the duplicate
*/
DxLst     dxLstDup(IN OUT DxLst _this, void*(*elementDupCB)(void*));

/**
* @fn int       dxLstGetSize(IN DxLst)
* @brief get current amount of elements in dx link list
* @param _this - a pointer to dx link list
* @retval int - current amount of elements in dx link list, return -1 if _this is NULL
*/
int       dxLstGetSize(IN DxLst _this);

/**
* @fn DxLstType dxLstGetType(IN DxLst _this)
* @brief get the type of dx link list
* @param _this - a pointer to dx link list
* @retval DxLstType - type of this dx link list
*/
DxLstType dxLstGetType(IN DxLst _this);

/**
* @fn RCODE     dxLstPutHead(IN OUT DxLst _this, void* elm)
* @brief put element in head of dx link list
* @param _this - a pointer to dx link list
* @param elm - a pointer to element which will be inserted
* @retval RCODE - return RC_OK for success, RC_ERROR for failed, RC_E_PARAM for NULL parameter
*/
RCODE     dxLstPutHead(IN OUT DxLst _this, void* elm);

/**
* @fn RCODE     dxLstPutTail(IN OUT DxLst _this, void* elm)
* @brief put element in tail of dx link list
* @param _this - a pointer to dx link list
* @param elm - a pointer to element which will be inserted
* @retval RCODE - return RC_OK for success, RC_ERROR for failed, RC_E_PARAM for NULL parameter
*/
RCODE     dxLstPutTail(IN OUT DxLst _this, void* elm);

/**
* @fn void*     dxLstGetHead(IN OUT DxLst _this)
* @brief retrieve the element in head of dx link list, user needs to free the memory of retrieved element
* @param _this - a pointer to dx link list
* @retval void* - a pointer to the element
*/
void*     dxLstGetHead(IN OUT DxLst _this);

/**
* @fn void*     dxLstGetTail(IN OUT DxLst)
* @brief retrieve the element in tail of dx link list, user needs to free the memory of retrieved element
* @param _this - a pointer to dx link list
* @retval void* - a pointer to the element
*/
void*     dxLstGetTail(IN OUT DxLst _this);

/**
* @fn int       dxLstInsert(IN OUT DxLst _this, IN void* elm, IN int pos)
* @brief insert element into dx link list at indicated position
* @param _this - a pointer to dx link list
* @param elm - a pointer to element
* @param pos - indicated position
* @retval int - if >=0 imply the real position inserted; if -1 imply error.
*/
int       dxLstInsert(IN OUT DxLst _this, IN void* elm, IN int pos);

/**
* @fn void*     dxLstGetAt(IN OUT DxLst _this, IN int pos)
* @brief retrieve the element at indicated position of dx link list, user needs to free the memory of retrieved element
* @param _this - a pointer to dx link list
* @param pos - indicated position
* @retval void* - a pointer to the element
*/
void*     dxLstGetAt(IN OUT DxLst _this, IN int pos);

/**
* @fn void*     dxLstPeek(IN DxLst _this, IN int pos)
* @brief peek the element at indicated position of dx link list
* @param _this - a pointer to dx link list
* @param pos - indicated position
* @retval void* - a pointer to the element
*/
void*     dxLstPeek(IN DxLst _this, IN int pos);

/**
* @fn void*     dxLstPeekTail(IN DxLst _this)
* @brief peek the element in tail of dx link list
* @param _this - a pointer to dx link list
* @retval void* - a pointer to the element
*/
void*     dxLstPeekTail(IN DxLst _this);

/**
* @fn void*	  dxLstPeekByIter(IN DxLst _this)
* @brief sequentially peek the element in dx link list; before call dxLstPeekByIter(),you must call dxLstResetIter(), otherwise you will get nothing from list
* @param _this - a pointer to dx link list
* @retval void* - a pointer to the element
*/
void*	  dxLstPeekByIter(IN DxLst _this);

/**
* @fn RCODE	  dxLstResetIter(IN OUT DxLst _this)
* @brief initialize the Iter in dx link list to be equaled to head
* @param _this - a pointer to dx link list
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
RCODE	  dxLstResetIter(IN OUT DxLst _this);

#ifdef  __cplusplus
}
#endif

#endif /* DX_LST_H */
