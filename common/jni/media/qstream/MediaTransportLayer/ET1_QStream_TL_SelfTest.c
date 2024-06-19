/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) ian 2009 <ian.tuan@gmail.com>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include  "config.h"
#include "config_unix.h"
#include "ET1_QStream_TransportLayer.h"

#define MAX_BUFF_SIZE 80000
#define MAX_AUDIO_BUFF_SIZE 1000

char FrameBuffer[MAX_BUFF_SIZE];
char RecvFrame[MAX_BUFF_SIZE];

const char H264StartCode[4] = {0x00, 0x00, 0x00, 0x01}; 

typedef struct
{
    uint32_t frame_len;
    uint16_t group_id;
    uint16_t frame_id;
}ext_info_t;

#define H264_SEQ_PARAMETER_SET  7
#define H264_PIC_PARAMETER_SET  8
#define H264_I_SLICE            5
#define H264_P_SLICE            1

PayloadType_t payload_type_g7221 = {
     TYPE(PAYLOAD_AUDIO_PACKETIZED),
    CLOCK_RATE(16000),
	BITS_PER_SAMPLE(0),
	ZERO_PATTERN(NULL),
	PATTERN_LENGTH(0),
	NORMAL_BITRATE(24000),
	MIME_TYPE ("G7221"),
	CHANNELS(1),
	PAYLOAD_TYPE_NUM(121)

};

PayloadType_t payload_type_h264={
	TYPE(PAYLOAD_VIDEO),
	CLOCK_RATE(90000),
	BITS_PER_SAMPLE(0),
	ZERO_PATTERN(NULL),
	PATTERN_LENGTH(0),
	NORMAL_BITRATE(256000),
	MIME_TYPE ("H264"),
	CHANNELS(0),
	PAYLOAD_TYPE_NUM(99)
};

static inline unsigned char get_nal_type(const unsigned char *h){
	return (*h) & ((1<<5)-1);
}

char *SeekStartCode(char *buf1, char *buf_end_ptr)
{
	char * result = buf1;
    int i;
    int buflen = buf_end_ptr - buf1;

    for(i=0;i < buflen-3; i++)    {
         if(!memcmp(result,H264StartCode,4))
             return result;
		 result ++;
    }

	return NULL;
}

char *SeekFrameData(char* FrameData, int length, int *frame_len, int *type)
{
     char *frame_start;
     char *start_ptr;
     char *start_code;
     int nal_type;
     int full_frame = FALSE;
     int first_start_code = TRUE;
     
     start_ptr = FrameData;

     while(1)
     {
          /*search the start code 0x00000001 */   
          /*start_code = strstr((char*)start_ptr, H264StartCode);*/
          start_code = SeekStartCode(start_ptr, FrameData+length);
          if(start_code)
	  {
	       if(first_start_code)
	       {
                      frame_start = start_code;
                      first_start_code = FALSE;
                }

		start_ptr = start_code + 4;

                nal_type = get_nal_type((unsigned char*)start_ptr);

                if((nal_type == H264_I_SLICE) || (nal_type == H264_P_SLICE))
                {   
                     *type = nal_type;
                     full_frame = TRUE;
                     start_code = SeekStartCode(start_ptr, FrameData+length);
               
                    if(start_code)
                       *frame_len = start_code - frame_start;
                    else
                      *frame_len = FrameData + length - frame_start;

                      break;
                 }
                 else
                      continue;
	   
	   }
	   else
               break;

        }

	return frame_start;
}

static void print_usage(void) 
{
	printf("Usage: transportlayer [-r <address>] [-p <port>] [-f <file>]\n");
}

void Rtcp_Callback(struct TL_Session_t *session, struct TL_Rtcp_Event_t *e)
{
     TL_Rtcp_SR_t   *sr;
     uint32_t   ntp_sec, ntp_frac, rtp_ts, sender_pcount, sender_bcount; /*sender report related parameters*/
     TL_Rtcp_RR_t *rr;
     uint32_t	total_lost ,fract_lost, last_seq, jitter,lsr,dlsr; /*receiver report related parameters*/
     TL_Rtcp_Tfrc_RX_t *tfrc;
     uint32_t	ts,tdelay,x_recv,p; /*tfrc related parameters*/
     uint8_t appinfo[12]; 
     
     switch(e->type)
     {
     case TL_RX_SR:
           printf("sender report callback\n");
           sr = (TL_Rtcp_SR_t*)(e->data);
           ntp_sec = sr->ntp_sec;
           ntp_frac = sr->ntp_frac,
           rtp_ts = sr->rtp_ts;
           sender_pcount = sr->sender_pcount;
           sender_bcount = sr->sender_bcount;
           break;
     case TL_RX_RR:
           printf("receiver report callback\n");
           rr = (TL_Rtcp_RR_t*)(e->data);
           total_lost = rr->total_lost;
           fract_lost = rr->fract_lost;
           last_seq = rr->last_seq;
           jitter = rr->jitter;
           lsr = rr->lsr;
           dlsr = rr->dlsr;
           break;
     case TL_RX_TFRC_RX:
           printf("tfrc rx callback\n");
           tfrc = (TL_Rtcp_Tfrc_RX_t*)(e->data);
           ts = tfrc->ts;
           tdelay = tfrc->tdelay;
           x_recv = tfrc->x_recv;
           p = tfrc->p;
           break;
     case  TL_APP_SET_TIME_INFO:  /*for lip sync set time information*/

           
           TransportLayer_SetRTCPTimeIfno(session, rtp_ts, ntp_sec, ntp_frac);
           
           break;
           
     case  TL_APP_SET_RTCP_APP:
           TransportLayer_SetAppInfo(session, appinfo, 12);
           break;
     default:
           printf("unknow RTCP type\n");
           break;
     } 

}


void Audio_SelfTest()
{
     FILE *Streamaudio, *RecvStream;
     int numwritten = 0;
    unsigned long timestamp = 1234;
    const char *address = "none";
    struct TL_Session_t *session;
    int port = 0;
    unsigned long recv_len = 0, recv_ts;
    unsigned long frame_len = 0;
    uint8_t partial_flag;

   address = "127.0.0.1";
   port = 12346;    
    Streamaudio = fopen("/home/iantuan/DSocket.cpp", "rb");
    RecvStream = fopen("/home/iantuan/tl_rx_file.cpp","wb");
    
    
    session = TransportLayer_Init(address, port, port, FALSE, &payload_type_g7221, Rtcp_Callback);
    
    TransportLayer_Start(session);
    
    usleep(10000);

    while(1)
    {

         memset(FrameBuffer, 0, MAX_BUFF_SIZE);

         numwritten = fread(FrameBuffer, sizeof(char), MAX_AUDIO_BUFF_SIZE, Streamaudio);

         frame_len = numwritten;

       	 if(!numwritten)
	    {
              printf("end of file!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
              usleep(10000);

         }
         else
        {
              session->send_data(session, (uint8_t*)FrameBuffer, (uint32_t)frame_len, timestamp,NULL,0);
        }


        session->recv_data(session, (uint8_t*)RecvFrame, &recv_len, &recv_ts, &partial_flag);
	
	    if(recv_len)
	    {
             printf("recv_len = %d, recv_ts=%d\n",recv_len, recv_ts);
	         fwrite(RecvFrame, sizeof(char), recv_len, RecvStream);
	    }
        else
         {
                printf("frame data is not yet complete\n");
         }
			
        timestamp += 160;
    
        //usleep(1000);
     }/*end while*/

    fclose(Streamaudio);
    fclose(RecvStream);

}

void Video_SelfTest()
{
    FILE *Stream264, *RecvStream;
    int numwritten = 0;
    char * frame_start;
    unsigned long frame_len = 0;
    long seeked_len = 0;
    int frame_type;
    unsigned long timestamp = 1234;
    const char *address = "none";
    const char *file_name = "none";
    struct TL_Session_t *session;
    int port = 0;
    unsigned long recv_len = 0, recv_ts;
    int i = 0;
	int ch = 0;
    int group =0;
    int frame = 0;
    uint8_t ext_hdr_data[8];
    uint16_t ext_len = 8;
    ext_info_t *ext_info, *recv_ext_info;
    uint8_t partial_flag;
    uint8_t recv_ext_hdr_data[8];
    uint16_t recv_ext_len;

   address = "127.0.0.1";
   port = 12346;    Stream264 = fopen("/home/iantuan/tx_GOP5_spsr1.h264", "rb");
    RecvStream = fopen("/home/iantuan/tl_rx_file.h264","wb");
    
    ext_info = (ext_info_t*)ext_hdr_data;
    recv_ext_info = (ext_info_t*)recv_ext_hdr_data;

    session = TransportLayer_Init(address, port, port, FALSE, &payload_type_h264,Rtcp_Callback);
    
    TransportLayer_Start(session);
    
    usleep(10000);
        while(1)
    {

         memset(FrameBuffer, 0, MAX_BUFF_SIZE);

         numwritten = fread(FrameBuffer, sizeof(char), MAX_BUFF_SIZE, Stream264);
        
	 if(!numwritten)
	 {
              printf("end of file!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
              usleep(10000);

         }
        else      
       { 
            frame_start = SeekFrameData(FrameBuffer,numwritten, &frame_len, &frame_type);
        
            switch (frame_type)
           {
             case H264_I_SLICE:
                    group ++;
                    frame = 1;
                   break;
             case H264_P_SLICE:
                    frame ++;
                break;
               default:
                 break;
             }

            // printf("the %d th frame\n", i);
            
             i++;
             //printf("timestamp = %d\n",timestamp);
             ext_info->frame_len = frame_len;
             ext_info->group_id = group;
             ext_info->frame_id = frame;

             printf("[send] ext header data: frame_len = %d, group_id = %d frame_id = %d\n", ext_info->frame_len, ext_info->group_id, ext_info->frame_id);

             session->send_data(session, (uint8_t*)FrameBuffer, (uint32_t)frame_len, timestamp, ext_hdr_data, ext_len);
        }
        
        session->poll_data(session, &recv_ts, recv_ext_hdr_data, &recv_ext_len);
        
        printf("[recv] recv ext header data: frame_len = %d, group_id = %d, frame_id=%d\n", recv_ext_info->frame_len, recv_ext_info->group_id, recv_ext_info->frame_id);
        
        session->recv_data(session, (uint8_t*)RecvFrame, &recv_len, &recv_ts, &partial_flag);
	
	    if(recv_len)
	    {
	         fwrite(RecvFrame, sizeof(char), recv_len, RecvStream);
	    }
        else
         {
                printf("frame data is not yet complete\n");
         }
	
		
        timestamp += 3000;
        
        if(!frame_len)
        {
             printf("no frame data\n");
             break;
        }

        seeked_len += frame_len;
        
        fseek(Stream264, seeked_len, SEEK_SET);
        
        usleep(1000);
    }/*end while*/
    
    fclose(Stream264);
    fclose(RecvStream);
    
}

int main(int argc, char *argv[])
{
     Video_SelfTest();
     //Audio_SelfTest();
      return (0);
}
