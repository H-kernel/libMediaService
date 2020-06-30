/*
 * StreamStdRtpSession.h
 *
 *  Created on: 2016-5-20
 *      Author:
 */

#ifndef STREAMSTDRTPSESSION_H_
#define STREAMSTDRTPSESSION_H_

#include <list>
#include "ms_engine_session.h"
#include "ms_engine_rtp_session.h"
#include "svs_rtsp_setup_message.h"
#include "svs_static_preassign_buffer.h"
#include "ms_engine_rtcp_packet.h"

typedef std::list<ACE_Message_Block*>  RTP_SEND_LIST;
typedef RTP_SEND_LIST::iterator        RTP_SEND_LIST_ITER;

class CStreamStdRtpSession: public CStreamSession
{
public:
    CStreamStdRtpSession();
    virtual ~CStreamStdRtpSession();
public:

    int32_t initStdRtpSession(uint64_t ullPeerStreamID,
                          PLAY_TYPE      enPlayType,
                          const ACE_INET_Addr &localAddr,
                          const ACE_INET_Addr &peerAddr);

    int32_t startStdRtpSession(const CRtspSetupMessage &rtspMessage);

    void setRtspHandle(ACE_HANDLE handle, const ACE_INET_Addr &addr);

    void setSessionId(uint64_t ullSessionId);

    void setPlayLoad(uint16_t unVedioPT,uint16_t unAudioPT );

    int32_t sendMessage(const char* pData, uint32_t unDataSize,bool updateTime);

    int32_t sendStartRequest();

public:
    int32_t initSesssion(PEER_TYPE unPeerType);

    int32_t sendMediaData(ACE_Message_Block **pMbArray, uint32_t MsgCount);

    int32_t sendVcrMessage(CRtspPacket &rtspPack);

    int32_t handleInnerMessage(const STREAM_INNER_MSG &innerMsg,
                            uint32_t unMsgSize,
                            CStreamSession&  peerSession);

    ACE_INET_Addr getPeerAddr()const;

    ACE_INET_Addr getMediaAddr()const;

    int32_t sendSessionStopMessage(uint32_t unStopType);

    void setSdpInfo(CMediaSdp& rtspSdp);

    virtual uint32_t getMediaTransType()const;

protected:
    int32_t allocMediaPort();

    int32_t startMediaPort();

    int32_t stopMediaPort();

    int32_t sendNatResponse(CStreamNatMessage &natMsg){return RET_OK;};

    bool checkMediaChannelStatus();

    int32_t setRemoteAddress(uint16_t Deviceport, char* setRemoteAddress){return RET_OK;};

private:
    bool checkIsDisconnect(int32_t nErrNo) const;

    int32_t saveLeastData(ACE_Message_Block ** const pMbArray, uint32_t MsgCount,
                      uint32_t nSendSize, uint32_t nSendCount);

    int32_t sendLeastData();

    int32_t sendTcpMediaData(ACE_Message_Block **pMbArray, uint32_t MsgCount);

    int32_t sendUdpMediaData(ACE_Message_Block **pMbArray, uint32_t MsgCount);

    void sendRtcpReport();
private:
    ACE_HANDLE       m_rtspHandle;
    ACE_INET_Addr    m_rtspAddr;
    std::string      m_strRtspSessionId;
    CRtcpPacket      m_rtcpPacket;
    uint32_t         m_unTransType;

    ACE_Thread_Mutex m_TcpSendMutex;
    RTP_SEND_LIST    m_TcpSendList;

    CNetworkHandle*  m_pUdpHandle[HANDLE_TYPE_MAX];
    ACE_INET_Addr    m_UdpPeerAddr[HANDLE_TYPE_MAX];

    uint64_t         m_ullPeerStreamID;

    uint32_t         m_ulLastRecvTime;
    uint32_t         m_unStartTime;
    CMediaSdp         m_rtspSdp;
};

#endif /* STREAMSTDRTPSESSION_H_ */


