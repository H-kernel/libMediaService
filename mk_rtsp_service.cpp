#include <string.h>
#include "svs_rtsp_service.h"




mk_rtsp_service::mk_rtsp_service()
{
    m_ulUdpPairCount      = 0;
    m_pUdpRtpArray        = NULL;
    m_pUdpRtcpArray       = NULL;
}

mk_rtsp_service::~mk_rtsp_service()
{
}

int32_t mk_rtsp_service::init(uint16_t udpPort,uint32_t count)
{
    int32_t nRet = AS_ERROR_CODE_OK;
    m_ConnMgr.setLogWriter(&m_connLog);
    nRet = m_ConnMgr.init(DEFAULT_SELECT_PERIOD,AS_TRUE,AS_TRUE,AS_TRUE);
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"init the network module fail.");
        return AS_ERROR_CODE_FAIL;
    }

    nRet = m_ConnMgr.run();
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"run the network module fail.");
        return AS_ERROR_CODE_FAIL;
    }

    SVS_LOG(SVS_LOG_INFO,"init the network module success.")
    
    if(0 == count) {
        SVS_LOG(SVS_LOG_INFO,"the udp port is zore,no need create udp ports.")
        return AS_ERROR_CODE_OK;
    }
    return create_rtp_rtcp_udp_pairs(udpPort,count);
}

void mk_rtsp_service::release()
{
    destroy_rtp_rtcp_udp_pairs();
    m_ConnMgr.exit();
    return;
}
mk_rtsp_server* mk_rtsp_service::create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx)
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
    int32_t nRet = m_ConnMgr.regTcpServer(&local,pRtspServer);
    if(AS_ERROR_CODE_OK != nRet) {
        AS_DELETE(pRtspServer);
        pRtspServer = NULL;
    }
    return pRtspServer;
}
void mk_rtsp_service::destory_rtsp_server(mk_rtsp_server* pServer)
{
    if(NULL == pServer) {
        return;
    }
    m_ConnMgr.removeTcpServer(pServer);
    AS_DELETE(pServer);
    return;
}
mk_rtsp_client* mk_rtsp_service::create_rtsp_client(char* url,rtsp_client_status cb,void* ctx)
{
    mk_rtsp_client* pClient = NULL
    return NULL;
}
void mk_rtsp_service::destory_rtsp_client(mk_rtsp_client* pClient)
{
    return;
}
int32_t mk_rtsp_service::create_rtp_rtcp_udp_pairs(uint16_t udpPort,uint32_t count)
{
    AS_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs,start port:[%d],count:[%d].",udpPort,count);
    
    uint32_t port = udpPort;
    uint32_t pairs = count/2;
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

    m_ulUdpPairCount = pairs;

    for(uint32_t i = 0;i < pairs;i++) {
        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL == pRtpHandle) {
            AS_LOG(AS_LOG_CRITICAL,"create rtp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }
        addr.m_usPort = port;
        port++;
        nRet = m_ConnMgr.regUdpSocket(&addr,pRtpHandle);
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
        nRet = m_ConnMgr.regUdpSocket(&addr,pRtcpHandle);
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
void    mk_rtsp_service::destroy_rtp_rtcp_udp_pairs()
{
    AS_LOG(AS_LOG_INFO,"m_pUdpRtcpArray udp rtp and rtcp pair.");

    mk_rtsp_rtp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_rtcp_udp_handle* pRtcpHandle = NULL;

    for(uint32_t i = 0;i < m_ulUdpPairCount;i++) {
        pRtpHandle  = m_pUdpRtpArray[i];
        pRtcpHandle = m_pUdpRtcpArray[i];

        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL != pRtpHandle) {
            m_ConnMgr.removeUdpSocket(pRtpHandle);
            AS_DELETE(pRtpHandle);
            m_pUdpRtpArray[i] = NULL;
        }
        pRtcpHandle = AS_NEW(pRtcpHandle);
        if(NULL != pRtpHandle) {
            m_ConnMgr.removeUdpSocket(pRtcpHandle);
            AS_DELETE(pRtcpHandle);
             m_pUdpRtcpArray[i] = NULL;
        }
    }
    AS_LOG(AS_LOG_INFO,"destory udp rtp and rtcp pairs success.");
    return;
}