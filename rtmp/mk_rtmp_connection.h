#ifndef __MK_RTMP_CONNECTION_INCLUDE_H__
#define __MK_RTMP_CONNECTION_INCLUDE_H__
#include "as.h"
#include "mk_librtmp.h"
#include "mk_client_connection.h"


class mk_rtmp_connection:public as_tcp_monitor_handle,mk_client_connection
{
public:
    mk_rtmp_connection();
    virtual ~mk_rtmp_connection();
    virtual int32_t start(const char* pszUrl);
    virtual void    stop();
    virtual int32_t recv_next();
public:
    virtual void handle_recv(void);
    virtual void handle_send(void);
private:
    srs_rtmp_t    m_rtmpHandle;
};

#endif /* __MK_RTMP_CONNECTION_INCLUDE_H__ */