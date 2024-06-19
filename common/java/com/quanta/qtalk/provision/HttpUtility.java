package com.quanta.qtalk.provision;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;

import com.quanta.qtalk.util.Log;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui._FakeX509TrustManager;

public class HttpUtility
{
	//private static String TAG = "HttpUtility";
	private static final String DEBUGTAG = "QTFa_HttpUtility";
	private _FakeX509TrustManager tm = new _FakeX509TrustManager();
    public static String downloadFile(String url,String fileName) throws IOException
    {
        String file_path = null;
        URL url_update;
        url_update = new URL((url.startsWith("http://")?"":"http://")+url);
        URLConnection conn = url_update.openConnection();
        conn.setConnectTimeout(500);
        conn.connect();
        InputStream is = conn.getInputStream();
        File myTempFile = File.createTempFile(fileName.substring(0, fileName.lastIndexOf(".")), fileName.substring(fileName.lastIndexOf("."))); 
        FileOutputStream fos = new FileOutputStream(myTempFile); 

        int file_length = conn.getContentLength();
        if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"file length:"+file_length);
        byte buf[] = new byte[128];
        int numread = 0;
        while (file_length>0)
        {   
            numread = is.read(buf);
            for(int cnt = 0; cnt < numread; cnt++)
            {
            	if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,""+buf[cnt]);
            }
            file_length -= numread;
            if (numread <= 0) 
            { 
                break; 
            }
            fos.write(buf, 0, numread);
        }
        is.close(); 
        fos.close();
        file_path = myTempFile.getAbsolutePath();
        return file_path;
    }
    
    public static String httpsdownloadFile(String url,String fileName) throws IOException
    {
    	try{
	        String file_path = null;
	        URL url_update;
	        url_update = new URL((url.startsWith("https://")?"":"https://")+url);
	        URLConnection conn = url_update.openConnection();
	        conn.setConnectTimeout(10000);
	        conn.setReadTimeout(10000);
	        //conn.connect();
	        conn.setRequestProperty("Content-Type","text/xml;   charset=utf-8");
	        InputStream is = conn.getInputStream();
	        
	        File myTempFile = File.createTempFile(fileName.substring(0, fileName.lastIndexOf(".")), fileName.substring(fileName.lastIndexOf(".")));
	        FileOutputStream fos = new FileOutputStream(myTempFile);         
//	        int file_length = conn.getContentLength();
	        byte buf[] = new byte[128];
	        int numread = 0;
	        while (true)
	        {   
	            numread = is.read(buf);
	            if (numread <= 0) 
	            { 
	                break; 
	            }
	            fos.write(buf, 0, numread);
	        }
	        is.close(); 
	        fos.close();
	        if(myTempFile.length()==0){
	        	if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Download File Length =0 !!");
	        	myTempFile.delete();
	        	throw new Exception();
	        }
	        file_path = myTempFile.getAbsolutePath();
	        if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"file_path="+file_path);
	        return file_path;
    	}catch(Exception ex){
    		Log.e(DEBUGTAG,"",ex);
    		Log.d(DEBUGTAG,"ex:"+ex);
    		if (ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Download File failed!!");
    		throw new IOException("Download File failed!!");
    	}
    }

	public _FakeX509TrustManager getTm() {
		return tm;
	}

	public void setTm(_FakeX509TrustManager tm) {
		this.tm = tm;
	}
    
}
