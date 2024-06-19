package org.ffmpeg.service;

public class FFMpegServiceCommand {
    public static final String SERVICE_ACTION_NAME = "org.ffmpeg.service.h264_decoder";
    /**
     * Command to service to initial X264 encoder.
     * IN:KEY_FRAME_WIDTH,KEY_FRAME_HEIGHT
     * OUT:NONE   
     */
    public static final int MSG_START         = 3;
    /**
     * Command to service to destroy X264 encoder.
     * IN:NONE
     * OUT:NONE   
     */
    public static final int MSG_STOP         = 4;
    /**
     * Command to service to update the bitrate value of x264 encoder.
     * IN:    KEY_FRAME_DATA,    KEY_FRAME_LENGTH,     KEY_SAMPLE_TIME
     * OUT:    KEY_BITSTREAM,     KEY_BITSTREAM_LENGTH, KEY_SAMPLE_TIME   
     */
    public static final int MSG_DECODE         = 7;    

    public static final String KEY_LOCAL_SOCKET_NAME = "LOCAL_SOCKET_NAME";   //int     : pixel format of video data
    public static final String KEY_FRAME_FORMAT     = "FRAME_FORMAT";       //int         : pixel format of video data
    public static final String KEY_FRAME_WIDTH      = "FRAME_WIDTH";        //int         : width of video data
    public static final String KEY_FRAME_HEIGHT     = "FRAME_HEIGHT";       //int         : height of video data
    public static final String KEY_DATA             = "DATA";         //byte[]      : Video Data from camera
    public static final String KEY_DATA_LENGTH      = "DATA_LENGTH";       //int         : length of frame data
    public static final String KEY_SAMPLE_TIME      = "SAMPLE_TIME";        //long        : capture time ms 
    public static final String KEY_ROTATION         = "ROTATION";        //long        : capture time ms 
    public static final String KEY_GOP              = "GOP";                //int         : gop value
    public static final String KEY_BITRATE          = "BITRATE";            //int         : bitrate value

    
    public static final int RESULT_NA               = 0;
    public static final int RESULT_SUCCESS          = 1;
    public static final int RESULT_FAILED           = -1;
    public static final int RESULT_INVALID_INPUT    = -2;
    
    public static final int RESULT_EXCEPTION        = -255;
    
}
