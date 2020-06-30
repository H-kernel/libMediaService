/*
 * StreamRtspPushSession.cpp
 *
 *  Created on: 2016-5-16
 *      Author:
 */
#include <sstream>
#include "ms_engine_svs_retcode.h"
#include "ms_engine_time.h"
#include "svs_log.h"
#include <vms.h>
#include "md5.h"
#include "svs_rtsp_push_session.h"
#include "svs_vms_msg_factory.h"
#include "svs_vms_media_setup_req.h"
#include "svs_vms_media_setup_resp.h"
#include "svs_vms_media_play_req.h"
#include "svs_vms_media_play_resp.h"
#include "svs_vms_media_teardown_req.h"
#include "svs_vms_media_keyframe_req.h"
#include "ms_engine_session_factory.h"
#include "ms_engine_business_manager.h"
#include "ms_engine_inner_msg.h"
#include "ms_engine_business_manager.h"
#include "ms_engine_service_task.h"

#include "ms_engine_media_processor.h"
#include "ms_engine_media_block_buffer.h"
#include "ms_engine_media_exchange.h"

#include "svs_rtsp_options_message.h"
#include "svs_rtsp_describe_message.h"
#include "svs_rtsp_setup_message.h"
#include "svs_rtsp_play_message.h"
#include "svs_rtsp_pause_message.h"
#include "svs_rtsp_teardown_message.h"
#include "svs_rtsp_record_message.h"
#include "svs_rtsp_get_parameter_message.h"
#include "svs_rtsp_announce_message.h"
#include "svs_rtsp_service.h"
#include "ms_engine_svs_def.h"
#include "ms_engine_config.h"

mk_rtsp_client::mk_rtsp_client()
{
    m_unSessionIndex  = 0;
    m_enPlayType      = PLAY_TYPE_LIVE;
    m_bSetUp          = false;
    m_sockHandle      = ACE_INVALID_HANDLE;
    m_pRecvBuffer     = NULL;
    m_pRtpSession     = NULL;
    m_pPeerSession    = NULL;
    m_pLastRtspMsg    = NULL;

    m_unSessionStatus  = RTSP_SESSION_STATUS_INIT;
    m_ulStatusTime     = SVS_GetSecondTime();

    m_strContentID     = "";
    m_bFirstSetupFlag  = true;
    m_strPlayRange     = "";
    m_lRedoTimerId     = -1;

    m_unTransType      = TRANS_PROTOCAL_UDP;
    m_cVideoInterleaveNum = 0;
    m_cAudioInterleaveNum = 0;
}

mk_rtsp_client::~mk_rtsp_client()
{
    m_unSessionIndex  = 0;
    m_sockHandle      = ACE_INVALID_HANDLE;
    if(NULL != m_pRecvBuffer) {
        delete m_pRecvBuffer;
    }
    m_pRecvBuffer     = NULL;
    m_pRtpSession     = NULL;
    m_pPeerSession    = NULL;
    m_pLastRtspMsg    = NULL;

    m_bFirstSetupFlag  = true;
    m_lRedoTimerId     = -1;
}

int32_t mk_rtsp_client::open(uint32_t unIndex, const ACE_INET_Addr &peerAddr)
{
    m_unSessionIndex  = unIndex;
    m_PeerAddr       = peerAddr;

    if (ACE_INVALID_HANDLE == m_sockHandle)
    {
        SVS_LOG(SVS_LOG_WARNING,"open rtsp push session[%u] fail, socket handle invalid",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = setSockOpt();
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"open rtsp push session[%u] fail, set socket option fail.",
                         m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (NULL == m_pRecvBuffer)
    {
        try
        {
            m_pRecvBuffer = new Static_PreAssign_Buffer;
        }
        catch (...)
        {
            return RET_ERR_SYS_NEW;
        }
        m_pRecvBuffer->wr_ptr(0);
        m_pRecvBuffer->rd_ptr(0);
    }

    ACE_Reactor *pReactor = ACE_Reactor::instance();
    if (!pReactor)
    {
        SVS_LOG(SVS_LOG_WARNING,"open rtsp push session fail, can't find reactor instance.");
        return AS_ERROR_CODE_FAIL;
    }

    nRet = pReactor->register_handler(m_sockHandle, this,
                                      ACE_Event_Handler::READ_MASK);
    /*
    nRet = pReactor->register_handler(m_sockHandle, this,
                                      ACE_Event_Handler::READ_MASK|ACE_Event_Handler::WRITE_MASK);
    */
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"open rtsp server fail, register read mask fail[%d].",
                ACE_OS::last_error());
        return AS_ERROR_CODE_FAIL;
    }

    SVS_LOG(SVS_LOG_INFO,"open rtsp push session[%u] success, peer addr[%s:%d], handle[%d].",
                    m_unSessionIndex, m_PeerAddr.get_host_addr(),
                    m_PeerAddr.get_port_number(),
                    m_sockHandle);
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_client::open(const char* pszUrl)
{
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_client::close()
{
    (void)ACE_OS::shutdown(m_sockHandle, SHUT_RDWR);
    SVS_LOG(SVS_LOG_INFO,"close rtsp push session[%u] success.handle[%d]",  
        m_unSessionIndex, m_sockHandle);
    return;
}

void mk_rtsp_client::setStatus(uint32_t unStatus)
{
    uint32_t unOldStatus = m_unSessionStatus;
    m_unSessionStatus        = unStatus;
    m_ulStatusTime           = SVS_GetSecondTime();
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] switch status[%u] to [%u].",
            m_unSessionIndex, unOldStatus, m_unSessionStatus);
    return;
}

uint32_t mk_rtsp_client::getStatus() const
{
    return m_unSessionStatus;
}
void  mk_rtsp_client::set_rtp_over_tcp()
{
    return 
}

 void  mk_rtsp_client::set_status_callback(tsp_client_status cb,void* ctx)
 {

 }

int32_t mk_rtsp_client::handleSvsMessage(CStreamSvsMessage &message)
{
    if(SVS_MSG_TYPE_STREAM_SESSION_PLAY_RESP == message.getMsgType())
    {
        /* Key Frame request */
        sendKeyFrameReq();
        return AS_ERROR_CODE_OK;
    }
    CStreamMediaSetupResp *pResp = dynamic_cast<CStreamMediaSetupResp*>(&message);
    if (!pResp)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle svs message fail, invalid message.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (0 != pResp->getRespCode())
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle svs message fail, retcode[0x%x].",
                                m_unSessionIndex, pResp->getRespCode());
        if (NULL != m_pLastRtspMsg)
        {
            sendCommonResp(RTSP_SERVER_INTERNAL, m_pLastRtspMsg->getCSeq());
        }
        return AS_ERROR_CODE_FAIL;
    }

    ACE_Guard<ACE_Recursive_Thread_Mutex> locker(m_RtspMutex);
    if (NULL == m_pLastRtspMsg)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle svs message fail, last rtsp message is null.",
                         m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if ((NULL == m_pPeerSession )||(AS_ERROR_CODE_OK != m_pPeerSession->start(pResp)))
    {
        SVS_LOG(SVS_LOG_ERROR,"start the peer session fail.");
        return AS_ERROR_CODE_FAIL;
    }

    uint64_t ullPeerSessionId = 0;
    ullPeerSessionId = m_pPeerSession->getStreamId();

    if (AS_ERROR_CODE_OK != handleRtspMessage(*m_pLastRtspMsg))
    {
        delete m_pLastRtspMsg;
        m_pLastRtspMsg = NULL;
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle last rtsp message fail, peer session id[%lld].",
                         m_unSessionIndex, ullPeerSessionId);
        return AS_ERROR_CODE_FAIL;
    }

    delete m_pLastRtspMsg;
    m_pLastRtspMsg = NULL;

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle svs message success, peer session id[%lld].",
                    m_unSessionIndex, ullPeerSessionId);
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_client::handle_input(ACE_HANDLE handle)
{
    if ((NULL == m_pRecvBuffer) || (ACE_INVALID_HANDLE == m_sockHandle))
    {
        return -1;
    }

    int32_t iRecvLen = (int32_t) m_pRecvBuffer->size() - (int32_t) m_pRecvBuffer->length();
    if (iRecvLen <= 0)
    {
        SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] recv buffer is full, size[%u] length[%u].",
                m_unSessionIndex,
                m_pRecvBuffer->size(),
                m_pRecvBuffer->length());
        return 0;
    }

    ACE_OS::last_error(0);
    iRecvLen = ACE::recv(m_sockHandle, m_pRecvBuffer->wr_ptr(), (size_t) iRecvLen);
    if (iRecvLen <= 0)
    {
        int32_t iErrorCode = ACE_OS::last_error();
        if (!(EAGAIN == iErrorCode
                || ETIME == iErrorCode
                || EWOULDBLOCK == iErrorCode
                || ETIMEDOUT == iErrorCode
                || EINTR == iErrorCode))
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] recv data fail, "
                    "close handle[%d]. errno[%d].",
                    m_unSessionIndex,
                    m_sockHandle,
                    iErrorCode);

            return -1;
        }

        SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] recv data fail, wait retry. errno[%d].",
                m_unSessionIndex,
                iErrorCode);
        return 0;
    }

    m_pRecvBuffer->wr_ptr((size_t)(m_pRecvBuffer->length() + (size_t) iRecvLen));
    m_pRecvBuffer->rd_ptr((size_t) 0);

    size_t processedSize = 0;
    size_t totalSize = m_pRecvBuffer->length();
    int32_t nSize = 0;
    do
    {
        nSize = processRecvedMessage(m_pRecvBuffer->rd_ptr() + processedSize,
                                     m_pRecvBuffer->length() - processedSize);
        if (nSize < 0)
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] process recv data fail, close handle[%d]. ",
                    m_unSessionIndex,
                    m_sockHandle);
            return -1;
        }

        if (0 == nSize)
        {
            break;
        }

        processedSize += (size_t) nSize;
    }
    while (processedSize < totalSize);

    size_t dataSize = m_pRecvBuffer->length() - processedSize;
    (void) m_pRecvBuffer->copy(m_pRecvBuffer->rd_ptr() + processedSize, dataSize);
    m_pRecvBuffer->rd_ptr((size_t) 0);
    m_pRecvBuffer->wr_ptr(dataSize);

    return 0;
}

int32_t mk_rtsp_client::handle_output (ACE_HANDLE handle)
{
    /*if (NULL != m_pRtpSession)
    {
        return m_pRtpSession->sendMessage(pData, unDataSize);
    }*/
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_client::handle_timeout(const ACE_Time_Value &tv, const void *arg)
{
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] retry process rtsp message.", m_unSessionIndex);

    if (!m_pLastRtspMsg)
    {
        clearRtspCachedMessage();
        return 0;
    }

    (void)handleRtspMessage(*m_pLastRtspMsg);

    return 0;
}

int32_t mk_rtsp_client::handle_close(ACE_HANDLE /*handle*/, ACE_Reactor_Mask /*close_mask*/)
{
    if (ACE_INVALID_HANDLE == m_sockHandle)
    {
        return 0;
    }

    ACE_Reactor *pReactor = ACE_Reactor::instance();
    if (!pReactor)
    {
        SVS_LOG(SVS_LOG_WARNING,"handle close rtsp push session[%u] fail, can't find reactor instance.",
                        m_unSessionIndex);
        return -1;
    }

    int32_t nRet = pReactor->remove_handler(m_sockHandle, ACE_Event_Handler::READ_MASK
                                                        | ACE_Event_Handler::DONT_CALL);
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"handle close rtsp push session[%u] fail, remove read mask fail[%d].",
                         m_unSessionIndex, ACE_OS::last_error());
        return -1;
    }

    (void)ACE_OS::close(m_sockHandle);
    m_sockHandle = ACE_INVALID_HANDLE;

    clearRtspCachedMessage();

    destroyMediaSession();

    setStatus(RTSP_SESSION_STATUS_TEARDOWN);
    SVS_LOG(SVS_LOG_INFO,"handle close rtsp push session[%u] success.", m_unSessionIndex);

    return 0;
}

void mk_rtsp_client::setHandle(ACE_HANDLE handle, const ACE_INET_Addr &localAddr)
{
    m_sockHandle = handle;
    m_LocalAddr  = localAddr;

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] set handle[%d], local addr[%s:%d].",
                    m_unSessionIndex, handle,
                    m_LocalAddr.get_host_addr(), m_LocalAddr.get_port_number());
    return;
}

ACE_HANDLE mk_rtsp_client::get_handle() const
{
    return m_sockHandle;
}

uint32_t mk_rtsp_client::getSessionIndex() const
{
    return m_unSessionIndex;
}

int32_t mk_rtsp_client::check()
{
    uint32_t ulCostTime = SVS_GetSecondTime() - m_ulStatusTime;
    if ((RTSP_SESSION_STATUS_SETUP >= getStatus())
            && (ulCostTime > STREAM_STATUS_TIMEOUT_INTERVAL))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] check status abnormal,"
                " close session.",
                m_unSessionIndex);
        close();
        return AS_ERROR_CODE_OK;
    }

    if (m_pRtpSession)
    {
        if ((STREAM_SESSION_STATUS_ABNORMAL == m_pRtpSession->getStatus())
                || (STREAM_SESSION_STATUS_RELEASED == m_pRtpSession->getStatus()))
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] check status abnormal,"
                    " close rtp session[%lld].",
                    m_unSessionIndex, m_pRtpSession->getStreamId());
            close();
            return AS_ERROR_CODE_OK;
        }
    }

    if ((RTSP_SESSION_STATUS_TEARDOWN == getStatus())
            && (ulCostTime > STREAM_STATUS_ABNORMAL_INTERVAL))
    {
        SVS_LOG(SVS_LOG_INFO,"check rtsp push session[%u] teardown, release session.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_client::setSockOpt()
{
    if (ACE_INVALID_HANDLE == m_sockHandle)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (ACE_OS::fcntl(m_sockHandle, F_SETFL, ACE_OS::fcntl(m_sockHandle, F_GETFL) | O_NONBLOCK))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] set O_NONBLOCK fail, errno[%d].",
                        m_unSessionIndex,
                        errno);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_client::sendMessage(const char* pData, uint32_t unDataSize)
{
    if (NULL != m_pRtpSession)
    {
        return m_pRtpSession->sendMessage(pData, unDataSize,true);
    }

    ACE_Time_Value timeout(1);
    int32_t nSendSize = ACE::send_n(m_sockHandle, pData, unDataSize, &timeout);
    if (unDataSize != (uint32_t)nSendSize)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] send message fail, close handle[%d].",
                        m_unSessionIndex, m_sockHandle);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_client::sendMediaSetupReq(CSVSMediaLink* linkInof)
{
    std::string   strUrl = linkInof->Url();
    //uint64_t ullBusinessId = 0;

    m_enPlayType    = linkInof->PlayType();

    /* allocate the local session first */
    int32_t nRet = createDistribute(linkInof);
    if(AS_ERROR_CODE_OK != nRet) {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] set the media sdp info.rtspUrl:\n %s",
                    m_unSessionIndex, strUrl.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pPeerSession) {
        return AS_ERROR_CODE_FAIL;
    }
    //ullBusinessId = m_pPeerSession->getBusinessId();
    nRet = m_pPeerSession->sendSetup2Control(m_unSessionIndex,linkInof);
    if(AS_ERROR_CODE_OK != nRet) {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] set the media sdp info.rtspUrl:\n %s",
                    m_unSessionIndex, strUrl.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    m_bSetUp = true;

    return AS_ERROR_CODE_OK;
}

void mk_rtsp_client::sendMediaPlayReq()
{
    uint32_t unMsgLen = sizeof(SVS_MSG_STREAM_SESSION_PLAY_REQ);

    CStreamSvsMessage *pMessage = NULL;
    int32_t nRet = CStreamMsgFactory::instance()->createSvsMsg(SVS_MSG_TYPE_STREAM_SESSION_PLAY_REQ,
                                                    unMsgLen, 0,pMessage);
    if ((AS_ERROR_CODE_OK != nRet) || (NULL == pMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] create play request fail.",
                        m_unSessionIndex);
        return ;
    }

    CStreamMediaPlayReq *pReq = dynamic_cast<CStreamMediaPlayReq*>(pMessage);
    if ((NULL == pReq)|| (NULL == m_pPeerSession))
    {
        return ;
    }

    // nRet = pReq->initMsgBody((uint8_t*)m_strContentID.c_str(),m_pPeerSession->getBusinessId());
    nRet = pReq->initMsgBody((uint8_t*)(m_pMediaLink.DeviceID()).c_str(),m_pPeerSession->getBusinessId());
    if (AS_ERROR_CODE_OK != nRet)
    {
        CStreamMsgFactory::instance()->destroySvsMsg(pMessage);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] init stream play request fail.",
                                m_unSessionIndex);
        return ;
    }

    if (AS_ERROR_CODE_OK != pReq->handleMessage())
    {
        CStreamMsgFactory::instance()->destroySvsMsg(pMessage);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle stream play request fail.",
                m_unSessionIndex);
        return ;
    }

    CStreamMsgFactory::instance()->destroySvsMsg(pMessage);
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] send stream play request success.",
            m_unSessionIndex);
    return;
}

void mk_rtsp_client::sendKeyFrameReq()
{
    uint32_t unMsgLen = sizeof(SVS_MSG_STREAM_KEY_FRAME_REQ);

    CStreamSvsMessage *pMessage = NULL;
    int32_t nRet = CStreamMsgFactory::instance()->createSvsMsg(SVS_MSG_TYPE_MEDIA_KEYFRAME_REQ,
                                                    unMsgLen, 0,pMessage);
    if ((AS_ERROR_CODE_OK != nRet) || (NULL == pMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] create stream key frame request fail.",
                        m_unSessionIndex);
        return ;
    }

    CStreamMediaKeyFrameReq *pReq = dynamic_cast<CStreamMediaKeyFrameReq*>(pMessage);
    if ((NULL == pReq)|| (NULL == m_pPeerSession))
    {
        return ;
    }

    nRet = pReq->initMsgBody((uint8_t*)m_strContentID.c_str(),m_pPeerSession->getBusinessId());

    if (AS_ERROR_CODE_OK != nRet)
    {
        CStreamMsgFactory::instance()->destroySvsMsg(pMessage);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] init stream key frame request fail.",
                                m_unSessionIndex);
        return ;
    }

    if (AS_ERROR_CODE_OK != pReq->handleMessage())
    {
        CStreamMsgFactory::instance()->destroySvsMsg(pMessage);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle stream key frame request fail.",
                m_unSessionIndex);
        return ;
    }

    CStreamMsgFactory::instance()->destroySvsMsg(pMessage);
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] send stream key frame request success.",
            m_unSessionIndex);
    return;
}

int32_t mk_rtsp_client::createMediaSession()
{
    CStreamSession *pSession = CStreamSessionFactory::instance()->createSession(PEER_TYPE_CU, RTSP_SESSION,true);
    if (!pSession)
    {
        SVS_LOG(SVS_LOG_ERROR,"rtsp push session[%u] create media session fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    CStreamStdRtpSession* pStdSession = dynamic_cast<CStreamStdRtpSession*>(pSession);
    if (!pStdSession)
    {
        CStreamSessionFactory::instance()->releaseSession(pSession);
        SVS_LOG(SVS_LOG_ERROR,"rtsp push session[%u] create std media session fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (!m_pPeerSession)
    {
        CStreamSessionFactory::instance()->releaseSession(pSession);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtsp play request fail, "
                "can't find peer session ContentID[%s]",
                m_unSessionIndex, m_strContentID.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    /* the peer session release by the destory the media session */
    uint64_t ullPeerSessionId = 0;
    ullPeerSessionId = m_pPeerSession->getStreamId();

    pStdSession->setRtspHandle(m_sockHandle, m_PeerAddr);
    uint8_t VideoPayloadType = PT_TYPE_H264;
    uint8_t AudioPayloadType = PT_TYPE_PCMU;
    MEDIA_INFO_LIST VideoinfoList;
    MEDIA_INFO_LIST AudioinfoList;
    m_RtspSdp.getVideoInfo(VideoinfoList);
    m_RtspSdp.getAudioInfo(AudioinfoList);

    if(0 <VideoinfoList.size()) {
        VideoPayloadType = VideoinfoList.front().ucPayloadType;
    }
    if(0 <AudioinfoList.size()) {
        AudioPayloadType = AudioinfoList.front().ucPayloadType;
    }

    pStdSession->setPlayLoad(VideoPayloadType,AudioPayloadType);
    if (AS_ERROR_CODE_OK != pStdSession->initStdRtpSession(ullPeerSessionId,
                                                 m_enPlayType,
                                                 m_LocalAddr,
                                                 m_PeerAddr))
    {
        CStreamSessionFactory::instance()->releaseSession(pSession);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] init rtp session fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    m_pRtpSession = pStdSession;
    if(NULL != m_pRtpSession)
    {
        m_pRtpSession->setSdpInfo(m_RtspSdp);
        m_pRtpSession->setContentID(m_pPeerSession->getContentID());
    }


    CStreamBusiness *pBusiness = CStreamBusinessManager::instance()->createBusiness(ullPeerSessionId,
                                                                        pStdSession->getStreamId(),
                                                                        pStdSession->getPlayType());
    if (!pBusiness)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] create business fail, pu[%lld] cu[%lld].",
                        m_unSessionIndex, pSession->getStreamId(), ullPeerSessionId);

        CStreamSessionFactory::instance()->releaseSession(pSession);
        m_pRtpSession = NULL;
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != pBusiness->start())
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] start business fail, pu[%lld] cu[%lld].",
                m_unSessionIndex, pSession->getStreamId(), ullPeerSessionId);

        CStreamSessionFactory::instance()->releaseSession(pSession);

        CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
        m_pRtpSession = NULL;
        return AS_ERROR_CODE_FAIL;
    }


    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] create session success, cu[%lld] pu[%lld].",
                    m_unSessionIndex, pSession->getStreamId(), ullPeerSessionId);
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_client::destroyMediaSession()
{
    SVS_LOG(SVS_LOG_DEBUG,"rtsp push session destory media session.");
    uint64_t ullRtpSessionId  = 0;
    uint64_t ullPeerSessionId = 0;
    uint64_t ullBusinessId = 0;
    if (m_pRtpSession)
    {
        ullRtpSessionId = m_pRtpSession->getStreamId();
        CStreamBusiness *pBusiness = CStreamBusinessManager::instance()->findBusiness(ullRtpSessionId);
        if (!pBusiness)
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] not find the buinsess.", m_unSessionIndex);
            return;
        }

        CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
        CStreamBusinessManager::instance()->releaseBusiness(pBusiness);

        CStreamSessionFactory::instance()->releaseSession(ullRtpSessionId);
        m_pRtpSession = NULL;
    }
    else
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] media session already released.", m_unSessionIndex);
    }

    if(NULL != m_pPeerSession)
    {
        ullPeerSessionId = m_pPeerSession->getStreamId();
        ullBusinessId = m_pPeerSession->getBusinessId();
        SVS_LOG(SVS_LOG_DEBUG,"rtsp push session:[%lld], release PeerSession:[%lld].",
                                                          ullRtpSessionId,ullPeerSessionId);
        CStreamSessionFactory::instance()->releaseSession(ullPeerSessionId);
        m_pPeerSession = NULL;
    }

    return;
}


int32_t mk_rtsp_client::processRecvedMessage(const char* pData, uint32_t unDataSize)
{
    if ((NULL == pData) || (0 == unDataSize))
    {
        return -1;
    }

    if (RTSP_INTERLEAVE_FLAG == pData[0])
    {
        return handleRTPRTCPData(pData, unDataSize);
    }

    int32_t nMessageLen = m_RtspProtocol.IsParsable(pData, unDataSize);
    if (0 > nMessageLen)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] parse rtsp message fail.", m_unSessionIndex);
        return -1;
    }

    if (0 == nMessageLen)
    {
        return 0;
    }

    CRtspMessage *pMessage = NULL;
    int32_t nRet = m_RtspProtocol.DecodeRtspMessage(pData, (uint32_t)nMessageLen, pMessage);
    if ((AS_ERROR_CODE_OK != nRet) || (!pMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] decode rtsp message fail.", m_unSessionIndex);
        return nMessageLen;
    }

    if (AS_ERROR_CODE_OK != handleRtspMessage(*pMessage))
    {
        delete pMessage;
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtsp message fail.", m_unSessionIndex);
        return nMessageLen;
    }

    delete pMessage;
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] success to process rtsp message.", m_unSessionIndex);
    return nMessageLen;
}

int32_t mk_rtsp_client::handleRTPRTCPData(const char* pData, uint32_t unDataSize) const
{
    if (unDataSize < RTSP_INTERLEAVE_HEADER_LEN)
    {
        return 0;
    }
    uint32_t unMediaSize = (uint32_t) ACE_NTOHS(*(uint16_t*)(void*)&pData[2]);
    if (unDataSize - RTSP_INTERLEAVE_HEADER_LEN < unMediaSize)
    {
        return 0;
    }

    if (m_pRtpSession)
    {
        if (!m_pPeerSession)
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtcp message fail, "
                    "can't find peer session.",m_unSessionIndex);

            return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN);
        }
        if(TRANS_PROTOCAL_TCP == m_unTransType)
        {
            if ((m_cVideoInterleaveNum == pData[1])
            || (m_cAudioInterleaveNum == pData[1]))
            {
                handleMediaData((const char*)(pData+RTSP_INTERLEAVE_HEADER_LEN),unMediaSize);
            }
        }

        STREAM_INNER_MSG innerMsg;
        fillStreamInnerMsg((char*)&innerMsg,
                        m_pRtpSession->getStreamId(),
                        NULL,
                        m_PeerAddr.get_ip_address(),
                        m_PeerAddr.get_port_number(),
                        INNER_MSG_RTCP,
                        0);
        (void)m_pRtpSession->handleInnerMessage(innerMsg, sizeof(innerMsg), *m_pPeerSession);
    }

    return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN);
}

void mk_rtsp_client::handleMediaData(const char* pData, uint32_t unDataSize) const
{
    uint64_t ullRtpSessionId = 0;

    if(NULL == m_pRtpSession)
    {
        SVS_LOG(SVS_LOG_WARNING,"RtspPushSession,the rtp session is null.");
        return;
    }
    ullRtpSessionId = m_pRtpSession->getStreamId();

    ACE_Message_Block *pMsg = CMediaBlockBuffer::instance().allocMediaBlock();
    if (NULL == pMsg)
    {
        SVS_LOG(SVS_LOG_WARNING,"RtspPushSession alloc media block fail.");
        return ;
    }



    STREAM_TRANSMIT_PACKET *pPacket = (STREAM_TRANSMIT_PACKET *) (void*) pMsg->base();
    pMsg->wr_ptr(sizeof(STREAM_TRANSMIT_PACKET) - 1); //

    CRtpPacket rtpPacket;
    (void)rtpPacket.ParsePacket(pData ,unDataSize);

    pMsg->copy(pData, unDataSize);

    pPacket->PuStreamId = ullRtpSessionId;
    pPacket->enPacketType = STREAM_PACKET_TYPE_MEDIA_DATA;

    int32_t nRet = AS_ERROR_CODE_OK;
    nRet = CStreamMediaExchange::instance()->addData(pMsg);


    if (AS_ERROR_CODE_OK != nRet)
    {
        CMediaBlockBuffer::instance().freeMediaBlock(pMsg);

        return;
    }
    return;
}

int32_t mk_rtsp_client::handleRtspMessage(CRtspMessage &rtspMessage)
{
    if (RTSP_MSG_REQ != rtspMessage.getMsgType())
    {
        if (RTSP_METHOD_ANNOUNCE != rtspMessage.getMethodType())
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle not accepted method[%u].",
                              m_unSessionIndex, rtspMessage.getMethodType());
            return AS_ERROR_CODE_FAIL;
        }
    }

    int32_t nRet = AS_ERROR_CODE_OK;
    ACE_Guard<ACE_Recursive_Thread_Mutex> locker(m_RtspMutex);

    switch(rtspMessage.getMethodType())
    {
    case RTSP_METHOD_OPTIONS:
        nRet = handleRtspOptionsReq(rtspMessage);
        break;
    case RTSP_METHOD_DESCRIBE:
        nRet = handleRtspDescribeReq(rtspMessage);
        break;
    case RTSP_METHOD_SETUP:
        nRet = handleRtspSetupReq(rtspMessage);
        break;
    case RTSP_METHOD_PLAY:
        nRet = handleRtspPlayReq(rtspMessage);
        break;
    case RTSP_METHOD_PAUSE:
        nRet = handleRtspPauseReq(rtspMessage);
        break;
    case RTSP_METHOD_TEARDOWN:
        nRet = handleRtspTeardownReq(rtspMessage);
        break;
    case RTSP_METHOD_ANNOUNCE:
        nRet = handleRtspAnnounceReq(rtspMessage);
        break;
    case RTSP_METHOD_RECORD:
        nRet = handleRtspRecordReq(rtspMessage);
        break;
    case RTSP_METHOD_GETPARAMETER:
        nRet = handleRtspGetParameterReq(rtspMessage);
        break;
    default:
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle not accepted method[%u].",
                        m_unSessionIndex, rtspMessage.getMethodType());
        return AS_ERROR_CODE_FAIL;
    }
    return nRet;
}

int32_t mk_rtsp_client::handleRtspOptionsReq(CRtspMessage &rtspMessage)
{
    CRtspOptionsMessage *pRequest = dynamic_cast<CRtspOptionsMessage*>(&rtspMessage);
    if (NULL == pRequest)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle options request fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }


    pRequest->setRange(m_strPlayRange);
    pRequest->setStatusCode(RTSP_SUCCESS_OK);
    pRequest->setMsgType(RTSP_MSG_RSP);
    std::stringstream sessionIdex;
    sessionIdex << m_unSessionIndex;
    pRequest->setSession(sessionIdex.str());

    std::string strResp;
    if (AS_ERROR_CODE_OK != pRequest->encodeMessage(strResp))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] encode options response fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] send options response fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] send options response success.", m_unSessionIndex);

    simulateSendRtcpMsg();

    return AS_ERROR_CODE_OK;
}

uint32_t mk_rtsp_client::getRange(const std::string strUrl,std::string & strStartTime,std::string & strStopTime)
{
    std::string::size_type nStart = 0;
    std::string::size_type nStop  = 0;
    nStart = strUrl.find("-");
    std::string tmp = std::string(strUrl);

    for (int32_t i=0;i<3;i++)
    {
        nStart = tmp.find("-");
        tmp = tmp.substr(nStart+1);
    }

    nStop  = tmp.find("&user");

    tmp = tmp.substr(0,nStop);


    nStop  = tmp.find("-");

    strStartTime = "";
    strStopTime  = "";

    strStartTime = tmp.substr(0,nStop);
    strStopTime  = tmp.substr(nStop+1);

    std::stringstream strStart;
    std::stringstream strEnd;

    struct tm rangeTm;
    memset(&rangeTm, 0x0, sizeof(rangeTm));

    char* pRet = strptime(strStartTime.c_str(), "%Y%m%d%H%M%S", &rangeTm);
    if (NULL == pRet)
    {
        return 0;
    }

    uint32_t iStartTime = 0;
    iStartTime =  (uint32_t) mktime(&rangeTm);
    strStart<<iStartTime;
    strStart>>strStartTime;

    memset(&rangeTm, 0x0, sizeof(rangeTm));

    pRet = strptime(strStopTime.c_str(), "%Y%m%d%H%M%S", &rangeTm);
    if (NULL == pRet)
    {
        return 0;
    }

    uint32_t iStopTime = 0;
    iStopTime =  (uint32_t) mktime(&rangeTm);
    strEnd<<iStopTime;
    strEnd>>strStopTime;


    return iStopTime-iStartTime;
}


int32_t mk_rtsp_client::handleRtspDescribeReq(const CRtspMessage &rtspMessage)
{
    //CSVSMediaLink MediaLink;
    if (RTSP_SESSION_STATUS_INIT != getStatus())
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle describe req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = CSVSMediaLinkFactory::instance().parseMediaUrl(rtspMessage.getRtspUrl(),&m_pMediaLink);
    if((SVS_MEDIA_LINK_RESULT_SUCCESS != nRet)
        &&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL != nRet))
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle describe req fail, content invalid.");
        return AS_ERROR_CODE_FAIL;
    }
    if(SVS_MEDIA_LINK_RESULT_AUTH_FAIL == nRet)
    {
        if(CStreamConfig::instance()->getUrlEffectiveWhile())
        {
            close();
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle describe req fail, auth invalid.",m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }
    }

    m_strContentID = m_pMediaLink.ContentID();

    if(NULL == m_pPeerSession)
    {
        m_pPeerSession = CStreamSessionFactory::instance()->findSession(m_strContentID);
        if ((NULL != m_pPeerSession)&&(PLAY_TYPE_AUDIO_LIVE == m_enPlayType))
        {
              CStreamSessionFactory::instance()->releaseSession(m_pPeerSession);
              m_pPeerSession = NULL;
              SVS_LOG(SVS_LOG_ERROR, "The Audio Request failed");
              return AS_ERROR_CODE_FAIL;
        }
    }
    
 
    if (NULL == m_pPeerSession)
    {
        CRtspDescribeMessage *pReq = new CRtspDescribeMessage();
        if (!pReq)  //lint !e774
        {
            return AS_ERROR_CODE_FAIL;
        }
        pReq->setRtspUrl(rtspMessage.getRtspUrl());
        pReq->setCSeq(rtspMessage.getCSeq());
        std::stringstream sessionIdex;
        sessionIdex << m_unSessionIndex;
        pReq->setSession(sessionIdex.str());

        if (!m_pLastRtspMsg)
        {
            delete m_pLastRtspMsg;
        }
        m_pLastRtspMsg = pReq;
        SVS_LOG(SVS_LOG_INFO,"rtsp session[%u] save describe request[%p].",
                        m_unSessionIndex, m_pLastRtspMsg);

        if (AS_ERROR_CODE_OK != sendMediaSetupReq(&m_pMediaLink))
        {
            delete m_pLastRtspMsg;
            m_pLastRtspMsg = NULL;

            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle describe request fail, "
                    "send setup request fail.",
                    m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        return AS_ERROR_CODE_OK;
    }


    std::string strSdp;
    SDP_MEDIA_INFO  videoInfo, audioInfo;


    SDP_MEDIA_INFO stVideoInfo;
    SDP_MEDIA_INFO stAideoInfo;

    stVideoInfo.strControl   = "";
    stVideoInfo.strFmtp      = "";
    stVideoInfo.strRtpmap    = "";
    stVideoInfo.ucPayloadType= PT_TYPE_H264;
    stVideoInfo.usPort       = 0;

    stAideoInfo.strControl   = "";
    stAideoInfo.strFmtp      = "";
    stAideoInfo.strRtpmap    = "";
    stAideoInfo.ucPayloadType= PT_TYPE_PCMU;
    stAideoInfo.usPort       = 0;

     if ( (PLAY_TYPE_LIVE == m_enPlayType) || (PLAY_TYPE_FRONT_RECORD == m_enPlayType) || (PLAY_TYPE_PLAT_RECORD == m_enPlayType))
    {
        stVideoInfo.ucPayloadType = PT_TYPE_H264; /* PS -->H264 */
        m_RtspSdp.addVideoInfo(stVideoInfo);
        stVideoInfo.ucPayloadType = PT_TYPE_H265; /* PS -->H265 */
        m_RtspSdp.addVideoInfo(stVideoInfo);
        // stAideoInfo.ucPayloadType= PT_TYPE_PCMU;
        // m_RtspSdp.addAudioInfo(stAideoInfo);
        stAideoInfo.ucPayloadType= PT_TYPE_PCMA;
        m_RtspSdp.addAudioInfo(stAideoInfo);
    }
    else if ( PLAY_TYPE_AUDIO_LIVE == m_enPlayType)//�����GB28181Э�������PCMA,�����EHOME
    {
        
        stAideoInfo.ucPayloadType= PT_TYPE_PCMA;
        m_RtspSdp.addAudioInfo(stAideoInfo);
    }

    int32_t isplayback = 0;
    std::string strtimeRange="";
    if ( PLAY_TYPE_PLAT_RECORD == m_enPlayType)
    {
        isplayback = 1;
        if ("" == m_RtspSdp.getRange())
        {
            std::string strtmp = "";
            std::string strtmpUrl = m_RtspSdp.getUrl();
            std::string strStartTime = "";
            std::string strEndTime = "";
            strtmp += "range:npt=0-";
            uint32_t num = 0;
            num = getRange(strtmpUrl,strStartTime,strEndTime);

        if( 0 == num)
        {
            SVS_LOG(SVS_LOG_WARNING,"get timeRange fail,range in url is 0.");
            return AS_ERROR_CODE_FAIL;
        }

         std::stringstream stream;
         stream<<num;
         stream>>strtimeRange;
         strtmp += strtimeRange;
         SVS_LOG(SVS_LOG_WARNING,"time is = [%s]",strtmp.c_str());
         m_RtspSdp.setRange(strtmp);
        }
    }

    if (AS_ERROR_CODE_OK != m_RtspSdp.encodeSdp(strSdp,isplayback,strtimeRange,0,0))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] encode sdp info fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }


    CRtspDescribeMessage resp;
    resp.setMsgType(RTSP_MSG_RSP);
    resp.setCSeq(rtspMessage.getCSeq());
    resp.setStatusCode(RTSP_SUCCESS_OK);

    std::stringstream sessionIdex;
    sessionIdex << m_unSessionIndex;
    resp.setSession(sessionIdex.str());
    resp.setSdp(strSdp);

    std::string strResp;
    if (AS_ERROR_CODE_OK != resp.encodeMessage(strResp))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] encode describe response fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] send describe response fail.", m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }
    m_bSetUp = true;

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle describe request success.", m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_client::handleRtspSetupReq(CRtspMessage &rtspMessage)
{
    std::string  strContentID;
    //CSVSMediaLink MediaLink;

    if (RTSP_SESSION_STATUS_SETUP < getStatus())
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle setup req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspSetupMessage *pReq = dynamic_cast<CRtspSetupMessage*>(&rtspMessage);
    if (!pReq)  //lint !e774
    {
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = CSVSMediaLinkFactory::instance().parseMediaUrl(rtspMessage.getRtspUrl(),&m_pMediaLink);
    if((SVS_MEDIA_LINK_RESULT_SUCCESS != nRet)
        &&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL != nRet))
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle setup req fail, content invalid.");
        return AS_ERROR_CODE_FAIL;
    }
    if((!m_bSetUp)&&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL == nRet))
    {
        if(CStreamConfig::instance()->getUrlEffectiveWhile())
        {
            close();
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle setup req fail, auth invalid.",m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }
    }
    strContentID = m_pMediaLink.ContentID();

    if(NULL == m_pPeerSession)
    {
        m_pPeerSession = CStreamSessionFactory::instance()->findSession(m_strContentID);
    }

    if (NULL == m_pPeerSession)
    {
        CRtspSetupMessage *pSetupReq = new CRtspSetupMessage();
        if (!pSetupReq)  //lint !e774
        {
            return AS_ERROR_CODE_FAIL;
        }
        pSetupReq->setRtspUrl(rtspMessage.getRtspUrl());
        pSetupReq->setCSeq(rtspMessage.getCSeq());
        //pSetupReq->setSession(rtspMessage.getSession());
        std::stringstream sessionIdex;
        sessionIdex << m_unSessionIndex;
        pSetupReq->setSession(sessionIdex.str());
        pSetupReq->setTransType(pReq->getTransType());
        pSetupReq->setInterleaveNum(pReq->getInterleaveNum());
        pSetupReq->setClientPort(pReq->getClientPort());
        pSetupReq->setDestinationIp(pReq->getDestinationIp());

        if (!m_pLastRtspMsg)
        {
            delete m_pLastRtspMsg;
        }
        m_pLastRtspMsg = pSetupReq;

        if (AS_ERROR_CODE_OK != sendMediaSetupReq(&m_pMediaLink))
        {
            delete m_pLastRtspMsg;
            m_pLastRtspMsg = NULL;

            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle describe request fail, "
                    "send setup request fail.",
                    m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        SVS_LOG(SVS_LOG_INFO,"rtsp session[%u] save setup request[%p], send media setup request to SCC.",
                        m_unSessionIndex, m_pLastRtspMsg);
        return AS_ERROR_CODE_OK;
    }


    if (!m_pRtpSession)
    {
        if (AS_ERROR_CODE_OK != createMediaSession())
        {
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle setup request fail, "
                    "create media session fail.",
                    m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] create media session success.",
                        m_unSessionIndex);
    }

    pReq->setDestinationIp(m_PeerAddr.get_ip_address());

    std::stringstream sessionIdex;
    sessionIdex << m_unSessionIndex;
    pReq->setSession(sessionIdex.str());
    if (AS_ERROR_CODE_OK != m_pRtpSession->startStdRtpSession(*pReq))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] start media session fail.",
                                m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    ACE_INET_Addr addr;
    pReq->setMsgType(RTSP_MSG_RSP);
    pReq->setStatusCode(RTSP_SUCCESS_OK);
    m_unTransType = pReq->getTransType();
    if (TRANS_PROTOCAL_UDP == pReq->getTransType())
    {
        if (m_bFirstSetupFlag)
        {
            m_cVideoInterleaveNum = pReq->getInterleaveNum();
            addr.set(m_pRtpSession->getVideoAddr());
        }
        else
        {
            m_cAudioInterleaveNum = pReq->getInterleaveNum();
            addr.set(m_pRtpSession->getAudioAddr());
        }
        pReq->setServerPort(addr.get_port_number());
        pReq->setSourceIp(addr.get_ip_address());

        m_bFirstSetupFlag = false;
    }
    else
    {
        if (m_bFirstSetupFlag)
        {
            m_cVideoInterleaveNum = pReq->getInterleaveNum();
        }
        else
        {
            m_cAudioInterleaveNum = pReq->getInterleaveNum();
        }
        m_bFirstSetupFlag = false;
    }

    std::string strResp;
    if (AS_ERROR_CODE_OK != pReq->encodeMessage(strResp))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] encode setup response fail.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] send setup response fail.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    setStatus(RTSP_SESSION_STATUS_SETUP);
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle setup request success.",
                            m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_client::handleRtspRecordReq(CRtspMessage &rtspMessage)
{
    if(PLAY_TYPE_AUDIO_LIVE == m_enPlayType)
    {
        SVS_LOG(SVS_LOG_INFO,"Receive Remote Client Audio Request OK!");
        if (STREAM_SESSION_STATUS_WAIT_CHANNEL_REDAY == m_pRtpSession->getStatus())
        {
            BUSINESS_LIST businessList;
            CStreamBusinessManager::instance()->findBusiness( m_pRtpSession->getStreamId(), businessList);
            for (BUSINESS_LIST_ITER iter = businessList.begin();
                    iter != businessList.end(); iter++)
            {
                CStreamBusiness *pBusiness = *iter;
                if (AS_ERROR_CODE_OK != pBusiness->start())
                {
                    CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
                    SVS_LOG(SVS_LOG_WARNING,"start distribute fail, stream[%lld] start business fail.",
                                    m_pRtpSession->getStreamId());

                    return AS_ERROR_CODE_FAIL;
                }

                CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
            }

            STREAM_INNER_MSG innerMsg;
            innerMsg.ullStreamID = m_pRtpSession->getStreamId();
            innerMsg.unBodyOffset = sizeof(STREAM_INNER_MSG);
            innerMsg.usMsgType = INNER_MSG_RTSP;

            (void) m_pRtpSession->handleInnerMessage(innerMsg,  sizeof(STREAM_INNER_MSG), *m_pPeerSession);

            sendMediaPlayReq();
        }

        if ((STREAM_SESSION_STATUS_DISPATCHING != m_pPeerSession->getStatus()))
        {
            return cacheRtspMessage(rtspMessage);
        }
        //sendMediaPlayReq();
        sendCommonResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

        setStatus(RTSP_SESSION_STATUS_PLAY);

        clearRtspCachedMessage();

        SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle rtsp play aduio request success.",
                        m_unSessionIndex);
        return  AS_ERROR_CODE_OK;
    }
    if (!m_pRtpSession)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_SESSION_STATUS_SETUP > getStatus())
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle record req fail, status[%u] invalid.",
                    m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspRecordMessage *pReq = dynamic_cast<CRtspRecordMessage*>(&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    sendCommonResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    setStatus(RTSP_SESSION_STATUS_PLAY);

    clearRtspCachedMessage();

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle rtsp record request success.",
                        m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_client::handleRtspGetParameterReq(CRtspMessage &rtspMessage)
{
    sendCommonResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] send get parameter response success.", m_unSessionIndex);

    simulateSendRtcpMsg();

    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_client::handleRtspPlayReq(CRtspMessage &rtspMessage)
{
    if (!m_pRtpSession)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_SESSION_STATUS_SETUP > getStatus())
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle play req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspPlayMessage *pReq = dynamic_cast<CRtspPlayMessage*>(&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (!m_pPeerSession)
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtsp play request fail, "
                "can't find peer session.",m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (STREAM_SESSION_STATUS_WAIT_CHANNEL_REDAY == m_pRtpSession->getStatus())
    {
        BUSINESS_LIST businessList;
        CStreamBusinessManager::instance()->findBusiness( m_pRtpSession->getStreamId(), businessList);
        for (BUSINESS_LIST_ITER iter = businessList.begin();
                iter != businessList.end(); iter++)
        {
            CStreamBusiness *pBusiness = *iter;
            if (AS_ERROR_CODE_OK != pBusiness->start())
            {
                CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
                SVS_LOG(SVS_LOG_WARNING,"start distribute fail, stream[%lld] start business fail.",
                                m_pRtpSession->getStreamId());

                return AS_ERROR_CODE_FAIL;
            }

            CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
        }

        STREAM_INNER_MSG innerMsg;
        innerMsg.ullStreamID = m_pRtpSession->getStreamId();
        innerMsg.unBodyOffset = sizeof(STREAM_INNER_MSG);
        innerMsg.usMsgType = INNER_MSG_RTSP;

        (void) m_pRtpSession->handleInnerMessage(innerMsg,  sizeof(STREAM_INNER_MSG), *m_pPeerSession);

        if (!pReq->hasRange())
        {
            MEDIA_RANGE_S   stRange;
            stRange.enRangeType = RANGE_TYPE_NPT;
            stRange.MediaBeginOffset = OFFSET_CUR;
            stRange.MediaEndOffset = OFFSET_END;

            pReq->setRange(stRange);
        }
        else
        {
            pReq->getRange(m_strPlayRange);
        }
    }

    if ((STREAM_SESSION_STATUS_DISPATCHING != m_pPeerSession->getStatus()))
    {
        return cacheRtspMessage(rtspMessage);
    }


    sendCommonResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());


    setStatus(RTSP_SESSION_STATUS_PLAY);

    clearRtspCachedMessage();

    //send the play request
    if(m_bSetUp){
        sendMediaPlayReq();
    }
    else
    {
        /* Key Frame request */
        sendKeyFrameReq();
    }
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle rtsp play request success.",
                    m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_client::handleRtspAnnounceReq(const CRtspMessage &rtspMessage)
{
    if (RTSP_SESSION_STATUS_INIT != getStatus())
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle describe req fail, status[%u] invalid.",
                    m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }
    //CSVSMediaLink MediaLink;

    int32_t nRet = CSVSMediaLinkFactory::instance().parseMediaUrl(rtspMessage.getRtspUrl(),&m_pMediaLink);
    if((SVS_MEDIA_LINK_RESULT_SUCCESS != nRet)
        &&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL != nRet))
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle announce req fail, content invalid.");
        return AS_ERROR_CODE_FAIL;
    }
    if(SVS_MEDIA_LINK_RESULT_AUTH_FAIL == nRet)
    {
        if(CStreamConfig::instance()->getUrlEffectiveWhile())
        {
            close();
            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle announce req fail, auth invalid.",m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(NULL == m_pPeerSession)
    {
        m_pPeerSession = CStreamSessionFactory::instance()->findSession(m_strContentID);
    }

    if (NULL == m_pPeerSession)
    {
        CRtspAnnounceMessage *pReq = new CRtspAnnounceMessage();
        if (!pReq)  //lint !e774
        {
            return AS_ERROR_CODE_FAIL;
        }
        pReq->setRtspUrl(rtspMessage.getRtspUrl());
        pReq->setCSeq(rtspMessage.getCSeq());
        //pReq->setSession(rtspMessage.getSession());
        std::stringstream sessionIdex;
        sessionIdex << m_unSessionIndex;
        pReq->setSession(sessionIdex.str());
        std::string strContendType = rtspMessage.getContetType();
        std::string strContend = rtspMessage.getBody();
        pReq->setBody(strContendType,strContend);

        if (!m_pLastRtspMsg)
        {
            delete m_pLastRtspMsg;
        }
        m_pLastRtspMsg = pReq;
        SVS_LOG(SVS_LOG_INFO,"rtsp session[%u] save Announce request[%p].",
                            m_unSessionIndex, m_pLastRtspMsg);

        if (AS_ERROR_CODE_OK != sendMediaSetupReq(&m_pMediaLink))
        {
            delete m_pLastRtspMsg;
            m_pLastRtspMsg = NULL;

            SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle Announce request fail, "
                        "send setup request fail.",
                        m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        return AS_ERROR_CODE_OK;
    }

    std::string strSdp = rtspMessage.getBody();
    if (AS_ERROR_CODE_OK == m_RtspSdp.decodeSdp(strSdp))
    {
        m_RtspSdp.setUrl(rtspMessage.getRtspUrl());
    }

    sendCommonResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle describe request success.", m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_client::handleRtspPauseReq(CRtspMessage &rtspMessage)
{
    if (RTSP_SESSION_STATUS_PLAY != getStatus()
            && RTSP_SESSION_STATUS_PAUSE != getStatus())
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle pause req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspPauseMessage *pReq = dynamic_cast<CRtspPauseMessage*>(&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (!m_pPeerSession)
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtsp play request fail, "
                "can't find peer session.",m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    std::string strRtsp;
    (void)pReq->encodeMessage(strRtsp);

    CRtspPacket rtspPack;
    if (0 != rtspPack.parse(strRtsp.c_str(), strRtsp.length()))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtsp pause request fail, "
                        "parse rtsp packet fail.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != m_pPeerSession->sendVcrMessage(rtspPack))
    {
        sendCommonResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] handle rtsp pause request fail, "
                "peer session send vcr message fail.",
                m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    setStatus(RTSP_SESSION_STATUS_PAUSE);
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle rtsp pause request success.",
                     m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}

// TEARDOWN
int32_t mk_rtsp_client::handleRtspTeardownReq(CRtspMessage &rtspMessage)
{
    rtspMessage.setMsgType(RTSP_MSG_RSP);
    rtspMessage.setStatusCode(RTSP_SUCCESS_OK);

    std::string strRtsp;
    if (AS_ERROR_CODE_OK != rtspMessage.encodeMessage(strRtsp))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] encode teardown response fail.", m_unSessionIndex);
    }

    if (AS_ERROR_CODE_OK != sendMessage(strRtsp.c_str(), strRtsp.length()))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] send teardown response fail.", m_unSessionIndex);
    }

    //close the session
    (void)handle_close(m_sockHandle, 0);
    setStatus(RTSP_SESSION_STATUS_TEARDOWN);

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] handle rtsp teardown request success.",
                     m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_client::sendCommonResp(uint32_t unStatusCode, uint32_t unCseq)
{
    std::string strResp;
    CRtspMessage::encodeCommonResp(unStatusCode, unCseq, m_unSessionIndex, strResp);

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] send common response fail.", m_unSessionIndex);
    }
    else
    {
        SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] send common response success.", m_unSessionIndex);
    }

    SVS_LOG(SVS_LOG_DEBUG,"%s", strResp.c_str());
    return;
}

int32_t mk_rtsp_client::cacheRtspMessage(CRtspMessage &rtspMessage)
{
    if (m_pLastRtspMsg)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] cache rtsp play message fail, "
                "last message[%p] invalid.",
                m_unSessionIndex, m_pLastRtspMsg);
        return AS_ERROR_CODE_FAIL;
    }

    ACE_Reactor *pReactor = reactor();
    if (!pReactor)
    {
        return AS_ERROR_CODE_FAIL;
    }

    CRtspPlayMessage *pReq = dynamic_cast<CRtspPlayMessage*> (&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    CRtspPlayMessage *pPlayMsg = new CRtspPlayMessage;
    if (!pPlayMsg)  //lint !e774
    {
        return AS_ERROR_CODE_FAIL;
    }

    pPlayMsg->setMsgType(RTSP_MSG_REQ);
    pPlayMsg->setRtspUrl(rtspMessage.getRtspUrl());
    pPlayMsg->setCSeq(rtspMessage.getCSeq());
    pPlayMsg->setSession(rtspMessage.getSession());

    MEDIA_RANGE_S stRange;
    pReq->getRange(stRange);
    pPlayMsg->setRange(stRange);
    pPlayMsg->setSpeed(pReq->getSpeed());
    pPlayMsg->setSpeed(pReq->getScale());

    m_pLastRtspMsg = pPlayMsg;


    if (-1 != m_lRedoTimerId)
    {
        (void)pReactor->cancel_timer(m_lRedoTimerId);
        SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] cancel redo timer[%d].",
                m_unSessionIndex, m_lRedoTimerId);
    }

    ACE_Time_Value timeout(0, RTSP_RETRY_INTERVAL);
    m_lRedoTimerId = pReactor->schedule_timer(this, 0, timeout, timeout);
    if (-1 == m_lRedoTimerId)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] cache rtsp play message, create timer fail.",
                    m_unSessionIndex);
        delete m_pLastRtspMsg;
        return AS_ERROR_CODE_FAIL;
    }

    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] cache rtsp play message[%p], timer id[%d].",
            m_unSessionIndex, m_pLastRtspMsg, m_lRedoTimerId);
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_client::clearRtspCachedMessage()
{
    if (-1 != m_lRedoTimerId)
    {
        ACE_Reactor *pReactor = reactor();
        if (pReactor)
        {
            (void)pReactor->cancel_timer(m_lRedoTimerId);
        }

        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] cancel redo timer[%d].",
                m_unSessionIndex, m_lRedoTimerId);
        m_lRedoTimerId = -1;
    }

    if (m_pLastRtspMsg)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] delete cache message[%p].",
                        m_unSessionIndex, m_pLastRtspMsg);
        delete m_pLastRtspMsg;
        m_pLastRtspMsg = NULL;
    }
    return;
}

int32_t mk_rtsp_client::checkTransDirection(uint32_t unPeerType, uint32_t unTransDirection) const
{
    if (PEER_TYPE_PU == unPeerType)
    {
        if ((TRANS_DIRECTION_RECVONLY == unTransDirection)
          ||(TRANS_DIRECTION_SENDRECV == unTransDirection))
        {
            return AS_ERROR_CODE_OK;
        }
    }
    else if (PEER_TYPE_CU == unPeerType)
    {
        if (TRANS_DIRECTION_RECVONLY == unTransDirection)
        {
            return AS_ERROR_CODE_OK;
        }
    }
    else if (PEER_TYPE_RECORD == unPeerType)
    {
        if (TRANS_DIRECTION_RECVONLY == unTransDirection)
        {
            return AS_ERROR_CODE_OK;
        }
    }
    else if (PEER_TYPE_STREAM == unPeerType)
    {
        if (TRANS_DIRECTION_RECVONLY == unTransDirection)
        {
            return AS_ERROR_CODE_OK;
        }
    }

    return AS_ERROR_CODE_FAIL;
}

int32_t mk_rtsp_client::createDistribute(CSVSMediaLink* linkinfo)
{
    std::string strUrl;
    CStreamSession *pPeerSession  = NULL;

    if(NULL == linkinfo)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] create distribute fail,get content fail.",m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }
    m_strContentID = linkinfo->ContentID();

    //create the peer session first
    if(SVS_DEV_TYPE_GB28181 == linkinfo->DevType()) {
        pPeerSession = CStreamSessionFactory::instance()->createSourceSession(m_strContentID, PEER_TYPE_PU, RTP_SESSION,false);
    }
    else if(SVS_DEV_TYPE_EHOME == linkinfo->DevType()) {
        pPeerSession = CStreamSessionFactory::instance()->createSourceSession(m_strContentID, PEER_TYPE_PU, EHOME_SESSION,false);
    }
    if (NULL == pPeerSession)
    {
        SVS_LOG(SVS_LOG_ERROR,"Create distribute fail, create peer session fail.");
        return AS_ERROR_CODE_FAIL;
    }
    if (AS_ERROR_CODE_OK != pPeerSession->init(m_strContentID.c_str(),linkinfo->PlayType()))
    {
        SVS_LOG(SVS_LOG_ERROR,"Create distribute fail,session init fail.");
        CStreamSessionFactory::instance()->releaseSession(pPeerSession);
        return AS_ERROR_CODE_FAIL;
    }

    m_pPeerSession     = pPeerSession;

    return AS_ERROR_CODE_OK;
}

void mk_rtsp_client::simulateSendRtcpMsg()
{
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] simulate send rtcp message begin.", m_unSessionIndex);


    uint64_t ullRtpSessionId = 0;

    if(NULL == m_pRtpSession)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] simulate Send Rtcp Msg,the rtp session is null.",
                        m_unSessionIndex);
        return;
    }
    ullRtpSessionId = m_pRtpSession->getStreamId();

    ACE_Message_Block *pMsg = CMediaBlockBuffer::instance().allocMediaBlock();
    if (NULL == pMsg)
    {
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] simulate Send Rtcp Msg, alloc media blockfail.",
                        m_unSessionIndex);
        return;
    }

    fillStreamInnerMsg(pMsg->base(),
                    ullRtpSessionId,
                    this,
                    m_PeerAddr.get_ip_address(),
                    m_PeerAddr.get_port_number(),
                    INNER_MSG_RTCP,
                    sizeof(STREAM_INNER_MSG));
    pMsg->wr_ptr(sizeof(STREAM_INNER_MSG));


    if (AS_ERROR_CODE_OK != CStreamServiceTask::instance()->enqueueInnerMessage(pMsg))
    {
        CMediaBlockBuffer::instance().freeMediaBlock(pMsg);
        SVS_LOG(SVS_LOG_WARNING,"rtsp push session[%u] simulate Send Rtcp Msg, enqueue inner message fail.",
                         m_unSessionIndex);
    }
    SVS_LOG(SVS_LOG_INFO,"rtsp push session[%u] simulate send rtcp message end.", m_unSessionIndex);
    return;
}


