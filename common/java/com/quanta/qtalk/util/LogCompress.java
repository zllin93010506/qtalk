package com.quanta.qtalk.util;

import android.os.Handler;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Calendar;
import java.util.zip.GZIPOutputStream;

public class LogCompress {
    public static String logStorage = "/sdcard" + java.io.File.separatorChar
            + "Android" + java.io.File.separatorChar
            + "data" + java.io.File.separatorChar
            + "com.quanta.qtalk" + java.io.File.separatorChar
            + "QSPT_LOG" + java.io.File.separatorChar;
    private static final String DEBUGTAG = "QTFa_LogCompress";

    private static void deleteOldLogs(int daysago) {
        File directory = new File(logStorage);
        if (!directory.exists()) {
            Log.i(DEBUGTAG, "delete: directory is not exist"+directory.getName());
            return;
        }
        File[] files = directory.listFiles();
        for (int j = 0; j < files.length; j++) {
            Calendar calendar = Calendar.getInstance();
            for (int i = 0; i < daysago; i++) {
                if(files[j] == null)
                    continue;
                String name = files[j].getName();
                String date = String.format("%04d-%02d-%02d", calendar.get(Calendar.YEAR) , (calendar.get(Calendar.MONTH)+1), calendar.get(Calendar.DAY_OF_MONTH));
                //Log.i(DEBUGTAG, "date:" + date + " name:" + name);
                calendar.add(Calendar.DAY_OF_MONTH, -1);
                //delete .log.gz files 
                if ((name.indexOf(date) != -1) && (!name.endsWith("log.gz")))  {
                    files[j] = null;
                    Log.i(DEBUGTAG, "skip delete:" + name);
                    continue;
                }
            }
        }

        for (int j = 0; j < files.length; j++) {
            if (files[j] != null) {
                files[j].delete();
                Log.i(DEBUGTAG, files[j].getName() + " is deleted.");

            }
        }
    }

    private static void compressLogs() {
        File directory = new File(logStorage);
        boolean today_is_exist = false;
        if (!directory.exists()) {
            Log.i(DEBUGTAG, "compress: directory is not exist" + directory.getName());
            return;
        }
        File[] files = directory.listFiles();
        for (int j = 0; j < files.length; j++) {
            String name = files[j].getName();
            Calendar calendar = Calendar.getInstance();
            String date = String.format("%04d-%02d-%02d", calendar.get(Calendar.YEAR) , (calendar.get(Calendar.MONTH)+1), calendar.get(Calendar.DAY_OF_MONTH));
            //Log.i(DEBUGTAG, "date:" + date + " name:" + name);
            //skip today
            if (name.indexOf(date) != -1) {
                files[j] = null;
                Log.i(DEBUGTAG, "skip compress:" + name);
                today_is_exist = true;
                continue;
            }
            //skip compressed files
            if(name.indexOf("gz")!=-1) {
                files[j] = null;
                Log.i(DEBUGTAG, "skip compress:" + name);
                continue;
            }
        }
        //today is not logrotate by logback
        if(!today_is_exist) {
            Log.i(DEBUGTAG, "today's logfile is not exist, skip compress:");
            return;
        }
        for (int j = 0; j < files.length; j++) {
            if (files[j] != null) {
                final File logfile = files[j];
                new Thread(new Runnable() {
                    @Override
                    public void run() {

                        try {
                            String gzipFile = logfile.getAbsolutePath() + ".gz";
                            FileInputStream fis = new FileInputStream(logfile);
                            FileOutputStream fos = new FileOutputStream(gzipFile);
                            GZIPOutputStream gzipOS = new GZIPOutputStream(fos);
                            byte[] buffer = new byte[1024];
                            int len;
                            while ((len = fis.read(buffer)) != -1) {
                                gzipOS.write(buffer, 0, len);
                            }
                            gzipOS.close();
                            fos.close();
                            fis.close();
                            Log.i(DEBUGTAG, "compress file:" + logfile.getName() + " to " + logfile.getAbsolutePath() + ".gz");
                            logfile.delete();
                        } catch (Exception e) {
                            Log.e(DEBUGTAG, "compress file failed", e);
                        }
                    }
                }).start();
            }
        }
    }

    public static void handleLogs() {
        deleteOldLogs(7);
        compressLogs();
    }
}
