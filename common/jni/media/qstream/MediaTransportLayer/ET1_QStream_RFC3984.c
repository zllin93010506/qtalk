/*
 * FILE:     ET1_QStream_RFC3984.c
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED: 
 *
 * The routines in this file implement the RTP Payload Format for H.264 Video, 
 * as specified in RFC 3984.
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */

#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)


#ifdef WIN32
#include "config_w32.h"
#endif
#include "pbuf.h"
#include "pdb.h"
#include "ET1_QStream_RFC3984.h"
#include "ET1_QStream_SendingBuf.h"
#include "debug.h"
#include "rtp.h"

#define VIDEO_RECV_BUFFER_PKT_NUM 5

#define MTU_MAX_PAYLOAD_SIZE            1380
 
#define RFC3984_TYPE_SINGLE_NALU_01     01
#define RFC3984_TYPE_SINGLE_NALU_23     23
#define RFC3984_TYPE_STAPA              24
#define RFC3984_TYPE_STAPB              25
#define RFC3984_TYPE_MTAP16             26
#define RFC3984_TYPE_MTAP24             27
#define RFC3984_TYPE_FUA                28
#define RFC3984_TYPE_FUB                29

#define H264_START_CODE_LEN             4
 
const char m_h264_start_code[H264_START_CODE_LEN] = {0x00, 0x00, 0x00, 0x01}; 
 

#define return_val_if_fail(expr,ret) if (!(expr)) {TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5,"%s:%i- assertion" #expr "failed\n",__FILE__,__LINE__); return (ret);}


static inline void MemoryBlock_Ref(MemoryBlock_t *mb)
{
	mb->memory_ref++;
}
 
 /*
 decrease the reference counter, if nobody reference it 
 then release this memory block
 */
 static inline void MemoryBlock_Unref(MemoryBlock_t *mb)
 {
	mb->memory_ref--;
	if (mb->memory_ref==0){
		free(mb); /*free memory block*/
	}
}


/*This function allocate a real free memory block*/ 
MemoryBlock_t *AllocMemoryBlock(int size)
{
    MemoryBlock_t *mb;
    int total_size = sizeof(MemoryBlock_t) + size;
    mb=(MemoryBlock_t *) malloc(total_size);
	if (NULL == mb)
	{
		return NULL;
	}
	
	memset(mb,0,total_size);
    mb->memory_base = (uint8_t*)mb+sizeof(MemoryBlock_t);
    mb->memory_tail = mb->memory_base+size;
    /*first allocate so reference counter is 1*/
    mb->memory_ref = 1;
    return mb;

}

/*
the data block maintain the start pointer
and end pointer in the real memory block to display a data content
so we can seperate the real memory and the present data block
*/
DataBlock_t *AllocDataBlock(int size)
{
    DataBlock_t *db;
    MemoryBlock_t *mb;
    
    db = (DataBlock_t*) malloc(sizeof(DataBlock_t));
	if (NULL == db)
	{
		return NULL;
	}
	
    memset(db,0,sizeof(DataBlock_t));
    /*allocate the real continous memory block for this data block*/
    mb = AllocMemoryBlock(size);
    
    db->mb_ptr = mb;
    db->start_ptr = db->end_ptr= mb->memory_base;
    db->next = db->prev = db->concate = NULL;

	return db;
}

void FreeDataBlock(DataBlock_t *db)
{
    DataBlock_t *temp1, *temp2;
	temp1 = db;
	while(temp1!=NULL)
	{
		temp2=temp1->concate;
		MemoryBlock_Unref(temp1->mb_ptr);
		free(temp1);
		temp1 = temp2;
	}
}

DataBlock_t *DuplicateDataBlock(DataBlock_t *db)
{
	DataBlock_t *new_datablock;
	
	return_val_if_fail(db->mb_ptr!=NULL,NULL);
	return_val_if_fail(db->mb_ptr->memory_base!=NULL,NULL);
	
	MemoryBlock_Ref(db->mb_ptr);
	new_datablock = (DataBlock_t *) malloc(sizeof(DataBlock_t));
	if (NULL == new_datablock)
	{
		return NULL;
	}
	
	memset(new_datablock, 0, sizeof(DataBlock_t));
	new_datablock->mb_ptr = db->mb_ptr;
	new_datablock->start_ptr = db->start_ptr;
	new_datablock->end_ptr = db->end_ptr;
	
	return new_datablock;
}

int ConcatedDataBlockSize(const DataBlock_t *db)
{
	int concate_blocksize = 0;

	while(db!=NULL){
		concate_blocksize += (int)(db->end_ptr- db->start_ptr);
		db = db->concate;
	}
	
	return concate_blocksize;
}

unsigned char *RFC3984_SeekStartCode(unsigned char *buf1, unsigned char *buf_end_ptr,unsigned int *prefix_length, uint8_t to_px1)
{
    unsigned char * result;    
    if (to_px1)
    {   //Separate h264 NAL with only 0001
        for(result = buf1; result < buf_end_ptr - 3; result++)
        {
            if(!memcmp((char*)result,m_h264_start_code,4))
            {
                *prefix_length = 4;
                return result;
            }
        }
    }else
    {   //Separate H264 NAL with 001 or 0001
        for(result = buf1; result < buf_end_ptr - 3; result++)
        {
            if(!memcmp((char*)result,m_h264_start_code+1,3))  
            {
                *prefix_length = 3;
                //Check for full compitable
                if(result>buf1)
                {
                    if(!memcmp((char*)result-1,m_h264_start_code,4))
                    {
                        *prefix_length = 4;
                        return result-1;
                    }
                }
                return result;
            }
        }
    }

    return NULL;
}



int RFC3984_SendPacketToBuff(SendingBuf_t *buf, DataBlock_t* nalu, uint32_t timestamp, int marker, 
                           uint8_t *ext_hdr_data, uint16_t ext_len)
{
    
    DataBlock_t *temp = NULL;
    int len = 0;
    int acclen = 0;
    unsigned char final_payload[MTU_MAX_PAYLOAD_SIZE];
    
    
    for(temp=nalu; temp != NULL; temp = temp->concate)
    {
        len = temp->end_ptr - temp->start_ptr;
        memcpy(final_payload+acclen,temp->start_ptr,len);
        acclen += len;
    }
    
    SendingBuf_Insert(buf, final_payload, acclen, timestamp, marker, ext_hdr_data, ext_len, 0);
    
    FreeDataBlock(nalu);

    return 0;
}



/**********************************************************
*
*
***********************************************************/
int RFC3984_Init()
{
    return 0;
}

 /****************************************************************************
 * Funciton Name: RFC3984_RetrieveNALU                                     
 * Parameters: 
 *                start_ptr: the frame buffer start pointer
 *                end_ptr:   the frame buffer end pointer
 *
 * Author:        Ian Tuan
 * Description:
 *                The QL330 will give us a continous buffer data
 *                that may have several NALUs seperated by a specific
 *                start code 0x00000001 which is defined in H.264 standard.
 *                Our goal is to retrieve these NALU in the frame data. 
 *****************************************************************************/
 DataBlock_t *RFC3984_RetrieveNALU(DataBlock_t *frame_data, int *end_flag, uint8_t to_px1)
{
    unsigned char *first_start_code = NULL;
    unsigned char *next_start_code = NULL;
    DataBlock_t *retrieved_nalu = NULL;
    unsigned int prefix_length;
    /*check if the frame has been retrieved all nalus */
    if (frame_data->start_ptr == frame_data->end_ptr) 
        return NULL;
    
    /*search the start code 0x00000001 */   
    first_start_code = RFC3984_SeekStartCode(frame_data->start_ptr, frame_data->end_ptr,&prefix_length, to_px1);
    
    if(first_start_code) 
    {
        retrieved_nalu = DuplicateDataBlock(frame_data);
        
        /*drop the start code*/ 
        retrieved_nalu->start_ptr = first_start_code+prefix_length;
                 
        /*start search the next start_code*/
        next_start_code = RFC3984_SeekStartCode(retrieved_nalu->start_ptr, retrieved_nalu->end_ptr,&prefix_length, to_px1);
          
        /* shift the start_ptr for next time to retreive 
        and cacluate the return nalu's size*/
        if(next_start_code) 
        {
            retrieved_nalu->end_ptr = next_start_code;;
        
            frame_data->start_ptr = next_start_code;   
            
            *end_flag = FALSE;           
        }
        else
        {
            /*we can't find next start code, it means all the remainder bytes are
            belong to this nalu*/
            retrieved_nalu->end_ptr = frame_data->end_ptr;
              
            frame_data->start_ptr = frame_data->end_ptr;             
            
            *end_flag = TRUE;  
        }         
    }
    else
    {
        /*we can't find any start code in this remainder frame
        so just shift start pointer to last*/
        frame_data->start_ptr = frame_data->end_ptr;
        
        *end_flag = TRUE;
    }

    return retrieved_nalu;
}

static inline void RFC3984_InitNALHeader(uint8_t *h, uint8_t nri, uint8_t type){
	*h=((nri&0x3)<<5) | (type & ((1<<5)-1));
}

static inline uint8_t RF3984_GetNALType(const uint8_t *h){
	return (*h) & ((1<<5)-1);
}

static inline uint8_t RFC3984_GetNALNRI(const uint8_t *h){
	return ((*h) >> 5) & 0x3;
}

void RFC3984_ConcateDataBlocks(DataBlock_t* front, DataBlock_t* rear)
{
    DataBlock_t *temp = front;
    /*search the last payload block*/
    while(temp->concate != NULL)
        temp = temp->concate; 
    /*concate rear payload block to the tail*/
    temp->concate = rear;
        
}

static void RFC3984_AddNALSize(DataBlock_t *p, uint16_t sz)
{   
	/*
	uint16_t size=htons(sz);
	*/
	//uint16_t size = sz;
	//*(uint16_t*)p->end_ptr=size;
    /*fix for ET1 arm platform, this way can fit x86 also*/
    p->end_ptr[0] = (uint8_t)(sz & 0xff);
    p->end_ptr[1] = (uint8_t)(sz >> 8);

	p->end_ptr+=2;
}

static DataBlock_t * RFC3984_FormatSTAPA(DataBlock_t *nalu)
{
    DataBlock_t *stapa_nal_block = AllocDataBlock(3);
	
    RFC3984_InitNALHeader(stapa_nal_block->end_ptr,RFC3984_GetNALNRI(nalu->start_ptr),RFC3984_TYPE_STAPA);
    stapa_nal_block->end_ptr+=1; /*shift 1 index to add NALU Size*/
    RFC3984_AddNALSize(stapa_nal_block,ConcatedDataBlockSize(nalu)); /*get nalu size and put result in nalu header*/
	
    stapa_nal_block->concate= nalu;
	
    return stapa_nal_block;
}


 /************************************************************************
 * Funciton Name: RFC3984_ConcateNALUs                                      
 * Parameters: 
 *                prev_payloadb
 *                newpayloadb
 *            
 * Author:        Ian Tuan
 * Description:
 *     
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                         NALU 1 Data                           |
 *     :                                                               :
 *     +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |               | NALU 2 Size                   | NALU 2 HDR    |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                         NALU 2 Data                           |
 *     :                                                               :
 *                
 ************************************************************************/
DataBlock_t *RFC3984_ConcateNALUs(DataBlock_t* prev_nalu, DataBlock_t *new_nalu)
{
    DataBlock_t *nal_size_block = AllocDataBlock(2);
    
    /*eventually append a stap-A header to prev_payloadb, if not already done*/
    if (RF3984_GetNALType(prev_nalu->start_ptr)!= RFC3984_TYPE_STAPA)
    {
        prev_nalu = RFC3984_FormatSTAPA(prev_nalu);
    }
    
    /*add 16-bits length nalu size ahead the new nalu */
    RFC3984_AddNALSize(nal_size_block,ConcatedDataBlockSize(new_nalu));
    nal_size_block->concate= new_nalu;
    
    /*concate them*/    
    RFC3984_ConcateDataBlocks(prev_nalu, nal_size_block);

	return prev_nalu;

}


/*
*    Pend the Nalu header to the fragment nalu
*    Reference the description of RF3984_ConcateNALUs
*/
DataBlock_t *RFC3984_FormatFUInidcatorNHeader(DataBlock_t *frag_nal, uint8_t indicator,
	int start, int end, uint8_t nal_type)
{
    DataBlock_t *frag_header = AllocDataBlock(2);

    frag_header->start_ptr[0]=indicator;
    frag_header->start_ptr[1]=((start&0x1)<<7)|((end&0x1)<<6)|nal_type;
    frag_header->end_ptr+=2;
    frag_header->concate = frag_nal;

    if (start)
    {
        /*skip original nalu header */
        frag_nal->start_ptr++;
    }

    return frag_header;
}

 /************************************************************************
 * Funciton Name: RFC3984_FragNALUToSend                                      
 * Parameters: 
 *                NALU
 *                timestamp
 *            
 * Author:        Ian Tuan
 * Description:
 *     
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     | FU indicator  |   FU header   |                               |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
 *     |                                                               |
 *     |                         FU payload                            |
 *     |                                                               |
 *     |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                               :...OPTIONAL RTP padding        |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                
 *     FU indicator:
 *     +---------------+
 *     |0|1|2|3|4|5|6|7|
 *     +-+-+-+-+-+-+-+-+
 *     |F|NRI|  Type   |
 *     +---------------+
 *     
 *     FU header: 
 *     +---------------+
 *     |0|1|2|3|4|5|6|7|
 *     +-+-+-+-+-+-+-+-+
 *     |S|E|R|  Type   |
 *     +---------------+
 ************************************************************************/
void RFC3984_FragNALUToSend(struct TL_Session_t *session, DataBlock_t *nalu, uint32_t timestamp, int marker, int maxsize, uint8_t *ext_hdr_data, uint16_t ext_len)
{
    DataBlock_t *fraged_nalu;
    int i =0;

	int payload_max_size=maxsize-2;/*minus FUA header*/
	
	uint8_t fu_indicator;
	uint8_t type = RF3984_GetNALType(nalu->start_ptr);
	uint8_t nri = RFC3984_GetNALNRI(nalu->start_ptr);
	
	int start = TRUE;
	
        /*prepare the FU indicator for each fragment*/
	RFC3984_InitNALHeader(&fu_indicator,nri,RFC3984_TYPE_FUA);
	
	/*start fragment process loop*/
	while((nalu->end_ptr - nalu->start_ptr) > payload_max_size){
		
		fraged_nalu = DuplicateDataBlock(nalu);
		
		nalu->start_ptr += payload_max_size;
		fraged_nalu->end_ptr = nalu->start_ptr;
		
		fraged_nalu = RFC3984_FormatFUInidcatorNHeader(fraged_nalu,fu_indicator,
		                                             start,FALSE,type);
		
		TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "Sending FU-A packets %d\n", i);
		RFC3984_SendPacketToBuff(session->sending_buffer, fraged_nalu,timestamp,FALSE, ext_hdr_data, ext_len);
		start=FALSE;
		i++;
	}
	
	TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "Sending FU-A packets %d\n", i);
	/*send last packet */
	fraged_nalu = RFC3984_FormatFUInidcatorNHeader(nalu, fu_indicator, FALSE, TRUE, type);
	RFC3984_SendPacketToBuff(session->sending_buffer, fraged_nalu, timestamp, marker, ext_hdr_data, ext_len);
    
}



 /************************************************************************
 * Funciton Name: RFC3984_Packetize                                      
 * Parameters: 
 *                FrameData-frame data pointer
 *                length-frame data length
 *            
 * Author:        Ian Tuan
 * Description:
 *                This function packetize the H.264 frame data 
 *                from upper layer 
 *                
 ************************************************************************/
int RFC3984_Packetize(struct TL_Session_t *session, TL_Send_Para_t *para)
{
    uint32_t     frame_len;
    uint32_t     nalu_size = 0;
    //uint8_t      *start_ptr = NULL;
    DataBlock_t  *nalu_db = NULL;
    //DataBlock_t  *prev_nalu_db = NULL;
    int          end = FALSE;
    DataBlock_t  *frame_db;
    //int          i = 0;

    frame_len = para->length;
    
    /*the PL330 will add zero bytes for 4 bytes alignment*/
    /*some codec will not accept this, and we need to remove them*/
/*
    for(i=0; i < para->length; i++)
    {
      if(para->frame_data[para->length-1-i] == 0x00)
	frame_len --;
      else
	break;
    }
*/    
    frame_db = AllocDataBlock(frame_len);
    
    memcpy(frame_db->mb_ptr->memory_base, para->frame_data, frame_len);
    frame_db->end_ptr = frame_db->mb_ptr->memory_base + frame_len;
     
    SENDING_BUF_LOCK(session->sending_buffer);
    //TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "RFC3984_Packetize\n");
    /*retreive NALU from frameData until no more NALUs*/
    while(nalu_db = RFC3984_RetrieveNALU(frame_db, &end,para->to_px1))
    {
         nalu_size = nalu_db->end_ptr - nalu_db->start_ptr;
         /*send as single nal or FU-A*/
         //if (nalu_size > MTU_MAX_PAYLOAD_SIZE)
         
	 
	 /*modify by willy*/
	 if (nalu_size > session->maximum_payload_size) //  modify by willy, 2010-08-19
         {
             /*
             TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "Sending FU-A packets\n");
             */
	      //if(end)
                       //      TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "end flag raise\n");
                       //TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "RFC3984_FragNALUToSend\n");
             RFC3984_FragNALUToSend(session, nalu_db,para->timestamp,end, session->maximum_payload_size, para->ext_hdr_data, para->ext_len);  // modify by willy, 2010-08-19
         }
         else
         {
	      TL_DEBUG_MSG(TL_RFC3984, DBG_MASK2, "Sending Single NAL\n");

              RFC3984_SendPacketToBuff(session->sending_buffer, nalu_db, para->timestamp, end, para->ext_hdr_data, para->ext_len);
         }
     
     }/*end while*/
    
    
    SENDING_BUF_UNLOCK(session->sending_buffer);
    
    /*release the frame_db data block*/
    FreeDataBlock(frame_db);
    
    return 1;
}

/************************************************************************
 * Funciton Name: RFC3984_FrameComplete                                      
 * Parameters: 
 *                
 *            
 * Author:        Ian Tuan
 * Description:   
 *                
 *                
 ************************************************************************/
int RFC3984_FrameComplete(struct TL_Session_t *session, TL_Poll_Para_t *para)
{
    pdb_e         *cp = NULL;
    pbuf_node     *curr_frame;
    rtp_packet    *packet;
    int           complete = 0;
	int           max_header_size;
    uint8_t first_packet_data = 0;

    if(session->has_opposite_ssrc)
        cp = pdb_get(session->participants, session->opposite_ssrc);
    else
        cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));

    if(!cp)
         return -1;
     
    RECVING_BUF_LOCK(cp->playout_buffer);
     
    if(!(cp->playout_buffer->frst))
    {
        
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        para->frame_buffer_size = 0;
		para->packet_buffer_size = 0;
        /*the buffer is empty so return -1*/
        return -1;
    }   

    para->frame_buffer_size = cp->playout_buffer->size;
	
	para->packet_buffer_size = cp->playout_buffer->pkt_count;
    
    curr_frame = cp->playout_buffer->frst;
    
    para->timestamp = curr_frame->rtp_timestamp; 
    
    packet = curr_frame->cdata->data;
	max_header_size = para->ext_len;
	
    /*get the exten data*/
    if(packet->meta.extn_len)
    {
        if(session->tfrc_on)
        {
           if((packet->meta.extn_len - 2) == 0)
               para->ext_len = 0; /*only tfrc information*/
           else
           {     
                if(para->ext_hdr_data)
               {
                   /*tfrc with exten information*/
                   para->ext_len = (packet->meta.extn_len-2)*4;

                    if(para->ext_len > max_header_size) 
                        para->ext_len = 0;

                    if(para->ext_len) {
                        memcpy(para->ext_hdr_data, packet->meta.extn + 12, para->ext_len);
                    }
                }
           }
        }
        else
        {
            /*only exten information*/
            para->ext_len = packet->meta.extn_len*4;
            if(para->ext_len > max_header_size) 
                para->ext_len = 0;

            if(para->ext_len) {
                memcpy(para->ext_hdr_data, packet->meta.extn + 4, para->ext_len);
            }
        }
    }
    else
        para->ext_len = 0;
    
    /*check if the oldest frame is complete*/
    complete = frame_complete(curr_frame, &(para->predict_length),cp->playout_buffer->last_removed_frame_max_seq, &first_packet_data);
    para->nal_type = RF3984_GetNALType(&first_packet_data);
    RECVING_BUF_UNLOCK(cp->playout_buffer);
     
    if(complete) {
        return 1; /*the frame complete, return positive flag*/
    }
#if 0 //RFC4585_ENABLE
    else
    {
        if(curr_frame->next != NULL)
        {
            rtp_check_rfc4585(session->rtp_session);
        }
    }
#endif

    return 0; /*the frame is not yet complete, lower the flag*/
}

/************************************************************************
* Funciton Name: RFC3984_Unpacketize                                      
* Parameters: 
*                session
*                FrameData - frame data pointer
*           
* Author:        Ian Tuan
* Description:
*                This function make packets to be frame data 
*                
************************************************************************/
int RFC3984_Unpacketize(struct TL_Session_t *session,TL_Recv_Para_t *para)
{
     pbuf_node	           *curr_frame = NULL;
     uint8_t               type;
     int                   complete = 0;
     coded_data            *codeddata = NULL;
     rtp_packet            *packet = NULL;
     uint8_t               *curr_position = NULL;
     uint8_t               fu_header;
     uint8_t               nri, start, end;
     pdb_e	           	   *cp = NULL;
     //struct     timeval    curr_time;
     unsigned char         *p = NULL;
     //uint32_t              test;
     int                   first_fua;
     //int                   i =0;
     static int            wait_count = 0;
     uint16_t              temp;
     uint32_t              predict_size;
     //uint32_t              remainder = 0;
     //uint32_t              align_zero = 0;
     uint8_t               first_packet_data = 0;
     curr_position = para->frame_data;
     para->length = 0;
     /*
         get the playout buffer
         in current case we should have only one participant
     */
     //cp = pdb_iter_init(session->participants);
    
    //cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));
    
    if(session->has_opposite_ssrc)
        cp = pdb_get(session->participants, session->opposite_ssrc);
    else
        cp = pdb_get(session->participants, rtp_my_ssrc(session->rtp_session));

     if(!cp)
         return -1;
	 
     RECVING_BUF_LOCK(cp->playout_buffer);
    
    if(!(cp->playout_buffer->frst))
   {
       wait_count++;
       if(wait_count > 10000)
       {
                TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5, "buffer empty too long session=%p, playout_buffer = %p, buffer size = %d\n", session, cp->playout_buffer, cp->playout_buffer->size);
                wait_count = 0;
       }
       //TL_DEBUG_MSG("playout_buffer is null frst=%x size=%d\n",cp->playout_buffer->frst,cp->playout_buffer->size);
       if(cp->playout_buffer->size && !(cp->playout_buffer->frst))
            TL_DEBUG_MSG(TL_RFC3984, DBG_MASK1, "playout_buffer->size and playout_buffer->frst not match\n");
       RECVING_BUF_UNLOCK(cp->playout_buffer);
       return -1;
   } 
   
   /* Queue enough packets to perform FEC recovery */
    if(session->ec_on && cp->playout_buffer->pkt_count <= VIDEO_RECV_BUFFER_PKT_NUM)		//recv pkt until number of pkts > VIDEO_RECV_BUFFER_PKT_NUM
    {
        //TL_DEBUG_MSG("video playout_buffer is buffering")
        RECVING_BUF_UNLOCK(cp->playout_buffer);
        return -1;
    }


     /*get the oldest frame from the receiving buffer*/
     curr_frame = cp->playout_buffer->frst;
     
     /*check if the oldest frame is complete*/
     complete = frame_complete(curr_frame, &predict_size,cp->playout_buffer->last_removed_frame_max_seq, &first_packet_data);
     
     
     para->nal_type = RF3984_GetNALType(&first_packet_data);
     
     para->timestamp = curr_frame->rtp_timestamp;
   
     /*if frame is complete then we can aggregate these frame packets*/
     if(complete)
         para->partial_flag = FALSE;
     else 
         para->partial_flag = TRUE;
         
         
     for(codeddata = curr_frame->cdata; codeddata->next != NULL; codeddata = codeddata->next);
        
     first_fua = TRUE;
     
     //TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5, "frame timestamp = %d\n",curr_frame->rtp_timestamp);
     while(codeddata)
     {
         packet = codeddata->data;
             
         type = RF3984_GetNALType(packet->meta.data);
             
         if(type == RFC3984_TYPE_STAPA)
         {             
            /*in case STAP-A, try to split the nalu and copy to the frame buffer*/
            uint16_t  nalu_size;
            uint8_t *buf=(uint8_t*)&nalu_size;

            TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5,"type = parameter time=%u\n",packet->fields.ts);
             
            for(p=packet->meta.data + 1 ;p < packet->meta.data + packet->meta.data_len;)
            {
                /*in QL330 each nalu must follow the start code*/
                memcpy(curr_position, m_h264_start_code, H264_START_CODE_LEN);
                curr_position += H264_START_CODE_LEN;
                buf[0] = p[0];
                buf[1] = p[1];
                    
                     
                p+=2;
                 
                memcpy(curr_position, p, nalu_size);
                curr_position += nalu_size;
                     
                p += nalu_size;
            }   
             
         }
         else if(type == RFC3984_TYPE_FUA)
         {
             /*
             in case FU-A, try to aggregate the following packet to a single NALU
             and copy to the frame buffer
             */

             fu_header = packet->meta.data[1];
             type = RF3984_GetNALType(&fu_header);

	         start = fu_header>>7;
	         end = (fu_header>>6)&0x1;

             if(first_fua)
             {
                 if(start)
                 {
                     TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5,"type = %d time=%u\n",type, packet->fields.ts);
                     first_fua = FALSE;
                     memcpy(curr_position, m_h264_start_code, H264_START_CODE_LEN);
                     curr_position += H264_START_CODE_LEN;
                    
                     /*handle the nal header if this is the start fragment*/
      	             nri = RFC3984_GetNALNRI(&(packet->meta.data[0]));    
                     RFC3984_InitNALHeader(curr_position, nri, type);
                     curr_position ++;
                  }
                  else
                  {
                      TL_DEBUG_MSG(TL_RFC3984, DBG_MASK1, "RFC3984_Unpacketize Error 1\n");
                      goto ERR_HANDLER;
                  }
             }
             else
             {
                 if(start)
                 {
                     TL_DEBUG_MSG(TL_RFC3984, DBG_MASK1, "RFC3984_Unpacketize Error 2\n");
                     goto ERR_HANDLER;
                 }
                  
                 if(end)
                 {
                     first_fua = TRUE; /*we have combined all the fragments, leave this loop*/
                     TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5,"fragment end \n");
                 }
             }
                     
             //printf("combine FUA seq=%d ts=%u\n",codeddata->seqno,packet->fields.ts);
             /*copy the nalu content into frame buffer*/
             memcpy(curr_position, &(packet->meta.data[2]), packet->meta.data_len - 2);
             curr_position += packet->meta.data_len - 2;
             //printf("copy complete\n");                 
         }
         else if((type >= RFC3984_TYPE_SINGLE_NALU_01) && (type <= RFC3984_TYPE_SINGLE_NALU_23))
         {
             /*in case single NALU, just copy the payload to frame buffer*/
             memcpy(curr_position, m_h264_start_code, H264_START_CODE_LEN);
             curr_position += H264_START_CODE_LEN;
           
             memcpy(curr_position, packet->meta.data, packet->meta.data_len);
                 curr_position += packet->meta.data_len;

         }
		 else {
             printf("<rfc3984> don't support nalu type %d.\n", type);
             goto ERR_HANDLER;
         }
        
         temp = codeddata->seqno + 1u;
         
         /*proceed the next codeddata*/
         codeddata = codeddata->prev; 
         
         if(codeddata)
         {
             if (codeddata->seqno!= temp) 
            { 
                 para->partial_flag = TRUE;
                 break;     
             }
         }
         
     }/*end while(codeddata)*/
      

     /*set the frame data length*/
     para->length = curr_position - para->frame_data;
     
     /*the pl330 need four byte align zero, give them back*/

     //remainder = (para->length) % 4;
     
     //if(remainder)
     //    printf("remainder = %u\n", remainder);

/*     
     if(remainder)
     {
         align_zero = 4 - remainder;
         for(i=0; i<align_zero;i++)
         {
             para->frame_data[para->length] = 0x00;
             para->length++; 
         }
     }
*/         
     /*remove the playout buffer*/       
     //gettimeofday(&curr_time, NULL);
     //pbuf_remove(cp->playout_buffer, curr_time);
     
     /*delete this frame from frame buffer*/
     pbuf_remove_first(cp->playout_buffer);
         
     RECVING_BUF_UNLOCK(cp->playout_buffer);

     //TL_DEBUG_MSG(TL_RFC3984, DBG_MASK5,"frame complete time = %d\n", *timestamp);
    
     return 1;
    

ERR_HANDLER:
  TL_DEBUG_MSG(TL_RFC3984, DBG_MASK1,"RF3984_Unpacketize ERR_HANDLER\n");
  para->length = 0;    
  pbuf_remove_first(cp->playout_buffer);
  RECVING_BUF_UNLOCK(cp->playout_buffer);
  
  return -1;
}




 
