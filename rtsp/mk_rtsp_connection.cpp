
#include <sstream>
#include "mk_rtsp_connection.h"
#include "mk_rtsp_packet.h"
#include "mk_media_service.h"
#include "mk_rtsp_service.h"
#include "mk_media_common.h"
#include "mk_rtsp_rtp_packet.h"
#if AS_APP_OS == AS_OS_LINUX
#include <arpa/inet.h>
#elif AS_APP_OS == AS_OS_WIN32
#include <Ws2tcpip.h>
#endif

mk_rtsp_connection::mk_rtsp_connection()
{
    m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]    = NULL;
    m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]   = NULL;
    m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]    = NULL;
    m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]   = NULL;

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

    m_rtpFrameOrganizer.init(this);

    m_ucH264PayloadType = PT_TYPE_MAX;
    m_ucH265PayloadType = PT_TYPE_MAX;

    m_ulLastRecv = time(NULL);
    m_ulAuthenTime = 0;
    m_strAuthenticate = "";
    m_strSdpInfo = "";
}

mk_rtsp_connection::~mk_rtsp_connection()
{
}


int32_t mk_rtsp_connection::start()
{
    as_network_addr local;
    as_network_addr remote;
    if (AS_ERROR_CODE_OK != as_digest_init(&m_Authen,1)) {
        return AS_ERROR_CODE_FAIL;
    }
    MK_LOG(AS_LOG_INFO,"rtsp client connect url [%s] ",m_strurl.c_str());

    if(AS_ERROR_CODE_OK != as_parse_url(m_strurl.c_str(),&m_url)) {
        return AS_ERROR_CODE_FAIL;
    }
    as_network_svr* pNetWork = mk_media_service::instance().get_client_network_svr(this->get_index());
    struct in_addr sin_addr;    /* AF_INET */
#if AS_APP_OS == AS_OS_LINUX
	inet_aton((char*)&m_url.host[0],&sin_addr);
#elif AS_APP_OS == AS_OS_WIN32
	inet_pton(AF_INET, (char*)&m_url.host[0], &sin_addr);
#endif
    
	

    remote.m_ulIpAddr = sin_addr.s_addr;
    if(0 == m_url.port) {
        remote.m_usPort   = htons(RTSP_DEFAULT_PORT);
    }
    else {
        remote.m_usPort   = htons(m_url.port);
    }

    MK_LOG(AS_LOG_INFO,"rtsp client connect to [%s:%d] ,uri:[%s][%d],args[%s]",
        (char*)&m_url.host[0],m_url.port,(char*)&m_url.uri[0],AS_URL_MAX_LEN,(char*)&m_url.args[0]);
    MK_LOG(AS_LOG_INFO,"rtsp client usrname:[%s] password:[%s].",(char*)&m_url.username[0],(char*)&m_url.password[0]);
    
    if(AS_ERROR_CODE_OK != pNetWork->regTcpClient(&local,&remote,this,enSyncOp,5)) {
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
    //resetRtspConnect();
    setHandleRecv(AS_FALSE);
    /* unregister the network service */
    as_network_svr* pNetWork = mk_media_service::instance().get_client_network_svr(this->get_index());
    pNetWork->removeTcpClient(this);
    handle_connection_status(MR_CLIENT_STATUS_TEARDOWN);
    MK_LOG(AS_LOG_INFO,"close rtsp client.");
    return;
}
int32_t mk_rtsp_connection::recv_next()
{
    m_bDoNextRecv = AS_TRUE;
    return AS_ERROR_CODE_OK;
}
void  mk_rtsp_connection::check_client()
{
    time_t cur = time(NULL);
    if(MK_CLIENT_RECV_TIMEOUT < (cur - m_ulLastRecv)) {
        handle_connection_status(MR_CLIENT_STATUS_TIMEOUT);
    }
    return;
}

void  mk_rtsp_connection::set_rtp_over_tcp()
{
    m_bSetupTcp = true;
    return;
}
void  mk_rtsp_connection::get_rtp_stat_info(RTP_PACKET_STAT_INFO &statinfo)
{
    m_rtpFrameOrganizer.getRtpPacketStatInfo(statinfo.ulTotalPackNum,statinfo.ulLostRtpPacketNum,statinfo.ulLostFrameNum,statinfo.ulDisorderSeqCounts);
    return;
}
void  mk_rtsp_connection::get_rtsp_sdp_info(char* info)
{
    if(m_strSdpInfo.length() > 0)
    {
        memcpy(info,m_strSdpInfo.c_str(),m_strSdpInfo.length());
    }
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

    iRecvLen = this->recv((char*)&m_RecvTcpBuf[m_ulRecvSize],&peer,iRecvLen,enAsyncOp);
    if (iRecvLen <= 0)
    {
        //stop_recv();
        handle_connection_status(MR_CLIENT_STATUS_TEARDOWN);
        MK_LOG(AS_LOG_WARNING,"rtsp connection recv data fail.");
        return;
    }

    m_ulRecvSize += iRecvLen;

    uint32_t processedSize = 0;
    uint32_t totalSize = m_ulRecvSize;
    int32_t nSize = 0;
    do
    {
        nSize = processRecvedMessage((const char*)&m_RecvTcpBuf[processedSize],
                                     m_ulRecvSize - processedSize);
        if (nSize < 0) {
            MK_LOG(AS_LOG_WARNING,"rtsp connection process recv data fail, close handle. ");
            handle_connection_status(MR_CLIENT_STATUS_TEARDOWN);
            //stop_recv();
            return;
        }

        if (0 == nSize) {
            break;
        }
        processedSize += (uint32_t) nSize;
    }while (processedSize < totalSize);

    uint32_t dataSize = m_ulRecvSize - processedSize;
    if(0 < dataSize) {
        memmove(&m_RecvTcpBuf[0],&m_RecvTcpBuf[processedSize], dataSize);
    }
    m_ulRecvSize = dataSize;
    setHandleRecv(AS_TRUE);
    m_ulLastRecv = time(NULL);

    return;
}
void mk_rtsp_connection::handle_send(void)
{
    setHandleSend(AS_FALSE);
}
int32_t mk_rtsp_connection::handle_rtp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len) 
{
    MR_MEDIA_CODE enCode = MR_MEDIA_CODE_MAX;

    if(MK_RTSP_UDP_VIDEO_RTP_HANDLE == type) {
        mk_rtp_packet rtpPacket;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, len))
        {
            MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_FAIL;
        }

        m_rtpFrameOrganizer.updateTotalRtpPacketNum();

        if(m_ucH264PayloadType == rtpPacket.GetPayloadType()) {
            FU_HEADER* pFu_hdr = (FU_HEADER*)&pData[rtpPacket.GetHeadLen()];
            if(RTP_H264_NALU_TYPE_FU_A != pFu_hdr->TYPE) {    
                uint32_t rtpHeadLen   = rtpPacket.GetHeadLen();
                uint32_t TimeStam     = rtpPacket.GetTimeStamp();
                uint32_t rtpPayloadLen = len - rtpHeadLen - rtpPacket.GetTailLen();

                uint32_t  bufflen;
                char* recBuf = handle_connection_databuf(rtpPayloadLen,bufflen);
                if(NULL == recBuf)
                {
                    MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
                    return AS_ERROR_CODE_FAIL;
                }

                recBuf[0] = 0x00;
                recBuf[1] = 0x00;
                recBuf[2] = 0x00;
                recBuf[3] = 0x01;

                uint32_t m_ulRecvLen  = 4;

                memcpy(&recBuf[m_ulRecvLen],&pData[rtpHeadLen],rtpPayloadLen);
                m_ulRecvLen += rtpPayloadLen;
                enCode = MR_MEDIA_CODE_OK;
                m_rtpFrameOrganizer.updateLastRtpSeq(rtpPacket.GetSeqNum(),false);

                MEDIA_DATA_INFO dataInfo;
                memset(&dataInfo,0x0,sizeof(dataInfo));
                parse_media_info(MR_MEDIA_TYPE_H264,TimeStam,recBuf,dataInfo);

                handle_connection_media(dataInfo,m_ulRecvLen);
                mk_rtsp_service::instance().free_rtp_recv_buf(pData);
                return AS_ERROR_CODE_OK;
            }
        }
        else if(m_ucH265PayloadType == rtpPacket.GetPayloadType()) {
            H265_NALU_HEADER* pNalu_hdr = (H265_NALU_HEADER*)&pData[rtpPacket.GetHeadLen()];
            if(49 != pNalu_hdr->TYPE) {     
                uint32_t rtpHeadLen   = rtpPacket.GetHeadLen();
                uint32_t TimeStam     = rtpPacket.GetTimeStamp();
                uint32_t rtpPayloadLen = len - rtpHeadLen - rtpPacket.GetTailLen();

                uint32_t  bufflen;
                char* recBuf = handle_connection_databuf(rtpPayloadLen,bufflen);
                if(NULL == recBuf)
                {
                    MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
                    return AS_ERROR_CODE_FAIL;
                }

                recBuf[0] = 0x00;
                recBuf[1] = 0x00;
                recBuf[2] = 0x00;
                recBuf[3] = 0x01;
                uint32_t m_ulRecvLen  = 4;

                memcpy(&recBuf[m_ulRecvLen],&pData[rtpHeadLen],rtpPayloadLen);
                m_ulRecvLen += rtpPayloadLen;
                enCode = MR_MEDIA_CODE_OK;
                m_rtpFrameOrganizer.updateLastRtpSeq(rtpPacket.GetSeqNum(),false);

                MEDIA_DATA_INFO dataInfo;
                memset(&dataInfo,0x0,sizeof(dataInfo));
                parse_media_info(MR_MEDIA_TYPE_H265,TimeStam,recBuf,dataInfo);

                handle_connection_media(dataInfo,m_ulRecvLen);
                mk_rtsp_service::instance().free_rtp_recv_buf(pData);
                return AS_ERROR_CODE_OK;
            }
        }
        return m_rtpFrameOrganizer.insertRtpPacket(pData,len);
    }
    else if(MK_RTSP_UDP_AUDIO_RTP_HANDLE == type) {
        mk_rtp_packet rtpPacket;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, len))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_FAIL;
        }

        //m_rtpFrameOrganizer.updateLastRtpSeq(rtpPacket.GetSeqNum(),false);

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
        uint32_t ulRtpHeadLen = rtpPacket.GetHeadLen();
        uint32_t ulAudioLen = len - ulRtpHeadLen- rtpPacket.GetTailLen();

        /* send direct */

        uint32_t  bufflen;
        char* recBuf = handle_connection_databuf(ulAudioLen,bufflen);
        if(NULL == recBuf)
        {
            MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
            return AS_ERROR_CODE_FAIL;
        }

        memcpy(recBuf,&pData[ulRtpHeadLen],ulAudioLen);
        enCode = MR_MEDIA_CODE_OK;

        MEDIA_DATA_INFO dataInfo;
        memset(&dataInfo,0x0,sizeof(dataInfo));

        parse_media_info(enType,rtpPacket.GetTimeStamp(),recBuf,dataInfo);

        handle_connection_media(dataInfo,ulAudioLen);
        mk_rtsp_service::instance().free_rtp_recv_buf(pData);
    }
    else {
        return AS_ERROR_CODE_FAIL;
    }
    return AS_ERROR_CODE_OK;
}
int32_t mk_rtsp_connection::handle_rtcp_packet(MK_RTSP_HANDLE_TYPE type,char* pData,uint32_t len)
{
    mk_rtsp_service::instance().free_rtp_recv_buf(pData);
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
        MK_LOG(AS_LOG_DEBUG,"rtsp connection need recv more data.");
        return 0; /* need more data deal */
    }

    nRet = rtspPacket.parse(pData,unDataSize);
    if (AS_ERROR_CODE_OK != nRet)
    {
        MK_LOG(AS_LOG_WARNING,"rtsp connection parser rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    MK_LOG(AS_LOG_INFO,"rtsp connection handle method index:[%d].",rtspPacket.getMethodIndex());

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

    if(unMediaSize > RTP_RECV_BUF_SIZE ) {
        MK_LOG(AS_LOG_ERROR,"rtsp connection process rtp or rtcp size:[%d] is too big.",unMediaSize);
        return AS_ERROR_CODE_FAIL;
    }

    char* buf = mk_rtsp_service::instance().get_rtp_recv_buf();
    if(NULL == buf) {
        /* drop it */
        return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN);
    }
    //MK_LOG(AS_LOG_ERROR,"rtsp connection process rtp or rtcp size:[%d].",unMediaSize);

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
int32_t mk_rtsp_connection::sendRtspReq(enRtspMethods enMethod,std::string& strUri,std::string& strTransport,uint64_t ullSessionId)
{
    std::string strUrl =  std::string("rtsp://") + (char*)&m_url.host[0];
    if(0 != m_url.port) {
        std::stringstream strPort;
        strPort << m_url.port;
        strUrl += ":" + strPort.str();
    }
    if("" == strUri) {
        strUrl += (char*)&m_url.uri[0];
    }
    else {
        strUrl = strUri;
    }

    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(enMethod);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(strUrl);
    if("" != strTransport) {
        rtspPacket.setTransPort(strTransport);
    }
    /* Authorization */
    if(('\0' != m_url.username[0]) && ('\0' != m_url.password[0]) && ("" != m_strAuthenticate)) {
        char   Digest[RTSP_DIGEST_LENS_MAX] = {0};
        as_digest_attr_value_t value;
        value.string = (char*)&m_url.username[0];
        as_digest_set_attr(&m_Authen, D_ATTR_USERNAME,value);
        value.string = (char*)&m_url.password[0];
        as_digest_set_attr(&m_Authen, D_ATTR_PASSWORD,value);
        value.number = DIGEST_ALGORITHM_NOT_SET;
        as_digest_set_attr(&m_Authen, D_ATTR_ALGORITHM,value);

        value.string = (char*)strUrl.c_str();
        as_digest_set_attr(&m_Authen, D_ATTR_URI,value);
        //value.number = DIGEST_QOP_AUTH;
        //as_digest_set_attr(&m_Authen, D_ATTR_QOP, value);
        std::string strMethod = mk_rtsp_packet::getMethodString(enMethod);
        value.string = (char*)strMethod.c_str();
        as_digest_set_attr(&m_Authen, D_ATTR_METHOD, value);

        MK_LOG(AS_LOG_INFO,"rtsp connection auth username:[%s] passwrod:[%s]\n"
                           "url:[%s] method:[%s]",
                           (char*)&m_url.username[0],(char*)&m_url.password[0],
                           (char*)strUri.c_str(),strMethod.c_str());

        if (-1 == as_digest_client_generate_header(&m_Authen, Digest, sizeof (Digest))) {
            MK_LOG(AS_LOG_INFO,"generate digest fail.");
            return AS_ERROR_CODE_FAIL;
        }
        MK_LOG(AS_LOG_INFO,"generate digest :[%s].",Digest);
        std::string strAuthorization = (char*)&Digest[0];
        rtspPacket.setAuthorization(strAuthorization);
    }

    if(0 != ullSessionId) {
        rtspPacket.setSessionID(ullSessionId);
    }
    std::string m_strMsg;
    if(AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(m_strMsg)) {
        MK_LOG(AS_LOG_INFO,"rtsp connect  generate Rtsp Req fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq,enMethod));
    m_ulSeq++;
    MK_LOG(AS_LOG_INFO,"rtsp connection send request:\n%s",m_strMsg.c_str());
    return sendMsg(m_strMsg.c_str(),m_strMsg.length());
}
int32_t mk_rtsp_connection::sendRtspOptionsReq()
{
    return sendRtspReq(RtspOptionsMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::sendRtspDescribeReq()
{
    return sendRtspReq(RtspDescribeMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::sendRtspSetupReq(SDP_MEDIA_INFO* info)
{
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
        if(MEDIA_TYPE_VALUE_VIDEO == info->ucMediaType) {
            strChannelNo << RTSP_INTERLEAVE_NUM_VIDEO_RTP;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info->ucMediaType){
            strChannelNo << RTSP_INTERLEAVE_NUM_AUDIO_RTP;
        }

        strTransport += strChannelNo.str() + SIGN_H_LINE;

        strChannelNo.str("");
        if(MEDIA_TYPE_VALUE_VIDEO == info->ucMediaType) {
            strChannelNo << RTSP_INTERLEAVE_NUM_VIDEO_RTCP;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info->ucMediaType){
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

        if(MEDIA_TYPE_VALUE_VIDEO == info->ucMediaType) {
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]  = pRtcpHandle;
            rtpType = MK_RTSP_UDP_VIDEO_RTP_HANDLE;
            rtcpType = MK_RTSP_UDP_VIDEO_RTCP_HANDLE;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info->ucMediaType){
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]  = pRtcpHandle;
            rtpType = MK_RTSP_UDP_AUDIO_RTP_HANDLE;
            rtcpType = MK_RTSP_UDP_AUDIO_RTCP_HANDLE;
        }
        else {
            MK_LOG(AS_LOG_ERROR,"rtsp connection client ,the media type is not rtp and rtcp.");
            return AS_ERROR_CODE_FAIL;
        }

		if (AS_ERROR_CODE_OK != pRtpHandle->start_handle(rtpType, this)) {
            return AS_ERROR_CODE_FAIL;
        }
		if (AS_ERROR_CODE_OK != pRtcpHandle->start_handle(rtpType, this)) {
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

    MK_LOG(AS_LOG_INFO,"rtsp connection client ,setup control:[%s].",info->strControl.c_str());

    std::string strUri =  STR_NULL;
    if("" != info->strControl) {
        strUri = info->strControl;
    }
    
    return sendRtspReq(RtspSetupMethod,strUri,strTransport);
}
int32_t mk_rtsp_connection::sendRtspPlayReq(uint64_t ullSessionId)
{
    return sendRtspReq(RtspPlayMethod,STR_NULL,STR_NULL,ullSessionId);
}
int32_t mk_rtsp_connection::sendRtspRecordReq()
{
    return sendRtspReq(RtspRecordMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::sendRtspGetParameterReq()
{
    return sendRtspReq(RtspGetParameterMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::sendRtspAnnounceReq()
{
    return sendRtspReq(RtspAnnounceMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::sendRtspPauseReq()
{
    return sendRtspReq(RtspPauseMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::sendRtspTeardownReq()
{
    return sendRtspReq(RtspTeardownMethod,STR_NULL,STR_NULL);
}
int32_t mk_rtsp_connection::handleRtspResp(mk_rtsp_packet &rtspMessage)
{
    uint32_t nCseq     = rtspMessage.getCseq();
    uint32_t nRespCode = rtspMessage.getRtspStatusCode();
    uint32_t enMethod   = RtspIllegalMethod;

    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse seq:[%d] ,Response Code:[%d].",nCseq,nRespCode);

    SEQ_METHOD_ITER iter = m_SeqMethodMap.find(nCseq);
    if(iter == m_SeqMethodMap.end()) {
        MK_LOG(AS_LOG_WARNING,"rtsp client there server reponse, not find the request Cseq:[%d].",nCseq);
        return AS_ERROR_CODE_FAIL;
    }

    enMethod = iter->second;
    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse seq:[%d] ,mothod:[%d].",nCseq,enMethod);

    if(RTSP_STATUS_UNAUTHORIZED == nRespCode) {
        if(RTSP_AUTH_TRY_MAX < m_ulAuthenTime) {
            MK_LOG(AS_LOG_WARNING,"rtsp server need Authen, but try:[%d] time,so error auth info.",m_ulAuthenTime);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != rtspMessage.getAuthenticate(m_strAuthenticate)) {
            MK_LOG(AS_LOG_WARNING,"rtsp server need Authen, get Authenticate header fail.");
            return AS_ERROR_CODE_FAIL;
        }
        MK_LOG(AS_LOG_INFO,"rtsp client handle authInfo:[%s].",m_strAuthenticate.c_str());
        if (AS_ERROR_CODE_OK != as_digest_is_digest(m_strAuthenticate.c_str())) {
            MK_LOG(AS_LOG_WARNING,"the WWW-Authenticate is not digest.");
            return AS_ERROR_CODE_FAIL;
        }

        if (AS_ERROR_CODE_OK == as_digest_client_parse(&m_Authen, m_strAuthenticate.c_str())) {
            MK_LOG(AS_LOG_WARNING,"parser WWW-Authenticate info fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_ulAuthenTime++;
        MK_LOG(AS_LOG_INFO,"rtsp server need Authen, send request with Authorization info,time:[%d].",m_ulAuthenTime);
        return sendRtspReq((enRtspMethods)enMethod,STR_NULL,STR_NULL);
    }
    else if(RTSP_STATUS_OK != nRespCode) {
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
    m_strSdpInfo = strSdp;

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

    SDP_MEDIA_INFO* info = m_mediaInfoList.front();
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
        SDP_MEDIA_INFO* info = m_mediaInfoList.front();
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
        MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,sessionid:[%lld],send play messgae.",rtspMessage.getSessionID());
        nRet = sendRtspPlayReq(rtspMessage.getSessionID());
    }
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response end.");
    return nRet;
}
int32_t mk_rtsp_connection::sendMsg(const char* pszData,uint32_t len)
{
    int32_t lSendSize = as_tcp_conn_handle::send(pszData,len,enSyncOp);
    if(lSendSize != len) {
        MK_LOG(AS_LOG_WARNING,"rtsp connection send msg len:[%d] return:[%d] fail.",len,lSendSize);
        return AS_ERROR_CODE_FAIL;
    }
    MK_LOG(AS_LOG_WARNING,"rtsp connection send msg len:[%d] succcess.",len);
    return AS_ERROR_CODE_OK;
}
void mk_rtsp_connection::handleH264Frame(RTP_PACK_QUEUE &rtpFrameList)
{
    //MK_LOG(AS_LOG_DEBUG, "handleH264Frame rtp packet list:[%d].",rtpFrameList.size());
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char*         pData;
    uint32_t      DataLen;
    uint32_t      TimeStam = 0;
    H264_NALU_HEADER  *nalu_hdr = NULL;
    FU_INDICATOR  *fu_ind   = NULL;
    FU_HEADER     *fu_hdr   = NULL;
    MR_MEDIA_CODE  enCode = MR_MEDIA_CODE_MAX;
    if(0 == rtpFrameList.size()) {
        return;
    }

    uint32_t ulTotaldatalen =  checkFrameTotalDataLen(rtpFrameList);

    uint32_t  bufflen;
    char* recBuf = handle_connection_databuf(ulTotaldatalen,bufflen);

    if(NULL == recBuf)
    {
        MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return;
    }
    recBuf[0] = 0x00;
    recBuf[1] = 0x00;
    recBuf[2] = 0x00;
    recBuf[3] = 0x01;

    uint32_t m_ulRecvLen  = 4;


    if(1 == rtpFrameList.size()) {  
        pData =  rtpFrameList[0].pRtpMsgBlock;
        DataLen = rtpFrameList[0].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, parse rtp packet fail.");
            return ;
        }
        
        rtpHeadLen   = rtpPacket.GetHeadLen();
        TimeStam     = rtpPacket.GetTimeStamp();
        rtpPayloadLen = DataLen - rtpHeadLen - rtpPacket.GetTailLen();

        if( (DataLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen()))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, DataLen invalid.TailLen[%d]",rtpPacket.GetTailLen());
            return;
        }

        nalu_hdr = (H264_NALU_HEADER*)&pData[rtpHeadLen];
        //MK_LOG(AS_LOG_DEBUG, "**handle nalu:[%d] rtpHeadLen[%d]pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        memcpy(&recBuf[m_ulRecvLen],&pData[rtpHeadLen],rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
        enCode = MR_MEDIA_CODE_OK;

        MEDIA_DATA_INFO dataInfo;
        memset(&dataInfo,0x0,sizeof(dataInfo));
        parse_media_info(MR_MEDIA_TYPE_H264,TimeStam,recBuf,dataInfo);

        handle_connection_media(dataInfo,m_ulRecvLen);
        return;
    }
    
    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, parse rtp packet fail.");
            return ;
        }
        rtpHeadLen   = rtpPacket.GetHeadLen();
        TimeStam     = rtpPacket.GetTimeStamp();
        rtpPayloadLen = DataLen - rtpHeadLen - rtpPacket.GetTailLen();

        if( (DataLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen()))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, DataLen invalid.TailLen[%d]",rtpPacket.GetTailLen());
            return;
        }

        pData = &pData[rtpHeadLen];

        fu_ind = (FU_INDICATOR*)pData;    pData++;
        fu_hdr = (FU_HEADER*)pData; pData++;
        rtpPayloadLen -= 2;
        if(0 == i) {
            /* first packet */
            nalu_hdr = (H264_NALU_HEADER*)&recBuf[m_ulRecvLen];
            nalu_hdr->TYPE = fu_hdr->TYPE;
            nalu_hdr->F    = fu_ind->F;
            nalu_hdr->NRI  = fu_ind->NRI;
            m_ulRecvLen++; /* 1 byte */
            TimeStam     = rtpPacket.GetTimeStamp();
            //MK_LOG(AS_LOG_DEBUG, "##handle nalu:[%d],rtpHeadLen[%d] pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        }
        memcpy(&recBuf[m_ulRecvLen],pData,rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    enCode = MR_MEDIA_CODE_OK;
    
    MEDIA_DATA_INFO dataInfo;
    memset(&dataInfo,0x0,sizeof(dataInfo));
    parse_media_info(MR_MEDIA_TYPE_H264,TimeStam,recBuf,dataInfo);

    handle_connection_media(dataInfo,m_ulRecvLen);
}
void mk_rtsp_connection::handleH265Frame(RTP_PACK_QUEUE &rtpFrameList)
{
    //MK_LOG(AS_LOG_DEBUG, "handleH265Frame rtp packet list:[%d].",rtpFrameList.size());
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char*         pData;
    uint32_t      DataLen;
    uint32_t      TimeStam = 0;
    uint32_t      fu_type;
    H265_NALU_HEADER  *nalu_hdr = NULL;
    MR_MEDIA_CODE  enCode = MR_MEDIA_CODE_MAX;

    if(0 == rtpFrameList.size()) {
        return;
    }

    uint32_t ulTotaldatalen =  checkFrameTotalDataLen(rtpFrameList);

    uint32_t  bufflen;
    char* recBuf = handle_connection_databuf(ulTotaldatalen,bufflen);

    if(NULL == recBuf)
    {
        MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return;
    }
    recBuf[0] = 0x00;
    recBuf[1] = 0x00;
    recBuf[2] = 0x00;
    recBuf[3] = 0x01;

    uint32_t m_ulRecvLen  = 4;

    if(1 == rtpFrameList.size()) {  
        pData =  rtpFrameList[0].pRtpMsgBlock;
        DataLen = rtpFrameList[0].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "handleH265Frame rtp packet, parse rtp packet fail.");
            return ;
        }
        
        rtpHeadLen   = rtpPacket.GetHeadLen();
        TimeStam     = rtpPacket.GetTimeStamp();
        rtpPayloadLen = DataLen - rtpHeadLen - rtpPacket.GetTailLen();

        if( (DataLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen()))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, DataLen invalid.TailLen[%d]",rtpPacket.GetTailLen());
            return;
        }


        nalu_hdr = (H265_NALU_HEADER*)&pData[rtpHeadLen];
        //MK_LOG(AS_LOG_DEBUG, "**handle nalu:[%d] rtpHeadLen[%d]pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        memcpy(&recBuf[m_ulRecvLen],&pData[rtpHeadLen],rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
        enCode = MR_MEDIA_CODE_OK;

        MEDIA_DATA_INFO dataInfo;
        memset(&dataInfo,0x0,sizeof(dataInfo));
        parse_media_info(MR_MEDIA_TYPE_H265,TimeStam,recBuf,dataInfo);

        handle_connection_media(dataInfo,m_ulRecvLen);
        return;
    }
    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "handleH265Frame rtp packet, parse rtp packet fail.");
            return ;
        }
        rtpHeadLen   = rtpPacket.GetHeadLen();
        pData = &pData[rtpHeadLen];
        rtpPayloadLen = DataLen - rtpHeadLen - rtpPacket.GetTailLen();       

        if( (DataLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen()))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, DataLen invalid.TailLen[%d]",rtpPacket.GetTailLen());
            return;
        }

        if(0 == i) {
            //fu_type = pData[2] & 0x3f;
            /* first packet */
            u_int8_t nal_unit_type = pData[2]  & 0x3f;

            recBuf[m_ulRecvLen] = (pData[0] & 0x81) | (nal_unit_type << 1);
            m_ulRecvLen++;
            recBuf[m_ulRecvLen] = pData[1];
            m_ulRecvLen++;

            TimeStam     = rtpPacket.GetTimeStamp();
        }
        pData += 3;
        rtpPayloadLen -= 3;

        memcpy(&recBuf[m_ulRecvLen],pData,rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    enCode = MR_MEDIA_CODE_OK;
    MEDIA_DATA_INFO dataInfo;
    memset(&dataInfo,0x0,sizeof(dataInfo));
    parse_media_info(MR_MEDIA_TYPE_H265,TimeStam,recBuf,dataInfo);

    handle_connection_media(dataInfo,m_ulRecvLen);
}
void mk_rtsp_connection::handleOtherFrame(uint8_t PayloadType,RTP_PACK_QUEUE &rtpFrameList)
{
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char*         pData;
    uint32_t      DataLen;
    uint32_t      TimeStam = 0;
    MR_MEDIA_TYPE type = MR_MEDIA_TYPE_INVALID;
    MR_MEDIA_CODE enCode = MR_MEDIA_CODE_MAX;

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

    uint32_t ulTotaldatalen =  checkFrameTotalDataLen(rtpFrameList);
    uint32_t  bufflen;
    char* recBuf = handle_connection_databuf(ulTotaldatalen,bufflen);

    if(NULL == recBuf)
    {
        MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return ;
    }

    uint32_t m_ulRecvLen  = 0;

    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return ;
        }
		rtpHeadLen = rtpPacket.GetHeadLen();
        pData = &pData[rtpHeadLen];
        rtpPayloadLen = DataLen - rtpHeadLen;

        TimeStam     = rtpPacket.GetTimeStamp();

        memcpy(&recBuf[m_ulRecvLen],pData,rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    enCode =  MR_MEDIA_CODE_OK;
    MEDIA_DATA_INFO dataInfo;
    memset(&dataInfo,0x0,sizeof(dataInfo));
    parse_media_info(type,TimeStam,recBuf,dataInfo);

    handle_connection_media(dataInfo,m_ulRecvLen);
}

int32_t mk_rtsp_connection::checkFrameTotalDataLen(RTP_PACK_QUEUE &rtpFrameList)
{
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen = 0;
    char*         pData;
    uint32_t      DataLen;
    for(uint32_t i = 0; i < rtpFrameList.size();i++) {
        pData =  rtpFrameList[i].pRtpMsgBlock;
        DataLen = rtpFrameList[i].len;
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData,DataLen))
        {
            MK_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, parse rtp packet fail.");
            return 0;
        }
        rtpHeadLen   = rtpPacket.GetHeadLen();
        uint32_t ulen = DataLen - rtpHeadLen - rtpPacket.GetTailLen();
        rtpPayloadLen += ulen;
    }
    return rtpPayloadLen;
}

void mk_rtsp_connection::resetRtspConnect()
{
    //m_rtpFrameOrganizer.release();
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