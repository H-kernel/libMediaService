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

class mk_rtsp_client: public as_tcp_conn_handle
{
public:
    mk_rtsp_client();
    virtual ~mk_rtsp_client();

public:
    int32_t open(const char* pszUrl);
    int32_t send_rtsp_options(); 
    void    close();

    const char* get_connect_addr();
    uint16_t    get_connect_port();

public:
    /* override */
    virtual void handle_recv(void);
    virtual void handle_send(void);

    void setHandle(ACE_HANDLE handle, const ACE_INET_Addr &localAddr);

    ACE_HANDLE get_handle() const;

    uint32_t getSessionIndex() const;

    int32_t check();

    void setStatus(uint32_t unStatus);

    uint32_t getStatus() const;

    void  set_rtp_over_tcp();

    void  set_status_callback(rtsp_client_status cb,void* ctx);

    int32_t do_recv_media_data(char* buf,uint32_t len, rtsp_client_media cb,void* ctx);
private:
    int32_t setSockOpt();

    int32_t createStreamSession();

    int32_t sendMessage(const char* pData, uint32_t unDataSize);

    int32_t sendMediaSetupReq(CSVSMediaLink* linkInof);

    void    sendMediaPlayReq();

    void    sendKeyFrameReq();

    int32_t createMediaSession();

    void    destroyMediaSession();

private:
    int32_t processRecvedMessage(const char* pData, uint32_t unDataSize);

    int32_t handleRTPRTCPData(const char* pData, uint32_t unDataSize) const;

    void    handleMediaData(const char* pData, uint32_t unDataSize) const;

    int32_t handleRtspMessage(CRtspMessage &rtspMessage);

    // OPITONS
    int32_t handleRtspOptionsReq(CRtspMessage &rtspMessage);

    // DESCRIBE
    int32_t handleRtspDescribeReq(const CRtspMessage &rtspMessage);

    // SETUP
    int32_t handleRtspSetupReq(CRtspMessage &rtspMessage);

    // PLAY
    int32_t handleRtspPlayReq(CRtspMessage &rtspMessage);

    // RECORD
    int32_t handleRtspRecordReq(CRtspMessage &rtspMessage);

    // GET_PARAMETER
    int32_t handleRtspGetParameterReq(CRtspMessage &rtspMessage);

    uint32_t getRange(const std::string strUrl,std::string & strStartTime,std::string & strStopTime);

    int32_t handleRtspAnnounceReq(const CRtspMessage &rtspMessage) ;

    int32_t handleRtspPauseReq(CRtspMessage &rtspMessage);

    int32_t handleRtspTeardownReq(CRtspMessage &rtspMessage);

    void    sendCommonResp(uint32_t unStatusCode, uint32_t unCseq);

    int32_t cacheRtspMessage(CRtspMessage &rtspMessage);

    void    clearRtspCachedMessage();

    int32_t checkTransDirection(uint32_t unPeerType, uint32_t unTransDirection) const;
private:
    int32_t createDistribute(CSVSMediaLink* linkinfo);

    void    simulateSendRtcpMsg();

private:
    as_url_t                     m_url;
    char                         m_RecvBuf[MAX_RTSP_PROTOCOL_MSG_LEN];
    uint32_t                     m_ulRecvSize;


    
    ACE_Recursive_Thread_Mutex   m_RtspMutex;
    uint32_t                     m_unSessionIndex;
    PLAY_TYPE                    m_enPlayType;
    bool                         m_bSetUp;

    ACE_HANDLE                   m_sockHandle;
    ACE_INET_Addr                m_PeerAddr;
    ACE_INET_Addr                m_LocalAddr;

    ACE_Thread_Mutex             m_StatusMutex;
    uint32_t                     m_unSessionStatus;
    uint32_t                     m_ulStatusTime;

    Static_PreAssign_Buffer*     m_pRecvBuffer;
    CRtspProtocol                m_RtspProtocol;
    CMediaSdp                    m_RtspSdp;

    std::string                  m_strContentID;
    CStreamStdRtpSession*           m_pRtpSession;
    CStreamSession*                 m_pPeerSession;

    CRtspMessage*                m_pLastRtspMsg;

    bool                         m_bFirstSetupFlag;
    std::string                  m_strPlayRange;
    int32_t                      m_lRedoTimerId;

    uint32_t                     m_unTransType;
    char                         m_cVideoInterleaveNum;
    char                         m_cAudioInterleaveNum;
    CSVSMediaLink                m_pMediaLink;
  
};

#endif /* STREAMRTSPPUSHSESSION_H_ */
