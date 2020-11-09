/*
 * StreamRtspService.h
 *
 *  Created on: 2016-5-12
 *      Author:
 */

#ifndef _MK_RTSP_SERVICE_INCLUDE_H__
#define _MK_RTSP_SERVICE_INCLUDE_H__

#include <map>
#include <list>
#include "as.h"
#include "mk_rtsp_udp_handle.h"

#define RTP_RECV_BUF_SIZE       1500
#define RTP_RECV_BUF_COUNT      (100*RTSP_CONNECTION_DEFAULT)

#define RTP_RTCP_START_PORT     10000
#define RTP_RTCP_PORT_COUNT     (4*RTSP_CONNECTION_DEFAULT)


class mk_rtsp_service
{
public:
    static mk_rtsp_service& instance()
    {
        static mk_rtsp_service obj_mk_rtsp_service;
        return obj_mk_rtsp_service;
    }
    virtual ~mk_rtsp_service();  
    int32_t init();
    void    release();
public:
    void    set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count);
    void    get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count);
    void    set_rtp_recv_buf_info(uint32_t maxCount);
    void    get_rtp_recv_buf_info(uint32_t& maxCount);
public:
    int32_t get_rtp_rtcp_pair(mk_rtsp_udp_handle*&  pRtpHandle,mk_rtsp_udp_handle*&  pRtcpHandle);
    void    free_rtp_rtcp_pair(mk_rtsp_udp_handle* pRtpHandle);
    char*   get_rtp_recv_buf();
    void    free_rtp_recv_buf(char* buf);
private:
    mk_rtsp_service();
    int32_t create_rtp_rtcp_udp_pairs();
    void    destory_rtp_rtcp_udp_pairs();
    int32_t create_rtp_recv_bufs();
    void    destory_rtp_recv_bufs();
private:
    as_mutex_t*               m_pMutex;
    uint16_t                  m_usUdpStartPort;
    uint32_t                  m_ulUdpPairCount;
    mk_rtsp_udp_handle**      m_pUdpRtpArray;
    mk_rtsp_udp_handle**      m_pUdpRtcpArray;

    typedef std::list<uint32_t> RTP_RTCP_UDP_PAIR_LIST;
    RTP_RTCP_UDP_PAIR_LIST    m_RtpRtcpfreeList;

    typedef std::list<char*>  RECV_BUF_LIST;
    RECV_BUF_LIST             m_RtpRecvBufList;

    uint32_t                  m_ulRtpBufCount;
};

#endif /* _MK_RTSP_SERVICE_INCLUDE_H__ */
