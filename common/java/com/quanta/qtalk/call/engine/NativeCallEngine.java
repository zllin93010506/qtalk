package com.quanta.qtalk.call.engine;

import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Collection;
import java.util.Hashtable;
import java.util.Vector;

import android.provider.CallLog;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.call.CallState;
import com.quanta.qtalk.call.ICall;
import com.quanta.qtalk.call.ICallEngine;
import com.quanta.qtalk.call.ICallEngineListener;
import com.quanta.qtalk.call.MDP;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.CommandService;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.QTService;
import com.quanta.qtalk.util.IPAddress;
import com.quanta.qtalk.util.Log;

public class NativeCallEngine implements ICallEngine
{
//    private static final String TAG = "NativeCallEngine";
    private static final String DEBUGTAG = "QTFa_NativeCallEngine";
    private static final boolean MYDEBUG = true;
    //=========REGION for NativeCallEngine properties
    private int mListenPort = 0;
    private ICallEngineListener mListerner = null;
    private final static Hashtable<Integer,NativeCall> mCallTable = new Hashtable<Integer,NativeCall>();

    private static Hashtable<String, Integer> mCallEngineTable = new Hashtable<String,Integer>();
    private static String mRegisterLocalIP = null;
    private static boolean mEnableCameraMute = false;
    private static int mNativeHandle = 0;     // this memeber is obtained from onLogonState
    private static boolean mCallEngineCreated = false;
    //=========ENDREGION for NativeCallEngine properties
    public NativeCallEngine(int listenPort)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>NativeCallEngine:"+listenPort);
        mNativeHandle = 0;
        mCallEngineCreated = false;
        mListenPort = listenPort;
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "<==NativeCallEngine:"+listenPort);
    }
    protected void finalized()
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>finalized");
        logout();
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "<==finalized");
    }
    //=========REGION for ICallEngine implementation=========================
	//TODO update interface to assign realm
    @Override
    public synchronized void login(final String server,
                      final String domain,
                      final String realm,
                      final int port,
                      final String id,
                      final String name,
                      final String password,
                      final int expireTime,
                      final ICallEngineListener listerner,
                      final String displayName,
                      final String registrarServer,
                      final boolean enablePrack,
                      final int network_type)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>login mNativeHandle="+mNativeHandle);
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"server:"+server+
								                  ", domain:"+domain+
								                  ", realm:"+realm+
								                  ", port:"+port+
								                  ", id:"+id+
								                  ", name:"+name+
								                  ", password:"+password+
								                  ", expireTime:"+expireTime+
								                  ", displayName:"+displayName+
								                  ", registrarServer:"+registrarServer+
								                  ", enablePrack:"+enablePrack+
								                  ", network_type"+network_type);
        if((mNativeHandle != 0) || mCallEngineCreated)
        {
            if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"call logout - 1 mNativeHandle="+mNativeHandle+", mCallEngineCreated="+mCallEngineCreated);
            logout();
        }
        if(mNativeHandle == 0)
        {
            new Thread( new Runnable(){
                @Override
                public void run()
                {
                    synchronized(this)
                    {
                        if((mNativeHandle != 0) || mCallEngineCreated)
                        {
                            if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"call logout - 2 mNativeHandle="+mNativeHandle+", mCallEngineCreated="+mCallEngineCreated);
                            logout();
                        }
                        if(mNativeHandle == 0)
                        {
                            String local_server = server;
                            try
                            {
                                InetAddress address = java.net.InetAddress.getByName(server);
                                local_server = address.getHostAddress();
                                // record a Hashtable<IP, NativeHandle>
                                String[] local_ips = IPAddress.getLocalIPs();
                                if (local_ips!=null)
                                {
                                    for(int i=0; i < local_ips.length; i++)
                                    {
                                        int native_handle = 0;
                                        native_handle = _createEngine(local_ips[i],mListenPort);

                                        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "local_ip:"+local_ips[i]);
                                        if(native_handle != 0) // 0 --> erroe
                                        {
                                            if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "local_ip:"+local_ips[i]);
                                            mCallEngineTable.put(local_ips[i], native_handle);
                                            mCallEngineCreated = true;
                                            _login(native_handle,local_server,domain,realm,port,id,name,password,expireTime,listerner, displayName, registrarServer, enablePrack, network_type);
                                        }
                                        
                                    }
                                }
                                if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "<==login mNativeHandle="+mNativeHandle);
                            } catch (UnknownHostException e) {
                                Log.e(DEBUGTAG, "login:can not get inetaddress from host name");
                            } catch (SocketException e)
                            {
                                Log.e(DEBUGTAG,"login:can not get local ip");
                            }
                        }
                    }
                }
            }).start();
        }
    }

    @Override
    public synchronized void logout()
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>logout mNativeHandle="+mNativeHandle+", mCallEngineTable.size:"+mCallEngineTable.size());
        mNativeHandle = 0;
        mCallEngineCreated = false;
        mCallTable.clear();
        if(mCallEngineTable.size() > 0)
        {
            Collection<Integer> engines = mCallEngineTable.values();
            Vector<Integer> handle_vector = new Vector<Integer>();
            handle_vector.addAll(engines);
            mCallEngineTable.clear();
            for (int i=0;i<handle_vector.size();i++)
            {
                try
                {
                    int native_handle = handle_vector.elementAt(i);
                    if(native_handle != 0)
                    {
                        _logout(native_handle);
                        _destroyEngine(native_handle);
                    }
                } catch (Exception e)
                {
                    Log.e(DEBUGTAG,"logout", e);
                }
            }
        }
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "<==logout mNativeHandle="+mNativeHandle);
    }
    @Override
    public void setCapability(MDP[] audio, MDP[] video, int width, int height, int fps)
            throws FailedOperateException
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>setCapability:"+mNativeHandle + " width: "+width+" height: "+height+" fps: "+fps);
        if(mNativeHandle != 0)
            _setCapability(mNativeHandle, audio, video, width, height, fps);
        else
        {
            Log.e(DEBUGTAG,"setCapability Error");
            throw new FailedOperateException("setCapability Error");
        }
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "<==setCapability:"+mNativeHandle);
    }

    @Override
    public void makeCall(ICall call, String id, String domain, int audioPort,
            int videoPort, boolean isHold, boolean enableCameraMute) throws FailedOperateException
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"makeCall - call:"+call + ", id:"+id +", domain:"+domain +", audio port:"+audioPort +", video port:"+videoPort+", enableCameraMute:"+enableCameraMute);
        //Convert to private call
        NativeCall local_call = call==null?null:(NativeCall)call;
        mEnableCameraMute = enableCameraMute;
        if(local_call==null)
        	Log.d(DEBUGTAG,"local call null");
        else
        	Log.d(DEBUGTAG,"local call not null");
        //Call native function
        if(mNativeHandle != 0)
        {
            if(isHold)
                _makeCall(mNativeHandle,
                        local_call==null?0:local_call.mNativeHandle,
                        id,
                        domain,
                        "0.0.0.0",
                        audioPort,
                        videoPort);
            else
                _makeCall(mNativeHandle,
                          local_call==null?0:local_call.mNativeHandle,
                          id,
                          domain,
                          mRegisterLocalIP,
                          audioPort,
                          videoPort);
        }
        else{
            if(ProvisionSetupActivity.debugMode)Log.e(DEBUGTAG,"mNativeHandle error");
            QTService.outgoingcallfail();
        }
    }

    @Override
    public void makeSeqCall(ICall call, String display, String id, String domain, int audioPort,
            int videoPort, boolean isHold, boolean enableCameraMute) throws FailedOperateException
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"makeSeqCall - call:"+call + ", id:"+id +", domain:"+domain +", audio port:"+audioPort +", video port:"+videoPort);
        //Convert to private call
        NativeCall local_call = call==null?null:(NativeCall)call;
        mEnableCameraMute = enableCameraMute;
        if(local_call==null)
        	Log.d(DEBUGTAG,"local call null");
        else
        	Log.d(DEBUGTAG,"local call not null");
        //Call native function
        if(mNativeHandle != 0)
        {
            if(isHold)
            	_sequentialMakeCall(mNativeHandle,
                        local_call==null?0:local_call.mNativeHandle,
                        display,
                        id,
                        domain,
                        "0.0.0.0",
                        audioPort,
                        videoPort);
            else
            	_sequentialMakeCall(mNativeHandle,
                          local_call==null?0:local_call.mNativeHandle,
                          display,
                          id,
                          domain,
                          mRegisterLocalIP,
                          audioPort,
                          videoPort);
        }
        else{
            if(ProvisionSetupActivity.debugMode)Log.e(DEBUGTAG,"mNativeHandle error");
            QTService.outgoingcallfail();
        }
    }    
    
    
    @Override
    public void acceptCall(ICall call, int audioPort,
            int videoPort, boolean enableCameraMute) throws FailedOperateException
    {
        if(ProvisionSetupActivity.debugMode)
        	Log.d(DEBUGTAG,"acceptCall - call:"+call +", audio port:"+audioPort +", video port:"+videoPort+" enableCameraMute:"+enableCameraMute);
        //Convert to private call
        NativeCall local_call = call==null?null:(NativeCall)call;
        mEnableCameraMute = enableCameraMute;
        //Call native function
        if(mNativeHandle != 0 && local_call!=null)
        {
            local_call.mCallState = CallState.ACCEPT;
            _acceptCall(mNativeHandle,
                        local_call.mNativeHandle,
                        mRegisterLocalIP,
                        audioPort,
                        videoPort);
        }
    }

    @Override
    public void hangupCall(ICall call,final boolean isReinvite) throws FailedOperateException
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"hangupCall - call:"+call);
        //Convert to private call
        NativeCall local_call = call==null?null:(NativeCall)call;
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"state :"+local_call.mCallState);
        if (!isReinvite)
        {
            //Modify the calltype
            if (local_call.mCallState!=CallState.ACCEPT && local_call.getCallType()== CallLog.Calls.INCOMING_TYPE)
                local_call.mCallType = CallLog.Calls.MISSED_TYPE;
            if (local_call!=null)
            {
                mCallTable.remove(Integer.valueOf(local_call.mNativeHandle));
            }
        }
        //Call native function
        if(mNativeHandle != 0){
        	if(CommandService.lastcall)
        		QtalkSettings.nurseCall = false;
        	if(!(QtalkSettings.nurseCall))
        		_hangupCall(mNativeHandle,local_call==null?0:local_call.mNativeHandle);
        }
    }

    @Override
    public void keepAlive() throws FailedOperateException
    {
        if(mNativeHandle != 0)
            _keepAlive(mNativeHandle);
    }
    
    @Override
    public synchronized void requestIDR(ICall call)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>requestIDR");
        //Convert to private call
        NativeCall local_call = call==null?null:(NativeCall)call;
        if(mNativeHandle != 0)
            _requestIDR(mNativeHandle,local_call==null?0:local_call.mNativeHandle);
    }

    @Override
	public void sr2_switch_network_interface(int interface_type, String ip_addr) {
        if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getLocalIpAddress:"+ip_addr+", interface_type:"+interface_type);
        if(ip_addr.trim() != "")
            networkInterfaceSwitch(mNativeHandle, ip_addr, mListenPort, interface_type);
        else
            if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"skip networkInterfaceSwitch ip_addr:"+ip_addr+", interface_type:"+interface_type);
	}
    
	@Override
	public void qth_switch_call_to_px(ICall call, String PX_SIP_ID ) {
		// TODO Auto-generated method stub
		if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>qth_switch_cal_to_px");
        //Convert to private call
        NativeCall local_call = call==null?null:(NativeCall)call;
        if(mNativeHandle != 0)
            _SwitchCallToPX(mNativeHandle,local_call==null?0:local_call.mNativeHandle, PX_SIP_ID);
	}

    @Override
    public void setProvision(ProvisionInfo provision)
    {
		if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "==>setProvision:"+mNativeHandle);
		
        if(mNativeHandle != 0)
            _setProvision(mNativeHandle, provision);
        else
        {
            Log.e(DEBUGTAG,"setProvision Error");
            //throw new FailedOperateException("setProvision Error");
        }
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG, "<==setProvision:"+mNativeHandle);
    }

    //=========ENDREGION for ICallEngine implementation=========================
    //=========REGION for native callback=========================
    private static void onLogonState(final ICallEngineListener listener,
            final boolean state,
            final String msg,
            final int expireTime,
            final String localIP)
    {
        // if multi-local ip available, this callback may be not triggered correctly, a protection scheme should be implemented. 
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onLogonState:"+localIP+", expireTime:"+expireTime +", msg:"+msg);
        if (mNativeHandle == 0 && state && mCallEngineTable.get(localIP)!=null)
        {
            mRegisterLocalIP = localIP;            
            mNativeHandle = mCallEngineTable.get(mRegisterLocalIP);
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"mNativeHandle:"+mNativeHandle+"   state:"+state);
        }
        /*if (mNativeHandle == 0 ||   // didn't login successfully
            (mNativeHandle!=0 && 0 == mRegisterLocalIP.compareToIgnoreCase(localIP))
            || state)    //has logon successfully, filter message
        {*//********************Vivian**********************/
        if (mNativeHandle!=0 && listener!=null)
        {
            listener.onLogonState(state, msg, expireTime);
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"mNativeHandle"+mNativeHandle+"   listener="+listener);
        }
        //}
    }
    
    private static void onCallOutgoing(final ICallEngineListener listener,
            final int call,
            final String account,
            final String domain,
            final boolean enableVideo,
            final int callState)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallOutgoing- call:"+call +", account:"+account +", enableVideo:"+enableVideo+ "mCallTable.size:"+mCallTable.size());
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (local_call==null)
        {
            Log.d(DEBUGTAG,"call.getCallState() "+callState);

        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"local_call==null");
            local_call = new NativeCall(call);
            if(callState>=0)
            	mCallTable.put(Integer.valueOf(call), local_call);
            local_call.mCallType = CallLog.Calls.OUTGOING_TYPE;
            //local_call.mCallState = CallState.RINGING;
            local_call.mCallState = callState;
            local_call.mRemoteID = account;
            local_call.mDomain = domain;
            local_call.mEnableVideo = enableVideo;
            if (listener!=null)
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"listener!=null");
                listener.onCallOutgoing(local_call);
            }
            else
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"listener==null");
            }
        }else
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"local_call!=null");
            if(callState == CallState.HOLD || callState == CallState.UNHOLD)
            {
                return;
            }
            if (local_call.mListener!=null)
            {
                local_call.mListener.onSelfCallChange(enableVideo);
            }
        }
    }

    private static void onCallMCUOutgoing(final ICallEngineListener listener,
            final int call,
            final String account,
            final String domain,
            final boolean enableVideo,
            final int callState)
    {
    	
    }
    
    private static void onCallIncoming(final ICallEngineListener listener,
            final int call,
            final String account,
            final String domain,
            final String remoteDisplayName,
            final boolean enableVideo,
            final int callState)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallIncoming- call:"+call +", account:"+account + ", remoteDisplayName:"+remoteDisplayName+", enableVideo:"+enableVideo +", callState:"+callState);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (local_call==null)
        {
            local_call = new NativeCall(call);
            mCallTable.put(Integer.valueOf(call), local_call);
            local_call.mRemoteID = account;
            local_call.mRemoteDisplayName = remoteDisplayName;
            local_call.mDomain = domain;
            local_call.mEnableVideo = enableVideo;
            local_call.mCallState = CallState.RINGING;
            local_call.mCallType = CallLog.Calls.INCOMING_TYPE;
            if (listener!=null)
                listener.onCallIncoming(local_call);
        }else
        {
            if (local_call.mListener!=null)
            {
                if(callState == CallState.HOLD)
                {
                    local_call.mListener.onRemoteCallHold(true);
                }
                else if (callState == CallState.UNHOLD)
                {
                    local_call.mListener.onRemoteCallHold(false);
                }
                else
                    local_call.mListener.onRemoteCallChange(enableVideo);
            }
        }
    }
    
    private static void onCallMCUIncoming(final ICallEngineListener listener,
            final int call,
            final String account,
            final String domain,
            final String remoteDisplayName,
            final boolean enableVideo,
            final int callState)
    {
    	if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallMCUIncoming- call:"+call +", account:"+account + ", remoteDisplayName:"+remoteDisplayName+", enableVideo:"+enableVideo +", callState:"+callState);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (Flag.InsessionFlag.getState())
        {
        	local_call = new NativeCall(call);
            mCallTable.put(Integer.valueOf(call), local_call);
            local_call.mRemoteID = account;
            local_call.mDomain = domain;
            local_call.mEnableVideo = enableVideo;
            local_call.mCallState = CallState.RINGING;
            local_call.mCallType = CallLog.Calls.INCOMING_TYPE;
            if (listener!=null)
                listener.onCallMCUIncoming(local_call);
        }
    	Flag.MCU.setState(true);
    	local_call = new NativeCall(call);
        mCallTable.put(Integer.valueOf(call), local_call);
        local_call.mRemoteID = account;
        local_call.mDomain = domain;
        local_call.mEnableVideo = enableVideo;
        local_call.mCallState = CallState.RINGING;
        local_call.mCallType = CallLog.Calls.INCOMING_TYPE;
        if (listener!=null)
            listener.onCallIncoming(local_call);
    }
    
    
    private static void onCallPTTIncoming(final ICallEngineListener listener,
            final int call,
            final String account,
            final String domain,
            final String remoteDisplayName,
            final boolean enableVideo,
            final int callState)
    {
    	if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallPTTIncoming- call:"+call +", account:"+account + ", remoteDisplayName:"+remoteDisplayName+", enableVideo:"+enableVideo +", callState:"+callState);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        Log.d("PTT","PTT incoming");

        if (local_call==null)
        {
	    	Flag.PTT.setState(true);
	    	local_call = new NativeCall(call);
	        mCallTable.put(Integer.valueOf(call), local_call);
	        local_call.mRemoteID = account;
	        local_call.mRemoteDisplayName = remoteDisplayName;
	        local_call.mDomain = domain;
	        local_call.mEnableVideo = enableVideo;
	        local_call.mCallState = CallState.RINGING;
	        local_call.mCallType = CallLog.Calls.INCOMING_TYPE;
	        if (listener!=null){
	            listener.onCallPTTIncoming(local_call);
	        }
        }
    }
    

    private static void onCallAnswered(final ICallEngineListener listener,
            final int call)
    {
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onCallAnswered- call:"+call);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (local_call!=null)
        {
            local_call.mCallState = CallState.ACCEPT;
            if (listener!=null)
                listener.onCallAnswered(local_call);
        }else
        {
            Log.e(DEBUGTAG,"onCallAnswered: failed on get local_call");
        }
    }

    private static void onCallHangup(final ICallEngineListener listener,
            final int call,
            final String msg)
    { 
    	 
	        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallHangup- call:"+call+" msg:"+msg);

	        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
	        mCallTable.remove(Integer.valueOf(call));
	        if (local_call != null)
	        {
	            local_call.mCallStateMsg = msg;
	            if (local_call.mCallState!=CallState.ACCEPT && local_call.getCallType()== CallLog.Calls.INCOMING_TYPE){
	            	local_call.mCallType = CallLog.Calls.MISSED_TYPE;
	            }
	            local_call.mCallState = CallState.HANGUP;
	            if (listener!=null)
	            { 
	            	if(CommandService.lastcall)
	            		QtalkSettings.nurseCall = false;
	            	if(!(QtalkSettings.nurseCall)){
	            		listener.onCallHangup(local_call,msg);
	            	}
	            }
	            if (local_call.mListener!=null)
	            {
	            	if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onRemoteHangup");
	                local_call.mListener.onRemoteHangup(msg);
	            }
	        }else
	        {
	        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"onCallHangup: failed on get local_call");
	        }
    	
    }
    private static void onCallStateChanged(final ICallEngineListener listener,
            final int call,
            final int state,
            final String msg)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallStateChanged- call:"+call+" state :"+state+" msg:"+msg);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (local_call != null)
        {
            //local_call.mCallState = state;
            local_call.mCallStateMsg = msg;
            if (listener!=null)
                listener.onCallStateChange(local_call,state,msg);
        }else
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"onCallStateChanged: failed on get local_call");
        }
    }
    private static void onSessionStart(final ICallEngineListener listener,
            final int call,
            final String remoteAudioIP,
            final int    remoteAudioPort,
            final String remoteVideoIP,
            final int    remoteVideoPort,
            final String localIP,
            final int    localAudioPort,
            final int    localVideoPort,
            final String audioMimeType,
            final int    audioPayloadID,
            final int    audioClockRate,
            final int    audioBitRate,
            final String videoMimeType,
            final int    videoPayloadID,
            final int    videoClockRate,
            final int    videoBitRate,
            final int    productType,
            final boolean enagleVideo,
            final int    width,
            final int    height,
            final int    fps,
            final int    kbps,
            final long   startTime,
            final boolean enablePLI,
            final boolean enableFIR,
            final boolean enableTelEvt,
            final int audioFecCtrl,
            final int audioFecSendPType,
            final int audioFecRecvPType,
            final boolean enableAudioSRTP,
            final String audioSRTPSendKey,
            final String audioSRTPReceiveKey,
            final int videoFecCtrl,
            final int videoFecSendPType,
            final int videoFecRecvPType,
            final boolean enableVideoSRTP,
            final String videoSRTPSendKey,
            final String videoSRTPReceiveKey)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onSessionStart :"+startTime+" call :"+call);
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"width:"+width+", height:"+height+", fps:"+fps+", kbps:"+kbps +", enablePLI:"+enablePLI+", enableFIR:"+enableFIR +", productType:"+productType + "FECCtrl:"+audioFecCtrl+"FECSendPType:"+audioFecSendPType);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        
        //--------------------------------------------[PT]
        QtalkSettings.nurseCall = false;
        //--------------------------------------------
        
        if (local_call != null)
        {
            MDP audioMDP = new MDP(audioMimeType,audioPayloadID,audioClockRate,audioBitRate, audioFecCtrl, audioFecSendPType, audioFecRecvPType, enableAudioSRTP, audioSRTPSendKey, audioSRTPReceiveKey);
            MDP videoMDP = new MDP(videoMimeType,videoPayloadID,videoClockRate,videoBitRate, videoFecCtrl, videoFecSendPType, videoFecRecvPType, enableVideoSRTP, videoSRTPSendKey, videoSRTPReceiveKey);
            local_call.mRemoteAudioIP = remoteAudioIP;
            local_call.mRemoteAudioPort = remoteAudioPort;
            local_call.mRemoteVideoIP = remoteVideoIP;
            local_call.mRemoteVideoPort = remoteVideoPort;
            local_call.mLocalIP = localIP;
            local_call.mLocalAudioPort = localAudioPort;
            local_call.mLocalVideoPort = localVideoPort;
            local_call.mAudioMDP = audioMDP;
            local_call.mVideoMDP = videoMDP;
            local_call.mEnableVideo = enagleVideo;
            local_call.mWidth = width;
            local_call.mHeight = height;
            local_call.mFPS = fps;
            local_call.mKbps = kbps;
            local_call.mEnablePLI = enablePLI;
            local_call.mEnableFIR = enableFIR;
            local_call.mEnableTelEvt = enableTelEvt;
            local_call.mAudioFECCtrl = audioFecCtrl;					//eason - FEC info from SIP server
            local_call.mAudioFECSendPType = audioFecSendPType;
            local_call.mAudioFECRecvPType = audioFecRecvPType;
            local_call.mVideoFECCtrl = videoFecCtrl;
            local_call.mVideoFECSendPType = videoFecSendPType;
            local_call.mVideoFECRecvPType = videoFecRecvPType;
            
            local_call.menableAudioSRTP = enableAudioSRTP;             //Iron - SRTP info from SIP server
            local_call.mAudioSRTPSendKey = audioSRTPSendKey;
            local_call.mAudioSRTPReceiveKey = audioSRTPReceiveKey;
            local_call.menableVideoSRTP = enableVideoSRTP;
            local_call.mVideoSRTPSendKey = videoSRTPSendKey;
            local_call.mVideoSRTPReceiveKey = videoSRTPReceiveKey;
            if(local_call.mStartTime == 0)
                local_call.mStartTime = System.currentTimeMillis();
            local_call.mEnableCameraMute = mEnableCameraMute;
            local_call.mProductType = productType;
            if (listener!=null){
                listener.onSessionStart(local_call.getCallID(), 
                    local_call.getCallType(),
                    local_call.getRemoteID(),
                    local_call.getRemoteAudioIP(),
                    local_call.getLocalAudioPort(),
                    local_call.getRemoteAudioPort(),
                    local_call.getFinalAudioMDP(),
                    local_call.getRemoteVideoIP(),
                    local_call.getLocalVideoPort(),
                    local_call.getRemoteVideoPort(),
                    local_call.getFinalVideoMDP(),
                    local_call.getProductType(),
                    local_call.getStartTime(),
                    local_call.getWidth(),
                    local_call.getHeight(),
                    local_call.getFPS(),
                    local_call.getKbps(),
                    local_call.getEnablePLI(),
                    local_call.getEnableFIR(),
                    local_call.getEnableTelEvt(),
                    local_call.getEnableCameraMute());
                if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"width:"+local_call.getWidth()+", height:"+local_call.getHeight()+", fps:"+local_call.getFPS());

            }
        }else
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"onSessionStart: failed on get local_call");
        }
    }
    private static void onMCUSessionStart(final ICallEngineListener listener,
            final int call,
            final String remoteAudioIP,
            final int    remoteAudioPort,
            final String remoteVideoIP,
            final int    remoteVideoPort,
            final String localIP,
            final int    localAudioPort,
            final int    localVideoPort,
            final String audioMimeType,
            final int    audioPayloadID,
            final int    audioClockRate,
            final int    audioBitRate,
            final String videoMimeType,
            final int    videoPayloadID,
            final int    videoClockRate,
            final int    videoBitRate,
            final int    productType,
            final boolean enagleVideo,
            final int    width,
            final int    height,
            final int    fps,
            final int    kbps,
            final long   startTime,
            final boolean enablePLI,
            final boolean enableFIR,
            final boolean enableTelEvt,
            final int audioFecCtrl,
            final int audioFecSendPType,
            final int audioFecRecvPType,
            final int videoFecCtrl,
            final int videoFecSendPType,
            final int videoFecRecvPType)
    {
         
    }
    private static void onSessionEnd(final ICallEngineListener listener,
            final int call)
    {
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onSessionEnd- call:"+call);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
       // mCallTable.remove(Integer.valueOf(call));
        if (local_call != null)
        {
            if (listener!=null)
                listener.onSessionEnd(local_call);
        }else
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG," onSessionEnd: failed on get local_call");
        }
    }
    
    private static void onMCUSessionEnd(final ICallEngineListener listener,
            final int call)
    {
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onMCUSessionEnd- call:"+call);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
       // mCallTable.remove(Integer.valueOf(call));
        if (local_call != null)
        {
            if (listener!=null)
                listener.onMCUSessionEnd(local_call);
        }else
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"onMCUSessionEnd: failed on get local_call");
        }
    }

    private static void onReinviteFailed(final ICallEngineListener listener,
            final int call)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onReinviteFailed- call:"+call);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (local_call != null)
        {
            if (local_call.mListener!=null)
            {
                local_call.mListener.onReinviteFailed();
            }
        }else
        {
            Log.e(DEBUGTAG,"onSessionEnd: failed on get local_call");
        }
    }
    
    private static void onRequestIDR(final ICallEngineListener listener,
            final int call)
    {
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onRequestIDR- call:"+call);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        if (local_call != null)
        {
            if (local_call.mListener!=null)
            {
                local_call.mListener.onRequestIDR();
            }
        }else
        {
            Log.e(DEBUGTAG,"onRequestIDR: failed on get local_call");
        }
    }
    private static void onCallTransferToPX(final ICallEngineListener listener,
    		final int call,
            final int success)
    {
        
        if(ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"onCallTransferToPX- :"+success+" call:"+call);
        NativeCall local_call = mCallTable.get(Integer.valueOf(call));
        /*
        if( success == 1)
        {
        	mCallTable.remove(Integer.valueOf(call));
        }
        */
        if (local_call != null)
        {
            if (listener!=null)
            {
            	listener.onCallTransferToPX(local_call, success);
            }
              
        }
        
    }
    
    // =========ENDREGION for native callback=========================

    //=========REGION for native functions=========================
    private native static int _createEngine(String localIP, int listenPort);
    private native static void _destroyEngine(int handle);
    
    private native static void _login(int handle,
                               String server,
                               String domain,
                               String realm,
                               int port,
                               String id,
                               String name,
                               String password,
                               int expireTime,
                               ICallEngineListener listerner,
                               String displayName,
                               String registrarServer,
                               boolean enablePrack,
                               int network_type);
    private native static void _logout(int handle);
    private native static void _setCapability(int handle,
                                MDP[] audio,
                                MDP[] video,
                                int width,
                                int height,
                                int fps);
    private native static void _makeCall(int handle,
                                int call,
                                String id,
                                String domain,
                                String localIP,
                                int audioPort,
                                int videoPort);
    private native static void _sequentialMakeCall(int handle,
            					int call,
            					String display,
            					String id,
            					String domain,
            					String localIP,
            					int audioPort,
            					int videoPort);
    private native static void _acceptCall(int handle,
                             int call,
                             String localIP,
                             int audioPort,
                             int videoPort);
    private native static void _hangupCall(int handle,int call);
    private native static void _keepAlive(int handle);
    private native static void _requestIDR(int handle,int call);
    private native static void networkInterfaceSwitch(int handle, String IP,int sip_port, int interface_type);
    private native static void _SwitchCallToPX(int handle, int call, String PX_SIP_ID);
    private native static void _SetQTDND(int handle, int isDND);
    private native static void _setProvision(int handle,ProvisionInfo provision);

    static
    {
    	System.loadLibrary("cm");
    	System.loadLibrary("low");
    	System.loadLibrary("adt");
    	System.loadLibrary("sipccl");
    	System.loadLibrary("sdpccl");
    	System.loadLibrary("sipTx");
    	System.loadLibrary("DlgLayer");
    	System.loadLibrary("callcontrol");
    	System.loadLibrary("SessAPI");
    	System.loadLibrary("MsgSummary");
    	
        System.loadLibrary("call_engine");
        
    }

}
