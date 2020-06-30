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

std::string CRtspProtocol::m_RtspCode[] = RTSP_CODE_STRING;
std::string CRtspProtocol::m_strRtspMethod[] = RTSP_METHOD_STRING;

char *private_strchr(const char *s, char *c)
{
    return strchr((char*)s, (int64_t)c);   //lint !e605
}

CRtspProtocol::CRtspProtocol()
{
    m_unCSeq = 1;
}

CRtspProtocol::~CRtspProtocol()
{
}


int32_t CRtspProtocol::init() const
{
    return RET_OK;
}


uint32_t CRtspProtocol::getCseq()
{
    ACE_Guard<ACE_Thread_Mutex> locker(m_CseqMutex);
    return m_unCSeq++;
}

int32_t CRtspProtocol::saveSendReq(uint32_t unCSeq, uint32_t unReqMethodType)
{
    ACE_Guard<ACE_Thread_Mutex>    m_locker(m_MapMutex);
    if (0 != m_CseqReqMap.count(unCSeq))
    {
        SVS_LOG(SVS_LOG_WARNING,"save request message fail, cseq[%u] already saved, method[%u].",
                        unCSeq, unReqMethodType);
        return RET_FAIL;
    }

    m_CseqReqMap[unCSeq] = unReqMethodType;
    SVS_LOG(SVS_LOG_INFO,"save request message cseq[%u] method[%u].", unCSeq, unReqMethodType);
    return RET_OK;
}


int32_t CRtspProtocol::IsParsable(const char* pMsgData, uint32_t unDataLen) const
{
    uint32_t unMsgLen = 0;
    if (0 != CRtspPacket::checkRtsp(pMsgData,unDataLen ,unMsgLen))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp protocol parse rtsp message fail.");
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

    SVS_LOG(SVS_LOG_INFO,"rtsp protocol parse rtsp return message length[%u].", unMsgLen);
    return (int32_t)unMsgLen;
}


int32_t CRtspProtocol::DecodeRtspMessage(const char* pMsgData,
                                     uint32_t unDataLen,
                                     CRtspMessage *&pMsg)
{
    if ((NULL == pMsgData) || (0 == unDataLen))
    {
        return RET_FAIL;
    }

    pMsg = NULL;
    std::string strRtspMsg = "";
    strRtspMsg.append(pMsgData, unDataLen);
    SVS_LOG(SVS_LOG_DEBUG,"start decode rtsp message:\n%s", strRtspMsg.c_str());


    CRtspPacket objRtspPacket;
    if (0 != objRtspPacket.parse(pMsgData,unDataLen))
    {
        SVS_LOG(SVS_LOG_WARNING,"decode rtsp message fail");
        return RET_FAIL;
    }

    int32_t nRet = RET_OK;
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
        SVS_LOG(SVS_LOG_WARNING,"decode rtsp message fail, choice[%d] invalid.", objRtspPacket.getMethodIndex());
        return RET_FAIL;
    }
    return nRet;
}

int32_t CRtspProtocol::parseRtspRequest(CRtspPacket& objRtspPacket,
                                    CRtspMessage *&pMessage) const
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
            SVS_LOG(SVS_LOG_WARNING,"rtsp protocol not accepted method[%u].",
                    objRtspPacket.getMethodIndex());
            break;
        }
    }catch(...)
    {
    }

    if (!pMessage)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp protocol create rtsp request message fail, method[%u].",
                            objRtspPacket.getMethodIndex());
        return RET_FAIL;
    }

    if (RET_OK != pMessage->decodeMessage(objRtspPacket))
    {
        delete pMessage;
        return RET_FAIL;
    }

    return RET_OK;
}

int32_t CRtspProtocol::parseRtspResponse(CRtspPacket& objRtspPacket,
                                    CRtspMessage *&pMessage)
{

    uint32_t unCseq = objRtspPacket.getCseq();
    ACE_Guard<ACE_Thread_Mutex> locker(m_MapMutex);
    REQ_TYPE_MAP_ITER iter = m_CseqReqMap.find(unCseq);
    if (m_CseqReqMap.end() == iter)
    {
        SVS_LOG(SVS_LOG_WARNING,"No corresponding request to this response msg, cseq=%u.", unCseq);
        return RET_FAIL;
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
        SVS_LOG(SVS_LOG_WARNING,"rtsp protocol create rtsp response message fail, CSeq[%u].",
                        unCseq);
        return RET_FAIL;
    }

    if (RET_OK != pMessage->decodeMessage(objRtspPacket))
    {
        delete pMessage;
        return RET_FAIL;
    }

    return RET_OK;
}
