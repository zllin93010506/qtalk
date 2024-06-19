package com.quanta.qtalk.ui.contact;

import com.quanta.qtalk.ui.QtalkCheckActivity2;
import com.quanta.qtalk.util.Hack;

public class QTContact2 {
	String name = "";
	String alias = "";
	String number = "";
//	String Ringtone = "";
	String role = "";
	String image = "";
//	String Location = "";
	String Key = "";
	String uid = "";
	String online = "";
	String title = "";
	
	public void setName(String name)
	{
		this.name = name;
	}
	public void setAlias(String alias)
	{
		this.alias = alias;
	}
	public void setPhoneNumber(String number)
	{
		this.number = number;
	}
/*	public void setRingtone(String Ringtone)
	{
		this.Ringtone = Ringtone;
	}*/
	public void setRole(String role)
	{
		this.role = role;
	}
	public void setImage(String image)
	{
		image = Hack.getStoragePath()+image;
		this.image = image;
	}
	public void setUid(String uid)
	{
		this.uid = uid;
	}
	public void setOnline(String online)
	{
		this.online = online;
	}
	public void setTitle(String title)
	{
		this.title = title;
	}
/*	public void setLocation(String Location)
	{
		this.Location = Location;
	}*/
	public void setKey(String Key)
	{
		this.Key = Key;
	}
	
	
	public String getName()
	{
		return this.name;
	}
	public String getAlias()
	{
		return this.alias;
	}
	public String getPhoneNumber()
	{
		return this.number;
	}
	public String getUid()
	{
		return this.uid;
	}
	public String getOnline()
	{
		return this.online;
	}
/*	public String getRingtone()
	{
		return this.Ringtone;
	}*/
	public String getTitle()
	{
		return this.title;
	}
	public String getRole()
	{
		return this.role;
	}
	public String getImage()
	{
		return this.image;
	}
	/*public String getLocation()
	{
		return this.Location;
	}*/
	public String getKey()
	{
		return this.Key;
	}
	
	
}
