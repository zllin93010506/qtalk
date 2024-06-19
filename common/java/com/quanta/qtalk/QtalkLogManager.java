package com.quanta.qtalk;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.FTPUtility;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.SFTPUtility;

import android.os.ConditionVariable;

public class QtalkLogManager implements com.quanta.qtalk.util.Log.LogWriter
{
//    private static final String TAG ="QtalkLogManager";
    private static final String DEBUGTAG = "QTFa_QtalkLogManager";
    private static final DateFormat MSG_DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    private static final DateFormat FILE_DATE_FORMAT = new SimpleDateFormat("yyyyMMddHHmmss");
    private UploadThread mUploadThread = null;
    
    private String mFilePath = null;
    private String mUploadFolder = null;
    public QtalkLogManager(String deviceID,String filePath, String uploadFolder)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"QtalkLogManager => deviceID:"+deviceID+" filePath:"+filePath+" uploadFolder:"+uploadFolder);
        mFilePath = filePath;
        mUploadFolder = uploadFolder;
        mUploadThread = new UploadThread(uploadFolder,deviceID);
    }
    @Override
    public void writeLog(String msg)
    {
    	//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "writeLog:"+msg);
        try{
            File file = new File(mFilePath);
            if(!file.exists())
                file.createNewFile();
            /*else
            {
            	FileInputStream fis;
                fis = new FileInputStream(file);
                int fileLen = fis.available();
                if (fileLen > 3000000)
                {
                	file.delete();
                	file.createNewFile();
                }
            }*/
            FileWriter writer =  new FileWriter(file, true);
            writer.write(MSG_DATE_FORMAT.format(new Date())+"\t"+msg+"\n");
            writer.close();
        }catch(Exception e){
            //Log.e(DEBUGTAG, "writeLog",e);
        }
    }
    @Override
    public void writeLog(String string, Throwable err)
    {
    	//if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "writeLog:"+string);
        try{
            File file = new File(mFilePath);
            if(!file.exists())
                file.createNewFile();
            /*else
            {
            	FileInputStream fis;
                fis = new FileInputStream(file);
                int fileLen = fis.available();
                if (fileLen > 3000000)
                {
                	file.delete();
                	file.createNewFile();
                }
            }*/
            PrintWriter writer =  new PrintWriter(new FileWriter(file, true));
            writer.write(MSG_DATE_FORMAT.format(new Date())+"\t"+string+"\n");
            err.printStackTrace(writer);
            writer.close();
        }catch(Exception e){
            //Log.e(DEBUGTAG, "writeLog",e);
        }
    }
    public synchronized void uploadLog()
    {
        // create a File object for the parent directory
        File tempLogDirectory = new File(mUploadFolder);
        // have the object build the directory structure, if needed.
        if(tempLogDirectory.exists() == false)
            tempLogDirectory.mkdirs();
        
        // get date
        String time = FILE_DATE_FORMAT.format(new Date());
        String file_name =  String.format("%s%06d.log", time, (long)(Math.random()*1000000));
        
        // create a File object for the output file
        File output_file = new File(tempLogDirectory, file_name);
        File tmp_log_file = new File(mFilePath);
        try
        {
            tmp_log_file.renameTo(output_file.getAbsoluteFile());
        }
        catch(Exception e)
        {
            Log.e(DEBUGTAG,"",e);
        }
        tmp_log_file = null;
        output_file = null;
        //notify the upload thread to upload file
        mUploadThread.wakup();
    }
    
    public void startUploadThread()
    {
        //Create Thread to upload 
        mUploadThread.start();
    }
    public void stopUploadThread()
    {
        //Stop the upload thread
        mUploadThread.stop();
    }
    
    public static void disableSftp()
    {
        enableSftp = false;
    }
    
    public static void enableSftp(final String server, final String id, final String password)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>setSftp:"+server+", id:"+id+", password:"+password);
        enableSftp = true;
        if(server != null)
            mFtpServer = server;
        mFtpFolder = SFTP_FOLDER;
        if(id != null)
            mFtpID = id;
        if(password != null)
        {
            mFtpPassword = password;
            //!To verify user
            SFTPUtility.setPassword(mFtpPassword);
        }
    }
    
//    private static final String SFTP_SERVER = "192.168.1.6";
    private static final String SFTP_FOLDER = "qta";
//    private static final String SFTP_ID     = "PX1";
//    private static final String SFTP_PWD    = "PX1";
    private static final String FTP_SERVER  = "60.248.131.22";
    private static final String FTP_FOLDER  = "qta";
    private static final String FTP_ID      = "qtw";
    private static final String FTP_PWD     = "qtw";
    
    //!Initial value
    private static String mFtpServer = FTP_SERVER;
    private static String mFtpFolder = FTP_FOLDER;
    private static String mFtpID = FTP_ID;
    private static String mFtpPassword = FTP_PWD;
    
    private static boolean enableSftp = false;
    
    private class UploadThread implements Runnable
    {
        public boolean mStart = false;
        private final ConditionVariable mWorkEvent = new ConditionVariable();
        public String mFolder = null;
        public String mDeviceID = null;
        public UploadThread(String localFolder,String deviceID)
        {
            mFolder = localFolder;
            mDeviceID = deviceID;
            try
            {
                new File(mFolder).mkdir();
            }catch(Exception e)
            {
                Log.e(DEBUGTAG, "UploadThread",e);
            }
        }
        public void wakup()
        {
            mWorkEvent.open();
        }
        public void start()
        {
            if (!mStart)
            {
                mStart = true;
                mWorkEvent.close();
                new Thread(this).start();
            }
        }
        public void stop()
        {
            if (mStart)
            {
                mStart = false;
                wakup();
            }
        }

        @Override
        public void run()
        {
            while(mStart)
            {
                mWorkEvent.block();
                mWorkEvent.close();
                if (mStart)
                {   
                    try
                    {
                        //create upload folder
                        boolean result = false;
                        if(!enableSftp) 
                        {   //using FTP server
                            result = FTPUtility.makeDirectory(FTP_SERVER, 
                                                              FTP_FOLDER, // path
                                                              FTP_ID, // user name
                                                              FTP_PWD, // password
                                                              mDeviceID); // device ID
                        }
                        else
                        {	//using SFTP server
                            result = SFTPUtility.makeDirectory(mFtpServer, 
                                                               mFtpFolder, // path
                                                               mFtpID, // user name
                                                               mFtpPassword, // password
                                                               mDeviceID); // device ID
                        }
                        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"makeDirectory:"+result);
                        if (result)
                        {
                            //Enum the files in upload folder
                            File folder = new File(mFolder);
                            File[] files = folder.listFiles();
                            for (int i=0;i<files.length;i++)
                            {
                            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"uploadFile:"+files[i].getAbsolutePath());
                                //upload file
                                
                                if(!enableSftp)
                                {	//upload to FTP server
                                    if(FTPUtility.uploadFile(FTP_SERVER, 
                                                             FTP_FOLDER+File.separatorChar+mDeviceID, // path
                                                             FTP_ID, // user name
                                                             FTP_PWD,
                                                             files[i].getAbsolutePath()))
                                    {
                                        files[i].delete();//Delete the file
                                    }
                                }
                                else
                                {	//upload to SFTP server
                                    if(SFTPUtility.uploadFile(mFtpServer,
                                                              mFtpFolder+File.separatorChar+mDeviceID, // path
                                                              mFtpID, // user name
                                                              mFtpPassword,
                                                              files[i].getAbsolutePath()))
                                    {
                                    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "upload file success");
                                        files[i].delete();//Delete the file
                                    }
                                }

                            }
                        }
                    }catch(Exception e)
                    {
                        Log.e(DEBUGTAG,"UploadThread.Run",e);
                    }
                }
            }
        }
    }
}
