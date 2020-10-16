#ifndef __MK_RTSP_CONNECTION_INCLUDE_H__
#define __MK_RTSP_CONNECTION_INCLUDE_H__

#include "as.h"
#include "mk_rtsp_defs.h"
#include "mk_media_sdp.h"
#include "mk_rtsp_udp_handle.h"
#include "mk_rtsp_rtp_frame_organizer.h"
#include "mk_client_connection.h"
#include "mk_rtsp_packet.h"


typedef enum RTSP_SESSION_STATUS
{
    RTSP_SESSION_STATUS_INIT     = 0,
    RTSP_SESSION_STATUS_SETUP    = 1,
    RTSP_SESSION_STATUS_PLAY     = 2,
    RTSP_SESSION_STATUS_PAUSE    = 3,
    RTSP_SESSION_STATUS_TEARDOWN = 4
}RTSP_STATUS;

enum RTSP_INTERLEAVE_NUM
{
    RTSP_INTERLEAVE_NUM_VIDEO_RTP   = 0,
    RTSP_INTERLEAVE_NUM_VIDEO_RTCP  = 1,
    RTSP_INTERLEAVE_NUM_AUDIO_RTP   = 2,
    RTSP_INTERLEAVE_NUM_AUDIO_RTCP  = 3,
    RTSP_INTERLEAVE_NUM_MAX
};


#define RTSP_RETRY_INTERVAL     (200 * 1000)

#define RTP_INTERLEAVE_LENGTH   4

#define RTSP_AUTH_TRY_MAX       3

#define RTSP_DIGEST_LENS_MAX    1024

static std::string STR_NULL = std::string("");



class mk_rtsp_connection: public mk_client_connection,as_tcp_conn_handle,mk_rtsp_rtp_udp_observer,mk_rtp_frame_handler
{
public:
    mk_rtsp_connection();
    virtual ~mk_rtsp_connection();

public:
    virtual int32_t start();
    virtual void    stop();
    virtual int32_t recv_next();
    virtual void    check_client();
    virtual void    set_rtp_over_tcp();
    virtual void    get_rtp_stat_info(RTP_PACKET_STAT_INFO &statinfo);
public:
    /* override */
    virtual void handle_recv(void);
    virtual void handle_send(void);

    virtual int32_t handle_rtp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len);
    virtual int32_t handle_rtcp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len);

    virtual void handleRtpFrame(uint8_t PayloadType,RTP_PACK_QUEUE &rtpFrameList); 
private:
    int32_t processRecvedMessage(const char* pData, uint32_t unDataSize);
    int32_t handleRTPRTCPData(const char* pData, uint32_t unDataSize);
private:
    int32_t sendRtspReq(enRtspMethods enMethod,std::string& strUri,std::string& strTransport,uint64_t ullSessionId = 0);
    int32_t sendRtspOptionsReq();
    int32_t sendRtspDescribeReq();
    int32_t sendRtspSetupReq(SDP_MEDIA_INFO* info);
    int32_t sendRtspPlayReq(uint64_t ullSessionId);
    int32_t sendRtspRecordReq();
    int32_t sendRtspGetParameterReq();
    int32_t sendRtspAnnounceReq();
    int32_t sendRtspPauseReq();
    int32_t sendRtspTeardownReq();
    int32_t handleRtspResp(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspDescribeResp(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspSetUpResp(mk_rtsp_packet &rtspMessage);
private:
    void    handleH264Frame(RTP_PACK_QUEUE &rtpFrameList);
    void    handleH265Frame(RTP_PACK_QUEUE &rtpFrameList);
    void    handleOtherFrame(uint8_t PayloadType,RTP_PACK_QUEUE &rtpFrameList);
    
    int32_t checkFrameTotalDataLen(RTP_PACK_QUEUE &rtpFrameList);
private:
    int32_t sendMsg(const char* pszData,uint32_t len);
    void    resetRtspConnect();
    void    trimString(std::string& srcString) const;
private:
    as_network_svr*              m_pNetWorker;
    RTSP_STATUS                  m_Status;
    bool                         m_bSetupTcp;
    mk_rtsp_udp_handle*          m_udpHandles[MK_RTSP_UDP_TYPE_MAX];
    as_url_t                     m_url;
    mk_media_sdp                 m_sdpInfo;
    MEDIA_INFO_LIST              m_mediaInfoList;
    char                         m_RecvTcpBuf[MAX_BYTES_PER_RECEIVE];
    uint32_t                     m_ulRecvSize;
    uint32_t                     m_ulSeq;
    mk_rtp_frame_organizer       m_rtpFrameOrganizer;
    volatile AS_BOOLEAN          m_bDoNextRecv;

    typedef std::map<uint32_t,uint32_t>        SEQ_METHOD_MAP;
    typedef SEQ_METHOD_MAP::iterator           SEQ_METHOD_ITER;
    SEQ_METHOD_MAP               m_SeqMethodMap;

    uint8_t                      m_ucH264PayloadType;
    uint8_t                      m_ucH265PayloadType;

    time_t                       m_ulLastRecv;
    as_digest_t                  m_Authen;
    uint32_t                     m_ulAuthenTime;
    std::string                  m_strAuthenticate;
};

class mk_rtsp_server : public as_tcp_server_handle
{
public:
    mk_rtsp_server();
    virtual ~mk_rtsp_server();
    void set_callback(rtsp_server_request cb,void* ctx);
public:
    /* override */
    virtual int32_t handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle);
private:
    rtsp_server_request m_cb;
    void*               m_ctx;
};
#endif /* __MK_RTSP_CONNECTION_INCLUDE_H__ */
