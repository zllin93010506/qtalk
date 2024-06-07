package com.quanta.qtalk.media.video;

import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.util.Hack;

public class VideoLevelID{
	public String LevelID = "2.1";
	public int width = 0;
	public int height = 0;
	public int frame_rate = 0;
	public int bit_rate= 0;
	public int GOP = 1000;
	public VideoLevelID videolevelID;
	
	public VideoLevelID(){
		if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
			LevelID = "3.1";
			width = 1280;
			height = 720;
			frame_rate = 30;
			bit_rate= 4000;
			GOP = 3000;
		}else if(Hack.isQF3a()||Hack.isQF5()){
			LevelID = "3.1";
			width = 1280; //1280;
			height = 720; //720;
			frame_rate = 30;
			bit_rate= 4000;
			GOP = 3000;
		}else{
			LevelID = "2.2";
			width = 640;
			height = 480;
			frame_rate = 30;
			bit_rate= 1200;
			GOP = 3000;
		}
	}
}
