/*
 * StreamRtspService.cpp
 *
 *  Created on: 2016-5-12
 *      Author:
 */
#include <string.h>
#include "ms_engine_svs_retcode.h"
#include <vms.h>
#include "svs_rtsp_service.h"
#include "ms_engine_config.h"
#include "svs_rtsp_protocol.h"
#include "svs_vms_media_setup_resp.h"
#include "svs_vms_media_play_resp.h"



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
    ACE_Reactor *pReactor = ACE_Reactor::instance();
    if (!pReactor)
    {
        SVS_LOG(SVS_LOG_WARNING,"close http live stream server fail, can't find reactor instance.");
        return;
    }
    pReactor->cancel_timer(m_ulCheckTimerId);
    ACE_OS::shutdown(m_RtspAcceptor.get_handle(), SHUT_RDWR);

    SVS_LOG(SVS_LOG_INFO,"success to close rtsp server.");
    return;
}
mk_rtsp_server* mk_rtsp_service::create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx)
{
    return NULL;
}
void mk_rtsp_service::destory_rtsp_server(mk_rtsp_server* pServer)
{
    return;
}
mk_rtsp_client* mk_rtsp_service::create_rtsp_client(char* url,rtsp_client_status cb,void* ctx)
{
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
