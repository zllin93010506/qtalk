package com.quanta.qtalk.provision;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import org.xeustechnologies.jtar.TarEntry;
import org.xeustechnologies.jtar.TarInputStream;

public class Jtar {
	
	public void untar(String tarFile, String destFolder) throws IOException {
		  
		  // Create a TarInputStream
		  TarInputStream tis = new TarInputStream(new BufferedInputStream(new FileInputStream(tarFile)));
		  TarEntry entry;
		  
		  while((entry = tis.getNextEntry()) != null) {
		     int count;
		     byte data[] = new byte[2048];
		     FileOutputStream fos = new FileOutputStream(destFolder + "/" + entry.getName());
		     BufferedOutputStream dest = new BufferedOutputStream(fos);
		  
		     while((count = tis.read(data)) != -1) {
		        dest.write(data, 0, count);
		     }
		  
		     dest.flush();
		     dest.close();
		     fos.close();
		  }
		  tis.close();
    }
}
