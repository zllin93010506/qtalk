package com.quanta.qtalk.util;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import com.quanta.qtalk.ui.ProvisionSetupActivity;

import cz.dhl.ftp.Ftp;
import cz.dhl.ftp.FtpConnect;
import cz.dhl.ftp.FtpFile;
import cz.dhl.ftp.FtpInputStream;
import cz.dhl.ftp.FtpOutputStream;

public class FTPUtility
{
//    private static final String TAG = "FTPUtility";
    private static final String DEBUGTAG = "QTFa_FTPUtility";
    public static boolean makeDirectory(String DirectoryName) throws IOException
    {
        boolean result = false;
        /*
        result = makeDirectory("60.248.131.22", 
                               "qta", // path
                               "qtw", // user name
                               "qtw", // password
                               DirectoryName);*/
        result = makeDirectory("", 
                "", // path
                "", // user name
                "", // password
                DirectoryName);
        
        return result;
    }
    public static boolean makeDirectory(String ftpHost, String ftpDir, String user, String passWord, String DirectoryName) throws IOException
    {
        boolean result = false;
        FtpConnect ftp_connect = null;
        Ftp ftp = null;

        ftp_connect = new FtpConnect();
        ftp_connect.setHostName(ftpHost);
        //The dir path format must be path/xxx/
        String dir = ftpDir.startsWith("/")?ftpDir.substring(1):ftpDir;
        dir = dir+(dir.endsWith("/")?"":"/");
        ftp_connect.setPathName(dir);
        ftp_connect.setUserName(user);
        ftp_connect.setPassWord(passWord);
        //Log.d("ST",ftpHost+" "+dir);
        ftp = new Ftp();
        try
        {
            boolean is_connected = ftp.connect(ftp_connect);
            if(!ftp.cd(DirectoryName))
                result = ftp.mkdir(DirectoryName);
            else
                result = true;
            //Log.d(DEBUGTAG,"current position:"+ftp.pwd());
        }
        catch(IOException err)
        {

            /* disconnect from server 
             * this must be always run */
            ftp.disconnect();
            throw err;
        }
        finally
        { 
             /* disconnect from server 
              * this must be always run */
             ftp.disconnect();
        }
        //Log.d(DEBUGTAG, "result:"+result);
        return result;
    }
    //download file from ftp server,    
	public static String downloadFile(String ftpHost, String ftpDir, String user, String passWord, String fileName) throws IOException//final FtpConnect ftpConnect,final Ftp mFtp,final String fileEx, final String fileNa, final Context context) throws Exception 
    { 
        String file_path = null; 
        FtpInputStream is = null;
        FtpConnect ftp_connect = null;
        Ftp ftp = null;

        ftp_connect = new FtpConnect();
        ftp_connect.setHostName(ftpHost);
        //The dir path format must be path/xxx/
        String dir = ftpDir.startsWith("/")?ftpDir.substring(1):ftpDir;
        dir = dir+(dir.endsWith("/")?"":"/");
        ftp_connect.setPathName(dir);
        ftp_connect.setUserName(user);
        ftp_connect.setPassWord(passWord);
        //Log.d("ST",ftpHost+" "+dir+" "+fileName);
        ftp = new Ftp();
        try
        {
            boolean is_connected = ftp.connect(ftp_connect);
            if(is_connected)
            {
                //Log.d("ST","ftp.pwd():"+ftp.pwd());
                /* source FtpFile remote file */
                FtpFile file = new FtpFile(ftp.pwd()+"/"+fileName, ftp);
                /* open ftp input stream */
                is = new FtpInputStream(file);
                BufferedReader br = new BufferedReader(new InputStreamReader(is));
                File myTempFile = File.createTempFile(fileName.substring(0, fileName.lastIndexOf(".")), fileName.substring(fileName.lastIndexOf(".")));
                FileOutputStream fos = new FileOutputStream(myTempFile);
                byte buf[] = new byte[128];
                int total_load = 0;
                do 
                {   
                    int numread = is.read(buf);
                    total_load += numread;
                    if (numread <= 0) 
                    { 
                        break; 
                    } 
                    fos.write(buf, 0, numread);
                }while (true);
                /* close reader */
                br.close();
                fos.close();
                file_path = myTempFile.getAbsolutePath();
            }
            else
                throw new FileNotFoundException(ftpHost+ftpDir+fileName);
        }
        catch(IOException err)
        {
            if (is != null)
                try {
                    is.close();
                    
                } catch (IOException e) {
                }
            /* disconnect from server 
             * this must be always run */
            ftp.disconnect();
            throw err;
        }
        finally
        { /* close ftp input stream 
             * this must be always run */
            if (is != null)
             try {
                 is.close();
                 
             } catch (IOException e) {
             }
             /* disconnect from server 
              * this must be always run */
             ftp.disconnect();
             return file_path;
        }
    }
	
	//Upload file to ftp server
    public static boolean uploadFile(String ftpHost, String ftpDir, String user, String passWord, String filePath) throws IOException//final FtpConnect ftpConnect,final Ftp mFtp,final String fileEx, final String fileNa, final Context context) throws Exception 
    { 
        boolean result = false;
        FtpOutputStream fos = null;
        FtpConnect ftp_connect = null;
        Ftp ftp = null;

        ftp_connect = new FtpConnect();
        ftp_connect.setHostName(ftpHost);
        //The dir path format must be path/xxx/
        String dir = ftpDir.startsWith("/")?ftpDir.substring(1):ftpDir;
        dir = dir+(dir.endsWith("/")?"":"/");
        ftp_connect.setPathName(dir);
        ftp_connect.setUserName(user);
        ftp_connect.setPassWord(passWord);
        //Log.d("ST",ftpHost+" "+dir);
        ftp = new Ftp();
        try
        {
            boolean is_connected = ftp.connect(ftp_connect);
            if(is_connected)
            {
                //Log.d("ST","ftp.pwd():"+ftp.pwd());
                /* source FtpFile remote file */
                String fileName = null;
                if(filePath != null)
                    fileName = filePath.substring(filePath.lastIndexOf("/")+1);
                FtpFile ftp_file = new FtpFile(ftp.pwd()+"/"+fileName, ftp);
                //Log.d("ST","filePath:"+filePath+", fileName:"+fileName);
                File temp_file = new File(filePath);
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"upload file:"+temp_file+":"+temp_file.length());
                if(!temp_file.exists() && temp_file.length() > 0)
                {
                    Log.d(DEBUGTAG,"create new file");
                    temp_file.createNewFile();
                }
                /* open ftp output stream */
                fos = new FtpOutputStream(ftp_file);
                InputStream input_stream = null;
                if(temp_file != null && temp_file.length() >0)
                    input_stream = new BufferedInputStream(new FileInputStream(temp_file));
                byte buf[] = new byte[1024];
                int total_load = 0;
                int numread = -1;
                do 
                {   
                    if(input_stream != null)
                    {
                        numread = input_stream.read(buf);
                        total_load += numread;
                        if (numread <= 0) 
                        { 
                            break; 
                        } 
                        fos.write(buf, 0, numread);
                    }

                }while (true);
                /* close reader */
                //br.close();
                fos.close();
                input_stream.close();
                result = true;
            }
            else
                throw new FileNotFoundException(ftpHost+ftpDir);
        }
        catch(IOException err)
        {
            if (fos != null)
                try {
                    fos.close();
                } catch (IOException e) {
                }
            /* disconnect from server 
             * this must be always run */
            ftp.disconnect();
            throw err;
        }
        finally
        { /* close ftp output stream 
             * this must be always run */
            if (fos != null)
            {
                try {
                    fos.close();
                } 
                catch (IOException e) {
                 }
            }
             /* disconnect from server 
              * this must be always run */
             ftp.disconnect();
             return result;
        }
    }
}
