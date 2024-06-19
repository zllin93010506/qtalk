package com.quanta.qtalk.provision;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;

//import com.quanta.qtalk.util.Log;

import cz.dhl.ftp.Ftp;
import cz.dhl.ftp.FtpConnect;
import cz.dhl.ftp.FtpFile;
import cz.dhl.ftp.FtpInputStream;

public class FTPUtility
{
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
}
