/*
 * StreamRtspService.h
 *
 *  Created on: 2016-5-12
 *      Author:
 */

#ifndef STREAMRTSPSERVICE_H_
#define STREAMRTSPSERVICE_H_

#include <map>
#include "as.h"
#include "mk_rtsp_client.h"
#include "mk_rtsp_server.h"


class mk_rtsp_service
{
public:
    virtual ~mk_rtsp_service();

    static mk_rtsp_service& instance()
    {
        static mk_rtsp_service obj_mk_rtsp_service;
        return obj_mk_rtsp_service;
    }
    
    int32_t init(uint16_t udpPort,uint32_t count);
    void    release();
    mk_rtsp_server* create_rtsp_server(uint16_t port,rtsp_server_request cb,void* ctx);
    void destory_rtsp_server(mk_rtsp_server* pServer);
    mk_rtsp_client* create_rtsp_client(char* url,rtsp_client_status cb,void* ctx);
    void destory_rtsp_client(mk_rtsp_client* pClient);
private:
    mk_rtsp_service();
    uint32_t getLocalSessionIndex();
private:
    uint32_t               m_unLocalSessionIndex;
    ACE_INET_Addr          m_RtspAddr;
    ACE_SOCK_Acceptor      m_RtspAcceptor;

    RTSP_SESSION_MAP       m_RtspSessionMap;
    ACE_Thread_Mutex       m_MapMutex;

    CRtspSessionCheckTimer m_SessionCheckTimer;
    int32_t                m_ulCheckTimerId;
};

#endif /* STREAMRTSPSERVICE_H_ */
