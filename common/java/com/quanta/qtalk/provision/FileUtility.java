package com.quanta.qtalk.provision;

import java.io.File;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import com.quanta.qtalk.util.Log;

public class FileUtility
{
	private final static String DEBUGTAG = "QTFa_FileUtility";
    // delete file with absolute path
    public static void delFile(String strFilePath) 
    { 
        if(strFilePath != null)
        {
            File myFile = new File(strFilePath); 
            if(myFile.exists()) 
            { 
              myFile.delete(); 
            }
        } 
    }
    
    public static void delAllFile(String strFilePath, long systime) 
    { 
        if(strFilePath != null)
        {
            File myFile = new File(strFilePath); 
            if(myFile.exists()) 
            { 
              File redir = new File(myFile.getAbsolutePath() + systime);
              myFile.renameTo(redir); // rename to avoid EBUSY
              Log.d(DEBUGTAG,"rename folder :"+redir.getAbsolutePath());
              
              myFile = redir;
              strFilePath = redir.getAbsolutePath()+java.io.File.separatorChar;
              
              String[] templist = myFile.list();
              File temp = null;
              for (int i=0 ; i<templist.length ; i++)
              {
            	  if(strFilePath.endsWith(File.separator))
            	  {
            		  temp = new File(strFilePath+templist[i]);
            	  }
            	  else
            	  {
            		  temp = new File(strFilePath+File.separator+templist[i]);
            	  }
            	  if(temp.isFile())
            	  {
            		  temp.delete();
            	  }
            	  if(temp.isDirectory())
            	  {
            		  long systime2 = System.currentTimeMillis();
            		  delAllFile(strFilePath+"/"+templist[i], systime2);
            		  delFolder(strFilePath+"/"+templist[i]+systime2);
            	  }
              }
              myFile.delete();
            }
        } 
    }
    
 // delete folder with absolute path
    public static void delFolder(String strFolderPath) 
    { 
    	try{
    		long systime = System.currentTimeMillis();
    		delAllFile(strFolderPath, systime);
    	}catch (Exception e){
    		System.out.println("delete folder error");
    		Log.e(DEBUGTAG,"",e);
    	}
        
    }
    
    // install file
    public static void openFile(File file, Context context) 
    {
        Intent intent = new Intent();
//        cb.onDownloadPackageFinished(file.getAbsolutePath());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setAction(android.content.Intent.ACTION_VIEW);
        String type = FileUtility.getMIMEType(file);
        intent.setDataAndType(Uri.fromFile(file),type);
        context.startActivity(intent);
    }
    
    // check file format
    public static String getMIMEType(File f) 
    { 
        String type="";
        String fName=f.getName();
        String end=fName.substring(fName.lastIndexOf(".")+1,fName.length()).toLowerCase(); 
            
        if(end.equals("m4a")||end.equals("mp3")||end.equals("mid")||end.equals("xmf")||end.equals("ogg")||end.equals("wav"))
        {
            type = "audio";
        }
        else if(end.equals("3gp")||end.equals("mp4"))
        {
            type = "video";
        }
        else if(end.equals("jpg")||end.equals("gif")||end.equals("png")||end.equals("jpeg")||end.equals("bmp"))
        {
            type = "image";
        }
        else if(end.equals("apk")) 
        { 
            /* android.permission.INSTALL_PACKAGES */ 
            type = "application/vnd.android.package-archive";
        } 
        else
        {
            type="*";
        }
        if(end.equals("apk")) 
        { 
        } 
        else 
        { 
            type += "/*";  
        } 
        return type;
    }
}
