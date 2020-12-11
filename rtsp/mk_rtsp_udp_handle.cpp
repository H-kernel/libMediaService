#include "mk_rtsp_udp_handle.h"
#include <string>
#include "mk_rtsp_service.h"
#include "mk_media_common.h"
#include "mk_media_service.h"

mk_rtsp_udp_handle::mk_rtsp_udp_handle()
{
    m_enType      = MK_RTSP_UDP_TYPE_MAX;
    m_ulIdx       = 0;
    m_usPort      = 0;
    m_RtpObserver = NULL;
    m_bRunning    = false;
}

mk_rtsp_udp_handle::~mk_rtsp_udp_handle()
{
}
void mk_rtsp_udp_handle::init(uint32_t idx,uint16_t port)
{
    
    m_ulIdx       = idx;
    m_usPort      = port;
}

uint32_t mk_rtsp_udp_handle::get_index()
{
    return m_ulIdx;
}
uint16_t mk_rtsp_udp_handle::get_port()
{
    return m_usPort;
}
int32_t mk_rtsp_udp_handle::start_handle(MK_RTSP_HANDLE_TYPE type, mk_rtsp_rtp_udp_observer* pObserver)
{
    m_enType      = type;
    m_RtpObserver = pObserver;
    as_network_addr local;
    local.m_ulIpAddr = 0;
    local.m_usPort   = m_usPort;
    as_network_svr* pNetWork =  mk_media_service::instance().get_client_network_svr(m_ulIdx);
    int nRet = pNetWork->regUdpSocket(&local,this);
    if(AS_ERROR_CODE_OK != nRet) {
        return AS_ERROR_CODE_FAIL;
    }
    setHandleRecv(AS_TRUE);
    return AS_ERROR_CODE_OK;
}
void mk_rtsp_udp_handle::stop_handle()
{
    if(!m_bRunning) {
        return;
    }
    m_bRunning    = false;
    as_network_svr* pNetWork =  mk_media_service::instance().get_client_network_svr(m_ulIdx);
    pNetWork->removeUdpSocket(this);
    
}
void mk_rtsp_udp_handle::handle_recv(void)
{
    if(false == m_bRunning) {
        recv_dummy_data();
        return;
    }
    if(NULL == m_RtpObserver) {
        recv_dummy_data();
        return;
    }

    as_network_addr peer;

    char* buf = mk_rtsp_service::instance().get_rtp_recv_buf();
    if(NULL == buf) {
        /* there is no free buffer ,recv and drop it */
        recv_dummy_data();
        return;
    }
    int32_t lens = recv(buf,&peer,RTP_RECV_BUF_SIZE,enAsyncOp);
    if(0 >= lens) {
        mk_rtsp_service::instance().free_rtp_recv_buf(buf);
        setHandleRecv(AS_TRUE);
        return;
    }
    int32_t nRet = AS_ERROR_CODE_OK;
    if((MK_RTSP_UDP_VIDEO_RTP_HANDLE == m_enType)||(MK_RTSP_UDP_AUDIO_RTP_HANDLE == m_enType)) {
        nRet =m_RtpObserver->handle_rtp_packet(m_enType,buf,lens);
    }
    else if((MK_RTSP_UDP_VIDEO_RTCP_HANDLE == m_enType)||(MK_RTSP_UDP_AUDIO_RTCP_HANDLE == m_enType)) {
        nRet =m_RtpObserver->handle_rtcp_packet(m_enType,buf,lens);
    }
    else {
        mk_rtsp_service::instance().free_rtp_recv_buf(buf);
    }

    if(AS_ERROR_CODE_OK != nRet) {
        mk_rtsp_service::instance().free_rtp_recv_buf(buf);
    }
    setHandleRecv(AS_TRUE);
}
void mk_rtsp_udp_handle::handle_send(void)
{
    setHandleSend(AS_FALSE);
}
void mk_rtsp_udp_handle::recv_dummy_data()
{
    char buf[MK_RTSP_UDP_DUMMY_SIZE];
    as_network_addr peer;
    (void)recv(&buf[0],&peer,MK_RTSP_UDP_DUMMY_SIZE,enAsyncOp);

}