#include "libMediaService.h"
#include "mk_media_service.h"
#include "mk_client_connection.h"

/* init the media rtsp libary */
MR_API int32_t   mk_lib_rtsp_init(uint32_t EvnCount)
{
    return mk_media_service::instance().init(udpstart,count);
}
/* release the media rtsp bibary */
MR_API void      mk_lib_rtsp_release()
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
MR_API MR_CLIENT mk_create_client_handle(char* url,handle_client_status cb,void* ctx)
{
    return mk_media_service::instance().create_client(url,cb,ctx);
}
/* destory the media client handle */
MR_API void      mk_destory_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    mk_media_service::instance().destory_rtsp_client(pClient);
    return;
}
/* set a media client callback */
MR_API void      mk_set_client_callback(MR_CLIENT client,handle_client_status cb,void* ctx)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->set_status_callback(cb,ctx);
    return;
}
/* recv media data from media client */
MR_API int32_t   mk_recv_next_media_data(MR_CLIENT client,char* buf,uint32_t len,handle_client_media cb,void* data)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    return pClient->do_next_recv(buf,len,cb,data);
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