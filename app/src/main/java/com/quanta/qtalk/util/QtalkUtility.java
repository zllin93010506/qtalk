package com.quanta.qtalk.util;

import android.content.Context;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;

public class QtalkUtility {
	private final static String DEBUGTAG = "QTFa_QtalkUtility";
	public static QtalkSettings getSettings(Context ctx, QtalkEngine engine){
		QtalkSettings settings = null;
		//engine = QTReceiver.engine( ctx, false);
        try {
        	settings = engine.getSettings(ctx);
		} catch (FailedOperateException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"getSettings",e);
		}
		return settings; 
		
	}

}
