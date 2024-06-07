package com.quanta.qtalk.provision;

//import java.util.Hashtable;
import java.util.List;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.net.NetworkInfo.State;
import com.quanta.qtalk.util.Log;

import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;
import com.quanta.qtalk.ui.ProvisionSetupActivity;


public class PackageChecker 
{
//    private static final boolean isDebug = true;
    static final String TAG_VERSION_NAME                = "version_name";
    static final String TAG_VERSION_CODE                = "version_code";
    static final String TAG_VERSION_INFO                = "version_info";
    static final String TAG_ANDROID                     = "android";
    static final String TAG_CALL_SERVICE                = "call_service";
    static final String TAG_X264                        = "x264";
    static final String TAG_QTALK                       = "qtalk";
    private static final String DEBUGTAG = "QTFa_PackageChecker";
    
    public static class AppInfo {
        String appname          = "";
        String packageName      = "";
        String versionName      = "";
        int versionCode         = 0;
        String link             = "";
        public String getVersionName()
        {
            return this.versionName;
        }
        public int getVersionCode()
        {
            return this.versionCode;
        }
        public String getAppName()
        {
            return this.appname;
        }
    }
    
    public static class DecryptedInfo {
        String serverName = null;
        String phoneBookLink = null;

    }
    private static List<AppInfo>    mCheckReleaseResult = null; //app information list in the release information from server
    //private static final Hashtable<String,AppInfo> mAppInfoTable = new Hashtable<String,AppInfo>();
    private static ProvisionInfo mProvisionInfo = null;
    
    public static void setProvisionInfo(ProvisionInfo info)
    {
        mProvisionInfo = info;
    }
    
    public static ProvisionInfo getProvisionInfo()
    {
        return mProvisionInfo;
    }
    


    public static void clearReleaseInfo()
    {
        if(mCheckReleaseResult != null)
            mCheckReleaseResult = null;
    }
    
    public static AppInfo getInstalledQTInfo(Context context)
    {
        try
        {
            return getInstalledAppInfo(PackageParas.QTALK_PACKAGE_NAME,context);
        } catch (NameNotFoundException e)
        {
            Log.e(DEBUGTAG,"getInstalledQTInfo ", e);
        }
        return null;
    }
    public static AppInfo getInstalledCallEngineInfo(Context context)
    {
        try
        {
            return getInstalledAppInfo(PackageParas.CALL_ENGINE_PACKAGE_NAME,context);
        } catch (NameNotFoundException e)
        {
            Log.e(DEBUGTAG,"getInstalledCallEngineInfo ", e);
        }
        return null;
    }
    public static AppInfo getInstalledX264Info(Context context)
    {
//        try
//        {
//            return getInstalledAppInfo(PackageParas.X264_PACKAGE_NAME,context);
//        } catch (NameNotFoundException e)
//        {
//            Log.e(DEBUGTAG,"getInstalledX264Info ", e);
//        }
        return null;
    }
    
    
    public static boolean isCallEngineInstalled(Context context)
    {
        return true;//isInstalled(context,PackageParas.CALL_ENGINE_PACKAGE_NAME);
    }
    public static boolean isX264Installed(Context context)
    {
        return isInstalled(context,PackageParas.X264_PACKAGE_NAME);
    }
    public static boolean isQTInstalled(Context context)
    {
        return isInstalled(context,PackageParas.QTALK_PACKAGE_NAME);
    }
    private static boolean isInstalled(Context context, String packageName)
    {
        boolean result = false;
        try
        {
            result = getInstalledAppInfo(packageName,context)!=null;
        }catch(Exception err)
        {}
        return result;
    }
    public static boolean isCallEngineUpdate(Context context)
    {
        return false;//isUpdateNeed(context,PackageParas.CALL_ENGINE_PACKAGE_NAME);
    }
    /*public static boolean isX264ServiceUpdate(Context context)
    {
        return isUpdateNeed(context,PackageParas.X264_PACKAGE_NAME);
    }
    public static boolean isQtalkUpdate(Context context)
    {
        return isUpdateNeed(context,PackageParas.QTALK_PACKAGE_NAME);
    }*/
    /*public static boolean isUpdateNeed(Context context, String packageName)
    {
        boolean result = false;
        AppInfo install_info = null;
        try
        {
            install_info = getInstalledAppInfo(packageName,context);
        }catch(Exception err)
        {}
        if (install_info==null)  //not install
            result = true;
        else                    //installed, then check version
        {
            AppInfo release_info = mAppInfoTable.get(packageName);
            if(release_info != null)
                result = compareVersion(install_info, release_info);
        }
        return result;
    }*/
    /*private static boolean compareVersion(AppInfo installed_info, AppInfo release_info)
    {
        boolean result = false;
        float install_version_name = Float.valueOf(installed_info.versionName);
        float release_version_name  = Float.valueOf(release_info.versionName);
        if(release_version_name > install_version_name)
        {	
            result = true;
        }
        else if(release_version_name == install_version_name)
        {
            int install_version_code = Integer.valueOf(installed_info.versionCode);
            int release_version_code  = Integer.valueOf(release_info.versionCode);
            if(release_version_code > install_version_code)
            {
                result = true;
            }
        }
        return result;
    }*/
    
    /*public static boolean compareVersion(AppInfo installedInfo, String savedVersionNumber)
    {
        boolean result = false;
        float install_version_name = Float.valueOf(installedInfo.versionName);
        Log.d(TAG,"==>compareVersion");
        //Log.d(TAG,"saved_version_name:"+savedVersionNumber.substring(0,savedVersionNumber.lastIndexOf(".")));
        
        //Log.d(TAG,"saved_version_code:"+savedVersionNumber.substring(savedVersionNumber.lastIndexOf(".")+1));
        
        float saved_version_name  = Float.valueOf(savedVersionNumber.substring(0,savedVersionNumber.lastIndexOf(".")));
        int saved_version_code = Integer.valueOf(savedVersionNumber.substring(savedVersionNumber.lastIndexOf(".")+1));
        Log.d(TAG,"saved_version:"+saved_version_name+"."+saved_version_code);
        Log.d(TAG,"install version:"+installedInfo.versionName+"."+installedInfo.versionCode);
        if(saved_version_name < install_version_name)
        { 
            result = true;
        }
        else if(saved_version_name == install_version_name)
        {
            int install_version_code = Integer.valueOf(installedInfo.versionCode);
            if(saved_version_code <= install_version_code)
            {
                result = true;
            }
        }
        Log.d(TAG,"<==compareVersion");
       
        return result;
    }*/
    public static AppInfo getInstalledAppInfo(String packageName, Context context) throws NameNotFoundException
    {
        AppInfo result = new AppInfo();
        PackageInfo package_info;
        package_info = context.getPackageManager().getPackageInfo(packageName,0);
        result.appname = package_info.applicationInfo.loadLabel(context.getPackageManager()).toString();
        result.packageName = package_info.packageName;
        result.versionName = package_info.versionName;
        result.versionCode = package_info.versionCode;
        return result;
    }
    
    public static boolean isOnline(Context context) //throws Exception
    {
        boolean result = false;
        ConnectivityManager con_manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        //check if there is any active network device and check its info
        if (con_manager != null &&
            con_manager.getActiveNetworkInfo() != null &&
            con_manager.getActiveNetworkInfo().isConnectedOrConnecting()) 
        {
            // if network device is active, check if wifi or 3g is working,
            // check wifi first, and then check 3G if wifi is not active.
            // neighter wifi nor 3g is active, keep connection result false.
            State state_wifi = State.DISCONNECTED;
            State state_mobile = State.DISCONNECTED;
            try{
                state_wifi = con_manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI).getState();
                if(state_wifi == State.CONNECTED)
                    result = true;
            }
            catch(Exception err_wifi)
            {
                try{
                    state_mobile = con_manager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE).getState();
                }
                catch (Exception err_mobile)
                {
                    if(state_mobile == State.CONNECTED )
                        result = true;
                }
            }
        }
        return result;
    }
    
    public static void CheckInstallX264(Context context)
    {
    	boolean X264Installed;
    	final Context Context = context;
    	X264Installed = PackageChecker.isX264Installed(context);
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"X264Installed="+X264Installed);
    	if (X264Installed == false)
    	{
    		new AlertDialog.Builder(context)
			.setMessage("您必須安裝QOCA Player 解壓包,才能正常使用QOCA視訊")
			.setPositiveButton("確定", new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// TODO Auto-generated method stub
					String appName = "org.x264.service";
					try
	        	    {
	        	        // Open app with Google Play app
	        	    	Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id="+appName));
	        	    	Context.startActivity(intent);
	        	  //  	((Activity) Context).finish();
	        	    }
	        	    catch (android.content.ActivityNotFoundException anfe)
	        	    {
	        	        // Open Google Play website
	        	    	Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://play.google.com/store/apps/details?id="+appName));
	        	    	Context.startActivity(intent);
	        	   // 	Context.finish();
	        	    }
				}
			})
			.show();
    	}
    }
/*    public static String installQTPackage(Context context)
    {
        try
        {
            return installPackageViaFTP(PackageParas.QTALK_PACKAGE_NAME, context);
        } catch (Exception e)
        {
            Log.e(TAG,"installQTPackage ",e);
        }
        return null;
    }
    public static String installCallEnginePackage(Context context)
    {
        try
        {
            return installPackageViaFTP(PackageParas.CALL_ENGINE_PACKAGE_NAME, context);
        } catch (Exception e)
        {
            Log.e(TAG,"installCallEnginePackage ",e);
        }
        return null;
    }
    public static String installX264Package(Context context)
    {
        try
        {
            return installPackageViaFTP(PackageParas.X264_PACKAGE_NAME, context);
        } catch (Exception e)
        {
            Log.e(TAG,"installX264Package ",e);
        }
        return null;
    }
    
    private static String installPackageViaFTP(String packageName,Context context) throws IOException
    {
        AppInfo app_info = mAppInfoTable.get(packageName);
        if(isDebug)Log.d(TAG,"installPackageViaFTP:"+packageName +", link:"+app_info.link);
        if(mProvisionInfo != null && app_info!=null) 
        {
            String file_path = FTPUtility.downloadFile(mProvisionInfo.ftpServer,
                                    app_info.link.substring(0,app_info.link.lastIndexOf("/")+1), 
                                    mProvisionInfo.ftpUserName, 
                                    mProvisionInfo.ftpPassWord, 
                                    app_info.link.substring(app_info.link.lastIndexOf("/")+1));
            if(file_path != null)
                FileUtility.openFile(new File(file_path),context);
            return file_path;
        }
        return null;
    }
    
    public static Hashtable<String,AppInfo> getAppInfoTable()
    {
        return mAppInfoTable;
    }
    public static Hashtable<String,AppInfo> parsePackageVersionInfo(String packageName,String versionInfoXML) throws ParserConfigurationException, SAXException, IOException
    {
        if(isDebug)Log.d(TAG,"==>parsePackageVersionInfo:"+ "packageName:"+packageName +", file"+versionInfoXML);
        if(versionInfoXML != null)
        {
            DocumentBuilder documentBuilder  = null;
            Document document        = null;
            NodeList elements        = null;
            int iElementLength= 0;
            documentBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
            document = documentBuilder.parse(new File(versionInfoXML));
            document.getDocumentElement().normalize(); 
            
            elements  = document.getElementsByTagName(TAG_VERSION_INFO);
            iElementLength = elements.getLength();
            if (iElementLength == 0 )
                return null;
            //parse version information
            for (int i = 0; i < iElementLength ; i++) 
            {
                Node node = elements.item(i);
                NodeList properties = node.getChildNodes();
                AppInfo app_info = new AppInfo();
                for(int j = 0; j< properties.getLength(); j++)
                {
                    Node properties_node = properties.item(j);
                    String properties_node_name = properties_node.getNodeName();
                    if(properties_node_name.equalsIgnoreCase(TAG_VERSION_NAME))
                    {
                        String version_name = properties_node.getFirstChild().getNodeValue();
                        if(isDebug)Log.d(TAG,"VERSION_NAME: "+version_name);
                        app_info.versionName = version_name;
                    }
                    else if(properties_node_name.equalsIgnoreCase(TAG_VERSION_CODE))
                    {
                        String version_code = properties_node.getFirstChild().getNodeValue();
                        if(isDebug)Log.d(TAG,"VERSION_CODE: "+version_code);
                        app_info.versionCode = Integer.valueOf(version_code);
                    }
                    if(mProvisionInfo != null )
                    {
                        if(packageName == PackageParas.QTALK_PACKAGE_NAME)
                            app_info.link = mProvisionInfo.qtalkPath;
                        if(packageName == PackageParas.CALL_ENGINE_PACKAGE_NAME)
                            app_info.link = mProvisionInfo.callEnginePath;
                        if(packageName == PackageParas.X264_PACKAGE_NAME)
                            app_info.link = mProvisionInfo.x264Path;
                    }
                    else
                        Log.e(TAG,"mProvisionInfo==null");
                }
                app_info.packageName = packageName;
                mAppInfoTable.put(app_info.packageName, app_info);
            }
        }
        else
            Log.e(TAG,"parseVersionInfo invalid input path = "+versionInfoXML);
        if(isDebug)Log.d(TAG,"<==parseVersionInfo");
        return mAppInfoTable;
    }
    
    public static Hashtable<String,AppInfo> parseVersionInfo(String versionInfoXML) throws ParserConfigurationException, SAXException, IOException
    {
        if(isDebug)Log.d(TAG,"==>parseVersionInfo:"+versionInfoXML);
        if(versionInfoXML != null)
        {
            DocumentBuilder documentBuilder  = null;
            Document document        = null;
            NodeList elements        = null;
            int iElementLength= 0;
            documentBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
            document = documentBuilder.parse(new File(versionInfoXML));
            document.getDocumentElement().normalize(); 
            
            elements  = document.getElementsByTagName(TAG_ANDROID);
            iElementLength = elements.getLength();
            if (iElementLength == 0 )
                return null;
            //parse version information for Qtalk
            for (int i = 0; i < iElementLength ; i++) 
            {
                Node node = elements.item(i);
                NodeList properties = node.getChildNodes();
                for(int j = 0; j< properties.getLength(); j++)
                {
                    Node properties_node = properties.item(j);
                    NodeList property_list = properties_node.getChildNodes();
                    String properties_node_name = properties_node.getNodeName();
                    if(properties_node_name.equalsIgnoreCase(TAG_QTALK))
                    {
                        AppInfo app_info = new AppInfo();
                        app_info.packageName = PackageParas.QTALK_PACKAGE_NAME;
                        for(int k = 0 ; k < property_list.getLength() ; k++)
                        {
                            Node property_list_node = property_list.item(k);
                            String name = property_list_node.getNodeName();
                            if(name.equalsIgnoreCase(TAG_VERSION_NAME))
                            {
                                String version_name = property_list_node.getFirstChild().getNodeValue();
                                if(isDebug)Log.d(TAG,"VERSION_NAME: "+version_name);
                                app_info.versionName = version_name;
                            }
                            else if(name.equalsIgnoreCase(TAG_VERSION_CODE))
                            {
                                String version_code = property_list_node.getFirstChild().getNodeValue();
                                if(isDebug)Log.d(TAG,"VERSION_CODE: "+version_code);
                                app_info.versionCode = Integer.valueOf(version_code);
                            }
                            if(mProvisionInfo != null && mProvisionInfo.qtalkPath != null)
                                app_info.link = mProvisionInfo.qtalkPath;
                            else
                                Log.e(TAG,"mProvisionInfo.qtalkPath==null");
                            mAppInfoTable.put(app_info.packageName, app_info);
                        }
                    }
                    
                    if(properties_node_name.equalsIgnoreCase(TAG_CALL_SERVICE))
                    {
                        AppInfo app_info = new AppInfo();
                        app_info.packageName = PackageParas.CALL_ENGINE_PACKAGE_NAME;
                        for(int k = 0 ; k < property_list.getLength() ; k++)
                        {
                            Node property_list_node = property_list.item(k);
                            String name = property_list_node.getNodeName();
                            if(name.equalsIgnoreCase(TAG_VERSION_NAME))
                            {
                                String version_name = property_list_node.getFirstChild().getNodeValue();
                                if(isDebug)Log.d(TAG,"VERSION_NAME: "+version_name);
                                app_info.versionName = version_name;
                            }
                            else if(name.equalsIgnoreCase(TAG_VERSION_CODE))
                            {
                                String version_code = property_list_node.getFirstChild().getNodeValue();
                                if(isDebug)Log.d(TAG,"VERSION_CODE: "+version_code);
                                app_info.versionCode = Integer.valueOf(version_code);
                            }
                            if(mProvisionInfo != null && mProvisionInfo.callEnginePath != null)
                                app_info.link = mProvisionInfo.callEnginePath;
                            else
                                Log.e(TAG,"mProvisionInfo.callServicePath==null");
                            mAppInfoTable.put(app_info.packageName, app_info);
                        }
                    }
                    
                    if(properties_node_name.equalsIgnoreCase(TAG_X264))
                    {
                        AppInfo app_info = new AppInfo();
                        app_info.packageName = PackageParas.X264_PACKAGE_NAME;
                        for(int k = 0 ; k < property_list.getLength() ; k++)
                        {
                            Node property_list_node = property_list.item(k);
                            String name = property_list_node.getNodeName();
                            if(name.equalsIgnoreCase(TAG_VERSION_NAME))
                            {
                                String version_name = property_list_node.getFirstChild().getNodeValue();
                                if(isDebug)Log.d(TAG,"VERSION_NAME: "+version_name);
                                app_info.versionName = version_name;
                            }
                            else if(name.equalsIgnoreCase(TAG_VERSION_CODE))
                            {
                                String version_code = property_list_node.getFirstChild().getNodeValue();
                                if(isDebug)Log.d(TAG,"VERSION_CODE: "+version_code);
                                app_info.versionCode = Integer.valueOf(version_code);
                            }
                            if(mProvisionInfo != null && mProvisionInfo.x264Path != null)
                                app_info.link = mProvisionInfo.x264Path;
                            else
                                Log.e(TAG,"mProvisionInfo.x264Path==null");
                            mAppInfoTable.put(app_info.packageName, app_info);
                        }
                    }
                }
            }
        }
        else
            Log.e(TAG,"parseVersionInfo invalid input path = "+versionInfoXML);
        if(isDebug)Log.d(TAG,"<==parseVersionInfo");
        return mAppInfoTable;
    }
    
*/
    //--- download from http server and install package START---//
//    public static boolean installPackage(String packageName, Context context,ICheckDependence cb) throws Exception
//    {
//        boolean result = true;
//        String strURL = null;
//        String fileEx = null;
//        String fileNa = null;
//        // get AppInfo from temp AppInfo List
//        AppInfo info = mAppInfoTable.get(packageName);
//        strURL = info.link;
//        fileEx = strURL.substring(strURL.lastIndexOf(".")+1,strURL.length()).toLowerCase();
//        fileNa = strURL.substring(strURL.lastIndexOf("/")+1,strURL.lastIndexOf("."));
//        if(isDebug)Log.d(TAG,"download " +strURL + ", packageName:"+packageName);
//        if (cb!=null)
//            cb.onDownloadPackage(packageName);
//        try
//        {
//            // download package with thread.
//            // then callBack and UI will not be blocked.
//            getFile(cb,strURL,fileEx,fileNa,context);
//        } catch (Exception e)
//        {
//            Log.e(TAG,"installPackage: ",e );
//        }
//        return result;
//    }
//    
//    private static void getFile(final ICheckDependence cb,final String strPath, final String fileEx, final String fileNa, final Context context) throws Exception 
//    { 
//        try
//        {  
//            Runnable r = new Runnable()
//            {
//                public void run()
//                {
//                    getDataSource(cb,strPath, fileEx, fileNa ,context); 
//                } 
//            };   
//            new Thread(r).start();
//        } 
//        catch(Exception e) 
//        { 
//            Log.e(TAG,"getFile -2 :",e);
//        }
//    } 
//  
//    private static void getDataSource(ICheckDependence cb,String strPath, String fileEx, String fileNa, Context context) 
//    { 
//        if (!URLUtil.isNetworkUrl(strPath)) 
//        { 
//          Log.e(TAG,"wrong URL"); 
//          if (cb!=null)
//              cb.onDownloadPackageFinished(null);
//        } 
//        else 
//        {
//            try{
//                URL myURL = new URL(strPath); 
//                URLConnection conn = myURL.openConnection();   
//                conn.setConnectTimeout(1000);
//                conn.connect();
//                InputStream is = conn.getInputStream();   
//                if (is == null) 
//                { 
//                    throw new RuntimeException("stream is null"); 
//                } 
//                File myTempFile = File.createTempFile(fileNa, "."+fileEx); 
//                FileOutputStream fos = new FileOutputStream(myTempFile); 
//                byte buf[] = new byte[128];
//                int total_load = 0;
//                int file_length = conn.getContentLength();
//                do 
//                {   
//                    int numread = is.read(buf);
//                    total_load += numread;
//                    if (cb!=null)
//                        cb.onDownloadProgress(total_load*100/file_length);
//                    if (numread <= 0) 
//                    { 
//                        break; 
//                    } 
//                    fos.write(buf, 0, numread);
//                }while (true);
//                openFile(myTempFile, context);
//                if (cb!=null)
//                    cb.onDownloadPackageFinished(myTempFile.getAbsolutePath());
//                try 
//                { 
//                    is.close(); 
//                } 
//                catch (Exception ex) 
//                { 
//                    Log.e(TAG,"getDataSource ",ex);
//                }
//            }
//            catch(Exception err)
//            {
//                Log.e(TAG,"getDataSource 2",err);
//                if (cb!=null)
//                    cb.onDownloadPackageFinished(null);
//            }
//        }
//    }
    
    //--- download from http server and install package END---//
}
