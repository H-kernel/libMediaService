
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
    return 
}

void  mk_rtsp_connection::set_rtp_over_tcp()
{
    m_bSetupTcp = true;
    return 
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

void mk_rtsp_connection::handleRtpFrame(RTP_PACK_QUEUE &rtpFrameList)
{

    mk_rtp_packet rtpPacket;
    if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, len))
    {
        MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
        return ;
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
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspDescribeReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspDescribeMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspSetupReq(SDP_MEDIA_INFO& info)
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspDescribeMethod);
    rtspPacket.setCseq(m_ulSeq++);
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

        if(AS_ERROR_CODE_OK != mk_rtsp_service::instance().get_rtp_rtcp_pair(pRtpHandle,pRtcpHandle)) {
            MK_LOG(AS_LOG_ERROR,"rtsp connection client get rtp and rtcp handle for udp fail.");
            return AS_ERROR_CODE_FAIL;
        }

        if(MEDIA_TYPE_VALUE_VIDEO == info.ucMediaType) {
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]  = pRtcpHandle;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info.ucMediaType){
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]  = pRtcpHandle;
        }
        else {
            MK_LOG(AS_LOG_ERROR,"rtsp connection client ,the media type is not rtp and rtcp.");
            return AS_ERROR_CODE_FAIL;
        }

        pRtpHandle->open(this);
        pRtcpHandle->open(this);

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
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspPlayReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspPlayMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspRecordReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspRecordMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspGetParameterReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspGetParameterMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspAnnounceReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspAnnounceMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspPauseReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspPauseMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspTeardownReq()
{
    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(RtspTeardownMethod);
    rtspPacket.setCseq(m_ulSeq++);
    rtspPacket.setRtspUrl(&m_url.uri[0]);
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        return AS_ERROR_CODE_FAIL;
    }
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::handleRtspResp(mk_rtsp_packet &rtspMessage)
{
    uint32_t nCseq     = rtspMessage.getCseq();
    uint32_t nRespCode = rtspMessage.getRtspStatusCode();
    enRtspMethods enMethod   = RtspIllegalMethod;

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
void mk_rtsp_connection::resetRtspConnect()
{
    as_init_url(&m_url);
    m_url.port        = RTSP_DEFAULT_PORT;
    m_RecvBuf         = NULL;
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