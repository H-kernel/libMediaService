
#include "as.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_media_common.h"
#include <netinet/in.h>

mk_rtp_packet::mk_rtp_packet()
{
    m_pRtpData      = NULL;
    m_ulPacketLen   = 0;
    m_ulHeadLen     = 0;
    m_ulTailLen     = 0;

    m_pFixedHead    = NULL;
    m_pExtHead      = NULL;
}

mk_rtp_packet::~mk_rtp_packet()
{
    m_pRtpData      = NULL;
    m_pFixedHead    = NULL;
    m_pExtHead      = NULL;
}

int32_t mk_rtp_packet::ParsePacket(const char* pRtpData,uint32_t ulLen)
{
    if (NULL == pRtpData)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, rtp data is null.");
        return AS_ERROR_CODE_PARAM;
    }

    if (ulLen < sizeof(RTP_FIXED_HEADER))
    {
        MK_LOG(AS_LOG_DEBUG,"mk_rtp_packet::Parse fail, rtp data len is shorter than fixed head len.");
        return AS_ERROR_CODE_PARAM;
    }
    m_pRtpData = (char*)pRtpData;
    m_ulPacketLen = ulLen;

    m_pFixedHead = (RTP_FIXED_HEADER*)(void*)m_pRtpData;

    if(1 == m_pFixedHead->padding)
    {
        const char* pTail = pRtpData + ulLen -1;

         m_ulTailLen = (uint32_t)(*(uint8_t*)pTail);
        if(AS_ERROR_CODE_OK != SetPadding(0))
        {
            MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, SetPadding fail.");
        }
    }
    uint32_t ulHeadLen = sizeof(RTP_FIXED_HEADER) + m_pFixedHead->csrc_len * RTP_CSRC_LEN;
    if (ulLen < ulHeadLen)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, rtp data len is shorter than fixed head len.");
        return AS_ERROR_CODE_PARAM;
    }

    if (AS_ERROR_CODE_OK != CheckVersion())
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, check version fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (1 != m_pFixedHead->extension)
    {
        m_ulHeadLen = ulHeadLen;
        return AS_ERROR_CODE_OK;
    }

    if (ulLen < ulHeadLen + sizeof(RTP_EXTEND_HEADER))
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, packet len is too int16_t to contain extend head.");
        return AS_ERROR_CODE_PARAM;
    }

    m_pExtHead = (RTP_EXTEND_HEADER*)(void*)(m_pRtpData + ulHeadLen);


    ulHeadLen += sizeof(RTP_EXTEND_HEADER) + ntohs(m_pExtHead->usLength) * RTP_EXTEND_PROFILE_LEN;
    if (ulLen < ulHeadLen)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, packet len is too int16_t.");
        return AS_ERROR_CODE_PARAM;
    }

    m_ulHeadLen = ulHeadLen;

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::GeneratePacket(char*  pRtpPacket,uint32_t  ulLen)
{
    if (NULL == pRtpPacket)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GeneratePacket fail, packet is null.");
        return AS_ERROR_CODE_PARAM;
    }


    memset(pRtpPacket, 0x0, ulLen);
    m_pRtpData = pRtpPacket;
    m_ulPacketLen = ulLen;

    m_pFixedHead = (RTP_FIXED_HEADER*)(void*)m_pRtpData;

    m_pFixedHead->version = RTP_PACKET_VERSION;
    m_pFixedHead->marker  = 0;
    m_pFixedHead->payload = 96;
    m_pFixedHead->extension = 0;

    m_ulHeadLen = sizeof(RTP_FIXED_HEADER);

    return AS_ERROR_CODE_OK;
}
int32_t mk_rtp_packet::GetVersion(char& cVersion)const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetVersion fail, fixed head is null.");
        return AS_ERROR_CODE_FAIL;
    }

    cVersion = m_pFixedHead->version;

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::CheckVersion()const
{
    char cVersion = 0;
    if (AS_ERROR_CODE_OK != GetVersion(cVersion))
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::CheckVersion fail, get packet version fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (RTP_PACKET_VERSION != cVersion)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::CheckVersion fail, version [%d] is invalid.",
            cVersion);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

uint16_t mk_rtp_packet::GetSeqNum()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetSeqNum fail, packet is null.");
        return 0;
    }

    return ntohs(m_pFixedHead->seq_no);
}

uint32_t  mk_rtp_packet::GetTimeStamp()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetTimeStamp fail, packet is null.");
        return 0;
    }

    return ntohl(m_pFixedHead->timestamp);
}

char  mk_rtp_packet::GetPayloadType()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetPayloadType fail, packet is null.");
        return 0;
    }

    return m_pFixedHead->payload;
}

bool mk_rtp_packet::GetMarker()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetMarker fail, packet is null.");
        return false;
    }

    return m_pFixedHead->marker;
}

uint32_t mk_rtp_packet::GetSSRC()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetSSRC fail, packet is null.");
        return 0;
    }

    return m_pFixedHead->ssrc;
}


uint32_t mk_rtp_packet::GetHeadLen()const
{
    return m_ulHeadLen;
}
uint32_t mk_rtp_packet::GetTailLen()const
{
    return m_ulTailLen;
}

int32_t mk_rtp_packet::SetVersion(uint8_t ucVersion)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetVersion fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->version = ucVersion;

    return AS_ERROR_CODE_OK;
}
int32_t mk_rtp_packet::SetPadding(uint8_t ucPadding)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetPadding fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->padding= ucPadding;

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::SetSeqNum(uint16_t usSeqNum)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetSeqNum fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->seq_no = ntohs(usSeqNum);

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::SetTimeStamp(uint32_t ulTimeStamp)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetTimeStamp fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->timestamp = ntohl(ulTimeStamp);

    return AS_ERROR_CODE_OK;
}

void mk_rtp_packet::SetSSRC(uint32_t unSsrc)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetTimeStamp fail, packet is null.");
        return;
    }

    m_pFixedHead->ssrc = unSsrc;
    return;
}

int32_t mk_rtp_packet::SetPayloadType(uint8_t ucPayloadType)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetPayloadType fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->payload = ucPayloadType;

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::SetMarker(bool bMarker)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetMarker fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->marker = bMarker;

    return AS_ERROR_CODE_OK;
}

uint16_t mk_rtp_packet::GetSeqNum(const char* pMb)
{

    if (NULL == pMb)
    {
        MK_LOG(AS_LOG_ERROR, "mk_rtp_packet::GetSeqNum fail, mb is null.");
        return 0;
    }

    RTP_FIXED_HEADER* FixedHead = (RTP_FIXED_HEADER*)(void*)pMb;

    return ntohs(FixedHead->seq_no);
}

