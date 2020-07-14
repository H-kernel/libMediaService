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
mk_rtsp_message::mk_rtsp_message()
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

mk_rtsp_message::~mk_rtsp_message()
{
}

void mk_rtsp_message::encodeCommonResp(uint32_t unStatusCode,
                                     uint32_t unCseq,
                                     uint32_t unSession,
                                     std::string &strMsg)
{

    if (unStatusCode >= RTSP_STATUS_CODES_BUTT)
    {
        AS_LOG(AS_LOG_WARNING,"encode rtsp common response fail, status code[%u] invalid.",
                        unStatusCode);
        return;
    }
    strMsg = RTSP_PROTOCOL_VERSION;
    strMsg += " ";
    strMsg += mk_rtsp_protocol::m_RtspCode[unStatusCode];
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

    AS_LOG(AS_LOG_DEBUG,"success to encode common response.\n%s", strMsg.c_str());
    return;
}

uint32_t mk_rtsp_message::getMethodType() const
{
    return m_unMethodType;
}

void mk_rtsp_message::setMsgType(uint32_t unMsgType)
{
    m_unMsgType = unMsgType;
    return;
}

uint32_t mk_rtsp_message::getMsgType() const
{
    return m_unMsgType;
}

std::string mk_rtsp_message::getSession() const
{
    return m_strSession;
}

void mk_rtsp_message::setSession(const std::string &strSession)
{
    m_strSession = strSession;
    return;
}

uint32_t mk_rtsp_message::getCSeq() const
{
    return m_unCSeq;
}

void mk_rtsp_message::setCSeq(uint32_t unCSeq)
{
    m_unCSeq = unCSeq;
    return;
}

void mk_rtsp_message::setStatusCode(uint32_t unCode)
{
    m_unStatusCode = unCode;
    return;
}

uint32_t mk_rtsp_message::getStatusCode() const
{
    return m_unStatusCode;
}


void mk_rtsp_message::setRtspUrl(const std::string &strUrl)
{
    m_strRtspUrl = strUrl;
    return;
}

std::string mk_rtsp_message::getRtspUrl() const
{
    return m_strRtspUrl;
}


uint32_t mk_rtsp_message::getContentLength() const
{
    return m_unContentLength ;
}
std::string mk_rtsp_message::getContetType() const
{
    return m_strContetType;
}
std::string mk_rtsp_message::getBody() const
{
    return m_strBody ;
}
void mk_rtsp_message::setBody(std::string& strContentType,std::string& strContent)
{
    m_strContetType = strContentType;
    m_unContentLength = strContent.size();
    m_strBody = strContent;
}
int32_t mk_rtsp_message::decodeMessage(CRtspPacket& objRtspPacket)
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

int32_t mk_rtsp_message::encodeMessage(std::string &strMessage)
{
    if (RTSP_MSG_REQ == m_unMsgType)
    {
        if (getMethodType() >= RTSP_REQ_METHOD_NUM)
        {
            AS_LOG(AS_LOG_WARNING,"encode rtsp request fail, method type[%d] invalid.",
                            getMethodType());
            return AS_ERROR_CODE_FAIL;
        }

        strMessage += mk_rtsp_protocol::m_strRtspMethod[getMethodType()];
        strMessage += " " + m_strRtspUrl + " " + RTSP_PROTOCOL_VERSION;
        strMessage += RTSP_END_TAG;
    }
    else
    {

        if (m_unStatusCode >= RTSP_STATUS_CODES_BUTT)
        {
            AS_LOG(AS_LOG_WARNING,"encode rtsp response fail, status code[%u] invalid.",
                            m_unStatusCode);
            return AS_ERROR_CODE_FAIL;
        }
        strMessage = RTSP_PROTOCOL_VERSION;
        strMessage += " ";
        strMessage += mk_rtsp_protocol::m_RtspCode[m_unStatusCode];
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
