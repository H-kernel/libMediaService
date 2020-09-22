#ifndef __MK_CLIENT_CONNECTION_INCLUDE_H__
#define __MK_CLIENT_CONNECTION_INCLUDE_H__

#include "as.h"
#include "libMediaService.h"

enum MK_CLIENT_TYPE
{
    MK_CLIENT_TYPE_RTSP = 0,
    MK_CLIENT_TYPE_RTMP = 1,
    MK_CLIENT_TYPE_MAX
};

#define MK_CLIENT_TIMER_INTERVAL 100
#define MK_CLIENT_RECV_TIMEOUT   5

class mk_client_timer:public ITrigger
{
public:
    mk_client_timer();
    virtual ~mk_client_timer();
    virtual void onTrigger(void *pArg, ULONGLONG ullScales, TriggerStyle enStyle);
};

class mk_client_connection
{
public:
    mk_client_connection();
    virtual ~mk_client_connection();
    MK_CLIENT_TYPE client_type();
    MR_CLIENT_STATUS get_status();
    void     set_url(const char* pszUrl);
    void     set_status_callback(handle_client_status cb,void* ctx);
    int32_t  start_recv(char* buf,uint32_t len,handle_client_media cb,void* data);
    int32_t  stop_recv();
    int32_t  do_next_recv(char* buf,uint32_t len);
    void     set_index(uint32_t ulIdx);
    uint32_t get_index();
public:
    virtual int32_t start() = 0;
    virtual void    stop() = 0;
    virtual int32_t recv_next() = 0;
    virtual void    check_client() = 0;
protected:
    void    handle_connection_media(MR_MEDIA_TYPE enType,uint32_t pts);
    void    handle_connection_status(MR_CLIENT_STATUS enStatus);
protected:
    char*                m_recvBuf;
    uint32_t             m_ulRecvBufLen;
    uint32_t             m_ulRecvLen;
    std::string          m_strurl;
private:
    uint32_t             m_ulIndex;
    MK_CLIENT_TYPE       m_enType;
    MR_CLIENT_STATUS     m_enStatus;
    handle_client_media  m_MediaCallBack;
    void*                m_pMediaCbData;
    handle_client_status m_StatusCallBack;
    void*                m_pStatusCbData;

    mk_client_timer      m_ClientCheckTimer;
};

#endif /* __MK_CLIENT_CONNECTION_INCLUDE_H__ */
