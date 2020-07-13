/*
 * StreamRtspService.h
 *
 *  Created on: 2016-5-12
 *      Author:
 */

#ifndef STREAMRTSPSERVICE_H_
#define STREAMRTSPSERVICE_H_

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
    
    int32_t init(uint16_t udpPort,uint32_t count);
    void    release();
    mk_rtsp_server* create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx);
    void destory_rtsp_server(mk_rtsp_server* pServer);
    mk_rtsp_connection* create_rtsp_client(char* url,rtsp_client_status cb,void* ctx);
    void destory_rtsp_client(mk_rtsp_connection* pClient);
private:
    mk_rtsp_service();
    int32_t create_rtp_rtcp_udp_pairs(uint16_t udpPort,uint32_t count);
    void    destroy_rtp_rtcp_udp_pairs();
private:
    as_conn_mgr               m_ConnMgr;
    mk_conn_log               m_connLog;
    uint32_t                  m_ulUdpPairCount;
    mk_rtsp_rtp_udp_handle**  m_pUdpRtpArray;
    mk_rtsp_rtcp_udp_handle** m_pUdpRtcpArray;

    typedef std::list<uint32_t> RTP_RTCP_UDP_PAIR_LIST;
    RTP_RTCP_UDP_PAIR_LIST    m_RtpRtcpfreeList;
};

#endif /* STREAMRTSPSERVICE_H_ */
