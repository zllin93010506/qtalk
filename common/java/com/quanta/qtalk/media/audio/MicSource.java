package com.quanta.qtalk.media.audio;

//import java.util.Random;
//import java.util.Vector;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.annotation.SuppressLint;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.AutomaticGainControl;
import android.os.ConditionVariable;

//import com.quanta.qtalk.media.audio.StatisticAnalyzer;
//import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public class MicSource implements IAudioSource, IAudioMute, IAudioDTMF {
    private	static final String 	DEBUGTAG      = "QSE_MicSource";
    private	IAudioSink				mAudioSink    = null;
    private	int						mSampleRate   = 0;
    private	int						mFrameSize    = 0;
    private	AudioRecord				mAudioRecord  = null; 
    private	int						mAudioBufSize = 0;
    private	boolean					mKeepRunning  = false;
    private	boolean					mStarted      = false;
    private	ConditionVariable		mThreadDone   = null;
    private	final Object			mSyncObject   = new Object();
    private	boolean					mIsSilence    = false;
    
	// AEC & AGC
    private AcousticEchoCanceler    mAEC = null;
    private	AutomaticGainControl	mAGC = null;
    private static AudioManager     mAudioManager = null;
    
    @SuppressLint("NewApi")
	@Override
    public void start() throws FailedOperateException, UnSupportException 
    {   	
    	Log.d(DEBUGTAG,"==>start");
        stop();
        
        synchronized(mSyncObject)
        {        	
            try
            {            	
                // get the minimum buffer size requirement
                mAudioBufSize =  AudioRecord.getMinBufferSize(mSampleRate,
                										      AudioFormat.CHANNEL_IN_MONO,
                										      AudioFormat.ENCODING_PCM_16BIT);
                //mAudioBufSize *= 3/2;
                if(Hack.isUI2G()){   //for QUANTA UI2G
                	mAudioBufSize = 8192;
                }

                mAudioRecord = new AudioRecord(Hack.getAudioSourceType(),
                							   mSampleRate,
                                               AudioFormat.CHANNEL_IN_MONO,
                                               AudioFormat.ENCODING_PCM_16BIT,
                                               mAudioBufSize);
                if (mAudioRecord.getState()!= AudioRecord.STATE_INITIALIZED)
                    throw new FailedOperateException("Failed on initilizing the audio recorder");
                
                if (Hack.isEnableSWAEC() == false) {
                    Hack.setRecodeSessionID(mAudioRecord.getAudioSessionId());
        			AGC_Init(mAudioRecord.getAudioSessionId());
        			AEC_Init(mAudioRecord.getAudioSessionId());
                }
                
                mAudioRecord.startRecording();
                mKeepRunning = true;
                mIsSilence = false;
                
                // Start a audio recording thread to recording audio
                new Thread(new Runnable() 
                {
                    public void run() 
                    {                    	
                        final short[] output_array = new short [mFrameSize] ;
                        final short[] silence_array = new short [mFrameSize] ;
                        int			output_length = 0;                        
                        long		curr_timestamp = 0;
                        long		fake_timestamp = 0;
                        boolean		first_frame = true;
                        
                        mThreadDone = new ConditionVariable();
                        
                        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
                        for(int i = 0; i < mFrameSize; i++)
                            silence_array[i] = 0;                       
                        
                        while(mKeepRunning && mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) 
                        {
                            try
                            {	
                                output_length = mAudioRecord.read(output_array, 0, mFrameSize);                           
                                if (output_length > 0)
                                {   
                                	curr_timestamp = System.currentTimeMillis();
                                    if(first_frame) {
                                    	first_frame=false;
                                    	fake_timestamp = curr_timestamp;
                                    }
                                    else{
                                    	fake_timestamp += 20;
                                    }
                                    
                                    if(mIsSilence == true) {
                                        mAudioSink.onMedia(silence_array, output_length, curr_timestamp);                                    
                                    }                                    
                                    else {
                                    	if(Hack.isUI2F()){
                                    		mAudioSink.onMedia(output_array, output_length, fake_timestamp);	
                                    	}else{
                                    		mAudioSink.onMedia(output_array, output_length, curr_timestamp);
                                    	}
                                    }                                    
                                }
                                else if(output_length == 0) {
                                    Thread.sleep(1);
                                    Log.e(DEBUGTAG,"Error on read audio data length:"+output_length+"/"+mFrameSize);
                                }
                                else if (output_length < 0) {
                                	Log.e(DEBUGTAG,"Error on read audio data length:"+output_length+"/"+mFrameSize);
                                    break;
                                }
                                else ;
                            } 
                            catch(java.lang.Throwable e)
                            {
                                Log.e(DEBUGTAG,"run", e);
                            }
                        }

                        if (mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING)
                            mAudioRecord.stop();
                        
                        mAudioRecord.release();
                        mAudioRecord = null;
                        mThreadDone.open();
                        
                        if(mAEC != null)
                        	mAEC.release();
                        if(mAGC != null)
                        	mAGC.release();
                        
                        Hack.setRecodeSessionID(-1);
                    }
                }).start();
                mStarted = true;
            }
            catch(Throwable err) {
                if (mAudioRecord!=null) {
                    mAudioRecord.release();
                    mAudioRecord = null;
                }
                throw new FailedOperateException(err);
            }
        }
    }

    @Override
    public void stop() 
    {
        synchronized(mSyncObject)
        {
            mKeepRunning = false;
            if (mThreadDone!=null) {
                mThreadDone.block();
                mThreadDone = null;
            }
            mStarted = false;
        }
    }

    @Override
    public void setSink(IAudioSink sink) throws FailedOperateException, UnSupportException 
    {
        mFrameSize = mSampleRate/50;
        
        if (sink!=null)
            sink.setFormat(myAudioFormat.RAW16, mSampleRate, 1);
        
        mAudioSink = sink;
    }

    @Override
    public void setFormat(int format, int sampleRate, int numChannels)throws FailedOperateException,    UnSupportException
    {        
        boolean restart = false;
        
        mSampleRate = sampleRate;
        
        if (mStarted) {
            stop();
            restart = true;
        }
        
        setSink(mAudioSink);
        
        if (restart) {
            start();
        }
    }
    
    public void setMute(boolean enable)
    {
        mIsSilence = enable;
    }
    
    //=======Start implement IAudioDTMF=========
//    private IAudioDTMFRender mDTMFRender=null;
//    private IAudioDTMFSender mDTMFSender=null;
    public void sendDTMF(final char press)
    {
//        mDTMFVector.addElement(press);
    }
    @Override
    public void setDTMFRender(IAudioDTMFRender render)
    {
//        mDTMFRender = render;
    }

    @Override
    public void setDTMFSender(IAudioDTMFSender sender)
    {
//        mDTMFSender = sender;
    }
    //=======End implement IAudioDTMF=========
    
    // =====================================================================================================
 	// Audio Effect
 	// =====================================================================================================
 	private void AEC_Init(int sessionID) {
 		Log.d(DEBUGTAG, "AEC_Init(), session:"+sessionID);
 		
 		//if(AcousticEchoCanceler.isAvailable() != true) {
 		//	Log.d(DEBUGTAG, "Android AEC is not available");
 		//	return;
 		//}
 		
 		mAEC = AcousticEchoCanceler.create(sessionID);
 		if(mAEC != null) {
 			mAEC.setEnabled(true);
 			if(mAEC.getEnabled()) 
 				Log.d(DEBUGTAG, "Android AEC is active");
 		}
 		else Log.d(DEBUGTAG, "Can't create Android AEC");
 	}
 	
 	private void AGC_Init(int sessionID) {
 		Log.d(DEBUGTAG, "AGC_Init(), session:"+sessionID);
 		
 		//if(AutomaticGainControl.isAvailable() != true) {
 		//	Log.d(DEBUGTAG, "Android AGC is not available");
 		//	return;
 		//}
 		
 		mAGC = AutomaticGainControl.create(sessionID);
 		if(mAGC != null) {
 			mAGC.setEnabled(true);
 			if(mAGC.getEnabled()) 
 				Log.d(DEBUGTAG, "Android AGC is active");
 		}
 		else Log.d(DEBUGTAG, "Can't create Android AGC");
 	}

}
