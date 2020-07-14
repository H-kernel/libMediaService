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
#include "mk_rtsp_connection.h"
#include "mk_rtsp_server.h"
#include "mk_rtsp_rtp_udp_handle.h"



class mk_conn_log:public as_conn_mgr_log
{
public:
    mk_conn_log(){};
    virtual ~mk_conn_log(){};
    virtual void writeLog(long lType, long llevel,const char *szLogDetail, const long lLogLen)
    {
        uint32_t nLevel = AS_LOG_INFO;

        if(CONN_EMERGENCY == llevel) {
            nLevel = AS_LOG_EMERGENCY;
        }
        else if(CONN_ERROR == llevel) {
            nLevel = AS_LOG_ERROR;
        }
        else if(CONN_WARNING == llevel) {
            nLevel = AS_LOG_WARNING;
        }
        else if(CONN_DEBUG == llevel) {
            nLevel = AS_LOG_DEBUG;
        }

        AS_LOG(nLevel,"[connect]:%s",szLogDetail);
    }
};

class mk_rtsp_service
{
public:
    static mk_rtsp_service& instance()
    {
        static mk_rtsp_service obj_mk_rtsp_service;
        return obj_mk_rtsp_service;
    }
    virtual ~mk_rtsp_service();  
    int32_t init(uint32_t maxConnection);
    void    release();
    mk_rtsp_server* create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx);
    void destory_rtsp_server(mk_rtsp_server* pServer);
    mk_rtsp_connection* create_rtsp_client(char* url,rtsp_client_status cb,void* ctx);
    void destory_rtsp_client(mk_rtsp_connection* pClient);
public:
    void    set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count);
    void    get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count);
    void    set_rtp_recv_buf_info(uint32_t maxSize,uint32_t maxCount);
    void    get_rtp_recv_buf_info(uint32_t& maxSize,uint32_t& maxCount);
    void    set_media_frame_buffer(uint32_t maxSize,uint32_t maxCount);
    void    get_media_frame_buffer(uint32_t& maxSize,uint32_t& maxCount);
public:
    char*   get_rtp_recv_buf();
    void    free_rtp_recv_buf(char* buf);
    char*   get_frame_buf();
    void    free_frame_buf(char* buf);
private:
    mk_rtsp_service();
    int32_t create_rtp_rtcp_udp_pairs();
    void    destory_rtp_rtcp_udp_pairs();
    int32_t create_rtp_recv_bufs();
    void    destory_rtp_recv_bufs();
    int32_t create_frame_recv_bufs();
    void    destory_frame_recv_bufs();
    int32_t create_rtsp_connections(uint32_t count);
    void    destory_rtsp_connections();
private:
    as_conn_mgr               m_ConnMgr;
    mk_conn_log               m_connLog;

    uint16_t                  m_usUdpStartPort;
    uint32_t                  m_ulUdpPairCount;
    mk_rtsp_rtp_udp_handle**  m_pUdpRtpArray;
    mk_rtsp_rtcp_udp_handle** m_pUdpRtcpArray;

    typedef std::list<uint32_t> RTP_RTCP_UDP_PAIR_LIST;
    RTP_RTCP_UDP_PAIR_LIST    m_RtpRtcpfreeList;

    uint32_t                  m_ulRtpBufSize;
    uint32_t                  m_ulRtpBufCount;

    uint32_t                  m_ulFrameBufSize;
    uint32_t                  m_ulFramebufCount;

    typedef std::list<char*>  RECV_BUF_LIST;
    RECV_BUF_LIST             m_RtpRecvBufList;
    RECV_BUF_LIST             m_FrameBufList;

    typedef std::list<mk_rtsp_connection*>  RTSP_CONN_LIST;
    RTSP_CONN_LIST            m_RtspConnect;
};

#endif /* _MK_RTSP_SERVICE_INCLUDE_H__ */
