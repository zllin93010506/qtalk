package com.quanta.qtalk.ui;


import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;

import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.http.HttpMethod;
import com.quanta.qtalk.provision.DecryptUtility;
import com.quanta.qtalk.provision.FileUtility;
import com.quanta.qtalk.provision.HttpUtility;
import com.quanta.qtalk.provision.Jtar;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.contact.QTContact2;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.PtmsData;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.ConnectException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;

import javax.net.ssl.HttpsURLConnection;

import okhttp3.Response;


public class QThProvisionUtility {
	private String URL;
    private static final String DEBUGTAG = "QSE_Provision";
	private String password = null;
	
	private final int HTTP_TIMEOUT = 3000; //10000; // set http-timeout to 3 seconds, to avoid trigger ANR (> 5 seconds)
	private final int PROVISION_UPDATE_SUCCESS = 1;
	private final int PHONEBOOK_UPDATE_SUCCESS = 2;
	private final int ACTIVATE_LOGIN_SUCCESS = 1;
	private final int LOGIN_PROVISION_SUCCESS = 3;
	private final int LOGIN_PHONEBOOK_SUCCESS = 4;
	private final int DEACTIVATION_SUCCESS = 6 ;
	private final int AVATAR_SUCCESS = 7;
	private final int AVATAR_FAIL = 8;
	private final int BROADCAST_SUCCESS = 9;
	private final int BROADCAST_FAIL = 10;
	private final int BROADCAST_BUSY = 11;
	private final int BROADCAST_IN_PROGRESS = 12;
	private final int BUSY = 0;
	private final String TAG_SERVICE = "service";
	
	private final String TAG_PROVISION = "provision";
	private final String TAG_PHONEBOOK = "phonebook";
	private final String TAG_BROADCAST = "broadcastmcu";
	private final String PROVISION_FILENAME = "provision.xml";
	private final String PHONEBOOK_FILENAME = "phonebook.xml";
	
	final String TAG_SERVICE_ACTIVATE = "activate";
	final String TAG_SERVICE_DEACTIVATE = "deactivate";
	final String TAG_ROLE = "role";
	final String TAG_UID = "uid";
	final String TAG_MSG = "msg";
	final String TAG_CONFIRM = "confirm";
	final String TAG_KEY = "key";
	final String TAG_TYPE = "type";
	final String TAG_AVATAR = "avatar";
	final String TAG_SUCCESS = "success";
	final String TAG_TIMESTAMP = "timestamp";
	final String TAG_TIMESTAMP_OLD = "timestamp_old";
	final String TAG_SESSION = "session";
	final String TAG_BUSY = "busy";
	final String TAG_WEBVIEW = "webview";
	final String provision_file = Hack.getStoragePath()+PROVISION_FILENAME;
	final String phonebook_file = Hack.getStoragePath()+PHONEBOOK_FILENAME;
	int count = 0;
	
	public static ProvisionInfo provision_info = null;
	QtalkSettings qtalk_settings;
	Object lock_obj = new Object();
    private Handler mUI_Handler;
	private _FakeX509TrustManager tm = new _FakeX509TrustManager();
	public QThProvisionUtility (Handler m_Handler,String URL){
		this.setURL(URL);
		mUI_Handler=m_Handler;
	}
	private File FilePhonebook = new File (phonebook_file);
	private File FileProvision = new File (provision_file);


	void response_handler(String result, Bundle result_bundle) throws Exception
	{
		if((result_bundle!=null)&&("phonebook".indexOf(result_bundle.getString("service", ""))<0)) //do not print phonebook check response messsage
			Log.d(DEBUGTAG,"response(), " + result_bundle);
		
		/***************************************login check******************************************/		
		if(result.startsWith("service=activate&msg=idfail"))
		{
			Log.d(DEBUGTAG,"=> login_failed, invaild uid");
			Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.BUSY;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
		}
		else if(result.startsWith("service=activate&msg=confirmfail"))
		{
			Log.d(DEBUGTAG, "=> login failed, invaild password");
			Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.BUSY;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
		}
		/***************************************provision check******************************************/
		else if(result_bundle.getString(TAG_SERVICE).contentEquals(TAG_PROVISION))
		{
			if(result_bundle.getString(TAG_TIMESTAMP) != null)
			{
				if (result_bundle.getString(TAG_TIMESTAMP).equals(result_bundle.getString(TAG_TIMESTAMP_OLD)))
				{
					if (!FileProvision.exists()) {	
						ProvisionDownload(result_bundle,result);
					}
					provision_info = ProvisionUtility.parseProvisionConfig(provision_file);
					Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
					provision_response_msg.what = this.LOGIN_PROVISION_SUCCESS;
					provision_response_msg.setData(result_bundle);
		            mUI_Handler.sendMessage(provision_response_msg);
				}
				else {
					Log.d(DEBUGTAG,"=> it needs to provision.");
					ProvisionDownload(result_bundle,result);
					provision_info = ProvisionUtility.parseProvisionConfig(provision_file);
					Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
					provision_response_msg.what = this.PROVISION_UPDATE_SUCCESS;
					provision_response_msg.setData(result_bundle);
		            mUI_Handler.sendMessage(provision_response_msg);
				}
			}
			else
			{
				if (!FileProvision.exists()) {	
					ProvisionDownload(result_bundle,result);
				}
				provision_info = ProvisionUtility.parseProvisionConfig(provision_file);
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.LOGIN_PROVISION_SUCCESS;
				provision_response_msg.setData(result_bundle);
	            mUI_Handler.sendMessage(provision_response_msg);
			}
		}
		/***************************************phonebook check******************************************/
		else if(result_bundle.getString(TAG_SERVICE).contentEquals(TAG_PHONEBOOK))
		{
			//Log.d(DEBUGTAG,"=> phonebook check success");
			if(result_bundle.getString(TAG_TIMESTAMP) != null)
			{
				if (result_bundle.getString(TAG_TIMESTAMP).equals(TAG_TIMESTAMP_OLD))
				{
					if (!FilePhonebook.exists()) {	
						PhonebookDownload(result_bundle,result);
					}
					Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
					provision_response_msg.what = this.LOGIN_PHONEBOOK_SUCCESS;
					provision_response_msg.setData(result_bundle);
		            mUI_Handler.sendMessage(provision_response_msg);
				}
				else
				{
					Log.d(DEBUGTAG,"=> it needs to provision phonebook.");
					PhonebookDownload(result_bundle,result);
					Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
					provision_response_msg.what = this.PHONEBOOK_UPDATE_SUCCESS;
					provision_response_msg.setData(result_bundle);
		            mUI_Handler.sendMessage(provision_response_msg);
				}
			}
			else
			{
				if (!FilePhonebook.exists()) {	
					PhonebookDownload(result_bundle,result);
				}
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.LOGIN_PHONEBOOK_SUCCESS;
				provision_response_msg.setData(result_bundle);
	            mUI_Handler.sendMessage(provision_response_msg);
			}
		}
		/***************************************activate check******************************************/
		else if(result_bundle.getString(TAG_SERVICE).contentEquals(TAG_SERVICE_ACTIVATE) && result_bundle.getString(TAG_MSG).equals(TAG_SUCCESS))
		{
				if(result_bundle.getString(TAG_KEY) != null)
				{
					if(result_bundle.getString(TAG_SESSION) != null)
					{
						Log.d(DEBUGTAG,"=> activate login_success");
						QtalkDB qtalkdb = new QtalkDB();
						qtalkdb.insertATEntry(result_bundle.getString(TAG_SESSION),
								result_bundle.getString(TAG_KEY),
								result_bundle.getString("server"),
								result_bundle.getString(TAG_UID),
								result_bundle.getString(TAG_TYPE),
								result_bundle.getString("version"));
						Cursor cs = qtalkdb.returnAllATEntry();
				        cs.moveToFirst();
				        int cscnt = 0;
				        for( cscnt = 0; cscnt < cs.getCount(); cscnt++) {
				        	cs.moveToNext();
				        }
				        cs.close();
				        qtalkdb.close();
						Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
						provision_response_msg.what = this.ACTIVATE_LOGIN_SUCCESS;
						provision_response_msg.setData(result_bundle);
			            mUI_Handler.sendMessage(provision_response_msg);
			            
					}
					else    //service=activate&msg=success&key={key}
					{
						throw new IOException("=> there's no sessionID.");
					}
				}
				else   //service=activate&msg=success&session={session}
				{
					if(result_bundle.getString(TAG_ROLE) != null){
			        	Log.d(DEBUGTAG,"=> login_success");
						Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
						provision_response_msg.what = this.ACTIVATE_LOGIN_SUCCESS;
						provision_response_msg.setData(result_bundle);
			            mUI_Handler.sendMessage(provision_response_msg);
			        }
					else
						throw new IOException("=> there's no authtication key.");
				}
				
			}
		/***************************************deactivate check******************************************/
		else if(result_bundle.getString(TAG_SERVICE).contentEquals(TAG_SERVICE_ACTIVATE) && result_bundle.getString(TAG_MSG).equals(TAG_SERVICE_DEACTIVATE))  //service=activate&msg=deactivate
		{
			Log.d(DEBUGTAG,"=> deactivate");
	        Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.DEACTIVATION_SUCCESS;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
		}
		/***************************************avatar check******************************************/
		else if(result_bundle.getString(TAG_SERVICE).equals(TAG_AVATAR))
		{
			if(result_bundle.getString(TAG_MSG)!=null)
			{
				Log.d(DEBUGTAG,"=> avatar success!!");
		        
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.AVATAR_SUCCESS;
				provision_response_msg.setData(result_bundle);
	            mUI_Handler.sendMessage(provision_response_msg);
			}
			else
			{
				Log.d(DEBUGTAG,"=> avatar FAILED!!");
		        
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.AVATAR_FAIL;
				provision_response_msg.setData(result_bundle);
	            mUI_Handler.sendMessage(provision_response_msg);
			}
		}
		/*************************************server status check****************************************/
		else if(result_bundle.getString(TAG_MSG).contentEquals(TAG_BUSY))
		{
			Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.BUSY;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
		}
		/*******************************************broadcast*******************************************/
		else if(result_bundle.getString(TAG_SERVICE).equals(TAG_BROADCAST)) {
			String msg = result_bundle.getString(TAG_MSG);
			if (msg.equals("success")) {
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.BROADCAST_SUCCESS;
				provision_response_msg.setData(result_bundle);
				mUI_Handler.sendMessage(provision_response_msg);
			} else if (msg.equals("no_available_broadcast_agent")) {
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.BROADCAST_BUSY;
				provision_response_msg.setData(result_bundle);
				mUI_Handler.sendMessage(provision_response_msg);
			} else if (msg.equals("department_during_broadcasting")) {
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.BROADCAST_IN_PROGRESS;
				provision_response_msg.setData(result_bundle);
				mUI_Handler.sendMessage(provision_response_msg);
			} else {
				Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
				provision_response_msg.what = this.BROADCAST_FAIL;
				provision_response_msg.setData(result_bundle);
				mUI_Handler.sendMessage(provision_response_msg);
			}
		}
		/*******************************************else************************************************/
		else
		{
			Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.BUSY;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
		}
	}
	//response_handler end

	/**************************************HttpsURLConnection Start********************************************/
	
	/*************keep alive***************/
	public void httpsConnect(String url) throws IOException
	{
		synchronized(lock_obj) {
	    	try{ 
	    		Log.d(DEBUGTAG, "httpsConnect(keep alive), url:"+url);

				HttpMethod http = new HttpMethod();
				Response detailRes = http.getRequest(url);
				String result = detailRes.body().string();
				Log.d(DEBUGTAG, "httpsConnect detailRes=" + result);

				Bundle result_bundle = cmd_parser( result);
				response_handler( result, result_bundle);
	
	    	}
	    	catch(Exception ex) {
	    		Log.e(DEBUGTAG,"",ex);
	    		throw new IOException("<error> keep alive, ex:"+ex);
	    	}
	    	finally {
	    	}
		}
    	
    }
	
	/*************broadcast request***************/
	public void httpsConnect(String url,String x) throws IOException
	{
		synchronized(lock_obj) {
	    	try{ 
	    		Log.d(DEBUGTAG,"httpsConnect(broadcast), url:"+url);

				HttpMethod http = new HttpMethod();
				Response detailRes = http.getRequest(url);
				String result = detailRes.body().string();
				Log.d(DEBUGTAG, "httpsConnect detailRes=" + result);

				Bundle result_bundle = cmd_parser( result);
				response_handler( result, result_bundle);
	
	    	}catch(Exception ex){
	    		Log.e(DEBUGTAG,"",ex);
	    		throw new IOException("<error> broadcast failed, ex:"+ex);
	    	}finally {
	    	}
		}
    }
	/*************login check (session&key exist)***************/
	public void httpsConnect(String url, String session, String version) throws IOException
	{
		synchronized(lock_obj) {
			HttpsURLConnection con = null;
	    	try{ 
	    		Log.d(DEBUGTAG,"httpsConnect(login check), url:" + url);

				HttpMethod http = new HttpMethod();
				Response detailRes = http.getRequest(url);
				String result = detailRes.body().string();
				Log.d(DEBUGTAG, "httpsConnect detailRes=" + result);

				Bundle result_bundle = cmd_parser( result);
				result_bundle.putString("session", session);
				result_bundle.putString("version", version);
				response_handler( result, result_bundle);
	
	    	}catch(Exception ex){
	    		Log.e(DEBUGTAG,"",ex);
	    		throw new IOException("<error> login check failed. ex:"+ex);
	    	}finally {
	    		if(con!=null){
	    			con.disconnect();
	    			con = null;
	    		}
	    	}
		}
    }
	/*************activate check (session&key doesn't exist)***************/
	public void httpsConnect(String url, String server, String uid, String type, String version) throws IOException
	{
		synchronized(lock_obj) {
	    	try{ 
	    		Log.d(DEBUGTAG,"httpsConnect(activate), url:"+url);

				HttpMethod http = new HttpMethod();
				Response detailRes = http.getRequest(url);
				String result = detailRes.body().string();
				Log.d(DEBUGTAG, "httpsConnect detailRes=" + result);

				Bundle result_bundle = cmd_parser( result);
				result_bundle.putString("server", server);
				result_bundle.putString("uid", uid);
				result_bundle.putString("type", type);
				result_bundle.putString("version", version);
				response_handler( result, result_bundle);
	
	    	}catch(Exception ex){
	    		Log.e(DEBUGTAG,"",ex);
	    		throw new IOException("<error> activate failed, ex:"+ex);
	    	}finally {
	    	}
		}
    }	
	/*************login provision & phonebook check***************/
	public void httpsConnect(String url, String session, String timestamp_old, String uid, String type, String Pserver, String key) throws IOException
	{
		synchronized(lock_obj) {

	    	try{ 
	    		//Log.d(DEBUGTAG,"httpsConnect(phonebook check), url:"+url);
				HttpMethod http = new HttpMethod();
				Response detailRes = http.getRequest(url);
				String result = detailRes.body().string();
				Log.d(DEBUGTAG, "httpsConnect detailRes=" + result);

				Bundle result_bundle = cmd_parser( result);
				result_bundle.putString("session", session);
				result_bundle.putString("timestamp_old", timestamp_old);
				result_bundle.putString("Pserver", Pserver);
				result_bundle.putString("uid", uid);
				result_bundle.putString("type", type);
				result_bundle.putString("key", key);
				response_handler( result, result_bundle);
	    	}catch(ConnectException connex){
				Log.d(DEBUGTAG,"httpsConnect(phonebook check), url:"+url);
	    		throw connex;
	    	}catch(Exception ex){
				Log.d(DEBUGTAG,"httpsConnect(phonebook check), url:"+url);
	    		Log.e(DEBUGTAG,"",ex);
	    		throw new IOException("<error> phonebook check failed, ex:"+ex);
	    	}finally {
	    	}
		}
    }
	
	public final static String MOBILE_ACTION_UNDEF = "";
	public final static String MOBILE_ACTION_WAKE = "wake";
	public final static String MOBILE_ACTION_SLEEP = "sleep";
	public final static String MOBILE_ACTION_CLEAR = "clear";
	
	
	private void setMobileStatus(Context context, boolean status)	{
		Log.i(DEBUGTAG, "set VCLOGIN="+status);
		PreferenceManager.getDefaultSharedPreferences(context).edit().putBoolean("MOBILE_STATUS", status).commit();
	}

	private boolean getMobileStatus(Context context) {
		boolean ret = PreferenceManager.getDefaultSharedPreferences(context).getBoolean("MOBILE_STATUS", false);
		Log.i(DEBUGTAG, "VCLOGIN="+ret);
		return ret;
	}
	
	private void setMobileAction(Context context, String action)	{
		Log.i(DEBUGTAG, "set MOBILE_ACTION="+action);
		PreferenceManager.getDefaultSharedPreferences(context).edit().putString("MOBILE_ACTION", action).commit();
	}

	private String getMobileAction(Context context) {
		String ret = PreferenceManager.getDefaultSharedPreferences(context).getString("MOBILE_ACTION", MOBILE_ACTION_UNDEF);
		Log.i(DEBUGTAG, "MOBILE_ACTION="+ret);
		return ret;
	}

	public void mobile(final Context context, final String uid, final String action, final String sip_id, final String token) throws IOException
	{
		synchronized(lock_obj) {
			HttpsURLConnection con = null;
			try{
				String url = QThProvisionUtility.provision_info.phonebook_uri
							+ "?service=mobile&type=qtfa"
							+ "&uid="
							+ uid
							+ "&action="
							+ action
							+ "&sip_id="
							+ sip_id;
				if(action.compareTo("wake") == 0)
						url += ("&token=" + token);
				Log.d(DEBUGTAG,"mobile, url:"+url);

	            con = (HttpsURLConnection) new URL(url).openConnection();
	            con.setConnectTimeout(HTTP_TIMEOUT);
	            con.setReadTimeout(HTTP_TIMEOUT);
	            con.setRequestProperty("Content-Type","text/xml;   charset=utf-8");
	            con.setRequestProperty("Connection", "close");
	            con.connect();

	            InputStream instream = con.getInputStream();
				String result = convertStreamToString(instream);
				instream.close();
				instream = null;

				setMobileAction(context, action);
				if (result.startsWith("service=mobile&msg=success")) {
					Log.d(DEBUGTAG, "=> mobile " + action + " success");
					setMobileStatus(context, true);
				} else {
					setMobileStatus(context, false);
					if (result.startsWith("service=mobile&msg=nosipid")) {
						Log.e(DEBUGTAG, "=> mobile " + action + " failed, parameter with out sip id.");
					} else if (result.startsWith("service=mobile&msg=noaction")) {
						Log.e(DEBUGTAG, "=> mobile " + action + " failed, parameter with out action code");
					} else if (result.startsWith("service=mobile&msg=nosuchaction")) {
						Log.e(DEBUGTAG, "=> mobile " + action + " failed, action parameter is wrong");
					} else if (result.startsWith("service=mobile&msg=notoken")) {
						Log.e(DEBUGTAG, "=> mobile " + action + " failed, invalid token");
					} else if (result.startsWith("service=mobile&msg=busy")) {
						Log.e(DEBUGTAG, "=> mobile " + action + " failed url:" + url + " service is error");
					} else {
						Log.e(DEBUGTAG, "=> mobile " + action + " failed url:" + url + " unknown error " + result);
					}
				}
			} catch (ConnectException connex) {
				throw connex;
			}catch(Exception ex){
				Log.e(DEBUGTAG,"",ex);
				throw new IOException("<error> mobile "+action+" failed ex:"+ex);
			}finally {
				if(con!=null){
					con.disconnect();
					con = null;
				}
			}
		}
    }
	
	public void doFileUpload(String path, String uid, String type, String session, String server) throws Exception
	{
		HttpURLConnection conn = null;
		DataOutputStream dos = null;

		// Is this the place are you doing something wrong.
		String lineEnd = "\r\n";
		String twoHyphens = "--";
		String boundary =  "===============================";

		int bytesRead, bytesAvailable, bufferSize;
		byte[] buffer;
		int maxBufferSize = 1*1024*1024;
		String urlString = server+type+"&service=avatar&action=upload&uid="+uid+"&session="+session;
		Log.d(DEBUGTAG,"doFileUpload(), DOING avatar!!");
		try
		{
			//------------------ CLIENT REQUEST 
			Log.e(DEBUGTAG,"doFileUpload(), Inside second Method");
			FileInputStream fileInputStream = new FileInputStream(new File(path) );
			// Open a HTTP connection to the URL
			conn = (HttpsURLConnection) new URL(urlString).openConnection();
            conn.setConnectTimeout(HTTP_TIMEOUT);
            conn.setReadTimeout(HTTP_TIMEOUT);
            // Allow Inputs
            conn.setDoInput(true);
            // Allow Outputs
            conn.setDoOutput(true);
            // Don't use a cached copy.
            conn.setUseCaches(false);
            // Use a post method.
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Connection", "Keep-Alive");
            conn.setRequestProperty("Content-Type", "multipart/form-data;boundary="+boundary);
            conn.setRequestProperty("Connection", "close");

            dos = new DataOutputStream( conn.getOutputStream() );
            dos.writeBytes(twoHyphens + boundary + lineEnd);
            dos.writeBytes("Content-Disposition: form-data; name=\"file\";filename=\"QOCAMyimage.jpg\""+ lineEnd);
            dos.writeBytes("Content-Type: image/jpeg"+ lineEnd);
            dos.writeBytes(lineEnd);
            Log.e(DEBUGTAG,"doFileUpload(), Headers are written");

            // create a buffer of maximum size
            bytesAvailable = fileInputStream.available();
            bufferSize = Math.min(bytesAvailable, maxBufferSize);
            buffer = new byte[bufferSize];

            // read file and write it into form...
            bytesRead = fileInputStream.read(buffer, 0, bufferSize);
            while (bytesRead > 0)
            {
                     dos.write(buffer, 0, bufferSize);
                     bytesAvailable = fileInputStream.available();
                     bufferSize = Math.min(bytesAvailable, maxBufferSize);
                     bytesRead = fileInputStream.read(buffer, 0, bufferSize);                                                
            }

            // send multipart form data necesssary after file data...
            dos.writeBytes(lineEnd);
            dos.writeBytes(twoHyphens + boundary + twoHyphens + lineEnd);

            // close streams
            Log.e(DEBUGTAG,"doFileUpload(), File is written");
            fileInputStream.close();
            dos.flush();
            dos.close();
		}
		catch (MalformedURLException ex)
		{
			Log.e(DEBUGTAG, "doFileUpload(), error: " + ex.getMessage(), ex);
		}
		catch (IOException ioe)
		{
			Log.e(DEBUGTAG, "doFileUpload(), error: " + ioe.getMessage(), ioe);
		}
		catch (Exception e)
		{
			Log.e(DEBUGTAG, "doFileUpload(), error: " + e.getMessage(), e);
		}

		//------------------ read the SERVER RESPONSE
		try 
		{
			InputStream instream = conn.getInputStream();
			String result = convertStreamToString(instream);
			instream.close();
			instream = null;
			Bundle result_bundle = cmd_parser(result);
			result_bundle.putString("path", path);
			result_bundle.putString("uid", uid);
			result_bundle.putString("session", session);
			response_handler( result, result_bundle);
		}
		catch (IOException ioex){
			Log.e(DEBUGTAG, "doFileUpload(), error: " + ioex.getMessage(), ioex);
        }finally {
    		if(conn!=null){
    			conn.disconnect();
    			conn = null;
    		}
    	}		
	}
	
	Bundle cmd_parser(String cmd)
	{
		Bundle ret_b = new Bundle();
		String[] fields_ = cmd.split("!!");
		
		
		String[] fields = fields_[0].split("&");
		String[] items;
		String token = null, content = null;
		for(int cnt = 0; cnt < fields.length; cnt++)
		{
			items = fields[cnt].split("=");
			for(int cmd_cnt = 0;cmd_cnt < items.length ;cmd_cnt++)
	    	{
				if( cmd_cnt == 1)
				{
					content = items[cmd_cnt];
				}
				else
				{
					token = items[cmd_cnt];
				}
	    	}
			ret_b.putString(token, content);
		}
		return ret_b;	
	}

	
    private static String convertStreamToString(InputStream is){
		BufferedReader reader =new BufferedReader(new InputStreamReader(is));
		StringBuilder sb =new StringBuilder();
		String line =null;
		try{
			while ((line=reader.readLine())!=null){
				sb.append(line+"\n");
			}
		}catch(IOException e){
			Log.e(DEBUGTAG,"convertStreamToString:"+e);
		}finally{
			try{
				is.close();
			}catch (IOException e){
				Log.e(DEBUGTAG,"convertStreamToString:"+e);
			}
			try{
				reader.close();
			}catch (IOException e){
				Log.e(DEBUGTAG,"convertStreamToString:"+e);
			}			
		}
    	return sb.toString();
    }
    
    private void PhonebookDownload(Bundle result_bundle, String result) throws Exception{
    	String provisionType = result_bundle.getString(TAG_TYPE, "");
    	String provision_url = result_bundle.getString("Pserver") +provisionType+"&service=phonebook&action=download&uid="+result_bundle.getString("uid")+"&session="+result_bundle.getString("session");
		
    	Log.d(DEBUGTAG,"PhonebookDownload(), url:"+provision_url);
		
    	password=result_bundle.getString("key");
    	try{
			String download_file_path = HttpUtility.httpsdownloadFile(provision_url, PHONEBOOK_FILENAME);
			String outputFile=download_file_path.substring(download_file_path.lastIndexOf("/")+1);
			String aescript_file=DecryptUtility.decryptFile(download_file_path, password, outputFile);
			Jtar jtar = new Jtar();
		    jtar.untar(aescript_file,Hack.getStoragePath());
			ArrayList<QTContact2> temp_list = QtalkDB.parseProvisionPhonebook(phonebook_file);
			QtalkDB qtalkdb = new QtalkDB();
			qtalkdb.importProvisioningPhonebook(temp_list);
			Cursor cs = qtalkdb.returnAllPBEntry();
	        cs.close();
	        qtalkdb.close();
	        FileUtility.delFile(download_file_path);
	        FileUtility.delFile(outputFile);
			FileUtility.delFile(aescript_file);
		}catch (FileNotFoundException e){
			Log.d(DEBUGTAG,"PhonebookDownload(), ex:"+e);
			Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.BUSY;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
            return;
		}
    }
    
    private void ProvisionDownload(Bundle result_bundle, String result) throws Exception{
    	String provisionType = result_bundle.getString(TAG_TYPE, "");
    	String provision_url = result_bundle.getString("Pserver") +provisionType+"&service=provision&action=download&uid="+result_bundle.getString("uid")+"&session="+result_bundle.getString("session");
		Log.d(DEBUGTAG,"ProvisionDownload(), url:"+provision_url);
		try{
			String download_file_path = HttpUtility.httpsdownloadFile(provision_url, PROVISION_FILENAME);
			String outputFile=download_file_path.substring(download_file_path.lastIndexOf("/")+1);
			password=result_bundle.getString("key");
			String aescript_file=DecryptUtility.decryptFile(download_file_path, password, outputFile);
			Jtar jtar = new Jtar();
		    jtar.untar(aescript_file,Hack.getStoragePath());
			FileUtility.delFile(download_file_path);
			FileUtility.delFile(outputFile);
			FileUtility.delFile(aescript_file);
		}catch (FileNotFoundException e){
			Log.e(DEBUGTAG,"ProvisionDownload(), ex:"+e);
			Message provision_response_msg = mUI_Handler.obtainMessage(1, result);
			provision_response_msg.what = this.BUSY;
			provision_response_msg.setData(result_bundle);
            mUI_Handler.sendMessage(provision_response_msg);
           return;
		}
    }
	Calendar mLastServiceChangedTime = null;
	Object mLock = new Object();
    public void ServiceChanged(final String uid, final boolean realChanged) {
    	if(!realChanged && !PtmsData.isExpired(5)) {
			Log.d(DEBUGTAG, "ServiceChanged: skip fake ServiceChanged 1");
			return;
		}
		Log.d(DEBUGTAG, "ServiceChanged start, real:" + realChanged);
    	synchronized (mLock) {
			if (mLastServiceChangedTime != null) {
				Calendar now = Calendar.getInstance();
				if(mLastServiceChangedTime.compareTo(now) > 0) {
					if(!realChanged) {
						Log.d(DEBUGTAG, "ServiceChanged: skip fake ServiceChanged 2");
						return;
					}
					mLastServiceChangedTime.clear();
					now.add(Calendar.SECOND, 5);
					mLastServiceChangedTime = now;
					Log.d(DEBUGTAG, "ServiceChanged: delay ServiceChanged");
					new Thread(new Runnable() {
						@Override
						public void run() {
							try {
								Thread.sleep(5 * 1000);
								QThProvisionUtility.this.ServiceChanged(uid, true);
							} catch (InterruptedException e) {
								Log.e(DEBUGTAG, "ServiceChanged delay ServiceChanged error", e);
							}
						}
					}
					).start();
					return;
				}
				else {
					mLastServiceChangedTime = Calendar.getInstance();
					mLastServiceChangedTime.add(Calendar.SECOND, 5);
				}
			} else {
				mLastServiceChangedTime = Calendar.getInstance();
				mLastServiceChangedTime.add(Calendar.SECOND, 5);
			}
		}
		if(!Hack.isPhone()) {
			return;
		}
			if((QtalkSettings.ptmsServer == null) || (!Hack.isUUID(QtalkSettings.tts_deviceId))) {
			Log.d(DEBUGTAG, "ServiceChanged ptmsServer is not valid ptmsServer="+QtalkSettings.ptmsServer+", uuid="+QtalkSettings.tts_deviceId);
			return;
		}
		try {
			String url = "https://" + QtalkSettings.ptmsServer + "/api/v2/practitioners?user=" + uid;
			Log.d(DEBUGTAG, "ServiceChanged url=" + url);
			HttpMethod http = new HttpMethod();
			Response detailRes = http.getRequestTokenAuth(url);
			if (detailRes == null) {
				Log.e(DEBUGTAG, "ServiceChanged read practitioners error");
				return;
			}
			if (detailRes.body() == null) {
				Log.e(DEBUGTAG, "ServiceChanged read practitioners is null");
				return;
			}

			JSONObject obj = new JSONObject(detailRes.body().string());
			int count = obj.getInt("count");
			Log.d(DEBUGTAG, "user:" + uid + "detailRes:" + count);
			String[] patients = new String[count];
			JSONArray objResults = (JSONArray) obj.get("results");
			for (int i = 0; i < count; i++) {
				JSONObject dataObj = objResults.getJSONObject(i);
				patients[i] = dataObj.getString("patient");
			}
			GetPatients(patients);
		}
		catch (Exception ex) {
			Log.d(DEBUGTAG,"ServiceChanged ex:", ex);
		}
	}
	private String buildFullName(JSONObject jsonObjResult){
		String firstName = "";
		String lastName = "";

		try {
			if(jsonObjResult.isNull( "first_name" )){
				Log.d(DEBUGTAG, "firstName is null");
			}
			else{
				firstName = jsonObjResult.get("first_name").toString();
			}

			if(jsonObjResult.isNull( "last_name" )){
				Log.d(DEBUGTAG, "lastName is null");
			}
			else{
				lastName = jsonObjResult.get("last_name").toString();
			}

		} catch (JSONException e) {
			Log.e(DEBUGTAG, "buildFullName error: ", e);
		}

		String fullName = "";

		if(firstName.length() !=0 && lastName.length() !=0){
			//name is chinese
			if(firstName.matches("[\\u4E00-\\u9FFF]+") ||
					lastName.matches("[\\u4E00-\\u9FFF]+")) {
				fullName = lastName + firstName;
			}else{
				fullName = firstName + " " + lastName;
			}
		}else{
			if(lastName.length() != 0){
				fullName = lastName.toString();
			}else if(firstName.length() !=0){
				fullName = firstName.toString();
			}

		}

		return fullName.trim();
	}

	private void GetPatients(String[] patients) {
		if(patients == null) {
			return;
		}
		try {
			String charnolist = "";
			for(int i=0; i<patients.length; i++) {
				charnolist += (patients[i] + ",");
			}
			String url = "https://" + QtalkSettings.ptmsServer + "/api/v2/patients?chart_no=" + charnolist.replaceAll("null,", "");
			Log.d(DEBUGTAG, "ServiceChanged GetPatients url=" + url);
			HttpMethod http = new HttpMethod();
			Response detailRes = http.getRequestTokenAuth(url);
			if (detailRes == null) {
				Log.e(DEBUGTAG, "ServiceChanged GetPatients read practitioners error");
				return;
			}
			if (detailRes.body() == null) {
				Log.e(DEBUGTAG, "ServiceChanged GetPatients read practitioners is null");
				return;
			}

			JSONObject obj = new JSONObject(detailRes.body().string());
			int count = obj.getInt("count");
			HashMap<String, PtmsData> map = new HashMap<>();
			JSONArray objResults = (JSONArray) obj.get("results");
			for (int i = 0; i < count; i++) {
				JSONObject dataObj = objResults.getJSONObject(i);
				//Log.d(DEBUGTAG, "ServiceChanged GetPatients=" + dataObj.toString());
				PtmsData data = new PtmsData();
				data.alias = dataObj.getString("alias");
				data.title = dataObj.getString("room_bed");
				data.bedid = dataObj.getString("chart_no");
				data.name = buildFullName(dataObj);
				//Log.d(DEBUGTAG, "ServiceChanged GetPatients name=" + data.name);
				//Log.d(DEBUGTAG, "ServiceChanged GetPatients alias=" + data.alias);
				//Log.d(DEBUGTAG, "ServiceChanged GetPatients title=" + data.title);
				//Log.d(DEBUGTAG, "ServiceChanged GetPatients bedid=" + data.bedid);
				if(data.title != null) {
					Log.d(DEBUGTAG, "ServiceChanged GetPatients title=" + data.title);
					map.put(data.title, data);
				}
			}
			if(map.size() != 0) {
				GetBeds(map);
			}
		}
		catch (Exception ex) {
			Log.d(DEBUGTAG,"ServiceChanged GetPatients ex:", ex);
		}
	}

	private void GetBeds(HashMap<String, PtmsData> map) {
		if(map == null) {
			return;
		}
		try {
			String bedlist = "";
			for(Object bed : map.keySet()) {
				bedlist += ((String) bed + ",");
			}

			if(bedlist.equals("")) {
				Log.d(DEBUGTAG,"ServiceChanged GetBeds bed is empty");
				return;
			}
			String url = "https://" + QtalkSettings.ptmsServer + "/api/v2/beds?id=" + bedlist.replaceAll("null,", "");
			Log.d(DEBUGTAG, "ServiceChanged GetBeds url=" + url);
			HttpMethod http = new HttpMethod();
			Response detailRes = http.getRequestTokenAuth(url);
			if (detailRes == null) {
				Log.e(DEBUGTAG, "ServiceChanged GetBeds read practitioners error");
				return;
			}
			if (detailRes.body() == null) {
				Log.e(DEBUGTAG, "ServiceChanged GetBeds read practitioners is null");
				return;
			}

			JSONObject obj = new JSONObject(detailRes.body().string());
			int count = obj.getInt("count");
			HashMap<String, PtmsData> bedmap = new HashMap<>();
			JSONArray objResults = (JSONArray) obj.get("results");
			for (int i = 0; i < count; i++) {
				JSONObject dataObj = objResults.getJSONObject(i);
				//Log.d(DEBUGTAG, "ServiceChanged GetBeds=" + dataObj.toString());
				PtmsData data = new PtmsData();
				if(!dataObj.has("sip") || !dataObj.has("id") || !dataObj.has("name"))
					continue;
				data.sipid = dataObj.getString("sip");
				data.bedid = dataObj.getString("id");
				data.name = dataObj.getString("name");
				//Log.d(DEBUGTAG, "ServiceChanged GetBeds name=" + data.name);
				//Log.d(DEBUGTAG, "ServiceChanged GetBeds bedid=" + data.bedid);
				//Log.d(DEBUGTAG, "ServiceChanged GetBeds sipid=" + data.sipid);
				Log.d(DEBUGTAG, "ServiceChanged GetBeds:"+data.bedid+","+data.sipid);
				if(data.bedid != null) {
					bedmap.put(data.bedid, data);
				}
			}
			PtmsData.clearData();
			synchronized (PtmsData.getLockedObject()) {
				for (Object bedid : bedmap.keySet()) {
					PtmsData patient = map.get(bedid);
					PtmsData bed = bedmap.get(bedid);
					try {
						if ((patient == null) || (bed == null)) {
							Log.e(DEBUGTAG, "ServiceChanged GetBeds patient=null?" + (patient == null));
							Log.e(DEBUGTAG, "ServiceChanged GetBeds bed=null?" + (bed == null));
							return;
						}
						PtmsData.addData(bed.sipid, new PtmsData(patient.title, patient.name, patient.alias, bed.sipid, bed.name));
						//Log.d(DEBUGTAG, "ServiceChanged GetBeds final name=" + patient.name);
						//Log.d(DEBUGTAG, "ServiceChanged GetBeds final alias=" + patient.alias);
						//Log.d(DEBUGTAG, "ServiceChanged GetBeds final title=" + bed.name);
						//Log.d(DEBUGTAG, "ServiceChanged GetBeds final sipid=" + bed.sipid);
						//Log.d(DEBUGTAG, "ServiceChanged GetBeds final id=" + patient.title);
						Log.d(DEBUGTAG, "ServiceChanged GetBeds:"+bed.name+","+bed.sipid);
					} catch (Exception ex) {
						Log.e(DEBUGTAG, "ServiceChanged GetBeds read " + bedid + " failed");
					}
				}
			}
		}
		catch (Exception ex) {
			Log.d(DEBUGTAG,"ServiceChanged GetBeds ex:", ex);
		}
		Log.d(DEBUGTAG, "ServiceChanged end");
	}

	public void ServiceChangedForNS(String uid) {
		if(uid == null) {
			Log.e(DEBUGTAG, "ServiceChanged uid == null");
			return;
		}
		Log.d(DEBUGTAG, "ServiceChangedForNS start");
		try {
			String url = "https://" + QtalkSettings.ptmsServer + "/api/beds?nstation=" + uid;
			Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS url=" + url);
			HttpMethod http = new HttpMethod();
			Response detailRes = http.getRequestTokenAuth(url);
			if (detailRes == null) {
				Log.e(DEBUGTAG, "ServiceChanged GetBedsForNS read practitioners error");
				return;
			}
			if (detailRes.body() == null) {
				Log.e(DEBUGTAG, "ServiceChanged GetBedsForNS read practitioners is null");
				return;
			}

			HashMap<String, PtmsData> bedmap = new HashMap<>();
			JSONArray objResults;
			try {
				objResults = new JSONArray(detailRes.body().string());
			}
			catch (JSONException ex) {
				Log.e(DEBUGTAG, "ServiceChanged body is not a json array");
				return;
			}
			int count = objResults.length();
			PtmsData.clearData();
			synchronized (PtmsData.getLockedObject()) {
				for (int i = 0; i < count; i++) {
					JSONObject dataObj = objResults.getJSONObject(i);
					//Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS=" + dataObj.toString());
					PtmsData data = new PtmsData();
					if (!dataObj.has("sip") || !dataObj.has("name") || !dataObj.has("patient"))
						continue;
					JSONObject patientObj;
					if (dataObj.isNull("patient"))
						continue;

					patientObj = dataObj.getJSONObject("patient");

					if ((patientObj == null) || !patientObj.has("name"))
						continue;
					data.sipid = dataObj.getString("sip");
					data.title = dataObj.getString("name");
					data.bedid = dataObj.getString("id");
					data.name = patientObj.getString("name");
					if (patientObj.has("alias"))
						data.alias = patientObj.getString("alias");

					//Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS final name=" + data.name);
					//Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS final sipid=" + data.sipid);
					//Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS final bedid=" + data.bedid);
					//Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS final alias=" + data.alias);
					//Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS final title=" + data.title);
					Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS:"+data.title+","+data.sipid);
					if (data.sipid == null) {
						continue;
					}
					data.sipid = data.sipid.replaceAll("\\(", "").replaceAll("\\)", "");
					Log.d(DEBUGTAG, "ServiceChanged GetBedsForNS sipid=" + data.sipid);
					PtmsData.addData(data.sipid, data);
				}
			}
		}
		catch (Exception ex) {
			Log.e(DEBUGTAG,"ServiceChanged GetBedsForNS ex:", ex);
		}
		Log.d(DEBUGTAG, "ServiceChangedForNS end");
	}


	public void setURL(String uRL) {
		URL = uRL;
	}

}
