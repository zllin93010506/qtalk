package com.quanta.qtalk.util;

import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.SocketException;
import java.util.Enumeration;

public class IPAddress
{
    public static String[] getLocalIPs() throws SocketException
    {
        java.util.ArrayList<String> result_vector = new java.util.ArrayList<String>();
        for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) 
        {
            NetworkInterface intf = en.nextElement();
            for (Enumeration<InetAddress> enumIpAddr =intf.getInetAddresses(); enumIpAddr.hasMoreElements();) 
            {
                InetAddress inetAddress = enumIpAddr.nextElement();
                if (!inetAddress.isLoopbackAddress() && inetAddress instanceof Inet4Address) 
                {
                    result_vector.add(inetAddress.getHostAddress().toString());
                }
            }
        }
        String[] result = new String[result_vector.size()];
        for (int i=0;i<result.length;i++)
            result[i] = result_vector.get(i);
        return result;
    }

    /**
     * Checks to see if a specific port is available.
     *
     * @param port the port to check for availability
     */
    public static boolean isPortAvailable(int port) 
    {
        ServerSocket ss = null;
        DatagramSocket ds = null;
        try {
            ss = new ServerSocket(port);
            ss.setReuseAddress(true);
            ds = new DatagramSocket(port);
            ds.setReuseAddress(true);
            return true;
        } catch (java.io.IOException e)
        {
        } finally 
        {
            if (ds != null) {
                ds.close();
            }

            if (ss != null) {
                try {
                    ss.close();
                } catch (java.io.IOException e) {
                    /* should not be thrown */
                }
            }
        }
        return false;
    }

    public static int findAvailablePort(int portStart,int portEnd) throws SocketException
    {
        int start = portStart;
        int end  = portEnd;
        if (start>end)
        {
            start = portEnd;
            end = portStart;
        }
        for (int i=start;i<=end;i++)
        {
            if (isPortAvailable(i))
                return i;
        }
        throw new SocketException("No avaiable port on specified range:"+portStart+"~"+portEnd);
    }
}
