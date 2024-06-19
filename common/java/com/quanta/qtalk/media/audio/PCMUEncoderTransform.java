package com.quanta.qtalk.media.audio;

public class PCMUEncoderTransform extends G711EncoderTransform
{
    public  final static String CODEC_NAME = "PCMU";
    public  final static int    DEFAULT_ID = myAudioFormat.PCMU;
    protected PCMUEncoderTransform()
    {
        super(DEFAULT_ID);
    }
}