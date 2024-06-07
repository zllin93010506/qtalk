package com.quanta.qtalk;



enum Status {
    IDEL,
    SESSION,
    INCOMING,
    OUTGOING
}

public class AppStatus{

	public static int IDEL = 0;
	public static int SESSION = 1;
	public static int INCOMING = 2;
	public static int OUTGOING = 3;
	private static int status = 0;
	
	public AppStatus() {
		
	}
	public static void setStatus(int st){
		status = st;
	}
	public static int getStatus(){
		return status;
	}
}
