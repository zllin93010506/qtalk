//
//  provinsion.h
//  callengine
//
//  Created by Liu Abbi on 12/10/2.
//  Copyright (c) 2012年 Quanta. All rights reserved.
//

#ifndef callengine_provision_h
#define callengine_provision_h

namespace call
{
    typedef struct{
        char general_period_check_timer[8];
        char short_period_check_timer[8];
        char config_file_download_error_retry_timer[8];
        char config_file_forced_provision_periodic_timer[8];
    }ConfigGenericProvisioning;
    
    typedef struct{
        char sip_tos_setting[8];
        char sip_address_mapping_mechanism[8];
        char dns_retry_counts_to_failover[8];
        char tls_enable[8];
    }ConfigGenericSip;
    
    typedef struct{
        char audio_rtp_tos_setting[8];
        char video_rtp_tos_setting[8];
        char rtp_audio_port_range_start[8];
        char rtp_audio_port_range_end[8];
        char rtp_video_port_range_start[8];
        char rtp_video_port_range_end[8];
        char outgoing_max_bandwidth[8];
        char incoming_max_bandwidth[8];
        char auto_bandwidth[8];
        char video_mtu[8];
        char video_fec_enable[8];
        char video_rfc4588_enable[8];
        char video_rfc4588_rtxtime[8];
        char srtp_enable[8];
    }ConfigGenericRtp;
    
    typedef struct{
        char prack_support[8];
        char info_support[8];
        char dtmf_transmit_method[8];
        char registration_expiration_time[8];
        char registration_resend_time[8];
        char challenge_sip_invite[8];
        char session_timer_enable[8];
        char session_expire_time[8];
        char minimum_session_expire_time[8];
    }ConfigGenericLines;
    
    typedef struct{
        char provision_uri[128];
        char phonebook_uri[128];
        char phonebook_group_id[8];
        char firmware_uri[128];
        char log_uri[128];
        char x264_uri[128];
    }ConfigQtalkProvisioning;
    #define CODEC_PREFERENCE_LENGTH 8
    typedef struct{
        char displayName[64];
        char user_id[64];
        char authentication_id[64];
        char authentication_password[64];
        char registrar_address[64];
        char registrar_port[8];
        char proxy_server_address[64];
        char proxy_server_port[8];
        char outbound_proxy_enable[8];
        char outbound_proxy_address[64];
        char outbound_proxy_port[8];
        char invite_user_id[64];
        char invite_authentication_id[64];
        char invite_authentication_password[64];
        char sip_realm[64];
        char notify_user_id[64];
        char notify_authentication_id[64];
        char notify_authentication_password[64];
        char audio_codec_preference_1[CODEC_PREFERENCE_LENGTH];
        char audio_codec_preference_2[CODEC_PREFERENCE_LENGTH];
        char audio_codec_preference_3[CODEC_PREFERENCE_LENGTH];
        char audio_codec_preference_4[CODEC_PREFERENCE_LENGTH];
        char video_codec_preference_1[CODEC_PREFERENCE_LENGTH];
        char video_codec_preference_2[CODEC_PREFERENCE_LENGTH];
        char video_codec_preference_3[CODEC_PREFERENCE_LENGTH];
        char local_signaling_port[8];
        char E_164_phone_number[64];
    }ConfigQtalkSip;
    
    typedef struct{
        ConfigGenericProvisioning provisioning;
        ConfigGenericSip sip;
        ConfigGenericRtp rtp;
        ConfigGenericLines lines;
    }ConfigGeneric;
    
    typedef struct{
        ConfigQtalkProvisioning provisioning;
        ConfigQtalkSip sip;
    }ConfigQtalk;
    
    typedef struct{
        ConfigGeneric generic;
        ConfigQtalk qtalk;
    }Config;

}
#endif
