package com.quanta.qtalk.media.audio;

public interface IAudioStreamReportCB {
    void onReport(final int kbpsRecv,final int kbpsSend,
                  final int fpsRecv,final int fpsSend,
                  final int secGapRecv,final int secGapSend);
}
