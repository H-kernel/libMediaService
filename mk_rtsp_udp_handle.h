#ifndef __MK_RTSP_RTP_UDP_HANDLE_H__
#define __MK_RTSP_RTP_UDP_HANDLE_H__

#include <list>
#include "as.h"

class mk_rtsp_rtp_udp_observer
{
public:
    mk_rtsp_rtp_udp_observer();
    virtual ~mk_rtsp_rtp_udp_observer();
    virtual int32_t handle_rtp_packet(char* pData,uint32_t len) = 0;
    virtual int32_t handle_rtcp_packet(char* pData,uint32_t len) = 0;
}

enum MK_RTSP_UDP_HANDLE
{
    MK_RTSP_UDP_HANDLE_RTP   = 0,
    MK_RTSP_UDP_HANDLE_RTCP  = 1,
    MK_RTSP_UDP_HANDLE_MAX
};

#define MK_RTSP_UDP_DUMMY_SIZE 1500

class mk_rtsp_udp_handle: public as_udp_sock_handle
{
public:
    mk_rtsp_udp_handle(MK_RTSP_UDP_HANDLE type,uint32_t idx,uint16_t port);
    virtual ~mk_rtsp_udp_handle();
public:
    uint32_t get_index();
    uint16_t get_port();
    void     open(mk_rtsp_rtp_udp_observer* pObserver);
    void     close();
public:
    /* override as_udp_sock_handle */
    virtual void handle_recv(void);
    virtual void handle_send(void);
private:
    void recv_dummy_data();
protected:
    MK_RTSP_UDP_HANDLE        m_enType;
    mk_rtsp_rtp_udp_observer* m_RtpObserver;
    uint32_t                  m_ulIdx;
    uint16_t                  m_usPort;
    volatile bool             m_bRunning;      
};

class mk_rtsp_rtp_udp_handle: public mk_rtsp_udp_handle
{
public:
    mk_rtsp_rtp_udp_handle(uint32_t idx,uint16_t port);
    virtual ~mk_rtsp_rtp_udp_handle();
};

class mk_rtsp_rtcp_udp_handle: public mk_rtsp_udp_handle
{
public:
    mk_rtsp_rtcp_udp_handle(uint32_t idx,uint16_t port);
    virtual ~mk_rtsp_rtcp_udp_handle();
};
#endif /* __MK_RTSP_RTP_UDP_HANDLE_H__ */


