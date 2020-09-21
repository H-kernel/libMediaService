
#include <sstream>
#include "mk_rtsp_connection.h"
#include "mk_rtsp_packet.h"
#include "mk_media_service.h"
#include "mk_rtsp_service.h"
#include <arpa/inet.h>
#include "mk_media_common.h"
#include "mk_rtsp_rtp_packet.h"

mk_rtsp_connection::mk_rtsp_connection()
{
    m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]    = NULL;
    m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]   = NULL;
    m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]    = NULL;
    m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]   = NULL;
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
    as_network_svr* pNetWork = mk_media_service::instance().get_client_network_svr(this->get_index());
    struct in_addr sin_addr;    /* AF_INET */
    inet_aton((char*)&m_url.host[0],&sin_addr);

    remote.m_ulIpAddr = sin_addr.s_addr;
    if(0 == m_url.port) {
        remote.m_usPort   = RTSP_DEFAULT_PORT;
    }
    else {
        remote.m_usPort   = m_url.port;
    }
    
    if(AS_ERROR_CODE_OK != pNetWork->regTcpClient(&remote,&remote,this,enSyncOp,5)) {
        return AS_ERROR_CODE_FAIL;
    }
    handle_connection_status(MR_CLIENT_STATUS_CONNECTED);

    if(AS_ERROR_CODE_OK != this->sendRtspOptionsReq()) {
        MK_LOG(AS_LOG_WARNING,"options:rtsp client send message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    setHandleRecv(AS_TRUE);
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_connection::stop()
{
    resetRtspConnect();
    setHandleRecv(AS_FALSE);
    MK_LOG(AS_LOG_INFO,"close rtsp client.");
    return;
}
int32_t mk_rtsp_connection::recv_next()
{
    m_bDoNextRecv = AS_TRUE;
    return AS_ERROR_CODE_OK;
}

void  mk_rtsp_connection::set_rtp_over_tcp()
{
    m_bSetupTcp = true;
    return;
}

void mk_rtsp_connection::handle_recv(void)
{
    as_network_addr peer;
    int32_t iRecvLen = (int32_t) (MAX_BYTES_PER_RECEIVE -  m_ulRecvSize);
    if (iRecvLen <= 0)
    {
        MK_LOG(AS_LOG_INFO,"rtsp connection,recv buffer is full, size[%u] length[%u].",
                MAX_BYTES_PER_RECEIVE,
                m_ulRecvSize);
        return;
    }

    iRecvLen = this->recv((char*)&m_RecvBuf[iRecvLen],&peer,iRecvLen,enAsyncOp);
    if (iRecvLen <= 0)
    {
        MK_LOG(AS_LOG_INFO,"rtsp connection recv data fail.");
        return;
    }

    m_ulRecvSize += iRecvLen;

    uint32_t processedSize = 0;
    uint32_t totalSize = m_ulRecvSize;
    int32_t nSize = 0;
    do
    {
        nSize = processRecvedMessage((const char*)&m_RecvBuf[processedSize],
                                     m_ulRecvSize - processedSize);
        if (nSize < 0) {
            MK_LOG(AS_LOG_WARNING,"tsp connection process recv data fail, close handle. ");
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
int32_t mk_rtsp_connection::handle_rtp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len) 
{
    if(MK_RTSP_UDP_VIDEO_RTP_HANDLE == type) {
        return m_rtpFrameOrganizer.insertRtpPacket(pData,len);
    }
    else if(MK_RTSP_UDP_VIDEO_RTP_HANDLE == type) {
        if(m_ulRecvBufLen < len) {
            return AS_ERROR_CODE_FAIL;/* drop it */
        }

        mk_rtp_packet rtpPacket;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, len))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_FAIL;
        }
        MR_MEDIA_TYPE enType = MR_MEDIA_TYPE_INVALID;
        if(RTSP_PT_TYPE_PCMU == rtpPacket.GetPayloadType()) {
            enType = MR_MEDIA_TYPE_G711U;
        }
        else if(RTSP_PT_TYPE_PCMA == rtpPacket.GetPayloadType()) {
            enType = MR_MEDIA_TYPE_G711A;
        }
        else {
            return AS_ERROR_CODE_FAIL;/* drop it */
        }
        /* send direct */
        memcpy(m_recvBuf,pData,len);
        m_ulRecvLen = len;
        handle_connection_media(enType,rtpPacket.GetTimeStamp());
    }
    else {
        return AS_ERROR_CODE_FAIL;
    }
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handle_rtcp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len)
{
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_connection::handleRtpFrame(uint8_t PayloadType,RTP_PACK_QUEUE &rtpFrameList)
{
    if(m_ucH264PayloadType == PayloadType) {
        handleH264Frame(rtpFrameList);
    }
    else if(m_ucH265PayloadType == PayloadType) {
        handleH265Frame(rtpFrameList);
    }
    else{
        handleOtherFrame(PayloadType,rtpFrameList);
    }
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
        MK_LOG(AS_LOG_WARNING,"rtsp connection check rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    if(0 == ulMsgLen) {
        return 0; /* need more data deal */
    }

    nRet = rtspPacket.parse(pData,unDataSize);
    if (AS_ERROR_CODE_OK != nRet)
    {
        MK_LOG(AS_LOG_WARNING,"rtsp connection parser rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    switch (rtspPacket.getMethodIndex())
    {
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
        MK_LOG(AS_LOG_WARNING,"rtsp connection process rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    MK_LOG(AS_LOG_INFO,"rtsp connection success to process rtsp message.");
    return ulMsgLen;
}

int32_t mk_rtsp_connection::handleRTPRTCPData(const char* pData, uint32_t unDataSize)
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

    if(unDataSize > RTP_RECV_BUF_SIZE ) {
        return AS_ERROR_CODE_FAIL;
    }

    char* buf = mk_rtsp_service::instance().get_rtp_recv_buf();
    if(NULL == buf) {
        /* drop it */
        return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN);
    }

    memcpy(buf,&pData[RTSP_INTERLEAVE_HEADER_LEN],unMediaSize);

    int32_t nRet = AS_ERROR_CODE_OK;

    if (RTSP_INTERLEAVE_NUM_VIDEO_RTP == pData[1]) {
        nRet = this->handle_rtp_packet(MK_RTSP_UDP_VIDEO_RTP_HANDLE,buf,unMediaSize);
    }
    else if(RTSP_INTERLEAVE_NUM_AUDIO_RTP == pData[1]) {
        nRet = this->handle_rtp_packet(MK_RTSP_UDP_AUDIO_RTP_HANDLE,buf,unMediaSize);
    }
    else if (RTSP_INTERLEAVE_NUM_VIDEO_RTCP == pData[1]) {
        nRet = this->handle_rtcp_packet(MK_RTSP_UDP_VIDEO_RTCP_HANDLE,buf,unMediaSize);
    }
    else if(RTSP_INTERLEAVE_NUM_AUDIO_RTCP == pData[1]) {
        nRet = this->handle_rtcp_packet(MK_RTSP_UDP_AUDIO_RTCP_HANDLE,buf,unMediaSize);
    }
    else {
        mk_rtsp_service::instance().free_rtp_recv_buf(buf);
    }

    if(AS_ERROR_CODE_OK != nRet) {
        mk_rtsp_service::instance().free_rtp_recv_buf(buf);
    }

    return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN);
}

int32_t mk_rtsp_connection::sendRtspOptionsReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspOptionsMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspOptionsMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspDescribeReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspDescribeMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspDescribeMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspSetupReq(SDP_MEDIA_INFO& info)
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspSetupMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);

    std::string strTransport = "";

    // (RTP)
    strTransport += RTSP_TRANSPORT_RTP;
    strTransport += RTSP_TRANSPORT_SPEC_SPLITER;
    strTransport += RTSP_TRANSPORT_PROFILE_AVP;

    //(TCP/UDP)
    if (m_bSetupTcp)
    {
        strTransport += RTSP_TRANSPORT_SPEC_SPLITER;
        strTransport += RTSP_TRANSPORT_TCP;
    }
    strTransport += SIGN_SEMICOLON;

    if (m_bSetupTcp)
    {
        strTransport += RTSP_TRANSPORT_UNICAST;
        strTransport += SIGN_SEMICOLON;
        strTransport += RTSP_TRANSPORT_INTERLEAVED;
        std::stringstream strChannelNo;
        if(MEDIA_TYPE_VALUE_VIDEO == info.ucMediaType) {
            strChannelNo << RTSP_INTERLEAVE_NUM_VIDEO_RTP;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info.ucMediaType){
            strChannelNo << RTSP_INTERLEAVE_NUM_AUDIO_RTP;
        }

        strTransport += strChannelNo.str() + SIGN_H_LINE;

        strChannelNo.str("");
        if(MEDIA_TYPE_VALUE_VIDEO == info.ucMediaType) {
            strChannelNo << RTSP_INTERLEAVE_NUM_VIDEO_RTCP;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info.ucMediaType){
            strChannelNo << RTSP_INTERLEAVE_NUM_AUDIO_RTCP + 1;
        }
        strTransport += strChannelNo.str();
    }
    else
    {
        strTransport += RTSP_TRANSPORT_UNICAST;
        strTransport += SIGN_SEMICOLON;

        mk_rtsp_udp_handle*  pRtpHandle  = NULL;
        mk_rtsp_udp_handle* pRtcpHandle = NULL;
        MK_RTSP_HANDLE_TYPE rtpType = MK_RTSP_UDP_TYPE_MAX;
        MK_RTSP_HANDLE_TYPE rtcpType = MK_RTSP_UDP_TYPE_MAX;

        if(AS_ERROR_CODE_OK != mk_rtsp_service::instance().get_rtp_rtcp_pair(pRtpHandle,pRtcpHandle)) {
            MK_LOG(AS_LOG_ERROR,"rtsp connection client get rtp and rtcp handle for udp fail.");
            return AS_ERROR_CODE_FAIL;
        }

        if(MEDIA_TYPE_VALUE_VIDEO == info.ucMediaType) {
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]  = pRtcpHandle;
            rtpType = MK_RTSP_UDP_VIDEO_RTP_HANDLE;
            rtcpType = MK_RTSP_UDP_VIDEO_RTCP_HANDLE;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info.ucMediaType){
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]  = pRtcpHandle;
            rtpType = MK_RTSP_UDP_AUDIO_RTP_HANDLE;
            rtcpType = MK_RTSP_UDP_AUDIO_RTCP_HANDLE;
        }
        else {
            MK_LOG(AS_LOG_ERROR,"rtsp connection client ,the media type is not rtp and rtcp.");
            return AS_ERROR_CODE_FAIL;
        }

        if(AS_ERROR_CODE_OK != pRtpHandle->open(rtpType,this)) {
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != pRtcpHandle->open(rtpType,this)) {
            return AS_ERROR_CODE_FAIL;
        }

        strTransport += RTSP_TRANSPORT_CLIENT_PORT;
        std::stringstream strPort;
        strPort << pRtpHandle->get_port();
        strTransport += strPort.str() + SIGN_H_LINE;
        strPort.str("");
        strPort << pRtcpHandle->get_port();
        strTransport += strPort.str();
    }
    rtspPacket.setTransPort(strTransport);
    
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspSetupMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspPlayReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspPlayMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspPlayMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspRecordReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspRecordMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspRecordMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspGetParameterReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspGetParameterMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspGetParameterMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspAnnounceReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspAnnounceMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspAnnounceMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspPauseReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspPauseMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspPauseMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspTeardownReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspTeardownMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,RtspTeardownMethod));
    m_ulSeq++;
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::handleRtspResp(mk_rtsp_packet &rtspMessage)
{
    uint32_t nCseq     = rtspMessage.getCseq();
    uint32_t nRespCode = rtspMessage.getRtspStatusCode();
    uint32_t enMethod   = RtspIllegalMethod;

    SEQ_METHOD_ITER iter = m_SeqMethodMap.find(nCseq);
    if(iter == m_SeqMethodMap.end()) {
        return AS_ERROR_CODE_FAIL;
    }

    enMethod = iter->second;
    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse seq:[%d] ,mothod:[%d].",nCseq,enMethod);

    if(RtspStatus_200 != nRespCode) {
        /*close socket */
        MK_LOG(AS_LOG_WARNING,"rtsp client there server reponse code:[%d].",nRespCode);
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
            handle_connection_status(MR_CLIENT_STATUS_TEARDOWN);
            break;
        }
        case RtspPlayMethod:
        {
            //start rtcp and media check timer
            m_Status = RTSP_SESSION_STATUS_PLAY;
            handle_connection_status(MR_CLIENT_STATUS_RUNNING);
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
    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse end.");
    return nRet;
}

int32_t mk_rtsp_connection::handleRtspDescribeResp(mk_rtsp_packet &rtspMessage)
{
    std::string strSdp = "";
    rtspMessage.getContent(strSdp);

    MK_LOG(AS_LOG_INFO,"rtsp client connection handle describe response sdp:[%s].",strSdp.c_str());

    int32_t nRet = m_sdpInfo.decodeSdp(strSdp);
    if(AS_ERROR_CODE_OK != nRet) {
        MK_LOG(AS_LOG_WARNING,"rtsp connection handle describe sdp:[%s] fail.",strSdp.c_str());
        return AS_ERROR_CODE_FAIL;
    }
    std::string rtmMap = H264_VIDEO_RTPMAP;
    m_ucH264PayloadType = m_sdpInfo.getPayloadTypeByRtpmap(rtmMap);
    rtmMap = H265_VIDEO_RTPMAP;
    m_ucH265PayloadType = m_sdpInfo.getPayloadTypeByRtpmap(rtmMap);; 

    m_mediaInfoList.clear();
    m_sdpInfo.getVideoInfo(m_mediaInfoList);
    m_sdpInfo.getAudioInfo(m_mediaInfoList);

    if(0 == m_mediaInfoList.size()) {
        MK_LOG(AS_LOG_WARNING,"rtsp connection handle describe response fail,there is no media info.");
        return AS_ERROR_CODE_FAIL;
    }

    SDP_MEDIA_INFO& info = m_mediaInfoList.front();
    nRet = sendRtspSetupReq(info);
    if(AS_ERROR_CODE_OK != nRet) {
        MK_LOG(AS_LOG_WARNING,"rtsp connection handle describe response,send setup fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_mediaInfoList.pop_front();
    
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle describe response end.");
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handleRtspSetUpResp(mk_rtsp_packet &rtspMessage)
{
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response begin.");
    int32_t nRet = AS_ERROR_CODE_OK;
    if(0 < m_mediaInfoList.size()) {
        MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,send next setup messgae.");
        SDP_MEDIA_INFO& info = m_mediaInfoList.front();
        nRet = sendRtspSetupReq(info);
        if(AS_ERROR_CODE_OK != nRet) {
            MK_LOG(AS_LOG_WARNING,"rtsp connection handle setup response,send next setup fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_mediaInfoList.pop_front();
    }
    else {
        m_Status = RTSP_SESSION_STATUS_SETUP;
        handle_connection_status(MR_CLIENT_STATUS_HANDSHAKE);
        /* send play */
        MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,send play messgae.");
        nRet = sendRtspPlayReq();
    }
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response end.");
    return nRet;
}
int32_t mk_rtsp_connection::sendMsg(const char* pszData,uint32_t len)
{
    return AS_ERROR_CODE_OK;
}
void mk_rtsp_connection::handleH264Frame(RTP_PACK_QUEUE &rtpFrameList)
{
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char*         pData;
    uint32_t      DataLen;
    uint32_t      TimeStam;
    H264_NALU_HEADER  *nalu_hdr = NULL;
    FU_INDICATOR  *fu_ind   = NULL;
    FU_HEADER     *fu_hdr   = NULL;
    if(0 == rtpFrameList.size()) {
        return;
    }
    m_recvBuf[0] = 0x00;
    m_recvBuf[1] = 0x00;
    m_recvBuf[2] = 0x00;
    m_recvBuf[3] = 0x01;
    m_ulRecvLen  = 4;

    if(1 == rtpFrameList.size()) {  
        pData =  rtpFrameList[0].pRtpMsgBlock;
        DataLen = rtpFrameList[0].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return ;
        }
        
        rtpHeadLen   = rtpPacket.GetHeadLen();
        TimeStam     = rtpPacket.GetTimeStamp();
        rtpPayloadLen = DataLen - rtpHeadLen;
        memcpy(&m_recvBuf[m_ulRecvLen],&pData[rtpHeadLen],rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
        handle_connection_media(MR_MEDIA_TYPE_H264,TimeStam);
        return;
    }
    
    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return ;
        }

        pData = &pData[rtpHeadLen];
        rtpPayloadLen = DataLen - rtpHeadLen;

        fu_ind = (FU_INDICATOR*)pData;    pData++;
        fu_hdr = (FU_HEADER*)pData; pData++;
        rtpPayloadLen -= 2;
        if(0 == i) {
            /* first packet */
            nalu_hdr = (H264_NALU_HEADER*)&m_recvBuf[m_ulRecvLen];
            nalu_hdr->TYPE = fu_hdr->TYPE;
            nalu_hdr->F    = fu_ind->F;
            nalu_hdr->NRI  = fu_ind->NRI;
            m_ulRecvLen++; /* 1 byte */
            TimeStam     = rtpPacket.GetTimeStamp();
        }
        memcpy(&m_recvBuf[m_ulRecvLen],pData,rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    handle_connection_media(MR_MEDIA_TYPE_H264,TimeStam);
}
void mk_rtsp_connection::handleH265Frame(RTP_PACK_QUEUE &rtpFrameList)
{
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char*         pData;
    uint32_t      DataLen;
    uint32_t      TimeStam;
    uint32_t      fu_type;
    if(0 == rtpFrameList.size()) {
        return;
    }
    m_recvBuf[0] = 0x00;
    m_recvBuf[1] = 0x00;
    m_recvBuf[2] = 0x00;
    m_recvBuf[3] = 0x01;
    m_ulRecvLen  = 4;

    if(1 == rtpFrameList.size()) {  
        pData =  rtpFrameList[0].pRtpMsgBlock;
        DataLen = rtpFrameList[0].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return ;
        }
        
        rtpHeadLen   = rtpPacket.GetHeadLen();
        TimeStam     = rtpPacket.GetTimeStamp();
        rtpPayloadLen = DataLen - rtpHeadLen;
        memcpy(&m_recvBuf[m_ulRecvLen],&pData[rtpHeadLen],rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
        handle_connection_media(MR_MEDIA_TYPE_H265,TimeStam);
        return;
    }
    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return ;
        }

        pData = &pData[rtpHeadLen];
        rtpPayloadLen = DataLen - rtpHeadLen;
        fu_type = pData[3] & 0x3f;        
        if(0 == i) {
            /* first packet */
            m_recvBuf[m_ulRecvLen] = (pData[0] & 0x81) | (fu_type << 1);
            m_ulRecvLen++;
            m_recvBuf[m_ulRecvLen] = pData[1];
            m_ulRecvLen++;
            TimeStam     = rtpPacket.GetTimeStamp();
        }
        pData += 3;
        rtpPayloadLen -= 3;

        memcpy(&m_recvBuf[m_ulRecvLen],pData,rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    handle_connection_media(MR_MEDIA_TYPE_H265,TimeStam);
}
void mk_rtsp_connection::handleOtherFrame(uint8_t PayloadType,RTP_PACK_QUEUE &rtpFrameList)
{
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char*         pData;
    uint32_t      DataLen;
    uint32_t      TimeStam;
    MR_MEDIA_TYPE type = MR_MEDIA_TYPE_INVALID;
    if(0 == rtpFrameList.size()) {
        return;
    }

    if(PT_TYPE_PCMU == PayloadType) {
        type = MR_MEDIA_TYPE_G711U;
    }
    else if(PT_TYPE_PCMA == PayloadType) {
        type = MR_MEDIA_TYPE_G711A;
    }
    else {
        return;
    }

    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return ;
        }

        pData = &pData[rtpHeadLen];
        rtpPayloadLen = DataLen - rtpHeadLen;

        memcpy(&m_recvBuf[m_ulRecvLen],pData,rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    handle_connection_media(type,TimeStam);
}
void mk_rtsp_connection::resetRtspConnect()
{
    as_init_url(&m_url);
    m_url.port        = RTSP_DEFAULT_PORT;
    m_ulRecvSize      = 0;
    m_ulSeq           = 0;
    m_Status          = RTSP_SESSION_STATUS_INIT;
    m_bSetupTcp       = false;
    if(NULL != m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]) {
        mk_rtsp_service::instance().free_rtp_rtcp_pair(m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]);
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]    = NULL;
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]   = NULL;
    }

    if(NULL != m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]) {
        mk_rtsp_service::instance().free_rtp_rtcp_pair(m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]);
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]    = NULL;
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]   = NULL;
    }
    m_rtpFrameOrganizer.release();
    m_rtpFrameOrganizer.init(this);

    m_ucH264PayloadType = PT_TYPE_MAX;
    m_ucH265PayloadType = PT_TYPE_MAX;    
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
int32_t mk_rtsp_server::handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle)
{
    return AS_ERROR_CODE_OK;
}