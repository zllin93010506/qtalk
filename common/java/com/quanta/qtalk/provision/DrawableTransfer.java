package com.quanta.qtalk.provision;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.ArrayList;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.QtalkCheckActivity2;
import com.quanta.qtalk.ui.contact.QTContact2;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;

public class DrawableTransfer {
	
	private static final String DEBUGTAG = "QTFa_DrawableTransfer";

	public static Drawable LoadImageFromWebOperations(String url) {
		//System.gc();
        try {
                InputStream is = null;
                if (url.indexOf("http") >= 0)
                        is = (InputStream) new URL(url).getContent();
                else
                        is = new FileInputStream(new File(url));

                Drawable d = Drawable.createFromStream(is, "src name");
                is.close();
                is = null;
                return d;
        } catch (Exception e) {
                System.out.println("Exc=" + e);
                return null;
        }
	}
		
	
	
	public static void LoadImageFromDrawabletoSDcard(InputStream is) throws IOException {
		
		File myTempFile = new File("sdcard/no_contact.jpg");
		FileOutputStream fos = new FileOutputStream(myTempFile);
		byte buf[] = new byte[1024];
		do {
				int numread = is.read(buf);
				if (numread <= 0) 
				{
					break;
				}
			fos.write(buf, 0, numread);
			} 
		while (true);
		is.close();
		fos.close();
	}

	
	public static void DrawImageConer(String url) throws IOException {
	//	System.gc();
		Bitmap input = BitmapFactory.decodeFile(url); 
		Bitmap output = getRoundedCornerBitmap(input);  
		try {  
		FileOutputStream out = new FileOutputStream(url+".jpg");
		output.compress(Bitmap.CompressFormat.PNG, 100, out);
		out.close();
		} catch (Exception e) {  
		Log.e(DEBUGTAG,"",e);  
		}
		input.recycle();
		output.recycle();
		input = null;
		output = null;
	}
	
	
/*	public static void DrawImageConerAvatar(String url) throws IOException {
		
		Bitmap input = BitmapFactory.decodeFile(url); 
		Bitmap output = getRoundedCornerBitmapAvatar(input);  
		try {  
		FileOutputStream out = new FileOutputStream(url);
		output.compress(Bitmap.CompressFormat.PNG, 100, out);
		} catch (Exception e) {  
		Log.e(DEBUGTAG,"",e);  
		}  
	}*/
	
	public static Bitmap getRoundedCornerBitmap(Bitmap bitmap) {
		Bitmap output = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Config.ARGB_8888);
		Canvas canvas = new Canvas(output);  
		  
		final int color = 0xff424242;  
		final Paint paint = new Paint();  
		final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());  
		final RectF rectF = new RectF(rect);  
		// radius 
		final float roundPx = 40;
		paint.setAntiAlias(true);  
		canvas.drawARGB(0, 0, 0, 0);  
		paint.setColor(color);  
		//resizeBitmap(bitmap,300,300);
		// draw radius 
		canvas.drawRoundRect(rectF, roundPx, roundPx, paint);  
		// combine
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));  
		// show photo 
		canvas.drawBitmap(bitmap, rect, rect, paint);
		if (ProvisionSetupActivity.debugMode)Log.d(DEBUGTAG,"drawBitmap");
		bitmap.recycle();
		bitmap = null;
		return output;  
		}  
	
	public static void DeletePhonebookCornerPhoto(ArrayList<QTContact2> temp_list)
	    {
	  	  if(temp_list!=null){
		  	  for (int i=0; i< temp_list.size(); i++)
				{
		  		  String FilePath = temp_list.get(i).getImage()+".jpg";
		  		  if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "FilePath ="+FilePath);
		  		  File file = new File (FilePath);
		  		  if (file.exists())
		  		  {
		  			  if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "delFile="+FilePath);
		  			  FileUtility.delFile(FilePath);
		  		  }
				}
	  	  }
	    }
	


	public static Bitmap drawableToBitmap (Drawable drawable) {

	    Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight(), Config.ARGB_8888);
	    Canvas canvas = new Canvas(bitmap); 
	    drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
	    drawable.draw(canvas);
	
	    return bitmap;
	}


    public static Bitmap getImageFromAssetsFile(Context context, String fileName) {
    	Log.d(DEBUGTAG, "Copy default icn file to data/data");
        String path= Hack.getStoragePath();
        File file = new File(path);
        Bitmap image = null;
        boolean isExist = file.exists();
        if(!isExist){
            file.mkdirs();
        }
        AssetManager am = context.getAssets();
        try {
            InputStream is = am.open("pictures/"+fileName);
            image = BitmapFactory.decodeStream(is);
            FileOutputStream out = new FileOutputStream(path+fileName);
            image.compress(Bitmap.CompressFormat.PNG,100,out);
            out.flush();
            out.close();
            is.close();
        } catch (IOException e) {
            Log.e(DEBUGTAG,"",e);
        }
        return image;
    }
	
/*	public static Bitmap getRoundedCornerBitmapAvatar(Bitmap bitmap) {  
		Bitmap output = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Config.ARGB_8888);
		Canvas canvas = new Canvas(output);  
		  
		final int color = 0xff424242;  
		final Paint paint = new Paint();  
		final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());  
		final RectF rectF = new RectF(rect);  
		// radius 
		final float roundPx = 10;
		paint.setAntiAlias(true);  
		canvas.drawARGB(0, 0, 0, 0);  
		paint.setColor(color);  
		//resizeBitmap(bitmap,300,300);
		// draw radius 
		canvas.drawRoundRect(rectF, roundPx, roundPx, paint);  
		// combine
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));  
		// show photo 
		canvas.drawBitmap(bitmap, rect, rect, paint);
		Log.d("GetRoundedCorner","drawBitmap");
		return output;  
		} */
	

 /*   public static Bitmap resizeBitmap(Bitmap bitmap, int maxWidth, int maxHeight) {

        int originWidth  = bitmap.getWidth();
        int originHeight = bitmap.getHeight();

        // no need to resize
        if (originWidth < maxWidth && originHeight < maxHeight) {
            return bitmap;
        }

        int width  = originWidth;
        int height = originHeight;

        // 若图片过宽, 则保持长宽比缩放图片
        if (originWidth > maxWidth) {
            width = maxWidth;

            double i = originWidth * 1.0 / maxWidth;
            height = (int) Math.floor(originHeight / i);

            bitmap = Bitmap.createScaledBitmap(bitmap, width, height, false);
        }

        // 若图片过长, 则从上端截取
        if (height > maxHeight) {
            height = maxHeight;
            bitmap = Bitmap.createBitmap(bitmap, 0, 0, width, height);
        }

        return bitmap;
    }
	*/
	
}
