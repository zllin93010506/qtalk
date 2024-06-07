package com.quanta.qtalk.call;

public class CallState 
{
	public static int MAKE_CALL_WITHOUT_LOGIN = -5;
	public static int MAKE_CALL_DURING_SESSION = -3;
	public static int MAKE_CALL_DURING_INVITE = -2;
	public static int MAKE_CALL_WITHOUT_NETWORK = -1;
    public static int NA = 0;
    public static int TRYING = 1;
    public static int RINGING = 2;
    public static int ACCEPT = 3;
    public static int TRASFERRED = 4;
    public static int HOLD = 5;
    public static int UNHOLD = 6;
    public static int HANGUP = 7;
    public static int CLOSED = 8;
    public static int FAILED = 0xFF;
}
