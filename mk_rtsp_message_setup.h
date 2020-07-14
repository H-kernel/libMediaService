/*
 * RtspSetupReq.h
 *
 *  Created on: 2016-5-19
 *      Author:
 */

#ifndef RTSPSETUPREQ_H_
#define RTSPSETUPREQ_H_

#include "svs_ace_header.h"
#include "svs_rtsp_message.h"

class CRtspSetupMessage: public mk_rtsp_message
{
public:
    CRtspSetupMessage();
    virtual ~CRtspSetupMessage();

    void setTransType(uint32_t unTransType);

    uint32_t getTransType() const;

    void setInterleaveNum(uint32_t unNum);

    uint32_t getInterleaveNum() const;

    void setClientPort(uint16_t usPort);

    uint16_t getClientPort() const;

    void setDestinationIp(uint32_t unIp);

    uint32_t getDestinationIp() const;

    void setServerPort(uint16_t usPort);

    uint16_t getServerPort() const;

    void setSourceIp(uint32_t unIp);

    uint32_t getSourceIp() const;

    int32_t decodeMessage(CRtspPacket& objRtspPacket);

    int32_t encodeMessage(std::string &strMessage);
private:
    int32_t parseUdpTransParam(const std::string &strTransport);

    int32_t parsePort(const std::string &srcStr, const std::string &strName, std::string &strValue) const;

    int32_t parseIp(const std::string &srcStr, const std::string &strName, std::string &strValue) const;
private:
    uint32_t        m_unTransType;              // ����ģʽ: TCP/UDP
    uint32_t        m_unInterleaveNum;         // ��֯ͨ���ţ�����ֻ��RTPͨ���ģ�RTCPĬ��Ϊ+1
    uint16_t        m_usClientPort;             // Rtp�˿ڣ�RTCPĬ��Ϊ+1
    uint32_t        m_unDestIp;
    uint16_t        m_usServerPort;             // Rtp�˿ڣ�RTCPĬ��Ϊ+1
    uint32_t        m_unSrcIp;
};

#endif /* RTSPSETUPREQ_H_ */
