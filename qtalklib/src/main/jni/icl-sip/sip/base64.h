/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * base64.h
 *
 * $Id: base64.h,v 1.1.1.1 2008/08/01 07:00:25 judy Exp $
 */

#ifndef _BASE64_H
#define _BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>

CCLAPI long	base64decode(char* source, char* target, int targetlen);
CCLAPI long	base64encode(char* source, char* target, int targetlen);

#ifdef __cplusplus
}
#endif

#endif /* _BASE64_H */
