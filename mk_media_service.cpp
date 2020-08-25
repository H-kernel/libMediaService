#include <string.h>
#include "mk_media_service.h"
#include "mk_rtsp_connection.h"
#include "mk_rtmp_connection.h"

mk_media_service::mk_media_service()
{
    m_NetWorkArray        = NULL;
    m_ulEvnCount          = 0;
    m_usUdpStartPort      = RTP_RTCP_START_PORT;
    m_ulUdpPairCount      = RTP_RTCP_PORT_COUNT;
    m_ulRtpBufSize        = RTP_RECV_BUF_SIZE;
    m_ulRtpBufCount       = RTP_RECV_BUF_COUNT;
    m_ulFrameBufSize      = FRAME_RECV_BUF_SIZE;
    m_ulFramebufCount     = FRAME_RECV_BUF_COUNT;
    m_pUdpRtpArray        = NULL;
    m_pUdpRtcpArray       = NULL;
}

mk_media_service::~mk_media_service()
{
}

int32_t mk_media_service::init(uint32_t EvnCount,uint32_t MaxClient)
{
    int32_t nRet = AS_ERROR_CODE_OK;
    uint32_t i = 0;

    m_ulEvnCount = EvnCount;
    as_network_svr* pNetWork = NULL;

    /* create the network service array */
    m_NetWorkArray = AS_NEW(m_NetWorkArray,m_ulEvnCount);
    if(NULL == m_NetWorkArray){
        return AS_ERROR_CODE_FAIL;
    }
    for(i = 0;i < m_ulEvnCount;i++) {
        pNetWork = AS_NEW(pNetWork);
        if(NULL == pNetWork) {
            return AS_ERROR_CODE_FAIL;
        }
        pNetWork->setLogWriter(&m_connLog);

        nRet = pNetWork->init(DEFAULT_SELECT_PERIOD,AS_TRUE,AS_TRUE,AS_TRUE);
        if (AS_ERROR_CODE_OK != nRet)
        {
            AS_LOG(AS_LOG_WARNING,"init the network module fail.");
            return AS_ERROR_CODE_FAIL;
        }

        nRet = pNetWork->run();
        if (AS_ERROR_CODE_OK != nRet)
        {
            AS_LOG(AS_LOG_WARNING,"run the network module fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_NetWorkArray[i] = pNetWork;        
    }

    for(i = 0;i < MaxClient;i++) {
        m_ClientFreeList.push_back(i);    
    } 

    AS_LOG(AS_LOG_INFO,"init the network module success.");

    nRet = create_rtp_recv_bufs();
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"create rtp recv bufs fail.");
        return AS_ERROR_CODE_FAIL;
    }

    AS_LOG(AS_LOG_INFO,"create rtp recv bufs success.");

    nRet = create_frame_recv_bufs();
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"create frame recv bufs fail.");
        return AS_ERROR_CODE_FAIL;
    }

    AS_LOG(AS_LOG_INFO,"create frame recv bufs success.");
    
    nRet = create_rtp_rtcp_udp_pairs();
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"create rtp and rtcp pairs fail.");
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

void mk_media_service::release()
{
    destory_frame_recv_bufs();
    destory_rtp_recv_bufs();
    destory_rtp_rtcp_udp_pairs();
    m_NetWorkArray.exit();
    return;
}
mk_rtsp_server* mk_media_service::create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx)
{
    mk_rtsp_server* pRtspServer = NULL;
    pRtspServer = AS_NEW(pRtspServer);
    if(NULL == pRtspServer) {
        return NULL;
    }
    pRtspServer->set_callback(cb,ctx);
    as_network_addr local;
    local.m_lIpAddr = 0;
    local.m_usPort  = port;
    int32_t nRet = m_NetWorkArray.regTcpServer(&local,pRtspServer);
    if(AS_ERROR_CODE_OK != nRet) {
        AS_DELETE(pRtspServer);
        pRtspServer = NULL;
    }
    return pRtspServer;
}
void mk_media_service::destory_rtsp_server(mk_rtsp_server* pServer)
{
    if(NULL == pServer) {
        return;
    }
    m_NetWorkArray.removeTcpServer(pServer);
    AS_DELETE(pServer);
    return;
}
mk_client_connection* mk_media_service::create_client(char* url,handle_client_status cb,void* ctx)
{
    mk_client_connection* pClient   = NULL;
    mk_rtsp_connection* pRtspClient = NULL;
    mk_rtmp_connection* pRtmpClient = NULL;
    as_network_addr  local;
    as_network_addr  peer;

    if(0 == strncmp(url,RTSP_URL_PREFIX,strlen(RTSP_URL_PREFIX))) {
        pClient = AS_NEW(pRtspClient);
    }
    else if(0 == strncmp(url,RTMP_URL_PREFIX,strlen(RTMP_URL_PREFIX))) {
        pClient = AS_NEW(pRtmpClient);
    }

    if(NULL == pClient) {
        return AS_ERROR_CODE_FAIL;
    }


    pClient->set_status_callback(cb,ctx);
    if(AS_ERROR_CODE_OK != pClient->start(url)) {
        return NULL;
    }
    return pClient;
}
void mk_media_service::destory_client(mk_client_connection* pClient)
{
    if(NULL == pClient) {
        return;
    }
    pClient->stop();
    AS_DELETE(pClient);
    return;
}
as_network_svr* mk_media_service::get_client_network_svr(mk_client_connection* pClient)
{
    uint32_t ulIdx = pClient->get_index();
    ulIdx = ulIdx%m_ulEvnCount;
    return m_NetWorkArray[ulIdx];
}
void    mk_media_service::set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count)
{
    m_usUdpStartPort      = udpPort;
    m_ulUdpPairCount      = count;
}
void    mk_media_service::get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count)
{
    udpPort = m_usUdpStartPort;
    count   = m_ulUdpPairCount;
}
void    mk_media_service::set_rtp_recv_buf_info(uint32_t maxSize,uint32_t maxCount)
{
    m_ulRtpBufSize        = maxSize;
    m_ulRtpBufCount       = maxCount;
}
void    mk_media_service::get_rtp_recv_buf_info(uint32_t& maxSize,uint32_t& maxCount)
{
    maxSize  = m_ulRtpBufSize;
    maxCount = m_ulRtpBufCount;
}
void    mk_media_service::set_media_frame_buffer(uint32_t maxSize,uint32_t maxCount)
{
    m_ulFrameBufSize      = maxSize;
    m_ulFramebufCount     = maxCount;
}
void    mk_media_service::get_media_frame_buffer(uint32_t& maxSize,uint32_t& maxCount)
{
    maxSize               = m_ulFrameBufSize;
    maxCount              = m_ulFramebufCount;
}
int32_t mk_media_service::get_rtp_rtcp_pair(mk_rtsp_rtp_udp_handle*&  pRtpHandle,mk_rtsp_rtcp_udp_handle*&  pRtcpHandle)
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
void    mk_media_service::free_rtp_rtcp_pair(mk_rtsp_rtp_udp_handle* pRtpHandle)
{
    uint32_t index = pRtpHandle->get_index();
    m_RtpRtcpfreeList.push_back(index);
    return;
}
char*   mk_media_service::get_rtp_recv_buf()
{
    if(m_RtpRecvBufList.empty()) {
        return NULL;
    }

    char* buf = m_RtpRecvBufList.front();
    m_RtpRecvBufList.pop_front();
    return buf;
}
void    mk_media_service::free_rtp_recv_buf(char* buf)
{
    if(NULL == buf) {
        return;
    }
    m_RtpRecvBufList.push_back(buf);
    return;
}
char*   mk_media_service::get_frame_buf()
{
    if(m_FrameBufList.empty()) {
        return NULL;
    }

    char* buf = m_FrameBufList.front();
    m_FrameBufList.pop_front();
    return buf;
}
void    mk_media_service::free_frame_buf(char* buf)
{
    if(NULL == buf) {
        return;
    }
    m_FrameBufList.push_back(buf);
    return;
}
int32_t mk_media_service::create_rtp_rtcp_udp_pairs()
{
    AS_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs,start port:[%d],count:[%d].",m_usUdpStartPort,m_ulUdpPairCount);
    
    uint32_t port = m_usUdpStartPort;
    uint32_t pairs = m_ulUdpPairCount/2;
    mk_rtsp_rtp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_rtcp_udp_handle* pRtcpHandle = NULL;
    as_network_addr addr;
    addr.m_lIpAddr = 0;
    addr.m_usPort = 0;
    int32_t nRet = AS_ERROR_CODE_OK;

    m_pUdpRtpArray  = AS_NEW(m_pUdpRtpArray,pairs);
    if(NULL == m_pUdpRtpArray) {
        AS_LOG(AS_LOG_CRITICAL,"create rtp handle array fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_pUdpRtcpArray = AS_NEW(m_pUdpRtcpArray,pairs);
    if(NULL == m_pUdpRtcpArray) {
        AS_LOG(AS_LOG_CRITICAL,"create rtcp handle array fail.");
        return AS_ERROR_CODE_FAIL;
    }

    for(uint32_t i = 0;i < pairs;i++) {
        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL == pRtpHandle) {
            AS_LOG(AS_LOG_CRITICAL,"create rtp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }
        addr.m_usPort = port;
        port++;
        nRet = m_NetWorkArray.regUdpSocket(&addr,pRtpHandle);
        if(AS_ERROR_CODE_OK != nRet) {
            AS_LOG(AS_LOG_ERROR,"register the rtp udp handle fail,port:[%d].",addr.m_usPort);
            return AS_ERROR_CODE_FAIL;
        }
        pRtcpHandle = AS_NEW(pRtcpHandle);
        if(NULL == pRtcpHandle) {
            AS_LOG(AS_LOG_CRITICAL,"create rtcp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }
        addr.m_usPort = port;
        port++;
        nRet = m_NetWorkArray.regUdpSocket(&addr,pRtcpHandle);
        if(AS_ERROR_CODE_OK != nRet) {
            AS_LOG(AS_LOG_ERROR,"register the rtcp udp handle fail,port:[%d].",addr.m_usPort);
            return AS_ERROR_CODE_FAIL;
        }
        m_pUdpRtpArray[i]  = pRtpHandle;
        m_pUdpRtcpArray[i] = pRtcpHandle;
        m_RtpRtcpfreeList.push_back(i);
    }
    AS_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs success.");
    return AS_ERROR_CODE_OK;
}
void    mk_media_service::destory_rtp_rtcp_udp_pairs()
{
    AS_LOG(AS_LOG_INFO,"m_pUdpRtcpArray udp rtp and rtcp pair.");

    mk_rtsp_rtp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_rtcp_udp_handle* pRtcpHandle = NULL;

    for(uint32_t i = 0;i < m_ulUdpPairCount;i++) {
        pRtpHandle  = m_pUdpRtpArray[i];
        pRtcpHandle = m_pUdpRtcpArray[i];

        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL != pRtpHandle) {
            m_NetWorkArray.removeUdpSocket(pRtpHandle);
            AS_DELETE(pRtpHandle);
            m_pUdpRtpArray[i] = NULL;
        }
        pRtcpHandle = AS_NEW(pRtcpHandle);
        if(NULL != pRtpHandle) {
            m_NetWorkArray.removeUdpSocket(pRtcpHandle);
            AS_DELETE(pRtcpHandle);
             m_pUdpRtcpArray[i] = NULL;
        }
    }
    AS_LOG(AS_LOG_INFO,"destory udp rtp and rtcp pairs success.");
    return;
}

int32_t mk_media_service::create_rtp_recv_bufs()
{
    AS_LOG(AS_LOG_INFO,"create rtp recv buf begin.");
    if((0 == m_ulRtpBufSize)||(0 == m_ulRtpBufCount)) {
        AS_LOG(AS_LOG_ERROR,"there is no init rtp recv buf arguments,size:[%d] count:[%d].",m_ulRtpBufSize,m_ulRtpBufCount);
        return AS_ERROR_CODE_FAIL;
    }
    char* pBuf = NULL;
    for(uint32_t i = 0;i < m_ulRtpBufCount;i++) {
        pBuf = AS_NEW(pBuf,m_ulRtpBufSize);
        if(NULL == pBuf) {
            AS_LOG(AS_LOG_ERROR,"create the rtp recv buf fail,size:[%d] index:[%d].",m_ulRtpBufSize,i);
            return AS_ERROR_CODE_FAIL;
        }
        m_RtpRecvBufList.push_back(pBuf);
    }
    AS_LOG(AS_LOG_INFO,"create rtp recv buf end.");
    return AS_ERROR_CODE_OK;
}
void    mk_media_service::destory_rtp_recv_bufs()
{
    AS_LOG(AS_LOG_INFO,"destory rtp recv buf begin.");
    char* pBuf = NULL;
    while(0 <m_RtpRecvBufList.size()) {
        pBuf = m_RtpRecvBufList.front();
        m_RtpRecvBufList.pop_front();
        if(NULL == pBuf) {
            continue;
        }
        AS_DELETE(pBuf,MULTI);
    }
    AS_LOG(AS_LOG_INFO,"destory rtp recv buf end.");
    return;
}
int32_t mk_media_service::create_frame_recv_bufs()
{
    AS_LOG(AS_LOG_INFO,"create frame recv buf begin.");
    if((0 == m_ulFrameBufSize)||(0 == m_ulFramebufCount)) {
        AS_LOG(AS_LOG_ERROR,"there is no init frame recv buf arguments,size:[%d] count:[%d].",m_ulRtpBufSize,m_ulRtpBufCount);
        return AS_ERROR_CODE_FAIL;
    }
    char* pBuf = NULL;
    for(uint32_t i = 0;i < m_ulFramebufCount;i++) {
        pBuf = AS_NEW(pBuf,m_ulFrameBufSize);
        if(NULL == pBuf) {
            AS_LOG(AS_LOG_ERROR,"create the frame recv buf fail,size:[%d] index:[%d].",m_ulRtpBufSize,i);
            return AS_ERROR_CODE_FAIL;
        }
        m_FrameBufList.push_back(pBuf);
    }
    AS_LOG(AS_LOG_INFO,"create frame recv buf end.");
    return AS_ERROR_CODE_OK;
}
void    mk_media_service::destory_frame_recv_bufs()
{
    AS_LOG(AS_LOG_INFO,"destory frame recv buf begin.");
    char* pBuf = NULL;
    while(0 <m_FrameBufList.size()) {
        pBuf = m_FrameBufList.front();
        m_FrameBufList.pop_front();
        if(NULL == pBuf) {
            continue;
        }
        AS_DELETE(pBuf,MULTI);
    }
    AS_LOG(AS_LOG_INFO,"destory frame recv buf end.");
    return;
}