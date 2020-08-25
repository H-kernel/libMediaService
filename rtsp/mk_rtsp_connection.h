#ifndef STREAMRTSPPUSHSESSION_H_
#define STREAMRTSPPUSHSESSION_H_

#include "as.h"
#include "mk_rtsp_defs.h"
#include "mk_media_sdp.h"
#include "mk_rtsp_udp_handle.h"


typedef enum RTSP_SESSION_STATUS
{
    RTSP_SESSION_STATUS_INIT     = 0,
    RTSP_SESSION_STATUS_SETUP    = 1,
    RTSP_SESSION_STATUS_PLAY     = 2,
    RTSP_SESSION_STATUS_PAUSE    = 3,
    RTSP_SESSION_STATUS_TEARDOWN = 4
}RTSP_STATUS;

typedef enum _enRTP_HANDLE_TYPE
{
    VIDEO_RTP_HANDLE,
    AUDIO_RTP_HANDLE,
    RTP_TYPE_MAX
}RTP_HANDLE_TYPE;

typedef enum _enRTCP_HANDLE_TYPE
{
    VIDEO_RTCP_HANDLE,
    AUDIO_RTCP_HANDLE,
    RTCP_TYPE_MAX
}RTCP_HANDLE_TYPE;


#define RTSP_RETRY_INTERVAL     (200 * 1000)

#define RTP_INTERLEAVE_LENGTH   4

class mk_rtsp_connection: public as_tcp_conn_handle,mk_rtsp_rtp_udp_observer
{
public:
    mk_rtsp_connection();
    virtual ~mk_rtsp_connection();

public:
    int32_t start(const char* pszUrl);
    int32_t send_rtsp_request(); 
    void    stop();

    const char* get_connect_addr();
    uint16_t    get_connect_port();

public:
    /* override */
    virtual void handle_recv(void);
    virtual void handle_send(void);

    virtual int32_t handle_rtp_packet(RTP_HANDLE_TYPE type,char* pData,uint32_t len);
    virtual int32_t handle_rtcp_packet(RTCP_HANDLE_TYPE type,char* pData,uint32_t len);
public:
    void  set_rtp_over_tcp();

    void  set_status_callback(handle_client_status cb,void* ctx);
private:
    int32_t sendMessage(const char* pData, uint32_t unDataSize);
private:
    int32_t processRecvedMessage(const char* pData, uint32_t unDataSize);

    int32_t handleRTPRTCPData(const char* pData, uint32_t unDataSize) const;

    int32_t handleRtspMessage(mk_rtsp_message &rtspMessage);
private:
    int32_t sendRtspOptionsReq();
    int32_t sendRtspDescribeReq();
    int32_t sendRtspSetupReq(SDP_MEDIA_INFO& info);
    int32_t sendRtspPlayReq();
    int32_t sendRtspRecordReq();
    int32_t sendRtspGetParameterReq();
    int32_t sendRtspAnnounceReq();
    int32_t sendRtspPauseReq();
    int32_t sendRtspTeardownReq();
    int32_t sendRtspCmdWithContent(enRtspMethods type,char* headstr,char* content,uint32_t lens);
    int32_t handleRtspResp(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspDescribeResp(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspSetUpResp(mk_rtsp_packet &rtspMessage);
private:
    int32_t handleRtspOptionsReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspDescribeReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspSetupReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspPlayReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspRecordReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspGetParameterReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspSetParameterReq(mk_rtsp_packet &rtspMessage);    
    int32_t handleRtspAnnounceReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspPauseReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspTeardownReq(mk_rtsp_packet &rtspMessage);
    int32_t handleRtspRedirect(mk_rtsp_packet &rtspMessage);
    void    sendRtspResp(uint32_t unStatusCode, uint32_t unCseq);
private:
    void    resetRtspConnect();
    void    trimString(std::string& srcString) const;
private:
    RTSP_STATUS                  m_Status;
    bool                         m_bSetupTcp;
    mk_rtsp_rtp_udp_handle*      m_rtpHandles[RTP_TYPE_MAX];
    mk_rtsp_rtcp_udp_handle*     m_rtcpHandles[RTCP_TYPE_MAX];

    char                         m_cVideoInterleaveNum;
    char                         m_cAudioInterleaveNum;

    as_url_t                     m_url;
    mk_media_sdp                 m_sdpInfo;
    MEDIA_INFO_LIST              m_mediaInfoList;
    char*                        m_RecvBuf[MAX_BYTES_PER_RECEIVE];
    uint32_t                     m_ulRecvSize;
    uint32_t                     m_ulSeq;

    typedef std::map<uint32_t, uint32_t>    REQ_TYPE_MAP;
    typedef REQ_TYPE_MAP::iterator          REQ_TYPE_MAP_ITER;
    REQ_TYPE_MAP                 m_CseqReqMap;  
};

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
#endif /* STREAMRTSPPUSHSESSION_H_ */
