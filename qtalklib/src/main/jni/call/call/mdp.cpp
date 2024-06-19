//
//  Utility.cpp
//  libsofia-sip
//
//  Created by Bill Tocute on 2011/8/19.
//  Copyright 2011年 Quanta. All rights reserved.
//  Steven say to do it 

#include "mdp.h"
#include "logging.h"

#ifdef WIN32
/* Define this as locale-independent strcmp().  */
#define strcasecmp  _stricmp
#endif
//#define TAG "mdp"
#define TAG "SR2_CDRC"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

void empty_mdp_list(std::list<struct call::MDP*> *list)
{
    while(list->size()>0)
    {
        void* data = list->back();
        list->pop_back();
        free(data);
    }
    list->clear();
}

void init_mdp(call::MDP *mdp)
{
    mdp->payload_id  = 0;
    mdp->clock_rate  = 0;
    mdp->bitrate     = 0;
    memset(mdp->mime_type , 0, MINE_TYPE_LENGTH*sizeof(char));
#if ENABLE_IOT
    mdp->profile    = 0;
    mdp->level_idc  = 0;
    mdp->max_mbps   = 0;
    mdp->max_fs     = 0;
    mdp->tias_kbps  = 0;
    mdp->packetization_mode = 0;
    mdp->enable_rtcp_fb_fir = false;
    mdp->enable_rtcp_fb_pli = false;
#endif
}

void copy_mdp(call::MDP *tar,const call::MDP *src)
{
    tar->payload_id  = src->payload_id;
    tar->clock_rate  = src->clock_rate;
    tar->bitrate     = src->bitrate;
    strncpy(tar->mime_type, src->mime_type, MINE_TYPE_LENGTH*sizeof(char));
#if ENABLE_IOT    
    tar->profile    = src->profile;
    tar->level_idc  = src->level_idc;
    tar->max_mbps   = src->max_mbps;
    tar->max_fs     = src->max_fs;
    tar->tias_kbps  = src->tias_kbps;
    tar->packetization_mode = src->packetization_mode;
    tar->enable_rtcp_fb_fir = src->enable_rtcp_fb_fir;
    tar->enable_rtcp_fb_pli = src->enable_rtcp_fb_pli;
#endif
}

#if ENABLE_IOT
#define SET_TIAS(x,y) if (x && x->tias_kbps<=y) x->tias_kbps = y;

void iot_set_mdp(unsigned int width,unsigned int height,unsigned int fps,unsigned int kbps,unsigned int packetization_mode,bool enablePLI, bool enableFIR,call::MDP *mdp)
{
	LOGD("width: %d, height: %d, fps: %d, kbps: %d",width,height,fps,kbps);
    if(strncasecmp(mdp->mime_type, "H264", MINE_TYPE_LENGTH*sizeof(char))==0 && width!=0 && height!=0 && fps!=0)
    {
        if(mdp->profile == 0)
            mdp->profile = 66;

        mdp->packetization_mode = packetization_mode;
        mdp->tias_kbps = kbps;
        if(width <= 176 && height <= 144 && fps <= 30)       //ox0B 11 => QVGA/10fps   3000  396
        {
            mdp->level_idc = 11;                             //android (low) iPhone(low)
            mdp->max_fs = 396;
            SET_TIAS(mdp,192);
        }
        else if(width <= 320 && height <= 240 && fps <= 10)  //ox0B 11 => QVGA/10fps   3000  396
        {
            mdp->level_idc = 11;                             //android (low) iPhone(low)
            mdp->max_fs = 396;
            SET_TIAS(mdp,192);
        }
        else if(width <= 352 && height <= 288 && fps <= 15)  //ox0C 12 =>  CIF/30fps  11880  396
        {    
            mdp->level_idc = 12;
            mdp->max_fs = 396;
            SET_TIAS(mdp,384);
        }
        else if(width <= 352 && height <= 288 && fps <= 30)  //ox14 20 =>  QVGA/30fps  11880  396
        {    
            mdp->level_idc = 20;
            mdp->max_fs = 396;
            SET_TIAS(mdp,396);
        }
        else if(width <= 352 && height <= 480 && fps <= 30)  //ox15 21 =>     19800  792
        {    
            mdp->level_idc = 21;
            mdp->max_fs = 792;
            SET_TIAS(mdp,450);
        }
        else if(width <= 640 && height <= 480 && fps <= 15)  //ox16 22 => VGA/15fps   20250  1620
        {
            mdp->level_idc = 22;                             //android (med) iPhone(med) QTw
            mdp->max_fs = 1620;
            SET_TIAS(mdp,800);
        }
        else if(width <= 720 && height <= 480 && fps <= 30)  //ox1E 30 => VGA/25fps   40500  1620 
        {
            mdp->level_idc = 30;                             //QTw
            mdp->max_fs = 1620;
            SET_TIAS(mdp,1000);
        }
        else if(width <= 1280 && height <= 720 && fps <= 30) //ox1F 31 => 720p/30fps 108000  3600
        {
            mdp->level_idc = 31;
            mdp->max_fs = 3600;
            SET_TIAS(mdp,3000);
        }
        else
        {
            mdp->level_idc = 31;
            mdp->max_fs = 3600;
            SET_TIAS(mdp,2000);
        }
        //mdp->max_fs   = (width*height)/(16*16);
        //mdp->max_mbps = mdp->max_fs * fps;
        mdp->max_fs   = 0;
        mdp->max_mbps = 0;
        mdp->enable_rtcp_fb_pli = enablePLI;
        mdp->enable_rtcp_fb_fir = enableFIR;
    }
}

//http://en.wikipedia.org/wiki/H.264/MPEG-4_AVC#Levels
void iot_get_mdp(const call::MDP mdp ,unsigned int *width,unsigned int *height,unsigned int *fps,unsigned int *kbps, bool *enablePLI,bool *enableFIR)
{
    if(strncasecmp(mdp.mime_type, "H264", MINE_TYPE_LENGTH*sizeof(char))==0 && mdp.level_idc!=0 && width!=NULL && height!=NULL && fps!=NULL && kbps!=NULL)
    {
        if(mdp.level_idc >= 31)      { *width  =1280; *height = 720; *fps = 30; if(mdp.tias_kbps<=0) *kbps = 3000; else *kbps = mdp.tias_kbps;} //14000
        else if(mdp.level_idc >= 30) { *width  = 720; *height = 480; *fps = 30; if(mdp.tias_kbps<=0) *kbps = 1000; else *kbps = mdp.tias_kbps;} //10000
        else if(mdp.level_idc >= 22) { *width  = 640; *height = 480; *fps = 15; if(mdp.tias_kbps<=0) *kbps = 512; else *kbps = mdp.tias_kbps;}  //4000
        else if(mdp.level_idc >= 13) { *width  = 352; *height = 288; *fps = 30; if(mdp.tias_kbps<=0) *kbps = 384; else *kbps = mdp.tias_kbps;}  //768
        else if(mdp.level_idc >= 11) { *width  = 320; *height = 240; *fps = 15; if(mdp.tias_kbps<=0) *kbps = 192; else *kbps = mdp.tias_kbps;}  //192
        else                         { *width  = 176; *height = 144; *fps = 15; if(mdp.tias_kbps<=0) *kbps = 64; else *kbps = mdp.tias_kbps;}   //64
        LOGD("mdp.max_fs: %d, mdp.max_mbps: %d",(int)mdp.max_fs,(int)mdp.max_mbps);
        if ((int)mdp.max_fs> 0 && (int)mdp.max_mbps>0)
		{
			if ((int)mdp.max_fs >= 3600 || (int)mdp.max_mbps >=108000){
				*width  =1280; *height = 720; *fps = 30;
			}else if ((int)mdp.max_fs >= 1620 || (int)mdp.max_mbps >=40500){
				*width  = 720; *height = 480; *fps = 30;
			}else if ((int)mdp.max_fs >= 1620 || (int)mdp.max_mbps >=20250){
				*width  = 640; *height = 480; *fps = 15;
			}else if ((int)mdp.max_fs >= 396 || (int)mdp.max_mbps >=11880){
				*width  = 352; *height = 288; *fps = 30;
			}else{
				*width  = 176; *height = 144; *fps = 15;
			}

		}else{
			//*fps = mdp.max_mbps/mdp.max_fs;
		}

        if((int)mdp.tias_kbps != 0)
            *kbps = mdp.tias_kbps;
        LOGD("*width: %d, *height: %d, *fps: %d, *kbps: %d",*width,*height,*fps,*kbps);
        *enablePLI = mdp.enable_rtcp_fb_pli;
        *enableFIR = mdp.enable_rtcp_fb_fir;
    }
}
#endif
