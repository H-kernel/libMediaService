
#include <sstream>
#include "mk_rtsp_connection.h"
#include "mk_rtsp_packet.h"
#include "mk_media_service.h"
#include "mk_rtsp_message_options.h"



mk_rtsp_connection::mk_rtsp_connection()
{
    resetRtspConnect();
}

mk_rtsp_connection::~mk_rtsp_connection()
{
}


int32_t mk_rtsp_connection::start(const char* pszUrl)
{
    as_network_addr local;
    as_network_addr remote;
    if(AS_ERROR_CODE_OK != as_parse_url(pszUrl,&m_url)) {
        return AS_ERROR_CODE_FAIL;
    }
    as_network_svr* pNetWork = mk_media_service::instance().get_client_network_svr(this);

    remote.m_ulIpAddr = inet_aton((char*)&m_url.host[0]);
    if(0 == m_url.port) {
        remote.m_usPort   = RTSP_DEFAULT_PORT;
    }
    else {
        remote.m_usPort   = m_url.port;
    }
    
    if(AS_ERROR_CODE_OK != pNetWork->regTcpClient(&remote,&remote,this,enSyncOp,5)) {
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != this->sendRtspOptionsReq()) {
        AS_LOG(AS_LOG_WARNING,"options:rtsp client send message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    setHandleRecv(AS_TRUE);
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_connection::stop()
{
    resetRtspConnect();
    setHandleRecv(AS_FALSE);
    AS_LOG(AS_LOG_INFO,"close rtsp client.");
    return;
}
const char* mk_rtsp_connection::get_connect_addr()
{
    return (const char*)&m_url.host[0];
}
uint16_t    mk_rtsp_connection::get_connect_port()
{
    return m_url.port;
}

void  mk_rtsp_connection::set_rtp_over_tcp()
{
    m_bSetupTcp = true;
    return 
}

void  mk_rtsp_connection::set_status_callback(tsp_client_status cb,void* ctx)
{

}
void mk_rtsp_connection::handle_recv(void)
{
    as_network_addr peer;
    int32_t iRecvLen = (int32_t) (MAX_BYTES_PER_RECEIVE -  m_ulRecvSize);
    if (iRecvLen <= 0)
    {
        AS_LOG(AS_LOG_INFO,"rtsp connection,recv buffer is full, size[%u] length[%u].",
                MAX_BYTES_PER_RECEIVE,
                m_ulBufSize);
        return;
    }

    iRecvLen = this->recv(&m_RecvBuf[iRecvLen],&peer,iRecvLen,enAsyncOp);
    if (iRecvLen <= 0)
    {
        AS_LOG(AS_LOG_INFO,"rtsp connection recv data fail.");
        return;
    }

    m_ulRecvSize += iRecvLen;

    uint32_t processedSize = 0;
    uint32_t totalSize = m_ulRecvSize;
    int32_t nSize = 0;
    do
    {
        nSize = processRecvedMessage(&m_RecvBuf[processedSize],
                                     m_ulRecvSize - processedSize);
        if (nSize < 0) {
            AS_LOG(AS_LOG_WARNING,"tsp connection process recv data fail, close handle. ");
            return;
        }

        if (0 == nSize) {
            break;
        }

        processedSize += (uint32_t) nSize;
    }while (processedSize < totalSize);

    uint32_t dataSize = m_ulRecvSize - processedSize;
    if(0 < dataSize) {
        memmove(&m_RecvBuf[0],&m_RecvBuf[processedSize], dataSize);
    }
    m_ulRecvSize = dataSize;
    setHandleSend(AS_TRUE);
    return;
}
void mk_rtsp_connection::handle_send(void)
{
    setHandleSend(AS_FALSE);
}
int32_t mk_rtsp_connection::handle_rtp_packet(RTP_HANDLE_TYPE type,char* pData,uint32_t len) 
{
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handle_rtcp_packet(RTCP_HANDLE_TYPE type,char* pData,uint32_t len)
{
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_connection::sendMessage(const char* pData, uint32_t unDataSize)
{
    if (NULL != m_pRtpSession)
    {
        return m_pRtpSession->sendMessage(pData, unDataSize,true);
    }

    ACE_Time_Value timeout(1);
    int32_t nSendSize = ACE::send_n(m_sockHandle, pData, unDataSize, &timeout);
    if (unDataSize != (uint32_t)nSendSize)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection send message fail, close handle[%d].",
                        m_unSessionIndex, m_sockHandle);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_connection::processRecvedMessage(const char* pData, uint32_t unDataSize)
{
    if ((NULL == pData) || (0 == unDataSize))
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_INTERLEAVE_FLAG == pData[0])
    {
        return handleRTPRTCPData(pData, unDataSize);
    }
    
    /* rtsp message */
    mk_rtsp_packet rtspPacket;
    uint32_t ulMsgLen  = 0;

    int32_t nRet = rtspPacket.checkRtsp(pData,unDataSize,ulMsgLen);

    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection check rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    if(0 == ulMsgLen) {
        return 0; /* need more data deal */
    }

    nRet = rtspPacket.parse(pData,unDataSize);
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection parser rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    switch (rtspPacket.getMethodIndex())
    {
        case RtspDescribeMethod:
        {
            nRet = handleRtspDescribeReq(rtspPacket);
            break;
        }
        case RtspSetupMethod:
        {
            nRet = handleRtspSetupReq(rtspPacket);
            break;
        }        
        case RtspTeardownMethod:
        {
            nRet = handleRtspTeardownReq(rtspPacket);
            break;
        }
        case RtspPlayMethod:
        {
            nRet = handleRtspPlayReq(rtspPacket);
            break;
        }
        case RtspPauseMethod:
        {
            nRet = handleRtspPauseReq(rtspPacket);
            break;
        }
        case RtspOptionsMethod:
        {
            nRet = handleRtspOptionsReq(rtspPacket);
            break;
        }
        case RtspAnnounceMethod:
        {
            nRet = handleRtspOptionsReq(rtspPacket);
            break;
        }
        case RtspGetParameterMethod:
        {
            nRet = handleRtspGetParameterReq(rtspPacket);
            break;
        }
        case RtspSetParameterMethod:
        {
            nRet = handleRtspSetParameterReq(rtspPacket);
            break;
        }
        case RtspRedirectMethod:
        {
            nRet = handleRtspRedirect(rtspPacket);
            break;
        }
        case RtspRecordMethod:
        {
            nRet = handleRtspRecordReq(rtspPacket);
            break;
        }
        case RtspResponseMethod:
        {
            nRet = handleRtspResp(rtspPacket);
            break;
        }
        
        default:
        {
            nRet = AS_ERROR_CODE_FAIL;
            break;
        }
    }
    if(AS_ERROR_CODE_OK != nRet) {
        AS_LOG(AS_LOG_WARNING,"rtsp connection process rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    AS_LOG(AS_LOG_INFO,"rtsp connection success to process rtsp message.");
    return ulMsgLen;
}

int32_t mk_rtsp_connection::handleRTPRTCPData(const char* pData, uint32_t unDataSize) const
{
    if (unDataSize < RTSP_INTERLEAVE_HEADER_LEN)
    {
        return 0;
    }
    uint32_t unMediaSize = (uint32_t) ntohs(*(uint16_t*)(void*)&pData[2]);
    if (unDataSize - RTSP_INTERLEAVE_HEADER_LEN < unMediaSize)
    {
        return 0;
    }

    if (m_cVideoInterleaveNum == pData[1]) {
        handle_rtp_packet(VIDEO_RTP_HANDLE,(const char*)(pData+RTSP_INTERLEAVE_HEADER_LEN),unMediaSize);
    }
    else if(m_cAudioInterleaveNum == pData[1]) {
        handle_rtp_packet(AUDIO_RTP_HANDLE,(const char*)(pData+RTSP_INTERLEAVE_HEADER_LEN),unMediaSize);
    }
    else if ((m_cVideoInterleaveNum + 1) == pData[1]) {
        handle_rtcp_packet(VIDEO_RTCP_HANDLE,(const char*)(pData+RTSP_INTERLEAVE_HEADER_LEN),unMediaSize);
    }
    else if((m_cAudioInterleaveNum + 1 ) == pData[1]) {
        handle_rtcp_packet(AUDIO_RTCP_HANDLE,(const char*)(pData+RTSP_INTERLEAVE_HEADER_LEN),unMediaSize);
    }

    return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN);
}


int32_t mk_rtsp_connection::handleRtspMessage(mk_rtsp_message &rtspMessage)
{
    if (RTSP_MSG_REQ != rtspMessage.getMsgType())
    {
        if (RTSP_METHOD_ANNOUNCE != rtspMessage.getMethodType())
        {
            AS_LOG(AS_LOG_WARNING,"rtsp connection handle not accepted method[%u].",
                              m_unSessionIndex, rtspMessage.getMethodType());
            return AS_ERROR_CODE_FAIL;
        }
    }

    int32_t nRet = AS_ERROR_CODE_OK;

    switch(rtspMessage.getMethodType())
    {
    case RtspOptionsMethod:
        nRet = handleRtspOptionsReq(rtspMessage);
        break;
    case RtspDescribeMethod:
        nRet = handleRtspDescribeReq(rtspMessage);
        break;
    case RtspSetupMethod:
        nRet = handleRtspSetupReq(rtspMessage);
        break;
    case RtspPlayMethod:
        nRet = handleRtspPlayReq(rtspMessage);
        break;
    case RtspPauseMethod:
        nRet = handleRtspPauseReq(rtspMessage);
        break;
    case RtspTeardownMethod:
        nRet = handleRtspTeardownReq(rtspMessage);
        break;
    case RtspAnnounceMethod:
        nRet = handleRtspAnnounceReq(rtspMessage);
        break;
    case RtspRecordMethod:
        nRet = handleRtspRecordReq(rtspMessage);
        break;
    case RtspGetParameterMethod:
        nRet = handleRtspGetParameterReq(rtspMessage);
        break;
    default:
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle not accepted method[%u].",
                        m_unSessionIndex, rtspMessage.getMethodType());
        return AS_ERROR_CODE_FAIL;
    }
    return nRet;
}
int32_t mk_rtsp_connection::sendRtspOptionsReq()
{
    return sendRtspCmdWithContent(RTSP_METHOD_OPTIONS,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspDescribeReq()
{
    return sendRtspCmdWithContent(RTSP_METHOD_DESCRIBE,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspSetupReq(SDP_MEDIA_INFO& info)
{
    std::string strSetUpHead = "";
    // Transport
    strSetUpHead += RTSP_TOKEN_STR_TRANSPORT;

    // (RTP)
    strSetUpHead += RTSP_TRANSPORT_RTP;
    strSetUpHead += RTSP_TRANSPORT_SPEC_SPLITER;
    strSetUpHead += RTSP_TRANSPORT_PROFILE_AVP;

    //(TCP/UDP)
    if (m_bSetupTcp)
    {
        strSetUpHead += RTSP_TRANSPORT_SPEC_SPLITER;
        strSetUpHead += RTSP_TRANSPORT_TCP;
    }
    strSetUpHead += SIGN_SEMICOLON;

    if (m_bSetupTcp)
    {
        strSetUpHead += RTSP_TRANSPORT_UNICAST;
        strSetUpHead += SIGN_SEMICOLON;
        strSetUpHead += RTSP_TRANSPORT_INTERLEAVED;
        std::stringstream strChannelNo;
        strChannelNo << m_unInterleaveNum;
        strSetUpHead += strChannelNo.str() + SIGN_H_LINE;

        strChannelNo.str("");
        strChannelNo << m_unInterleaveNum + 1;
        strSetUpHead += strChannelNo.str();
    }
    else
    {
        strSetUpHead += RTSP_TRANSPORT_UNICAST;
        strSetUpHead += SIGN_SEMICOLON;

        mk_rtsp_rtp_udp_handle*  pRtpHandle  = NULL;
        mk_rtsp_rtcp_udp_handle* pRtcpHandle = NULL;

        if(AS_ERROR_CODE_OK != mk_media_service::instance().get_rtp_rtcp_pair(pRtpHandle,pRtcpHandle)) {
            AS_LOG(AS_LOG_ERROR,"rtsp connection client get rtp and rtcp handle for udp fail.");
            return AS_ERROR_CODE_FAIL;
        }

        if(MEDIA_TYPE_VALUE_VIDEO == info.ucMediaType) {
            m_rtpHandles[VIDEO_RTP_HANDLE]   = pRtpHandle;
            m_rtcpHandles[VIDEO_RTCP_HANDLE] = pRtcpHandle;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info.ucMediaType){
            m_rtpHandles[AUDIO_RTP_HANDLE]   = pRtpHandle;
            m_rtcpHandles[AUDIO_RTCP_HANDLE] = pRtcpHandle;
        }
        else {
            AS_LOG(AS_LOG_ERROR,"rtsp connection client ,the media type is not rtp and rtcp.");
            return AS_ERROR_CODE_FAIL;
        }

        pRtpHandle->open(this);
        pRtcpHandle->open(this);

        strSetUpHead += RTSP_TRANSPORT_CLIENT_PORT;
        std::stringstream strPort;
        strPort << pRtpHandle->get_port();
        strSetUpHead += strPort.str() + SIGN_H_LINE;
        strPort.str("");
        strPort << pRtcpHandle->get_port();
        strSetUpHead += strPort.str();
        
        /*
        if (RTSP_MSG_RSP == getMsgType())
        {
            strSetUpHead += SIGN_SEMICOLON;

            addr.set(m_usServerPort, m_unSrcIp);
            strMessage += RTSP_TRANSPORT_SERVER_PORT;
            strPort.str("");
            strPort << addr.get_port_number();
            strMessage += strPort.str() + SIGN_H_LINE;
            strPort.str("");
            strPort << addr.get_port_number() + 1;
            strMessage += strPort.str();
        }
        */
    }
    strSetUpHead += RTSP_END_TAG;

    return sendRtspCmdWithContent(RtspSetupMethod,strSetUpHead.c_str(),NULL,0);
}
int32_t mk_rtsp_connection::sendRtspPlayReq()
{
    return sendRtspCmdWithContent(RtspPlayMethod,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspRecordReq()
{
    return sendRtspCmdWithContent(RtspRecordMethod,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspGetParameterReq()
{
    return sendRtspCmdWithContent(RtspGetParameterMethod,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspAnnounceReq()
{
    return sendRtspCmdWithContent(RtspAnnounceMethod,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspPauseReq()
{
    return sendRtspCmdWithContent(RtspPauseMethod,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspTeardownReq()
{
    return sendRtspCmdWithContent(RtspTeardownMethod,NULL,NULL,0);
}
int32_t mk_rtsp_connection::sendRtspCmdWithContent(enRtspMethods type,char* headstr,char* content,uint32_t lens)
{
    char message[MAX_RTSP_MSG_LEN] = {0};
    
    uint32_t ulHeadLen = 0;
    char*    start     = &message[ulHeadLen];
    uint32_t ulBufLen  = MAX_RTSP_MSG_LEN - ulHeadLen;
    
    /* first line */
    snprintf(start, ulBufLen, "%s %s RTSP/1.0\r\n"
                              "CSeq: %d\r\n"
                              "User-Agent: h.kernel\r\n", 
                              mk_rtsp_packet::m_strRtspMethods[type].c_str(), &m_url.uri[0],m_ulSeq);
    ulHeadLen = strlen(message);
    start     = &message[ulHeadLen];
    ulBufLen  = MAX_RTSP_MSG_LEN - ulHeadLen;

    if(NULL != headstr) {
        snprintf(start, ulBufLen, "%s",headstr);
        ulHeadLen = strlen(message);
        start     = &message[ulHeadLen];
        ulBufLen  = MAX_RTSP_MSG_LEN - ulHeadLen;
    }
    /*
    if (rt->auth[0]) {
        char *str = ff_http_auth_create_response(&rt->auth_state,
                                                 rt->auth, url, method);
        if (str)
            av_strlcat(buf, str, sizeof(buf));
        av_free(str);
    }
    */

    /* content */
    if (lens > 0 && content) {
        snprintf(start, ulBufLen,"Content-Length: %d\r\n\r\n", lens);
        ulHeadLen = strlen(message);
        start     = &message[ulHeadLen];
        ulBufLen  = MAX_RTSP_MSG_LEN - ulHeadLen;
        memcpy(start,content,lens);
        ulHeadLen += lens;
    }
    else {
        snprintf(start, ulBufLen, "\r\n");
        ulHeadLen = strlen(message);
    }

    int nSendSize = this->send(&message[0],ulHeadLen,enSyncOp);
    if(nSendSize != ulHeadLen) {
        AS_LOG(AS_LOG_WARNING,"rtsp client send message fail,lens:[%d].",ulHeadLen);
        return AS_ERROR_CODE_FAIL;
    }

    setHandleRecv(AS_TRUE);

    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspResp(mk_rtsp_packet &rtspMessage)
{
    uint32_t nCseq     = rtspMessage.getCseq();
    uint32_t nRespCode = rtspMessage.getRtspStatusCode();
    enRtspMethods enMethod   = RTSP_REQ_METHOD_NUM;

    enMethod = iter->second;
    AS_LOG(AS_LOG_INFO,"rtsp client handle server reponse seq:[%d] ,mothod:[%d].",nCseq,enMethod);

    if(RtspStatus_200 != nRespCode) {
        /*close socket */
        AS_LOG(AS_LOG_WARNING,"rtsp client there server reponse code:[%d].",nRespCode);
        return AS_ERROR_CODE_FAIL;
    }
    int nRet = AS_ERROR_CODE_OK;
    switch (enMethod)
    {
        case RtspDescribeMethod:
        {
            nRet = handleRtspDescribeResp(rtspMessage);
            break;
        }
        case RtspSetupMethod:
        {
            nRet = handleRtspSetUpResp(rtspMessage);
            break;
        }        
        case RtspTeardownMethod:
        {
            //nothing to do close the socket
            m_Status = RTSP_SESSION_STATUS_TEARDOWN;
            break;
        }
        case RtspPlayMethod:
        {
            //start rtcp and media check timer
            m_Status = RTSP_SESSION_STATUS_PLAY;
            break;
        }
        case RtspPauseMethod:
        {
            //nothing to do
            m_Status = RTSP_SESSION_STATUS_PAUSE;
            break;
        }
        case RtspOptionsMethod:
        {
            if(RTSP_SESSION_STATUS_INIT == m_Status) {
                nRet = sendRtspDescribeReq();
            }
            break;
        }
        case RtspAnnounceMethod:
        {
            //TODO: 
            break;
        }
        case RtspGetParameterMethod:
        {
            // nothing to do
            break;
        }
        case RtspSetParameterMethod:
        {
            // nothing to do
            break;
        }
        case RtspRedirectMethod:
        {
            // nothing to do
            break;
        }
        case RtspRecordMethod:
        {
            // rtsp client push media to server
            m_Status = RTSP_SESSION_STATUS_PLAY;
            break;
        }
        case RtspResponseMethod:
        {
            // nothing to do
            break;
        }
        
        default:
        {
            nRet = AS_ERROR_CODE_FAIL;
            break;
        }
    }
    AS_LOG(AS_LOG_INFO,"rtsp client handle server reponse end.");
    return nRet;
}

int32_t mk_rtsp_connection::handleRtspDescribeResp(mk_rtsp_packet &rtspMessage)
{
    std::string strSdp = "";
    rtspMessage.getContent(strSdp);

    AS_LOG(AS_LOG_INFO,"rtsp client connection handle describe response sdp:[%s].",strSdp.c_str());

    int32_t nRet = m_sdpInfo.decodeSdp(strSdp);
    if(AS_ERROR_CODE_OK != nRet) {
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe sdp:[%s] fail.",strSdp.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    m_mediaInfoList.clear();
    m_sdpInfo.getVideoInfo(m_mediaInfoList);
    m_sdpInfo.getAudioInfo(m_mediaInfoList);

    if(0 == m_mediaInfoList.size()) {
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe response fail,there is no media info.");
        return AS_ERROR_CODE_FAIL;
    }

    SDP_MEDIA_INFO& info = m_mediaInfoList.front();
    nRet = sendRtspSetupReq(info);
    if(AS_ERROR_CODE_OK != nRet) {
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe response,send setup fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_mediaInfoList.pop_front();
    
    AS_LOG(AS_LOG_INFO,"rtsp client connection handle describe response end.");
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspSetUpResp(mk_rtsp_packet &rtspMessage)
{
    AS_LOG(AS_LOG_INFO,"rtsp client connection handle setup response begin.");
    int32_t nRet = AS_ERROR_CODE_OK;
    if(0 < m_mediaInfoList.size()) {
        AS_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,send next setup messgae.");
        SDP_MEDIA_INFO& info = m_mediaInfoList.front();
        nRet = sendRtspSetupReq(info);
        if(AS_ERROR_CODE_OK != nRet) {
            AS_LOG(AS_LOG_WARNING,"rtsp connection handle setup response,send next setup fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_mediaInfoList.pop_front();
    }
    else {
        m_Status = RTSP_SESSION_STATUS_SETUP;
        /* send play */
        AS_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,send play messgae.");
        nRet = sendRtspPlayReq();
    }
    AS_LOG(AS_LOG_INFO,"rtsp client connection handle setup response end.");
    return nRet;
}

int32_t mk_rtsp_connection::handleRtspOptionsReq(mk_rtsp_message &rtspMessage)
{
    CRtspOptionsMessage *pRequest = dynamic_cast<CRtspOptionsMessage*>(&rtspMessage);
    if (NULL == pRequest)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle options request fail.");
        return AS_ERROR_CODE_FAIL;
    }


    pRequest->setRange(m_strPlayRange);
    pRequest->setStatusCode(RTSP_SUCCESS_OK);
    pRequest->setMsgType(RTSP_MSG_RSP);
    std::stringstream sessionIdex;
    sessionIdex << m_unSessionIndex;
    pRequest->setSession(sessionIdex.str());

    std::string strResp;
    if (AS_ERROR_CODE_OK != pRequest->encodeMessage(strResp))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection encode options response fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection send options response fail.");
        return AS_ERROR_CODE_FAIL;
    }

    AS_LOG(AS_LOG_INFO,"rtsp connection send options response success.");

    return AS_ERROR_CODE_OK;
}



int32_t mk_rtsp_connection::handleRtspDescribeReq(const mk_rtsp_message &rtspMessage)
{
    //CSVSMediaLink MediaLink;
    if (RTSP_SESSION_STATUS_INIT != getStatus())
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = CSVSMediaLinkFactory::instance().parseMediaUrl(rtspMessage.getRtspUrl(),&m_pMediaLink);
    if((SVS_MEDIA_LINK_RESULT_SUCCESS != nRet)
        &&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL != nRet))
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe req fail, content invalid.");
        return AS_ERROR_CODE_FAIL;
    }
    if(SVS_MEDIA_LINK_RESULT_AUTH_FAIL == nRet)
    {
        if(CStreamConfig::instance()->getUrlEffectiveWhile())
        {
            close();
            AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe req fail, auth invalid.",m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }
    }

    m_strContentID = m_pMediaLink.ContentID();

    if(NULL == m_pPeerSession)
    {
        m_pPeerSession = CStreamSessionFactory::instance()->findSession(m_strContentID);
        if ((NULL != m_pPeerSession)&&(PLAY_TYPE_AUDIO_LIVE == m_enPlayType))
        {
              CStreamSessionFactory::instance()->releaseSession(m_pPeerSession);
              m_pPeerSession = NULL;
              AS_LOG(AS_LOG_ERROR, "The Audio Request failed");
              return AS_ERROR_CODE_FAIL;
        }
    }
    
 
    if (NULL == m_pPeerSession)
    {
        CRtspDescribeMessage *pReq = new CRtspDescribeMessage();
        if (!pReq)  //lint !e774
        {
            return AS_ERROR_CODE_FAIL;
        }
        pReq->setRtspUrl(rtspMessage.getRtspUrl());
        pReq->setCSeq(rtspMessage.getCSeq());
        std::stringstream sessionIdex;
        sessionIdex << m_unSessionIndex;
        pReq->setSession(sessionIdex.str());

        if (!m_pLastRtspMsg)
        {
            delete m_pLastRtspMsg;
        }
        m_pLastRtspMsg = pReq;
        AS_LOG(AS_LOG_INFO,"rtsp session[%u] save describe request[%p].",
                        m_unSessionIndex, m_pLastRtspMsg);

        if (AS_ERROR_CODE_OK != sendMediaSetupReq(&m_pMediaLink))
        {
            delete m_pLastRtspMsg;
            m_pLastRtspMsg = NULL;

            AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe request fail, "
                    "send setup request fail.",
                    m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        return AS_ERROR_CODE_OK;
    }


    std::string strSdp;
    SDP_MEDIA_INFO  videoInfo, audioInfo;


    SDP_MEDIA_INFO stVideoInfo;
    SDP_MEDIA_INFO stAideoInfo;

    stVideoInfo.strControl   = "";
    stVideoInfo.strFmtp      = "";
    stVideoInfo.strRtpmap    = "";
    stVideoInfo.ucPayloadType= PT_TYPE_H264;
    stVideoInfo.usPort       = 0;

    stAideoInfo.strControl   = "";
    stAideoInfo.strFmtp      = "";
    stAideoInfo.strRtpmap    = "";
    stAideoInfo.ucPayloadType= PT_TYPE_PCMU;
    stAideoInfo.usPort       = 0;

     if ( (PLAY_TYPE_LIVE == m_enPlayType) || (PLAY_TYPE_FRONT_RECORD == m_enPlayType) || (PLAY_TYPE_PLAT_RECORD == m_enPlayType))
    {
        stVideoInfo.ucPayloadType = PT_TYPE_H264; /* PS -->H264 */
        m_RtspSdp.addVideoInfo(stVideoInfo);
        stVideoInfo.ucPayloadType = PT_TYPE_H265; /* PS -->H265 */
        m_RtspSdp.addVideoInfo(stVideoInfo);
        // stAideoInfo.ucPayloadType= PT_TYPE_PCMU;
        // m_RtspSdp.addAudioInfo(stAideoInfo);
        stAideoInfo.ucPayloadType= PT_TYPE_PCMA;
        m_RtspSdp.addAudioInfo(stAideoInfo);
    }
    else if ( PLAY_TYPE_AUDIO_LIVE == m_enPlayType)//�����GB28181Э�������PCMA,�����EHOME
    {
        
        stAideoInfo.ucPayloadType= PT_TYPE_PCMA;
        m_RtspSdp.addAudioInfo(stAideoInfo);
    }

    int32_t isplayback = 0;
    std::string strtimeRange="";
    if ( PLAY_TYPE_PLAT_RECORD == m_enPlayType)
    {
        isplayback = 1;
        if ("" == m_RtspSdp.getRange())
        {
            std::string strtmp = "";
            std::string strtmpUrl = m_RtspSdp.getUrl();
            std::string strStartTime = "";
            std::string strEndTime = "";
            strtmp += "range:npt=0-";
            uint32_t num = 0;
            num = getRange(strtmpUrl,strStartTime,strEndTime);

        if( 0 == num)
        {
            AS_LOG(AS_LOG_WARNING,"get timeRange fail,range in url is 0.");
            return AS_ERROR_CODE_FAIL;
        }

         std::stringstream stream;
         stream<<num;
         stream>>strtimeRange;
         strtmp += strtimeRange;
         AS_LOG(AS_LOG_WARNING,"time is = [%s]",strtmp.c_str());
         m_RtspSdp.setRange(strtmp);
        }
    }

    if (AS_ERROR_CODE_OK != m_RtspSdp.encodeSdp(strSdp,isplayback,strtimeRange,0,0))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection encode sdp info fail.");
        return AS_ERROR_CODE_FAIL;
    }


    CRtspDescribeMessage resp;
    resp.setMsgType(RTSP_MSG_RSP);
    resp.setCSeq(rtspMessage.getCSeq());
    resp.setStatusCode(RTSP_SUCCESS_OK);

    std::stringstream sessionIdex;
    sessionIdex << m_unSessionIndex;
    resp.setSession(sessionIdex.str());
    resp.setSdp(strSdp);

    std::string strResp;
    if (AS_ERROR_CODE_OK != resp.encodeMessage(strResp))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection encode describe response fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection send describe response fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_bSetUp = true;

    AS_LOG(AS_LOG_INFO,"rtsp connection handle describe request success.");
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_connection::handleRtspSetupReq(mk_rtsp_message &rtspMessage)
{
    std::string  strContentID;
    //CSVSMediaLink MediaLink;

    if (RTSP_SESSION_STATUS_SETUP < getStatus())
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle setup req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspSetupMessage *pReq = dynamic_cast<CRtspSetupMessage*>(&rtspMessage);
    if (!pReq)  //lint !e774
    {
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = CSVSMediaLinkFactory::instance().parseMediaUrl(rtspMessage.getRtspUrl(),&m_pMediaLink);
    if((SVS_MEDIA_LINK_RESULT_SUCCESS != nRet)
        &&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL != nRet))
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle setup req fail, content invalid.");
        return AS_ERROR_CODE_FAIL;
    }
    if((!m_bSetUp)&&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL == nRet))
    {
        if(CStreamConfig::instance()->getUrlEffectiveWhile())
        {
            close();
            AS_LOG(AS_LOG_WARNING,"rtsp connection handle setup req fail, auth invalid.",m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }
    }
    strContentID = m_pMediaLink.ContentID();

    if(NULL == m_pPeerSession)
    {
        m_pPeerSession = CStreamSessionFactory::instance()->findSession(m_strContentID);
    }

    if (NULL == m_pPeerSession)
    {
        CRtspSetupMessage *pSetupReq = new CRtspSetupMessage();
        if (!pSetupReq)  //lint !e774
        {
            return AS_ERROR_CODE_FAIL;
        }
        pSetupReq->setRtspUrl(rtspMessage.getRtspUrl());
        pSetupReq->setCSeq(rtspMessage.getCSeq());
        //pSetupReq->setSession(rtspMessage.getSession());
        std::stringstream sessionIdex;
        sessionIdex << m_unSessionIndex;
        pSetupReq->setSession(sessionIdex.str());
        pSetupReq->setTransType(pReq->getTransType());
        pSetupReq->setInterleaveNum(pReq->getInterleaveNum());
        pSetupReq->setClientPort(pReq->getClientPort());
        pSetupReq->setDestinationIp(pReq->getDestinationIp());

        if (!m_pLastRtspMsg)
        {
            delete m_pLastRtspMsg;
        }
        m_pLastRtspMsg = pSetupReq;

        if (AS_ERROR_CODE_OK != sendMediaSetupReq(&m_pMediaLink))
        {
            delete m_pLastRtspMsg;
            m_pLastRtspMsg = NULL;

            AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe request fail, "
                    "send setup request fail.",
                    m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        AS_LOG(AS_LOG_INFO,"rtsp session[%u] save setup request[%p], send media setup request to SCC.",
                        m_unSessionIndex, m_pLastRtspMsg);
        return AS_ERROR_CODE_OK;
    }


    if (!m_pRtpSession)
    {
        if (AS_ERROR_CODE_OK != createMediaSession())
        {
            AS_LOG(AS_LOG_WARNING,"rtsp connection handle setup request fail, "
                    "create media session fail.",
                    m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        AS_LOG(AS_LOG_INFO,"rtsp connection create media session success.",
                        m_unSessionIndex);
    }

    pReq->setDestinationIp(m_PeerAddr.get_ip_address());

    std::stringstream sessionIdex;
    sessionIdex << m_unSessionIndex;
    pReq->setSession(sessionIdex.str());
    if (AS_ERROR_CODE_OK != m_pRtpSession->startStdRtpSession(*pReq))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection start media session fail.",
                                m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    ACE_INET_Addr addr;
    pReq->setMsgType(RTSP_MSG_RSP);
    pReq->setStatusCode(RTSP_SUCCESS_OK);
    m_unTransType = pReq->getTransType();
    if (TRANS_PROTOCAL_UDP == pReq->getTransType())
    {
        if (m_bFirstSetupFlag)
        {
            m_cVideoInterleaveNum = pReq->getInterleaveNum();
            addr.set(m_pRtpSession->getVideoAddr());
        }
        else
        {
            m_cAudioInterleaveNum = pReq->getInterleaveNum();
            addr.set(m_pRtpSession->getAudioAddr());
        }
        pReq->setServerPort(addr.get_port_number());
        pReq->setSourceIp(addr.get_ip_address());

        m_bFirstSetupFlag = false;
    }
    else
    {
        if (m_bFirstSetupFlag)
        {
            m_cVideoInterleaveNum = pReq->getInterleaveNum();
        }
        else
        {
            m_cAudioInterleaveNum = pReq->getInterleaveNum();
        }
        m_bFirstSetupFlag = false;
    }

    std::string strResp;
    if (AS_ERROR_CODE_OK != pReq->encodeMessage(strResp))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection encode setup response fail.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection send setup response fail.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    setStatus(RTSP_SESSION_STATUS_SETUP);
    AS_LOG(AS_LOG_INFO,"rtsp connection handle setup request success.",
                            m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspRecordReq(mk_rtsp_message &rtspMessage)
{
    if(PLAY_TYPE_AUDIO_LIVE == m_enPlayType)
    {
        AS_LOG(AS_LOG_INFO,"Receive Remote Client Audio Request OK!");
        if (STREAM_SESSION_STATUS_WAIT_CHANNEL_REDAY == m_pRtpSession->getStatus())
        {
            BUSINESS_LIST businessList;
            CStreamBusinessManager::instance()->findBusiness( m_pRtpSession->getStreamId(), businessList);
            for (BUSINESS_LIST_ITER iter = businessList.begin();
                    iter != businessList.end(); iter++)
            {
                CStreamBusiness *pBusiness = *iter;
                if (AS_ERROR_CODE_OK != pBusiness->start())
                {
                    CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
                    AS_LOG(AS_LOG_WARNING,"start distribute fail, stream[%lld] start business fail.",
                                    m_pRtpSession->getStreamId());

                    return AS_ERROR_CODE_FAIL;
                }

                CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
            }

            STREAM_INNER_MSG innerMsg;
            innerMsg.ullStreamID = m_pRtpSession->getStreamId();
            innerMsg.unBodyOffset = sizeof(STREAM_INNER_MSG);
            innerMsg.usMsgType = INNER_MSG_RTSP;

            (void) m_pRtpSession->handleInnerMessage(innerMsg,  sizeof(STREAM_INNER_MSG), *m_pPeerSession);

            sendMediaPlayReq();
        }

        if ((STREAM_SESSION_STATUS_DISPATCHING != m_pPeerSession->getStatus()))
        {
            return cacheRtspMessage(rtspMessage);
        }
        //sendMediaPlayReq();
        sendRtspResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

        setStatus(RTSP_SESSION_STATUS_PLAY);

        clearRtspCachedMessage();

        AS_LOG(AS_LOG_INFO,"rtsp connection handle rtsp play aduio request success.",
                        m_unSessionIndex);
        return  AS_ERROR_CODE_OK;
    }
    if (!m_pRtpSession)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_SESSION_STATUS_SETUP > getStatus())
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle record req fail, status[%u] invalid.",
                    m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspRecordMessage *pReq = dynamic_cast<CRtspRecordMessage*>(&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    sendRtspResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    setStatus(RTSP_SESSION_STATUS_PLAY);

    clearRtspCachedMessage();

    AS_LOG(AS_LOG_INFO,"rtsp connection handle rtsp record request success.",
                        m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspGetParameterReq(mk_rtsp_message &rtspMessage)
{
    sendRtspResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    AS_LOG(AS_LOG_INFO,"rtsp connection send get parameter response success.");

    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspSetParameterReq(mk_rtsp_message &rtspMessage)
{
    sendRtspResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    AS_LOG(AS_LOG_INFO,"rtsp connection send get parameter response success.");

    return AS_ERROR_CODE_OK;
}



int32_t mk_rtsp_connection::handleRtspPlayReq(mk_rtsp_message &rtspMessage)
{
    if (!m_pRtpSession)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_SESSION_STATUS_SETUP > getStatus())
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle play req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspPlayMessage *pReq = dynamic_cast<CRtspPlayMessage*>(&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (!m_pPeerSession)
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle rtsp play request fail, "
                "can't find peer session.",m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (STREAM_SESSION_STATUS_WAIT_CHANNEL_REDAY == m_pRtpSession->getStatus())
    {
        BUSINESS_LIST businessList;
        CStreamBusinessManager::instance()->findBusiness( m_pRtpSession->getStreamId(), businessList);
        for (BUSINESS_LIST_ITER iter = businessList.begin();
                iter != businessList.end(); iter++)
        {
            CStreamBusiness *pBusiness = *iter;
            if (AS_ERROR_CODE_OK != pBusiness->start())
            {
                CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
                AS_LOG(AS_LOG_WARNING,"start distribute fail, stream[%lld] start business fail.",
                                m_pRtpSession->getStreamId());

                return AS_ERROR_CODE_FAIL;
            }

            CStreamBusinessManager::instance()->releaseBusiness(pBusiness);
        }

        STREAM_INNER_MSG innerMsg;
        innerMsg.ullStreamID = m_pRtpSession->getStreamId();
        innerMsg.unBodyOffset = sizeof(STREAM_INNER_MSG);
        innerMsg.usMsgType = INNER_MSG_RTSP;

        (void) m_pRtpSession->handleInnerMessage(innerMsg,  sizeof(STREAM_INNER_MSG), *m_pPeerSession);

        if (!pReq->hasRange())
        {
            MEDIA_RANGE_S   stRange;
            stRange.enRangeType = RANGE_TYPE_NPT;
            stRange.MediaBeginOffset = OFFSET_CUR;
            stRange.MediaEndOffset = OFFSET_END;

            pReq->setRange(stRange);
        }
        else
        {
            pReq->getRange(m_strPlayRange);
        }
    }

    if ((STREAM_SESSION_STATUS_DISPATCHING != m_pPeerSession->getStatus()))
    {
        return cacheRtspMessage(rtspMessage);
    }


    sendRtspResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());


    setStatus(RTSP_SESSION_STATUS_PLAY);

    clearRtspCachedMessage();

    //send the play request
    if(m_bSetUp){
        sendMediaPlayReq();
    }
    else
    {
        /* Key Frame request */
        sendKeyFrameReq();
    }
    AS_LOG(AS_LOG_INFO,"rtsp connection handle rtsp play request success.",
                    m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_connection::handleRtspAnnounceReq(const mk_rtsp_message &rtspMessage)
{
    if (RTSP_SESSION_STATUS_INIT != getStatus())
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle describe req fail, status[%u] invalid.",
                    m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }
    //CSVSMediaLink MediaLink;

    int32_t nRet = CSVSMediaLinkFactory::instance().parseMediaUrl(rtspMessage.getRtspUrl(),&m_pMediaLink);
    if((SVS_MEDIA_LINK_RESULT_SUCCESS != nRet)
        &&(SVS_MEDIA_LINK_RESULT_AUTH_FAIL != nRet))
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle announce req fail, content invalid.");
        return AS_ERROR_CODE_FAIL;
    }
    if(SVS_MEDIA_LINK_RESULT_AUTH_FAIL == nRet)
    {
        if(CStreamConfig::instance()->getUrlEffectiveWhile())
        {
            close();
            AS_LOG(AS_LOG_WARNING,"rtsp connection handle announce req fail, auth invalid.",m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(NULL == m_pPeerSession)
    {
        m_pPeerSession = CStreamSessionFactory::instance()->findSession(m_strContentID);
    }

    if (NULL == m_pPeerSession)
    {
        CRtspAnnounceMessage *pReq = new CRtspAnnounceMessage();
        if (!pReq)  //lint !e774
        {
            return AS_ERROR_CODE_FAIL;
        }
        pReq->setRtspUrl(rtspMessage.getRtspUrl());
        pReq->setCSeq(rtspMessage.getCSeq());
        //pReq->setSession(rtspMessage.getSession());
        std::stringstream sessionIdex;
        sessionIdex << m_unSessionIndex;
        pReq->setSession(sessionIdex.str());
        std::string strContendType = rtspMessage.getContetType();
        std::string strContend = rtspMessage.getBody();
        pReq->setBody(strContendType,strContend);

        if (!m_pLastRtspMsg)
        {
            delete m_pLastRtspMsg;
        }
        m_pLastRtspMsg = pReq;
        AS_LOG(AS_LOG_INFO,"rtsp session[%u] save Announce request[%p].",
                            m_unSessionIndex, m_pLastRtspMsg);

        if (AS_ERROR_CODE_OK != sendMediaSetupReq(&m_pMediaLink))
        {
            delete m_pLastRtspMsg;
            m_pLastRtspMsg = NULL;

            AS_LOG(AS_LOG_WARNING,"rtsp connection handle Announce request fail, "
                        "send setup request fail.",
                        m_unSessionIndex);
            return AS_ERROR_CODE_FAIL;
        }

        return AS_ERROR_CODE_OK;
    }

    std::string strSdp = rtspMessage.getBody();
    if (AS_ERROR_CODE_OK == m_RtspSdp.decodeSdp(strSdp))
    {
        m_RtspSdp.setUrl(rtspMessage.getRtspUrl());
    }

    sendRtspResp(RTSP_SUCCESS_OK, rtspMessage.getCSeq());

    AS_LOG(AS_LOG_INFO,"rtsp connection handle describe request success.");
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_connection::handleRtspPauseReq(mk_rtsp_message &rtspMessage)
{
    if (RTSP_SESSION_STATUS_PLAY != getStatus()
            && RTSP_SESSION_STATUS_PAUSE != getStatus())
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle pause req fail, status[%u] invalid.",
                m_unSessionIndex, getStatus());
        return AS_ERROR_CODE_FAIL;
    }

    CRtspPauseMessage *pReq = dynamic_cast<CRtspPauseMessage*>(&rtspMessage);
    if (!pReq)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (!m_pPeerSession)
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle rtsp play request fail, "
                "can't find peer session.",m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    std::string strRtsp;
    (void)pReq->encodeMessage(strRtsp);

    mk_rtsp_packet rtspPack;
    if (0 != rtspPack.parse(strRtsp.c_str(), strRtsp.length()))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle rtsp pause request fail, "
                        "parse rtsp packet fail.",
                        m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != m_pPeerSession->sendVcrMessage(rtspPack))
    {
        sendRtspResp(RTSP_SERVER_INTERNAL, rtspMessage.getCSeq());
        AS_LOG(AS_LOG_WARNING,"rtsp connection handle rtsp pause request fail, "
                "peer session send vcr message fail.",
                m_unSessionIndex);
        return AS_ERROR_CODE_FAIL;
    }

    setStatus(RTSP_SESSION_STATUS_PAUSE);
    AS_LOG(AS_LOG_INFO,"rtsp connection handle rtsp pause request success.",
                     m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}

// TEARDOWN
int32_t mk_rtsp_connection::handleRtspTeardownReq(mk_rtsp_message &rtspMessage)
{
    rtspMessage.setMsgType(RTSP_MSG_RSP);
    rtspMessage.setStatusCode(RTSP_SUCCESS_OK);

    std::string strRtsp;
    if (AS_ERROR_CODE_OK != rtspMessage.encodeMessage(strRtsp))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection encode teardown response fail.");
    }

    if (AS_ERROR_CODE_OK != sendMessage(strRtsp.c_str(), strRtsp.length()))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection send teardown response fail.");
    }

    //close the session
    (void)handle_close(m_sockHandle, 0);
    setStatus(RTSP_SESSION_STATUS_TEARDOWN);

    AS_LOG(AS_LOG_INFO,"rtsp connection handle rtsp teardown request success.",
                     m_unSessionIndex);
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspRedirect(mk_rtsp_packet &rtspMessage)
{
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_connection::sendRtspResp(uint32_t unStatusCode, uint32_t unCseq)
{
    std::string strResp;
    mk_rtsp_message::encodeCommonResp(unStatusCode, unCseq, strResp);

    if (AS_ERROR_CODE_OK != sendMessage(strResp.c_str(), strResp.length()))
    {
        AS_LOG(AS_LOG_WARNING,"rtsp connection send common response fail.");
    }
    else
    {
        AS_LOG(AS_LOG_INFO,"rtsp connection send common response success.");
    }

    AS_LOG(AS_LOG_DEBUG,"%s", strResp.c_str());
    return;
}
void mk_rtsp_connection::resetRtspConnect()
{
    as_init_url(&m_url);
    m_url.port        = RTSP_DEFAULT_PORT;
    m_RecvBuf         = NULL;
    m_ulRecvSize      = 0;
    m_ulSeq           = 0;
    m_Status          = RTSP_SESSION_STATUS_INIT;
    m_bSetupTcp       = false;
    m_rtpHandles[VIDEO_RTP_HANDLE]   = NULL;
    m_rtpHandles[AUDIO_RTP_HANDLE]   = NULL;
    m_rtcpHandles[VIDEO_RTCP_HANDLE] = NULL;
    m_rtcpHandles[AUDIO_RTCP_HANDLE] = NULL;

}
void mk_rtsp_connection::trimString(std::string& srcString) const
{
    string::size_type pos = srcString.find_last_not_of(' ');
    if (pos != string::npos)
    {
        (void) srcString.erase(pos + 1);
        pos = srcString.find_first_not_of(' ');
        if (pos != string::npos)
            (void) srcString.erase(0, pos);
    }
    else
        (void) srcString.erase(srcString.begin(), srcString.end());

    return;
}


/*******************************************************************************************************
 * 
 * 
 *
 * ****************************************************************************************************/

mk_rtsp_server::mk_rtsp_server()
{
    m_cb  = NULL;
    m_ctx = NULL;
}

mk_rtsp_server::~mk_rtsp_server()
{
}
void mk_rtsp_server::set_callback(rtsp_server_request cb,void* ctx)
{
    m_cb = cb;
    m_ctx = ctx;
}
long mk_rtsp_server::handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle)
{
    
}