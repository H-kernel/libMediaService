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

class mk_client_connection
{
public:
    mk_client_connection();
    virtual ~mk_client_connection();
    MK_CLIENT_TYPE client_type();
    MR_CLIENT_STATUS get_status();
    void     set_status_callback(handle_client_status cb,void* ctx);
    int32_t  do_next_recv(char* buf,uint32_t len,handle_client_media cb,void* data);
public:
    virtual int32_t start(const char* pszUrl) = 0;
    virtual void    stop() = 0;
    virtual int32_t recv_next() = 0;
protected:
    void    handle_connection_status();
    void    handle_connection_media(MR_CLIENT_STATUS enStatus);
protected:
    char*                m_recvBuf;
    uint32_t             m_ulRecvBufLen;
    uint32_t             m_ulRecvLen;
private:
    MK_CLIENT_TYPE       m_enType;
    MR_CLIENT_STATUS     m_enStatus;
    handle_client_media  m_MediaCallBack;
    void*                m_pMediaCbData;
    handle_client_status m_StatusCallBack;
    void*                m_pStatusCbData;
};

#endif /* __MK_CLIENT_CONNECTION_INCLUDE_H__ */
