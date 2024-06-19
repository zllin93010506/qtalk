/* ----------------------------------------------------------------------------------------------
   GetResolution_h264 By XiaoQ.c
   <Code base is from jggq710@yahoo.com.cn ���y>
   
   Description:
   Parsing h264 header to get some information

   History:
        2010/01/14, Frank.T        got code base from ���y
        2010/01/16, Frank.T        1. rewrite all function form C++ to C code 
                                   2. fix a bug that the program can't get correct resolution when the seq_scaling_matrix_present_flag = 1
                                   3. implement h264_GetFrameNumber(unsigned char *pInBuf, int *frame_num) that can get frame number from I slice and P slice


   Reference:
   T-REC-H.264-200903-I!!PDF-E.pdf

  ------------------------------------------------------------------------------------------------
*/
#include "QStreamH264Parser.h"
#ifndef CDRC_PARSER
#include "stdlib.h"
#ifndef X264_BUILD
enum {
    NAL_SLICE=1,
    NAL_DPA,
    NAL_DPB,
    NAL_DPC,
    NAL_SLICE_IDR,//NAL_IDR_SLICE,
    NAL_SEI,
    NAL_SPS,
    NAL_PPS,
    NAL_AUD,
    NAL_END_SEQUENCE,
    NAL_END_STREAM,
    NAL_FILLER_DATA,
    NAL_SPS_EXT,
    NAL_AUXILIARY_SLICE=19
};
#endif
#define AV_RB32(x)  ((((const uint8_t*)(x))[0] << 24) | \
                     (((const uint8_t*)(x))[1] << 16) | \
                     (((const uint8_t*)(x))[2] <<  8) | \
                      ((const uint8_t*)(x))[3])

const uint8_t ff_golomb_vlc_len[512]={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

const int8_t ff_se_golomb_vlc_code[512]={
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
  4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,  6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

const uint8_t ff_ue_golomb_vlc_code[512]={
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const uint8_t ff_log2_tab[256]={
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static inline int av_log(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}

/**
 * reads 1-17 bits.
 * Note, the alt bitstream reader can read up to 25 bits, but the libmpeg2 reader can't
 */
static inline unsigned int get_bits( int n,int *index,const uint8_t *buffer)
{
    register int tmp;
    //OPEN_READER(re, s)
    int re_index= *index;
    int re_cache= 0;
    //UPDATE_CACHE(re, s)
    re_cache= AV_RB32( (buffer)+(re_index>>3) ) << (re_index&0x07);
    //tmp= SHOW_UBITS(re, s, n);
    tmp= (((uint32_t)re_cache)>>(32-n));

    //LAST_SKIP_BITS(re, s, n)
    re_index += n;
    //CLOSE_READER(re, s)
    *index= re_index;
    return tmp;
}

static inline unsigned int get_bits1(int *index1,const uint8_t *buffer){
#ifdef ALT_BITSTREAM_READER
    int index= *index1;
    uint8_t result= buffer[ index>>3 ];
#ifdef ALT_BITSTREAM_READER_LE
    result>>= (index&0x07);
    result&= 1;
#else
    result<<= (index&0x07);
    result>>= 8 - 1;
#endif
    index++;
    *index1= index;

    return result;
#else
    return get_bits(1,index1,buffer);
#endif
}


 /**
 * read unsigned exp golomb code.
 */
static inline int get_ue_golomb(int *index,const uint8_t *buffer)
{
    unsigned int buf;
    int log;

    //OPEN_READER(re, s)
    int re_index= *index;
    int re_cache= 0;
    //UPDATE_CACHE(re, s)
    re_cache= AV_RB32( buffer+(re_index>>3) ) << (re_index&0x07);

    //buf=GET_CACHE(re, gb);
    buf =((uint32_t)re_cache);

    if(buf >= (1<<27))
    {
        buf >>= 32 - 9;

        //LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
        re_index += ff_golomb_vlc_len[buf];
        //CLOSE_READER(re, gb);
        *index= re_index;

        return ff_ue_golomb_vlc_code[buf];
    }
    else
    {
        log= 2*av_log(buf) - 31;
        buf>>= log;
        buf--;
        //LAST_SKIP_BITS(re, gb, 32 - log);
        re_index += 32 - log;
        //CLOSE_READER(re, gb);
        *index= re_index;
        return buf;
    }
}

/**
 * read signed exp golomb code.
 */
static inline int get_se_golomb(int *index,const uint8_t *buffer){
    unsigned int buf;
    int log;

    //OPEN_READER(re, s)
    int re_index= *index;
    int re_cache= 0;
    //UPDATE_CACHE(re, s)
    re_cache= AV_RB32( buffer+(re_index>>3) ) << (re_index&0x07);
    //re_cache= AV_RB32( ((const uint8_t *)buffer)+(re_index>>3) ) << (re_index&0x07);

    //buf=GET_CACHE(re, gb);
    buf =((uint32_t)re_cache);

    if(buf >= (1<<27)){
        buf >>= 32 - 9;

        //LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
        re_index += ff_golomb_vlc_len[buf];
        //CLOSE_READER(re, gb);
        *index= re_index;
        return ff_se_golomb_vlc_code[buf];
    }else{
        log= 2*av_log(buf) - 31;
        buf>>= log;

        //LAST_SKIP_BITS(re, gb, 32 - log);
        re_index += 32 - log;

        //CLOSE_READER(re, gb);
        *index= re_index;
        return buf;

        if(buf&1) buf= -(buf>>1);
        else      buf=  (buf>>1);

        return buf;
    }
}


static inline int _H264_GetResolution(const uint8_t *buffer,int *width,int *height)   //decode_seq_parameter_set
{
    int index = 0;
    int profile_idc;
    int poc_type,poc_cycle_length,ref_frame_count;
    int i;

    profile_idc = get_bits(8,&index,buffer);
    get_bits1(&index,buffer);       // constraint_set0_flag
    get_bits1(&index,buffer);       // constraint_set1_flag
    get_bits1(&index,buffer);       // constraint_set2_flag
    get_bits1(&index,buffer);       // constraint_set3_flag
    get_bits(4,&index,buffer);      // reserved
    get_bits(8,&index,buffer);      // level_idc
    get_ue_golomb(&index,buffer);   // sps_id

    if(profile_idc >= 100)//high profile
    {
        if(get_ue_golomb(&index,buffer) == 3) //chroma_format_idc
            get_bits1(&index,buffer);         //residual_color_transform_flag
        get_ue_golomb(&index,buffer);         //bit_depth_luma_minus8
        get_ue_golomb(&index,buffer);         //bit_depth_chroma_minus8
        get_bits1(&index,buffer);             //transform_bypass
        //decode_scaling_matrices(h, sps, NULL, 1, sps->scaling_matrix4, sps->scaling_matrix8);
    }
    /*else
        scaling_matrix_present = 0;*/

    get_ue_golomb(&index,buffer) + 4;      //log2_max_frame_num
    poc_type= get_ue_golomb(&index,buffer);//poc_type

    if(poc_type == 0)//FIXME #define
    {
        get_ue_golomb(&index,buffer) + 4;//log2_max_poc_lsb
    }
    else if(poc_type == 1)//FIXME #define
    {
        get_bits1(&index,buffer);           //delta_pic_order_always_zero_flag
        get_se_golomb(&index,buffer);       //offset_for_non_ref_pic
        get_se_golomb(&index,buffer);       //offset_for_top_to_bottom_field
        poc_cycle_length = get_ue_golomb(&index,buffer);//poc_cycle_length

        for(i=0; i<poc_cycle_length; i++)
            get_se_golomb(&index,buffer); //offset_for_ref_frame
    }
    else if(poc_type != 2)
    {
        return -1;
    }

    ref_frame_count = get_ue_golomb(&index,buffer);//ref_frame_count
    if(ref_frame_count > 32-2)
    {
        return -1;
    }

    get_bits1(&index,buffer); //gaps_in_frame_num_allowed_flag
    *width = (get_ue_golomb(&index,buffer) + 1)*16;
    *height= (get_ue_golomb(&index,buffer) + 1)*16;
    return 0;
}

/**
 * identifies the exact end of the bitstream
 * @return the length of the trailing, or 0 if damaged
 */
static inline int decode_rbsp_trailing(const uint8_t *src)
{
    int v= *src;
    int r;

    for(r=1; r<9; r++){
        if(v&1) return r;
        v>>=1;
    }
    return 0;
}
static const uint8_t *decode_nal(const uint8_t *src, int *dst_length, int *consumed,int length,int *type)
{
    int i, si, di;
    *type = src[0]&0x1F;
    src++; length--;

    for(i=0; i+1<length; i+=2){
        if(src[i]) continue;
        if(i>0 && src[i-1]==0) i--;
        if(i+2<length && src[i+1]==0 && src[i+2]<=3){
            if(src[i+2]!=3){
                /* startcode, so we must be past the end */
                length=i;
            }
            break;
        }
    }

    if(i>=length-1){ //no escaped 0
        *dst_length= length;
        *consumed= length+1; //+1 for the header
        return src;
    }
    //dst = (uint8_t *)malloc(17*length/16 + 32);
    //printf("decoding esc\n");
    si=di=0;
    while(si<length){
        //remove escapes (very rare 1:2^22)
        if(si+2<length && src[si]==0 && src[si+1]==0 && src[si+2]<=3){
            if(src[si+2]==3){ //escape
                //dst[di++]= 0;
                //dst[di++]= 0;
                si+=3;
                continue;
            }else //next start code
                break;
        }
        si++;
        //dst[di++]= src[si++];
    }

    *dst_length= si + 1;//di + 1;
    *consumed= si + 1;
    return src;
}
int h264_get_resolution(unsigned char *buffer,int buf_size,int *height,int *width)   //decode_seq_parameter_set
{
    int buf_index=0;
    int nal_unit_type;
    int consumed;
    int dst_length;
    const uint8_t *ptr = NULL;

    // start code prefix search find out 001
    for(; buf_index + 3 < buf_size; buf_index++)
    {   // This should always succeed in the first iteration.
        if(buffer[buf_index] == 0 && buffer[buf_index+1] == 0 && buffer[buf_index+2] == 1)
            break;
    }
    if(buf_index+3 < buf_size)
        buf_index+=3;

    //spilt one nal
    ptr= decode_nal(buffer + buf_index, &dst_length,&consumed, buf_size - buf_index,&nal_unit_type);
    if (ptr==NULL || dst_length < 0)
    {
        return QS_FAIL;
    }
    if(nal_unit_type == NAL_SPS)
    {
        _H264_GetResolution(ptr,width,height);
        return QS_TRUE;
    }
    return QS_FAIL;
}

int h264_get_frame_num(unsigned char *buf,int buf_size,int *frame_type, int *codecType, int *count)//,int *frame_width,int *frame_height
{
    int buf_index=0;
    for(;;)
    {
        int nal_unit_type;
        int consumed;
        int dst_length;
        int bit_length;
        const uint8_t *ptr = NULL;

        // start code prefix search find out 001
        for(; buf_index + 3 < buf_size; buf_index++)
        {   // This should always succeed in the first iteration.
            if(buf[buf_index] == 0 && buf[buf_index+1] == 0 && buf[buf_index+2] == 1)
                break;
        }
        if(buf_index+3 < buf_size)
            buf_index+=3;

        //spilt one nal
        ptr= decode_nal(buf + buf_index, &dst_length,&consumed, buf_size - buf_index,&nal_unit_type);
        if (ptr==NULL || dst_length < 0)
        {
            return -1;
        }

        while(ptr[dst_length - 1] == 0 && dst_length > 0)
            dst_length--;

        bit_length= !dst_length ? 0 : (8*dst_length - decode_rbsp_trailing( ptr + dst_length - 1));
        buf_index += consumed;

        switch(nal_unit_type)
        {
        case NAL_SLICE:     // P silde
        case NAL_SLICE_IDR: // I slide
        {
            // H264 Sequence Parameter Set (0x67), I slice (2)
            int index = 0;
            get_ue_golomb(&index,ptr);  //first_mb_in_slice
            int slice_type = get_ue_golomb(&index,ptr);  //slice_type
            get_ue_golomb(&index,ptr);  //pps_id

            if(slice_type > 9)
                return -1;

            switch(slice_type)
            {
            case 0:
            case 5:
                *frame_type = P_SLICE_TYPE;
                break;
            case 1:
            case 6:
                *frame_type = B_SLICE_TYPE;
                break;
            case 2:
            case 7:
                *frame_type = I_SLICE_TYPE;
                break;
            }

            int a4 = get_bits(4,&index,ptr);    //suppose streaming is X264.  X264`s magic number is 4
            int a10= get_bits(6,&index,ptr);    //suppose streaming is PL330. PL330`s magic number is 10

            if(*frame_type ==I_SLICE_TYPE && a10 > 1) //When streaming is X264 ,(a10 == 37 || a10 == 17)
            {                                          //when streaming is PL330, a4 always equal 0
                *codecType = 1;
            }
            else if(*frame_type ==I_SLICE_TYPE && a10 == 1) //When streaming is INTEL ,(a10 == 1)
            {
                *codecType = 2;
            }

            if(*codecType == 1)
                a10 = a4;
            else if(*codecType == 2)
                a10 = a10>>2;

            if(*frame_type == I_SLICE_TYPE)
            {
                *count = 0;
            }
            else if(a10 == 0)//P_SLICE_TYPE && a10 = 0 ==> stand for 16 account
            {
                (*count)++;
            }

            a10 +=16*(*count);
            return a10;
        }
        break;
        case NAL_SPS:
        {
            //_H264_GetResolution(ptr,frame_width,frame_height);
        }
        case NAL_PPS:
        default:
            ;
        }
    }
}

#else
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define dbgPrintf    //printf

inline void h264_SE_V(int *nIndex, int *nShiftCount, unsigned char *nShiftBuffer, int *nRecv, unsigned char *pInBuf);
inline void h264_MyShift(int nCount, int *nIndex, int *nShiftCount, unsigned char *nShiftBuffer, unsigned long *nRecv, unsigned char *pInBuf);
inline void h264_UE_V(int *nIndex, int *nShiftCount, unsigned char *nShiftBuffer, unsigned long *nRecv, unsigned char *pInBuf);


// parsing I slice and P slice header to get frame number
inline unsigned short h264_GetFrameNum_pl330only(unsigned char *buf, unsigned short len, int *frame_type)
{
    int index = 0;    
    unsigned short frame_num = 0;    
    
    if(buf[index+4] == 0x67) // H264 Sequence Parameter Set (0x67), I slice (2)
    {        
        index += 108;           // jump to I slice
        
        index += 5;             // jump to frame number byte
        frame_num = buf[index];
        frame_num <<= 8;        
        frame_num |= buf[index+1];       
        frame_num >>= 1;
        *frame_type = 2;
    }
    else { // P slice (0)
        index = index + 5;      // jump to frame number byte
        frame_num = buf[index];
        frame_num <<= 8;        
        frame_num |= buf[index+1];       
        frame_num >>= 3;
        *frame_type = 0;
    }
    
    return (frame_num & 0x3FF);
}


// Parsing sequence parameter set of h264 to get width and height
inline int h264_GetResolution_pl330only(unsigned char *pInBuf, int *nHeight, int *nWidth)
{
    int nIndex = 0;
    int nShiftCount = 0;
    unsigned char *pFrameHead;
    unsigned char nShiftBuffer=0;   
    unsigned long pic_width_in_mbs_minus1=0;
    unsigned long pic_height_in_map_units_minus1=0;
    unsigned long temp = 0;
    
    // resolution only be carried in the sequence parameter set
    if(!(pInBuf[0]==0x00 && 
         pInBuf[1]==0x00 && 
         pInBuf[2]==0x00 && 
         pInBuf[3]==0x01 && 
         pInBuf[4]==0x67))
        return -1;

    nIndex = 81;
    pFrameHead = pInBuf+nIndex; //jump to resolution field
    nIndex =- 1;                //reset index
    
    h264_MyShift(3, &nIndex, &nShiftCount, &nShiftBuffer, &temp,  pFrameHead);
    
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &pic_width_in_mbs_minus1, pFrameHead);
	
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &pic_height_in_map_units_minus1, pFrameHead);
	
    *nWidth = (pic_width_in_mbs_minus1+1)<<4;
    *nHeight = (pic_height_in_map_units_minus1+1)<<4;
    
    return 0;
}


int search_slice_offset(unsigned char *buf, unsigned short len, unsigned short *offset)
{
    unsigned short index = 0;
    int frame_type;

    frame_type = -1;
    for(;;) 
    {
        if((index) >= len-3) {
            dbgPrintf("<warning> can't find out i-slice\n");
            return -1;
        }

        if(buf[index+0] == 0x00 && buf[index+1] == 0x00 && buf[index+2] == 0x01) {
            
            if((buf[index+3]&NAL_TYPE_MASK) == NAL_TYPE_IDR) frame_type = I_SLICE_TYPE;
            else if((buf[index+3]&NAL_TYPE_MASK) == NAL_TYPE_NON_IDR) frame_type = P_SLICE_TYPE;
            else ;

            if(frame_type >= 0) {
                *offset = index;
                return frame_type;
            }          
        }
        index += 1;
    }
    return -1;
}

int h264_get_frame_num(unsigned char *buf, int len, int *frame_type, int log2_max_frame_num_minus4, int *pCount)
{
    unsigned short slice_offset;
    int index = 0, shift_cnt = 0, field_len;
    unsigned char *p_frame_head;
    unsigned char shift_buffer = 0;
    unsigned long tmp, frame_num, multi_num;
    int count = *pCount;

    field_len = log2_max_frame_num_minus4+4;
    multi_num = (unsigned long)0x00000001 << field_len;
    index = 0;
    if(((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_IDR)||((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_NON_IDR)) {        
        if((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_IDR) *frame_type = I_SLICE_TYPE;
        else if((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_NON_IDR) *frame_type = P_SLICE_TYPE;
        else *frame_type = -1;
        
        p_frame_head = buf+index+5;
        index = -1;
        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
        h264_MyShift(field_len, &index, &shift_cnt, &shift_buffer, &frame_num, p_frame_head);
    }
    else if((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_SEQ_PARA_SET) {
        *frame_type = search_slice_offset(buf, len, &slice_offset);
        if(*frame_type < 0) {         
            return 0;
        }
        
        p_frame_head = buf+slice_offset+4;
        index = -1;
        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
        h264_MyShift(field_len, &index, &shift_cnt, &shift_buffer, &frame_num, p_frame_head);
    }
    else {
        *frame_type = -1;
        return 0;
    }

    if(*frame_type == I_SLICE_TYPE) count = 0;
    else if(frame_num == 0) count++;

    frame_num += multi_num*count;
    *pCount = count;
    //printf("num:%02d => multi:%02dxcnt:%02d, %s\n", frame_num, multi_num, count, *frame_type==I_SLICE_TYPE?"i-slice":"p-slice");
    return (int)frame_num;
}


//inline int h264_get_slice_info(unsigned char *buf,
//                               unsigned short len,
//                               h264_para_set_t *para_set,
//                               h264_slice_info_t *slice)
//{
//    unsigned short slice_offset;
//    int index = 0, shift_cnt = 0;
//    unsigned char *p_frame_head;
//    unsigned char shift_buffer = 0;
//    unsigned long tmp, frame_num;
//
//
//    index = 0;
//    if(((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_IDR)||((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_NON_IDR)) {
//        if((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_IDR) slice->frame_type = I_SLICE_TYPE;
//        else if((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_NON_IDR) slice->frame_type = P_SLICE_TYPE;
//        else slice->frame_type = 0xFF;
//
//        p_frame_head = buf+index+5;
//        index = -1;
//    }
//    else if((buf[index+4] & NAL_TYPE_MASK) == NAL_TYPE_SEQ_PARA_SET) {
//        if(search_slice_offset(buf, len, &slice_offset) < 0) {
//            slice->frame_type = -1;
//            return QS_FAIL;
//        }
//        if((buf[slice_offset+4] & NAL_TYPE_MASK) == NAL_TYPE_IDR) slice->frame_type = I_SLICE_TYPE;
//        else if((buf[slice_offset+4] & NAL_TYPE_MASK) == NAL_TYPE_NON_IDR) slice->frame_type = P_SLICE_TYPE;
//        else slice->frame_type = 0xFF;
//
//        p_frame_head = buf+slice_offset+5;
//        index = -1;
//    }
//    else {
//        slice->frame_type = -1;
//        return QS_FAIL;
//    }
//    slice->nal_unit_type = (buf[index+4] & NAL_TYPE_MASK);
//
//    h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
//    h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
//    h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head);
//    h264_MyShift(10, &index, &shift_cnt, &shift_buffer, &frame_num, p_frame_head);
//    slice->frame_num = (unsigned short)(frame_num & 0xFF);
//
//    if(para_set->is_vaild == 0)
//        return QS_TRUE;
//
//    if(para_set->frame_mbs_only_flag == 0) {
//        h264_MyShift(1, &index, &shift_cnt, &shift_buffer, &tmp, p_frame_head); // field_pic_flag
//        if(tmp) {
//            h264_MyShift(1, &index, &shift_cnt, &shift_buffer, &tmp, p_frame_head); // bottom_field_flag
//        }
//    }
//    if(slice->nal_unit_type == 5) {
//        h264_UE_V(&index, &shift_cnt, &shift_buffer, &tmp, p_frame_head); // idr_pic_id
//    }
//    if(para_set->pic_order_cnt_type == 0) {
//        unsigned long poc_lsb;
//        h264_MyShift(5, &index, &shift_cnt, &shift_buffer, &poc_lsb, p_frame_head);
//        slice->pic_order_cnt = poc_lsb;
//    }
//
//    return QS_TRUE;
//}

int h264_get_parameter_set(unsigned char *pInBuf, int len, h264_para_set_t *para_set)
{
    int nIndex = 0;
    unsigned char *pFrameHead;
    int nShiftCount=0;
    int i = 0, j = 0;
    unsigned char nShiftBuffer=0;
    unsigned long temp=0;

    if(!(pInBuf[0]==0x00 && pInBuf[1]==0x00 && pInBuf[2]==0x00 && pInBuf[3]==0x01)) {
        if((pInBuf[4]&NAL_TYPE_MASK) != NAL_TYPE_SEQ_PARA_SET)
            return QS_FAIL;
    }

    pFrameHead = pInBuf+nIndex+5;   //point to the byte after 0x67
    nIndex =- 1;                    //reset index
    
    h264_MyShift(8, &nIndex, &nShiftCount, &nShiftBuffer, &(para_set->profile_idc),  pFrameHead);
    dbgPrintf("profile_idc: %u\n", (unsigned int)(para_set->profile_idc));

    nIndex += 1;    // bypass  constraint_set0_flag u(1),  constraint_set1_flag u(1), 
                    // constraint_set2_flag u(1), constraint_set3_flag u(1) and reserved_zero_4bits u(4)

    h264_MyShift(8, &nIndex, &nShiftCount, &nShiftBuffer, &(para_set->level_idc),  pFrameHead);
    dbgPrintf("level_idc: %u\n", (unsigned int)(para_set->level_idc));
    
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &(para_set->seq_parameter_set_id), pFrameHead);
    dbgPrintf("seq_parameter_set_id: %u\n", (unsigned int)(para_set->seq_parameter_set_id));
    
    if( para_set->profile_idc  ==  100  ||  para_set->profile_idc  ==  110  ||
        para_set->profile_idc  ==  122  ||  para_set->profile_idc  ==  144 ) {
        
        h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &(para_set->chroma_format_idc), pFrameHead);
        dbgPrintf("        chroma_format_idc: %u\n", (unsigned int)(para_set->chroma_format_idc));
            
        if( para_set->chroma_format_idc == 3)
        {
            unsigned long residual_colour_transform_flag=0;
            h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &residual_colour_transform_flag, pFrameHead);
            dbgPrintf("        residual_colour_transform_flag: %u\n", (unsigned int)residual_colour_transform_flag);
        }

        h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->bit_depth_luma_minus8, pFrameHead);
        dbgPrintf("        bit_depth_luma_minus8: %u\n", (unsigned int)para_set->bit_depth_luma_minus8);
            
        h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->bit_depth_chroma_minus8, pFrameHead);
        dbgPrintf("        bit_depth_chroma_minus8: %u\n", (unsigned int)para_set->bit_depth_chroma_minus8);
            
        h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &para_set->qpprime_y_zero_transform_bypass_flag, pFrameHead);
        dbgPrintf("        qpprime_y_zero_transform_bypass_flag: %u\n", (unsigned int)para_set->qpprime_y_zero_transform_bypass_flag);
            
        h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &para_set->seq_scaling_matrix_present_flag, pFrameHead);
        dbgPrintf("        seq_scaling_matrix_present_flag: %u\n", (unsigned int)para_set->seq_scaling_matrix_present_flag);
            
        if( para_set->seq_scaling_matrix_present_flag ) {

            for(i = 0; i <((para_set->chroma_format_idc != 3)?8:12); i++) {
                temp = 0;                                                                                                            
                h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &temp, pFrameHead);                                                            
                                                                                                                                     
                if(temp) {                                                                                                                           
                    unsigned char lastScale = 8;                                                                                                 
                    unsigned char nextScale = 8;                                                                                                     
                    int delta_scale = 0;
                                                                                                                                     
                    if(i < 6) { // sizeOfScalingList = 16                                                                                            
                        for(j = 0; j < 16; j++) {                                                                                                
                            if(nextScale != 0) {                                                                                                 
                                delta_scale = 0;                                                                                                 
                                h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &delta_scale, pFrameHead);                                           
                                nextScale = (lastScale + delta_scale + 256) % 256;                                                                   
                            }                                                                                                                        
                            lastScale = (nextScale == 0)?lastScale:nextScale;                                                                        
                        }                                                                                                                            
                    }                                                                                                                                
                    else { // sizeOfScalingList = 64                                                                                                 
                        for(j = 0; j < 64; j++) {                                                                                                
                            if(nextScale != 0) {                                                                                                 
                                delta_scale = 0;                                                                                                 
                                h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &delta_scale, pFrameHead);                                           
                                nextScale = (lastScale + delta_scale + 256) % 256;                                                                   
                            }                                                                                                                        
                            lastScale = (nextScale == 0)?lastScale:nextScale;                                                                        
                        }                                                                                                                            
                    }                                                                                                                                
                }                                                                                                                                    
            }

        }
    }
    
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->log2_max_frame_num_minus4, pFrameHead);    
    dbgPrintf("log2_max_frame_num_minus4: %u\n", (unsigned int)para_set->log2_max_frame_num_minus4);
    
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->pic_order_cnt_type, pFrameHead);
    dbgPrintf("pic_order_cnt_type: %u\n", (unsigned int)para_set->pic_order_cnt_type);

    if( para_set->pic_order_cnt_type  == 0 )
    {
        unsigned long log2_max_pic_order_cnt_lsb_minus4=0;
        h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &log2_max_pic_order_cnt_lsb_minus4, pFrameHead);
        dbgPrintf(" log2_max_pic_order_cnt_lsb_minus4: %u\n", (unsigned int)log2_max_pic_order_cnt_lsb_minus4);
    }
    else if( para_set->pic_order_cnt_type == 1 ) {
        unsigned long delta_pic_order_always_zero_flag=0;
        int offset_for_non_ref_pic=0;
        int offset_for_top_to_bottom_field=0;
        unsigned long num_ref_frames_in_pic_order_cnt_cycle=0;
        int *offset_for_ref_frame;
        
        h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &delta_pic_order_always_zero_flag, pFrameHead);       
        dbgPrintf(" delta_pic_order_always_zero_flag: %u\n", (unsigned int)delta_pic_order_always_zero_flag);
        
        h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &offset_for_non_ref_pic, pFrameHead);       
        dbgPrintf(" offset_for_non_ref_pic: %u\n", offset_for_non_ref_pic);
        
        h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &offset_for_top_to_bottom_field, pFrameHead);   
        dbgPrintf(" offset_for_top_to_bottom_field: %u\n", offset_for_top_to_bottom_field);
        
        h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &num_ref_frames_in_pic_order_cnt_cycle, pFrameHead);
        dbgPrintf(" num_ref_frames_in_pic_order_cnt_cycle: %u\n", (unsigned int)num_ref_frames_in_pic_order_cnt_cycle);
            
        offset_for_ref_frame = (int *)malloc(num_ref_frames_in_pic_order_cnt_cycle*sizeof(int)); //new int[ num_ref_frames_in_pic_order_cnt_cycle ];

        for(i=0;i<(int)num_ref_frames_in_pic_order_cnt_cycle;i++)
            offset_for_ref_frame[ i ] = 0;

        for(i = 0; i <(int) num_ref_frames_in_pic_order_cnt_cycle; i++ ) {
            h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &offset_for_ref_frame[ i ], pFrameHead);
            dbgPrintf(" offset_for_ref_frame[%u]: %u\n", i, offset_for_ref_frame[ i ]);
         }
        free(offset_for_ref_frame);
    }

    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->num_ref_frames, pFrameHead);   
    dbgPrintf("num_ref_frames: %u\n", (unsigned int)para_set->num_ref_frames);
    
    h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &para_set->gaps_in_frame_num_value_allowed_flag, pFrameHead);   
    dbgPrintf("gaps_in_frame_num_value_allowed_flag: %u\n", (unsigned int)para_set->gaps_in_frame_num_value_allowed_flag);
    
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->pic_width_in_mbs_minus1, pFrameHead);
    dbgPrintf("pic_width_in_mbs_minus1: %u\n", (unsigned int)para_set->pic_width_in_mbs_minus1);
    
    h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &para_set->pic_height_in_map_units_minus1, pFrameHead);
    dbgPrintf("pic_height_in_map_units_minus1: %u\n", (unsigned int)para_set->pic_height_in_map_units_minus1);
    
    h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &para_set->frame_mbs_only_flag, pFrameHead);   
    dbgPrintf("frame_mbs_only_flag: %u\n", (unsigned int)para_set->frame_mbs_only_flag);    
    
    para_set->is_vaild = 1;

    return QS_TRUE;
}

int h264_get_resolution(unsigned char *pInBuf,int buf_size, int *nHeight, int *nWidth)
{
    int nIndex = 0;
    unsigned char *pFrameHead;
    int nShiftCount=0;
    int i = 0, j = 0;
    unsigned char nShiftBuffer=0;
    unsigned long profile_idc=0;
    unsigned long seq_parameter_set_id=0;
    unsigned long bit_depth_chroma_minus8=0;
    unsigned long bit_depth_luma_minus8=0;
    unsigned long qpprime_y_zero_transform_bypass_flag=0;
    unsigned long seq_scaling_matrix_present_flag=0;
    unsigned long log2_max_frame_num_minus4=0;
    unsigned long pic_order_cnt_type=0;
    unsigned long gaps_in_frame_num_value_allowed_flag=0;
    unsigned long pic_width_in_mbs_minus1=0;
    unsigned long pic_height_in_map_units_minus1=0;
    unsigned long level_idc=0;
    unsigned long chroma_format_idc=0;
    unsigned long num_ref_frames;
    unsigned long temp=0;
	
    // resolution only be carried in the sequence parameter set
    if(!(pInBuf[0]==0x00 && 
         pInBuf[1]==0x00 && 
         pInBuf[2]==0x00 && 
         pInBuf[3]==0x01 && 
         pInBuf[4]==0x67))
        return -1;

	pFrameHead = pInBuf+nIndex+5;   //point to the byte after 0x67
	nIndex =- 1;                    //reset index
	
	h264_MyShift(8, &nIndex, &nShiftCount, &nShiftBuffer, &profile_idc,  pFrameHead);
	dbgPrintf("profile_idc: %u\n", (unsigned int)profile_idc);

	nIndex += 1;    // bypass  constraint_set0_flag u(1),  constraint_set1_flag u(1), 
                    // constraint_set2_flag u(1), constraint_set3_flag u(1) and reserved_zero_4bits u(4)

	h264_MyShift(8, &nIndex, &nShiftCount, &nShiftBuffer, &level_idc,  pFrameHead);
	dbgPrintf("level_idc: %u\n", (unsigned int)level_idc);
	
	h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &seq_parameter_set_id, pFrameHead);
	dbgPrintf("seq_parameter_set_id: %u\n", (unsigned int)seq_parameter_set_id);
	
	if( profile_idc  ==  100  ||  profile_idc  ==  110  ||
            profile_idc  ==  122  ||  profile_idc  ==  144 ) {
		
		h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &chroma_format_idc, pFrameHead);
		dbgPrintf("        chroma_format_idc: %u\n", (unsigned int)chroma_format_idc);
			
 		if( chroma_format_idc == 3)
		{
			unsigned long residual_colour_transform_flag=0;
			h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &residual_colour_transform_flag, pFrameHead);
			dbgPrintf("        residual_colour_transform_flag: %u\n", (unsigned int)residual_colour_transform_flag);
		}

		h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &bit_depth_luma_minus8, pFrameHead);
		dbgPrintf("        bit_depth_luma_minus8: %u\n", (unsigned int)bit_depth_luma_minus8);
			
		h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &bit_depth_chroma_minus8, pFrameHead);
		dbgPrintf("        bit_depth_chroma_minus8: %u\n", (unsigned int)bit_depth_chroma_minus8);
			
		h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &qpprime_y_zero_transform_bypass_flag, pFrameHead);
		dbgPrintf("        qpprime_y_zero_transform_bypass_flag: %u\n", (unsigned int)qpprime_y_zero_transform_bypass_flag);
			
		h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &seq_scaling_matrix_present_flag, pFrameHead);
		dbgPrintf("        seq_scaling_matrix_present_flag: %u\n", (unsigned int)seq_scaling_matrix_present_flag);
			
 		if( seq_scaling_matrix_present_flag ) {

			for(i = 0; i <((chroma_format_idc != 3)?8:12); i++) {
                temp = 0;                                                                                                            
                h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &temp, pFrameHead);                                                            
                                                                                                                                     
                if(temp) {                                                                                                                           
                    unsigned char lastScale = 8;                                                                                                 
                    unsigned char nextScale = 8;                                                                                                     
                    int delta_scale = 0;
                                                                                                                                     
                    if(i < 6) { // sizeOfScalingList = 16                                                                                            
                        for(j = 0; j < 16; j++) {                                                                                                
                            if(nextScale != 0) {                                                                                                 
                                delta_scale = 0;                                                                                                 
                                h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &delta_scale, pFrameHead);                                           
                                nextScale = (lastScale + delta_scale + 256) % 256;                                                                   
                            }                                                                                                                        
                            lastScale = (nextScale == 0)?lastScale:nextScale;                                                                        
                        }                                                                                                                            
                    }                                                                                                                                
                    else { // sizeOfScalingList = 64                                                                                                 
                        for(j = 0; j < 64; j++) {                                                                                                
                            if(nextScale != 0) {                                                                                                 
                                delta_scale = 0;                                                                                                 
                                h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &delta_scale, pFrameHead);                                           
                                nextScale = (lastScale + delta_scale + 256) % 256;                                                                   
                            }                                                                                                                        
                            lastScale = (nextScale == 0)?lastScale:nextScale;                                                                        
                        }                                                                                                                            
                    }                                                                                                                                
                }                                                                                                                                    
            }

		}
	}
	
	h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &log2_max_frame_num_minus4, pFrameHead);	
	dbgPrintf("log2_max_frame_num_minus4: %u\n", (unsigned int)log2_max_frame_num_minus4);
	
	h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &pic_order_cnt_type, pFrameHead);
	dbgPrintf("pic_order_cnt_type: %u\n", (unsigned int)pic_order_cnt_type);
	
  	if( pic_order_cnt_type  == 0 )
	{
		unsigned long log2_max_pic_order_cnt_lsb_minus4=0;
		h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &log2_max_pic_order_cnt_lsb_minus4, pFrameHead);
		dbgPrintf("	log2_max_pic_order_cnt_lsb_minus4: %u\n", (unsigned int)log2_max_pic_order_cnt_lsb_minus4);
	}
	else if( pic_order_cnt_type == 1 ) {
		unsigned long delta_pic_order_always_zero_flag=0;
		int offset_for_non_ref_pic=0;
		int offset_for_top_to_bottom_field=0;
		unsigned long num_ref_frames_in_pic_order_cnt_cycle=0;
		int *offset_for_ref_frame;
		
		h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &delta_pic_order_always_zero_flag, pFrameHead);		
		dbgPrintf("	delta_pic_order_always_zero_flag: %u\n", (unsigned int)delta_pic_order_always_zero_flag);
		
		h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &offset_for_non_ref_pic, pFrameHead);		
		dbgPrintf("	offset_for_non_ref_pic: %u\n", offset_for_non_ref_pic);
		
		h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &offset_for_top_to_bottom_field, pFrameHead);	
		dbgPrintf("	offset_for_top_to_bottom_field: %u\n", offset_for_top_to_bottom_field);
		
		h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &num_ref_frames_in_pic_order_cnt_cycle, pFrameHead);
		dbgPrintf("	num_ref_frames_in_pic_order_cnt_cycle: %u\n", (unsigned int)num_ref_frames_in_pic_order_cnt_cycle);
			
		offset_for_ref_frame = (int *)malloc(num_ref_frames_in_pic_order_cnt_cycle*sizeof(int)); //new int[ num_ref_frames_in_pic_order_cnt_cycle ];

		for(i=0;i<(int)num_ref_frames_in_pic_order_cnt_cycle;i++)
			offset_for_ref_frame[ i ] = 0;

	 	for(i = 0; i <(int) num_ref_frames_in_pic_order_cnt_cycle; i++ ) {
                        h264_SE_V(&nIndex, &nShiftCount, &nShiftBuffer, &offset_for_ref_frame[ i ], pFrameHead);
                        dbgPrintf("	offset_for_ref_frame[%u]: %u\n", i, offset_for_ref_frame[ i ]);
		 }
		free(offset_for_ref_frame);
	}

	h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &num_ref_frames, pFrameHead);	
	dbgPrintf("num_ref_frames: %u\n", (unsigned int)num_ref_frames);
	
	h264_MyShift(1, &nIndex, &nShiftCount, &nShiftBuffer, &gaps_in_frame_num_value_allowed_flag, pFrameHead);	
	dbgPrintf("gaps_in_frame_num_value_allowed_flag: %u\n", (unsigned int)gaps_in_frame_num_value_allowed_flag);
    
	h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &pic_width_in_mbs_minus1, pFrameHead);
	dbgPrintf("pic_width_in_mbs_minus1: %u\n", (unsigned int)pic_width_in_mbs_minus1);
	
	h264_UE_V(&nIndex, &nShiftCount, &nShiftBuffer, &pic_height_in_map_units_minus1, pFrameHead);
	dbgPrintf("pic_height_in_map_units_minus1: %u\n", (unsigned int)pic_height_in_map_units_minus1);
	
	*nWidth=(pic_width_in_mbs_minus1+1)*16;
	*nHeight=(pic_height_in_map_units_minus1+1)*16;

    return 0;
}


inline void h264_UE_V(int *nIndex, int *nShiftCount, unsigned char *nShiftBuffer, unsigned long *nRecv, unsigned char *pInBuf)
{
	int leadingZeroBits = -1;
	unsigned long b;
	unsigned long temp = 0;

	for( b = 0;  b!=1; leadingZeroBits++ )
        h264_MyShift(1, nIndex, nShiftCount, nShiftBuffer, &b, pInBuf);

	h264_MyShift(leadingZeroBits, nIndex, nShiftCount, nShiftBuffer, &temp, pInBuf);
	*nRecv =(unsigned long) pow(2, leadingZeroBits) - 1 + temp;    
}


inline void h264_SE_V(int *nIndex, int *nShiftCount, unsigned char *nShiftBuffer, int *nRecv, unsigned char *pInBuf)
{
	unsigned long k=0;
	h264_UE_V(nIndex,  nShiftCount, nShiftBuffer, &k, pInBuf);
	switch (k)
	{
		case 0:
			*nRecv=0;
			break;
		case 1:
			*nRecv=1;
			break;
		case 2:
			*nRecv=-1;
			break;
		case 3:
			*nRecv=2;
			break;
		case 4:
			*nRecv=-2;
			break;
		case 5:
			*nRecv=3;
			break;
		case 6:
			*nRecv=-3;
			break;
		default:
			*nRecv=(int)(pow(-1,k+1)*ceil(k/2));
			break;
	}
}

inline unsigned long h264_bitmask(int bits)
{
    int i;
    unsigned long mask = 0;
    for(i = 0; i < bits; i++) {
        if(i != 0) mask <<= 1;
        mask |= (unsigned long)0x00000001;        
    }
    return mask;
}

inline void h264_MyShift(int nCount, int *nIndex, int *nShiftCount, unsigned char *nShiftBuffer, unsigned long *nRecv, unsigned char *pInBuf)
{
    unsigned long ret_value = 0;
    int bitCnt;

    bitCnt = nCount;
	while (bitCnt != 0)
	{
		if(bitCnt > *nShiftCount)
		{
			ret_value = ret_value<<*nShiftCount;
			ret_value |= *nShiftBuffer>>(8-*nShiftCount);
			*nShiftBuffer = pInBuf[++*nIndex];
			bitCnt -= *nShiftCount;
			*nShiftCount = 8;
		}
		else
		{
			ret_value = ret_value<<bitCnt;
			ret_value |= *nShiftBuffer>>(8-bitCnt) ;

			*nShiftCount -= bitCnt;
			*nShiftBuffer <<= bitCnt;

			bitCnt = 0;
		}         
	}
    *nRecv = ret_value & h264_bitmask(nCount);
}
#endif //End else


int h264_is_iframe(const unsigned char *pInBuf, int len)
{
   if ((len>4 && (pInBuf[4]&NAL_TYPE_MASK) == NAL_TYPE_SEQ_PARA_SET))
   {
        return QS_TRUE;
   }else
        return QS_FAIL;
}
