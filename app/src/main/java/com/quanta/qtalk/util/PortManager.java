package com.quanta.qtalk.util;

import java.io.IOException;
import java.net.DatagramSocket;
//import java.net.ServerSocket;
import java.net.InetSocketAddress;

import com.quanta.qtalk.ui.ProvisionSetupActivity;

public class PortManager {
//    private static final String TAG = "PortManager";
    private static final String DEBUGTAG = "QTFa_PortManager";
    private static final int MIN_SIP_PORT_NUMBER   = 15060;
    private static final int MAX_SIP_PORT_NUMBER   = 15999;
//    private static final int MIN_AUDIO_PORT_NUMBER = 6000;
//    private static final int MAX_AUDIO_PORT_NUMBER = 7999;
//    private static final int MIN_VIDEO_PORT_NUMBER = 8000;
//    private static final int MAX_VIDEO_PORT_NUMBER = 10000;
//    
    private static final int CHECK_AUDIO_PORT   =   0;
    private static final int CHECK_VIDEOO_PORT  =   1;
    private static final int CHECK_SIP_PORT     =   2;
    private static int mVideoPortRangeStart     =   0;
    private static int mVideoPortRangeEnd       =   0;
    private static int mAudioPortRangeStart     =   0;
    private static int mAudioPortRangeEnd       =   0;
    
    private static int lastAudioPort = 0;
    private static int lastVideoPort = 0;
    //modify this function as private method
    public synchronized static int getSIPPort()
    {
        int tmp_port = MIN_SIP_PORT_NUMBER;
        while(tmp_port < MAX_SIP_PORT_NUMBER)
        {
            if(checkUDP(tmp_port, CHECK_SIP_PORT))
                break;
            else
                tmp_port++;
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "getSIPPort:"+tmp_port);
        return tmp_port;
    }
    
    public synchronized static void setAudioPortRange(int minAudioPort, int maxAudioPort)
    {
        //Log.d(DEBUGTAG,"setAudioPortRange:"+minAudioPort+"-"+maxAudioPort);
        mAudioPortRangeStart = minAudioPort;
        mAudioPortRangeEnd = maxAudioPort;
        lastAudioPort = minAudioPort;
    }
    
    public synchronized static void setVideoPortRange(int minVideoPort, int maxVideoPort)
    {
        //Log.d(DEBUGTAG,"setVideoPortRange:"+minVideoPort+"-"+maxVideoPort);
        mVideoPortRangeStart = minVideoPort;
        mVideoPortRangeEnd = maxVideoPort;
        lastVideoPort = minVideoPort;
    }
    
    public synchronized static int getNextVideoPort()
    {
        if(lastVideoPort >= mVideoPortRangeEnd)
        {
            lastVideoPort = mVideoPortRangeStart;
        }
        int tmp_port = lastVideoPort;
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>getNextVideoPort:"+tmp_port);
        while(tmp_port < mVideoPortRangeEnd)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "scan :"+tmp_port);
            if(tmp_port%2==0 &&
               checkUDP(tmp_port,CHECK_VIDEOO_PORT) &&
               checkUDP(tmp_port+1, CHECK_VIDEOO_PORT)&&
               tmp_port != lastVideoPort)
                break;
            else
                ++tmp_port;
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==getNextVideoPort:"+tmp_port);
        lastVideoPort = tmp_port;
        return tmp_port;
    }
    public synchronized static int getNextAudioPort()
    {
        if(lastAudioPort >= mAudioPortRangeEnd)
        {
            lastAudioPort = mAudioPortRangeStart;
        }
        int tmp_port = lastAudioPort;
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>getNextAudioPort:"+tmp_port);
        while(tmp_port < mAudioPortRangeEnd)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "scan :"+tmp_port);
            if(tmp_port%2==0 &&
               checkUDP(tmp_port, CHECK_AUDIO_PORT) &&
               checkUDP(tmp_port+1, CHECK_AUDIO_PORT)&&
               tmp_port != lastAudioPort)
                break;
            else
                ++tmp_port;
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==getNextAudioPort:"+tmp_port);
        lastAudioPort = tmp_port;
        return tmp_port;
    }
    //modify this function as private method
    public synchronized static int getAudioPort()
    {
    	int tmp_port;
    	if(lastAudioPort>=mAudioPortRangeStart && lastAudioPort<=mAudioPortRangeEnd)
    	{
    		tmp_port = lastAudioPort+2;
    	}
    	else
    	{
    		tmp_port = mAudioPortRangeStart;
    	}
        //int tmp_port = mAudioPortRangeStart;
        while(tmp_port < mAudioPortRangeEnd)
        {
            if(tmp_port%2==0 &&
               checkUDP(tmp_port, CHECK_AUDIO_PORT) &&
               checkUDP(tmp_port+1, CHECK_AUDIO_PORT))
                break;
            else
                ++tmp_port;
        }
        //Log.d(DEBUGTAG,"getAudioPort:"+tmp_port);
        lastAudioPort = tmp_port;
        return tmp_port;
    }
    
    public synchronized static int getVideoPort()
    {
    	int tmp_port;
    	if(lastVideoPort>=mVideoPortRangeStart && lastVideoPort<=mVideoPortRangeEnd)
    	{
    		tmp_port = lastVideoPort+2;
    	}
    	else
    	{
    		tmp_port = mVideoPortRangeStart;
    	}
        //int tmp_port = mVideoPortRangeStart;
        while(tmp_port < mVideoPortRangeEnd)
        {
            if(tmp_port%2==0 &&
               checkUDP(tmp_port,CHECK_VIDEOO_PORT) &&
               checkUDP(tmp_port+1, CHECK_VIDEOO_PORT))
                break;
            else
                ++tmp_port;
        }
        //Log.d(DEBUGTAG,"getVideoPort:"+tmp_port);
        lastVideoPort = tmp_port;
        return tmp_port;
    }

    
//    public synchronized static DatagramSocket[] getUDPSocketPair()
//    {
//        DatagramSocket[] result = null;
//        int retry = 10;
//        for (int i=0;i<retry;i++)
//        {
//            int rtp_port = getRTPPort();
//            if (rtp_port>0)
//            {
//                result = new DatagramSocket[2];
//                try {
//                    result[0] = new DatagramSocket(rtp_port);
//                    result[1] = new DatagramSocket(rtp_port+1);
//                    break;
//                } catch (SocketException e) {
//                    Log.e(DEBUGTAG, "getUDPSocketPair",e);
//                    if (result[0]!=null)
//                        result[0].close();
//                    if (result[1]!=null)
//                        result[1].close();
//                }
//            }
//        }
//        return result;
//    }
    public static boolean checkUDP(int port, int mode) 
    {
        switch(mode)
        {
            case CHECK_SIP_PORT:
                if (port < MIN_SIP_PORT_NUMBER || port > MAX_SIP_PORT_NUMBER) {
                    throw new IllegalArgumentException("Invalid start port: " + port);
                }
                break;
            case CHECK_AUDIO_PORT:
//                if (port < MIN_AUDIO_PORT_NUMBER || port > MAX_AUDIO_PORT_NUMBER) {
//                    throw new IllegalArgumentException("Invalid start port: " + port);
//                }
                if (port < mAudioPortRangeStart || port > mAudioPortRangeEnd) {
                    throw new IllegalArgumentException("Invalid start port: " + port);
                }
                break;
            case CHECK_VIDEOO_PORT:
//                if (port < MIN_VIDEO_PORT_NUMBER || port > MAX_VIDEO_PORT_NUMBER) {
//                    throw new IllegalArgumentException("Invalid start port: " + port);
//                }
                if (port < mVideoPortRangeStart || port > mVideoPortRangeEnd) {
                    throw new IllegalArgumentException("Invalid start port: " + port);
                }
                break;
        }
        DatagramSocket ds = null;
        try {
            //check port for RTP
            //ds = new DatagramSocket(port);
            if(ds==null){
            	ds = new DatagramSocket(null);
            	ds.setReuseAddress(true);
            	ds.bind(new InetSocketAddress(port));
            }
            ds.setReuseAddress(true);
            ds.close();
            return true;
        } catch (IOException e) {
            Log.e(DEBUGTAG,"checkUDP",e);
        } finally {
            if (ds != null) {
                try {
                    ds.close();
                } catch (Exception e) {
                    /* should not be thrown */
                    Log.e(DEBUGTAG,"ds.close()",e);
                }
            }
        }
        return false;
    }    
}
