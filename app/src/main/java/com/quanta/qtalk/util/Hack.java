package com.quanta.qtalk.util;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.AudioManager;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Debug;
import android.os.Environment;
import android.os.PowerManager;
import android.util.Base64;
import android.util.TypedValue;

import com.quanta.qtalk.QtalkApplication;
import com.quanta.qtalk.QtalkSettings;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.net.URL;
import java.security.SecureRandom;
import java.security.cert.X509Certificate;
import java.util.Calendar;
import java.util.Comparator;
import java.util.Scanner;
import java.util.regex.Pattern;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public final class Hack
{
	private static final String DEBUGTAG = "QSE_Hack";
	private static int mRecodeSessionID = -1;

	private static String getStringFromFile (String filePath) throws Exception {
		File fl = new File(filePath);
		FileInputStream fin = new FileInputStream(fl);
		String ret = convertStreamToString(fin);
		fin.close();
		return ret;
	}

	private static Context mContext;

	public static void init(Context context) {
		mContext = context;
	}

	public final static boolean isMotorolaAtrix()
	{
		return "MB860".compareToIgnoreCase(Build.MODEL)==0 && "motorola".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean isSamsungI9K()
	{
		return "GT-I9000".compareToIgnoreCase(Build.MODEL)==0 && "samsung".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	//    public final static boolean isSamsungS2()
//    {
//    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "device is Samsung S2:"+ ("GT-I9100".compareToIgnoreCase(Build.MODEL)==0 && "samsung".compareToIgnoreCase(Build.MANUFACTURER)==0));
//        return "GT-I9100".compareToIgnoreCase(Build.MODEL)==0 && "samsung".compareToIgnoreCase(Build.MANUFACTURER)==0;
//    }
//    public final static boolean isHTCnewOne()
//    {
//    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "device is HTC new one:"+ ("HTC One 801e".compareToIgnoreCase(Build.MODEL)==0 && "HTC".compareToIgnoreCase(Build.MANUFACTURER)==0));
//        return "HTC One 801e".compareToIgnoreCase(Build.MODEL)==0 && "HTC".compareToIgnoreCase(Build.MANUFACTURER)==0;
//    }
//    public final static boolean isSupportVGA()
//    {
//        String model = Build.MODEL;
//        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"model:"+model);
//        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "manufacturer:"+Build.MANUFACTURER);
//        boolean result = //model.equalsIgnoreCase("GT-I9100")||       		//              (Galaxy S2)
//                         //model.equalsIgnoreCase("GT-P7510");         		//              (Galaxy 10.1' Tablet)
//                         model.equalsIgnoreCase("Transformer Prime TF201"); //      (Asus Transformer 2)
//
//        return result && ("asus".compareToIgnoreCase(Build.MANUFACTURER)==0);
//        //return result && ("samsung".compareToIgnoreCase(Build.MANUFACTURER)==0 || "asus".compareToIgnoreCase(Build.MANUFACTURER)==0);
//    }
	public final static boolean isSamsungTablet()
	{
		return "GT-P1000".compareToIgnoreCase(Build.MODEL)==0 && "samsung".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean isUI2F()
	{
		return "UI2F".compareToIgnoreCase(Build.MODEL)==0 && "Quanta".compareToIgnoreCase(Build.MANUFACTURER)==0;
		//return "Quanta".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean isNEC()
	{
		return "NEC".compareToIgnoreCase(Build.MANUFACTURER)==0;
		//return "Quanta".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean isQF3()
	{
		return ("QSPT-1951".compareToIgnoreCase(Build.MODEL)==0 || "QF3".compareToIgnoreCase(Build.MODEL)==0)
				&& "intel".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean isQF3a()
	{
		return ("QT-195W-aa2".compareToIgnoreCase(Build.MODEL)==0 || "QF5".compareToIgnoreCase(Build.MODEL)==0);
		//return "QF5".compareToIgnoreCase(Build.MODEL)==0;
	}
	public final static boolean isQF5()
	{
		return ("QT-133W-aa2".compareToIgnoreCase(Build.MODEL) == 0) || ("QT-133W-aa3".compareToIgnoreCase(Build.MODEL) == 0);
	}
	public final static boolean isHTC820()
	{
		return "HTC_D820u".compareToIgnoreCase(Build.MODEL)==0;
	}
	public final static boolean isK72()
	{
		return "K72".compareToIgnoreCase(Build.MODEL)==0 && "rockchip".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean isUI2G()
	{
		return "Quanta".compareToIgnoreCase(Build.MANUFACTURER)==0;
	}
	public final static boolean canHandleRotation()
	{
		if(isPT() || isStation()) return false;
		
		return (isSamsungTablet() || isSamsungI9K() || Build.VERSION.SDK_INT >= 9);
	}
	public final static boolean isTabletDevice(Context context)
	{
		if (android.os.Build.VERSION.SDK_INT >= 11) { // honeycomb
			// test screen size, use reflection because isLayoutSizeAtLeast is only available since 11
			Configuration con = context.getResources().getConfiguration();
			try {
				Method mIsLayoutSizeAtLeast = con.getClass().getMethod("isLayoutSizeAtLeast", int.class);
				Boolean r = (Boolean) mIsLayoutSizeAtLeast.invoke(con, 0x00000004); // Configuration.SCREENLAYOUT_SIZE_XLARGE
				return r;
			} catch (Exception e) {
				Log.e(DEBUGTAG,"",e);
				return false;
			}
		}
		return false;
	}

	public static int getAudioSourceType()
	{
		if(isEnableSWAEC() == true) return MediaRecorder.AudioSource.MIC;
		else return MediaRecorder.AudioSource.VOICE_COMMUNICATION;
	}

	public static int getAudioStreamType() // AudioTrack() && setVolumeControlStream()
	{
		/* QF3 limitation:
		 *       STREAM_VOICE_CALL will cause too loud and cannot control by AudioManager.setStreamVolume()
		 *       USE_DEFAULT_STREAM_TYPE will cause exception, QF3 didn't support this setting
		 * */
		if(isQF3()) return AudioManager.STREAM_SYSTEM;
		else if(isQF3a()) return AudioManager.STREAM_VOICE_CALL;
		else if(isHTC820()) return AudioManager.STREAM_VOICE_CALL;
		else return AudioManager.STREAM_VOICE_CALL;

//    	 if(isEnableSWAEC() == true) return AudioManager.USE_DEFAULT_STREAM_TYPE;
//    	 else return AudioManager.STREAM_VOICE_CALL;
	}

	public static int getAudioMode(boolean isHeadsetPlugin, boolean isLoudeSpeaker) // AudioManager
	{
		return AudioManager.MODE_IN_COMMUNICATION;
//    	 if((isHeadsetPlugin == false) && (isLoudeSpeaker == false))
//    	 	 return AudioManager.MODE_IN_COMMUNICATION;
//
//    	 if(isQF3()) return AudioManager.MODE_NORMAL;
//    	 else if(isQF3a()) return AudioManager.MODE_NORMAL;
//    	 else if(isHTC820()) return AudioManager.MODE_NORMAL;
//    	 else ;
//
//    	 if(isEnableSWAEC() == true) return AudioManager.MODE_NORMAL;
//    	 else return AudioManager.MODE_IN_COMMUNICATION;
	}

	public static int getIntegerFromFile(String filename) throws Exception {
		Scanner input = null;
		int number = 0;

		input = new Scanner(new File(filename));
		while(input.hasNextInt()) {
			number = input.nextInt();
		}
		input.close();
		return number;
	}

	public static boolean isEnableSWAEC()
	{
		int isAndroidAEC = 0;
		try {
			isAndroidAEC = getIntegerFromFile("/sdcard/aec_android.txt");
		} catch (Exception e) {
			// Exception handle
			Log.d("QSE_AEC", "<warning> read aec_android.txt failed");
		}

		if((isAndroidAEC == 0) && (isQF3() || isQF3a() || isK72() || isHTC820() || isQF5())) {
			Log.d("QSE_AEC", "Use Speexdsp AEC/AGC: "+isAndroidAEC);
			return true;
		}
		else {
			Log.d("QSE_AEC", "Use Android AEC/AGC: "+isAndroidAEC);
			return false;
		}
	}

	public static boolean isPhone()
	{
		boolean isQuantaDevices = isQF3() || isQF3a() || isK72() || isQF5() || isStation() || isPT();
		Log.d(DEBUGTAG, "isPhone isQF3: " + isQF3());
		Log.d(DEBUGTAG, "isPhone isQF3a: " + isQF3a());
		Log.d(DEBUGTAG, "isPhone isK72: " + isK72());
		Log.d(DEBUGTAG, "isPhone isQF5: " + isQF5());
		return !isQuantaDevices;
	}

	public static boolean isStation() {
		Log.d(DEBUGTAG, "isStation type="+QtalkSettings.type);
		return QtalkSettings.type.equals("STATION");
	}

	public static boolean isPT() {
		Log.d(DEBUGTAG, "isPT type="+QtalkSettings.type);
		return QtalkSettings.type.equals("PATIENT");
	}

	public static void setRecodeSessionID(int session_id)
	{
		if(session_id < 0) {
			Log.e(DEBUGTAG, "invalid setRecodeSessionID():" + session_id);
		}
		else {
			Log.d(DEBUGTAG, "Build MODEL: " + Build.MODEL);
			Log.d(DEBUGTAG, "setRecodeSessionID():" + session_id);
			mRecodeSessionID = session_id;
		}
	}

	public static int getRecodeSessionID()
	{
		Log.d(DEBUGTAG, "getRecodeSessionID():"+mRecodeSessionID);
		return mRecodeSessionID;
	}


	private static String convertStreamToString(InputStream is) {
		BufferedReader reader = new BufferedReader(new InputStreamReader(is));
		StringBuilder sb = new StringBuilder();
		String line = null;
		try {
			while ((line = reader.readLine()) != null) {
				sb.append(line + "\n");
			}
		} catch (IOException e) {
			Log.e(DEBUGTAG, "convertStreamToString:" + e);
		} finally {
			try {
				is.close();
			} catch (IOException e) {
				Log.e(DEBUGTAG, "convertStreamToString:" + e);
			}
			try {
				reader.close();
			} catch (IOException e) {
				Log.e(DEBUGTAG, "convertStreamToString:" + e);
			}
		}
		return sb.toString();
	}

	@SuppressLint("TrulyRandom")
	private static void disableSSLCertificateChecking() {
		try {
			TrustManager[] trustAllCerts = new TrustManager[] { new X509TrustManager() {
				public X509Certificate[] getAcceptedIssuers() {
					X509Certificate[] myTrustedAnchors = new X509Certificate[0];
					return myTrustedAnchors;
				}

				@Override
				public void checkClientTrusted(X509Certificate[] certs, String authType) {
				}

				@Override
				public void checkServerTrusted(X509Certificate[] certs, String authType) {
				}
			} };

			SSLContext sc = SSLContext.getInstance("SSL");
			sc.init(null, trustAllCerts, new SecureRandom());
			HttpsURLConnection.setDefaultSSLSocketFactory(sc.getSocketFactory());
			HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier() {
				@Override
				public boolean verify(String arg0, SSLSession arg1) {
					return true;
				}
			});
		} catch (Exception e) {
		}
	}

	public static String httpsConnect(String url) throws IOException {
		HttpsURLConnection con = null;
		String result = null;
		try {
			String encoded = new String(Base64.encode(("pta:pta").getBytes(), Base64.NO_WRAP));
			disableSSLCertificateChecking();
			con = (HttpsURLConnection) new URL(url).openConnection();
			con.setConnectTimeout(3000);
			con.setReadTimeout(3000);
			con.setRequestProperty("Accept", "application/json");
			con.setRequestProperty("Content-Type", "application/json");
			con.setRequestProperty("Authorization", "Basic " + encoded);
			con.setRequestProperty("Connection", "close");
			con.connect();

			InputStream instream = con.getInputStream();
			result = convertStreamToString(instream);
			Log.d(DEBUGTAG, "httpsConnect(url), result:" + result);
			instream.close();
			instream = null;
		} catch (Exception ex) {
			Log.e(DEBUGTAG,"",ex);
			throw new IOException("<error> ptms tts failed, ex:" + ex);
		} finally {
			if (con != null) {
				con.disconnect();
				con = null;
			}
		}
		return result;
	}

	public static void startNursingApp(Context context)
	{
    	/*
        PackageManager pmForListener = context.getPackageManager();
        if(pmForListener!=null) {
            Intent launchIntent = pmForListener.getLaunchIntentForPackage("com.quanta.qoca.spt.nursingapp");
			Log.d(DEBUGTAG, "start nursingapp");
            if(launchIntent!=null) {
                launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK);
                context.startActivity(launchIntent);
                Log.d(DEBUGTAG, "start nursingapp: ok");
            }
            else
                Log.d(DEBUGTAG, "start nursingapp: nursingapp may be not installed");
        }
        else
            Log.d(DEBUGTAG, "start nursingapp:getPackageManager fail");
        */
		Log.e(DEBUGTAG, "should not call nursingapp any more");
	}

	public static Bitmap getBitmap(Context context, int resId){
		BitmapFactory.Options options = new BitmapFactory.Options();
		TypedValue value=new TypedValue();
		context.getResources().openRawResource(resId, value);
		options.inTargetDensity = value.density;
		options.inScaled = false;
		return BitmapFactory.decodeResource(context.getResources(), resId, options);
	}

	public static boolean isScreenOn(Context context) {
		PowerManager pm=(PowerManager)context.getSystemService(Context.POWER_SERVICE);
		boolean result;
		if (android.os.Build.VERSION.SDK_INT < 20)
			result = pm.isScreenOn();
		else
			result = pm.isInteractive();
		Log.e(DEBUGTAG, "isScreenOn="+result);
		return result;
	}

	private static PendingIntent mAlarmPendingIntent = null;
	public static void set_alarm (Context context, Class c, String action, int sec) {
		//Log.d(DEBUGTAG, "set_alarm class="+c.getName()+ "action="+action+" sec="+sec);

		Calendar calendar = Calendar.getInstance();
		calendar.add(Calendar.SECOND, sec);

		Intent intent = new Intent(context, c);
		intent.setAction(action);
		intent.putExtra("seconds", sec);

		AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);

		mAlarmPendingIntent = PendingIntent.getBroadcast(context, 1, intent, PendingIntent.FLAG_IMMUTABLE);

		alarmManager.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, calendar.getTimeInMillis(), mAlarmPendingIntent);
		AlarmManager.AlarmClockInfo alarmClockInfo = new AlarmManager.AlarmClockInfo(calendar.getTimeInMillis(), mAlarmPendingIntent);
		alarmManager.setAlarmClock(alarmClockInfo, mAlarmPendingIntent);
	}
	/*
    public static void set_alarm (Context context, Class c, String action, int hour, int min) {
        Log.d(DEBUGTAG, "set_alarm class="+c.getName()+ "action="+action+" at "+hour+":"+min+" tomorrow.");

        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.DATE, 1);
        calendar.set(Calendar.HOUR_OF_DAY, hour);
        calendar.set(Calendar.MINUTE, min);
        calendar.set(Calendar.SECOND, 0);

        Intent intent = new Intent(context, c);
        intent.setAction(action);
        intent.putExtra("hour", hour);
        intent.putExtra("min", min);

        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);

        mAlarmPendingIntent = PendingIntent.getBroadcast(context, 1, intent, 0);

        alarmManager.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, calendar.getTimeInMillis(), mAlarmPendingIntent);
        AlarmManager.AlarmClockInfo alarmClockInfo = new AlarmManager.AlarmClockInfo(calendar.getTimeInMillis(), mAlarmPendingIntent);
        alarmManager.setAlarmClock(alarmClockInfo, mAlarmPendingIntent);
    }
    */
	public static void cancel_alarm(Context context) {
		try {
			if(mAlarmPendingIntent != null) {
				AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
				alarmManager.cancel(mAlarmPendingIntent);
				mAlarmPendingIntent = null;
			}
		} catch (Exception ex) {
			Log.e(DEBUGTAG,"",ex);
		}
	}

	public static boolean isUUID(String uuid)
	{
		String uuid_pattern = "\\p{XDigit}{8}-\\p{XDigit}{4}-\\p{XDigit}{4}-\\p{XDigit}{4}-\\p{XDigit}{12}";
		return Pattern.matches(uuid_pattern, uuid);
	}

	public static boolean setPTMSInfo(String server, String device_id)
	{
		if(isUUID(device_id)) {
			QtalkSettings.ptmsServer = server;
			QtalkSettings.tts_deviceId = device_id;
		}
		else {
			Log.e(DEBUGTAG, "device id format is not match id=" + device_id);
			return false;
		}
		return true;
	}
	static class MemoryInfoSort implements Comparator<Debug.MemoryInfo>
	{
		//以book的ID升序排列
		public int compare(Debug.MemoryInfo a, Debug.MemoryInfo b)
		{
			return b.getTotalPss() - a.getTotalPss();
		}
	}
	public static void dumpAllResource(Context context, String tag) {
		/*
		ActivityManager activityManager = (ActivityManager) context.getSystemService(context.ACTIVITY_SERVICE);
		List<ActivityManager.RunningServiceInfo> serviceList = activityManager.getRunningServices(Integer.MAX_VALUE);

		int size= serviceList.size();
		int[] pids = new int[size];

		for(int i = 0; i < size ; ++i) {
			pids[i] = serviceList.get(i).pid;
		}
		Debug.MemoryInfo[] memoryInfoArray = activityManager.getProcessMemoryInfo(pids);
		Arrays.sort(memoryInfoArray, new MemoryInfoSort());
		Set<Integer> pidSet = new HashSet<Integer>();
		pidSet.add(0);
		Log.d(DEBUGTAG, "==============dump resource start==============");
		for(int i = 0; i < size ; ++i) {
			if(!pidSet.contains(pids[i])) {
				pidSet.add(pids[i]);
				Log.d(DEBUGTAG, String.format("%s: PSS:%4dmB USS:%4dmB TSS:%4dmB  %s(%d)", tag, (memoryInfoArray[i].getTotalPss() / 1024)
						, (memoryInfoArray[i].getTotalPrivateDirty() / 1024), (memoryInfoArray[i].getTotalSharedDirty() / 1024), serviceList.get(i).process, pids[i]));
			}
		}
		pidSet.clear();
		dumpResources(context, tag);
		Log.d(DEBUGTAG, "==============dump resource end==============");*/
	}
	public static void dumpResources(Context context, String tag) {
		ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
		ActivityManager activityManager = (ActivityManager) context.getSystemService(context.ACTIVITY_SERVICE);
		activityManager.getMemoryInfo(mi);
		long availableMem = mi.availMem / 1048576L;
		long totalMem = mi.totalMem / 1048576L;
		Log.d(DEBUGTAG, tag+" available: " + availableMem + "mB, total: " + totalMem + "mB" + getCPUUsage());
	}


	private static float restrictPercentage(float percentage) {
		if (percentage > 100)
			return 100;
		else if (percentage < 0)
			return 0;
		else return percentage;
	}

	static long oldTotal = 0;
	static long oldWork = 0;
	static long oldWorkAM = 0;

	public static String getCPUUsage() {
		BufferedReader reader;
		String[] sa;

		long work=0;
		long total=0;
		long workT=0;
		long totalT=0;
		long workAM = 0;
		long workAMT = 0;
		try {
			if (Build.VERSION.SDK_INT < 26) {
				reader = new BufferedReader(new FileReader("/proc/stat"));
				sa = reader.readLine().split("[ ]+", 9);
				work = Long.parseLong(sa[1]) + Long.parseLong(sa[2]) + Long.parseLong(sa[3]);
				total = work + Long.parseLong(sa[4]) + Long.parseLong(sa[5]) + Long.parseLong(sa[6]) + Long.parseLong(sa[7]);
				reader.close();

				reader = new BufferedReader(new FileReader("/proc/" + android.os.Process.myPid() + "/stat"));
				sa = reader.readLine().split("[ ]+", 18);
				workAM = Long.parseLong(sa[13]) + Long.parseLong(sa[14]) + Long.parseLong(sa[15]) + Long.parseLong(sa[16]);
				reader.close();

				workT -= oldWork;
				totalT -= oldTotal;
				workAMT -= oldWorkAM;
				oldWork = work;
				oldTotal = total;
				oldWorkAM = workAM;
			}
		}catch (Exception ex) {
			//Log.e(DEBUGTAG,"",ex);
		}
		if(totalT==0)
			return "";
		else
			return " cpu usage:"+restrictPercentage(workT * 100 / (float) totalT)+"% "+restrictPercentage(workAMT * 100 / (float) totalT)+"%";

	}
	/*
        public static void execShell(String cmd){
            try {
                Process process = Runtime.getRuntime().exec(cmd);
                BufferedReader bufferedReader = new BufferedReader(
                        new InputStreamReader(process.getInputStream()));

                String line = "";
                Log.d(DEBUGTAG, "============cmd:"+cmd+" start============");
                while ((line = bufferedReader.readLine()) != null) {
                    Log.d(DEBUGTAG, line);
                }
                Log.d(DEBUGTAG, "============cmd:"+cmd+" end============");
            }
            catch (IOException ex) {
                Log.e(DEBUGTAG,"",ex);
            }
        }
    */
	public static void checkLogFolder(Context context){
		try {
			File externalFilesDir = context.getExternalFilesDir(null);
			if (!externalFilesDir.exists()) {
				Log.d(DEBUGTAG,"log folder does not exist, create new: " + externalFilesDir.toString());
				externalFilesDir.mkdir();
			}else{
				Log.d(DEBUGTAG,"log folder exist: " + externalFilesDir.toString());
			}
			File logdir = new File(LogCompress.logStorage);
			if(!logdir.exists()) {
				Log.d(DEBUGTAG, "mkdirs:"+logdir.getAbsolutePath()+":"+logdir.mkdirs());
			}
		} catch (Exception e) {
			Log.d(DEBUGTAG,"create folder error: " + e.toString());
		}
	}
	final static String Storage = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath() +java.io.File.separatorChar
			+"QOCA"+java.io.File.separatorChar;

	final static String ttsInfo = Storage+"ttsInfo";
	public static String getStoragePath() {
		File filePath = new File(Storage);

		if (!filePath.exists()) {
			boolean result = filePath.mkdirs();
			Log.d(DEBUGTAG, "getStoragePath create folder : " + Storage + " result: " + result);
		} else {
			Log.d(DEBUGTAG, "getStoragePath folder : " + Storage + " exist");
		}

		return Storage;
	}
	public static String getTTSFile() {
		return ttsInfo;
	}
}
