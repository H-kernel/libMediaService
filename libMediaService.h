#ifndef __LIB_MEDIA_RTSP_H__
#define __LIB_MEDIA_RTSP_H__
#ifdef WIN32
#ifdef LIBMEDIASERVICE_EXPORTS
#define MR_API __declspec(dllexport)
#else
#define MR_API __declspec(dllimport)
#endif
#else
#define MR_API
#endif
//#include <as.h>
#include <stdint.h>
#include <stdarg.h>
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
    MR_CLIENT_STATUS_TIMEOUT   = 4,
    MR_CLIENT_STATUS_MAX
};

enum MR_MEDIA_CODE
{
    MR_MEDIA_CODE_OK            = 0,
    MR_MEDIA_CODE_MEMORY_OOM    = 0x1001,
    MR_MEDIA_CODE_MAX
};

typedef struct _stRTP_PACKET_STAT_INFO
{
    uint32_t ulTotalPackNum;
    uint32_t ulLostRtpPacketNum;
    uint32_t ulLostFrameNum;
    uint32_t ulDisorderSeqCounts;
}RTP_PACKET_STAT_INFO;

typedef struct _stMEDIA_DATA_INFO
{
    MR_MEDIA_TYPE type;
    MR_MEDIA_CODE code;
    uint32_t isKeyFrame;
    uint32_t pts;
}MEDIA_DATA_INFO;

typedef void (*mk_log)(const char* szFileName, int32_t lLine,int32_t lLevel, const char* format,va_list argp);

typedef int32_t (*rtsp_server_request)(MR_SERVER server,MR_CLIENT client);

typedef int32_t (*handle_client_status)(MR_CLIENT client,MR_CLIENT_STATUS status,void* ctx);

typedef char* (*handle_client_get_buffer)(MR_CLIENT client,uint32_t len,uint32_t& ulBufLen,void* ctx);

typedef int32_t (*handle_client_media)(MR_CLIENT client,MEDIA_DATA_INFO dataInfo,uint32_t len,void* ctx);

typedef struct
{
    handle_client_status        m_cb_status;
    handle_client_get_buffer    m_cb_buffer;
    handle_client_media         m_cb_data;
    void*                       ctx;
}MEDIA_CALL_BACK;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif
    /* init the media  library */
    MR_API int32_t   mk_lib_init(uint32_t EvnCount,mk_log log,uint32_t MaxClient,uint32_t RtpBufCountPerClient);
    /* release the media  library */
    MR_API void      mk_lib_release();
    /* create a media server handle */
    MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx);
    /* destory the media  server handle */
    MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server);
    /* create a media client handle */
    MR_API MR_CLIENT mk_create_client_handle(char* url,MEDIA_CALL_BACK* cb,void* ctx);
    /* destory the media client handle */
    MR_API void      mk_destory_client_handle(MR_CLIENT client);
    /* start the media client handle */
    MR_API int32_t   mk_start_client_handle(MR_CLIENT client);
    /* stop the media client handle */
    MR_API void      mk_stop_client_handle(MR_CLIENT client);
    /* set a media client callback */
    MR_API void      mk_set_client_callback(MR_CLIENT client,MEDIA_CALL_BACK cb,void* ctx);
    /* recv media data from media client */
    MR_API int32_t   mk_recv_next_media_data(MR_CLIENT client);
    /* set a media rtsp client media transport tcp*/
    MR_API void      mk_create_rtsp_client_set_tcp(MR_CLIENT client);
    /* set a media rtsp client/server rtp/rtcp udp port */
    MR_API void      mk_set_rtsp_udp_ports(uint16_t udpstart,uint32_t count);
    /* get a media rtsp client/server rtp packet stat info */
    MR_API void      mk_get_client_rtp_stat_info(MR_CLIENT client,RTP_PACKET_STAT_INFO &statinfo);
    /* get a media rtsp client sdp info */
    MR_API void      mk_get_client_rtsp_sdp_info(MR_CLIENT client,char* sdpInfo,uint32_t lens,uint32_t& copylen);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /*__LIB_MEDIA_RTSP_H__*/