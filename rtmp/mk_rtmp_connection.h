#ifndef __MK_RTMP_CONNECTION_INCLUDE_H__
#define __MK_RTMP_CONNECTION_INCLUDE_H__
#include "as.h"
#include "mk_librtmp.h"


class mk_rtmp_connection:public as_tcp_monitor_handle
{
public:
    mk_rtmp_connection();
    virtual ~mk_rtmp_connection();
    int32_t open(const char* pszUrl);
    void    close();
private:
    srs_rtmp_t    m_rtmpHandle;
};

#endif /* __MK_RTMP_CONNECTION_INCLUDE_H__ */