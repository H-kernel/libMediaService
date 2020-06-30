/*
 * StreamStdRtpSession.cpp
 *
 *  Created on: 2016-5-20
 *      Author:
 */
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "ms_engine_time.h"
#include "ms_engine_std_rtp_session.h"
#include "ms_engine_session_factory.h"
#include "ms_engine_media_block_buffer.h"
#include "ms_engine_rtp_packet.h"
#include "ms_engine_rtp_tcp_connect.h"
#include "ms_engine_media_data_queue.h"
#include "ms_engine_media_block_buffer.h"
#include "svs_rtsp_announce_message.h"
#include <string>

CStreamStdRtpSession::CStreamStdRtpSession()
{
    m_rtspHandle      = ACE_INVALID_HANDLE;
    m_strRtspUrl      = "";
    m_strRtspSessionId = "";

    m_unVideoInterleaveNum = 0;
    m_unAudioInterleaveNum = 2;
    memset(m_pUdpHandle, 0x0, sizeof(CNetworkHandle*) * HANDLE_TYPE_MAX);
    m_ullPeerStreamID = 0;

    m_ulLastRecvTime  = SVS_GetSecondTime();
    m_unStartTime     = 0;
    m_unTransType     = TRANS_PROTOCAL_UDP;
}

CStreamStdRtpSession::~CStreamStdRtpSession()
{
    try
    {
        (void) CStreamPortManager::instance()->releaseRtpUdpPort(getSpecifyIp(),
                                                                m_pUdpHandle[VIDEO_RTP_HANDLE],
                                                                m_pUdpHandle[VIDEO_RTCP_HANDLE]);
        (void) CStreamPortManager::instance()->releaseRtpUdpPort(getSpecifyIp(),
                                                                m_pUdpHandle[AUDIO_RTP_HANDLE],
                                                                m_pUdpHandle[AUDIO_RTCP_HANDLE]);
        while (!m_TcpSendList.empty())
        {
            ACE_Message_Block *pMsg = m_TcpSendList.front();
            m_TcpSendList.pop_front();
            CMediaBlockBuffer::instance().freeMediaBlock(pMsg);
        }
    }
    catch(...)
    {}
}

int32_t CStreamStdRtpSession::initStdRtpSession(uint64_t ullPeerStreamID,
                                         PLAY_TYPE      enPlayType,
                                         const ACE_INET_Addr &localAddr,
                                         const ACE_INET_Addr &/*peerAddr*/)
{
    m_ullPeerStreamID = ullPeerStreamID;
    CStreamSession *pPeerSession = CStreamSessionFactory::instance()->findSession(m_ullPeerStreamID);
    if (!pPeerSession)
    {
        SVS_LOG(SVS_LOG_WARNING,"init std rtp session[%lld] fail, can't find peer session[%lld].",
                getStreamId(), ullPeerStreamID);
        return AS_ERROR_CODE_FAIL;
    }

    m_stSessionInfo.SessionType = RTSP_SESSION;
    m_enPlayType   = enPlayType;
    m_ulVideoCodeType = pPeerSession->getVideoCodeType();
    m_stSessionInfo.PeerType = PEER_TYPE_CU;
    m_stSessionInfo.RecvStreamID = ullPeerStreamID;
    m_stSessionInfo.TransDirection = TRANS_DIRECTION_SENDONLY;
    m_stSessionInfo.SpecifyIp  = localAddr.get_ip_address();


    m_ulVideoCodeType = pPeerSession->getVideoCodecType();

    CStreamSessionFactory::instance()->releaseSession(pPeerSession);

    setStatus(STREAM_SESSION_STATUS_WAIT_START);

    SVS_LOG(SVS_LOG_INFO,"init std rtp session[%lld] service type[%d] success.",
                    getStreamId(), getPlayType());
    return AS_ERROR_CODE_OK;
}

int32_t CStreamStdRtpSession::startStdRtpSession(const CRtspSetupMessage &rtspMessage)
{
    if (1 < m_unStartTime)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (0 == m_unStartTime)
    {
        m_stSessionInfo.TransProtocol = rtspMessage.getTransType();
        m_strRtspSessionId = rtspMessage.getSession();
        if ("" == m_strRtspUrl)
        {
            m_strRtspUrl = rtspMessage.getRtspUrl();
        }

        m_unTransType = rtspMessage.getTransType();

        SVS_LOG(SVS_LOG_INFO,"session[%lld] start video channel transtype[%d].",
                                    getStreamId(),m_unTransType);

        if (TRANS_PROTOCAL_TCP == rtspMessage.getTransType())
        {
            m_unVideoInterleaveNum  = rtspMessage.getInterleaveNum();
        }
        else
        {
            if (AS_ERROR_CODE_OK != allocMediaPort())
            {
                return AS_ERROR_CODE_FAIL;
            }

            if (AS_ERROR_CODE_OK != startMediaPort())
            {
                return AS_ERROR_CODE_FAIL;
            }

            m_UdpPeerAddr[VIDEO_RTP_HANDLE].set(rtspMessage.getClientPort(),
                                                rtspMessage.getDestinationIp());
            m_UdpPeerAddr[VIDEO_RTCP_HANDLE].set(rtspMessage.getClientPort() + 1,
                                                 rtspMessage.getDestinationIp());
            SVS_LOG(SVS_LOG_INFO,"session[%lld] start video channel[%s:%d].",
                            getStreamId(),
                            m_UdpPeerAddr[VIDEO_RTP_HANDLE].get_host_addr(),
                            m_UdpPeerAddr[VIDEO_RTP_HANDLE].get_port_number());
        }
    }
    else
    {
        if (TRANS_PROTOCAL_TCP == rtspMessage.getTransType())
        {
            m_unAudioInterleaveNum = rtspMessage.getInterleaveNum();
        }
        else
        {
            m_UdpPeerAddr[AUDIO_RTP_HANDLE].set(rtspMessage.getClientPort(),
                                                rtspMessage.getDestinationIp());
            m_UdpPeerAddr[AUDIO_RTCP_HANDLE].set(rtspMessage.getClientPort() + 1,
                                                 rtspMessage.getDestinationIp());
            SVS_LOG(SVS_LOG_INFO,"session[%lld] start audio channel[%s:%d].",
                            getStreamId(),
                            m_UdpPeerAddr[AUDIO_RTP_HANDLE].get_host_addr(),
                            m_UdpPeerAddr[AUDIO_RTP_HANDLE].get_port_number());
        }
    }

    m_unStartTime++;

    setStatus(STREAM_SESSION_STATUS_WAIT_CHANNEL_REDAY);
    SVS_LOG(SVS_LOG_INFO,"start std rtp session[%lld]  success.", getStreamId());
    return AS_ERROR_CODE_OK;
}

int32_t CStreamStdRtpSession::sendStartRequest()
{
    setStatus(STREAM_SESSION_STATUS_DISPATCHING);
    return AS_ERROR_CODE_OK;
}

void CStreamStdRtpSession::setRtspHandle(ACE_HANDLE handle, const ACE_INET_Addr &addr)
{
    m_rtspHandle = handle;
    m_rtspAddr   = addr;
    return;
}

void CStreamStdRtpSession::setPlayLoad(uint16_t unVedioPT,uint16_t unAudioPT )
{
    m_unVedioPT = unVedioPT;
    m_unAudioPT = unAudioPT;
    return;
}

void CStreamStdRtpSession::setSessionId(uint64_t ullSessionId)
{
    m_stSessionInfo.StreamID = ullSessionId;
    return;
}

int32_t CStreamStdRtpSession::sendMessage(const char* pData, uint32_t unDataSize,bool updateTime)
{
    ACE_Guard<ACE_Thread_Mutex> locker(m_TcpSendMutex);
    if (TRANS_PROTOCAL_UDP == m_unTransType)
    {
        ACE_Time_Value timeout(1);
        int32_t nSendSize = ACE::send_n(m_rtspHandle, pData, unDataSize, &timeout);
        if (unDataSize != (uint32_t) nSendSize)
        {
            SVS_LOG(SVS_LOG_WARNING,"session[%lld] send message fail, error[%d] close handle[%d].",
                            getStreamId(), ACE_OS::last_error(), m_rtspHandle);

            m_rtspHandle = ACE_INVALID_HANDLE;
            setStatus(STREAM_SESSION_STATUS_ABNORMAL);
            return AS_ERROR_CODE_FAIL;
        }

        return AS_ERROR_CODE_OK;
    }

    if (!m_TcpSendList.empty() && (AS_ERROR_CODE_OK != sendLeastData()))
    {
        ACE_Message_Block *pMsgBlock = CMediaBlockBuffer::instance().allocMediaBlock();
        if (NULL == pMsgBlock)
        {
            SVS_LOG(SVS_LOG_WARNING,"tcp connect send stream[%lld] message data fail, "
                    "alloc cache buffer fail, close handle.",
                    getStreamId());
            m_rtspHandle = ACE_INVALID_HANDLE;
            setStatus(STREAM_SESSION_STATUS_ABNORMAL);
            return RET_ERR_DISCONNECT;
        }

        pMsgBlock->copy(pData, unDataSize);
        m_TcpSendList.push_back(pMsgBlock);

        return AS_ERROR_CODE_OK;
    }

    int32_t nSendSize = ACE::send(m_rtspHandle, pData, unDataSize);
    if (0 >= nSendSize)
    {
        int32_t iErrorCode = ACE_OS::last_error();
        if (checkIsDisconnect(iErrorCode))
        {
            SVS_LOG(SVS_LOG_WARNING,"stream[%lld] connect send message fail, errno[%d] "
                            ", close handle[%d].",
                            getStreamId(),
                            iErrorCode,
                            m_rtspHandle);

            m_rtspHandle = ACE_INVALID_HANDLE;
            setStatus(STREAM_SESSION_STATUS_ABNORMAL);
            return RET_ERR_DISCONNECT;
        }

        nSendSize = nSendSize > 0 ? nSendSize : 0;
    }
    else
    {
        if(updateTime){
            /* tcp send for media heartbeat */
            m_ulLastRecvTime = SVS_GetSecondTime();
        }
    }

    if (unDataSize != (uint32_t) nSendSize)
    {
        ACE_Message_Block *pMsgBlock =
                CMediaBlockBuffer::instance().allocMediaBlock();
        if (NULL == pMsgBlock)
        {
            SVS_LOG(SVS_LOG_WARNING,"tcp connect send stream[%lld] media data fail, "
                    "alloc cache buffer fail, close handle.",
                    getStreamId());
            m_rtspHandle = ACE_INVALID_HANDLE;
            setStatus(STREAM_SESSION_STATUS_ABNORMAL);
            return RET_ERR_DISCONNECT;
        }

        pMsgBlock->copy(pData + nSendSize, unDataSize - (uint32_t) nSendSize);
        m_TcpSendList.push_back(pMsgBlock);
    }

    return AS_ERROR_CODE_OK;
}
int32_t CStreamStdRtpSession::initSesssion(PEER_TYPE unPeerType)
{
    m_stSessionInfo.SessionType    = RTSP_SESSION;
    m_stSessionInfo.PeerType       = unPeerType;
    m_stSessionInfo.TransDirection = TRANS_DIRECTION_SENDONLY;
    m_stSessionInfo.MediaTransType = MEDIA_TRANS_TYPE_RTP;
    m_stSessionInfo.TransProtocol  = TRANS_PROTOCAL_TCP;
    return AS_ERROR_CODE_OK;
}

int32_t CStreamStdRtpSession::sendMediaData(ACE_Message_Block **pMbArray, uint32_t MsgCount)
{

    if (STREAM_SESSION_STATUS_DISPATCHING != getStatus())
    {
        SVS_LOG(SVS_LOG_INFO,"session[%lld] discard media data, the status[%d] invalid.",
                        getStreamId(), getStatus());
        return AS_ERROR_CODE_OK;
    }

    if (TRANS_PROTOCAL_TCP == m_unTransType)
    {
        return sendTcpMediaData(pMbArray, MsgCount);
    }

    (void) sendUdpMediaData(pMbArray, MsgCount);

    return AS_ERROR_CODE_OK;
}

uint32_t CStreamStdRtpSession::getMediaTransType()const
{
    return MEDIA_TRANS_TYPE_RTP;
}

int32_t CStreamStdRtpSession::sendVcrMessage(CRtspPacket &rtspPack)
{
    rtspPack.setSessionID((uint64_t)atoll(m_strRtspSessionId.c_str()));

    std::string strRtpInfo;
    rtspPack.getRtpInfo(strRtpInfo);
    if ("" != strRtpInfo)
    {
        std::string strSeq;
        std::string strRtptime;

        std::string::size_type nStartPos = strRtpInfo.find(";");
        if (std::string::npos == nStartPos)
        {
            SVS_LOG(SVS_LOG_INFO,"session[%lld] send vcr message fail, RTP-Info[%s] invalid.",
                            getStreamId(), strRtpInfo.c_str());
            return AS_ERROR_CODE_FAIL;
        }
        strRtpInfo = strRtpInfo.substr(nStartPos);

        std::string strNewRtpInfo = "url=";
        strNewRtpInfo += m_strRtspUrl;
        strNewRtpInfo += strRtpInfo;
        rtspPack.setRtpInfo(strNewRtpInfo);
    }

    std::string strMsg;
    if (0 != rtspPack.generateRtspResp(strMsg))
    {
        SVS_LOG(SVS_LOG_INFO,"session[%lld] send vcr message fail, generate rtsp resp message fail.",
                getStreamId());
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t unTmp = 0;
    if (0 == rtspPack.getRangeTime(unTmp, unTmp, unTmp))
    {
        strMsg.erase(strMsg.length() - 2, strMsg.length());
        strMsg += "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, ANNOUNCE\r\n\r\n";
    }

    int32_t nRet = sendMessage(strMsg.c_str(), strMsg.length(),true);
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"session[%lld] send vcr message fail.", getStreamId());
        return AS_ERROR_CODE_FAIL;
    }

    SVS_LOG(SVS_LOG_INFO,"session[%lld] send vcr message success.", getStreamId());
    SVS_LOG(SVS_LOG_DEBUG,"%s", strMsg.c_str());
    return AS_ERROR_CODE_OK;
}

int32_t CStreamStdRtpSession::sendSessionStopMessage(uint32_t unStopType)
{
    CRtspAnnounceMessage msg;
    msg.setMsgType(RTSP_MSG_REQ);
    msg.setRtspUrl(m_strRtspUrl);
    msg.setSession(m_strRtspSessionId);
    msg.setCSeq(1);

    std::string strMsg;
    if (AS_ERROR_CODE_OK != msg.encodeMessage(strMsg))
    {
        SVS_LOG(SVS_LOG_WARNING,"session[%lld] send vos message fail, encode announce msg fail.",
                        getStreamId());
        return AS_ERROR_CODE_FAIL;
    }


    return sendMessage(strMsg.c_str(), strMsg.length(),true);
}

void CStreamStdRtpSession::setSdpInfo(CMediaSdp& rtspSdp)
{
    m_rtspSdp.copy(rtspSdp);
}

ACE_INET_Addr CStreamStdRtpSession::getPeerAddr()const
{
    return m_rtspAddr;
}

ACE_INET_Addr CStreamStdRtpSession::getMediaAddr()const
{
    if (TRANS_PROTOCAL_TCP == m_unTransType)
    {
        return m_rtspAddr;
    }
    else
    {
        return m_UdpPeerAddr[VIDEO_RTP_HANDLE];
    }
}

int32_t CStreamStdRtpSession::handleInnerMessage(const STREAM_INNER_MSG &innerMsg,
                                          uint32_t unMsgSize,
                                          CStreamSession&  peerSession)
{
    //m_ulLastRecvTime = SVS_GetSecondTime();
    SVS_LOG(SVS_LOG_INFO,"session[%lld] ,handle inner message,time:[%u],type:[%d].",
                                        getStreamId(), m_ulLastRecvTime,innerMsg.usMsgType);

    if (INNER_MSG_RTSP == innerMsg.usMsgType)
    {
        (void)peerSession.handleRecvedNatRequest();

        setStatus(STREAM_SESSION_STATUS_DISPATCHING);

        SVS_LOG(SVS_LOG_INFO,"session[%lld] handle inner rtsp message success.", getStreamId());
        return AS_ERROR_CODE_OK;
    }

    if (INNER_MSG_RTCP == innerMsg.usMsgType)
    {
        return AS_ERROR_CODE_OK;
    }
    if ((INNER_MSG_RTPDUMMY == innerMsg.usMsgType)
        ||(INNER_MSG_RTCPDUMMY == innerMsg.usMsgType))
    {
        int32_t nHandleIndex = 0;
        for (nHandleIndex = 0; nHandleIndex < HANDLE_TYPE_MAX; nHandleIndex++)
        {
            if (innerMsg.pRecvHandle == m_pUdpHandle[nHandleIndex])
            {
                break;
            }
        }

        if ((HANDLE_TYPE_MAX <= nHandleIndex) || (NULL == m_pUdpHandle[nHandleIndex]))
        {
            SVS_LOG(SVS_LOG_WARNING,"session[%lld] handle dummy message fail, recv handle invalid.",
                    getStreamId());
            return AS_ERROR_CODE_FAIL;
        }

        m_UdpPeerAddr[nHandleIndex].set(innerMsg.usRemotePort, innerMsg.unRemoteIp);
        SVS_LOG(SVS_LOG_INFO,"session[%lld] handle inner [%d]dummy message success,remoteAddr:[%s] remoteport:[%d].",
                                            getStreamId(),innerMsg.usMsgType,
                                            m_UdpPeerAddr[nHandleIndex].get_host_addr(),
                                            m_UdpPeerAddr[nHandleIndex].get_port_number());
        return AS_ERROR_CODE_OK;
    }

    SVS_LOG(SVS_LOG_WARNING,"session[%lld] handle not accepted inner message[%d].",
                    getStreamId(), innerMsg.usMsgType);
    return AS_ERROR_CODE_FAIL;
}

int32_t CStreamStdRtpSession::allocMediaPort()
{
    if (TRANS_PROTOCAL_TCP == m_unTransType)
    {
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = CStreamPortManager::instance()->allocRtpUdpPort(getSpecifyIp(),
            m_pUdpHandle[VIDEO_RTP_HANDLE], m_pUdpHandle[VIDEO_RTCP_HANDLE]);
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] alloc video media port fail.", getStreamId());
        return AS_ERROR_CODE_FAIL;
    }
    m_VideoAddr.set(m_pUdpHandle[VIDEO_RTP_HANDLE]->getLocalAddr());

    nRet = CStreamPortManager::instance()->allocRtpUdpPort(getSpecifyIp(),
            m_pUdpHandle[AUDIO_RTP_HANDLE], m_pUdpHandle[AUDIO_RTCP_HANDLE]);
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] alloc audio media port fail.",  getStreamId());
        return AS_ERROR_CODE_FAIL;
    }
    m_AudioAddr.set(m_pUdpHandle[AUDIO_RTP_HANDLE]->getLocalAddr());

    SVS_LOG(SVS_LOG_INFO,
            "Rtp session alloc media port success. stream id[%lld] "
            "video rtp handle[%p] video rtcp handle[%p] "
            "audio rtp handle[%p] audio rtcp handle[%p].",
            getStreamId(),
            m_pUdpHandle[VIDEO_RTP_HANDLE],
            m_pUdpHandle[VIDEO_RTCP_HANDLE],
            m_pUdpHandle[AUDIO_RTP_HANDLE],
            m_pUdpHandle[AUDIO_RTCP_HANDLE]);

    return AS_ERROR_CODE_OK;
}

int32_t CStreamStdRtpSession::startMediaPort()
{
    if (TRANS_PROTOCAL_TCP == m_unTransType)
    {
        return AS_ERROR_CODE_FAIL;
    }


    if ((NULL == m_pUdpHandle[VIDEO_RTP_HANDLE])
            || (NULL == m_pUdpHandle[VIDEO_RTCP_HANDLE]))
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] start video port fail, handle  is null.",
                getStreamId());
        return AS_ERROR_CODE_FAIL;
    }
    int32_t iRet = m_pUdpHandle[VIDEO_RTP_HANDLE]->startHandle(getStreamId(),
                                                m_UdpPeerAddr[VIDEO_RTP_HANDLE]);
    if (AS_ERROR_CODE_OK != iRet)
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] start video rtp port fail.", getStreamId());
        return AS_ERROR_CODE_FAIL;
    }

    iRet = m_pUdpHandle[VIDEO_RTCP_HANDLE]->startHandle(getStreamId(),
                                                m_UdpPeerAddr[VIDEO_RTCP_HANDLE]);
    if (AS_ERROR_CODE_OK != iRet)
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] start video rtcp port fail.", getStreamId());
        return AS_ERROR_CODE_FAIL;
    }

    if ((NULL == m_pUdpHandle[AUDIO_RTP_HANDLE])
            || (NULL == m_pUdpHandle[AUDIO_RTCP_HANDLE]))
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] start audio port fail, handle  is null.",
                getStreamId());
        return AS_ERROR_CODE_FAIL;
    }
    iRet = m_pUdpHandle[AUDIO_RTP_HANDLE]->startHandle(getStreamId(),
                                                        m_UdpPeerAddr[AUDIO_RTP_HANDLE]);
    if (AS_ERROR_CODE_OK != iRet)
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] start audio rtp port fail.", getStreamId());
        return AS_ERROR_CODE_FAIL;
    }

    iRet = m_pUdpHandle[AUDIO_RTCP_HANDLE]->startHandle(getStreamId(),
                                                        m_UdpPeerAddr[AUDIO_RTCP_HANDLE]);
    if (AS_ERROR_CODE_OK != iRet)
    {
        SVS_LOG(SVS_LOG_ERROR,"session[%lld] start audio rtcp port fail.", getStreamId());
        return AS_ERROR_CODE_FAIL;
    }

    SVS_LOG(SVS_LOG_INFO,"Start media port success. stream id[%lld].",
            getStreamId());
    return AS_ERROR_CODE_OK;
}

int32_t CStreamStdRtpSession::stopMediaPort()
{
    if (TRANS_PROTOCAL_TCP == getTransProtocol())
    {
        SVS_LOG(SVS_LOG_INFO,"session[%lld] stop tcp media port success.",
                getStreamId());
        return AS_ERROR_CODE_OK;
    }

    for (int32_t i = 0; i < HANDLE_TYPE_MAX; i++)
    {
        if (NULL != m_pUdpHandle[i])
        {
            (void) m_pUdpHandle[i]->stopHandle(getStreamId());
        }
    }
    SVS_LOG(SVS_LOG_INFO,"Rtp udp session stop port success. stream id[%lld].",
                    getStreamId());
    return AS_ERROR_CODE_OK;
}

bool CStreamStdRtpSession::checkIsDisconnect(int32_t nErrNo) const
{
    if (EAGAIN == nErrNo
            || ETIME == nErrNo
            || EWOULDBLOCK == nErrNo
            || ETIMEDOUT == nErrNo
            || EINTR == nErrNo)
    {
        return false;
    }
    return true;
}

int32_t CStreamStdRtpSession::saveLeastData(ACE_Message_Block ** const pMbArray, uint32_t MsgCount,
                                     uint32_t nSendSize, uint32_t nSendCount)
{
    if (NULL == pMbArray)
    {
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t i = nSendCount;
    for (; i < MsgCount; i++)
    {
        if (NULL == pMbArray[i])
        {
            return AS_ERROR_CODE_FAIL;
        }

        ACE_Message_Block *pMsgBlock = CMediaBlockBuffer::instance().allocMediaBlock();
        if (!pMsgBlock)
        {
            SVS_LOG(SVS_LOG_WARNING,"session[%lld] alloc media data block fail, close handle[%d]",
                    getStreamId(), m_rtspHandle);

            m_rtspHandle = ACE_INVALID_HANDLE;
            setStatus(STREAM_SESSION_STATUS_ABNORMAL);
            return RET_ERR_DISCONNECT;
        }

        if ((0 == nSendSize) || (RTP_INTERLEAVE_LENGTH > nSendSize))
        {
            CRtpPacket rtpPack;
            if (AS_ERROR_CODE_OK != rtpPack.ParsePacket(pMbArray[i]->rd_ptr(),
                    pMbArray[i]->length()))
            {
                CMediaBlockBuffer::instance().freeMediaBlock(pMsgBlock);
                SVS_LOG(SVS_LOG_WARNING,"session[%lld] save send meida fail, parse data as rtp fail, "
                        "msg[%p] len[%u], close handle[%d].",
                        getStreamId(), pMbArray[i], pMbArray[i]->length(), m_rtspHandle);

                m_rtspHandle = ACE_INVALID_HANDLE;
                setStatus(STREAM_SESSION_STATUS_ABNORMAL);
                return RET_ERR_DISCONNECT;
            }

            char cPayloadType = rtpPack.GetPayloadType();
            char dataBuf[RTP_INTERLEAVE_LENGTH] = { 0 };
            dataBuf[0] = RTP_INTERLEAVE_FLAG;
            if ((char) m_unVedioPT == cPayloadType)
            {
                dataBuf[1] = (char) m_unVideoInterleaveNum;
            }
            else
            {
                dataBuf[1] = (char) m_unAudioInterleaveNum;
            }
            *(uint16_t*) &dataBuf[2] = htons(
                    (uint16_t) pMbArray[i]->length());

            (void) pMsgBlock->copy(dataBuf, RTP_INTERLEAVE_LENGTH - nSendSize);
            nSendSize = 0;
        }

        if (0 != nSendSize)
        {
            nSendSize -= RTP_INTERLEAVE_LENGTH;
        }

        (void) pMsgBlock->copy(pMbArray[i]->rd_ptr() + nSendSize,
                                pMbArray[i]->length() - nSendSize);
        m_TcpSendList.push_back(pMsgBlock);

        nSendSize = 0;
    }

    return AS_ERROR_CODE_OK;    //lint !e818
} //lint !e818

int32_t CStreamStdRtpSession::sendLeastData()
{
    while (!m_TcpSendList.empty())
    {
        ACE_Message_Block *pMsg = m_TcpSendList.front();
        int32_t nRet = ACE::send(m_rtspHandle, pMsg->rd_ptr(), pMsg->length());
        if (0 >= nRet)
        {
            int32_t iErrorCode = ACE_OS::last_error();
            if (checkIsDisconnect(iErrorCode))
            {
                SVS_LOG(SVS_LOG_WARNING,"stream[%lld] send tcp least data fail, "
                                    "errno[%d], close handle[%d].",
                                    getStreamId(),
                                    iErrorCode,
                                    m_rtspHandle);

                m_rtspHandle = ACE_INVALID_HANDLE;
                setStatus(STREAM_SESSION_STATUS_ABNORMAL);

                return RET_ERR_DISCONNECT;
            }
            nRet = nRet > 0 ? nRet : 0;
        }
        else
        {
            /* tcp send for media heartbeat */
            m_ulLastRecvTime = SVS_GetSecondTime();
        }

        if (pMsg->length() != (uint32_t) nRet)
        {
            pMsg->rd_ptr((uint32_t) nRet);
            return AS_ERROR_CODE_FAIL;
        }

        CMediaBlockBuffer::instance().freeMediaBlock(pMsg);
        m_TcpSendList.pop_front();
    }

    return AS_ERROR_CODE_OK;
}

/*lint -e818*/
int32_t CStreamStdRtpSession::sendUdpMediaData(ACE_Message_Block **pMbArray, uint32_t MsgCount)
{
    if (NULL == pMbArray)
    {
        return AS_ERROR_CODE_FAIL;
    }

    for (uint32_t unSendCount = 0; unSendCount < MsgCount; unSendCount++)
    {
        if (NULL == pMbArray[unSendCount])
        {
            return AS_ERROR_CODE_FAIL;
        }

        CRtpPacket rtpPack;
        if (AS_ERROR_CODE_OK != rtpPack.ParsePacket(pMbArray[unSendCount]->rd_ptr(),
                                          pMbArray[unSendCount]->length()))
        {
            SVS_LOG(SVS_LOG_ERROR,
                "session[%lld] send media data fail, parse rtp packet fail.",
                getStreamId());
            return -1;
        }

        char cPt = rtpPack.GetPayloadType();
        if ((char)m_unVedioPT == cPt )
        {
            return m_pUdpHandle[VIDEO_RTP_HANDLE]->sendMessage(getStreamId(),
                                                                pMbArray[unSendCount]->rd_ptr(),
                                                                pMbArray[unSendCount]->length(),
                                                                m_UdpPeerAddr[VIDEO_RTP_HANDLE]);
        }
        else
        {
            return m_pUdpHandle[AUDIO_RTP_HANDLE]->sendMessage(getStreamId(),
                                                                pMbArray[unSendCount]->rd_ptr(),
                                                                pMbArray[unSendCount]->length(),
                                                                m_UdpPeerAddr[AUDIO_RTP_HANDLE]);
        }
    }

    return -1;
}

int32_t CStreamStdRtpSession::sendTcpMediaData(ACE_Message_Block **pMbArray, uint32_t MsgCount)
{
    ACE_Guard<ACE_Thread_Mutex> locker(m_TcpSendMutex);
    if (!m_TcpSendList.empty() && (AS_ERROR_CODE_OK != sendLeastData()))
    {
        return RET_ERR_SEND_FAIL;
    }

    uint32_t unCount = 0;
    int32_t nSendSize = 0;
    for (; unCount < MsgCount; unCount++)
    {
        if (NULL == pMbArray[unCount])
        {
            SVS_LOG(SVS_LOG_WARNING,"session[%lld] send meida fail, data block is null, ",
                             getStreamId());
            return AS_ERROR_CODE_FAIL;
        }

        CRtpPacket rtpPack;
        if (AS_ERROR_CODE_OK != rtpPack.ParsePacket(pMbArray[unCount]->rd_ptr(), pMbArray[unCount]->length()))
        {
            SVS_LOG(SVS_LOG_WARNING,"session[%lld] send meida fail, parse data as rtp fail, "
                        "msg[%p] len[%u].",
                       getStreamId(), pMbArray[unCount], pMbArray[unCount]->length());
            return AS_ERROR_CODE_FAIL;
        }

        char interleaveData[RTP_INTERLEAVE_LENGTH] = {0};
        char cPt = rtpPack.GetPayloadType();
        interleaveData[0] = RTP_INTERLEAVE_FLAG;

        if ((char)m_unVedioPT == cPt)
        {
            interleaveData[1] = (char)m_unVideoInterleaveNum;
        }
        else
        {
            interleaveData[1] = (char)m_unAudioInterleaveNum;
        }
        *(uint16_t*)&interleaveData[2] = htons((uint16_t)pMbArray[unCount]->length());

        struct iovec dataVec[2];
        dataVec[0].iov_len  = RTP_INTERLEAVE_LENGTH;
        dataVec[0].iov_base = interleaveData;
        dataVec[1].iov_len  = pMbArray[unCount]->length();
        dataVec[1].iov_base = pMbArray[unCount]->rd_ptr();

        nSendSize = ACE::sendv(m_rtspHandle, dataVec, 2);
        if (0 >= nSendSize)
        {
            int32_t iErrorCode = ACE_OS::last_error();
            if (checkIsDisconnect(iErrorCode))
            {
                SVS_LOG(SVS_LOG_WARNING,
                    "stream[%lld] send tcp least data fail, errno[%d], close handle[%d].",
                    getStreamId(), iErrorCode, m_rtspHandle);
                m_rtspHandle = ACE_INVALID_HANDLE;
                setStatus(STREAM_SESSION_STATUS_ABNORMAL);
                return RET_ERR_DISCONNECT;
            }

            nSendSize = nSendSize > 0 ? nSendSize : 0;
        }
        else
        {
            /* tcp send for media heartbeat */
            m_ulLastRecvTime = SVS_GetSecondTime();
        }

        if (pMbArray[unCount]->length() + RTP_INTERLEAVE_LENGTH != (uint32_t)nSendSize)
        {
            break;
        }
    }

    if (unCount < MsgCount)
    {
        return saveLeastData(pMbArray, MsgCount, (uint32_t)nSendSize, unCount);
    }

    return AS_ERROR_CODE_OK;
}
/*lint +e818*/

bool CStreamStdRtpSession::checkMediaChannelStatus()
{
    uint32_t ulCostTime = SVS_GetSecondTime() - m_ulLastRecvTime;
    if (ulCostTime > STREAM_MEDIA_CHANNEL_INVAILD_INTERVAL)
    {
        SVS_LOG(SVS_LOG_WARNING,"std session[%lld] not recv data at [%u]s, check media channel status fail.",
                         getStreamId(), ulCostTime);
        return false;
    }

    sendRtcpReport();

    return true;
}

void CStreamStdRtpSession::sendRtcpReport()
{
    char buf[KILO] = { 0 };
    char* pRtcpBuff = buf + RTP_INTERLEAVE_LENGTH;
    uint32_t unRtcpLen = 0;
    (void) m_rtcpPacket.createSenderReport(pRtcpBuff, KILO - RTP_INTERLEAVE_LENGTH,unRtcpLen);

    if (TRANS_PROTOCAL_TCP == getTransProtocol())
    {
        buf[0] = RTP_INTERLEAVE_FLAG;
        buf[1] = (char)m_unVideoInterleaveNum + 1;
        *(uint16_t*) &buf[2] = htons((uint16_t)unRtcpLen);
        (void)sendMessage(buf, unRtcpLen + RTP_INTERLEAVE_LENGTH,false);

        buf[0] = RTP_INTERLEAVE_FLAG;
        buf[1] = (char)m_unAudioInterleaveNum + 1;
        *(uint16_t*) &buf[2] = htons((uint16_t)unRtcpLen);
        (void)sendMessage(buf, unRtcpLen + RTP_INTERLEAVE_LENGTH,false);
    }
    else
    {
        if (!m_pUdpHandle[VIDEO_RTCP_HANDLE])
        {
            SVS_LOG(SVS_LOG_INFO,"session[%lld] send rtcp fail, video rtcp handle invalid.", getStreamId());
            return;
        }

        (void) m_pUdpHandle[VIDEO_RTCP_HANDLE]->sendMessage(getStreamId(), pRtcpBuff,
                unRtcpLen, m_UdpPeerAddr[VIDEO_RTCP_HANDLE]);

        if (!m_pUdpHandle[AUDIO_RTCP_HANDLE])
        {
            SVS_LOG(SVS_LOG_INFO,"session[%lld] send rtcp fail, audio rtcp handle invalid.", getStreamId());
            return;
        }
        (void) m_pUdpHandle[AUDIO_RTCP_HANDLE]->sendMessage(getStreamId(), pRtcpBuff,
                unRtcpLen, m_UdpPeerAddr[AUDIO_RTCP_HANDLE]);
    }

    SVS_LOG(SVS_LOG_INFO,"std session[%lld] send rtcp sender report", getStreamId());
    return;
}
