/*
 * StreamRtspService.h
 *
 *  Created on: 2016-5-12
 *      Author:
 */

#ifndef __MK_RTSP_SERVER_INCLUDE_H__
#define __MK_RTSP_SERVER_INCLUDE_H__

#include <map>
#include "svs_ace_header.h"
#include "svs_rtsp_push_session.h"
#include "svs_vms_message.h"


class CRtspSessionCheckTimer : public ACE_Event_Handler
{
public:
    virtual int32_t handle_timeout(const ACE_Time_Value &tv, const void *arg);
};


typedef std::map<uint32_t, mk_rtsp_client*>  RTSP_SESSION_MAP;
typedef RTSP_SESSION_MAP::iterator                    RTSP_SESSION_MAP_ITER;

class mk_rtsp_server : public ACE_Event_Handler
{
public:
    mk_rtsp_server();
    virtual ~mk_rtsp_server();

    static mk_rtsp_server& instance()
    {
        static mk_rtsp_server streamRtspService;
        return streamRtspService;
    }

    int32_t open();

    void close() const;

    int32_t handleSvsMessage(CStreamSvsMessage &message);

    int32_t handle_input(ACE_HANDLE handle);

    int32_t handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

    void checkSession();

    void removeSession(mk_rtsp_client* pSession);

private:
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

#endif /* __MK_RTSP_SERVER_INCLUDE_H__ */
