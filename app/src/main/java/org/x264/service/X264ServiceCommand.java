package org.x264.service;

public class X264ServiceCommand {
    public static final String SERVICE_ACTION_NAME = "org.x264.service.encoder";
    
    public static final String MEMORY_FILE_NAME = "org.x264.service.encoder";
    /**
     * Command to the service to register a client, receiving callbacks
     * from the service.  The Message's replyTo field must be a Messenger of
     * the client where callbacks should be sent.
     */
    public static final int MSG_REGISTER_CLIENT = 1;
    /**
     * Command to the service to unregister a client, ot stop receiving callbacks
     * from the service.  The Message's replyTo field must be a Messenger of
     * the client as previously given with MSG_REGISTER_CLIENT.
     */
    public static final int MSG_UNREGISTER_CLIENT = 2;
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
     * Command to service to update the gop value of x264 encoder.
     * IN:KEY_GOP
     * OUT:NONE  
     */
    public static final int MSG_SET_GOP     = 5;
    /**
     * Command to service to update the bitrate value of x264 encoder.
     * IN:KEY_BITRATE
     * OUT:NONE   
     */
    public static final int MSG_SET_BITRATE = 6;
    /**
     * Command to service to update the bitrate value of x264 encoder.
     * IN:    KEY_FRAME_DATA,    KEY_FRAME_LENGTH,     KEY_SAMPLE_TIME
     * OUT:    KEY_BITSTREAM,     KEY_BITSTREAM_LENGTH, KEY_SAMPLE_TIME   
     */
    public static final int MSG_ENCODE         = 7;    
    
    /**
     * Command to service to update the frame rate value of x264 encoder.
     * KEY_FRAMERATE
     * OUT:NONE  
     */
    public static final int MSG_SET_FRAMERATE     = 8;
    /**
     * Command to service to set request IDR of x264 encoder.
     * IN:NONE
     * OUT:NONE  
     */
    public static final int MSG_SET_REQUESTIDR     = 9;

    public static final String KEY_LOCAL_SOCKET_NAME = "LOCAL_SOCKET_NAME";   //int     : pixel format of video data
    public static final String KEY_FRAME_FORMAT     = "FRAME_FORMAT";       //int         : pixel format of video data
    public static final String KEY_FRAME_WIDTH      = "FRAME_WIDTH";        //int         : width of video data
    public static final String KEY_FRAME_HEIGHT     = "FRAME_HEIGHT";       //int         : height of video data
    public static final String KEY_FRAME_DATA       = "FRAME_DATA";         //byte[]      : Video Data from camera
    public static final String KEY_FRAME_LENGTH     = "FRAME_LENGTH";       //int         : length of frame data
    public static final String KEY_BITSTREAM        = "BITSTREAM";          //byte[]      : Encoded Data
    public static final String KEY_BITSTREAM_LENGTH = "BITSTREAM_LENGTH";   //int         : length of encoded data
    public static final String KEY_SAMPLE_TIME      = "SAMPLE_TIME";        //long        : capture time ms 
    public static final String KEY_ROTATION         = "ROTATION";        //long        : capture time ms 
    public static final String KEY_GOP              = "GOP";                //int         : gop value
    public static final String KEY_BITRATE          = "BITRATE";            //int         : bitrate value
    public static final String KEY_FRAMERATE        = "FRAMERATE";          //int         : framerate value
    
    public static final int RESULT_NA               = 0;
    public static final int RESULT_SUCCESS          = 1;
    public static final int RESULT_FAILED           = -1;
    public static final int RESULT_INVALID_INPUT    = -2;
    
    public static final int RESULT_EXCEPTION        = -255;
    
}
