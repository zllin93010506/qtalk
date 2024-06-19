package com.quanta.qtalk.media.video;

public interface IVideoStreamReportCB 
{
	void onReport(int kbpsRecv,byte fpsRecv,int kbpsSend,byte fpsSend,double pkloss,int usRtt);
}
