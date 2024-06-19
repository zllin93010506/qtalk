#ifndef SIP_UTILITY_H
#define SIP_UTILITY_H

#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif
#include "callmanager.h"

char *str_replace (char *source, char *find,  char *rep);
int QuSessMediaGetAttribute(pSessMedia media,const char* attrib,const char* attrvalue);
void MediaCodecInfoPrint(MediaCodecInfo *media_info);
#endif
