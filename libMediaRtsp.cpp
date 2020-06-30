#include "libMediaRtsp.h"
#include "mk_rtsp_service.h"

/* init the media rtsp libary */
MR_API int32_t   mk_lib_rtsp_init(uint16_t udpstart,uint32_t count)
{
    return mk_rtsp_service::instance().init(udpstart,count);
}
/* release the media rtsp bibary */
MR_API void      mk_lib_rtsp_release()
{
    mk_rtsp_service::instance().release();
    return;
}
/* create a media rtsp server handle */
MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx)
{
    return mk_rtsp_service::instance().create_rtsp_server(port,cb,ctx);
}
/* destory the media rtsp server handle */
MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server)
{
    mk_rtsp_server* pServer = (mk_rtsp_server*)server;
    mk_rtsp_service::instance().destory_rtsp_server(pServer);
    return;
}
/* create a media rtsp client handle */
MR_API MR_CLIENT mk_create_rtsp_client_handle(char* url,rtsp_client_status cb,void* ctx)
{
    return mk_rtsp_service::instance().create_rtsp_client(url,cb,ctx);
}
/* destory the media rtsp client handle */
MR_API void      mk_destory_rtsp_client_handle(MR_CLIENT client)
{
    mk_rtsp_client* pClient = (mk_rtsp_client*)client;
    mk_rtsp_service::instance().destory_rtsp_client(pClient);
    return;
}
/* set a media rtsp client callback */
MR_API void      mk_create_rtsp_client_callback(MR_CLIENT client,rtsp_client_status cb,void* ctx)
{
    mk_rtsp_client* pClient = (mk_rtsp_client*)client;
    pClient->set_status_callback(cb,ctx);
    return;
}
/* set a media rtsp client media transport tcp */
MR_API void      mk_create_rtsp_client_set_tcp(MR_CLIENT client)
{
    mk_rtsp_client* pClient = (mk_rtsp_client*)client;
    pClient->set_rtp_over_tcp();
    return;
}
/* do the media data recvice */
MR_API int32_t   mk_do_recv_media_data(MR_CLIENT client, char* buf,uint32_t len, rtsp_client_media cb,void* ctx)
{
    mk_rtsp_client* pClient = (mk_rtsp_client*)client;
    return pClient->do_recv_media_data(buf,len,cb,ctx);
}