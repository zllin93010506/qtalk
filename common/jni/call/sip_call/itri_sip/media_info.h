#ifndef _MEDIA_INFO_H
#define _MEDIA_INFO_H

typedef struct ProfileLevelID{
  int profile_idc;
  bool constraint_set0_flag,constraint_set1_flag,constraint_set2_flag;
  int level_idc; //follows the QuantaLevelIdType
}QuProfileLevelID;

typedef struct _QuH264Parameter{
  QuProfileLevelID profile_level_id;
  int max_mbps;
  int max_fs;
  int packetization_mode;
}QuH264Parameter;

typedef struct _Generic_NACK{
    int pli;         // supported: 1 , n/a: -1
} Generic_NACK;

typedef struct _Codec_Control_Message{
    int fir;         // supported: 1 , n/a: -1
} Codec_Control_Message;

typedef struct {
    int ctrl;               // 0:disable, 1:enable (get it from provision file for enabling RFC4588 or not)

    int send_payloadType;   // from opposite SDP. set '-1' if opposite doesn't support RFC4588
    int receive_payloadType;// from self SDP (get it from provision file)

    int my_rtxTime;         // (unit: msec) the sender keeps an RTP packet in its buffers available for retransmission
    int opposite_rtxTime;   // (unit: msec) the opposite keeps an RTP packet in its buffers available for retransmission
} RFC4588_Control_t;

typedef struct _RTCP_FB_t{
    Generic_NACK nack;
    Codec_Control_Message ccm;
} RTCP_FB;

typedef struct _MediaCodecInfo{
	int audio_payload_type;
	char audio_payload[32];
	int audio_sample_rate;
	int audio_bit_rate;
	int video_payload_type;
	char video_payload[32];
	int dtmf_type;
	/*2011/10/24 media bandwith*/
	int media_bandwidth;
	QuH264Parameter h264_parameter;
	int RS_bps;     // (unit: bps) RTCP bandwidth allocated to active data senders
	int RR_bps;     // (unit: bps) RTCP bandwidth allocated to active data receivers
	RTCP_FB rtcp_fb;

    call::FEC_Control_t		audio_fec_ctrl;
    call::FEC_Control_t		video_fec_ctrl;
	RFC4588_Control_t	rfc4588_ctrl;

    call::MEDIA_SRTP_KEY_PARAMETER audio_srtp;
	call::MEDIA_SRTP_KEY_PARAMETER video_srtp;

}MediaCodecInfo;


#endif
