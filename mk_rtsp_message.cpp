/*
 * RtspMessage.cpp
 *
 *  Created on: 2016-5-17
 *      Author:
 */
#include <sstream>
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "ms_engine_common.h"

#include "svs_rtsp_message.h"
#include "svs_rtsp_protocol.h"
CRtspMessage::CRtspMessage()
{
    m_unMethodType  = RTSP_INVALID_MSG;
    m_unMsgType     = RTSP_MSG_REQ;
    m_unCSeq        = 0;
    m_strSession    = "";
    m_unStatusCode  = RTSP_STATUS_CODES_BUTT;
    m_strRtspUrl    = "";

    m_unContentLength = 0 ;
    m_strContetType   = "";
}

CRtspMessage::~CRtspMessage()
{
}

void CRtspMessage::encodeCommonResp(uint32_t unStatusCode,
                                     uint32_t unCseq,
                                     uint32_t unSession,
                                     std::string &strMsg)
{

    if (unStatusCode >= RTSP_STATUS_CODES_BUTT)
    {
        SVS_LOG(SVS_LOG_WARNING,"encode rtsp common response fail, status code[%u] invalid.",
                        unStatusCode);
        return;
    }
    strMsg = RTSP_PROTOCOL_VERSION;
    strMsg += " ";
    strMsg += CRtspProtocol::m_RtspCode[unStatusCode];
    strMsg += RTSP_END_TAG;

    // Cseq
    std::stringstream strCSeq;
    strCSeq << unCseq;
    strMsg += RTSP_TOKEN_STR_CSEQ;
    strMsg += strCSeq.str();
    strMsg += RTSP_END_TAG;


    if (0 != unSession)
    {
        std::stringstream strSession;
        strSession << unSession;
        strMsg += RTSP_TOKEN_STR_SESSION;
        strMsg += strSession.str();
        strMsg += RTSP_END_TAG;
    }


    // Date
    char szDate[RTSP_MAX_TIME_LEN + 1] = { 0 };
    CURTIMESTR(szDate, RTSP_MAX_TIME_LEN);
    strMsg += RTSP_TOKEN_STR_DATE;
    strMsg.append(szDate);
    strMsg += RTSP_END_TAG;

    // Server
    strMsg += RTSP_TOKEN_STR_SERVER;
    strMsg += RTSP_SERVER_AGENT;
    strMsg += RTSP_END_TAG;

    strMsg += RTSP_END_TAG;

    SVS_LOG(SVS_LOG_DEBUG,"success to encode common response.\n%s", strMsg.c_str());
    return;
}

uint32_t CRtspMessage::getMethodType() const
{
    return m_unMethodType;
}

void CRtspMessage::setMsgType(uint32_t unMsgType)
{
    m_unMsgType = unMsgType;
    return;
}

uint32_t CRtspMessage::getMsgType() const
{
    return m_unMsgType;
}

std::string CRtspMessage::getSession() const
{
    return m_strSession;
}

void CRtspMessage::setSession(const std::string &strSession)
{
    m_strSession = strSession;
    return;
}

uint32_t CRtspMessage::getCSeq() const
{
    return m_unCSeq;
}

void CRtspMessage::setCSeq(uint32_t unCSeq)
{
    m_unCSeq = unCSeq;
    return;
}

void CRtspMessage::setStatusCode(uint32_t unCode)
{
    m_unStatusCode = unCode;
    return;
}

uint32_t CRtspMessage::getStatusCode() const
{
    return m_unStatusCode;
}


void CRtspMessage::setRtspUrl(const std::string &strUrl)
{
    m_strRtspUrl = strUrl;
    return;
}

std::string CRtspMessage::getRtspUrl() const
{
    return m_strRtspUrl;
}


uint32_t CRtspMessage::getContentLength() const
{
    return m_unContentLength ;
}
std::string CRtspMessage::getContetType() const
{
    return m_strContetType;
}
std::string CRtspMessage::getBody() const
{
    return m_strBody ;
}
void CRtspMessage::setBody(std::string& strContentType,std::string& strContent)
{
    m_strContetType = strContentType;
    m_unContentLength = strContent.size();
    m_strBody = strContent;
}
int32_t CRtspMessage::decodeMessage(CRtspPacket& objRtspPacket)
{
    if (!objRtspPacket.isResponse())
    {
        objRtspPacket.getRtspUrl(m_strRtspUrl);
    }
    else
    {
        m_unStatusCode = objRtspPacket.getRtspStatusCode();
    }

    m_unCSeq = objRtspPacket.getCseq();

    uint64_t ullSessionID = objRtspPacket.getSessionID();
    char szSessionID[ID_LEN+1]={0};
    (void)ACE_OS::snprintf( szSessionID,ID_LEN,"%llu",ullSessionID);
    m_strSession = szSessionID;

    m_unContentLength = objRtspPacket.getContentLength();
    objRtspPacket.getContent(m_strBody);
    objRtspPacket.getContentType(m_strContetType);

    return AS_ERROR_CODE_OK;
}

int32_t CRtspMessage::encodeMessage(std::string &strMessage)
{
    if (RTSP_MSG_REQ == m_unMsgType)
    {
        if (getMethodType() >= RTSP_REQ_METHOD_NUM)
        {
            SVS_LOG(SVS_LOG_WARNING,"encode rtsp request fail, method type[%d] invalid.",
                            getMethodType());
            return AS_ERROR_CODE_FAIL;
        }

        strMessage += CRtspProtocol::m_strRtspMethod[getMethodType()];
        strMessage += " " + m_strRtspUrl + " " + RTSP_PROTOCOL_VERSION;
        strMessage += RTSP_END_TAG;
    }
    else
    {

        if (m_unStatusCode >= RTSP_STATUS_CODES_BUTT)
        {
            SVS_LOG(SVS_LOG_WARNING,"encode rtsp response fail, status code[%u] invalid.",
                            m_unStatusCode);
            return AS_ERROR_CODE_FAIL;
        }
        strMessage = RTSP_PROTOCOL_VERSION;
        strMessage += " ";
        strMessage += CRtspProtocol::m_RtspCode[m_unStatusCode];
        strMessage += RTSP_END_TAG;
    }

    // Cseq
    std::stringstream strCSeq;
    strCSeq<<m_unCSeq;
    strMessage += RTSP_TOKEN_STR_CSEQ;
    strMessage += strCSeq.str();
    strMessage += RTSP_END_TAG;

    if ("" != m_strSession)
    {
        strMessage += RTSP_TOKEN_STR_SESSION;
        strMessage += m_strSession;
        strMessage += RTSP_END_TAG;
    }

    if (RTSP_MSG_REQ == m_unMsgType)
    {
        strMessage += RTSP_TOKEN_STR_USERAGENT;
        strMessage += RTSP_SERVER_AGENT;
        strMessage += RTSP_END_TAG;
    }
    else
    {
        // Date
        char szDate[RTSP_MAX_TIME_LEN + 1] = {0};
        CURTIMESTR(szDate, RTSP_MAX_TIME_LEN);
        strMessage += RTSP_TOKEN_STR_DATE;
        strMessage.append(szDate);
        strMessage += RTSP_END_TAG;

        // Server
        strMessage += RTSP_TOKEN_STR_SERVER;
        strMessage += RTSP_SERVER_AGENT;
        strMessage += RTSP_END_TAG;
    }

    return AS_ERROR_CODE_OK;
}
