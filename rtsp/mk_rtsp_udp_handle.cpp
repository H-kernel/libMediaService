#include "mk_rtsp_udp_handle.h"
#include <string>

mk_rtsp_udp_handle::mk_rtsp_udp_handle(MK_RTSP_UDP_HANDLE type,uint32_t idx,uint16_t port)
{
    m_enType      = type;
    m_ulIdx       = idx;
    m_usPort      = port;
    m_RtpObserver = NULL;
    m_bRunning    = false;
}

mk_rtsp_udp_handle::~mk_rtsp_udp_handle()
{
}

uint32_t mk_rtsp_udp_handle::get_index()
{
    return m_ulIdx;
}
uint16_t mk_rtsp_udp_handle::get_port()
{
    return m_usPort;
}
void mk_rtsp_udp_handle::open(mk_rtsp_rtp_udp_observer* pObserver)
{
    m_RtpObserver = pObserver;
    setHandleRecv(AS_TRUE);
}
void mk_rtsp_udp_handle::close()
{
    m_bRunning    = false;
}
void mk_rtsp_udp_handle::handle_recv(void)
{
    if(false == m_bRunning) {
        recv_dummy_data();
        return;
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

mk_rtsp_rtp_udp_handle::mk_rtsp_rtp_udp_handle(uint32_t idx,uint16_t port)
    :mk_rtsp_udp_handle(MK_RTSP_UDP_HANDLE_RTP,idx,port)
{
}

mk_rtsp_rtp_udp_handle::~mk_rtsp_rtp_udp_handle()
{
}

mk_rtsp_rtcp_udp_handle::mk_rtsp_rtcp_udp_handle(uint32_t idx,uint16_t port)
    :mk_rtsp_udp_handle(MK_RTSP_UDP_HANDLE_RTCP,idx,port)
{
}

mk_rtsp_rtcp_udp_handle::~mk_rtsp_rtcp_udp_handle()
{
}
