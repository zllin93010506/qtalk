/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_vec.h
 *
 * $Id: dx_vec.h,v 1.1.1.1 2008/08/01 07:00:24 judy Exp $
 */
/**
* @file dx_vec.h
* @brief Declare dx vector related functions
* @remarks Copyright (C) 2000-2002 Computer & Communications Research Laboratories, Industrial Technology Research Institute
*/
#include <common/cm_def.h>
#include <common/cm_trace.h>

#ifndef VECTOR_H
#define VECTOR_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	DX_VECTOR_POINTER = 0,
	DX_VECTOR_INTEGER,
	DX_VECTOR_STRING
} DxVectorType;

typedef struct dxVectorObj*	DxVector;

/**
* @fn DxVector        dxVectorNew(IN int initialCapacity, IN int capacityIncrement, IN DxVectorType type)
* @brief create new dx vector
* @param initialCapacity - the number for allocating memeory
* @param capacityIncrement - the number for extending memory in the future
* @param type - type of dx vector
* @retval DxVector - a pointer to dx vector
*/
DxVector        dxVectorNew(IN int initialCapacity, IN int capacityIncrement, IN DxVectorType type);

/**
* @fn void		dxVectorFree(IN OUT DxVector _this, IN void(*elementFreeCB)(void*))
* @brief free dx vector
* @param _this - a pointer to dx vector
* @param elementFreeCB - function pointer used to free element in dx vector
*/
void		dxVectorFree(IN OUT DxVector _this, IN void(*elementFreeCB)(void*));

/**
* @fn DxVector  	dxVectorDup(IN DxVector _this, IN void*(*elementDupCB)(void*))
* @brief duplicate dx vector
* @param this - a pointer to dx vector
* @param elementFreeCB - function pointer used to duplicate element in dx vector
* @retval DxVector - a pointer to duplicate
*/
DxVector  	dxVectorDup(IN DxVector _this, IN void*(*elementDupCB)(void*));

/**
* @fn int		dxVectorGetSize(IN DxVector _this)
* @brief get the current amount of elements in dx vector
* @param this - a pointer to dx vector
* @retval int - the current amount of elements in dx vector
*/
int		dxVectorGetSize(IN DxVector _this);

/**
* @fn DxVectorType	dxVectorGetType(IN DxVector _this)
* @brief get the type of dx vector
* @param this - a pointer to dx vector
* @retval DxVectorTypet - the type of dx vector
*/
DxVectorType	dxVectorGetType(IN DxVector _this);

/**
* @fn int             dxVectorGetCapacity(IN DxVector _this)
* @brief get the capacity of dx vector
* @param this - a pointer to dx vector
* @retval int - the capacity of dx vector
*/
int             dxVectorGetCapacity(IN DxVector _this);

/**
* @fn void dxVectorTrimToSize(IN OUT DxVector _this)
* @brief Trim capability to current size
* @param this - a pointer to dx vector
*/
void dxVectorTrimToSize(IN OUT DxVector _this);

/**
* @fn RCODE dxVectorEnsureCapacity(IN OUT DxVector _this, IN int minCapacity)
* @brief Increases the capacity of this vector, if necessary, to ensure that it can hold at least the number of components specified by the $minCapacity argument
* @param this - a pointer to dx vector
* @param minCapacity - MIN capacity the dx vector should have
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
RCODE dxVectorEnsureCapacity(IN OUT DxVector _this, IN int minCapacity);

/**
* @fn RCODE dxVectorAddElement(IN OUT DxVector _this, IN void* elemt)
* @brief Appends the specified element to the end of dx vector
* @param this - a pointer to dx vector
* @param elemt - a pointer to element
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
RCODE dxVectorAddElement(IN OUT DxVector _this, IN void* elemt);

/**
* @fn RCODE dxVectorAddElementAt(IN OUT DxVector _this, IN void* elemt, IN int position)
* @brief Inserts the specified element at the indicated position in dx vector
* @param this - a pointer to dx vector
* @param elemt - a pointer to element
* @param elemt - a indicated position should >= current amount of elements in dx vector
* @retval RCODE - RC_OK for success, RC_ERROR for failed, RC_E_PARAM for bad param
*/
RCODE dxVectorAddElementAt(IN OUT DxVector _this, IN void* elemt, IN int position);

/**
* @fn void* dxVectorGetLastElement(IN DxVector _this)
* @brief Returns the last element in dx vector, dxVectorGetLastElement does NOT do duplicate
* @param this - a pointer to dx vector
* @retval void* - Last element, if success; NULL, if error
*/
void* dxVectorGetLastElement(IN DxVector _this);

/**
* @fn int dxVectorGetIndexOf(IN DxVector _this, IN void* elemt, IN int position)
* @brief Searches for the first occurence of the given argument, beginning the search at $position, and testing for equality as follows, DX_VECTOR_POINTER, compare two pointers directly. DX_VECTOR_INTEGER, compare the integer value. DX_VECTOR_STRING, use strcmp() function.
* @param this - a pointer to dx vector
* @param elemt - a pointer to wanted elemt
* @param position - beginning position of searching
* @retval int - index, if success; -1, if error
*/
int dxVectorGetIndexOf(IN DxVector _this, IN void* elemt, IN int position);

/**
* @fn int dxVectorGetLastIndexOf(IN DxVector _this, IN void* elemt, IN int position)
* @brief Searches backwards for the specified object, starting from $position, and returns an index to it
* @param this - a pointer to dx vector
* @param elemt - a pointer to wanted elemt
* @param position - beginning position of searching
* @retval int - index, if success; -1, if error
*/
int dxVectorGetLastIndexOf(IN DxVector _this, IN void* elemt, IN int position);

/**
* @fn void* dxVectorGetElementAt(IN DxVector _this, IN int position)
* @brief Returns the element at indicated position, dxVectorGetElementAt does NOT do duplicate
* @param this - a pointer to dx vector
* @param position - the indicated position
* @retval void* - a pointer to wanted element; NULL if error
*/
void* dxVectorGetElementAt(IN DxVector _this, IN int position);

/**
* @fn RCODE dxVectorRemoveElement(IN OUT DxVector _this, IN void* elemt)
* @brief Removes the first occurrence of $elemt from this vector. 
*				If vector type is DX_VECTOR_POINTER, user is responsible to free the memory pointed by $elemt. 
*				On the case of DX_VECTOR_INTEGER and DX_VECTOR_STRING, this function call free the memory directly. 
* @param this - a pointer to dx vector
* @param elemt - a pointer to element for searching
* @retval RCODE - RC_OK for success, RC_ERROR for failed
*/
RCODE dxVectorRemoveElement(IN OUT DxVector _this, IN void* elemt);

/**
* @fn void* dxVectorRemoveElementAt(IN OUT DxVector _this, IN int position)
* @brief Remove element at indicated position of dx vecter
* @param this - a pointer to dx vector
* @param position - a indicated position
* @retval void* - a pointer to the element at indicated position, user should free the memory space returned. ( call free() function )
*/
void* dxVectorRemoveElementAt(IN OUT DxVector _this, IN int position);

/*  Fun: dxVectorSetTraceFlag
 *
 *  Desc: Remove element at a specific position of vecter. 
 *
 *  Ret: Return element value, user should free the memory space 
 *       returned. ( call free() function )
 */
/**
* @fn void dxVectorSetTraceFlag(IN OUT DxVector _this, IN TraceFlag traceflag)
* @brief set the traceflag of dx vector
* @param this - a pointer to dx vector
* @param traceflag - int to indicate traceflag is ON or OFF
*/
void dxVectorSetTraceFlag(IN OUT DxVector _this, IN TraceFlag traceflag);

#ifdef  __cplusplus
}
#endif

#endif /* DX_VEC_H */
