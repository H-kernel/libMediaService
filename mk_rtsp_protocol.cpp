/*
 * RtspMessageFactory.cpp
 *
 *  Created on: 2016-5-17
 *      Author:
 */
#include <string.h>
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "svs_rtsp_protocol.h"

#include "svs_rtsp_options_message.h"
#include "svs_rtsp_describe_message.h"
#include "svs_rtsp_setup_message.h"
#include "svs_rtsp_play_message.h"
#include "svs_rtsp_pause_message.h"
#include "svs_rtsp_teardown_message.h"
#include "svs_rtsp_announce_message.h"
#include "svs_rtsp_record_message.h"
#include "svs_rtsp_get_parameter_message.h"

std::string mk_rtsp_protocol::m_RtspCode[] = RTSP_CODE_STRING;
std::string mk_rtsp_protocol::m_strRtspMethod[] = RTSP_METHOD_STRING;

char *private_strchr(const char *s, char *c)
{
    return strchr((char*)s, (int64_t)c);   //lint !e605
}

mk_rtsp_protocol::mk_rtsp_protocol()
{
    m_unCSeq = 1;
}

mk_rtsp_protocol::~mk_rtsp_protocol()
{
}


int32_t mk_rtsp_protocol::init() const
{
    return AS_ERROR_CODE_OK;
}


uint32_t mk_rtsp_protocol::getCseq()
{
    ACE_Guard<ACE_Thread_Mutex> locker(m_CseqMutex);
    return m_unCSeq++;
}

int32_t mk_rtsp_protocol::saveSendReq(uint32_t unCSeq, uint32_t unReqMethodType)
{
    ACE_Guard<ACE_Thread_Mutex>    m_locker(m_MapMutex);
    if (0 != m_CseqReqMap.count(unCSeq))
    {
        AS_LOG(AS_LOG_WARNING,"save request message fail, cseq[%u] already saved, method[%u].",
                        unCSeq, unReqMethodType);
        return AS_ERROR_CODE_FAIL;
    }

    m_CseqReqMap[unCSeq] = unReqMethodType;
    AS_LOG(AS_LOG_INFO,"save request message cseq[%u] method[%u].", unCSeq, unReqMethodType);
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_protocol::IsParsable(const char* pMsgData, uint32_t unDataLen) const
{
    uint32_t unMsgLen = 0;
    if (0 != CRtspPacket::checkRtsp(pMsgData,unDataLen ,unMsgLen))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp protocol parse rtsp message fail.");
        return -1;
    }

    if (!unMsgLen)
    {
        return 0;
    }

    if (unDataLen < unMsgLen)
    {
        return 0;
    }

    AS_LOG(AS_LOG_INFO,"rtsp protocol parse rtsp return message length[%u].", unMsgLen);
    return (int32_t)unMsgLen;
}


int32_t mk_rtsp_protocol::DecodeRtspMessage(const char* pMsgData,
                                     uint32_t unDataLen,
                                     mk_rtsp_message *&pMsg)
{
    if ((NULL == pMsgData) || (0 == unDataLen))
    {
        return AS_ERROR_CODE_FAIL;
    }

    pMsg = NULL;
    std::string strRtspMsg = "";
    strRtspMsg.append(pMsgData, unDataLen);
    AS_LOG(AS_LOG_DEBUG,"start decode rtsp message:\n%s", strRtspMsg.c_str());


    CRtspPacket objRtspPacket;
    if (0 != objRtspPacket.parse(pMsgData,unDataLen))
    {
        AS_LOG(AS_LOG_WARNING,"decode rtsp message fail");
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = AS_ERROR_CODE_OK;
    if (RtspResponseMethod > objRtspPacket.getMethodIndex())
    {
        nRet = parseRtspRequest(objRtspPacket, pMsg);
    }
    else  if (RtspResponseMethod == objRtspPacket.getMethodIndex())
    {
        nRet = parseRtspResponse(objRtspPacket, pMsg);;
    }
    else
    {
        AS_LOG(AS_LOG_WARNING,"decode rtsp message fail, choice[%d] invalid.", objRtspPacket.getMethodIndex());
        return AS_ERROR_CODE_FAIL;
    }
    return nRet;
}

int32_t mk_rtsp_protocol::parseRtspRequest(CRtspPacket& objRtspPacket,
                                    mk_rtsp_message *&pMessage) const
{

    pMessage = NULL;
    try
    {
        switch(objRtspPacket.getMethodIndex())
        {
        case RtspOptionsMethod:
            pMessage = new CRtspOptionsMessage();
            break;
        case RtspDescribeMethod:
            pMessage = new CRtspDescribeMessage();
            break;
        case RtspSetupMethod:
            pMessage = new CRtspSetupMessage();
            break;
        case RtspPlayMethod:
            pMessage = new CRtspPlayMessage();
            break;
        case RtspPauseMethod:
            pMessage = new CRtspPauseMessage();
            break;
        case RtspTeardownMethod:
            pMessage = new CRtspTeardownMessage();
            break;
        case RtspAnnounceMethod:
            pMessage = new CRtspAnnounceMessage();
            break;
        case RtspGetParameterMethod:
            pMessage = new CRtspGerParamMessage();
            break;
        case RtspRecordMethod:
            pMessage = new CRtspRecordMessage();
            break;
        default:
            AS_LOG(AS_LOG_WARNING,"rtsp protocol not accepted method[%u].",
                    objRtspPacket.getMethodIndex());
            break;
        }
    }catch(...)
    {
    }

    if (!pMessage)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp protocol create rtsp request message fail, method[%u].",
                            objRtspPacket.getMethodIndex());
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != pMessage->decodeMessage(objRtspPacket))
    {
        delete pMessage;
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_protocol::parseRtspResponse(CRtspPacket& objRtspPacket,
                                    mk_rtsp_message *&pMessage)
{

    uint32_t unCseq = objRtspPacket.getCseq();
    ACE_Guard<ACE_Thread_Mutex> locker(m_MapMutex);
    REQ_TYPE_MAP_ITER iter = m_CseqReqMap.find(unCseq);
    if (m_CseqReqMap.end() == iter)
    {
        AS_LOG(AS_LOG_WARNING,"No corresponding request to this response msg, cseq=%u.", unCseq);
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t unMethodType = iter->second;
    m_CseqReqMap.erase(iter);

    pMessage = NULL;
    try
    {
        switch(unMethodType)
        {
        case  RTSP_METHOD_OPTIONS:
            pMessage = new CRtspOptionsMessage;
            break;
        case RTSP_METHOD_DESCRIBE:
            pMessage = new CRtspDescribeMessage;
            break;
        case RTSP_METHOD_SETUP:
            pMessage = new CRtspSetupMessage;
            break;
        case RTSP_METHOD_PLAY:
            pMessage = new CRtspPlayMessage;
            break;
        case RTSP_METHOD_PAUSE:
            pMessage = new CRtspPauseMessage;
            break;
        case RTSP_METHOD_TEARDOWN:
            pMessage = new CRtspTeardownMessage;
            break;
        case RTSP_METHOD_ANNOUNCE:
            pMessage = new CRtspAnnounceMessage;
            break;
        case RtspRecordMethod:
            pMessage = new CRtspRecordMessage();
            break;
        default:
            break;
        }
    }
    catch(...)
    {
    }

    if (!pMessage)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp protocol create rtsp response message fail, CSeq[%u].",
                        unCseq);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != pMessage->decodeMessage(objRtspPacket))
    {
        delete pMessage;
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}
