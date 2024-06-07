package com.quanta.qtalk.call;
public class MDP
{
     public String MimeType      = null;
     public int    PayloadID = 0;
     public int    ClockRate = 0;
     public int    BitRate = 0;
     
     public FECInfo FEC_info;			//eason - add FEC info
     public SRTPInfo SRTP_info;        //Iron - add SRTP_info

//     public MDP[] MDPsTransform(org.qtalk.call.MDP[] MDPs)
//     {
//         MDP[] mdps= null;
//         for(int i = 0; i < MDPs.length; i++)
//         {
//             mdps[i].MimeType = MDPs[i].MimeType;
//             mdps[i].PayloadID = MDPs[i].PayloadID;
//             mdps[i].ClockRate = MDPs[i].ClockRate;
//             mdps[i].BitRate = MDPs[i].BitRate;
//         }
//         return mdps;
//     }
     public MDP(){};
     public MDP(String mimeType, int payloadId, int clockRate)
     {
          MimeType = mimeType;
          PayloadID = payloadId;
          ClockRate = clockRate;
     }
     public MDP(String mimeType, int payloadId, int clockRate, int bitRate)
    {
        MimeType = mimeType;
        PayloadID = payloadId;
        ClockRate = clockRate;
        BitRate = bitRate;
    }
     
     public MDP(String mimeType, int payloadId, int clockRate, int bitRate, int fecCtrl, int fecSendPType, int fecRecvPType){
         MimeType = mimeType;
         PayloadID = payloadId;
         ClockRate = clockRate;
         BitRate = bitRate;
         
         FEC_info = new FECInfo(fecCtrl, fecSendPType, fecRecvPType);
     }
     public MDP(String mimeType, int payloadId, int clockRate, int bitRate, int fecCtrl, int fecSendPType, int fecRecvPType, 
    		     boolean enableSRTP, String SRTPSendKey, String SRTPReceiveKey){
         MimeType = mimeType;
         PayloadID = payloadId;
         ClockRate = clockRate;
         BitRate = bitRate;
    	 
         FEC_info = new FECInfo(fecCtrl, fecSendPType, fecRecvPType);
         SRTP_info = new SRTPInfo(enableSRTP, SRTPSendKey, SRTPReceiveKey);
     }
     
     public String toString()
     {
          StringBuffer sb = new StringBuffer();
          sb.append("MIME:");
          sb.append(MimeType);
          sb.append(",");
          sb.append("PayloadID:");
          sb.append(PayloadID);
          sb.append(",");
          sb.append("ClockRate:");
          sb.append(ClockRate);
          sb.append("BitRate:");
          sb.append(BitRate);
          return sb.toString();
     }
     public static MDP getFirstMatchedMDP(MDP[] remote,MDP[] local)
     {
          MDP result = null;
          if (remote!=null && local!=null)
          {
               for (int j=0;j<local.length;j++)
               {
                    for (int i=0;i<remote.length;i++)
                    {
                         if (remote[i].MimeType!=null &&
                              local[j].MimeType!=null &&
                              remote[i].MimeType.compareToIgnoreCase(local[j].MimeType)==0 &&
                              remote[i].ClockRate == local[j].ClockRate &&
                              remote[i].BitRate == local[j].BitRate)
                         {
                              result = new MDP();
                              result.ClockRate = remote[i].ClockRate;
                              result.MimeType = new String(remote[i].MimeType);
                              result.PayloadID = remote[i].PayloadID;
                              result.BitRate = remote[i].BitRate;
                              return result;
                         }
                    }
               }
          }
          return result;
     }
}
