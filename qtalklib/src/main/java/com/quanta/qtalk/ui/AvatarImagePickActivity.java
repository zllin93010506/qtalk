package com.quanta.qtalk.ui;

import java.io.File;
import java.io.FileOutputStream;

//import com.quanta.qtalk.Flag;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.R;
import com.quanta.qtalk.provision.DrawableTransfer;
import com.quanta.qtalk.provision.FileUtility;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import com.quanta.qtalk.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class AvatarImagePickActivity extends AbstractVolumeActivity{

//	private String TAG="AvatarImagePickActivity";
    private static final String DEBUGTAG = "QTFa_AvatarImagePickActivity";
	Button camerabutton;
	Button gallerybutton;
	Button cancelbutton;
	Button defaultbutton;
//	ImageView imageView;
//	ImageView myphoto;
//	private static ICallAvatar aCallBack = null;
	final static int CROP_IMAGE = 2;
	final static int PICK_PICTURE = 1;
	final static int TAKE_PICTURE = 0;
	static final String path = Hack.getStoragePath()+"QOCAMyimage.jpg";

	
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(com.quanta.qtalk.R.layout.layout_image_picker);
		
		camerabutton = (Button) findViewById(R.id.camerabutton);
		gallerybutton = (Button) findViewById(R.id.gallerybutton);
		cancelbutton = (Button) findViewById(R.id.cancelbutton);
		defaultbutton = (Button) findViewById(R.id.defaultbutton);
		
		OnClickListener theclick=new OnClickListener(){
            @Override
            public void onClick(View v) {
                synchronized(AvatarImagePickActivity.this)
                {
                	switch(v.getId()){
                    case R.id.camerabutton:
        			    Intent intent_camera = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);  
        			    intent_camera.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(new File(Environment.getExternalStorageDirectory(), "temp.jpg")));  
                        startActivityForResult(intent_camera, TAKE_PICTURE); 
        				break;   
                    case R.id.gallerybutton:
                    	Intent photoPickerIntent = new Intent(Intent.ACTION_PICK);
                    	photoPickerIntent.setType("image/*");
                    	startActivityForResult(photoPickerIntent, PICK_PICTURE); 
        				break; 
                    case R.id.defaultbutton:
                    	File file = new File (path);
                		if (file.exists())
                		{	
                			file.delete();
                			Intent AvatarIntent = new Intent();
                			AvatarIntent.setClass(AvatarImagePickActivity.this, AvatarActivity.class);
                			startActivity(AvatarIntent);
                		}
                    	finish();
                    	break;
                    case R.id.cancelbutton:
                    	finish();
                    	break;
                	}
                }
            }//End of onclick
        };
        camerabutton.setOnClickListener(theclick);
        gallerybutton.setOnClickListener(theclick);
        defaultbutton.setOnClickListener(theclick);
        cancelbutton.setOnClickListener(theclick); 
	}//end of onCreate
	
	
	@SuppressLint("InlinedApi")
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
	//	FileOutputStream fop = null;
		if (resultCode == RESULT_OK)
		{
			switch (requestCode) {  
				case TAKE_PICTURE:
					File picture = new File(Environment.getExternalStorageDirectory() + "/temp.jpg");  
					doCropPhoto(Uri.fromFile(picture));
				break; 
				case PICK_PICTURE:
					Uri selectedImage = data.getData();
					doCropPhoto(selectedImage);
					break; 
				case CROP_IMAGE:
					//after cropping photo 
					Bitmap photo1 = data.getParcelableExtra("data");
					if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "2 width:"+photo1.getWidth()+" height:"+photo1.getHeight());
					Bitmap photo2 = Bitmap.createScaledBitmap(photo1, 300, 300, true);
					writePhotoJpg(photo2, path);
			/*		if(aCallBack != null)
					{
			            aCallBack.updateCallAvatar();
					}*/
					Intent AvatarIntent = new Intent();
        			AvatarIntent.setClass(AvatarImagePickActivity.this, AvatarActivity.class);
        			startActivity(AvatarIntent);
        			photo1.recycle();
        			photo2.recycle();
        			photo1 = null;
        			photo2 = null;
        			FileUtility.delFile(Environment.getExternalStorageDirectory() + "/temp.jpg");
        		    finish();
				break;   
			}
		}
	    
		super.onActivityResult(requestCode, resultCode, data);
	}
	
/*	public static void setCallBack(ICallAvatar cb)
    {
        aCallBack = cb;
    }
	
	 
     
     public interface ICallAvatar
     {
         public void updateCallAvatar();
     }
     */
     public static void writePhotoJpg(Bitmap data, String pathName) {
 	    File file = new File(pathName);
 	    try {
 	        file.createNewFile();
 	        FileOutputStream os = new FileOutputStream(file);
 	        data.compress(Bitmap.CompressFormat.JPEG, 100, os);
 	        os.flush();
 	        os.close();
 	       //DrawableTransfer.DrawImageConer(path);
 	    } catch (Exception e) {
 	    	Log.e(DEBUGTAG,"writePhotoJpg",e);
 	        Log.e(DEBUGTAG,"",e);
 	    }
 	    //data.recycle();
 	}

 	
 	protected void doCropPhoto(Uri uri) {
        Intent intent = getCropImageIntent(uri);
        startActivityForResult(intent, CROP_IMAGE);
    }
     
	
 	public static Intent getCropImageIntent(Uri uri) {
        Intent intent = new Intent("com.android.camera.action.CROP");
        intent.setDataAndType(uri, "image/*");
        intent.putExtra("crop", "true");
        intent.putExtra("aspectX", 1);
        intent.putExtra("aspectY", 1);
        intent.putExtra("outputX", 300);
        intent.putExtra("outputY", 300);
        intent.putExtra("scale", true);//去黑邊
        intent.putExtra("scaleUpIfNeeded", true);//去黑邊
        intent.putExtra("return-data", true);
        return intent;
    }
 	
 	
	@Override
	public void onResume()
	{
		super.onResume();
	}

	@Override
	public void onPause()
	{
		super.onPause();
	}
	
	@Override
	public void onStop()
	{
		super.onStop();
	}
	
	@Override
	public void onDestroy()
	{
		super.onDestroy();
		releaseAll();
	}
	
	private void releaseAll()
	{
		camerabutton = null;
		gallerybutton = null;
		cancelbutton = null;
		defaultbutton = null;
//		myphoto = null;
	//	aCallBack = null;
	}
}
