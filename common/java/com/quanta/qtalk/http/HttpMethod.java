package com.quanta.qtalk.http;

import android.content.Context;

import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.ui.CommandService;

import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;

import okhttp3.Credentials;
import okhttp3.Headers;
import okhttp3.Interceptor;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class HttpMethod {
    private static final String TAG = "HttpMethod-";

    private final OkHttpClient mOkHttpClient = HttpClient.trustAllSslClient(new OkHttpClient());
    private final OkHttpClient mOkHttpTokenClient = HttpClient.trustAllSslTokenClient(new OkHttpClient());
    private final MediaType JSON = MediaType.parse("application/json; charset=utf-8");

    private final Logger log = LoggerFactory.getLogger(HttpMethod.class);

    public static class BasicAuthInterceptor implements Interceptor {
        private final String credentials;

        public BasicAuthInterceptor(String user, String password) {
            this.credentials = Credentials.basic(user, password);
        }

        @Override
        public Response intercept(Chain chain) throws IOException {
            Request request = chain.request();
            Request authenticatedRequest = request.newBuilder()
                    .header("Authorization", credentials).build();
            return chain.proceed(authenticatedRequest);
        }
    }
    public static class TokenAuthInterceptor implements Interceptor {
        Logger log = LoggerFactory.getLogger(TokenAuthInterceptor.class);
        Context mContext;
        public static String mToken;

        @Override
        public Response intercept(Interceptor.Chain chain) throws IOException {
            log.debug("intercept begin");
            if(mToken == null) {
                HttpMethod.getToken();
                log.debug("Get Token:" + mToken);
            }
            Request request = chain.request();
            Request authenticatedRequest = request.newBuilder()
                    .header("Authorization", "Token " + mToken).build();
            Response response = chain.proceed(authenticatedRequest);
            if(!response.isSuccessful()) {
                log.debug("response code:"+response.code());
                request = chain.request();
                authenticatedRequest = request.newBuilder()
                        .header("Authorization", Credentials.basic("pta", "pta")).build();
                response = chain.proceed(authenticatedRequest);
            }
            return response;
        }
    }
    private static void getToken() {
        Logger log = LoggerFactory.getLogger(HttpMethod.class);
        Response response;
        String nfcAuthToken;
        String ptmsip = QtalkSettings.ptmsServer;
        String id;
        if(ptmsip == null || "".equals(ptmsip))
            return;
        if(CommandService.qtalk_settings == null)
            return;
        id = CommandService.qtalk_settings.provisionID;
        if(id == null || "".equals(id))
            return;
        String mTokenUrl = "https://" + ptmsip + "/api/search/users";

        HttpMethod http = new HttpMethod();

        try {
            mTokenUrl = mTokenUrl + "?username=pta";
            log.debug( "Get token url: "+mTokenUrl);

            response = http.getRequest(mTokenUrl);
            log.debug( "Get token response code = " + response.code());
            if (response.code() == 200) {
                JSONObject resp = new JSONObject(http.inputStreamToString(response.body().byteStream()));
                nfcAuthToken = resp.getString("auth_token");
                TokenAuthInterceptor.mToken = nfcAuthToken;
                //log.debug( "Get Token by username:" + resp.getString("username"));
                //log.debug( "Get Token" + TokenAuthInterceptor.mToken);
            } else {
                log.debug( "nfc get token response code = " + response.code());
            }
        } catch (JSONException e) {
            log.debug( "JSONException : " + e.toString());
        } catch (IOException e) {
            log.debug( "IOException : " + e.toString());
        }
    }
    public Response getRequestTokenAuth(String url) {
        //log.debug(TAG + "getRequest url:" + url);

        try {
            Request request = new Request.Builder()
                    .addHeader("Accept", "application/json")
                    .addHeader("Content-Type", "application/json")
                    .url(url)
                    .build();

            Response response = mOkHttpTokenClient.newCall(request).execute();
            log.debug(TAG +  "getRequest-"+ url +"res:" + response.code());
            if (!response.isSuccessful()) {
                throw new IOException("Unexpected code " + response);
            }

            return response;
        } catch (Exception ex) {
            log.error(TAG+"getRequest-"+oneLineStackTrace(ex));
        }

        return null;
    }
    public Response getRequest(String url) {
        //log.debug(TAG + "getRequest url:" + url);

        try {
            Request request = new Request.Builder()
                    .addHeader("Accept", "application/json")
                    .addHeader("Content-Type", "application/json")
                    .url(url)
                    .build();

            Response response = mOkHttpClient.newCall(request).execute();
            if(!url.contains("service=phonebook&action=check"))
                log.debug(TAG +  "getRequest-"+ url +"res:" + response.code());
            if (!response.isSuccessful()) {
                throw new IOException("Unexpected code " + response);
            }

            return response;
        } catch (Exception ex) {
            log.error(TAG+"getRequest-"+oneLineStackTrace(ex));
        }

        return null;
    }

    public Response deleteRequest(String url) {
        //log.debug(TAG + "deleteRequest url:" + url);
        try {
            Request request = new Request.Builder()
                    .addHeader("Accept", "application/json")
                    .addHeader("Content-Type", "application/json")
                    .url(url)
                    .delete()
                    .build();

            Response response = mOkHttpClient.newCall(request).execute();
            log.debug(TAG +  "deleteRequest-"+ url +"res:" + response.code());
            if (!response.isSuccessful()) {
                throw new IOException("Unexpected code " + response);
            }
            return response;
        } catch (Exception ex) {
            log.error(TAG+"deleteRequest-"+oneLineStackTrace(ex));
        }
        return null;
    }

    public boolean updateRequest(JSONObject payload, String url) {
        //log.debug(TAG + "updateRequest url:" + url);
        RequestBody body = RequestBody.create(JSON, payload.toString());

        try {
            Request request = new Request.Builder()
                    .addHeader("Accept", "application/json")
                    .addHeader("Content-Type", "application/json")
                    .url(url)
                    .patch(body)
                    .build();

            Response response = mOkHttpClient.newCall(request).execute();
            log.debug(TAG + "updateRequest-"+ url +"res:" + response.code());
            if (!response.isSuccessful()) {
                throw new IOException("Unexpected code " + response);
            }
            else {
                return true;
            }
        } catch (Exception ex) {
            log.error(TAG+"updateSensorRequest-"+oneLineStackTrace(ex));
            return false;
        }
    }

    public static String oneLineStackTrace(Throwable ex) {
        StringWriter errors = new StringWriter();
        ex.printStackTrace(new PrintWriter(errors));
        return errors.toString().replaceAll("\n", " ");
    }
    public boolean postRequest(JSONObject payload, String url) throws Exception {
        //log.debug(TAG + "postRequest url:" + url);
        RequestBody body = RequestBody.create(JSON, payload.toString());
        log.debug(TAG + "body:" + body);
        Request request = new Request.Builder()
                .addHeader("Accept", "application/json")
                .addHeader("Content-Type", "application/json")
                .url(url)
                .post(body)
                .build();
        try {
            Response response = mOkHttpClient.newCall(request).execute();
            if(response.code() == 201) {
                if(response.body() != null)
                {
                    JSONObject res = new JSONObject(response.body().string());
                    res.put("status", response.code());
                    return true;
                }
            } else {
                if(response.body() == null)
                    throw new Exception("status="+response.code());
                else
                    throw new Exception(response.body().string());
            }
        } catch (IOException ex) {
            log.error(TAG+"postRequest 2-"+oneLineStackTrace(ex));
        } catch (JSONException ex) {
            log.error(TAG+"postRequest 3-"+oneLineStackTrace(ex));
        }
        return false;
    }
    public String inputStreamToString(InputStream stream) throws IOException {
        BufferedReader bufferedReader = new BufferedReader( new InputStreamReader(stream));
        String line = "";
        String result = "";
        while((line = bufferedReader.readLine()) != null)
            result += line;

        stream.close();

        return result;
    }
}

