package com.quanta.qtalk.media.audio;


import java.io.IOException;
import java.nio.ByteBuffer;

import com.quanta.qtalk.UnSupportException;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.media.audio.StatisticAnalyzer;
//import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class AudioRender implements IAudioSink,IAudioRender, IAudioDTMFRender {
    private static final String DEBUGTAG = "QSE_AudioRender";
    private AudioTrack   mAudioTrack	= null ;
    private int          mInputSize		= 0;
    private boolean      mIsStarted		= false;
    private boolean      mIsFirstMedia  = true;
    private boolean      mStopping      = false;    
    private int          mSamplerate    = 0;
    
    @Override
    public void setVolume(final float leftVolumn,final float rightVolumn)
    {
//    	if (mAudioTrack != null)
//    	{
//    		mAudioTrack.setStereoVolume(leftVolumn, rightVolumn) ;
//    	}
    }
    
    @Override
    public void onMedia(ByteBuffer data, int length, long timestamp)
    {
        if (mStopping)
            return;

        if (mIsFirstMedia)
        {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
            mIsFirstMedia = false;
        }

        try
        {
            if (mAudioTrack!=null)
            {
                if (data.isDirect())
                {
                    byte[] array = new byte[length];
                    data.rewind();
                    data.get(array);
                    mAudioTrack.write(array, 0, length) ;
                }
                else {
                    mAudioTrack.write(data.array(), 0, length) ;
                }
                if(StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onRenderSuccess();
            }
        } 
        catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        }
    }

    @Override
    public void onMedia(final short[] data,final int length,final long timestamp) 
    {
        onMedia(data,length,timestamp,true);
    }


    int[] speaker_frame_interval_cnt = {0, 0, 0, 0};
    long previous_msec = 0;
    int frame_cnt = 0;

    void get_a_render_frame() {
        long current_msec = System.currentTimeMillis();
        long diff_msec = current_msec - previous_msec;        

        if(diff_msec <= 20) speaker_frame_interval_cnt[0]++;
        else if(diff_msec > 20 && diff_msec <= 40) speaker_frame_interval_cnt[1]++;
        else if(diff_msec > 40 && diff_msec <= 80) speaker_frame_interval_cnt[2]++;
        else speaker_frame_interval_cnt[3]++;

        if((frame_cnt++ % 150) == 0)  {
            String statics = String.format("%d fps (%d, %d, %d, %d)\n", (long)(150*1000) / (long)((current_msec-previous_msec)),
                    speaker_frame_interval_cnt[0],
                    speaker_frame_interval_cnt[1],
                    speaker_frame_interval_cnt[2],
                    speaker_frame_interval_cnt[3]);
            Log.d(DEBUGTAG, statics);
            speaker_frame_interval_cnt[0] = 0;
            speaker_frame_interval_cnt[1] = 0;
            speaker_frame_interval_cnt[2] = 0;
            speaker_frame_interval_cnt[3] = 0;

            previous_msec = current_msec;
            //start_msec =  current_msec;
        }
    }

    private void onMedia(final short[] data,final int length,final long timestamp, boolean checkDTMF)
    {
        if (mStopping)
            return;

        //get_a_render_frame();
        if(mAudioTrack == null) {        	
            if (Hack.isEnableSWAEC() == true) {
                mAudioTrack = new AudioTrack(Hack.getAudioStreamType(),
                                             mSamplerate,
                                             AudioFormat.CHANNEL_OUT_MONO,
                                             AudioFormat.ENCODING_PCM_16BIT,
                                             mInputSize,
                                             AudioTrack.MODE_STREAM);
        	}
            else {
                if(Hack.getRecodeSessionID() < 0)
            		return;
                mAudioTrack = new AudioTrack(Hack.getAudioStreamType(),
                                             mSamplerate,
                                             AudioFormat.CHANNEL_OUT_MONO,
                                             AudioFormat.ENCODING_PCM_16BIT,
                                             mInputSize,
                                             AudioTrack.MODE_STREAM,
                                             Hack.getRecodeSessionID());
            }
        	int state = mAudioTrack.getState();
            Log.d(DEBUGTAG, "AudioTrack state="+state);
            if (state != AudioTrack.STATE_INITIALIZED) {
                return;
            }
        	mAudioTrack.play();
        }
        
        if (mIsFirstMedia){
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
            mIsFirstMedia = false;
        }

        try
        {
            if (mAudioTrack!=null) {   
                int written = 0;
                int res;
                while (written < length)
                {
                    res = mAudioTrack.write(data, written, length - written) ;
                    switch (res) {
	                    case AudioTrack.ERROR_INVALID_OPERATION:
                            if(mAudioTrack.getState() == AudioTrack.STATE_UNINITIALIZED) {
                                Log.e(DEBUGTAG, "AudioTrack is UNINITIALIZED.");
                            }
                            else {
                                if(mAudioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_FLOAT)
                                    Log.e(DEBUGTAG, "AudioTrack format=ENCODING_PCM_FLOAT.");
                                else
                                    Log.e(DEBUGTAG, "AudioTrack ERROR_INVALID_OPERATION.");
                                mAudioTrack.release();
                            }
                            mAudioTrack = null;
	                    	throw new IOException("track.write() return ERROR_INVALID_OPERATION");
	                    case AudioTrack.ERROR_BAD_VALUE:
	                        int sizeInShorts = length - written;
                            if(data == null) {
                                Log.e(DEBUGTAG, "data is null");
                            }
                            else if((written < 0 ) || (sizeInShorts < 0)
                                || (written + sizeInShorts < 0)  // detect integer overflow
                                || (written + sizeInShorts > data.length)) {
                                Log.e(DEBUGTAG, "offset=" + written+", size=" + sizeInShorts + " data length=" + data.length);
                            }
                            else {
                                Log.e(DEBUGTAG, "ERROR_BAD_VALUE");
                            }
                            if(mAudioTrack.getState() != AudioTrack.STATE_UNINITIALIZED) {
                                mAudioTrack.release();
                            }
                            mAudioTrack = null;
                            throw new IOException("track.write() return ERROR_BAD_VALUE");
                        default:
                    }
                    written += res;
                }
            }
        } 
        catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia: Could not play audio", e);
        }
    }
    @Override
    public void setFormat(int format, int sampleRate, int numChannels) throws UnSupportException
    {
        boolean restart = false;
        
    	Log.d(DEBUGTAG,"=> setFormat(), format="+format+", sampleRate="+sampleRate+", numChannels="+numChannels);

        if(mIsStarted) {        
            restart = true;
            stop();
        }
        mInputSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        mSamplerate    = sampleRate;

        if (restart) {
            start();
        }
    }
    
    public void start()
    {
        if(!mIsStarted)
        {
//            mAudioTrack = new AudioTrack(AudioManager.STREAM_VOICE_CALL, //Hack.getAudioStreamType(), //AudioManager.STREAM_VOICE_CALL,
//                                         mSamplerate,
//                                         AudioFormat.CHANNEL_OUT_MONO,
//                                         AudioFormat.ENCODING_PCM_16BIT,
//                                         mInputSize,
//                                         AudioTrack.MODE_STREAM,
//                                         Hack.getRecodeSessionID());
            mStopping = false;
            mIsFirstMedia = true;
//            mAudioTrack.play();
            mIsStarted = true;
        }
    }
    
    public void stop()
    {
        if(mIsStarted)
        {
            mIsFirstMedia = false;
            mIsStarted = false;
            mStopping = true;
            if(mAudioTrack != null)
            {
                mAudioTrack.flush();
                mAudioTrack.release();
                mAudioTrack = null ;
            }
        }
    }
    
    @Override
    public void onDTMFSend(short[] data, int length, long timestamp)
    {
//        ++mLocalDTMFCount;
//        onMedia(data, length, timestamp,false);
    }
}
