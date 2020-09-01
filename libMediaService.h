#ifndef __LIB_MEDIA_RTSP_H__
#define __LIB_MEDIA_RTSP_H__
#ifdef WIN32
#ifdef LIBMEDIARTSP_EXPORTS
#define MR_API __declspec(dllexport)
#else
#define MR_API __declspec(dllimport)
#endif
#else
#define MR_API
#endif
#include <as.h>

typedef  void* MR_SERVER;
typedef  void* MR_CLIENT;

enum MR_MEDIA_TYPE {
    MR_MEDIA_TYPE_H264   = 0,
    MR_MEDIA_TYPE_H265   = 1,
    MR_MEDIA_TYPE_G711A  = 2,
    MR_MEDIA_TYPE_G711U  = 3,
    MR_MEDIA_TYPE_DATA   = 4,
    MR_MEDIA_TYPE_INVALID
};

enum MR_CLIENT_STATUS
{
    MR_CLIENT_STATUS_CONNECTED = 0,
    MR_CLIENT_STATUS_HANDSHAKE = 1,
    MR_CLIENT_STATUS_RUNNING   = 2,
    MR_CLIENT_STATUS_TEARDOWN  = 3,
    MR_CLIENT_STATUS_MAX
};

typedef void (*mk_log)(const char* szFileName, int32_t lLine,int32_t lLevel, const char* format,va_list argp);

typedef int32_t (*rtsp_server_request)(MR_SERVER server,MR_CLIENT client);

typedef int32_t (*handle_client_status)(MR_CLIENT client,MR_CLIENT_STATUS method,void* ctx);

typedef int32_t (*handle_client_media)(MR_CLIENT client,MR_MEDIA_TYPE type,uint32_t pts,char* data,uint32_t len,void* ctx);


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif
    /* init the media rtsp libary */
    MR_API int32_t   mk_lib_rtsp_init(uint32_t EvnCount,mk_log log);
    /* release the media rtsp bibary */
    MR_API void      mk_lib_rtsp_release();
    /* create a media rtsp server handle */
    MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx);
    /* destory the media rtsp server handle */
    MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server);
    /* create a media client handle */
    MR_API MR_CLIENT mk_create_client_handle(char* url,handle_client_status cb,void* ctx);
    /* destory the media client handle */
    MR_API void      mk_destory_client_handle(MR_CLIENT client);
    /* set a media client callback */
    MR_API void      mk_set_client_callback(MR_CLIENT client,handle_client_status cb,void* ctx);
    /* recv media data from media client */
    MR_API int32_t   mk_recv_next_media_data(MR_CLIENT client,char* buf,uint32_t len,handle_client_media cb,void* data);
    /* set a media rtsp client media transport tcp*/
    MR_API void      mk_create_rtsp_client_set_tcp(MR_CLIENT client);
    /* set a media rtsp client/server rtp/rtcp udp port */
    MR_API void      mk_set_rtsp_udp_ports(uint16_t udpstart,uint32_t count);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /*__LIB_MEDIA_RTSP_H__*/