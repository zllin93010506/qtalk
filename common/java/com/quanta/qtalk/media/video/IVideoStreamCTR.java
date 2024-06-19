package com.quanta.qtalk.media.video;

public interface IVideoStreamCTR {
	void setRenderReady();
	void onSendIDR();
	void updateResolutionToUI(int width, int height);
}
