package com.quanta.qtalk;

public class QtalkAction {
	 public static final String ACTION_APP_START =
         "com.quanta.qtalk.APP_START";

	 //
    // Broadcast Intent actions
    //
    /**
     * Broadcast intent action indicating that the call state (cellular)
     * on the device has changed.
     *
     * <p>
     * The {@link #EXTRA_STATE} extra indicates the new call state.
     * If the new state is RINGING, a second extra
     * {@link #EXTRA_INCOMING_NUMBER} provides the incoming phone number as
     * a String.
     *
     * <p class="note">
     * Requires the READ_PHONE_STATE permission.
     *
     * <p class="note">
     * This was a {@link android.content.Context#sendStickyBroadcast sticky}
     * broadcast in version 1.0, but it is no longer sticky.
     * Instead, use {@link #getCallState} to synchronously query the current call state.
     *
     * @see #EXTRA_STATE
     * @see #EXTRA_INCOMING_NUMBER
     * @see #getCallState
     */
    public static final String ACTION_PHONE_STATE_CHANGED =
            "android.intent.action.PHONE_STATE";

    /**
     * The lookup key used with the {@link #ACTION_PHONE_STATE_CHANGED} broadcast
     * for a String containing the new call state.
     *
     * @see #EXTRA_STATE_IDLE
     * @see #EXTRA_STATE_RINGING
     * @see #EXTRA_STATE_OFFHOOK
     *
     * <p class="note">
     * Retrieve with
     * {@link android.content.Intent#getStringExtra(String)}.
     */
    public static final String EXTRA_STATE = "state";
    /**
     * The phone state. One of the following:<p>
     * <ul>
     * <li>IDLE = no phone activity</li>
     * <li>RINGING = a phone call is ringing or call waiting.
     *  In the latter case, another call is active as well</li>
     * <li>OFFHOOK = The phone is off hook. At least one call
     * exists that is dialing, active or holding and no calls are
     * ringing or waiting.</li>
     * </ul>
     */
    enum State {
        IDLE, RINGING, OFFHOOK;
    };
    /**
     * Device call state: No activity.
     * Value used with {@link #EXTRA_STATE} corresponding to
     * {@link #CALL_STATE_IDLE}.
     * Constant Value: 0 (0x00000000)
     */
    public static final String EXTRA_STATE_IDLE = State.IDLE.toString();

    /**
     * Device call state: Ringing. A new call arrived and is ringing or waiting. In the latter case, another call is already active.
     * Value used with {@link #EXTRA_STATE} corresponding to
     * {@link #CALL_STATE_RINGING}.
     * Constant Value: 1 (0x00000001)
     */
    public static final String EXTRA_STATE_RINGING = State.RINGING.toString();

    /**
     * Device call state: Off-hook. At least one call exists that is dialing, active, or on hold, and no calls are ringing or waiting.
     * Value used with {@link #EXTRA_STATE} corresponding to
     * {@link #CALL_STATE_OFFHOOK}.
     * Constant Value: 2 (0x00000002)
     */
    public static final String EXTRA_STATE_OFFHOOK = State.OFFHOOK.toString();

    /**
     * The lookup key used with the {@link #ACTION_PHONE_STATE_CHANGED} broadcast
     * for a String containing the incoming phone number.
     * Only valid when the new call state is RINGING.
     *
     * <p class="note">
     * Retrieve with
     * {@link android.content.Intent#getStringExtra(String)}.
     */
    public static final String EXTRA_INCOMING_NUMBER = "incoming_number";
    /**
     * The state of a data connection.
     * <ul>
     * <li>CONNECTED = IP traffic should be available</li>
     * <li>CONNECTING = Currently setting up data connection</li>
     * <li>DISCONNECTED = IP not available</li>
     * <li>SUSPENDED = connection is created but IP traffic is
     *                 temperately not available. i.e. voice call is in place
     *                 in 2G network</li>
     * </ul>
     */
    
    public static final String ACTION_RECV_EVENT_SCANNING = "px2_android.eventmsg";
    public static final String ACTION_UDP_TX = "px2_android.udp_tx";
    public static final String ACTION_SIP = "px2_android.sip_actions";
    public static final String ACTION_PHONEBOOK = "px2_android.udp_tx";
    public static final String ACTION_SMARTCONNECT = "px2_android.smartconnect";
    public static final String ACTION_SR2_KEEPALIVE = "px2_android.sr2.keepalive";
    public static final String ACTION_VOLUME_CONTROL = "px2_android.volume_control";
    public static final String AUDIOTRACK_CONTROL = "px2_android.audiotrack_control";
    public static final String XML_PARSER = "px2_android.xmlparser";
    enum DataState {
        CONNECTED, CONNECTING, DISCONNECTED, SUSPENDED;
    };
}
