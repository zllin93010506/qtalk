package com.quanta.qtalk;

import com.quanta.qtalk.util.Log;
public enum Flag {
    InsessionFlag(false), AvatarSuccess(true), SIPRunnable(true), Activation(false), 
    Reprovision(false),Phonebook(true),//Check phonebook update or not
    SessionKeyExist(false), httpsConnectEnd(true),
    Notification(false),
    MCU(false),
    PTT(false);

    private boolean state;
    private Flag(boolean initialState) {
      this.state = initialState;
    }

    public boolean getState() {return state;}
    public void setState(boolean state) {
      StackTraceElement caller=Thread.currentThread().getStackTrace()[3];
      try {
        Log.d("Flag."+name()+".setState", 
              this.state+"->"+state + 
              " at " + caller.getFileName()+":"+caller.getLineNumber());
        }catch(Exception e) {
          Log.e("Flag."+name()+".setState", "max length="+Thread.currentThread().getStackTrace().length+" "+e.toString());
        }
      this.state = state;
    }
}