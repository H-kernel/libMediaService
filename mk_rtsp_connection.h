/*
 * StreamRtspPushSession.h
 *
 *  Created on: 2016-5-16
 *      Author:
 */

#ifndef STREAMRTSPPUSHSESSION_H_
#define STREAMRTSPPUSHSESSION_H_

#include "as.h"
#include "ms_engine_session.h"
#include "svs_static_preassign_buffer.h"
#include "svs_rtsp_protocol.h"
#include "ms_engine_std_rtp_session.h"
#include "svs_media_sdp.h"
#include "svs_media_hot_link.h"


enum RTSP_SESSION_STATUS
{
    RTSP_SESSION_STATUS_INIT     = 0,
    RTSP_SESSION_STATUS_SETUP    = 1,
    RTSP_SESSION_STATUS_PLAY     = 2,
    RTSP_SESSION_STATUS_PAUSE    = 3,
    RTSP_SESSION_STATUS_TEARDOWN = 4
};


#define RTSP_RETRY_INTERVAL     (200 * 1000)

#define RTP_INTERLEAVE_LENGTH   4

class mk_rtsp_connection: public as_tcp_conn_handle
{
public:
    mk_rtsp_connection();
    virtual ~mk_rtsp_connection();

public:
    int32_t open(const char* pszUrl);
    int32_t send_rtsp_request(); 
    void    close();

    const char* get_connect_addr();
    uint16_t    get_connect_port();

public:
    /* override */
    virtual void handle_recv(void);
    virtual void handle_send(void);

    void  set_rtp_over_tcp();

    void  set_status_callback(rtsp_client_status cb,void* ctx);
private:
    int32_t sendMessage(const char* pData, uint32_t unDataSize);
private:
    int32_t processRecvedMessage(const char* pData, uint32_t unDataSize);

    int32_t handleRTPRTCPData(const char* pData, uint32_t unDataSize) const;

    void    handleMediaData(const char* pData, uint32_t unDataSize) const;

    int32_t handleRtspMessage(mk_rtsp_message &rtspMessage);
private:
    int32_t sendRtspOptionsReq();
    int32_t sendRtspDescribeReq();
    int32_t sendRtspSetupReq();
    int32_t sendRtspPlayReq();
    int32_t sendRtspRecordReq();
    int32_t sendRtspGetParameterReq();
    int32_t sendRtspAnnounceReq();
    int32_t sendRtspPauseReq();
    int32_t sendRtspTeardownReq();
    int32_t sendRtspCmdWithContent(RtspMethodType type,char* headstr,char* content,uint32_t lens);
    int32_t handleRtspResp();
private:
    int32_t handleRtspOptionsReq(mk_rtsp_message &rtspMessage);
    int32_t handleRtspDescribeReq(const mk_rtsp_message &rtspMessage);
    int32_t handleRtspSetupReq(mk_rtsp_message &rtspMessage);
    int32_t handleRtspPlayReq(mk_rtsp_message &rtspMessage);
    int32_t handleRtspRecordReq(mk_rtsp_message &rtspMessage);
    int32_t handleRtspGetParameterReq(mk_rtsp_message &rtspMessage);
    int32_t handleRtspAnnounceReq(const mk_rtsp_message &rtspMessage);
    int32_t handleRtspPauseReq(mk_rtsp_message &rtspMessage);
    int32_t handleRtspTeardownReq(mk_rtsp_message &rtspMessage);
    void    sendRtspResp(uint32_t unStatusCode, uint32_t unCseq);
private:
    void    trimString(std::string& srcString) const;
private:
    ACE_Recursive_Thread_Mutex   m_RtspMutex;
    static std::string           m_RtspCode[];
    static std::string           m_strRtspMethod[];

    as_url_t                     m_url;
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
