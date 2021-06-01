#ifndef RTCPPACKET_H_
#define RTCPPACKET_H_

#include <map>
#include "as.h"
#include <string>
#include "mk_rtsp_rtp_head.h"

#define RTCP_PACKET_VERSION     2
#define RTCP_LEN_BASE           4
#define RTCP_INVALID_PT         0xFF
#define SDES_ITEM_TYPE_SIZE     1
#define SDES_ITEM_LEN_SIZE      1
#define SDES_ITEM_END_SIZE      2


typedef enum _enRTCP_PACKET_TYPE
{
    RTCP_PACKET_SR      = 200,              // RTCP Sender report
    RTCP_PACKET_RR      = 201,              // RTCP Receiver report
    RTCP_PACKET_SDES    = 202,              // RTCP Source description packet
    RTCP_PACKET_BYE     = 203,              // RTCP Bye packet
    RTCP_PACKET_APP     = 204               // RTCP App packet
}RTCP_PACKET_TYPE;

typedef struct _stRTCP_SR_PACKET
{
    RTCP_FIXED_HEADER   stHeader;
    uint32_t        unReporterSSRC;
    uint64_t        unNtpTimestamp;
    uint32_t        unRtpTimestamp;
    uint32_t        unPacketCount;
    uint32_t        unByteCount;
}RTCP_SR_PACKET;

typedef struct _stRTCP_RR_PACKET
{
    RTCP_FIXED_HEADER   stHeader;
    uint32_t        unReporterSSRC;
    uint32_t        unReporteeSSRC;
    uint32_t        unLossPercent : 8;              // ����������������
    uint32_t        unLostCount   : 24;
    uint32_t        unMaxReceivedSeq;
    uint32_t        unJitter;
    uint32_t        unLastSRTimestamp;
    uint32_t        unDelayFromLastSR;
}RTCP_RR_PACKET;

enum _enSDES_TYPE
{
    SDES_TYPE_END           = 0,            // ��������
    SDES_TYPE_CNAME         = 1             // CNAME
};

typedef struct _stRTCP_SDES_PACKET
{
    RTCP_FIXED_HEADER   stHeader;
    uint32_t        unReporterSSRC;
}RTCP_SDES_PACKET;

#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)

class mk_rtcp_packet
{
public:
    mk_rtcp_packet();

    virtual ~mk_rtcp_packet();

    void setLocalSSRC(uint32_t unSSRC);

    uint32_t getLocalSSRC() const;

    void setPeerSSRC(uint32_t unSSRC);

    uint32_t getPeerSSRC() const;

    void setLocalIp(uint32_t unLocalIp);

public:
    int32_t createReceiverReport(char *pBuffer, uint32_t unMaxLength, uint32_t &reportLength) const;

    int32_t createSenderReport(char *pBuffer, uint32_t unMaxLength,uint32_t &reportLength) ;

private:
    int32_t createSdesPacket(char *pBuffer, uint32_t unMaxLength, uint32_t &reportLength) const;
    uint64_t getNtpTimeforRtcp()const;
private:
    uint32_t        m_unReporterSSRC;
    uint32_t        m_unReporteeSSRC;
    std::string     m_strCname;
    uint64_t        m_ullNtpTime;
};

#endif /* RTCPPACKET_H_ */