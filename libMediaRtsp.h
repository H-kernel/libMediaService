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

enum MR_RTSP_MOTHOD
{
    MR_RTSP_METHOD_OPTIONS = 0,
    MR_RTSP_METHOD_DESCRIBE,
    MR_RTSP_METHOD_SETUP,
    MR_RTSP_METHOD_PLAY,
    MR_RTSP_METHOD_RECORD,
    MR_RTSP_METHOD_PAUSE,
    MR_RTSP_METHOD_TEARDOWN,
    MR_RTSP_METHOD_ANNOUNCE,
    MR_RTSP_METHOD_GETPARAMETER,
    RTSP_INVALID_MSG
};

typedef int32_t (*rtsp_server_request)(MR_SERVER server,MR_CLIENT client);

typedef int32_t (*rtsp_client_status)(MR_CLIENT client,MR_RTSP_MOTHOD method,int32_t ret,char* msg,void* ctx);

typedef int32_t (*rtsp_client_media)(MR_CLIENT client,MR_MEDIA_TYPE type,uint32_t pts,char* data,uint32_t len,void* ctx);

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif
    /* init the media rtsp libary */
    MR_API int32_t   mk_lib_rtsp_init(uint16_t udpstart,uint32_t count);
    /* release the media rtsp bibary */
    MR_API void      mk_lib_rtsp_release();
    /* create a media rtsp server handle */
    MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx);
    /* destory the media rtsp server handle */
    MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server);
    /* create a media rtsp client handle */
    MR_API MR_CLIENT mk_create_rtsp_client_handle(char* url,rtsp_client_status cb,void* ctx);
    /* destory the media rtsp client handle */
    MR_API void      mk_destory_rtsp_client_handle(MR_CLIENT client);
    /* set a media rtsp client callback */
    MR_API void      mk_create_rtsp_client_callback(MR_CLIENT client,rtsp_client_status cb,void* ctx);
    /* set a media rtsp client media transport tcp*/
    MR_API void      mk_create_rtsp_client_set_tcp(MR_CLIENT client);
    /* do the media data recvice */
    MR_API int32_t   mk_do_recv_media_data(MR_CLIENT client, char* buf,uint32_t len, rtsp_client_media cb,void* ctx);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /*__LIB_MEDIA_RTSP_H__*/