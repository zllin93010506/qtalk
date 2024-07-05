package com.quanta.qtalk.util;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Point;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

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

	public static float widthRatio = 1f, heightRatio = 1f, fontSizeHorizonRatio = 1f, fontSizeVerticalRatio = 1f;

	public static int screenWidth = 1600;
	public static int screenHeight = 900;


	public static float scaledDensity = 1.0F;
	public static float density = 1.0F;

	public static int densityDpi = 160;

	public static int widthPixels = 1600;
	public static int heightPixels = 900;
	public static int pxToDp(float px) {
		return (int) (px / density);
	}

	public static int dpToPx(float dp) {
		return (int) (dp * density+0.5f);
	}

	public static int ratioWidth(int width) {
		return (int)(width*widthRatio);
	}

	public static float ratioX(float x) {
		return (int)(x*widthRatio);
	}

	public static int ratioHeight(int height)
	{
		return (int)(height*heightRatio);
	}

	public static int ratioWidthFix(int height)
	{
		return (int)(height*widthRatio/heightRatio);
	}

	public static float ratioY(float y)
	{
		return (int)(y*heightRatio);
	}

	public static float ratioHFont(float fontSize)
	{
		return (int)(fontSize*fontSizeHorizonRatio);
	}

	public static float ratioVFont(float fontSize)
	{
		return (int)(fontSize*fontSizeVerticalRatio);
	}

	public static void initRatios(Activity activity) {
		WindowManager wm = (WindowManager) activity.getSystemService(activity.WINDOW_SERVICE);
		DisplayMetrics displayMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(displayMetrics);
		Point screenSize = new Point();
		Display display = wm.getDefaultDisplay();
		display.getRealSize(screenSize);
		screenWidth = screenSize.x;
		screenHeight = screenSize.y;

		Log.d(DEBUGTAG, "initRatios Screensize="+screenWidth + "," + screenHeight);
		scaledDensity = displayMetrics.scaledDensity;
		density = displayMetrics.density;
		densityDpi = displayMetrics.densityDpi;
		Log.d(DEBUGTAG, "" +
				" density="+density + ",densityDpi=" + densityDpi);

		widthPixels = displayMetrics.widthPixels;
		heightPixels = displayMetrics.heightPixels;

		widthRatio = screenWidth / 1600f;
		heightRatio = screenHeight / 900f;
		Log.d(DEBUGTAG,"initRatios widthRatio="+widthRatio+",heightRatio="+heightRatio);

		fontSizeHorizonRatio = screenWidth / density / 1600f;
		fontSizeVerticalRatio = screenHeight / density / 900f;

		QtalkSettings.ScreenSize = 4;
	}

}
