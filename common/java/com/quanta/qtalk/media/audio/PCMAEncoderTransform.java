package com.quanta.qtalk.media.audio;

public class PCMAEncoderTransform extends G711EncoderTransform
{
    public  final static String CODEC_NAME = "PCMA";
    public  final static int    DEFAULT_ID = myAudioFormat.PCMA;
    protected PCMAEncoderTransform()
    {
        super(DEFAULT_ID);
    }
}
