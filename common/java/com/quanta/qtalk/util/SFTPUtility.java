package com.quanta.qtalk.util;

import java.io.File;
import java.io.IOException;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Handler;
import com.quanta.qtalk.util.Log;

import com.jcraft.jsch.Channel;
import com.jcraft.jsch.ChannelSftp;
import com.jcraft.jsch.JSch;
import com.jcraft.jsch.Session;
import com.jcraft.jsch.SftpProgressMonitor;
import com.jcraft.jsch.UIKeyboardInteractive;
import com.jcraft.jsch.UserInfo;
import com.quanta.qtalk.ui.ProvisionSetupActivity;

public class SFTPUtility extends Activity
{
//    private static final String TAG = "SFTPUtility";
    private static final String DEBUGTAG = "QTFa_SFTPUtility";
    protected final Handler mHandler = new Handler();
    protected AlertDialog mAlertDialog = null;
//    private static final boolean Debug = false;

    public boolean connect(String user, String host, int port)
    {
        boolean result = false;
        try
        {
            JSch jsch=new JSch();
            Session session=jsch.getSession(user, host, port);
            
            // username and password will be given via UserInfo interface.
            UserInfo ui=new MyUserInfo();
            session.setUserInfo(ui);

            session.connect();

            Channel channel = session.openChannel("sftp");
            channel.connect();
            //ChannelSftp mChannelSftp = (ChannelSftp)mChannel;
            result = true;
        }
        catch (Exception err)
        {
            if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"",err);
        }
        return result;
    }
    
    private static boolean cdDir(ChannelSftp channelSftp, String path)
    {
        boolean result  = false;
        if(ProvisionSetupActivity.debugMode) if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>cdDir:"+path);
        try
        {
            result = channelSftp.cd(path);
        }
        catch (Throwable err)
        {
            if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"cdDir", err);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==cdDir:"+result);
        return result;
    }
    
    private static boolean makeDir(ChannelSftp channelSftp, String path)
    {
        boolean result  = false;
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>makeDir:"+path);
        try
        {
            result = channelSftp.mkdir(path);
        }
        catch (Throwable err)
        {
            if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"makeDir", err);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==makeDir:"+result);
        return result;
    }
    public static boolean makeDirectory(String ftpHost, String ftpDir, String user, String passWord, String DirectoryName) throws Exception
    {
        boolean result = false;
        JSch jsch = null;
        Channel channel = null;
        ChannelSftp channel_sftp = null;
        try
        {
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>makeDirectory");
            String dir = ftpDir.startsWith("/")?ftpDir.substring(1):ftpDir;
            //dir = dir+(dir.endsWith("/")?"":"/");
            jsch=new JSch();
            Session session=jsch.getSession(user, ftpHost, 22);
            //! username and password will be given via UserInfo interface.
            UserInfo ui=new MyUserInfo();
            session.setUserInfo(ui);
            session.connect();
            channel = session.openChannel("sftp");
            channel.connect();
            channel_sftp = (ChannelSftp)channel;
            try{
                if(!cdDir(channel_sftp,dir))
                {
                    makeDir(channel_sftp,dir);
                    cdDir(channel_sftp,dir);
                }
                if(!cdDir(channel_sftp,DirectoryName))
                {
                    makeDir(channel_sftp,DirectoryName);
                    result = cdDir(channel_sftp,DirectoryName);
                }
                else
                    result = true;
            }
            catch(Exception pathErr)
            {
                if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"wrong path", pathErr);
            }
        }
        catch (Exception err)
        {
            if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"",err);
            throw err;
        }
        finally
        { 
             /* disconnect from server 
              * this must be always run */
            if(channel != null)
                channel.disconnect();
        }
        //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==makeDirectory:"+result);
        return result;
    }
    //download file from sftp server,    
	public static String downloadFile(String ftpHost, String ftpDir, String user, String passWord, String fileName) throws IOException//final FtpConnect ftpConnect,final Ftp mFtp,final String fileEx, final String fileNa, final Context context) throws Exception 
    { 
	    String result = null;
        JSch jsch = null;
        Channel channel = null;
        ChannelSftp channel_sftp = null;

        String dir = ftpDir.startsWith("/")?ftpDir.substring(1):ftpDir;
        dir = dir+(dir.endsWith("/")?"":"/");
        try
        {
            jsch=new JSch();
            Session session=jsch.getSession(user, ftpHost, 22);
            
            //! username and password will be given via UserInfo interface.
            UserInfo ui=new MyUserInfo();
            session.setUserInfo(ui);
            session.connect();

            channel = session.openChannel("sftp");
            channel.connect();
            channel_sftp = (ChannelSftp)channel;
            SftpProgressMonitor monitor=new MyProgressMonitor();
            //int mode=ChannelSftp.OVERWRITE;
            int mode=ChannelSftp.RESUME;
            File myTempFile = File.createTempFile(fileName.substring(0, fileName.lastIndexOf(".")), fileName.substring(fileName.lastIndexOf(".")));
            channel_sftp.get( dir+fileName, myTempFile.getAbsolutePath(), monitor, mode);
            result = myTempFile.getAbsolutePath();
        }
        catch (Exception err)
        {
            if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"",err);
            throw err;
        }
        finally
        { 
             /* disconnect from server 
              * this must be always run */
            if(channel != null)
                channel.disconnect();
            return result;
        }
    }
	
	//Upload file to ftp server
    public static boolean uploadFile(String ftpHost, String ftpDir, String user, String passWord, String filePath) throws IOException//final FtpConnect ftpConnect,final Ftp mFtp,final String fileEx, final String fileNa, final Context context) throws Exception 
    { 
        boolean result = false;
        JSch jsch = null;
        Channel channel = null;
        ChannelSftp channel_sftp = null;

        String dir = ftpDir.startsWith("/")?ftpDir.substring(1):ftpDir;
        dir = dir+(dir.endsWith("/")?"":"/");
        //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>uploadFile to:"+ftpDir+", file from:"+filePath);
        try
        {
            jsch=new JSch();
            Session session=jsch.getSession(user, ftpHost, 22);
            
            //! username and password will be given via UserInfo interface.
            UserInfo ui=new MyUserInfo();
            session.setUserInfo(ui);
            session.connect();

            channel = session.openChannel("sftp");
            channel.connect();
            channel_sftp = (ChannelSftp)channel;
            SftpProgressMonitor monitor=new MyProgressMonitor();
            //int mode=ChannelSftp.OVERWRITE;
            int mode=ChannelSftp.RESUME;
            if(filePath != null)
            {
                channel_sftp.put(filePath, ftpDir, monitor, mode);
            }
            //if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"file:"+Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)+File.separatorChar+fileName);
            result = true;
        }
        catch (Exception err)
        {
            if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"",err);
            throw err;
        }
        finally
        { 
             /* disconnect from server 
              * this must be always run */
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==uploadFile:"+result);
            if(channel != null)
                channel.disconnect();
            return result;
        }
    }
    public static String mPassword ="PX1";
    public static void setPassword(String password)
    {
        mPassword = password;
    }
    public static class MyUserInfo implements UserInfo, UIKeyboardInteractive{
        public String getPassword(){ return mPassword; }
        public boolean promptYesNo(String str)
        {
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"promptYesNo:"+str);
            // ask for certification of sftp server, always response yes as true
            return true;
        }
        @Override
        public String[] promptKeyboardInteractive(String destination,
                String name, String instruction, String[] prompt, boolean[] echo)
        {
            return null;
        }
        @Override
        public String getPassphrase()
        {
            return null;
        }
        @Override
        public boolean promptPassword(String message)
        {
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"promptPassword:"+message);
            if(mPassword != null)
                return true;
            else
                return false;
        }
        @Override
        public boolean promptPassphrase(String message)
        {
            return false;
        }
        @Override
        public void showMessage(String message)
        {
            
        }
    }
    public static class MyProgressMonitor implements SftpProgressMonitor
    {
        long count=0;
        long max=0;
        public void init(int op, String src, String dest, long max)
        {
            this.max=max;
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>init src:"+src+ ", dest:"+dest + ", max:"+max);
        }
        private long percent=-1;
        public boolean count(long count)
        {
            this.count+=count;
            if(percent>=this.count*100/max){ return true; }
            percent=this.count*100/max;
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"percent:"+percent);
            return true;
        }
        
        public void end()
        {
//            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"end");
        }
    }
}
