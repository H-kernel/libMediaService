#ifndef __MK_SIP_CONNECTION_INCLUDE__
#define __MK_SIP_CONNECTION_INCLUDE__
#include "as_network_svr.h"
#include "mk_client_connection.h"
#include "as.h"
class mk_sip_handle
{
public:
    mk_sip_handle();
    virtual ~mk_sip_handle();
};
class mk_sip_udp_handle:public as_udp_sock_handle
{
public:
    mk_sip_udp_handle();
    virtual ~mk_sip_udp_handle();
};

class mk_sip_tcp_handle:public as_tcp_conn_handle
{
    mk_sip_tcp_handle();
    virtual ~mk_sip_tcp_handle();
};

class mk_sip_client_connection:public mk_client_connection
{
public:
    mk_sip_client_connection();
    virtual ~mk_sip_client_connection();
public:
    /* override */
    virtual int32_t start();
    virtual void    stop();
    virtual int32_t recv_next();
    virtual int32_t check_client();
    virtual void    get_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo);
    virtual void    get_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen);
};

#endif /*__MK_SIP_CONNECTION_INCLUDE__*/