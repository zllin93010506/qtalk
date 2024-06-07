package com.quanta.qtalk.provision;

import java.io.File;
import java.io.IOException;
import java.security.GeneralSecurityException;

import com.quanta.qtalk.aescrypt.AESCrypt;
import com.quanta.qtalk.ui.ProvisionSetupActivity;

public class DecryptUtility
{
   
    public static String decryptFile(String inputFilePath , final String password, String outputFile) throws IOException, GeneralSecurityException
    {
        String decrypt_file_path = null;
        File myDecryptedFile = File.createTempFile(outputFile.substring(0, outputFile.lastIndexOf(".")), outputFile.substring(outputFile.lastIndexOf("."))); 
        
        AESCrypt aes = new AESCrypt(ProvisionSetupActivity.debugMode, password);
        if(aes.decrypt(inputFilePath,myDecryptedFile.getAbsolutePath()))
        {
            decrypt_file_path = myDecryptedFile.getAbsolutePath();
        }
        return decrypt_file_path;
    }
}
