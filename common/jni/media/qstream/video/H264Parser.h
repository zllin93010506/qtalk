/* NAL unit types */
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

//#define ALT_BITSTREAM_READER  for get bit 1
/* h264 type define */
#define I_SLICE_TYPE                        2
#define B_SLICE_TYPE                        1
#define P_SLICE_TYPE                        0
/*#define I_SLICE_PARA_SET_CODE               0x67
#define I_SLICE_CODE                        0x65
#define P_SLICE_CODE                        0x41*/
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

static inline bool H264_GetResolution(const uint8_t *buffer,int buf_size,int *height,int *width)   //decode_seq_parameter_set
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
		return false;
	}
	if(nal_unit_type == NAL_SPS)
	{
		_H264_GetResolution(ptr,width,height);
		return true;
	}
    return false;
}

static inline int H264_GetFrameNumber(const uint8_t *buf,int buf_size,int *frame_type, bool *isX264, int *count)//,int *frame_width,int *frame_height
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

            if(*frame_type ==I_SLICE_TYPE && a10 != 0) //When streaming is X264 ,(a10 == 37 || a10 == 17)
            {                                          //when streaming is PL330, a4 always equal 0
                *isX264 = true;
            }
            if(*isX264)
                a10 = a4;       

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
#define h264_get_resolution H264_GetResolution
#define h264_get_frame_number H264_GetFrameNumber

#define IS_ISLICE(T) (I_SLICE_TYPE == T)
#define IS_PSLICE(T) (P_SLICE_TYPE == T)
#define IS_BSLICE(T) (B_SLICE_TYPE == T)
