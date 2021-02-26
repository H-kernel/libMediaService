#ifndef __MK_RTMP_CONNECTION_INCLUDE_H__
#define __MK_RTMP_CONNECTION_INCLUDE_H__
#include "as_network_svr.h"
#include "mk_librtmp.h"
#include "mk_client_connection.h"


class mk_rtmp_connection:public mk_client_connection,as_tcp_monitor_handle
{
public:
    mk_rtmp_connection();
    virtual ~mk_rtmp_connection();
    virtual int32_t start();
    virtual void    stop();
    virtual int32_t recv_next();
    virtual void    check_client();
public:
    virtual void handle_recv(void);
    virtual void handle_send(void);
    virtual void get_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo);
    virtual void get_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen);
private:
    srs_rtmp_t    m_rtmpHandle;
    time_t        m_ulLastRecv;
};

#endif /* __MK_RTMP_CONNECTION_INCLUDE_H__ */