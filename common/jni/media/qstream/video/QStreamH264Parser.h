#ifndef _H264_PARSER_H_
#define _H264_PARSER_H_

#define CDRC_PARSER 1

//#define inline __inline

#define NAL_TYPE_MASK               0x1F
#define NAL_TYPE_UNDEFINE           0
#define NAL_TYPE_NON_IDR            1
#define NAL_TYPE_SLICE_A            2
#define NAL_TYPE_SLICE_B            3
#define NAL_TYPE_SLICE_C            4
#define NAL_TYPE_IDR                5
#define NAL_TYPE_SEI                6
#define NAL_TYPE_SEQ_PARA_SET       7
#define NAL_TYPE_PIC_PARA_SET       8


#define QS_TRUE                             (0)
#define QS_FAIL                             (-1)
/* h264 type define */
#define I_SLICE_TYPE                        2
#define P_SLICE_TYPE                        0
#define B_SLICE_TYPE                        1
#define I_SLICE_PARA_SET_CODE               0x67
#define I_SLICE_CODE                        0x65
#define P_SLICE_CODE                        0x41

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int is_vaild;
    unsigned long profile_idc;
    unsigned long seq_parameter_set_id;
    unsigned long bit_depth_chroma_minus8;
    unsigned long bit_depth_luma_minus8;
    unsigned long qpprime_y_zero_transform_bypass_flag;
    unsigned long seq_scaling_matrix_present_flag;
    unsigned long log2_max_frame_num_minus4;
    unsigned long pic_order_cnt_type;
    unsigned long gaps_in_frame_num_value_allowed_flag;
    unsigned long pic_width_in_mbs_minus1;
    unsigned long pic_height_in_map_units_minus1;
    unsigned long level_idc;
    unsigned long chroma_format_idc;
    unsigned long num_ref_frames;
    unsigned long frame_mbs_only_flag;
} h264_para_set_t;

typedef struct {
    unsigned short frame_num;
    int frame_type;
    int nal_unit_type;
    int pic_order_cnt;
    int slice_type;
} h264_slice_info_t;

/* standard implementation according to H.264 spec */        

int h264_get_resolution(unsigned char *buffer,int buf_size,int *height,int *width);
//unsigned short h264_GetFrameNum(unsigned char *buf, unsigned short len, int *frame_type);
#ifndef CDRC_PARSER
    int h264_get_frame_num(unsigned char *buf,int buf_size,int *frame_type, int *codecType, int *count);//,int *frame_width,int *frame_height
//int h264_get_frame_num(unsigned char *buf, int len, int *frame_type, int log2_max_frame_num_minus4, int *pCount);
#else
    int h264_get_frame_num(unsigned char *buf, int len, int *frame_type, int log2_max_frame_num_minus4, int *pCount);
#endif
/* fine-turn for getting more higher performance */
//inline int h264_GetResolution_pl330only(unsigned char *pInBuf, int *nHeight, int *nWidth);
//inline unsigned short h264_GetFrameNum_pl330only(unsigned char *buf, unsigned short len, int *frame_type);

int h264_get_parameter_set(unsigned char *pInBuf, int len, h264_para_set_t *para_set);
//inline int h264_get_slice_info(unsigned char *buf,
//                               unsigned short len,
//                               h264_para_set_t *para_set,
//                               h264_slice_info_t *slice);
int h264_is_iframe(const unsigned char *pInBuf, int len);

#define IS_ISLICE(T) (I_SLICE_TYPE == T)
#define IS_PSLICE(T) (P_SLICE_TYPE == T)
#define IS_BSLICE(T) (B_SLICE_TYPE == T)
#ifdef __cplusplus
}
#endif

#endif // _H264_PARSER_H_
