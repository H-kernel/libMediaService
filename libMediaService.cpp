#include "libMediaService.h"
#include "mk_media_service.h"
#include "mk_client_connection.h"

mk_log g_log = NULL;
/* init the media rtsp libary */
MR_API int32_t   mk_lib_init(uint32_t EvnCount,mk_log log,uint32_t MaxClient,uint32_t RtpBufCountPerClient)
{
    g_log =  log;
    return mk_media_service::instance().init(EvnCount,MaxClient,RtpBufCountPerClient);
}
/* release the media rtsp bibary */
MR_API void      mk_lib_release()
{
    mk_media_service::instance().release();
    return;
}
/* create a media rtsp server handle */
MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx)
{
    return mk_media_service::instance().create_rtsp_server(port,cb,ctx);
}
/* destory the media rtsp server handle */
MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server)
{
    mk_rtsp_server* pServer = (mk_rtsp_server*)server;
    mk_media_service::instance().destory_rtsp_server(pServer);
    return;
}
/* create a media rtsp client handle */
MR_API MR_CLIENT mk_create_client_handle(char* url,MEDIA_CALL_BACK* cb,void* ctx)
{
    return mk_media_service::instance().create_client(url,cb,ctx);
}
/* destory the media client handle */
MR_API void      mk_destory_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    mk_media_service::instance().destory_client(pClient);
    return;
}
/* start the media client handle */
MR_API int32_t   mk_start_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    return pClient->start_recv();
}
/* stop the media client handle */
MR_API void   mk_stop_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->stop_recv();
    return;
}
/* set a media client callback */
MR_API void      mk_set_client_callback(MR_CLIENT client,MEDIA_CALL_BACK* cb,void* ctx)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->set_status_callback(cb,ctx);
    return;
}
/* recv media data from media client */
MR_API int32_t   mk_recv_next_media_data(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    return pClient->do_next_recv();
}
/* set a media rtsp client media transport tcp */
MR_API void      mk_create_rtsp_client_set_tcp(MR_CLIENT client)
{
    mk_rtsp_connection* pClient = (mk_rtsp_connection*)client;
    pClient->set_rtp_over_tcp();
    return;
}
/* set a media rtsp client/server rtp/rtcp udp port */
MR_API void      mk_set_rtsp_udp_ports(uint16_t udpstart,uint32_t count)
{
    return;
}
/* get a media rtsp client/server rtp packet stat info */
MR_API void      mk_get_client_rtp_stat_info(MR_CLIENT client,RTP_PACKET_STAT_INFO &statinfo)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->get_client_rtp_stat_info(statinfo);
    return;
}
/* get a media rtsp client sdp info */
MR_API void      mk_get_client_rtsp_sdp_info(MR_CLIENT client,char* sdpInfo)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->get_client_rtsp_sdp_info(sdpInfo);
    return;
}