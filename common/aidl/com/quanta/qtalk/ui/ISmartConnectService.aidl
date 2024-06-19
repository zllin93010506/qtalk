package com.quanta.qtalk.ui;

import com.quanta.qtalk.ui.ISmartConnectCallback;

interface ISmartConnectService
{
	void Start();
	void Stop(); 
   	void SmartConnect(boolean isChecked);
   	boolean isStarted();
    void RegisterCallback(ISmartConnectCallback cb);
    void UnregisterCallback(ISmartConnectCallback cb);
}
