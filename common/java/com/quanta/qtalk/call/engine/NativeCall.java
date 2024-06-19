package com.quanta.qtalk.call.engine;

//import com.quanta.qtalk.util.Log;

import com.quanta.qtalk.call.CallState;
import com.quanta.qtalk.call.ICall;
import com.quanta.qtalk.call.ICallListener;
import com.quanta.qtalk.call.MDP;

public class NativeCall implements ICall
{
    //=========REGION for NativeCall properties
    protected int mNativeHandle;
    protected String mRemoteDisplayName = null;
    protected String mRemoteID   = null;
    protected String mDomain     = null;

    protected String mRemoteAudioIP   = null;
    protected int    mRemoteAudioPort = 0;
    protected String mRemoteVideoIP   = null;
    protected int    mRemoteVideoPort = 0 ;

    protected String mLocalIP         = null;
    protected int    mLocalAudioPort  = 0;
    protected int    mLocalVideoPort  = 0;
    
    protected MDP    mAudioMDP = null;
    protected MDP    mVideoMDP = null;
    protected int    mProductType = 1; //0: none_qunata, 1: PL330, 2: QT
    protected long   mStartTime = 0;
    protected int    mCallType = 0;
    protected int    mCallState = 0;
    protected String mCallStateMsg = null;
    protected boolean mEnableVideo = false;
    protected ICallListener mListener = null;
    protected int    mWidth = 0;
    protected int    mHeight = 0;
    protected int    mFPS = 0;
    protected int    mKbps = 0;
    protected boolean mEnablePLI = false;
    protected boolean mEnableFIR = false;
    protected boolean mEnableTelEvt = false;
    protected int		mAudioFECCtrl = 0;
    protected int		mAudioFECSendPType = 0;
    protected int		mAudioFECRecvPType = 0;
    protected int		mVideoFECCtrl = 0;
    protected int		mVideoFECSendPType = 0;
    protected int		mVideoFECRecvPType = 0;
    protected boolean menableAudioSRTP = false;
    protected String   mAudioSRTPSendKey = null;
    protected String   mAudioSRTPReceiveKey = null;
    protected boolean menableVideoSRTP = false;
    protected String   mVideoSRTPSendKey = null;
    protected String   mVideoSRTPReceiveKey = null;
    
    protected boolean mEnableCameraMute = false;
    
    //=========ENDREGION for NativeCall properties
    protected NativeCall(int nativeHandle)
    {
        mNativeHandle = nativeHandle;
    }
    //=========REGION for ICall implementation=========================
    @Override
    public String getRemoteID() {
        // return remoteID
        return mRemoteID;
    }

    @Override
    public String getRemoteDomain() {
        // return remoteDomain
        return mDomain;
    }
    @Override
    public String getRemoteAudioIP() {
        // return remote IP
        return mRemoteAudioIP;
    }
    @Override
    public String getRemoteVideoIP() {
        // return remote IP
        return mRemoteVideoIP;
    }
    @Override
    public int getRemoteAudioPort() {
        // return remote audio port
        return mRemoteAudioPort;
    }
    @Override
    public int getRemoteVideoPort() {
        // return mRemoteVideoPort
        return mRemoteVideoPort;
    }
    @Override
    public int getLocalAudioPort() {
        // return mLocalAudioPort
        return mLocalAudioPort;
    }
    @Override
    public int getLocalVideoPort() {
        // return mLocalVideoPort
        return mLocalVideoPort;
    }

    @Override
    public MDP getFinalAudioMDP() {
        // return mAudioMDP
        return mAudioMDP;
    }
    @Override
    public MDP getFinalVideoMDP() {
        // return mVideoMDP
        return mVideoMDP;
    }

    @Override
    public int getProductType() {
        // return mProductType
        return mProductType;
    }

    @Override
    public long getStartTime() 
    {
        return mStartTime;
    }
    @Override
    public int getCallType() 
    {
        return mCallType;
    }
    @Override
    public String getCallID()
    {
        return String.valueOf(mNativeHandle);
    }
    @Override
    public boolean isVideoCall()
    {
        return mEnableVideo;
    }
    @Override
    public void setListener(ICallListener listener)
    {
        mListener = listener;
        if (mListener!=null && mCallState == CallState.HANGUP)
        {
            mListener.onRemoteHangup(mCallStateMsg);
        }
    }
    @Override
    public int getCallState()
    {
        return mCallState;
    }
    @Override
    public int getWidth()
    {
        return mWidth;
    }
    @Override
    public int getHeight()
    {
        return mHeight;
    }
    @Override
    public int getFPS()
    {
        return mFPS;
    }
    @Override
    public int getKbps()
    {
        return mKbps;
    }
    @Override
    public boolean getEnablePLI()
    {
        return mEnablePLI;
    }
    @Override
    public boolean getEnableFIR()
    {
        return mEnableFIR;
    }
    @Override
    public boolean getEnableTelEvt()
    {
        return mEnableTelEvt;
    }
    @Override
    public String getRemoteDisplayName()
    {
        return mRemoteDisplayName;
    }
	@Override
	public boolean getEnableCameraMute() {
		return mEnableCameraMute;
	}
    

//=========ENDREGION for ICall implementation=========================
    
}
