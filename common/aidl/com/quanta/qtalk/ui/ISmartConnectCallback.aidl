package com.quanta.qtalk.ui;
import com.quanta.qtalk.ui.SR2SmartConnectData;

interface ISmartConnectCallback
{
    void FinishScanning();
    void returnResult(in SR2SmartConnectData data);
}
