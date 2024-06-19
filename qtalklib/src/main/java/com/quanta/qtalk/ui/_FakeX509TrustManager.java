package com.quanta.qtalk.ui;


import com.quanta.qtalk.util.Log;

import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
//import javax.net.ssl.KeyManager;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public class _FakeX509TrustManager implements X509TrustManager {
    private static final String DEBUGTAG = "QTFa__FakeX509TrustManager";
	TrustManager[] trustAllCerts = new TrustManager[] { new X509TrustManager() {
        public void checkClientTrusted(
            X509Certificate[] x509Certificates,
            String string) {       
        }       
        public void checkServerTrusted(X509Certificate[] arg0, String arg1) {
        }       
        public X509Certificate[] getAcceptedIssuers() {
        	return new java.security.cert.X509Certificate[] {};
        }
      } };       
	HostnameVerifier hostnameverifier = new HostnameVerifier(){
        public boolean verify(String hostname, SSLSession session) {
            return true;
        }
	};
	SSLContext context = null;
	HostnameVerifier getHostnameVerifier()
	{
		return this.hostnameverifier;
	}
	SSLContext getSSLContext()
	{
		return this.context;
	}
	
    private static final X509Certificate[] _AcceptedIssuers = new X509Certificate[] {};

    public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException { }

    public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException { }

    public boolean isClientTrusted(X509Certificate[] chain) {
            return true;
    }

    public boolean isServerTrusted(X509Certificate[] chain) {
            return true;
    }

    public X509Certificate[] getAcceptedIssuers() {
            return _AcceptedIssuers;
    }
    public _FakeX509TrustManager(){
    	try {
            context = SSLContext.getInstance("TLS");
            context.init(null, trustAllCerts, new SecureRandom());
            HttpsURLConnection.setDefaultSSLSocketFactory(context.getSocketFactory());
            HttpsURLConnection.setDefaultHostnameVerifier(this.hostnameverifier);
	    } catch (NoSuchAlgorithmException e) {
	            Log.e(DEBUGTAG,"",e);
	    } catch (KeyManagementException e) {
	            Log.e(DEBUGTAG,"",e);
	    }
    }
}

