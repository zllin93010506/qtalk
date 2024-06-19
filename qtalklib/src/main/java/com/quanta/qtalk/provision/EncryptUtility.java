package com.quanta.qtalk.provision;

import java.io.File;
import java.io.IOException;
import java.security.GeneralSecurityException;

import android.content.Context;

import com.quanta.qtalk.aescrypt.AESCrypt;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class EncryptUtility
{
//    private static final String TAG ="EncryptUtility";
    private static final String DEBUGTAG = "QTFa_EncryptUtility";
    public static String encryptFile(String inputFilePath , final String password, String outputFile, Context context) throws IOException, GeneralSecurityException
    {
        String encrypt_file_path = null;
        try
        {
            AESCrypt aes = null;
            File myEncryptedFile = File.createTempFile(outputFile.substring(0, outputFile.lastIndexOf(".")), outputFile.substring(outputFile.lastIndexOf(".")));
            if(password != null && password.trim().length() >0)
                aes = new AESCrypt(ProvisionSetupActivity.debugMode, password);
            if(aes != null && context != null)
            {
                aes.setConetxt(context);
                if(inputFilePath != null && myEncryptedFile != null)
                {
                    if(aes.encrypt(2,inputFilePath,myEncryptedFile.getAbsolutePath()))
                    {
                        encrypt_file_path = myEncryptedFile.getAbsolutePath();
                    }
                }
            }
        }
        catch(Exception err)
        {
            Log.e(DEBUGTAG,"EncryptUtility",err);
        }
        //Log.d(TAG, "encrypt_file_path:"+encrypt_file_path);
        return encrypt_file_path;
    }
}
