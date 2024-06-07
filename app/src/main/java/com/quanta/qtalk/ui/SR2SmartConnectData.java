package com.quanta.qtalk.ui;

import android.os.Parcel;
import android.os.Parcelable;

public class SR2SmartConnectData implements Parcelable{

    String IP ;  
    int Port ;  
    String MAC ;
    String DISPLAY_NAME ;
    String SIP_ID ;
    public SR2SmartConnectData(String IP, int Port, String MAC, String DISPLAY_NAME, String SIP_ID)
    {
    	this.IP = IP;
    	this.Port = Port;
    	this.MAC = MAC;
    	this.DISPLAY_NAME = DISPLAY_NAME;
    	this.SIP_ID = SIP_ID;
    }
    
    public SR2SmartConnectData()
    {
    	;
    }
    public SR2SmartConnectData(Parcel p)
    {
    	this.IP = p.readString();
    	this.Port = p.readInt();
    	this.MAC = p.readString();
    	this.DISPLAY_NAME = p.readString();
    }
    

    
    public String getIP() {  
        return IP;  
    }  
  
    public void setIP(String IP) {  
        this.IP = IP;  
    }  
  
    public int getPort() {  
        return Port;  
    }  
  
    public void setPort(int Port) {  
        this.Port = Port;  
    }  
  
    public String getMAC() {  
        return MAC;  
    }  
  
    public void setMAC(String MAC) {  
        this.MAC = MAC;  
    }  
    
    public String getDISPLAY_NAME() {  
        return DISPLAY_NAME;  
    }  
  
    public void setDISPLAY_NAME(String DISPLAY_NAME) {  
        this.DISPLAY_NAME = DISPLAY_NAME;  
    }  
    public String getSIP_ID() {  
        return SIP_ID;  
    }  
  
    public void setSIP_ID(String SIP_ID) {  
        this.SIP_ID = SIP_ID;  
    }  
    @Override  
    public int describeContents() {  
        return 0;  
    }  
  
    /** 
     * 将对象序列号 
     * dest 就是对象即将写入的目的对象 
     * flags 有关对象序列号的方式的标识 
     * 这里要注意，写入的顺序要和在createFromParcel方法中读出的顺序完全相同。例如这里先写入的为name， 
     * 那么在createFromParcel就要先读name 
     */  
    @Override  
    public void writeToParcel(Parcel dest, int flags) {  
              
            dest.writeString(IP);  
            dest.writeInt(Port);  
            dest.writeString(MAC);
            dest.writeString(DISPLAY_NAME);
            dest.writeString(SIP_ID);
    }  
    /** 
     * 在想要进行序列号传递的实体类内部一定要声明该常量。常量名只能是CREATOR,类型也必须是 
     * Parcelable.Creator<T> 
     */  
    public static final Parcelable.Creator<SR2SmartConnectData> CREATOR = new Creator<SR2SmartConnectData>() {  
          
        /** 
         * 创建一个要序列号的实体类的数组，数组中存储的都设置为null 
         */  
        @Override  
        public SR2SmartConnectData[] newArray(int size) {  
            return new SR2SmartConnectData[size];  
        }  
          
        /*** 
         * 根据序列号的Parcel对象，反序列号为原本的实体对象 
         * 读出顺序要和writeToParcel的写入顺序相同 
         */  
        @Override  
        public SR2SmartConnectData createFromParcel(Parcel source) {  
            String IP = source.readString();  
            int Port = source.readInt();  
            String MAC = source.readString();  
            String DISPLAY_NAME = source.readString();  
            String SIP_ID = source.readString();  
            SR2SmartConnectData scanTempResult = new SR2SmartConnectData();  
            scanTempResult.setIP(IP);  
            scanTempResult.setPort(Port);  
            scanTempResult.setMAC(MAC);  
            scanTempResult.setDISPLAY_NAME(DISPLAY_NAME);  
            scanTempResult.setSIP_ID(SIP_ID);  
            return scanTempResult;  
        }  
    };

	public void readFromParcel(Parcel _reply) {
		IP = _reply.readString();
		Port = _reply.readInt();
		MAC = _reply.readString();
		DISPLAY_NAME = _reply.readString();
		SIP_ID = _reply.readString();
	}  
      
      
      
}
