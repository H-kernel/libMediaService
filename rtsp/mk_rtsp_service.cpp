#include <string.h>
#include "mk_rtsp_service.h"
#include "mk_rtsp_connection.h"
#include "mk_rtmp_connection.h"
#include "mk_media_common.h"

mk_rtsp_service::mk_rtsp_service()
{
    m_usUdpStartPort      = RTP_RTCP_START_PORT;
    m_ulUdpPairCount      = RTP_RTCP_PORT_COUNT;
    m_ulRtpBufCount       = 0;
    m_pUdpRtpArray        = NULL;
    m_pUdpRtcpArray       = NULL;
    m_pMutex              = NULL;
}

mk_rtsp_service::~mk_rtsp_service()
{
    if(NULL != m_pMutex)
    {
        as_destroy_mutex(m_pMutex);
        m_pMutex = NULL;
    }
}

int32_t mk_rtsp_service::init(uint32_t MaxClient,uint32_t RtpBufCountPerClient)
{
    int32_t nRet = AS_ERROR_CODE_OK;
    uint32_t i = 0;

    m_pMutex = as_create_mutex();
    if(NULL == m_pMutex)
    {
        MK_LOG(AS_LOG_WARNING,"create mutex fail.");
        return AS_ERROR_CODE_FAIL;
    }

    nRet = create_rtp_recv_bufs(MaxClient,RtpBufCountPerClient);
    if (AS_ERROR_CODE_OK != nRet)
    {
        MK_LOG(AS_LOG_WARNING,"create rtp recv bufs fail.");
        return AS_ERROR_CODE_FAIL;
    }

    MK_LOG(AS_LOG_INFO,"create rtp recv bufs success.");
    
    nRet = create_rtp_rtcp_udp_pairs();
    if (AS_ERROR_CODE_OK != nRet)
    {
        MK_LOG(AS_LOG_WARNING,"create rtp and rtcp pairs fail.");
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

void mk_rtsp_service::release()
{
    destory_rtp_recv_bufs();
    destory_rtp_rtcp_udp_pairs();
    return;
}
void    mk_rtsp_service::set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count)
{
    m_usUdpStartPort      = udpPort;
    m_ulUdpPairCount      = count;
}
void    mk_rtsp_service::get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count)
{
    udpPort = m_usUdpStartPort;
    count   = m_ulUdpPairCount;
}
void    mk_rtsp_service::set_rtp_recv_buf_info(uint32_t maxCount)
{
    m_ulRtpBufCount       = maxCount;
}
void    mk_rtsp_service::get_rtp_recv_buf_info(uint32_t& maxCount)
{
    maxCount = m_ulRtpBufCount;
}

int32_t mk_rtsp_service::get_rtp_rtcp_pair(mk_rtsp_udp_handle*&  pRtpHandle,mk_rtsp_udp_handle*&  pRtcpHandle)
{
    if(0 == m_RtpRtcpfreeList.size()) {
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t index = m_RtpRtcpfreeList.front();
    m_RtpRtcpfreeList.pop_front();
    pRtpHandle  = m_pUdpRtpArray[index];
    pRtcpHandle = m_pUdpRtcpArray[index];

    return AS_ERROR_CODE_OK;
}
void    mk_rtsp_service::free_rtp_rtcp_pair(mk_rtsp_udp_handle* pRtpHandle)
{
    uint32_t index = pRtpHandle->get_index();
    m_RtpRtcpfreeList.push_back(index);
    return;
}
char*   mk_rtsp_service::get_rtp_recv_buf()
{
    as_mutex_lock(m_pMutex);
    if(m_RtpRecvBufList.empty()) {
        MK_LOG(AS_LOG_ERROR,"get free recv buf fail,list is full");
        as_mutex_unlock(m_pMutex);
        return NULL;
    }

    char* buf = m_RtpRecvBufList.front();
    m_RtpRecvBufList.pop_front();

    as_mutex_unlock(m_pMutex);
    return buf;
}
void    mk_rtsp_service::free_rtp_recv_buf(char* buf)
{
    as_mutex_lock(m_pMutex);
    if(NULL == buf) {
        as_mutex_unlock(m_pMutex);
        return;
    }
    m_RtpRecvBufList.push_back(buf);

    as_mutex_unlock(m_pMutex);
    return;
}
int32_t mk_rtsp_service::create_rtp_rtcp_udp_pairs()
{
    MK_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs,start port:[%d],count:[%d].",m_usUdpStartPort,m_ulUdpPairCount);
    
    uint32_t port = m_usUdpStartPort;
    uint32_t pairs = m_ulUdpPairCount/2;
    mk_rtsp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_udp_handle* pRtcpHandle = NULL;
    int32_t nRet = AS_ERROR_CODE_OK;

    m_pUdpRtpArray  = AS_NEW(m_pUdpRtpArray,pairs);
    if(NULL == m_pUdpRtpArray) {
        MK_LOG(AS_LOG_CRITICAL,"create rtp handle array fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_pUdpRtcpArray = AS_NEW(m_pUdpRtcpArray,pairs);
    if(NULL == m_pUdpRtcpArray) {
        MK_LOG(AS_LOG_CRITICAL,"create rtcp handle array fail.");
        return AS_ERROR_CODE_FAIL;
    }

    for(uint32_t i = 0;i < pairs;i++) {
        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL == pRtpHandle) {
            MK_LOG(AS_LOG_CRITICAL,"create rtp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }        
        pRtpHandle->init(i,port);
        port++;

        pRtcpHandle = AS_NEW(pRtcpHandle);
        if(NULL == pRtcpHandle) {
            MK_LOG(AS_LOG_CRITICAL,"create rtcp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }        
        pRtcpHandle->init(i,port);
        port++;
        m_pUdpRtpArray[i]  = pRtpHandle;
        m_pUdpRtcpArray[i] = pRtcpHandle;
        m_RtpRtcpfreeList.push_back(i);
    }
    MK_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs success.");
    return AS_ERROR_CODE_OK;
}
void    mk_rtsp_service::destory_rtp_rtcp_udp_pairs()
{
    MK_LOG(AS_LOG_INFO,"m_pUdpRtcpArray udp rtp and rtcp pair.");

    mk_rtsp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_udp_handle* pRtcpHandle = NULL;

    for(uint32_t i = 0;i < m_ulUdpPairCount;i++) {
        pRtpHandle  = m_pUdpRtpArray[i];
        pRtcpHandle = m_pUdpRtcpArray[i];

        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL != pRtpHandle) {
            pRtpHandle->close();
            AS_DELETE(pRtpHandle);
            m_pUdpRtpArray[i] = NULL;
        }
        pRtcpHandle = AS_NEW(pRtcpHandle);
        if(NULL != pRtcpHandle) {
            pRtcpHandle->close();
            AS_DELETE(pRtcpHandle);
             m_pUdpRtcpArray[i] = NULL;
        }
    }
    MK_LOG(AS_LOG_INFO,"destory udp rtp and rtcp pairs success.");
    return;
}

int32_t mk_rtsp_service::create_rtp_recv_bufs(uint32_t MaxClient,uint32_t RtpBufCountPerClient)
{
    as_mutex_lock(m_pMutex);
    MK_LOG(AS_LOG_INFO,"create rtp recv buf begin.");
    m_ulRtpBufCount = MaxClient*RtpBufCountPerClient;
    if(0 == m_ulRtpBufCount) {
        MK_LOG(AS_LOG_ERROR,"there is no init rtp recv buf arguments,size:[%d] count:[%d].",RTP_RECV_BUF_SIZE,m_ulRtpBufCount);
        as_mutex_unlock(m_pMutex);
        return AS_ERROR_CODE_FAIL;
    }
    char* pBuf = NULL;
    for(uint32_t i = 0;i < m_ulRtpBufCount;i++) {
        pBuf = AS_NEW(pBuf,RTP_RECV_BUF_SIZE);
        if(NULL == pBuf) {
            MK_LOG(AS_LOG_ERROR,"create the rtp recv buf fail,size:[%d] index:[%d].",RTP_RECV_BUF_SIZE,i);
            as_mutex_unlock(m_pMutex);
            return AS_ERROR_CODE_FAIL;
        }
        m_RtpRecvBufList.push_back(pBuf);
    }
    MK_LOG(AS_LOG_INFO,"create rtp recv buf end.total rtp buf count[%d]",m_ulRtpBufCount);
    as_mutex_unlock(m_pMutex);
    return AS_ERROR_CODE_OK;
}
void    mk_rtsp_service::destory_rtp_recv_bufs()
{
    as_mutex_lock(m_pMutex);
    MK_LOG(AS_LOG_INFO,"destory rtp recv buf begin.");
    char* pBuf = NULL;
    while(0 <m_RtpRecvBufList.size()) {
        pBuf = m_RtpRecvBufList.front();
        m_RtpRecvBufList.pop_front();
        if(NULL == pBuf) {
            continue;
        }
        AS_DELETE(pBuf,MULTI);
    }
    MK_LOG(AS_LOG_INFO,"destory rtp recv buf end.");
    as_mutex_unlock(m_pMutex);
    return;
}