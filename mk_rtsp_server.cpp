/*
 * StreamRtspService.cpp
 *
 *  Created on: 2016-5-12
 *      Author:
 */
#include <string.h>
#include "ms_engine_svs_retcode.h"
#include <vms.h>
#include "svs_rtsp_server.h"
#include "ms_engine_config.h"
#include "svs_rtsp_protocol.h"
#include "svs_vms_media_setup_resp.h"
#include "svs_vms_media_play_resp.h"



mk_rtsp_server::mk_rtsp_server()
{
    m_cb  = NULL;
    m_ctx = NULL;
}

mk_rtsp_server::~mk_rtsp_server()
{
}
void mk_rtsp_server::set_callback(rtsp_server_request cb,void* ctx)
{
    m_cb = cb;
    m_ctx = ctx;
}
long mk_rtsp_server::handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle)
{
    
}
