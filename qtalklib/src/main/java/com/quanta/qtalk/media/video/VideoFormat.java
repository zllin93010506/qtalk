package com.quanta.qtalk.media.video;
import 	android.graphics.PixelFormat;
public final class VideoFormat 
{
	public static final int NV21 = PixelFormat.YCbCr_420_SP;//17
	public static final int NV16 = PixelFormat.YCbCr_422_SP;//16
	public static final int YUY2 = 20;//PixelFormat.YCbCr_422_I;
	
	public static final int H264 = 9398;
	public static final int MJPEG = H264+1;//9399
}
