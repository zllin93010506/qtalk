package com.quanta.qtalk.util;

import android.content.Context;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.ui.QTReceiver;

public class QtalkUtility {
	private final static String DEBUGTAG = "QTFa_QtalkUtility";
	public static QtalkSettings getSettings(Context ctx, QtalkEngine engine){
		QtalkSettings settings = null;
		if(engine == null)
			engine = QTReceiver.engine( ctx, false);
		if(engine == null) {
			Log.e(DEBUGTAG,"getSettings engine==null");
			return null;
		}
        try {
        	settings = engine.getSettings(ctx);
		} catch (FailedOperateException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"getSettings",e);
		}
		return settings; 
		
	}

}
