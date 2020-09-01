/*
 * RtpPacket.h
 *
 *  Created on: 2010-12-28
 *      Author:
 */

#ifndef RTPPACKET_H_
#define RTPPACKET_H_

#include <map>
#include "as.h"
#include "mk_rtsp_rtp_head.h"

#define RTP_PACKET_VERSION      2
#define RTP_CSRC_LEN            4
#define RTP_EXTEND_PROFILE_LEN  4


typedef struct
{
    uint16_t  usProfile;
    uint16_t  usLength;
}RTP_EXTEND_HEADER;


class mk_rtp_packet
{
public:
    mk_rtp_packet();
    virtual ~mk_rtp_packet();

    int32_t  ParsePacket(const char* pRtpData,uint32_t ulLen);

    int32_t  GeneratePacket(char* pRtpPacket,uint32_t ulLen);

    uint16_t GetSeqNum()const;

    uint32_t GetTimeStamp()const;

    char     GetPayloadType()const;

    bool     GetMarker()const;

    uint32_t GetSSRC()const;

    uint32_t GetHeadLen()const;

    uint16_t GetPacketType()const;

    uint32_t GetTailLen()const;

    int32_t  SetSeqNum(uint16_t usSeqNum);

    int32_t  SetTimeStamp(uint32_t ulTimeStamp);

    int32_t  SetPayloadType(uint8_t ucPayloadType);

    int32_t  SetMarker(bool bMarker);

    void     SetSSRC(uint32_t unSsrc);

    static uint16_t GetSeqNum(const char* pMb);

private:
    int32_t  GetVersion(char& cVersion)const;

    int32_t  CheckVersion()const;

    int32_t  SetVersion(uint8_t ucVersion);

    int32_t  SetPadding(uint8_t ucPadding);
    
    int32_t  ParsePacketType();
private:
    char*                   m_pRtpData;
    RTP_FIXED_HEADER*       m_pFixedHead;
    RTP_EXTEND_HEADER*      m_pExtHead;

    uint32_t                m_ulPacketLen;
    uint32_t                m_ulHeadLen;
    uint32_t                m_ulTailLen;
};

#endif /* RTPPACKET_H_ */
