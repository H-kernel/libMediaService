/*
 * RtspSetupReq.cpp
 *
 *  Created on: 2016-5-19
 *      Author:
 */
#include <sstream>
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "ms_engine_common.h"
#include <vms.h>
#include "svs_rtsp_setup_message.h"
#include "svs_rtsp_protocol.h"

using namespace std;

CRtspSetupMessage::CRtspSetupMessage()
{
    m_unMethodType     = RTSP_METHOD_SETUP;
    m_unTransType      = TRANS_PROTOCAL_TCP;
    m_unInterleaveNum  = 0;

    m_usClientPort     = 0;
    m_unDestIp         = (uint32_t)-1;

    m_usServerPort     = 0;
    m_unSrcIp          = (uint32_t)-1;
}

CRtspSetupMessage::~CRtspSetupMessage()
{
}

void CRtspSetupMessage::setTransType(uint32_t unTransType)
{
    m_unTransType = unTransType;
    return;
}

uint32_t CRtspSetupMessage::getTransType() const
{
    return m_unTransType;
}

void CRtspSetupMessage::setInterleaveNum(uint32_t unNum)
{
    m_unInterleaveNum = unNum;
    return;
}
uint32_t CRtspSetupMessage::getInterleaveNum() const
{
    return m_unInterleaveNum;
}
void CRtspSetupMessage::setClientPort(uint16_t usPort)
{
    m_usClientPort = usPort;
    return;
}

uint16_t CRtspSetupMessage::getClientPort() const
{
    return m_usClientPort;
}

void CRtspSetupMessage::setDestinationIp(uint32_t unIp)
{
    m_unDestIp = unIp;
    return;
}

uint32_t CRtspSetupMessage::getDestinationIp() const
{
    return m_unDestIp;
}

void CRtspSetupMessage::setServerPort(uint16_t usPort)
{
    m_usServerPort = usPort;
    return;
}

uint16_t CRtspSetupMessage::getServerPort() const
{
    return m_usServerPort;
}

void CRtspSetupMessage::setSourceIp(uint32_t unIp)
{
    m_unSrcIp = unIp;
    return;
}

uint32_t CRtspSetupMessage::getSourceIp() const
{
    return m_unSrcIp;
}

int32_t CRtspSetupMessage::decodeMessage(CRtspPacket& objRtspPacket)
{
    int32_t nRet = CRtspMessage::decodeMessage(objRtspPacket);
    if (RET_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"decode rtsp setup message fail.");
        return RET_FAIL;
    }


    string strTransport;
    objRtspPacket.getTransPort(strTransport);


    m_unTransType = TRANS_PROTOCAL_UDP;
    string::size_type nStrIndex = strTransport.find(RTSP_TRANSPORT_TCP);
    if (string::npos != nStrIndex)
    {
        m_unTransType = TRANS_PROTOCAL_TCP;

        nStrIndex = strTransport.find(RTSP_TRANSPORT_INTERLEAVED);
        if (string::npos != nStrIndex)
        {
            const char* pTmp = strTransport.c_str() + nStrIndex
                               + strlen(RTSP_TRANSPORT_INTERLEAVED);
            m_unInterleaveNum = (uint32_t)atoi(pTmp);
        }
        else
        {
            SVS_LOG(SVS_LOG_WARNING,"decode rtsp setup message fail, no interleave param.%s.",
                             strTransport.c_str());
            return RET_FAIL;
        }
    }
    else
    {
        // UDP���䷽ʽ
        if (RET_OK != parseUdpTransParam(strTransport))
        {
            SVS_LOG(SVS_LOG_WARNING,"decode rtsp setup message fail, parse udp trans param fail.%s.",
                        strTransport.c_str());
            return RET_FAIL;
        }
    }


    return RET_OK;
}

int32_t CRtspSetupMessage::encodeMessage(std::string &strMessage)
{
    strMessage.clear();
    if (RET_OK != CRtspMessage::encodeMessage(strMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"encode rtsp setup message message fail.");
        return RET_FAIL;
    }

    // Transport
    strMessage += RTSP_TOKEN_STR_TRANSPORT;

    //   �������?(RTP)
    strMessage += RTSP_TRANSPORT_RTP;
    strMessage += RTSP_TRANSPORT_SPEC_SPLITER;
    strMessage += RTSP_TRANSPORT_PROFILE_AVP;

    //  ��������(TCP/UDP)
    if (TRANS_PROTOCAL_TCP == m_unTransType)
    {
        strMessage += RTSP_TRANSPORT_SPEC_SPLITER;
        strMessage += RTSP_TRANSPORT_TCP;
    }
    strMessage += SIGN_SEMICOLON;

    if (TRANS_PROTOCAL_TCP == m_unTransType)
    {
        strMessage += RTSP_TRANSPORT_UNICAST;
        strMessage += SIGN_SEMICOLON;
        strMessage += RTSP_TRANSPORT_INTERLEAVED;
        stringstream strChannelNo;
        strChannelNo << m_unInterleaveNum;
        strMessage += strChannelNo.str() + SIGN_H_LINE;

        strChannelNo.str("");
        strChannelNo << m_unInterleaveNum + 1;
        strMessage += strChannelNo.str();
    }
    else
    {
        strMessage += RTSP_TRANSPORT_UNICAST;
        strMessage += SIGN_SEMICOLON;

        ACE_INET_Addr addr;
        addr.set(m_usClientPort, m_unDestIp);
        strMessage += RTSP_TRANSPORT_CLIENT_PORT;
        stringstream strPort;
        strPort << addr.get_port_number();
        strMessage += strPort.str() + SIGN_H_LINE;
        strPort.str("");
        strPort << addr.get_port_number() + 1;
        strMessage += strPort.str();

        if (RTSP_MSG_RSP == getMsgType())
        {
            // ��Ӧ��Ϣ������server_port��source�ֶ�
            strMessage += SIGN_SEMICOLON;

            addr.set(m_usServerPort, m_unSrcIp);
#if 0
            // source����
            strMessage += RTSP_TRANSPORT_SOURCE;
            strMessage += addr.get_host_addr();
            strMessage += SIGN_SEMICOLON;
#endif
            // server_port����
            strMessage += RTSP_TRANSPORT_SERVER_PORT;
            strPort.str("");
            strPort << addr.get_port_number();
            strMessage += strPort.str() + SIGN_H_LINE;
            strPort.str("");
            strPort << addr.get_port_number() + 1;
            strMessage += strPort.str();
        }
    }
    strMessage += RTSP_END_TAG; // ����ֻ��Transport����

    strMessage += RTSP_END_TAG; // ��Ϣ����

    SVS_LOG(SVS_LOG_DEBUG,"encode setup message:\n%s", strMessage.c_str());
    return RET_OK;
}

int32_t CRtspSetupMessage::parseUdpTransParam(const std::string &strTransport)
{
    string strPort  = "";
    string strIp    = "";

    // ����client_port.
    string strName = RTSP_TRANSPORT_CLIENT_PORT;
    if (RET_OK != parsePort(strTransport, strName, strPort))
    {
        SVS_LOG(SVS_LOG_WARNING,"get client_port fail.");
        return RET_FAIL;
    }
    m_usClientPort = (uint16_t)atoi(strPort.c_str());
    SVS_LOG(SVS_LOG_INFO,"client port: %d", m_usClientPort);

    // ���� destination ip.
    strName = RTSP_TRANSPORT_DESTINATIION;
    if (RET_OK == parseIp(strTransport, strName, strIp))
    {
        m_unDestIp = ACE_OS::inet_addr(strIp.c_str());
        m_unDestIp = ACE_NTOHL(m_unDestIp);
        SVS_LOG(SVS_LOG_INFO,"destination ip: %s", strIp.c_str());
    }
    else
    {
        SVS_LOG(SVS_LOG_INFO,"no destination ip in transport.");
    }

    // �����������Ϣ��ֱ�ӷ��أ���Ӧ��Ϣ�л���source��server_port��Ҫ����
    if (RTSP_MSG_REQ == getMsgType())
    {
        return RET_OK;
    }

    // ����server_port.
    strName = RTSP_TRANSPORT_SERVER_PORT;
    if (RET_OK != parsePort(strTransport, strName, strPort))
    {
        SVS_LOG(SVS_LOG_WARNING,"get server_port fail.");
        return RET_FAIL;
    }
    m_usServerPort = (uint16_t) atoi(strPort.c_str());
    SVS_LOG(SVS_LOG_INFO,"server port: %d", m_usServerPort);

    // ���� source ip.
    strName = RTSP_TRANSPORT_SOURCE;
    if (RET_OK == parseIp(strTransport, strName, strIp))
    {
        m_unSrcIp = ACE_OS::inet_addr(strIp.c_str());
        m_unSrcIp = ACE_NTOHL(m_unSrcIp);
        SVS_LOG(SVS_LOG_INFO,"source ip: %s", strIp.c_str());
    }
    else
    {
        SVS_LOG(SVS_LOG_INFO,"no source ip in transport.");
    }

    return RET_OK;
}

int32_t CRtspSetupMessage::parsePort(const string &srcStr,
                                 const string &strName,
                                 std::string &strValue) const
{
    uint32_t nLength = srcStr.length();
    uint32_t nKeyLength = strName.length();
    string::size_type nIndexKey = srcStr.find(strName);

    if (string::npos != nIndexKey)
    {
        string::size_type nIndexSemicolon = srcStr.find(SIGN_SEMICOLON, nIndexKey);
        if (string::npos != nIndexSemicolon)
        {
            strValue = srcStr.substr(nIndexKey + nKeyLength,
                                    (nIndexSemicolon - nIndexKey) - nKeyLength);
        }
        else
        {
            strValue = srcStr.substr(nIndexKey + nKeyLength,
                                    (nLength - nIndexKey) - nKeyLength);
        }

        string::size_type nIndexHLine = strValue.find(SIGN_H_LINE);
        if (string::npos != nIndexHLine)
        {
            strValue = strValue.substr(0, nIndexHLine);
        }

        M_COMMON::trimString(strValue);
    }
    else
    {
        SVS_LOG(SVS_LOG_WARNING,"can't find[%s] in string[%s].", strValue.c_str(), srcStr.c_str());
        return RET_FAIL;
    }

    return RET_OK;
}

int32_t CRtspSetupMessage::parseIp(const string &srcStr,
                               const string &strName,
                               std::string &strValue) const
{
    uint32_t nKeyLength = strName.length();
    string::size_type nIndexKey = srcStr.find(strName);

    if (string::npos != nIndexKey)
    {
        // ���ip���滹��";",��ȡ������IP; ����ȡ���������ַ�.
        string::size_type nIndexSemicolon = srcStr.find(SIGN_SEMICOLON, nIndexKey);
        if (string::npos != nIndexSemicolon)
        {
            strValue = srcStr.substr(nIndexKey + nKeyLength,
                                     (nIndexSemicolon - nIndexKey) - nKeyLength);
        }
        else
        {
            strValue = srcStr.substr(nIndexKey + nKeyLength,
                                     (srcStr.length() - nIndexKey) - nKeyLength);
        }

        // ȥ�� destination ip ��Ļس����У��Լ�ǰ��ո�.
        M_COMMON::trimString( strValue);

        // �ж�ip�ĺϷ���.
        if (ACE_OS::inet_addr(strValue.c_str()) == 0)
        {
            SVS_LOG(SVS_LOG_WARNING,"parse ip[%s] fail, invalid value param[%s].",
                            strName.c_str(), strValue.c_str());
            return RET_FAIL;
        }
    }

    return RET_OK;
}
