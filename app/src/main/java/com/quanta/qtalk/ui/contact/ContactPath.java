package com.quanta.qtalk.ui.contact;

import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

public class ContactPath {
	
	String path = "";
	String name = "";
	int imageID = 0;
	int textID = 0;
	int contactbgID = 0;
	ImageButton button = null;
	TextView textview = null;
	ImageView contactbg = null;
	
		public void setPath(String path)
		{
			this.path =path;
		}
		public void setName(String name)
		{
			this.name =name;
		}
		public void setImageID(int imageID)
		{
			this.imageID = imageID;
		}
		public void setTextID(int textID)
		{
			this.textID = textID;
		}
		public void setImageButton(ImageButton button)
		{
			this.button = button;
		}
		public void setTextView(TextView textview)
		{
			this.textview = textview;
		}
		public void setContactBGID(int contactbgID)
		{
			this.contactbgID = contactbgID;
		}
		public void setContactBG(ImageView contactbg)
		{
			this.contactbg = contactbg;
		}
		
		
		public String getPath()
		{
			return this.path;
		}
		public String getName()
		{
			return this.name;
		}
		public int getImageID()
		{
			return this.imageID;
		}
		public int getTextID()
		{
			return this.textID;
		}
		public int getContactBGID()
		{
			return this.contactbgID;
		}
		public ImageButton getImageButton()
		{
			return this.button;
		}
		public TextView getTextView()
		{
			return this.textview;
		}
		public ImageView getContactBG()
		{
			return this.contactbg;
		}

}
