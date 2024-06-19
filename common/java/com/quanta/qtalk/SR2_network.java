package com.quanta.qtalk;

import java.nio.channels.SocketChannel;

import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;

public class SR2_network{
	
	public static WifiManager wifimanager;
	public static WifiLock wifilock;
	public static final String WIFILOCK = "SR2WIFILOCK"; 
	
	public static int SERVER_PORT_FOR_SEARCHING = 18888;
	public static int SELF_UDP_PORT = SERVER_PORT_FOR_SEARCHING+1;
	
	//private static String remote_ip_addr_candidate; 

	//private static DatagramChannel udp_rx_channel;
	Object server_thread_checker, client_thread_checker;
	public static SocketChannel tcp_channel;
	public static String IP;
	private static String TCPCMDPort;
	
	public final static int SOUND_PORT_INIT = 9900;
	public final static int SOUND_PORT_LIMIT = 10000;
	public final static int SOUND_PACKET_SIZE = 640;
	public final static int SOUND_PACKET_SIZE_IN_SHORT = 320;
	
	public static int REMOTE_AUDIO_PORT;
	public static int LOCAL_AUDIO_PORT_RESULT = SOUND_PORT_INIT;
	public static boolean isUDPRXEnable = false, isTCPServiceEnable = false, isUDPSoundEnable = false, isXMLFileTXEnable = false, isUDPSoundClientEnable = false, isUDPBroadcasting = false;
	
	public static void setREMOTE_AUDIO_PORT(int rEMOTE_AUDIO_PORT) {
		REMOTE_AUDIO_PORT = rEMOTE_AUDIO_PORT;
	}
	public int getREMOTE_AUDIO_PORT() {
		return REMOTE_AUDIO_PORT;
	}
	
	public static void setIP(String iP) {
		IP = iP;
	}
	public static String getIP() {
		return IP;
	}
	public static void setTCPCMDPort(String port) {
		TCPCMDPort = port;
	}
	public static String getTCPCMDPort() {
		return TCPCMDPort;
	}
	public static void setSOUND_PORT_RESULT(int sOUND_PORT_RESULT) {
		LOCAL_AUDIO_PORT_RESULT = sOUND_PORT_RESULT;
	}
	public static int getSOUND_PORT_RESULT() {
		return LOCAL_AUDIO_PORT_RESULT;
	}
	
}
