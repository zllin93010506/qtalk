package com.quanta.qtalk.media.video;

public interface IVideoStreamSourceCB 
{
	void setGop(int gop);
	void setBitRate(int kbsBitRate);
	void setFrameRate(byte fpsFramerate);
	void requestIDR(); //receive IDR request from remote end
}
