package com.quanta.qtalk.ui.notification;
public class NotificationInfo {  
	
        String  mNotification = null;
        
        public void setString(String Notification)
        {
        	mNotification = new String(Notification);
        }
        public String getString()
        {
            return mNotification;
        }

}
