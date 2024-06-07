package com.quanta.qtalk.provision;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.contact.QTContact2;
import com.quanta.qtalk.ui.contact.QTDepartment;
import com.quanta.qtalk.util.Log;

public class ProvisionUtility
{	
	private static final String DEBUGTAG                = "QTFa_ProvisionUtility";
    //The first classification
    static final String TAG_GENERIC                     = "generic";
    static final String TAG_VC                     		= "vc_service";
    static final String TAG_RM                     		= "rm_service";
    static final String TAG_DEP                         = "department";
    
    //The second classification
    static final String TAG_PROVISIONING                = "provisioning";
    static final String TAG_SIP                         = "sip";
    static final String TAG_RTP                         = "rtp";
    static final String TAG_LINES                       = "lines";
    static final String TAG_DEPBUDDY                    = "dep";
    
    //Under TAG_GENERIC
    // provisioning
    static final String TAG_PROVISIONING_CONFIG_FILE_FORCED_PROVISION_PERIODIC_TIMER        = "config_file_forced_provision_periodic_timer"; // added by CDRC
    static final String TAG_PROVISIONING_GENERAL_PERIOD_CHECK_TIMER       					= "general_period_check_timer"; // added by CDRC
    static final String TAG_PROVISIONING_SHORT_PERIOS_CHECK_TIMER        					= "short_period_check_timer";
    static final String TAG_PROVISIONING_CONFIG_FILE_DOWNLOAD_ERROR_RETRY_TIMER    			= "config_file_download_error_retry_timer"; // added by CDRC
    // sip
    static final String TAG_SIP_TOS_SETTING    			= "sip_tos_setting"; // added by CDRC
    static final String TAG_SIP_ADDRESS_MAPPING_MECHANISM    	= "sip_address_mapping_mechanism"; // added by CDRC
    static final String TAG_DNS_RETRY_COYNTS_TO_FAILOVER      = "dns_retry_counts_to_failover";
    // rtp
    static final String TAG_AUDIO_RTP_TOS_SETTING       = "audio_rtp_tos_setting";
    static final String TAG_VIDEO_RTP_TOS_SETTING       = "video_rtp_tos_setting";
    static final String TAG_RTP_AUDIO_PORT_RANGE_START  = "rtp_audio_port_range_start";
    static final String TAG_RTP_AUDIO_PORT_RANGE_END    = "rtp_audio_port_range_end";
    static final String TAG_RTP_VIDEO_PORT_RANGE_START  = "rtp_video_port_range_start";
    static final String TAG_RTP_VIDEO_PORT_RANGE_END    = "rtp_video_port_range_end";
    static final String TAG_OUTGOING_MAX_BANDWIDTH      = "outgoing_max_bandwidth";
    static final String TAG_INCOMING_MAX_BANDWIDTH      = "incoming_max_bandwidth";
    static final String TAG_AUTO_BANDWIDTH              = "auto_bandwidth";
    static final String TAG_VIDEO_MTU                   = "video_mtu";
    static final String TAG_VIDEO_FEC_ENABLE            = "video_fec_enable";
    static final String TAG_VIDEO_RFC4588_ENABLE        = "video_rfc4588_enable";
    static final String TAG_VIDEO_RFC4588_RTXTIME       = "video_rfc4588_rtxtime";
    // line
    static final String TAG_PRACK_SUPPORT               = "prack_support";
    static final String TAG_INFO_SUPPORT                = "info_support";
    static final String TAG_DTMF_TRANSMIT_METHOD        = "dtmf_transmit_method";
    static final String TAG_REGISTRATION_EXPIRATION_TIME= "registration_expiration_time";
    static final String TAG_REGISTRATION_RESEND_TIME    = "registration_resend_time";
    static final String TAG_CHALLENGE_SIP_INVITE        = "challenge_sip_invite";
    static final String TAG_SESSION_TIMER_ENABLE        = "session_timer_enable";
    static final String TAG_SESSION_EXPIRE_TIME         = "session_expire_time";
    static final String TAG_MINIMUM_SESSION_EXPIRE_TIME = "minimum_session_expire_time";
    // dep
    static final String TAG_DEP_ID                      = "id";
    static final String TAG_DEP_NAME                    = "name";    
    //Under TAG_VC
    //provisioning
    static final String TAG_PROVISIONING_URI       		= "provision_uri";// added by CDRC
    static final String TAG_PHONEBOOK_URI     			= "phonebook_uri";// added by CDRC
    static final String TAG_FIRMWARE_URI     			= "firmware_uri"; // added by CDRC
    static final String TAG_LOG_URI        				= "log_uri"; // added by CDRC
    static final String TAG_X264_URL                	= "x264_url";
    //sip
    static final String TAG_DISPLAY_NAME                = "display_name";
    static final String TAG_SIP_ID                      = "sip_id";
    static final String TAG_AUTHENTICATION_ID           = "authentication_id";
    static final String TAG_AUTHENTICATION_PASSWORD     = "authentication_password";
    static final String TAG_OUTBOUND_PROXY_ADDRESS      = "outbound_proxy_address";
    static final String TAG_OUTBOUND_PROXY_PORT         = "outbound_proxy_port";
    static final String TAG_PROXY_SERVER_ADDRESS        = "proxy_server_address";
    static final String TAG_PROXY_SERVER_PORT           = "proxy_server_port";
    static final String TAG_SIP_REALM                   = "sip_realm";
    static final String TAG_AUDIO_CODEC_PREFERENCE_1    = "audio_codec_preference_1";
    static final String TAG_AUDIO_CODEC_PREFERENCE_2    = "audio_codec_preference_2";
    static final String TAG_AUDIO_CODEC_PREFERENCE_3    = "audio_codec_preference_3";
    static final String TAG_AUDIO_CODEC_PREFERENCE_4    = "audio_codec_preference_4";
    static final String TAG_VIDEO_CODEC_PREFERENCE_1    = "video_codec_preference_1";
    static final String TAG_VIDEO_CODEC_PREFERENCE_2    = "video_codec_preference_2";
    static final String TAG_VIDEO_CODEC_PREFERENCE_3    = "video_codec_preference_3";
    static final String TAG_LOCAL_SIGNALING_PORT        = "local_signaling_port";
    static final String TAG_SIP_E_164_PHONE_NUMBER  	= "E_164_phone_number"; // added by CDRC
    
    //Under TAG_RM
    // provisioning
    static final String TAG_RM_SERVICE_URI          = "rm_service_uri";
    
    ///End of TAG_QTALK
    public static class ProvisionInfo
    {
        //general:provisioning
    	public String config_file_forced_provision_periodic_timer       = null;
    	public String general_period_check_timer             			= null;
    	public String short_period_check_timer             				= null;
        public String config_file_download_error_retry_timer    		= null;
        //general:sip
        public String sip_tos_setting    				= null;// added by CDRC
        public String sip_address_mapping_mechanism    	= null;// added by CDRC
        public String dns_retry_counts_to_failover    	= null;// added by CDRC
        //generic:rtp
        public int    audio_rtp_tos_setting        = 0;
        public int    video_rtp_tos_setting        = 0;
        public int    rtp_audio_port_range_start   = 20000;
        public int    rtp_audio_port_range_end     = 20100;
        public int    rtp_video_port_range_start   = 21000;
        public int    rtp_video_port_range_end     = 21100;
        public int    outgoing_max_bandwidth       = 100;
        public int    incoming_max_bandwidth       = 100;
        public int    auto_bandwidth               = 1;
        public int    video_mtu                    = 1380;
        public int    video_fec_enable             = 1;
        public int    video_rfc4588_enable         = 1;
        public int    video_rfc4588_rtxtime        = 500;
        //generic:lines        
        public int    prack_support                = 0;
        public int    info_support                 = 1;
        public int    dtmf_transmit_method         = 2;
        public int    registration_expiration_time = 3600;
        public int    registration_resend_time     = 300;
        public int    challenge_sip_invite         = 0;
        public int    session_timer_enable         = 0;
        public int    session_expire_time          = 900;
        public int    minimum_session_expire_time  = 90;
        
        //vc_service:provisioning
        public String provision_uri       		= null;// added by CDRC
    	public String phonebook_uri     		= null;// added by CDRC
    	public String firmware_uri    			= null; // added by CDRC
    	public String log_uri        			= null; // added by CDRC
    	public String x264_url       			= null;
    	//vc_service:sip
    	public String display_name              = null;
        public String sip_id                    = "";
        public String authentication_id         = "";
        public String authentication_password   = null;
        public String outbound_proxy_address    = null;
        public int    outbound_proxy_port       = 5060;
        public String proxy_server_address      = null;
        public int    proxy_server_port         = 5060;
        public String sip_realm                 = null;
        public String audio_codec_preference_1  = null;
        public String audio_codec_preference_2  = null;
        public String audio_codec_preference_3  = null;
        public String audio_codec_preference_4  = null;
        public String video_codec_preference_1  = null;
        public String video_codec_preference_2  = null;
        public String video_codec_preference_3  = null;
        public int    local_signaling_port      = 5060;
        public String E_164_phone_number    	= null;// added by CDRC
        
    	//rm:provisioning
        public String rm_service_uri        = null;
    	public String session = null;
    	public String key = null;
    	public String lastProvisionFileTime             = null;
        public String lastPhonebookFileTime    			= null;
        public String provision_keepalive_expiretime	= null;    
    }

    //extracts parameters in class ProvisionInfo
    public static ProvisionInfo parseProvisionConfig(String provisionConfigPath) throws ParserConfigurationException, SAXException, IOException
    {
    	Log.d(DEBUGTAG,"parseProvisionConfig("+provisionConfigPath+"), enter");
    	
        ProvisionInfo provision_info = new ProvisionInfo();
        
        // parse ID.xml
        DocumentBuilder documentBuilder  = null;
        Document document        = null;
        NodeList elements        = null;
        int iElementLength= 0;
        documentBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        if(documentBuilder != null)
        {
            document = documentBuilder.parse(new File(provisionConfigPath));
            document.getDocumentElement().normalize(); 
        }
        if(document != null)
            elements  = document.getElementsByTagName(TAG_GENERIC);
        if(elements != null)
        {
            iElementLength = elements.getLength();
            if (iElementLength == 0 )
            {
            	Log.d(DEBUGTAG, "1");
                return null;
            }
            
            Log.d(DEBUGTAG,"iElementLength:"+iElementLength);
            for (int i = 0; i < iElementLength ; i++) 
            {
            	
                Node node = elements.item(i);
                NodeList properties = node.getChildNodes();
                Log.d(DEBUGTAG,"properties.getLength():"+properties.getLength());
                for(int j = 0; j< properties.getLength(); j++)
                {
                    Node properties_node = properties.item(j);
                    NodeList property_list = properties_node.getChildNodes();
                    String properties_node_name = properties_node.getNodeName();
                    if(properties_node_name != null)
                    {
                        if(properties_node_name.equalsIgnoreCase(TAG_PROVISIONING))
                        {
                            //parsing ftp server
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_PROVISIONING_CONFIG_FILE_FORCED_PROVISION_PERIODIC_TIMER))
                                {
                                    provision_info.config_file_forced_provision_periodic_timer = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_PROVISIONING_GENERAL_PERIOD_CHECK_TIMER))
                                {
                                    provision_info.general_period_check_timer = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_PROVISIONING_SHORT_PERIOS_CHECK_TIMER))
                                {
                                    provision_info.short_period_check_timer = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_PROVISIONING_CONFIG_FILE_DOWNLOAD_ERROR_RETRY_TIMER))
                                {
                                    provision_info.config_file_download_error_retry_timer = checkNodeValue(property_list_node.getFirstChild());
                                }
                            }
                        }
                        
                        /// SIP
                        if(properties_node_name.equalsIgnoreCase(TAG_SIP))
                        {
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_SIP_TOS_SETTING))
                                {
                                    provision_info.sip_tos_setting = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_SIP_ADDRESS_MAPPING_MECHANISM))
                                {
                                    provision_info.sip_address_mapping_mechanism = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_DNS_RETRY_COYNTS_TO_FAILOVER))
                                {
                                    provision_info.dns_retry_counts_to_failover = checkNodeValue(property_list_node.getFirstChild());
                                }
                            }
                        }
                        
                        ///RTP
                        if(properties_node_name.equalsIgnoreCase(TAG_RTP))
                        {
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_AUDIO_RTP_TOS_SETTING))
                                {
                                    provision_info.audio_rtp_tos_setting = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_RTP_TOS_SETTING))
                                {
                                    provision_info.video_rtp_tos_setting = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_RTP_AUDIO_PORT_RANGE_START))
                                {
                                    provision_info.rtp_audio_port_range_start = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_RTP_AUDIO_PORT_RANGE_END))
                                {
                                    provision_info.rtp_audio_port_range_end = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_RTP_VIDEO_PORT_RANGE_START))
                                {
                                    provision_info.rtp_video_port_range_start = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_RTP_VIDEO_PORT_RANGE_END))
                                {
                                    provision_info.rtp_video_port_range_end = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_OUTGOING_MAX_BANDWIDTH))
                                {
                                    provision_info.outgoing_max_bandwidth = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_INCOMING_MAX_BANDWIDTH))
                                {
                                    provision_info.incoming_max_bandwidth = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_AUTO_BANDWIDTH))
                                {
                                    provision_info.auto_bandwidth = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_MTU))
                                {
                                    provision_info.video_mtu = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_FEC_ENABLE))
                                {
                                    provision_info.video_fec_enable = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_RFC4588_ENABLE))
                                {
                                    provision_info.video_rfc4588_enable = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_RFC4588_RTXTIME))
                                {
                                    provision_info.video_rfc4588_rtxtime = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                            }
                        }
                        
                        /// lines
                        if(properties_node_name.equalsIgnoreCase(TAG_LINES))
                        {
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_PRACK_SUPPORT))
                                {
                                    provision_info.prack_support = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_PRACK_SUPPORT))
                                {
                                    provision_info.info_support = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_DTMF_TRANSMIT_METHOD))
                                {
                                    provision_info.dtmf_transmit_method = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_REGISTRATION_EXPIRATION_TIME))
                                {
                                    provision_info.registration_expiration_time = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_REGISTRATION_RESEND_TIME))
                                {
                                    provision_info.registration_resend_time = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_CHALLENGE_SIP_INVITE))
                                {
                                    provision_info.challenge_sip_invite = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_SESSION_TIMER_ENABLE))
                                {
                                    provision_info.session_timer_enable = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_SESSION_EXPIRE_TIME))
                                {
                                    provision_info.session_expire_time = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_MINIMUM_SESSION_EXPIRE_TIME))
                                {
                                    provision_info.minimum_session_expire_time = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                            }
                        }
                        
                    }
                }
            }
        }

        //parse provision information for VC
        if(document != null)
            elements  = document.getElementsByTagName(TAG_VC);
        
        if(elements != null)
        {
            iElementLength = elements.getLength();
            if (iElementLength == 0 )
            {
            	Log.d(DEBUGTAG, "2");
                return null;
            }
            for (int i = 0; i < iElementLength ; i++) 
            {
                Node node = elements.item(i);
                NodeList properties = node.getChildNodes();
                for(int j = 0; j< properties.getLength(); j++)
                {
                    Node properties_node = properties.item(j);
                    NodeList property_list = properties_node.getChildNodes();
                    String properties_node_name = properties_node.getNodeName();
                    if(properties_node_name!= null)
                    {
                        if(properties_node_name.equalsIgnoreCase(TAG_PROVISIONING))
                        {
                            //parse phone book information
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_PROVISIONING_URI))
                                {
                                    provision_info.provision_uri = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_PHONEBOOK_URI))
                                {
                                    provision_info.phonebook_uri = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_FIRMWARE_URI))
                                {
                                    provision_info.firmware_uri = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_LOG_URI))
                                {
                                    provision_info.log_uri = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_X264_URL))
                                {
                                    provision_info.x264_url = checkNodeValue(property_list_node.getFirstChild());
                                }
                                
                            }
                        }
                        if(properties_node_name.equalsIgnoreCase(TAG_SIP))
                        {
                            //parse SIP information
                            for(int l = 0 ; l < property_list.getLength() ; l++)
                            {
                                Node property_list_node = property_list.item(l);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_DISPLAY_NAME))
                                {
                                    provision_info.display_name = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_SIP_ID))
                                {
                                    provision_info.sip_id = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_AUTHENTICATION_ID))
                                {
                                    provision_info.authentication_id =checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_AUTHENTICATION_PASSWORD))
                                {
                                    provision_info.authentication_password = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_OUTBOUND_PROXY_ADDRESS))
                                {
                                    provision_info.outbound_proxy_address = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_OUTBOUND_PROXY_PORT))
                                {
                                    provision_info.outbound_proxy_port = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_PROXY_SERVER_ADDRESS))
                                {
                                    provision_info.proxy_server_address = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_PROXY_SERVER_PORT))
                                {
                                    provision_info.proxy_server_port = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_SIP_REALM))
                                {
                                    provision_info.sip_realm = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_AUDIO_CODEC_PREFERENCE_1))
                                {
                                    provision_info.audio_codec_preference_1 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_AUDIO_CODEC_PREFERENCE_2))
                                {
                                    provision_info.audio_codec_preference_2 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_AUDIO_CODEC_PREFERENCE_3))
                                {
                                    provision_info.audio_codec_preference_3 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_AUDIO_CODEC_PREFERENCE_4))
                                {
                                    provision_info.audio_codec_preference_4 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_CODEC_PREFERENCE_1))
                                {
                                    provision_info.video_codec_preference_1 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_CODEC_PREFERENCE_2))
                                {
                                    provision_info.video_codec_preference_2 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_VIDEO_CODEC_PREFERENCE_3))
                                {
                                    provision_info.video_codec_preference_3 = checkNodeValue(property_list_node.getFirstChild());
                                }
                                else if(name.equalsIgnoreCase(TAG_LOCAL_SIGNALING_PORT))
                                {
                                    provision_info.local_signaling_port = Integer.valueOf(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_SIP_E_164_PHONE_NUMBER))
                                {
                                    provision_info.E_164_phone_number = checkNodeValue(property_list_node.getFirstChild());
                                }
                            }
                        }
                    }
                }
            }
        }            
        
        Log.d(DEBUGTAG, "parseProvisionConfig(), exit");
        
        return provision_info;
    }
    
    
    public static ArrayList<QTDepartment> parseProvisionDepartment(String provisionConfigPath) throws ParserConfigurationException, SAXException, IOException
    {
		//TODO 1. initialize an ArrayList here and insert every PB entry into it
		//     2. insert this ArrayList into Database
		ArrayList<QTDepartment> temp_list = new ArrayList<QTDepartment>();
        DocumentBuilder documentBuilder  = null;
        Document document        = null;
        NodeList elements        = null;
        int iElementLength= 0;
        documentBuilder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
        if(documentBuilder != null)
        {
            document = documentBuilder.parse(new File(provisionConfigPath));
            document.getDocumentElement().normalize(); 
        }
        if(document != null)
            elements  = document.getElementsByTagName(TAG_DEP);
        if(elements != null)
        {
            iElementLength = elements.getLength();
            if (iElementLength == 0 )
                return null;
            
            Log.d(DEBUGTAG,"iElementLength:"+iElementLength);
            for (int i = 0; i < iElementLength ; i++) 
            {
            	
                Node node = elements.item(i);
                NodeList properties = node.getChildNodes();
                Log.d(DEBUGTAG,"properties.getLength():"+properties.getLength());
                for(int j = 0; j< properties.getLength(); j++)
                {
                    Node properties_node = properties.item(j);
                    NodeList property_list = properties_node.getChildNodes();
                    String properties_node_name = properties_node.getNodeName();
                    if(properties_node_name != null)
                    {
                        if(properties_node_name.equalsIgnoreCase(TAG_DEPBUDDY))
                        {   	
                        	QTDepartment temp = new QTDepartment();
                            //parsing ftp server
                        	//if(ProvisionSetupActivity.debugMode)	Log.d(DEBUGTAG,"properties_length:"+ property_list.getLength());
                            for(int k = 0 ; k < property_list.getLength() ; k++)
                            {                            	
                                Node property_list_node = property_list.item(k);
                                String name = property_list_node.getNodeName();
                                if(name.equalsIgnoreCase(TAG_DEP_ID))
                                {
                                	temp.setDepID(checkNodeValue(property_list_node.getFirstChild()));
                                }
                                else if(name.equalsIgnoreCase(TAG_DEP_NAME))
                                {
                                	temp.setDepName(checkNodeValue(property_list_node.getFirstChild()));
                                }                              
                            }
                            temp_list.add(temp); 
                        }
                    }
                    
                }
                
            }
            
        }
  
        return temp_list;
    }
       
    private static String checkNodeValue(Node firstChild)
    {
        String result = "";
        try
        {
            if(firstChild != null)
            {
                String temp_result = firstChild.getNodeValue();
                if(temp_result != null && temp_result.trim().length() > 0)
                    result = temp_result;
            }
        }
        catch(Exception e)
        {
            Log.e(DEBUGTAG,"checkNodeValue Error",e);
        }
        return result;
    }

}
