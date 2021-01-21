#include <string.h>
#include "mk_media_service.h"
#include "mk_rtsp_service.h"
#include "mk_rtsp_connection.h"
#include "mk_rtmp_connection.h"
#include "mk_media_common.h"

mk_media_service::mk_media_service()
{
    m_NetWorkArray        = NULL;
    m_ulEvnCount          = 0;
    m_ulFrameBufSize      = FRAME_RECV_BUF_SIZE;
    m_ulFramebufCount     = FRAME_RECV_BUF_COUNT;
}

mk_media_service::~mk_media_service()
{
}

int32_t mk_media_service::init(uint32_t EvnCount,uint32_t MaxClient,uint32_t RtpBufCountPerClient)
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
            MK_LOG(AS_LOG_WARNING,"init the network module fail.");
            return AS_ERROR_CODE_FAIL;
        }

        nRet = pNetWork->run();
        if (AS_ERROR_CODE_OK != nRet)
        {
            MK_LOG(AS_LOG_WARNING,"run the network module fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_NetWorkArray[i] = pNetWork;        
    }

    for(i = 0;i < MaxClient;i++) {
        m_ClientFreeList.push_back(i);    
    } 

    MK_LOG(AS_LOG_INFO,"init the network module success.");

    // nRet = create_frame_recv_bufs();
    // if (AS_ERROR_CODE_OK != nRet)
    // {
    //     MK_LOG(AS_LOG_WARNING,"create frame recv bufs fail.");
    //     return AS_ERROR_CODE_FAIL;
    // }

    MK_LOG(AS_LOG_INFO,"create frame recv bufs success.");

    if(AS_ERROR_CODE_OK != m_CheckTimer.init(DefaultTimerScale)) {
        MK_LOG(AS_LOG_WARNING,"init the timer fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != m_CheckTimer.run()) {
        MK_LOG(AS_LOG_WARNING,"run the timer fail.");
        return AS_ERROR_CODE_FAIL;
    }

    MK_LOG(AS_LOG_INFO,"run the timer success.");
    
    return mk_rtsp_service::instance().init(MaxClient,RtpBufCountPerClient);
}

void mk_media_service::release()
{
    //destory_frame_recv_bufs();
    as_network_svr* pNetWork = NULL;
    for(uint32_t i = 0;i < m_ulEvnCount;i++) {
        pNetWork = m_NetWorkArray[i];
        if(NULL == pNetWork) {
            continue;
        }
        pNetWork->exit();

        AS_DELETE(pNetWork);
        m_NetWorkArray[i] = NULL;
    }
    mk_rtsp_service::instance().release();
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
    local.m_ulIpAddr = 0;
    local.m_usPort  = port;
    int32_t nRet = m_NetWorkArray[0]->regTcpServer(&local,pRtspServer);
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
    m_NetWorkArray[0]->removeTcpServer(pServer);
    AS_DELETE(pServer);
    return;
}
mk_client_connection* mk_media_service::create_client(char* url,MEDIA_CALL_BACK* cb,void* ctx)
{
    mk_client_connection* pClient   = NULL;
    mk_rtsp_connection* pRtspClient = NULL;
    mk_rtmp_connection* pRtmpClient = NULL;
    as_network_addr  local;
    as_network_addr  peer;

    if(m_ClientFreeList.empty()) {
        return NULL;
    }
    

    if(0 == strncmp(url,RTSP_URL_PREFIX,strlen(RTSP_URL_PREFIX))) {
        pClient = AS_NEW(pRtspClient);
    }
    else if(0 == strncmp(url,RTMP_URL_PREFIX,strlen(RTMP_URL_PREFIX))) {
        pClient = AS_NEW(pRtmpClient);
    }

    if(NULL == pClient) {
        return NULL;
    }

    uint32_t ulIdx = m_ClientFreeList.front();
    m_ClientFreeList.pop_front();

    pClient->set_index(ulIdx);
    pClient->set_status_callback(cb,ctx);
    pClient->set_url(url);
    return pClient;
}
void mk_media_service::destory_client(mk_client_connection* pClient)
{
    if(NULL == pClient) {
        return;
    }
    uint32_t ulIdx = pClient->get_index();    
    AS_DELETE(pClient);
    m_ClientFreeList.push_back(ulIdx);
    return;
}
mk_mov_file_writer* mk_media_service::create_writer(char* path)
{
    mk_mov_file_writer* pWriter = NULL;
    pWriter = AS_NEW(pWriter);
    if(NULL == pWriter) {
        return NULL;
    }
    if (AS_ERROR_CODE_OK != pWriter->open_writer(path))
    {
        AS_DELETE(pWriter);
        pWriter = NULL;
    }

    return pWriter;    
}
void mk_media_service::destory_writer(mk_mov_file_writer* pWriter)
{
    if(NULL == pWriter) {
        return;
    }
    pWriter->close_writer();
    AS_DELETE(pWriter);
    pWriter = NULL;
}
as_network_svr* mk_media_service::get_client_network_svr(uint32_t ulIndex)
{
    uint32_t ulIdx = ulIndex%m_ulEvnCount;
    
    return m_NetWorkArray[ulIdx];
}
as_timer& mk_media_service::get_client_check_timer()
{
    return m_CheckTimer;
}
void    mk_media_service::set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count)
{
    mk_rtsp_service::instance().set_rtp_rtcp_udp_port(udpPort,count);
}
void    mk_media_service::get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count)
{
    mk_rtsp_service::instance().get_rtp_rtcp_udp_port(udpPort,count);
}
void    mk_media_service::set_rtp_recv_buf_info(uint32_t maxCount)
{
    mk_rtsp_service::instance().set_rtp_recv_buf_info(maxCount);
}
void    mk_media_service::get_rtp_recv_buf_info(uint32_t& maxCount)
{
    mk_rtsp_service::instance().get_rtp_recv_buf_info(maxCount);
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

int32_t mk_media_service::create_frame_recv_bufs()
{
    MK_LOG(AS_LOG_INFO,"create frame recv buf begin.");
    if((0 == m_ulFrameBufSize)||(0 == m_ulFramebufCount)) {
        MK_LOG(AS_LOG_ERROR,"there is no init frame recv buf arguments,size:[%d] count:[%d].",m_ulFrameBufSize,m_ulFramebufCount);
        return AS_ERROR_CODE_FAIL;
    }
    char* pBuf = NULL;
    for(uint32_t i = 0;i < m_ulFramebufCount;i++) {
        pBuf = AS_NEW(pBuf,m_ulFrameBufSize);
        if(NULL == pBuf) {
            MK_LOG(AS_LOG_ERROR,"create the frame recv buf fail,size:[%d] index:[%d].",m_ulFrameBufSize,i);
            return AS_ERROR_CODE_FAIL;
        }
        m_FrameBufList.push_back(pBuf);
    }
    MK_LOG(AS_LOG_INFO,"create frame recv buf end.");
    return AS_ERROR_CODE_OK;
}
void    mk_media_service::destory_frame_recv_bufs()
{
    MK_LOG(AS_LOG_INFO,"destory frame recv buf begin.");
    char* pBuf = NULL;
    while(0 <m_FrameBufList.size()) {
        pBuf = m_FrameBufList.front();
        m_FrameBufList.pop_front();
        if(NULL == pBuf) {
            continue;
        }
        AS_DELETE(pBuf,MULTI);
    }
    MK_LOG(AS_LOG_INFO,"destory frame recv buf end.");
    return;
}