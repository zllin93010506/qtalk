package com.quanta.qtalk.util;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;

import com.quanta.qtalk.util.Log;
import de.javawi.jstun.attribute.ChangedAddress;
import de.javawi.jstun.attribute.MappedAddress;
import de.javawi.jstun.attribute.MessageAttribute;
import de.javawi.jstun.header.MessageHeader;

public class DiscoveryIP {
//	private static final String TAG = "DiscoveryIP";
	private static final String DEBUGTAG = "QTFa_DiscoveryIP";
	static final int timeoutInitValue = 100; //ms
	static final int timeOutSinceFirstTransmission = 1000; //7900
	public static final String STUN_SERVER = "140.112.48.202";
	public static final int    STUN_PORT   = 3478;
	
	String[] stun_server = {"stun.xten.com",
							"10.242.120.38",
							"140.112.48.202",
							"stun.wirlab.net",
							"stun01.sipphone.com",
							"stun.iptel.org"};
	
	public static final InetSocketAddress getPublicAddress(InetSocketAddress localAddress)
	{
		InetSocketAddress result = null;
		int timeSinceFirstTransmission = 0;
		int timeout = timeoutInitValue;
		DatagramSocket datagramSocket = null;
		MappedAddress ma = null;
		ChangedAddress ca = null;
		
		while (true) 
		{
			try{
				try{
					if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"localIP="+localAddress.getAddress() +":"+localAddress.getPort());
					datagramSocket = new DatagramSocket(new InetSocketAddress(localAddress.getAddress(),localAddress.getPort()));
					datagramSocket.setReuseAddress(true);
					datagramSocket.connect(InetAddress.getByName(STUN_SERVER), STUN_PORT);
					datagramSocket.setSoTimeout(timeout);
					if( datagramSocket.isConnected())
					{
						MessageHeader sendMH = new MessageHeader(MessageHeader.MessageHeaderType.BindingRequest);
						sendMH.generateTransactionID();
						byte[] data = sendMH.getBytes();
						DatagramPacket send = new DatagramPacket(data, data.length);
						datagramSocket.send(send);
						MessageHeader receiveMH = new MessageHeader();
						while (!(receiveMH.equalTransactionID(sendMH))) {
							DatagramPacket receive = new DatagramPacket(new byte[200], 200);
							datagramSocket.receive(receive);
							receiveMH = MessageHeader.parseHeader(receive.getData());
							receiveMH.parseAttributes(receive.getData());
						}
						ma = (MappedAddress) receiveMH.getMessageAttribute(MessageAttribute.MessageAttributeType.MappedAddress);
						ca = (ChangedAddress) receiveMH.getMessageAttribute(MessageAttribute.MessageAttributeType.ChangedAddress);
						
						if ((ma == null) || (ca == null)) {
							Log.d(DEBUGTAG,"Response does not contain a Mapped Address or Changed Address message attribute.");
							break;
						} else {
							if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"public IP= " +ma.getAddress().toString() + ":"+ma.getPort() );
							result = new InetSocketAddress(ma.getAddress().toString(), ma.getPort());
							break;
						}
					}
				}
				catch(SocketTimeoutException e)
				{
					if (timeSinceFirstTransmission < timeOutSinceFirstTransmission) 
					{
						timeSinceFirstTransmission += timeout;
						int timeoutAddValue = (timeSinceFirstTransmission * 2);
						if (timeoutAddValue > timeOutSinceFirstTransmission) 
							timeoutAddValue = timeOutSinceFirstTransmission;
						timeout = timeoutAddValue;
					} else {
						// node is not capable of udp communication
						Log.e(DEBUGTAG,"Test 1: Socket timeout while receiving the response. Maximum retry limit exceed. Give up. ");
						if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Node is not capable of UDP communication. ",e);
						break;
					}
				}
				finally
				{
					if(datagramSocket != null)
					{	
						datagramSocket.close();
						datagramSocket = null;
					}
				}
			}
			catch(Throwable e)
			{
				Log.e(DEBUGTAG,"error",e);
			}
		}
		return result;
	}
}
