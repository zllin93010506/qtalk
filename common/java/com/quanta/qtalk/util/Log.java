package com.quanta.qtalk.util;
import android.os.Environment;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.PrintWriter;
import java.io.StringWriter;

import ch.qos.logback.classic.LoggerContext;
import ch.qos.logback.classic.util.ContextInitializer;
import ch.qos.logback.core.joran.spi.JoranException;

public class Log
{ 
    public static final boolean ENABLE_DEBUG = true;
    //public static final boolean ENABLE_LOGD = true;
    public interface LogWriter
    {
        public void writeLog(String msg);
        public void writeLog(String string, Throwable err);
    }
    
    private static LogWriter mLogWriter = null;
    private static Logger log = LoggerFactory.getLogger(Log.class);


    public static void stop() {
        //LoggerContext lc = (LoggerContext) LoggerFactory.getILoggerFactory();
        //lc.stop();
        //log = null;
    }

    public static void setSysLogConfigInLogback(String ipAddress, String deviceId){
        if(log != null) log.debug("QSPT_LOG_SERVER: " + ipAddress + " QSPT_DEVICE_ID: " + deviceId); else return;

        try {
            if(!Hack.isUUID(deviceId))
                return;
            File logdir = new File(LogCompress.logStorage);
            if(!logdir.exists()) {
                if(log == null) log = LoggerFactory.getLogger(Log.class);
                log.debug("mkdirs:"+logdir.getAbsolutePath()+":"+logdir.mkdirs());
            }
            System.setProperty("QSPT_DEVICE_ID", deviceId);
            System.setProperty("QSPT_LOG_SERVER", ipAddress);

            //Reload:
            LoggerContext lc = (LoggerContext) LoggerFactory.getILoggerFactory();
            ContextInitializer ci = new ContextInitializer(lc);
            lc.reset();

            //I prefer autoConfig() over JoranConfigurator.doConfigure()
            // so I wouldn't need to find the file myself.
            ci.autoConfig();
        } catch (JoranException e) {
            if(log != null) log.error("JoranException: " + e.toString());
        } catch (NullPointerException e) {
            if(log != null)  log.error("NullPointerException: " + e.toString());
        }
    }

    private static  String oneLineStackTrace(Throwable ex) {
        StringWriter errors = new StringWriter();
        ex.printStackTrace(new PrintWriter(errors));
        return errors.toString().replaceAll("\n", " ");
    }

    public static void setWriter(LogWriter writer)
    {
        mLogWriter = writer;
    }
    public static void e(String tag, String string, Throwable e)
    {
        if(log != null) log.error(tag+" - "+string+" "+oneLineStackTrace(e));
        if (mLogWriter!=null)
            mLogWriter.writeLog(tag+" - "+string,e);
    }

    public static void d(String tag, String string, Throwable e)
    {
        if(log != null) log.debug(tag+" - "+string+" "+oneLineStackTrace(e));
        if (mLogWriter!=null)
            mLogWriter.writeLog(tag+" - "+string,e);
    }
    public static void d(String tag, String string)
    {
        if (ENABLE_DEBUG)
        {
            if(log != null) log.debug(tag+" - "+string);
            /*if (ENABLE_LOGD)
            {
	            if (mLogWriter!=null)
	                mLogWriter.writeLog(tag+" - "+string);
            }*/
        }
    }
    public static void i(String tag, String string)
    {
        if (ENABLE_DEBUG)
        {
            if(log != null) log.info(tag+" - "+string);
            /*if (ENABLE_LOGD)
            {
	            if (mLogWriter!=null)
	                mLogWriter.writeLog(tag+" - "+string);
            }*/
        }
    }
    public static void e(String tag, String string)
    {
        if(log != null) log.error(tag+" - "+string);
        if (mLogWriter!=null)
            mLogWriter.writeLog(tag+" - "+string);
    }
    public static void l(String tag, String string)
    {
        if(log != null) log.debug(tag+" - "+string);
        /*if (mLogWriter!=null)
            mLogWriter.writeLog(tag+" - "+string);*/
    }

    public static void w(String tag, String string)
    {
        if(log != null) log.warn(tag+" - "+string);
        /*if (mLogWriter!=null)
            mLogWriter.writeLog(tag+" - "+string);*/
    }
}
