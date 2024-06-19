package com.quanta.qtalk.ui;

import java.io.IOException;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Vibrator;
import com.quanta.qtalk.R;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.Hack;

public class RingingUtil {
    private static final String DEBUGTAG = "QSE_RingingUtil";
    private static final Handler mHandler = new Handler(Looper.getMainLooper());

    private static    Ringtone mRingtone = null;
    private static    Vibrator mVibrator = null;    
    
    private static    MediaPlayer mMediaPlayer = null;
    private static    AudioManager mAudioManager = null;
    
    public static final int SOUND_INCOMING         = 0;
    public static final int SOUND_OUTGOING         = 1;
    public static final int SOUND_LOGIN            = 2;
    public static final int SOUND_LOGOUT           = 3;
    public static final int SOUND_HANGUP           = 4;
    public static final int SOUND_DIALPAD_CLICK    = 5;
    public static final int SOUND_BUSY_TONE        = 6;
    public static final int SOUND_HELLO_WORLD	   = 7;
    
    private static int last_sound_effect = -1;
    
    private static int original_volume = -1;
    /*
    private static boolean initial_loop_ringtone(final Context context)
    {
        boolean result = false;
        Uri alarmRingtoneUri = RingtoneManager.getActualDefaultRingtoneUri(context, RingtoneManager.TYPE_RINGTONE);

        mMediaPlayer = new MediaPlayer();
        try {
            mMediaPlayer.setDataSource(context, alarmRingtoneUri);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_RING);
            mMediaPlayer.setLooping(true);
            mMediaPlayer.prepare();
            result = true;
        } catch (IllegalArgumentException e) {
            Log.e(DEBUGTAG,"initial_loop_ringtone", e);
        } catch (SecurityException e) {
            Log.e(DEBUGTAG,"initial_loop_ringtone", e);
        } catch (IllegalStateException e) {
            Log.e(DEBUGTAG,"initial_loop_ringtone", e);
        } catch (IOException e) {
            Log.e(DEBUGTAG,"initial_loop_ringtone", e);
        }
        return result;
    }
    */
    public static void init(final Context context)
    {    	
    	//force JAVA load the classes in 
    	//otherwise, JNI callback may fail
    }

	public static void playSoundIncomingCall(final Context context)
    {
        playSound(context,SOUND_INCOMING);
    }
    public static void playSoundOutgoingingCall(final Context context)
    {
        playSound(context,SOUND_OUTGOING);
    }
    public static void playSoundLogin(final Context context)
    {
        playSound(context,SOUND_LOGIN);
    }
    public static void playSoundLogout(final Context context)
    {
        playSound(context,SOUND_LOGOUT);
    }
    public static void playSoundHangup(final Context context)
    {
        playSound(context,SOUND_HANGUP);
    }
    public static void playSoundDialpadClick(final Context context)
    {
        playSound(context,SOUND_DIALPAD_CLICK);
    }
    public static void playSoundBusyTone(final Context context)
    {
        playSound(context,SOUND_BUSY_TONE);
    }
    public static void playSoundHelloWorld(final Context context)
    {
        playSound(context,SOUND_HELLO_WORLD);
    }

    public static void setMaxVolume(final Context context) {
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        original_volume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        int max_volume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        if (Hack.isPhone())
            max_volume = max_volume * 4 / 5;
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, max_volume, 0);
    }

    public static void recoverVolume() {
        if (mAudioManager != null) {
            if (original_volume != -1) {
                mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, original_volume, 0);
                original_volume = -1;
            }
            mAudioManager = null;
        }
    }
    private static void playSound(final Context context,final int soundEffect)
    {    
        Log.d(DEBUGTAG,"playSound(), soundEffect:"+soundEffect+", last:"+last_sound_effect);
        //for nurse call
    	if( soundEffect==SOUND_INCOMING || soundEffect==SOUND_OUTGOING || last_sound_effect != soundEffect || soundEffect == SOUND_DIALPAD_CLICK)
        {
    		if( mMediaPlayer!= null && mMediaPlayer.isPlaying()) {
        		mMediaPlayer.stop();
        		mMediaPlayer.release();
        	}
        	switch(soundEffect)
        	{
	        	case SOUND_INCOMING:
                    if (Hack.isPhone())
                        setMaxVolume(context);
	        		mMediaPlayer = MediaPlayer.create(context, R.raw.call_incoming);
	            	mMediaPlayer.setLooping(true);                            	
	                break;
	            case SOUND_OUTGOING:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.call_outgoing);
	            	mMediaPlayer.setLooping(true);
	                break;
	            case SOUND_LOGIN:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.login);
	            	mMediaPlayer.setLooping(false);
	                break;
	            case SOUND_LOGOUT:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.logout);
	            	mMediaPlayer.setLooping(false);
	                break;
	            case SOUND_HANGUP:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.call_hangup);
	            	mMediaPlayer.setLooping(false);
	                break;
	            case SOUND_DIALPAD_CLICK:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.dialpad_click);
	            	mMediaPlayer.setLooping(false);
	                break;
	            case SOUND_BUSY_TONE:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.busy_tone);
	            	mMediaPlayer.setLooping(false);
	                break;
	            case SOUND_HELLO_WORLD:
	            	mMediaPlayer = MediaPlayer.create(context, R.raw.hello_world);
	            	mMediaPlayer.setLooping(false);
	                break;
        	}
        	if(mMediaPlayer != null)
        		mMediaPlayer.start();
    		
//    		mHandler.post(new Runnable()
//    		{
//    			public void run()
//    			{
//		        	Log.d(DEBUGTAG,"playSound(), run: "+soundEffect);
//		        	if( mMediaPlayer!= null && mMediaPlayer.isPlaying()) {
//		        		mMediaPlayer.stop();
//		        		mMediaPlayer.release();
//		        	}
//		        	switch(soundEffect)
//		        	{
//			        	case SOUND_INCOMING:
//			        		mMediaPlayer = MediaPlayer.create(context, R.raw.call_incoming);
//			            	mMediaPlayer.setLooping(true);                            	
//			                break;
//			            case SOUND_OUTGOING:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.call_outgoing);
//			            	mMediaPlayer.setLooping(true);
//			                break;
//			            case SOUND_LOGIN:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.login);
//			            	mMediaPlayer.setLooping(false);
//			                break;
//			            case SOUND_LOGOUT:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.logout);
//			            	mMediaPlayer.setLooping(false);
//			                break;
//			            case SOUND_HANGUP:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.call_hangup);
//			            	mMediaPlayer.setLooping(false);
//			                break;
//			            case SOUND_DIALPAD_CLICK:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.dialpad_click);
//			            	mMediaPlayer.setLooping(false);
//			                break;
//			            case SOUND_BUSY_TONE:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.busy_tone);
//			            	mMediaPlayer.setLooping(false);
//			                break;
//			            case SOUND_HELLO_WORLD:
//			            	mMediaPlayer = MediaPlayer.create(context, R.raw.hello_world);
//			            	mMediaPlayer.setLooping(false);
//			                break;
//		        	}
//		        	if(mMediaPlayer != null)
//		        		mMediaPlayer.start();	        	
//    			}
//    		});
        	Log.d(DEBUGTAG,"playSound(), exit");
        }
        last_sound_effect = soundEffect;
    }
    
//    public static void ringing(final Context context,final boolean incoming)
//    {    	
//    	mHandler.post(new Runnable()
//    	{
//    		public void run()
//    		{
//    			Log.d(DEBUGTAG,"ringing()");
//    			try
//    			{    
//    				if (mRingtone!=null || mVibrator!=null || mMediaPlayer != null)
//                    {
//    					stop();
//                    }
////                    mAudioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
//                    mVibrator = (Vibrator)context.getSystemService(Context.VIBRATOR_SERVICE);        
//                    mRingtone =    RingtoneManager.getRingtone(context, RingtoneManager.getActualDefaultRingtoneUri(context, RingtoneManager.TYPE_RINGTONE));
//                    if (mVibrator!=null)
//                    {
//                        mVibrator.cancel();
//                    }
//                    if(initial_loop_ringtone(context))
//                    {
//                        if(incoming == true && mMediaPlayer != null)
//                        {
//                            mMediaPlayer.start();
//                        }
//                    }
//    			}
//    			catch (Throwable e)
//    			{
//                     Log.e(DEBUGTAG,"stop : ringtone:"+mRingtone+" ,vibrator:"+mVibrator, e);
//    			}
//    			Log.d(DEBUGTAG,"ringing(), exit");
//             }
//         });
//    }
    
    public static void stop()
    {
    	Log.d(DEBUGTAG,"stop()");
    	if (mRingtone != null)
    		mRingtone.stop();
		
		if (mVibrator != null)
			mVibrator.cancel();
		
		if (mMediaPlayer != null) {
		    try {
		        if(mMediaPlayer.isPlaying())
		            mMediaPlayer.stop();
		    }
		    catch(java.lang.IllegalStateException e) {
		        Log.e(DEBUGTAG,"Media player stop error", e);
		    }
			mMediaPlayer.reset();
			mMediaPlayer.release();
		}
		recoverVolume();
		mRingtone = null;
        mVibrator = null;
        mMediaPlayer = null;
        Log.d(DEBUGTAG,"stop(), exit");    	 	
    }
}
