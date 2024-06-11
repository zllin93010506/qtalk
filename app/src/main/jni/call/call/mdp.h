#ifndef _CALL_MDP_H
#define _CALL_MDP_H
#define ENABLE_IOT 1
#include <string.h>
#include <list>

namespace call
{
enum Mdp_Status
{
    MDP_STATUS_INIT        = 0, //doesn't get final mdp
    MDP_STATUS_MULTICODEC  = 1, //handle 200_OK with multi codec
    MDP_STATUS_FINAL       = 2  //get final mdp can do session start
};   
    
#pragma pack(push, 1) //- packing is now 1

typedef struct {
    int ctrl;               // 0:disable, 1:enable (get it from provision file for enabling FEC or not)
    int send_payloadType;   // from opposite SDP. set '-1' if opposite doesn't support FEC
    int receive_payloadType;// from self SDP
}FEC_Control_t;

typedef struct _SRTP_KEY_MKI_t{
    int mki_value;
    int mki_length;
}SRTP_KEY_MKI;
    
typedef struct _SRTP_KEY_PARAMETER_t{
    int crypto_suit;
    char key_salt[64];
    unsigned long int lifetime;
    SRTP_KEY_MKI mki;
} SRTP_KEY_PARAMETER;
    
typedef struct _MEDIA_SRTP_KEY_PARAMETER_t{
    bool enable_srtp;
    SRTP_KEY_PARAMETER self_srtp_key;
    SRTP_KEY_PARAMETER opposite_srtp_key;
} MEDIA_SRTP_KEY_PARAMETER;

#define MINE_TYPE_LENGTH 16
typedef struct MDP
{
    unsigned int payload_id;
    unsigned int clock_rate;
    unsigned int bitrate;
    char         mime_type[MINE_TYPE_LENGTH];

    //H264 IOT
    //ox42 => 66 baseline
    //ox4D => 77 main profile
    //ox58 => 88 high profile
    unsigned int profile;
    //ox1F 31 => 720p/30fps  108000  3600(30)
    //ox1E 30 => VGA/25fps    40500  1620(25)
    //ox0D 13 => CIF/30fps    11880   396(30)
    unsigned int level_idc;
    // 0 = signal nalu
    // 1 = non-interleave
    unsigned int packetization_mode;
    
    unsigned int  max_mbps;    //(w*h)/(16*16)
    unsigned int  max_fs;      //max_mbps*fps
    unsigned int tias_kbps;    //transport independent application specific maximun
    
    //rtcp-fb
    bool enable_rtcp_fb_fir;
    bool enable_rtcp_fb_pli;
    
    FEC_Control_t fec_ctrl;
    
    MEDIA_SRTP_KEY_PARAMETER srtp;

    
    MDP()
    {
        payload_id  = 0;
        clock_rate  = 0;
        bitrate     = 0;
        memset(mime_type , 0, MINE_TYPE_LENGTH*sizeof(char));

        profile    = 0;
        level_idc  = 0;
        max_mbps   = 0;
        max_fs     = 0;
        tias_kbps  = 0;
        packetization_mode = 0;
        enable_rtcp_fb_fir = false;
        enable_rtcp_fb_pli = false;
        
        memset(&fec_ctrl, 0, sizeof(FEC_Control_t));//fec ctrl
        memset(&srtp, 0, sizeof(MEDIA_SRTP_KEY_PARAMETER));//srtp
    }
    ~MDP()
    {}
};
#pragma pack(pop) //- packing is 8
}
void empty_mdp_list(std::list<struct call::MDP*> *list);
void init_mdp(call::MDP *mdp);
void copy_mdp(call::MDP *tar,const call::MDP *src);
#if ENABLE_IOT
void iot_set_mdp(unsigned int width,unsigned int height,unsigned int fps,unsigned int kbps,unsigned int packetization_mode,bool enablePLI, bool enableFIR,call::MDP *mdp);
void iot_get_mdp(const call::MDP mdp ,unsigned int *width,unsigned int *height,unsigned int *fps,unsigned int *kbps, bool *enablePLI,bool *enableFIR);
#endif
#endif //_CALL_MDP_H
