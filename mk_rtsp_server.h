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

class mk_rtsp_server : public as_tcp_server_handle
{
public:
    mk_rtsp_server();
    virtual ~mk_rtsp_server();
    void set_callback(rtsp_server_request cb,void* ctx);
public:
    /* override */
    virtual long handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle);
private:
    rtsp_server_request m_cb;
    void*               m_ctx;
};

#endif /* __MK_RTSP_SERVER_INCLUDE_H__ */
