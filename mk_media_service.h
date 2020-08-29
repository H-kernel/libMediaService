/*
 * StreamRtspService.h
 *
 *  Created on: 2016-5-12
 *      Author:
 */

#ifndef _MK_MEDIA_SERVICE_INCLUDE_H__
#define _MK_MEDIA_SERVICE_INCLUDE_H__

#include <map>
#include <list>
#include "as.h"
#include "mk_rtsp_connection.h"
#include "mk_rtsp_udp_handle.h"
#include "mk_rtmp_connection.h"

#define RTP_RECV_BUF_SIZE       1500
#define RTP_RECV_BUF_COUNT      (100*RTSP_CONNECTION_DEFAULT)

#define FRAME_RECV_BUF_SIZE     (1024*1024)
#define FRAME_RECV_BUF_COUNT    (2*RTSP_CONNECTION_DEFAULT)

#define RTP_RTCP_START_PORT     10000
#define RTP_RTCP_PORT_COUNT     (4*RTSP_CONNECTION_DEFAULT)

#define RTSP_URL_PREFIX         "rtsp://"
#define RTMP_URL_PREFIX         "rtmp://"


class mk_conn_log:public as_conn_mgr_log
{
public:
    mk_conn_log(){};
    virtual ~mk_conn_log(){};
    virtual void writeLog(int32_t lType, int32_t llevel,const char *szLogDetail, const int32_t lLogLen)
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

class mk_media_service
{
public:
    static mk_media_service& instance()
    {
        static mk_media_service obj_mk_media_service;
        return obj_mk_media_service;
    }
    virtual ~mk_media_service();  
    int32_t init(uint32_t EvnCount,uint32_t MaxClient);
    void    release();
    mk_rtsp_server* create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx);
    void destory_rtsp_server(mk_rtsp_server* pServer);
    mk_client_connection* create_client(char* url,handle_client_status cb,void* ctx);
    void destory_client(mk_client_connection* pClient);
public:
    as_network_svr* get_client_network_svr(mk_client_connection* pClient);
    void    set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count);
    void    get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count);
    void    set_rtp_recv_buf_info(uint32_t maxCount);
    void    get_rtp_recv_buf_info(uint32_t& maxCount);
    void    set_media_frame_buffer(uint32_t maxSize,uint32_t maxCount);
    void    get_media_frame_buffer(uint32_t& maxSize,uint32_t& maxCount);
public:
    char*   get_frame_buf();
    void    free_frame_buf(char* buf);
private:
    mk_media_service();
    int32_t create_frame_recv_bufs();
    void    destory_frame_recv_bufs();
private:
    as_network_svr**          m_NetWorkArray;
    uint32_t                  m_ulEvnCount;
    mk_conn_log               m_connLog;

    uint32_t                  m_ulFrameBufSize;
    uint32_t                  m_ulFramebufCount;

    typedef std::list<char*>  RECV_BUF_LIST;
    RECV_BUF_LIST             m_RtpRecvBufList;
    RECV_BUF_LIST             m_FrameBufList;

    typedef std::list<uint32_t> CLIENT_INDEX_LIST;
    CLIENT_INDEX_LIST         m_ClientFreeList;
};

#endif /* _MK_MEDIA_SERVICE_INCLUDE_H__ */
