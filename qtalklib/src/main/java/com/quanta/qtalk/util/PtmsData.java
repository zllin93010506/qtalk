package com.quanta.qtalk.util;

import java.util.Calendar;
import java.util.HashMap;

public class PtmsData {
    public String bedid;
    public String name;
    public String alias;
    public String sipid;
    public String title;
    private static final String DEBUGTAG = "QTFa_PtmsData";
    public PtmsData() {
        this.name = "";
        this.alias = "";
        this.sipid = "";
        this.title = "";
        this.bedid = "";
    }
    public PtmsData(String bedid, String name, String alias, String sipid, String title) {
        if(bedid == null) {
            this.bedid = "";
        }
        else {
            this.bedid = bedid;
        }
        if(name == null) {
            this.name = "";
        }
        else {
            this.name = name;
        }
        if(alias == null) {
            this.alias = "";
        }
        else {
            this.alias = alias;
        }
        if(sipid == null) {
            this.sipid = "";
        }
        else {
            this.sipid = sipid;
            keylength = sipid.length();
        }
        if(title == null) {
            this.title = "";
        }
        else {
            this.title = title;
        }
    }

    private static HashMap<String, PtmsData> mData = new HashMap();
    private static int keylength = 0;
    private static Calendar lastUpdate = null;

    public static Object getLockedObject() {
        return mData;
    }

    public static boolean isExpired(int sec) {
        boolean ret = true;
        if(lastUpdate == null)
            return true;
        synchronized (getLockedObject()) {
            Calendar expireDate = lastUpdate;
            expireDate.add(Calendar.SECOND, 5);
            Calendar now = Calendar.getInstance();
            ret = now.after(expireDate);
        }
        return ret;
    }

    public static void clearData() {
        synchronized (getLockedObject()) {
            mData.clear();
            lastUpdate = Calendar.getInstance();
        }
    }

    public static void addData(String key, PtmsData value) {
        synchronized (getLockedObject()) {
            mData.put(key, value);
            lastUpdate = Calendar.getInstance();
        }
    }

    public static PtmsData findData(String sip) {
        PtmsData ret = null;
        Log.d(DEBUGTAG, "findData sip="+sip);
        synchronized (getLockedObject()) {
            try {
                String key = sip.substring(sip.length() - keylength);
                Log.d(DEBUGTAG, "findData key=" + key + ", keylength=" + keylength + ", sip length=" + sip.length());
                ret = mData.get(key);
                if (ret != null) {
                    Log.d(DEBUGTAG, "findData name=" + ret.name);
                    Log.d(DEBUGTAG, "findData title=" + ret.title);
                    Log.d(DEBUGTAG, "findData alias=" + ret.alias);
                    Log.d(DEBUGTAG, "findData sipid=" + ret.sipid);
                }
            } catch (Exception ex) {
            }
        }
        return ret;
    }
}
