package com.quanta.qtalk.ui;


import android.media.AudioManager;
import com.quanta.qtalk.call.ICallListener;

public abstract class AbstractOutgoingCallActivity extends AbstractCallTimerActivity implements ICallListener
{
    @Override
    protected void onResume()
    {
        super.onResume();
        boolean success = QTReceiver.engine(this,false).setCallListener(mCallID, this);
        if (success) {
            RingingUtil.playSoundOutgoingingCall(this);
        }
        else {
            this.finish();
        }
    }
    @Override
    protected void onPause()
    {
        super.onPause();
        RingingUtil.stop();
    }
    @Override
    protected void onStart()
    {
        super.onStart();
//        this.setVolumeControlStream(AudioManager.STREAM_RING);        
    }
    @Override
    protected void onStop()
    {
        super.onStop();
//        this.setVolumeControlStream(AudioManager.USE_DEFAULT_STREAM_TYPE);
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();        
    }
}
