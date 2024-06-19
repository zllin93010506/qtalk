//
//  sdp_process.h
//  libsofia-sip
//
//  Created by Bill Tocute on 2011/11/3.
//  Copyright 2011年 Quanta. All rights reserved.
//
#ifndef _SDP_PROCESS_H
#define _SDP_PROCESS_H

#include "sip_call.h"
#include "sip_call_engine.h"
#include "ctype.h"
#define TELEPHONE_EVENT "telephone-event"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
using namespace call;
namespace sipcall
{
    int hextodec(const char c) 
    {
        char temp = tolower(c);
        if ('0' <= temp && temp <= '9')
            return temp - '0';
        return temp - 'a' + 10;
    }

    void make_profile_level_id(char *str, int str_len, const int profile,const int level_idc)
    {
        //if(sizeof(str)/sizeof(char) < 6)
        //    return;
        snprintf(str, str_len, "%02xe0%02x",profile,level_idc);
    }

    void parse_profile_level_id(const char *str,MDP *mdp)
    {
        if(strlen(str) < 6)
            return;
#if ENABLE_IOT
        mdp->profile   = hextodec(str[0])*16 + hextodec(str[1]);
        mdp->level_idc = hextodec(str[4])*16 + hextodec(str[5]);
#endif
    }

    void create_profile_level_id(char *profile_str, int profile_str_len, const int product_type,const MDP video_mdp)
    {
#if ENABLE_IOT
        make_profile_level_id(profile_str, profile_str_len, video_mdp.profile,video_mdp.level_idc);
#else
        int profile_id = 0;
        int level_idc  = 0;
        switch (product_type) 
        {
            //ox1F 31 => 720p/30fps  108000  3600(30)
            //ox1E 30 => VGA/25fps    40500  1620(25)
            //ox0D 13 => CIF/30fps    11880   396(30)
            case call::PRODUCT_TYPE_NA:
                break;
            case call::PRODUCT_TYPE_PX: //720p
            case call::PRODUCT_TYPE_PX2:
                profile_id = 66;
                level_idc  = 31;
                break;
            case call::PRODUCT_TYPE_QTA://QVGA VGA
                profile_id = 66;
                level_idc  = 13;
                break;
            case call::PRODUCT_TYPE_QTI://192*144 VGA
                profile_id = 66;
                level_idc  = 13;
                break;
            case call::PRODUCT_TYPE_QTW://VGA
                profile_id = 66;
                level_idc  = 30;
                break;
            default:
                break;
        }
        make_profile_level_id(profile_str, profile_str_len, profile_id,level_idc);
#endif
    }
    
    void SDPTomdpList(sdp_rtpmap_t *rtpmap,std::list<struct call::MDP*> *list)
    {
        //reset old mdp
        empty_mdp_list(list);
        
        //parse new mdp
        while (rtpmap != NULL)
        {
            MDP *mdp_data = new MDP();
            init_mdp(mdp_data);
            strncpy(mdp_data->mime_type,rtpmap->rm_encoding, MINE_TYPE_LENGTH*sizeof(char));
            mdp_data->payload_id = rtpmap->rm_pt;
            mdp_data->clock_rate = rtpmap->rm_rate;
            mdp_data->bitrate    = 0;
            if(rtpmap->rm_fmtp != NULL)
            {
                char *pch =  strtok((char*)rtpmap->rm_fmtp,"=; ");
                while (pch != NULL)
                {
                    if(strcasecmp(pch, "bitrate")==0)
                    {
                        pch = strtok (NULL, "=; ");
                        mdp_data->bitrate = atoi(pch);
                        break;
                    }
#if ENABLE_IOT
                    else if(strcasecmp(pch, "profile-level-id")==0)
                    {
                        pch = strtok (NULL, "=; ");
                        parse_profile_level_id(pch,mdp_data);
                    }
                    else if(strcasecmp(pch, "packetization-mode")==0)
                    {
                        pch = strtok (NULL, "=; ");
                        mdp_data->packetization_mode = atoi(pch);
                    }
                    else if(strcasecmp(pch, "max-fs")==0)
                    {
                        pch = strtok (NULL, "=; ");
                        mdp_data->max_fs = atoi(pch);
                    }
                    else if(strcasecmp(pch, "max-mbps")==0)
                    {
                        pch = strtok (NULL, "=; ");
                        mdp_data->max_mbps = atoi(pch);
                    }
#endif
                    pch = strtok (NULL, "=; ");
                } 
            }
            list->push_back(mdp_data);
            rtpmap = rtpmap->rm_next;
        }
    }
    
    const MDP* mappingMDP(std::list<struct call::MDP*> localMDP,std::list<struct call::MDP*>remoteMDP ,struct call::MDP* final_mdp, bool *pSupportTelEvent)
    {
        if(localMDP.size() == 0 || remoteMDP.size() == 0)
            return NULL;

        //To negotiate the telephone-event 
        if (pSupportTelEvent != NULL)//check pointer
        {
            bool remote_support_tel_event = false;
            bool local_support_tel_event = false;
            for (std::list<struct call::MDP*>::iterator remote=remoteMDP.begin(); remote!=remoteMDP.end(); remote++)
            {
                if(strncasecmp((*remote)->mime_type,TELEPHONE_EVENT, MINE_TYPE_LENGTH*sizeof(char)) == 0 )
                {
                    remote_support_tel_event = true;
                    break;
                }
            }
            for (std::list<struct call::MDP*>::iterator local=localMDP.begin(); local!=localMDP.end(); local++)
            {
                if(strncasecmp((*local)->mime_type, TELEPHONE_EVENT, MINE_TYPE_LENGTH*sizeof(char)) == 0) 
                {
                    local_support_tel_event = true;
                    break;
                }
            }
            *pSupportTelEvent = remote_support_tel_event && local_support_tel_event;
        }
        //Refine to fix mapping error issue of remote answer with multipule audio codecs
        //Remote codec first
        for (std::list<struct call::MDP*>::iterator remote=remoteMDP.begin(); remote!=remoteMDP.end(); remote++)
        {     
            if(strncasecmp((*remote)->mime_type,TELEPHONE_EVENT, MINE_TYPE_LENGTH*sizeof(char)) == 0 )
                continue;
            for (std::list<struct call::MDP*>::iterator local=localMDP.begin(); local!=localMDP.end(); local++)
            {   
                if(strncasecmp((*local)->mime_type, TELEPHONE_EVENT, MINE_TYPE_LENGTH*sizeof(char)) == 0  ) 
                    continue;

                if(strncasecmp((*local)->mime_type,(*remote)->mime_type, MINE_TYPE_LENGTH*sizeof(char)) == 0 && 
                   (*local)->clock_rate            == (*remote)->clock_rate && 
                   (*local)->bitrate               == (*remote)->bitrate &&
                   (((*local)->packetization_mode  == 1)|| 
                    ((*local)->packetization_mode  == (*remote)->packetization_mode)) )
                {
                    //negociate mdp
                    //record self send info (remote can decode)
                    copy_mdp(final_mdp, *local);
                    final_mdp->payload_id   = (*remote)->payload_id;
#if ENABLE_IOT
                    // min(final_mdp->level_idc , (*remote)->level_idc)
                    //if(final_mdp->level_idc > (*remote)->level_idc)
                    //{
                        final_mdp->level_idc = (*remote)->level_idc;
                        final_mdp->max_fs    = (*remote)->max_fs;
                        final_mdp->max_mbps  = (*remote)->max_mbps;
                    //}

                    //Mapping rtcp-fb
                    final_mdp->enable_rtcp_fb_pli = final_mdp->enable_rtcp_fb_pli && (*remote)->enable_rtcp_fb_pli;
                    final_mdp->enable_rtcp_fb_fir = final_mdp->enable_rtcp_fb_fir && (*remote)->enable_rtcp_fb_fir;
                    final_mdp->tias_kbps = min(final_mdp->tias_kbps,(*remote)->tias_kbps);
#endif
                    return (*local);
                }
                
            }
        }
        return NULL;
    }
    
    bool mappingSDP(nua_magic_t  *magic, SipCall *sipCall)
    {
        bool is_audio_match = false;
        bool is_video_match = false;
        //reset mdp
        init_mdp(&sipCall->mCallData.audio_mdp);
        init_mdp(&sipCall->mCallData.video_mdp);
        
        if(magic->local_audio_mdps.size() > 0 && sipCall->remoteAudioMDP.size() > 0)
        {
            if(mappingMDP(magic->local_audio_mdps , sipCall->remoteAudioMDP,&sipCall->mCallData.audio_mdp,&sipCall->mCallData.enable_telephone_event))
            {
                sipCall->mCallData.is_video_call = false;
                is_audio_match = true;
            }
        }
        if (sipCall->remoteAudioMDP.size()==0 || is_audio_match == false )
        {
            sipCall->mCallData.remote_audio_port = 0;
        }
        
        if(magic->local_video_mdps.size() > 0 && sipCall->remoteVideoMDP.size() > 0)
        {
            if(mappingMDP(magic->local_video_mdps , sipCall->remoteVideoMDP,&sipCall->mCallData.video_mdp, NULL))
            {
                sipCall->mCallData.is_video_call = true;
                is_video_match = true;
            }
        }
        if (sipCall->remoteVideoMDP.size()==0 || is_video_match == false)
        {
            sipCall->mCallData.remote_video_port = 0;
        }
        return is_audio_match || is_video_match;
    }
    
    void parseSDP(const char *sdp_string , SipCall  *sipCall,bool is_recv_invite)
    {
        if(sdp_string == NULL)
            return;
        
        su_home_t g_su_home;
        su_home_init(&g_su_home);
        sdp_parser_t *local_parser = sdp_parse(&g_su_home, sdp_string, strlen(sdp_string), sdp_f_insane);
        sdp_session_t *l_sdp = sdp_session(local_parser);
        
        printf("receiver SDP \n%s",sdp_string);

        if (l_sdp->sdp_media != NULL)
        {
            //get product_type
            //sdp_attribute_t  *attributes = l_sdp->sdp_media->m_attributes;
            while (l_sdp->sdp_media->m_attributes != NULL) 
            {
                if(strcasecmp(l_sdp->sdp_media->m_attributes->a_name, "tool") == 0)
                {
                    if(strstr(l_sdp->sdp_media->m_attributes->a_value,"Quanta") != NULL)
                    {
                        if(strstr(l_sdp->sdp_media->m_attributes->a_value,"ET1") != NULL)
                            sipCall->mCallData.product_type = PRODUCT_TYPE_PX;
                        else if(strstr(l_sdp->sdp_media->m_attributes->a_value,"QT1.5A") != NULL)
                            sipCall->mCallData.product_type = PRODUCT_TYPE_QTA;
                        else if(strstr(l_sdp->sdp_media->m_attributes->a_value,"QT1.5W") != NULL)
                            sipCall->mCallData.product_type = PRODUCT_TYPE_QTW;
                        else if(strstr(l_sdp->sdp_media->m_attributes->a_value,"QT1.5I") != NULL)
                            sipCall->mCallData.product_type = PRODUCT_TYPE_QTI;
                        else if(strstr(l_sdp->sdp_media->m_attributes->a_value,"PX2") != NULL)
                            sipCall->mCallData.product_type = PRODUCT_TYPE_PX2;
                    }
                }
                l_sdp->sdp_media->m_attributes = l_sdp->sdp_media->m_attributes->a_next;
            }
            //l_sdp->sdp_media->m_attributes = attributes;
        }
        
        empty_mdp_list(&sipCall->remoteAudioMDP);
        empty_mdp_list(&sipCall->remoteVideoMDP);

        sipCall->mCallData.remote_video_port = 0;
        sipCall->mCallData.remote_audio_port = 0;
        sipCall->mCallData.is_video_call = false;
        //bool is_audio_hold = false;
        //bool is_video_hold = false;

        while (l_sdp->sdp_media != NULL)
        {
            if (strcasecmp(l_sdp->sdp_media->m_type_name,"audio") == 0) 
            {
                memset(sipCall->mCallData.remote_audio_ip,0,sizeof(sipCall->mCallData.remote_audio_ip));
                if(l_sdp->sdp_connection != NULL && l_sdp->sdp_connection->c_address != NULL)
                    strcpy(sipCall->mCallData.remote_audio_ip, l_sdp->sdp_connection->c_address);
                if(l_sdp->sdp_media->m_connections != NULL && l_sdp->sdp_media->m_connections->c_address != NULL)
                    strcpy(sipCall->mCallData.remote_audio_ip, l_sdp->sdp_media->m_connections->c_address);
                sipCall->mCallData.remote_audio_port = (unsigned int)l_sdp->sdp_media->m_port;
                if (sipCall->mCallData.remote_audio_port != 0) 
                {
                    SDPTomdpList(l_sdp->sdp_media->m_rtpmaps,&sipCall->remoteAudioMDP);
                    sipCall->mCallData.is_video_call = false;
                }
#if ENABLE_IOT
                //parse media level bandwidth
                while( l_sdp->sdp_media->m_bandwidths != NULL && l_sdp->sdp_media->m_bandwidths->b_modifier_name != NULL)
                {
                    if( strcasecmp(l_sdp->sdp_media->m_bandwidths->b_modifier_name, "TIAS")==0 )
                    {
                        for (std::list<struct call::MDP*>::iterator remote=sipCall->remoteAudioMDP.begin(); remote!=sipCall->remoteAudioMDP.end(); remote++)
                        {
                            (*remote)->tias_kbps = l_sdp->sdp_media->m_bandwidths->b_value/1000;
                        }
                    }
                    
                    l_sdp->sdp_media->m_bandwidths = l_sdp->sdp_media->m_bandwidths->b_next;
                }
#endif
                /*sofia sip can not parse
                 while (l_sdp->sdp_media->m_attributes != NULL) 
                 {
                 if(strcasecmp(l_sdp->sdp_media->m_attributes->a_name, "inactive") == 0)
                 {
                    is_audio_hold = true;
                 }
                 l_sdp->sdp_media->m_attributes = l_sdp->sdp_media->m_attributes->a_next;
                 }*/
            }
            else if (strcasecmp(l_sdp->sdp_media->m_type_name,"video") == 0) 
            {
                memset(sipCall->mCallData.remote_video_ip,0,sizeof(sipCall->mCallData.remote_video_ip));
                if(l_sdp->sdp_connection != NULL && l_sdp->sdp_connection->c_address != NULL)
                    strcpy(sipCall->mCallData.remote_video_ip, l_sdp->sdp_connection->c_address);
                if(l_sdp->sdp_media->m_connections != NULL && l_sdp->sdp_media->m_connections->c_address != NULL)
                    strcpy(sipCall->mCallData.remote_video_ip, l_sdp->sdp_media->m_connections->c_address);
                sipCall->mCallData.remote_video_port = (unsigned int)l_sdp->sdp_media->m_port;
                if (sipCall->mCallData.remote_video_port != 0) 
                {
                    SDPTomdpList(l_sdp->sdp_media->m_rtpmaps,&sipCall->remoteVideoMDP);
                    sipCall->mCallData.is_video_call = true;
                }
                
#if ENABLE_IOT
                //parse media level bandwidth
                while( l_sdp->sdp_media->m_bandwidths != NULL && l_sdp->sdp_media->m_bandwidths->b_modifier_name != NULL)
                {
                    if( strcasecmp(l_sdp->sdp_media->m_bandwidths->b_modifier_name, "TIAS")==0 )
                    {
                        for (std::list<struct call::MDP*>::iterator remote=sipCall->remoteVideoMDP.begin(); remote!=sipCall->remoteVideoMDP.end(); remote++)
                        {
                            (*remote)->tias_kbps = l_sdp->sdp_media->m_bandwidths->b_value/1000;
                        }
                    }
                    
                    l_sdp->sdp_media->m_bandwidths = l_sdp->sdp_media->m_bandwidths->b_next;
                }
                //parse media attribute to get rtcp-fb
                while (l_sdp->sdp_media->m_attributes != NULL) 
                {
                    if(strcasecmp(l_sdp->sdp_media->m_attributes->a_name, "rtcp-fb") == 0)
                    {
                        char *pch = strtok((char*)l_sdp->sdp_media->m_attributes->a_value,"=; ");
                        while (pch != NULL)
                        {
                            //a=rtcp-fb:ccm fir
                            if(strcasecmp(pch, "fir")==0)
                            {
                                for (std::list<struct call::MDP*>::iterator remote=sipCall->remoteVideoMDP.begin(); remote!=sipCall->remoteVideoMDP.end(); remote++)
                                {
                                    (*remote)->enable_rtcp_fb_fir = true;
                                }
                            }
                            //a=rtcp-fb:nack pli
                            else if(strcasecmp(pch, "pli")==0)
                            {
                                for (std::list<struct call::MDP*>::iterator remote=sipCall->remoteVideoMDP.begin(); remote!=sipCall->remoteVideoMDP.end(); remote++)
                                {
                                    (*remote)->enable_rtcp_fb_pli = true;
                                }
                            }
                            pch = strtok (NULL, "=; ");
                        }
                    }/*
                    else if(strcasecmp(l_sdp->sdp_media->m_attributes->a_name, "inactive") == 0)
                    {
                        //sofia sip can not parse
                    }*/
                    l_sdp->sdp_media->m_attributes = l_sdp->sdp_media->m_attributes->a_next;
                }
#endif
            }
            l_sdp->sdp_media = l_sdp->sdp_media->m_next;
        }
        // get hold state
        if(is_recv_invite)
        {
            bool is_hold = false;
            while (l_sdp->sdp_attributes != NULL) 
            {
                if(strcasecmp(l_sdp->sdp_attributes->a_name, "inactive") == 0 ||
                   strcasecmp(l_sdp->sdp_attributes->a_name, "sendonly") == 0)
                {
                    is_hold = true;
                }
                l_sdp->sdp_attributes = l_sdp->sdp_attributes->a_next;
            }
            
            if (strcmp(sipCall->mCallData.remote_audio_ip, "0.0.0.0")==0 ||
                is_hold       == true ||
                strstr(sdp_string, "inactive") != NULL ||
                strstr(sdp_string, "sendonly") != NULL )
            {
                sipCall->mCallData.is_recv_hold = true;
            }
            else if(sipCall->mCallData.call_state == CALL_STATE_HOLD)
            {
                sipCall->mCallData.is_recv_hold = false;
            }
            
            //setup the hold state
            if (sipCall->mCallData.is_send_hold || sipCall->mCallData.is_recv_hold)
                sipCall->mCallData.call_state = CALL_STATE_HOLD;
            else if (sipCall->mCallData.call_state == CALL_STATE_HOLD)
                sipCall->mCallData.call_state = CALL_STATE_UNHOLD;
        }

#if ENABLE_IOT
        //parse session level bandwidth
        while( l_sdp->sdp_bandwidths != NULL && l_sdp->sdp_bandwidths->b_modifier_name )
        {
            if( strcasecmp(l_sdp->sdp_bandwidths->b_modifier_name, "AS")==0 )
                sipCall->mCallData.remote_as = l_sdp->sdp_bandwidths->b_value;
            else if( strcasecmp(l_sdp->sdp_bandwidths->b_modifier_name, "CT")==0 )
                sipCall->mCallData.remote_ct = l_sdp->sdp_bandwidths->b_value;
            
            l_sdp->sdp_bandwidths = l_sdp->sdp_bandwidths->b_next;
        }
#endif
        //sdp_printer_t *printer = sdp_print(&g_su_home, l_sdp, NULL, 0, sdp_f_config | sdp_f_insane);
        //const char *l_sdp_str = sdp_message(printer);
        //sdp_printer_free(printer);
        sdp_parser_free(local_parser);
        su_home_deinit(&g_su_home);
    }
    
    void makeSDP(EngineData engineData,const CallData  callData,char* sdp_string, int sdp_string_size)
    {
        memset(sdp_string, 0, sdp_string_size);
        //sprintf(sdp_string, "c=IN IP4 %s\r\n",callData.local_audio_ip);

        //a=inactive
        //if(callData.call_state == CALL_STATE_HOLD)
        //    sprintf(sdp_string, "%sa=inactive\r\n",sdp_string);
#if ENABLE_IOT
        //b=AS:448
        //if(callData.local_as != 0)
        //    sprintf(sdp_string, "%sb=AS:%d\r\n",sdp_string,callData.local_as);
        //if(callData.local_ct != 0)
        //    sprintf(sdp_string, "%sb=CT:%d\r\n",sdp_string,callData.local_ct);
        //b=CT:video tias
        struct call::MDP* temp_vv = NULL;
        if(engineData.local_video_mdps.size() > 0)
            temp_vv = *(engineData.local_video_mdps.begin());
        if(temp_vv != NULL && temp_vv->tias_kbps != 0)
            snprintf(sdp_string, sdp_string_size, "%sb=CT:%d\r\n",sdp_string,temp_vv->tias_kbps);
#endif  
        //make audio sdp
        if(engineData.local_audio_mdps.size() > 0 && callData.local_audio_port != 0) //port = 0 disable
        {
            //m=audio 4444 RTP/AVP 0 8\r\n a=rtpmap:0 PCMU/8000\r\n a=rtpmap:8 PCMA/8000\r\n 
            snprintf(sdp_string, sdp_string_size, "%sm=audio %d RTP/AVP",sdp_string,callData.local_audio_port);
            if(callData.is_reinvite == true && strnlen(callData.audio_mdp.mime_type) != 0, MINE_TYPE_LENGTH)
            {
                snprintf(sdp_string, sdp_string_size, "%s %d",sdp_string,callData.audio_mdp.payload_id);
                //check to append telephone-event
                if (callData.enable_telephone_event)
                {   //a=rtpmap:101 telephone-event/8000
                    snprintf(sdp_string, sdp_string_size, "%s %d\r\na=rtpmap:%d %s/%d\r\n",sdp_string,101,101,TELEPHONE_EVENT,8000);
                }
                else
                {
                    snprintf(sdp_string, sdp_string_size, "%s\r\n",sdp_string);
                }
                //sprintf(sdp_string, "%sc=IN IP4 %s\r\n",sdp_string,callData.local_audio_ip);
                //a=rtpmap:0 PCMU/8000\r\n
                snprintf(sdp_string, sdp_string_size, "%sa=rtpmap:%d %s/%d\r\n",sdp_string,callData.audio_mdp.payload_id,callData.audio_mdp.mime_type,callData.audio_mdp.clock_rate);
            
                if(callData.audio_mdp.bitrate != 0)
                    snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d bitrate=%d\r\n",sdp_string,callData.audio_mdp.payload_id,callData.audio_mdp.bitrate);
            }
            else  //first make call
            {
                for (std::list<struct call::MDP*>::iterator it=engineData.local_audio_mdps.begin(); it!=engineData.local_audio_mdps.end() ; it++)
                {
                    snprintf(sdp_string, sdp_string_size, "%s %d",sdp_string,(*it)->payload_id);
                }
                snprintf(sdp_string, sdp_string_size, "%s\r\n",sdp_string);
                //sprintf(sdp_string, "%sc=IN IP4 %s\r\n",sdp_string,callData.local_audio_ip);
                //a=rtpmap:0 PCMU/8000\r\n
                for (std::list<struct call::MDP*>::iterator it=engineData.local_audio_mdps.begin(); it!=engineData.local_audio_mdps.end(); it++)
                {
                    snprintf(sdp_string, sdp_string_size, "%sa=rtpmap:%d %s/%d\r\n",sdp_string,(*it)->payload_id,(*it)->mime_type,(*it)->clock_rate);
                
                    if((*it)->bitrate != 0)
                        snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d bitrate=%d\r\n",sdp_string,(*it)->payload_id,(*it)->bitrate);
                }
            }
            //a=ptime:20
            snprintf(sdp_string, sdp_string_size, "%sa=ptime:20\r\n",sdp_string);
#if ENABLE_IOT
            //b=tias:???
            //struct call::MDP* temp_a = *(engineData.local_audio_mdps.begin());
            //if(temp_a != NULL && temp_a->tias_kbps != 0)
            //    sprintf(sdp_string, "%sb=TIAS:%u\r\n",sdp_string,temp_a->tias_kbps*1000);
#endif
            //a=tool:Quanta QT1.5A_0.0.1
            switch (engineData.product_type) 
            {
                case call::PRODUCT_TYPE_NA:
                    break;
                case call::PRODUCT_TYPE_PX:
                    snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta ET1_0.0.1\r\n",sdp_string);
                    break;
                case call::PRODUCT_TYPE_PX2:
                    snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta PX2_0.0.1\r\n",sdp_string);
                    break;
                case call::PRODUCT_TYPE_QTA:
                    snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta QT1.5A_0.0.1\r\n",sdp_string);
                    break;
                case call::PRODUCT_TYPE_QTI:
                    snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta QT1.5I_0.0.1\r\n",sdp_string);
                    break;
                case call::PRODUCT_TYPE_QTW:
                    snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta QT1.5W_0.0.1\r\n",sdp_string);
                    break;
                default:
                    break;
            } 
        }
        //make video sdp
        if(engineData.local_video_mdps.size() > 0 && callData.local_video_port != 0)
        {
            //m=video 6666 RTP/AVP 109\r\n a=rtpmap:109 H264/90000
            snprintf(sdp_string, sdp_string_size, "%sm=video %d RTP/AVP",sdp_string,callData.local_video_port);
            if(callData.is_reinvite == true && strlen(callData.video_mdp.mime_type) != 0)
            {
                snprintf(sdp_string, sdp_string_size, "%s %d\r\n",sdp_string,callData.video_mdp.payload_id);
                //sprintf(sdp_string, "%sc=IN IP4 %s\r\n",sdp_string,callData.local_video_ip);
                //a=rtpmap:0 PCMU/8000\r\n
                snprintf(sdp_string, sdp_string_size, "%sa=rtpmap:%d %s/%d\r\n",sdp_string,callData.video_mdp.payload_id, callData.video_mdp.mime_type, callData.video_mdp.clock_rate);
            
                if(strcasecmp("H264",callData.video_mdp.mime_type)==0)
                {
                    int profile_str_len = 7;
                    char profile_str[profile_str_len];
                    create_profile_level_id(profile_str, profile_str_len, engineData.product_type,callData.video_mdp);
                    
#if ENABLE_IOT
                    if(callData.video_mdp.packetization_mode == 1)
                        snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;packetization-mode=%d;max-mbps=%d;max-fs=%d\r\n",sdp_string,callData.video_mdp.payload_id,profile_str,callData.video_mdp.packetization_mode,callData.video_mdp.max_mbps,callData.video_mdp.max_fs);
                    else
                        snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;max-mbps=%d;max-fs=%d\r\n",sdp_string,callData.video_mdp.payload_id,profile_str,callData.video_mdp.max_mbps,callData.video_mdp.max_fs);
#else
                    snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;packetization-mode=%d\r\n",sdp_string,callData.video_mdp.payload_id,profile_str,1);
#endif
                }
            }
            else //first make call
            {
                for (std::list<struct call::MDP*>::iterator it=engineData.local_video_mdps.begin(); it!=engineData.local_video_mdps.end() ; it++)
                {
                    snprintf(sdp_string, sdp_string_size, "%s %d",sdp_string,(*it)->payload_id);
                }
                snprintf(sdp_string, sdp_string_size, "%s\r\n",sdp_string);
                //sprintf(sdp_string, "%sc=IN IP4 %s\r\n",sdp_string,callData.local_video_ip);

                for (std::list<struct call::MDP*>::iterator it=engineData.local_video_mdps.begin(); it!=engineData.local_video_mdps.end(); it++)
                {
                    snprintf(sdp_string, sdp_string_size, "%sa=rtpmap:%d %s/%d\r\n",sdp_string,(*it)->payload_id,(*it)->mime_type,(*it)->clock_rate);
                
                    //a=fmtp:109 profile-level-id=42E01E;packetization-mode=1
                    if(strncasecmp("H264", (*it)->mime_type, MINE_TYPE_LENGTH*sizeof(char))==0)
                    {
                        int profile_str_len = 7;
                        char profile_str[profile_str_len];
                        create_profile_level_id(profile_str, profile_str_len,engineData.product_type,*(*it));
#if ENABLE_IOT
                        if((*it)->packetization_mode == 1)
                            snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;packetization-mode=%d;max-mbps=%d;max-fs=%d\r\n",sdp_string,(*it)->payload_id,profile_str,(*it)->packetization_mode,(*it)->max_mbps,(*it)->max_fs);
                        else
                            snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;max-mbps=%d;max-fs=%d\r\n",sdp_string,(*it)->payload_id,profile_str,(*it)->max_mbps,(*it)->max_fs);
#else
                        snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;packetization-mode=%d\r\n",sdp_string,(*it)->payload_id,profile_str,1);
#endif
                    }
                }
            }
            //a=ptime:20
            snprintf(sdp_string, sdp_string_size, "%sa=ptime:20\r\n",sdp_string);
#if ENABLE_IOT
            //b=tias:???
            struct call::MDP* temp_v = *(engineData.local_video_mdps.begin());
            if(temp_v != NULL)
            {
                if(temp_v->tias_kbps != 0)
                    snprintf(sdp_string, sdp_string_size, "%sb=TIAS:%u\r\n",sdp_string,temp_v->tias_kbps*1000);
                if(temp_v->enable_rtcp_fb_fir)
                    snprintf(sdp_string, sdp_string_size, "%sa=rtcp-fb:* ccm fir\r\n",sdp_string);
                if(temp_v->enable_rtcp_fb_pli)
                    snprintf(sdp_string, sdp_string_size, "%sa=rtcp-fb:* nack pli\r\n",sdp_string);
            }
#endif
        }
        int temp = time(NULL)%1000;
        snprintf(sdp_string, sdp_string_size, "%sa=qtx:%d\r\n",sdp_string,temp);
        
        //const char *CAPS = "v=0\nm=audio 0 RTP/AVP 0\na=rtpmap:0 PCMU/8000";
        /*su_home_t g_su_home;
        su_home_init(&g_su_home);
        sdp_parser_t *local_parser = sdp_parse(&g_su_home, sdp_string, strlen(sdp_string), sdp_f_insane);
        sdp_session_t *l_sdp       = sdp_session(local_parser);
        if(l_sdp && l_sdp->sdp_media)
        {
            l_sdp->sdp_origin->o_version = 16384;
            l_sdp->sdp_origin->o_id      = 16384;
        }
        sdp_printer_t *printer = sdp_print(NULL, l_sdp, NULL, 0, sdp_f_config | sdp_f_insane);
        sdp_string = (char*)sdp_message(printer);
        sdp_printer_free(printer);
        sdp_parser_free(local_parser);
        su_home_deinit(&g_su_home);*/
    }
    
    bool answerSDP(EngineData engineData,SipCall *sipCall,char* sdp_string, int sdp_string_size)
    {
        memset(sdp_string, 0, sdp_string_size);
        bool is_match = false;  
        
        //reset mdp
        init_mdp(&sipCall->mCallData.audio_mdp);
        init_mdp(&sipCall->mCallData.video_mdp);
        
#if ENABLE_IOT
        //b=AS:448
        //if(sipCall->mCallData.remote_as != 0)
        //    snprintf(sdp_string, sdp_string_size, "%sb=AS:%u\r\n",sdp_string,sipCall->mCallData.remote_as);
        //if(sipCall->mCallData.remote_ct != 0)
        //    snprintf(sdp_string, sdp_string_size, "%sb=CT:%u\r\n",sdp_string,sipCall->mCallData.remote_ct);
        //b=CT:video tias
        struct call::MDP* temp_vv = NULL;
        if(engineData.local_video_mdps.size() > 0)
            temp_vv = *(engineData.local_video_mdps.begin());
        if(temp_vv != NULL && temp_vv->tias_kbps != 0)
            snprintf(sdp_string, sdp_string_size, "%sb=CT:%d\r\n",sdp_string,temp_vv->tias_kbps);
#endif  
        
        //answer audio sdp
        if(engineData.local_audio_mdps.size() > 0 && sipCall->remoteAudioMDP.size() > 0 && sipCall->mCallData.local_audio_port != 0)
        {
            //m=audio 4444 RTP/AVP 0 \r\n a=rtpmap:0 PCMU/8000\r\n
            if(mappingMDP(engineData.local_audio_mdps,sipCall->remoteAudioMDP,&sipCall->mCallData.audio_mdp, &sipCall->mCallData.enable_telephone_event))
            {
                const MDP* temp = &sipCall->mCallData.audio_mdp;
                //sprintf(sdp_string, "c=IN IP4 %s\r\n",sipCall->mCallData.local_audio_ip);
                snprintf(sdp_string, sdp_string_size, "%sm=audio %d RTP/AVP %d",sdp_string,sipCall->mCallData.local_audio_port,temp->payload_id);
                //check to append telephone-event
                if (sipCall->mCallData.enable_telephone_event)
                { 
                    //a=rtpmap:101 telephone-event/8000
                    snprintf(sdp_string, sdp_string_size, "%s %d\r\na=rtpmap:%d %s/%d\r\n",sdp_string,101,101,TELEPHONE_EVENT,8000);
                }
                else
                {
                    snprintf(sdp_string, sdp_string_size, "%s\r\n",sdp_string);
                }
                //sprintf(sdp_string, "%sc=IN IP4 %s\r\n",sdp_string,sipCall->mCallData.local_audio_ip);
                snprintf(sdp_string, sdp_string_size, "%sa=rtpmap:%d %s/%d\r\n",sdp_string,temp->payload_id,temp->mime_type,temp->clock_rate);
                
                if(temp->bitrate != 0)
                    snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d bitrate=%d\r\n",sdp_string,temp->payload_id,temp->bitrate);
                
                snprintf(sdp_string, "%sa=ptime:20\r\n",sdp_string);
#if ENABLE_IOT
                if(temp->tias_kbps != 0)
                    snprintf(sdp_string, sdp_string_size, "%sb=TIAS:%u\r\n",sdp_string,temp->tias_kbps*1000);
#endif
                sipCall->mCallData.is_video_call = false;
                
                //a=tool:Quanta QT1.5A_0.0.1
                switch (engineData.product_type) 
                {
                    case call::PRODUCT_TYPE_NA:
                        break;
                    case call::PRODUCT_TYPE_PX:
                        snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta ET1_0.0.1\r\n",sdp_string);
                        break;
                    case call::PRODUCT_TYPE_PX2:
                        snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta PX2_0.0.1\r\n",sdp_string);
                        break;
                    case call::PRODUCT_TYPE_QTA:
                        snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta QT1.5A_0.0.1\r\n",sdp_string);
                        break;
                    case call::PRODUCT_TYPE_QTI:
                        snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta QT1.5I_0.0.1\r\n",sdp_string);
                        break;
                    case call::PRODUCT_TYPE_QTW:
                        snprintf(sdp_string, sdp_string_size, "%sa=tool:Quanta QT1.5W_0.0.1\r\n",sdp_string);
                        break;
                    default:
                        break;
                }
                is_match = true;
            }
        }
        if(is_match == false)
            return is_match;
        //answer video sdp
        if(engineData.local_video_mdps.size() > 0 && sipCall->remoteVideoMDP.size() > 0 &&  sipCall->mCallData.local_video_port != 0)
        {
            //m=video 6666 RTP/AVP 109\r\n a=rtpmap:109 H264/90000
            const MDP* temp = mappingMDP(engineData.local_video_mdps,sipCall->remoteVideoMDP,&sipCall->mCallData.video_mdp, NULL);
            if(temp != NULL)
            {
                const unsigned int temp_payload_id = sipCall->mCallData.video_mdp.payload_id;
                snprintf(sdp_string, sdp_string_size, "%sm=video %d RTP/AVP %d \r\n",sdp_string,sipCall->mCallData.local_video_port,temp_payload_id);
                //sprintf(sdp_string, "%sc=IN IP4 %s\r\n",sdp_string,sipCall->mCallData.local_video_ip);//Setup connection information
                snprintf(sdp_string, sdp_string_size, "%sa=rtpmap:%d %s/%d\r\n",sdp_string,temp_payload_id,temp->mime_type,temp->clock_rate);
                
                if(temp->bitrate != 0)
                    snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d bitrate=%d\r\n",sdp_string,temp_payload_id,temp->bitrate);
                
                else if(strcasecmp("H264",temp->mime_type)==0)
                {
                    int profile_str_len = 7;
                    char profile_str[profile_str_len];
                    create_profile_level_id(profile_str,profile_str_len,engineData.product_type,*temp);
#if ENABLE_IOT
                    if(temp->packetization_mode == 1)
                        snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;packetization-mode=%d;max-mbps=%d;max-fs=%d\r\n",sdp_string,temp_payload_id, profile_str,temp->packetization_mode, temp->max_mbps, temp->max_fs);
                    else
                        snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;max-mbps=%d;max-fs=%d\r\n",sdp_string,temp_payload_id, profile_str, temp->max_mbps, temp->max_fs);
#else
                    snprintf(sdp_string, sdp_string_size, "%sa=fmtp:%d profile-level-id=%s;packetization-mode=%d\r\n",sdp_string,temp_payload_id, profile_str,1);
#endif
                }
                
                snprintf(sdp_string, sdp_string_size, "%sa=ptime:20\r\n",sdp_string);
#if ENABLE_IOT
                if(temp->tias_kbps != 0)
                    snprintf(sdp_string, sdp_string_size, "%sb=TIAS:%u\r\n",sdp_string,temp->tias_kbps*1000);
                if(sipCall->mCallData.video_mdp.enable_rtcp_fb_fir)
                    snprintf(sdp_string, sdp_string_size, "%sa=rtcp-fb:* ccm fir\r\n",sdp_string);
                if(sipCall->mCallData.video_mdp.enable_rtcp_fb_pli)
                    snprintf(sdp_string, sdp_string_size, "%sa=rtcp-fb:* nack pli\r\n",sdp_string);
#endif        
                sipCall->mCallData.is_video_call = true;
                is_match = true;
            }
        }
        return is_match;
    }
}

#endif