//
//  sofiasip_eventcallback.h
//  libsofia-sip
//
//  Created by Bill Tocute on 2011/6/24.
//  Copyright 2011年 Quanta. All rights reserved.
//
#include "sip_call_engine.h"
//SOFIAPUBVAR tag_typedef_t soatag_local_sdp_str_ref;

#include <sofia-sip/su.h>
#include <sofia-sip/nua.h> 
#include <sofia-sip/soa_tag.h>
#include <sofia-sip/sip_extra.h>    
#include "sdp_process.h"

#define SIP_PAI "P-Asserted-Identity"

#define SDP_STR_LENGTH 2048
using namespace call;
namespace sipcall
{
    SipCall* findCall(std::list<SipCall*> callList ,const ICall* pcall)
    {
        SipCall* internal_call = NULL;
        if (pcall!=NULL)
        {
            std::list<SipCall*>::iterator it;
            for ( it=callList.begin() ; it != callList.end(); it++ )
            {
                if (*it == pcall)//Call existed in list
                {
                    internal_call = *it;
                    break;
                }
            }
        }
        return internal_call;
    }
    
    SipCall* findCallByHandle(std::list<SipCall*> callList ,const nua_handle_t *nh)
    {
        SipCall* internal_call = NULL;
        if (nh!=NULL)
        {
            std::list<SipCall*>::iterator it;
            for ( it=callList.begin() ; it != callList.end(); it++ )
            {
                if ((*it)->mCallData.call_handle == nh)//Call existed in list
                {
                    internal_call = *it;
                    break;
                }
            }
        }
        return internal_call;
    }
    
    bool releaseCall(nua_magic_t  *magic ,const nua_handle_t *nh ,const char* msg ,bool is_self)
    {
        SipCall *internal_call = findCallByHandle(magic->call_list,nh);
        if(internal_call != NULL)
        {
            //cancel re-invite , back to original session
            /*if(internal_call->mCallData.is_reinvite == true)
            {   
                if(magic->event_listner != NULL && is_self != true)
                {
                    magic->event_listner->onCallHangup(internal_call,msg);
                }
                internal_call->mCallData.call_state = CALL_STATE_ACCEPT;
                internal_call->mCallData.is_reinvite = false;
                return true;
            }
            else*/ if(magic->event_listner != NULL)
            {
                //on session end 
                if(internal_call->mCallData.is_session_start == true)
                {
                    internal_call->mCallData.is_session_start = false;
                    if(is_self != true)
                        magic->event_listner->onSessionEnd(internal_call);
                }
                internal_call->mCallData.call_state = CALL_STATE_HANGUP;
                if(is_self != true)
                    magic->event_listner->onCallHangup(internal_call,msg);
            }

            magic->call_list.remove(internal_call);
            nua_handle_destroy(internal_call->mCallData.call_handle);
            internal_call->mCallData.call_handle = NULL;
            delete internal_call;

            return true;
        }
        return false;
    }
    
    // Authentication
    void sip_do_auth(nua_handle_t *nh, const char *scheme,
                     const char *realm, const char *user,
                     const char *password)
    {
        su_home_t g_su_home;
        su_home_init(&g_su_home);
        char * tmpstr = su_sprintf(&g_su_home, "%s:%s:%s:%s",scheme, realm, user, password);
        
        nua_authenticate(nh, NUTAG_AUTH(tmpstr), TAG_END());
        su_free(&g_su_home, tmpstr);
        su_home_deinit(&g_su_home);
    }
    
    void my_sip_authenticate(nua_handle_t *nh, nua_magic_t  *magic,const sip_t *sip, tagi_t tags[])
    {
        const sip_www_authenticate_t   *wa = sip->sip_www_authenticate;
        const sip_proxy_authenticate_t *pa = sip->sip_proxy_authenticate;
        
        tl_gets(tags,
                SIPTAG_WWW_AUTHENTICATE_REF(wa),
                SIPTAG_PROXY_AUTHENTICATE_REF(pa),
                TAG_NULL());  
        
        if(wa)
            sip_do_auth(nh, wa->au_scheme,
                        msg_params_find(wa->au_params, "realm="),
                        magic->user_name, 
                        magic->password);
        if(pa)
            sip_do_auth(nh, pa->au_scheme,
                        msg_params_find(pa->au_params, "realm="),
                        magic->user_name, 
                        magic->password);
    }
    
    char *parse_PAI(sip_unknown_t *sipUnknown)
    {
        if(sipUnknown == NULL)
            return NULL;
        
        //find P-Asserted-Identity string
        sip_unknown_t *sip_pai = sipUnknown;
        while (sip_pai->un_next != NULL) 
        {
            //parse remote id from P-Asserted-Identity
            if(sip_pai->un_name && sip_pai->un_value && strcasecmp(sip_pai->un_name, SIP_PAI) == 0 )
            {
                //"display_name"<sip:XXXXXXX@proxy >
                //tel:+XXXXXXXXX
                char * pch = strtok ((char*)sip_pai->un_value,":");
                //char * pch = strstr(sip_pai->un_value, "sip:")
                int length = strlen(pch);

                if(pch != NULL 
                   && (pch[length-3]=='s' || pch[length-3]=='S') 
                   && (pch[length-2]=='i' || pch[length-2]=='I')
                   && (pch[length-1]=='p' || pch[length-1]=='P'))
                {   
                    pch = strtok (NULL,"@");
                    if(pch != NULL)
                        return pch;
                }
                else if(pch != NULL 
                   && (pch[length-3]=='t' || pch[length-3]=='T') 
                   && (pch[length-2]=='e' || pch[length-2]=='E')
                   && (pch[length-1]=='l' || pch[length-1]=='L'))
                {   
                    pch = strtok (NULL,"");
                    if(pch != NULL)
                        return pch;
                }
            }
            sip_pai = sip_pai->un_next;
        }
        return NULL;
    }
    //hold/unhold or sdp no change(reinvitefail) 
    CallResult my_auto_answer(SipCall* internal_call,
                              nua_magic_t   *magic)
    {
        //Check input parameter
        if (internal_call== NULL ||
            magic        == NULL )
            return CALL_RESULT_INVALID_INPUT;
        
        //Send acceptCall message 
        if(internal_call->mCallData.is_reinvite == true)
        {
            char sdp_string[SDP_STR_LENGTH];
            char audio_mime[16];
            char video_mime[16];
            memset(audio_mime, 0, sizeof(audio_mime));
            memset(video_mime, 0, sizeof(video_mime));
            strcpy(audio_mime, internal_call->mCallData.audio_mdp.mime_type);
            strcpy(video_mime, internal_call->mCallData.video_mdp.mime_type);  
            
            bool result = answerSDP(*magic,internal_call,sdp_string, SDP_STR_LENGTH*sizeof(char));
            //Doesn't mapping MDP
            if(result == false)
            {
                nua_respond(internal_call->mCallData.call_handle, SIP_415_UNSUPPORTED_MEDIA, TAG_END());
                if(magic->event_listner != NULL)
                { 
                    magic->event_listner->onCallHangup(internal_call,sip_415_Unsupported_media);
                }
                magic->call_list.remove(internal_call);
                nua_handle_destroy(internal_call->mCallData.call_handle);
                internal_call->mCallData.call_handle = NULL;
                
                delete internal_call;
                return CALL_RESULT_FAIL;
            }
            
            //check if A->V or V->A or codec change
            internal_call->mCallData.is_session_change = false;
            if(internal_call->mCallData.is_reinvite)
            {
                if( strcasecmp( audio_mime ,internal_call->mCallData.audio_mdp.mime_type) ||
                    strcasecmp( video_mime ,internal_call->mCallData.video_mdp.mime_type) ||
                    internal_call->mCallData.old_remote_audio_port != internal_call->mCallData.remote_audio_port ||
                    internal_call->mCallData.old_remote_video_port != internal_call->mCallData.remote_video_port 
                   )
                {
                    internal_call->mCallData.is_session_change = true;
                }
            }
            
            printf("\"auto answerSDP\"\n%s\n",sdp_string);
            nua_respond(internal_call->mCallData.call_handle, 
                        SIP_200_OK, 
                        SIPTAG_CONTENT_TYPE_STR("application/sdp"),
                        SOATAG_USER_SDP_STR(sdp_string),
                        SOATAG_ADDRESS(internal_call->mCallData.local_audio_ip),
                        NUTAG_AUTOANSWER(0),
                        TAG_IF(internal_call->mCallData.call_state == CALL_STATE_HOLD,SOATAG_HOLD("#")),
                        TAG_IF(internal_call->mCallData.call_state != CALL_STATE_HOLD,SOATAG_HOLD((char*)NULL)),
                        TAG_IF(internal_call->mCallData.enable_telephone_event,SOATAG_AUDIO_AUX("telephone-event")),
                        TAG_END());
            internal_call->mCallData.mdp_status = MDP_STATUS_FINAL;
        } 
        
        return CALL_RESULT_OK;
        
    }
    void my_r_register(int           status,
                       char const   *phrase,
                       nua_t        *nua,
                       nua_magic_t  *magic,
                       nua_handle_t *nh,
                       nua_hmagic_t *hmagic,
                       sip_t const  *sip,
                       tagi_t        tags[])
    {
        //The reply code is an integer number from 100 to 699 and indicates type of the response. There are 6 classes of responses:
        //1xx are provisional responses. A provisional response is response that tells to its recipient that the associated request was received but result of the processing is not known yet. Provisional responses are sent only when the processing doesn't finish immediately. The sender must stop retransmitting the request upon reception of a provisional response.Typically proxy servers send responses with code 100 when they start processing an INVITE and user agents send responses with code 180 (Ringing) which means that the callee's phone is ringing.
        //2xx responses are positive final responses. A final response is the ultimate response that the originator of the request will ever receive. Therefore final responses express result of the processing of the associated request. Final responses also terminate transactionsi. Responses with code from 200 to 299 are positive responses that means that the request was processed successfully and accepted. For instance a 200 OK response is sent when a user accepts invitation to a session (INVITE request).A UACi may receive several 200 messages to a single INVITE request. This is because a forkingi proxy (described later) can forki the request so it will reach several UASi and each of them will accept the invitation. In this case each response is distinguished by the tag parameter in To header field. Each response represents a distinct dialogi with unambiguous dialog identifier.
        //3xx responses are used to redirect a caller. A redirection response gives information about the user's new location or an alternative service that the caller might use to satisfy the call. Redirection responses are usually sent by proxy servers. When a proxy receives a request and doesn't want or can't process it for any reason, it will send a redirection response to the caller and put another location into the response which the caller might want to try. It can be the location of another proxy or the current location of the callee (from the location database created by a registrar). The caller is then supposed to re-send the request to the new location. 3xx responses are final.
        //4xx are negative final responses. a 4xx response means that the problem is on the sender's side. The request couldn't be processed because it contains bad syntax or cannot be fulfilled at that server.
        //5xx means that the problem is on server's side. The request is apparently valid but the server failed to fulfill it. Clients should usually retry the request later.
        //6xx reply code means that the request cannot be fulfilled at any server. This response is usually sent by a server that has definitive information about a particular user. User agents usually send a 603 Decline response when the user doesn't want to participate in the session.
        if (status >= 200 && status < 300) //2xx responses are positive final responses.
        {
            magic->expire_time = (sip->sip_expires && sip->sip_expires->ex_delta > 0 && sip->sip_expires->ex_delta < magic->expire_time )?((sip->sip_expires->ex_delta)-1):magic->expire_time;
            if(magic->event_listner != NULL)
            {
                magic->event_listner->onLogonState(true,"Login Successully", magic->expire_time, magic->local_ip);
            }

            /*//Copy the the used ip address 
            if (sip->sip_contact && sip->sip_contact->m_url && sip->sip_contact->m_url->url_host)
            {
                nua_set_params(magic->nua,NUTAG_MEDIA_ADDRESS(sip->sip_contact->m_url->url_host),TAG_END());
                if (sip->sip_via !=NULL && sip->sip_via->v_host != NULL)
                {
                    strcpy(magic->local_ip,sip->sip_via->v_host);
                }
                if (sip->sip_contact                   != NULL &&
                    sip->sip_contact->m_url            != NULL &&
                    sip->sip_contact->m_url->url_host  != NULL)
                {
                    strcpy(magic->local_ip,sip->sip_contact->m_url->url_host);
                }
            }
            */
            
            nua_set_hparams(magic->regist_handle, 
                            NUTAG_KEEPALIVE(0),//Disable auto keep alive
                            TAG_END());
        }
        else if(status == 401 || status == 407)
        {
            my_sip_authenticate(nh, magic,sip, tags);
        }
        else if (status >= 400)//408 = Request Timeout
        {  
            if(magic->event_listner != NULL)
                magic->event_listner->onLogonState(false, phrase, 0,magic->local_ip);
        }
    }
    
    void my_r_unregister(int           status,
                         char const   *phrase,
                         nua_t        *nua,
                         nua_magic_t  *magic,
                         nua_handle_t *nh,
                         nua_hmagic_t *hmagic,
                         sip_t const  *sip,
                         tagi_t        tags[])
    {
        if(status >= 200) //503 Service Unavailable
        {
            printf("Unregistered successfully.\n");
            nua_shutdown(magic->nua);
        }
    }
    
    void my_r_invite(int           status,
                     char const   *phrase,
                     nua_t        *nua,
                     nua_magic_t  *magic,
                     nua_handle_t *nh,
                     nua_hmagic_t *hmagic,
                     sip_t const  *sip,
                     tagi_t        tags[])
    {   
        //someone response my invite 
        if(status == 180 || status == 183)  //Ringing
        {
            //send prack for responce 100rel
            if (sip_has_feature(sip->sip_require, "100rel")) 
            {
                sip_rack_t rack[1];
                sip_rack_init(rack);
                rack->ra_response       = sip->sip_rseq->rs_response;
                rack->ra_cseq           = sip->sip_cseq->cs_seq;
                rack->ra_method         = sip->sip_cseq->cs_method;
                rack->ra_method_name    = sip->sip_cseq->cs_method_name;
                
                char contact_url[256];
                snprintf(contact_url,256*sizeof(char),"sip:%s@%s:%u",
                        magic->sip_id,
                        magic->local_ip,
                        magic->listen_port);
                
                nua_prack(nh, 
                          SIPTAG_RACK(rack),
                          //SIPTAG_ALLOW_STR((char*)NULL), 
                          //SIPTAG_SUPPORTED_STR((char*)NULL),
                          SIPTAG_FROM(sip->sip_from),
                          SIPTAG_TO(sip->sip_to),
                          SIPTAG_CALL_ID(sip->sip_call_id),
                          NTATAG_REL100(0), 
                          SIPTAG_CONTACT_STR(contact_url),
                          TAG_END());  
            }

            SipCall *internal_call = findCallByHandle(magic->call_list,nh);
            if(magic->event_listner != NULL && internal_call != NULL)
            {
                // Enter the critical section
                pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
                
                internal_call->mCallData.call_state = CALL_STATE_RINGING;
                strcpy(internal_call->mCallData.call_id, sip->sip_call_id->i_id);
                magic->event_listner->onCallStateChanged(internal_call, internal_call->mCallData.call_state, phrase); 
                
                // 183 early media 來電答鈴
                // multiple dialog send DTMF signal            
                //A (Sofia)                  Proxy                      B
                //|-----------INVITE---------->|                        |
                //|                            |----- INVITE ---------->|
                //|                            |<------ 100 ------------|
                //|<-----------100-------------|                        |
                //|<-----------183-------------|                        |
                //|     (Media Source Proxy)   |                        |
                //|<===========RTP============>|                        |
                //|                            |<------ 200 ------------|
                //|<-----------200-------------|                        |
                //|     (Media Source B)       |                        |
                //|<==========================RTP=======================|
                //|------------ACK------------>|                        |
                //|                            |------- ACK ----------->|
                if(status == 183 && sip->sip_payload && sip->sip_payload->pl_data)
                {
                    //parse SDP
                    parseSDP(sip->sip_payload->pl_data, internal_call,false);
                    bool is_mapping = mappingSDP(magic,internal_call); 
                    if (is_mapping)
                    {
                        if(magic->event_listner != NULL && internal_call != NULL)
                        {
                            magic->event_listner->onCallAnswered(internal_call);
                        }
                    }
                    internal_call->mCallData.mdp_status = MDP_STATUS_FINAL;
                }

                //Leave the critical section
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
            }
        }
        else if (status == 200) 
        {
            nua_ack(nh, TAG_END());
            SipCall *internal_call = findCallByHandle(magic->call_list,nh);
            if(internal_call != NULL)
            {
                // Enter the critical section
                pthread_mutex_lock(&internal_call->mCallData.calldata_lock);

                char audio_mime[16];
                char video_mime[16];
                memset(audio_mime, 0, sizeof(audio_mime));
                memset(video_mime, 0, sizeof(video_mime));
                strcpy(audio_mime, internal_call->mCallData.audio_mdp.mime_type);
                strcpy(video_mime, internal_call->mCallData.video_mdp.mime_type);
           
                // parse SDP
                parseSDP(sip->sip_payload->pl_data, internal_call,false);
                // TODO
                // detect header contact isfocus (MRFS)
                if(sip->sip_contact != NULL && sip->sip_contact->m_params != NULL)
                {
                    if(msg_params_find(sip->sip_contact->m_params,"isfocus") != NULL)
                    {
                        internal_call->mCallData.is_mrf_server = true;
                        internal_call->mCallData.product_type = PRODUCT_TYPE_NA;
                    }
                }
                
                bool is_mapping = mappingSDP(magic,internal_call); 
                if (is_mapping)
                {
                    int have_telephone_event = 0;
                    if(internal_call->remoteAudioMDP.size() == 2)
                    {
                        for (std::list<struct call::MDP*>::iterator remote=internal_call->remoteAudioMDP.begin(); remote!=internal_call->remoteAudioMDP.end(); remote++)
                        {
                            if (strcasecmp((*remote)->mime_type, "telephone-event") == 0)
                            {
                                have_telephone_event = 1;
                            }
                        }
                    }
                    
                    //send invite again for handle 200_OK with multi codec (POLYCOM device)
                    if(internal_call->mCallData.mdp_status == MDP_STATUS_INIT && 
                        (internal_call->remoteAudioMDP.size()- have_telephone_event > 1 || 
                         internal_call->remoteVideoMDP.size() > 1))
                    {
                        //Send make call message
                        char sdp_string[SDP_STR_LENGTH];
                        //makeSDP_ACK(internal_call,sdp_string);
                        answerSDP(*magic,internal_call,sdp_string, SDP_STR_LENGTH*sizeof(char));
                        printf("\"makeSDP_ACK SDP\"\n%s\n",sdp_string);
                        internal_call->mCallData.mdp_status = MDP_STATUS_MULTICODEC;
                        
                        /*Leave the critical section  */
                        pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
                        nua_invite(internal_call->mCallData.call_handle,
                                   SOATAG_ADDRESS(internal_call->mCallData.local_audio_ip),
                                   SOATAG_USER_SDP_STR(sdp_string),
                                   SOATAG_RTP_SORT(SOA_RTP_SORT_REMOTE),
                                   SOATAG_RTP_SELECT(SOA_RTP_SELECT_ALL),
                                   NUTAG_AUTOACK(0),
                                   NUTAG_AUTOANSWER(0),
                                   //TAG_IF(internal_call->mCallData.enable_telephone_event,SOATAG_AUDIO_AUX("telephone-event")),
                                   TAG_END());
                        return;
                    }
                    
                    internal_call->mCallData.is_session_change = false;
                    //re-invite and session change
                    if(internal_call->mCallData.is_reinvite || internal_call->mCallData.is_session_start)
                    {
                        if( strcasecmp( audio_mime ,internal_call->mCallData.audio_mdp.mime_type) ||
                            strcasecmp( video_mime ,internal_call->mCallData.video_mdp.mime_type) ||
                            internal_call->mCallData.old_remote_audio_port != internal_call->mCallData.remote_audio_port ||
                            internal_call->mCallData.old_remote_video_port != internal_call->mCallData.remote_video_port)
                        {
                            internal_call->mCallData.is_session_change = true;
                        }
                    }
                    //first invite
                    else if(magic->event_listner != NULL && internal_call != NULL)
                    {
                        magic->event_listner->onCallAnswered(internal_call);
                        internal_call->mCallData.call_state = CALL_STATE_ACCEPT;
                    }
                }
                //TODO sip_415_Unsupported_media
                //else
                    //releaseCall(magic,nh,sip_415_Unsupported_media,false);
                
                //internal_call->mCallData.pre_hold_state = SEND_STATE_NOTHING;
                internal_call->mCallData.mdp_status = MDP_STATUS_FINAL;
                //Leave the critical section
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
            }
        }
        else if(status >= 400)  // 415 = unsupport media  486 = Busy 
        {
            switch(status)
            {
            case 401:// Authentication needed
            case 407:
                my_sip_authenticate(nh, magic, sip, tags);
                break;
            case 403://Forbidden
                releaseCall(magic,nh,phrase,false);
                //if(magic->event_listner != NULL)
                //    magic->event_listner->onLogonState(false, phrase, 0,magic->local_ip);
                break;
            case 404:
                releaseCall(magic,nh,"Remote is unreachable",false);
                break;
            case 486:
                releaseCall(magic,nh,"Busy",false);
                break;
            case 406:
                break;
            case 488:
                break;
            case 491:  
                break;  
            default:
                releaseCall(magic,nh,phrase,false);
            };
        }
    } // app_r_invite
    
    void my_i_invite(int           status,
                     char const   *phrase,
                     nua_t        *nua,
                     nua_magic_t  *magic,
                     nua_handle_t *nh,
                     nua_hmagic_t *hmagic,
                     sip_t const  *sip,
                     tagi_t        tags[])
    {
        //someone invite me 
        printf("###my_i_invite incoming call %d\n",status);
        
        //check whether call me
        if(sip->sip_request != NULL && sip->sip_request->rq_method == sip_method_invite)
            if(strcmp(sip->sip_request->rq_url[0].url_user, magic->sip_id) != 0)
            {
                printf("Someone call %s but not call me.",sip->sip_request->rq_url[0].url_user);
                nua_respond(nh,
                            SIP_403_FORBIDDEN, 
                            TAG_END());
                return;
            }

        if (status == 100) //tring
        {
            SipCall *pcall = findCallByHandle(magic->call_list,nh);
            
            //receive invite without SDP
            if(sip->sip_payload == NULL || sip->sip_payload->pl_data == NULL)
            {
                nua_respond(nh, SIP_415_UNSUPPORTED_MEDIA, TAG_END());
                if(pcall != NULL)
                {
                    releaseCall(magic,nh,sip_415_Unsupported_media,false);
                }
                return;
            }
            
            if(pcall == NULL)
            {
                //Create new call
                pcall = new SipCall();
                
                //copy remote id to call
                memset(pcall->mCallData.remote_id,0,sizeof(pcall->mCallData.remote_id));
                strcpy(pcall->mCallData.remote_id,"unknown");
                /*sip_p_asserted_identity_t *passerted = sip_p_asserted_identity(sip);
                if (passerted && passerted->paid_url && passerted->paid_url->url_user)
                    strcpy(pcall->mCallData.remote_id,passerted->paid_url->url_user);*/
                
                //parse remote id from P-Asserted-Identity
                if (sip->sip_unknown)
                {
                    char *remote_id = parse_PAI(sip->sip_unknown);
                    if(remote_id != NULL)
                        strcpy(pcall->mCallData.remote_id,remote_id);
                }
                if (sip->sip_from && sip->sip_from->a_url && sip->sip_from->a_url->url_user)
                    strcpy(pcall->mCallData.remote_id,sip->sip_from->a_url->url_user);
                
                //copy display name to icall
                memset(pcall->mCallData.remote_display_name,0,sizeof(pcall->mCallData.remote_display_name));
                if(sip->sip_from != NULL && sip->sip_from->a_display != NULL)
                    strcpy(pcall->mCallData.remote_display_name, sip->sip_from->a_display);
                
                //copy remote domain to call
                memset(pcall->mCallData.remote_domain,0,sizeof(pcall->mCallData.remote_domain));
                strcpy(pcall->mCallData.remote_domain,sip->sip_from->a_url[0].url_host);
                
                //copy call id to icall
                memset(pcall->mCallData.call_id,0,sizeof(pcall->mCallData.call_id));
                strcpy(pcall->mCallData.call_id, sip->sip_call_id->i_id);
                
                pcall->mCallData.call_handle = nh;
                pcall->mCallData.call_type   = CALL_TYPE_INCOMING;
                pcall->mCallData.call_state  = CALL_STATE_RINGING;
               
                //add to call list
                magic->call_list.push_back(pcall);
            }
            else    //receive re-invite
            {
                // Enter the critical section
                pthread_mutex_lock(&pcall->mCallData.calldata_lock);
                //receive a re-invite after I make a hold call
                /*if(pcall->mCallData.is_reinvite == true && 
                   pcall->mCallData.is_send_hold == true)
                {
                    printf("send hold & recv re-invite(hold) on the same time\n");
                }*/
                //receive a re-invite after I make a re-invite call
                if(pcall->mCallData.is_reinvite == true && 
                        pcall->mCallData.is_caller == true)
                {    
                    /*A UAS that receives an INVITE on a dialog while an INVITE it had sent
                    on that dialog is in progress MUST return a 491 (Request Pending)
                    response to the received INVITE.*/
                    nua_respond(pcall->mCallData.call_handle, 
                                SIP_491_REQUEST_PENDING, 
                                TAG_END());
                    //Leave the critical section 
                    pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                    return;
                }
                // After I send the hold signal, someone invite me
                /*else if(pcall->mCallData.call_state == CALL_STATE_HOLD && pcall->mCallData.is_send_hold == true)
                {
                    nua_respond(pcall->mCallData.call_handle, 
                                SIP_406_NOT_ACCEPTABLE, 
                                TAG_END());
                    
                    //Leave the critical section
                    pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                    return;
                }*/
                pcall->mCallData.is_reinvite = true;
            }
            
            if(pcall->mCallData.is_reinvite == false)
                nua_respond(nh, SIP_180_RINGING, TAG_END());  //SIPTAG_REQUIRE_STR("100rel"),

            //backup local/remote port
            pcall->mCallData.old_remote_audio_port = pcall->mCallData.remote_audio_port;
            pcall->mCallData.old_remote_video_port = pcall->mCallData.remote_video_port;
            pcall->mCallData.old_local_audio_port = pcall->mCallData.local_audio_port;
            pcall->mCallData.old_local_video_port = pcall->mCallData.local_video_port;

            //parse remote SDP
            parseSDP(sip->sip_payload->pl_data, pcall,true);

            //Check if this is a reinvite message
            if (pcall->mCallData.is_reinvite) 
            {
                // receive hold or unhold
                if (pcall->mCallData.call_state == CALL_STATE_HOLD ||
                    pcall->mCallData.call_state == CALL_STATE_UNHOLD)
                {
                    //Auto answer
                    my_auto_answer(pcall, magic);
                }
                //Check if there is nothing change in SDP
                else if((pcall->mCallData.old_remote_audio_port !=0 ) == (pcall->mCallData.remote_audio_port != 0) &&
                        (pcall->mCallData.old_remote_video_port !=0 ) == (pcall->mCallData.remote_video_port !=0) )
                {
                    //Auto answer
                    my_auto_answer(pcall, magic);
                        
                    //Leave the critical section  
                    pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                    return;
                }
            }
            
            //Leave the critical section  
            pthread_mutex_unlock(&pcall->mCallData.calldata_lock);

            if(magic->event_listner != NULL)
                magic->event_listner->onCallIncoming(pcall);
        }
        else if(status == 200)
        {
            SipCall *pcall = findCallByHandle(magic->call_list,nh);
            
            if(pcall != NULL)
            {
                // Enter the critical section
                pthread_mutex_lock(&pcall->mCallData.calldata_lock);
                
                pcall->mCallData.call_type = CALL_TYPE_INCOMING;
                pcall->mCallData.call_state = CALL_STATE_ACCEPT;
                //parse remote SDP
                parseSDP(sip->sip_payload->pl_data, pcall,true);
                
                //Leave the critical section  
                pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                
                if(magic->event_listner != NULL)
                    magic->event_listner->onCallIncoming(pcall);
            }
        }
        else if(status > 400 && status < 500)
        {
            releaseCall(magic,nh,phrase,false);
        }
    } // app_i_invite
    
    void my_i_state(int           status,
                    char const   *phrase,
                    nua_t        *nua,
                    nua_magic_t  *magic,
                    nua_handle_t *nh,
                    nua_hmagic_t *hmagic,
                    sip_t const  *sip,
                    tagi_t        tags[])
    {
        //nua_callstate state = nua_callstate_init;
        int state = (int)nua_callstate_init;
        //char const* local = NULL, * remote = NULL;

        tl_gets(tags,
                NUTAG_CALLSTATE_REF(state),
                //SOATAG_LOCAL_SDP_STR_REF(local),
                //SOATAG_REMOTE_SDP_STR_REF(remote),  //received ready
                TAG_END());

        //printf("my_i_state call %s\n", nua_callstate_name(state));
        SipCall *internal_call = findCallByHandle(magic->call_list,nh);
        if(magic->event_listner != NULL && internal_call != NULL)
        {
            magic->event_listner->onCallStateChanged(internal_call,
                                                     internal_call->mCallData.call_state,
                                                     phrase);
        }
        //printf("local %s\n", local);
        //printf("remote %s\n", remote);

        /*
         nua_callstate_init                      Initial state.
         nua_callstate_authenticating    401/407 received
         nua_callstate_calling                   INVITE sent. 
         nua_callstate_proceeding                18X received
         nua_callstate_completing                2XX received
         nua_callstate_received                  INVITE received.
         nua_callstate_early                     18X sent (w/SDP)
         nua_callstate_completed                 2XX sent
         nua_callstate_ready                     2XX received, ACK sent, or vice versa
         nua_callstate_terminating               BYE sent.
         nua_callstate_terminated                BYE complete.
         */ 
        
        /*
         CALL_STATE_FAILED      = -1,
         CALL_STATE_NA          = 0, ****** calling 0 
         CALL_STATE_TRYING      = 1,
         CALL_STATE_RINGING     = 2, ****** proceeding  ready  completed  early received 
         CALL_STATE_ACCEPT      = 3, ****** ready 3   ****** completing 3 
         CALL_STATE_TRANSFERRED = 4,
         CALL_STATE_HANGUP      = 5,
         CALL_STATE_CLOSED      = 6
         */
    } // app_i_state
    
    void my_i_active(int           status,
                     char const   *phrase,
                     nua_t        *nua,
                     nua_magic_t  *magic,
                     nua_handle_t *nh,
                     nua_hmagic_t *hmagic,
                     sip_t const  *sip,
                     tagi_t        tags[])
    {
        printf("call active\n");
        if(status == 200)
        {
            //on session start
            SipCall *internal_call = findCallByHandle(magic->call_list,nh);
            if(internal_call != NULL && internal_call->mCallData.mdp_status == MDP_STATUS_FINAL)
            {       
                // Enter the critical section
                pthread_mutex_lock(&internal_call->mCallData.calldata_lock);

                if(internal_call->mCallData.call_state != CALL_STATE_HOLD && internal_call->mCallData.call_state != CALL_STATE_UNHOLD)
                    internal_call->mCallData.call_state = CALL_STATE_ACCEPT;
                
                if(magic->event_listner != NULL)
                {
                    if(internal_call->mCallData.is_reinvite == true)
                    {
                        internal_call->mCallData.is_session_start = true;
                        //on session end
                        if(internal_call->mCallData.call_state == CALL_STATE_HOLD ||
                           internal_call->mCallData.call_state == CALL_STATE_UNHOLD )
                        {
                            if(internal_call->mCallData.call_state == CALL_STATE_UNHOLD )
                            {
                                //unhold
                                //internal_call->mCallData.is_send_hold  = false;
                                //internal_call->mCallData.is_recv_hold  = false;
                                internal_call->mCallData.call_state = CALL_STATE_ACCEPT;
                            }
                            magic->event_listner->onCallStateChanged(internal_call, internal_call->mCallData.call_state, NULL);
                            if(internal_call->mCallData.call_state != CALL_STATE_HOLD &&
                               internal_call->mCallData.is_session_change == true)
                            {
                                magic->event_listner->onSessionEnd(internal_call);
                                magic->event_listner->onSessionStart(internal_call);
                            } 
                        }
                        else if(internal_call->mCallData.is_session_change == true)
                        {
                            magic->event_listner->onSessionEnd(internal_call);
                            magic->event_listner->onSessionStart(internal_call);
                        } 
                        else //re-invite was rejected or session does not change
                        {
                            internal_call->mCallData.remote_audio_port  = internal_call->mCallData.old_remote_audio_port;
                            internal_call->mCallData.remote_video_port  = internal_call->mCallData.old_remote_video_port;
                            internal_call->mCallData.local_audio_port   = internal_call->mCallData.old_local_audio_port;
                            internal_call->mCallData.local_video_port   = internal_call->mCallData.old_local_video_port;
                            internal_call->mCallData.is_video_call      = internal_call->mCallData.local_video_port!=0 && internal_call->mCallData.remote_video_port!=0;
                            magic->event_listner->onReinviteFailed(internal_call);
                        }
                    }
                    else  //first invite
                    {
                        internal_call->mCallData.is_session_start = true;
                        internal_call->mCallData.start_time = time (NULL);
                        magic->event_listner->onSessionStart(internal_call);
                    }
                }
                internal_call->mCallData.is_caller      = false;
                internal_call->mCallData.is_reinvite    = false;
                internal_call->mCallData.is_video_call  = internal_call->mCallData.local_video_port!=0 && internal_call->mCallData.remote_video_port!=0;
                if (!internal_call->mCallData.is_video_call)
                {
                    internal_call->mCallData.local_video_port = 0;
                    internal_call->mCallData.remote_video_port = 0;
                }
                //Leave the critical section  
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
            }
        }
        else if(status == 406 || status == 491) //not acceptable || both re-invite
        {
            //on session start
            SipCall *internal_call = findCallByHandle(magic->call_list,nh);
            if(internal_call != NULL)
            {
                // Enter the critical section
                pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
                
                internal_call->mCallData.call_state         = CALL_STATE_ACCEPT;
                internal_call->mCallData.is_send_hold       = false;
                internal_call->mCallData.is_recv_hold       = false;
                internal_call->mCallData.is_caller          = false;
                internal_call->mCallData.is_reinvite        = false;
                internal_call->mCallData.is_session_change  = false;
                internal_call->mCallData.is_session_start   = true;
                //recover local/remote port
                internal_call->mCallData.remote_audio_port  = internal_call->mCallData.old_remote_audio_port;
                internal_call->mCallData.remote_video_port  = internal_call->mCallData.old_remote_video_port;
                if (status == 406)
                {
                    internal_call->mCallData.local_audio_port = internal_call->mCallData.old_local_audio_port;
                    internal_call->mCallData.local_video_port = internal_call->mCallData.old_local_video_port;
                }
                internal_call->mCallData.is_video_call =  internal_call->mCallData.remote_video_port!=0 && internal_call->mCallData.local_video_port!=0;
                if(magic->event_listner != NULL)
                {
                    magic->event_listner->onReinviteFailed(internal_call);
                }
                
                //Leave the critical section 
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
            }
        }
    } // app_i_active
    
    
    void my_r_bye(int           status,
                  char const   *phrase,
                  nua_t        *nua,
                  nua_magic_t  *magic,
                  nua_handle_t *nh,
                  nua_hmagic_t *hmagic,
                  sip_t const  *sip,
                  tagi_t        tags[])
    {
        switch(status)
        {
            case 200:
                printf("my_r_bye Bye was received by the remote host\n");
                break;   
            case 407:
                my_sip_authenticate(nh, magic, sip, tags);
                break;    
            default:
                printf("Fail :my_r_bye Bye not received by the remote host\n");
                break;
        }
        // release operation handle 
        releaseCall(magic,nh,"Declined",true); //self decline
    } // app_r_bye
    
    void my_i_bye(int           status,
                  char const   *phrase,
                  nua_t        *nua,
                  nua_magic_t  *magic,
                  nua_handle_t *nh,
                  nua_hmagic_t *hmagic,
                  sip_t const  *sip,
                  tagi_t        tags[])
    {
        printf("my_i_bye call released\n");
        
        // release operation handle
        releaseCall(magic,nh,phrase,false); //remote decline
    }// app_i_bye
    
    void my_r_cancel(int           status,
                     char const   *phrase,
                     nua_t        *nua,
                     nua_magic_t  *magic,
                     nua_handle_t *nh,
                     nua_hmagic_t *hmagic,
                     sip_t const  *sip,
                     tagi_t        tags[])
    {
        // release operation handle 
        releaseCall(magic,nh,"Canceled",true); //self cancel
    }
    
    void my_i_cancel(int           status,
                     char const   *phrase,
                     nua_t        *nua,
                     nua_magic_t  *magic,
                     nua_handle_t *nh,
                     nua_hmagic_t *hmagic,
                     sip_t const  *sip,
                     tagi_t        tags[])
    {
        // release operation handle 
        releaseCall(magic,nh,"Canceled",false); //remote cancel
    }
    
    void my_i_terminated(int           status,
                         char const   *phrase,
                         nua_t        *nua,
                         nua_magic_t  *magic,
                         nua_handle_t *nh,
                         nua_hmagic_t *hmagic,
                         sip_t const  *sip,
                         tagi_t        tags[])
    {
        // release operation handle 
        releaseCall(magic,nh,"Terminated",true);
    }// my_i_terminated
    
    void my_i_info(int           status,
                    char const   *phrase,
                    nua_t        *nua,
                    nua_magic_t  *magic,
                    nua_handle_t *nh,
                    nua_hmagic_t *hmagic,
                    sip_t const  *sip,
                    tagi_t        tags[])
    {
        if (status == 200) 
        {
            SipCall *internal_call = findCallByHandle(magic->call_list,nh);
            if(internal_call != NULL)
            {
                // Enter the critical section
                pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
                /*nua_respond(internal_call->mCallData.call_handle, 
                            SIP_200_OK, 
                            TAG_END());*/
                
                if(strcasecmp(sip->sip_content_type->c_type,"application/media_control+xml") == 0 &&
                   strcasecmp(sip->sip_content_type->c_subtype,"media_control+xml") == 0 )
                {
                    if(strstr(sip->sip_payload->pl_data,"picture_fast_update")!= NULL)
                    {
                        if(magic->event_listner != NULL)
                            magic->event_listner->onRequestIDR(internal_call);
                    }
                }
                else if((strcasecmp(sip->sip_content_type->c_type,"application/dtmf-info") == 0 ||
                         strcasecmp(sip->sip_content_type->c_type,"application/dtmf-relay") == 0 ) &&
                        (strcasecmp(sip->sip_content_type->c_subtype,"dtmf-info") == 0 ||
                         strcasecmp(sip->sip_content_type->c_subtype,"dtmf-relay") == 0 ))
                {
                    char *pch =  strtok((char*)sip->sip_payload->pl_data,"= \r\n");
                    char signal = 0;
                    while (pch != NULL)
                    {
                        if(strcasecmp(pch, "Signal") == 0)
                        {
                            pch = strtok (NULL, "= \r\n");
                            signal = pch[0];
                            printf("Signal: %s \n",pch);
                        }
                        else if(strcasecmp(pch, "Duration") == 0)
                        {
                            pch = strtok (NULL, "= \r\n");
                            printf("Duration: %s \n",pch);
                        }
                        pch = strtok (NULL, "= \n");
                    }
                    //if(magic->event_listner != NULL && signal != 0)
                        //magic->event_listner->onReceiveDTMF(internal_call,signal);
                }
                //Leave the critical section 
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
            }
        }
    }// my_i_info
    
    void my_r_info(int           status,
                   char const   *phrase,
                   nua_t        *nua,
                   nua_magic_t  *magic,
                   nua_handle_t *nh,
                   nua_hmagic_t *hmagic,
                   sip_t const  *sip,
                   tagi_t        tags[])
    {
    }// my_r_info
    
    void my_r_message(int           status,
                      char const   *phrase,
                      nua_t        *nua,
                      nua_magic_t  *magic,
                      nua_handle_t *nh,
                      nua_hmagic_t *hmagic,
                      sip_t const  *sip,
                      tagi_t        tags[])
    {
    } // app_r_message
    
    void my_i_message(int           status,
                      char const   *phrase,
                      nua_t        *nua,
                      nua_magic_t  *magic,
                      nua_handle_t *nh,
                      nua_hmagic_t *hmagic,
                      sip_t const  *sip,
                      tagi_t        tags[])
    {
        /*printf("From: %s%s \n",
               sip->sip_from->a_display ? sip->sip_from->a_display : "",
               URL_PRINT_ARGS(sip->sip_from->a_url));
        */
        if (sip->sip_subject) 
        {
            printf("Subject: %s\n", sip->sip_subject->g_value);
        }
    } // app_i_message
    
    void my_r_shutdown(int           status,
                       char const   *phrase,
                       nua_t        *nua,
                       nua_magic_t  *magic,
                       nua_handle_t *nh,
                       nua_hmagic_t *hmagic,
                       sip_t const  *sip,
                       tagi_t        tags[])
    {
        if(status >= 200)
            su_root_break(magic->root);
    }// my_r_shutdown
    
    void my_r_prack(int           status,
                    char const   *phrase,
                    nua_t        *nua,
                    nua_magic_t  *magic,
                    nua_handle_t *nh,
                    nua_hmagic_t *hmagic,
                    sip_t const  *sip,
                    tagi_t        tags[])
    {
        if (status == 200)
        {
            //on session start
            SipCall *internal_call = findCallByHandle(magic->call_list,nh);
            if(internal_call != NULL && internal_call->mCallData.mdp_status == MDP_STATUS_FINAL)
            {       
                // Enter the critical section
                pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
                internal_call->mCallData.call_state = CALL_STATE_ACCEPT;

                if(magic->event_listner != NULL)
                {
                    //first invite
                    internal_call->mCallData.is_session_start = true;
                    internal_call->mCallData.start_time = time (NULL);
                    magic->event_listner->onSessionStart(internal_call);
                }
                internal_call->mCallData.is_caller      = false;
                internal_call->mCallData.is_reinvite    = true;
                internal_call->mCallData.is_video_call  = internal_call->mCallData.local_video_port!=0 && internal_call->mCallData.remote_video_port!=0;

                //Leave the critical section  
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
            }
        }
        else if (status >= 400 && status < 500) //481  408 Request Timeout
        {
            //releaseCall(magic,nh,phrase,false);
        }
    }// my_r_prack
    
    /* This callback will be called by SIP stack to  
     * process incoming events   
     */  
    void event_callback(nua_event_t   event,  
                        int           status,  
                        char const   *phrase,  
                        nua_t        *nua,  
                        nua_magic_t  *magic,  
                        nua_handle_t *nh,  
                        nua_hmagic_t *hmagic,  
                        sip_t const  *sip,  
                        tagi_t        tags[])  
    {  
        printf ("Received an event %s status %d %s\n",nua_event_name(event), status, phrase);  
        switch (event) 
        {
            case nua_r_register:
                my_r_register(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_unregister:
                my_r_unregister(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_i_invite:
                my_i_invite(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_invite:
                my_r_invite(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
                
            case nua_i_state:
                my_i_state(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_i_active:
                my_i_active(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
                
            case nua_i_bye:
                my_i_bye(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_bye:
                my_r_bye(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_i_cancel: //someone cancel his incoming invite
                my_i_cancel(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;     
            case nua_r_cancel: //self cancel own outgoing invite
                my_r_cancel(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_i_terminated:
                my_i_terminated(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
                
            case nua_i_message:
                my_i_message(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_message:
                my_r_message(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_prack:
                my_r_prack(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_i_ack:
            case nua_r_set_params:
            case nua_i_options:
                break;
            case nua_i_info:
                my_i_info(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_info:
                my_r_info(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            case nua_r_shutdown:
                my_r_shutdown(status, phrase, nua, magic, nh, hmagic, sip, tags);
                break;
            default:
                if (status > 100) // unknown event -> print out error message
                {
                    printf("unknown event %d: %03d %s\n",event,status,phrase);
                }
                else 
                {
                    printf("unknown event %d\n", event);
                }
                break;
        }
    }  
}
