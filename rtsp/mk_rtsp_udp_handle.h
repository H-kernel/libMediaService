#ifndef __MK_RTSP_UDP_HANDLE_INCLUDE_H__
#define __MK_RTSP_UDP_HANDLE_INCLUDE_H__

#include <list>
#include "as.h"

#define MK_RTSP_UDP_DUMMY_SIZE 1500

typedef enum _enMK_RTSP_UDP_HANDLE_TYPE
{
    MK_RTSP_UDP_VIDEO_RTP_HANDLE    = 0,
    MK_RTSP_UDP_VIDEO_RTCP_HANDLE   = 1,    
    MK_RTSP_UDP_AUDIO_RTP_HANDLE    = 2,
    MK_RTSP_UDP_AUDIO_RTCP_HANDLE   = 3,
    MK_RTSP_UDP_TYPE_MAX
}MK_RTSP_HANDLE_TYPE;

class mk_rtsp_rtp_udp_observer
{
public:
    mk_rtsp_rtp_udp_observer(){};
    virtual ~mk_rtsp_rtp_udp_observer(){};
    virtual int32_t handle_rtp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len) = 0;
    virtual int32_t handle_rtcp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len) = 0;
};



class mk_rtsp_udp_handle : public as_udp_sock_handle
{
public:
    mk_rtsp_udp_handle();
    virtual ~mk_rtsp_udp_handle();
public:
    void     init(uint32_t idx,uint16_t port);
    uint32_t get_index();
    uint16_t get_port();
	int32_t  start_handle(MK_RTSP_HANDLE_TYPE type, mk_rtsp_rtp_udp_observer* pObserver);
    void     stop_handle();
public:
    /* override as_udp_sock_handle */
    virtual void handle_recv(void);
    virtual void handle_send(void);
private:
    void recv_dummy_data();
protected:
    MK_RTSP_HANDLE_TYPE       m_enType;
    mk_rtsp_rtp_udp_observer* m_RtpObserver;
    uint32_t                  m_ulIdx;
    uint16_t                  m_usPort;
    volatile bool             m_bRunning;      
};

#endif /* __MK_RTSP_UDP_HANDLE_INCLUDE_H__ */


