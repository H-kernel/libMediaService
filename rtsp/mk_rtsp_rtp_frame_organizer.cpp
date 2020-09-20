#include "as.h"
#include "mk_rtsp_rtp_frame_organizer.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_rtsp_service.h"
#include "mk_media_common.h"

mk_rtp_frame_organizer::mk_rtp_frame_organizer()
{
    m_unMaxCacheFrameNum    = 0;
    m_pRtpFrameHandler      = NULL;

}

mk_rtp_frame_organizer::~mk_rtp_frame_organizer()
{
    try
    {
        release();
    }
    catch(...){}

    m_pRtpFrameHandler = NULL;
}

int32_t mk_rtp_frame_organizer::init(mk_rtp_frame_handler* pHandler,
                             uint32_t unMaxFrameCache /*= MAX_RTP_FRAME_CACHE_NUM*/)
{
    if ((NULL == pHandler) || (0 == unMaxFrameCache))
    {
        return AS_ERROR_CODE_FAIL;
    }

    m_pRtpFrameHandler   = pHandler;
    m_unMaxCacheFrameNum = unMaxFrameCache;

    RTP_FRAME_INFO_S *pFramInfo = NULL;

    for(uint32_t i = 0;i < m_unMaxCacheFrameNum;i++)
    {
        pFramInfo = AS_NEW(pFramInfo);
        if(NULL == pFramInfo)
        {
            return AS_ERROR_CODE_FAIL;
        }
        m_RtpFrameFreeList.push_back(pFramInfo);
    }



    MK_LOG(AS_LOG_INFO,"success to init rtp frame organizer[%p].", this);
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_frame_organizer::insertRtpPacket(char* pRtpBlock,uint32_t len)
{
    if (NULL == pRtpBlock)
    {
        return AS_ERROR_CODE_FAIL;
    }

    mk_rtp_packet rtpPacket;
    if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pRtpBlock, len))
    {
        MK_LOG(AS_LOG_ERROR, "fail to insert rtp packet, parse rtp packet fail.");
        return AS_ERROR_CODE_FAIL;
    }

    RTP_FRAME_INFO_S* pFrameInfo = NULL;

    RTP_PACK_INFO_S  rtpInfo;
    rtpInfo.bMarker      = rtpPacket.GetMarker();
    rtpInfo.usSeq        = rtpPacket.GetSeqNum();
    rtpInfo.unTimestamp  = rtpPacket.GetTimeStamp();
    rtpInfo.pRtpMsgBlock = pRtpBlock;
    rtpInfo.len          = len;

    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.find(rtpInfo.unTimestamp);

    if(iter == m_RtpFrameMap.end())
    {
        pFrameInfo = InsertFrame(rtpPacket.GetPayloadType(),rtpInfo.unTimestamp);
    }
    else
    {
        pFrameInfo = iter->second;
    }


    if(NULL == pFrameInfo)
    {
         MK_LOG(AS_LOG_ERROR,"Insert Frame fail,becaus there is no free frame info.");
         RTP_FRAME_INFO_S *pFrameinfo = GetSmallFrame();
         releaseRtpPacket(pFrameinfo);
         return AS_ERROR_CODE_FAIL;
    }
    if(false == pFrameInfo->bMarker)
    {
        pFrameInfo->bMarker = rtpInfo.bMarker;
    }

    if (AS_ERROR_CODE_OK != insert(pFrameInfo,rtpInfo))
    {
        return AS_ERROR_CODE_FAIL;
    }

    checkFrame();


    return AS_ERROR_CODE_OK;
}

void mk_rtp_frame_organizer::release()
{
    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.begin();
    RTP_FRAME_INFO_S *pFramInfo = NULL;
    for(;iter != m_RtpFrameMap.end();++iter)
    {
        pFramInfo = iter->second;
        if(NULL == pFramInfo)
        {
            continue;
        }
        while (!pFramInfo->PacketQueue.empty())
        {
            mk_rtsp_service::instance().free_rtp_recv_buf(pFramInfo->PacketQueue[0].pRtpMsgBlock);
            pFramInfo->PacketQueue.pop_front();
        }

        AS_DELETE(pFramInfo);
    }

    m_RtpFrameMap.clear();

    while(!m_RtpFrameFreeList.empty())
    {
        pFramInfo = m_RtpFrameFreeList.front();
        m_RtpFrameFreeList.pop_front();
        AS_DELETE(pFramInfo);
    }

    MK_LOG(AS_LOG_INFO,"success to release rtp frame organizer[%p].", this);
    return;
}


int32_t mk_rtp_frame_organizer::insert(RTP_FRAME_INFO_S *pFrameinfo,const RTP_PACK_INFO_S &info)
{

    if(NULL == pFrameinfo)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (0 == pFrameinfo->PacketQueue.size())
    {
        pFrameinfo->PacketQueue.push_back(info);
        return AS_ERROR_CODE_OK;
    }

    uint16_t usFirstSeq = pFrameinfo->PacketQueue[0].usSeq;
    uint16_t usLastSeq = pFrameinfo->PacketQueue[pFrameinfo->PacketQueue.size() - 1].usSeq;


    if (info.usSeq >= usLastSeq)
    {
        pFrameinfo->PacketQueue.push_back(info);
    }
    else if(info.usSeq < usFirstSeq)
    {
        pFrameinfo->PacketQueue.push_front(info);
    }
    else
    {
        return insertRange(pFrameinfo,info);
    }
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_frame_organizer::insertRange(RTP_FRAME_INFO_S *pFrameinfo ,const RTP_PACK_INFO_S &info)
{
    uint32_t i = 0;
    uint32_t unSize = pFrameinfo->PacketQueue.size();
    while (i <= unSize)
    {
        if (info.usSeq <= pFrameinfo->PacketQueue[i].usSeq)
        {
            pFrameinfo->PacketQueue.insert(pFrameinfo->PacketQueue.begin() + (int32_t)i, info);
            return AS_ERROR_CODE_OK;
        }

        i++;
    }

    MK_LOG(AS_LOG_WARNING,"fail to insert rtp packet[%u : %u].",info.usSeq, info.unTimestamp);
    return AS_ERROR_CODE_FAIL;
}

void mk_rtp_frame_organizer::checkFrame()
{

    RTP_FRAME_INFO_S *pFrameinfo = GetSmallFrame();

    if(NULL == pFrameinfo)
    {
        MK_LOG(AS_LOG_DEBUG,"Get Small Frame fail.");
        return;
    }
    if (pFrameinfo->PacketQueue.empty())
    {
        MK_LOG(AS_LOG_DEBUG,"Get Small Frame is empty!.");
        return;
    }

    if(!pFrameinfo->bMarker)
    {
        return;
    }

    uint16_t usSeq = (uint16_t)pFrameinfo->PacketQueue[0].usSeq;
    for (uint32_t i = 0; i < pFrameinfo->PacketQueue.size(); i++)
    {
        if (usSeq != pFrameinfo->PacketQueue[i].usSeq)
        {
            MK_LOG(AS_LOG_DEBUG,"the Frame is not full,need Seq:[%d],Current Seq:[%d],Index:[%d]!.",usSeq,pFrameinfo->PacketQueue[i].usSeq,i);
            return;
        }
        usSeq++;
    }
    handleFinishedFrame(pFrameinfo);

    releaseRtpPacket(pFrameinfo);

    return;
}


void mk_rtp_frame_organizer::handleFinishedFrame(RTP_FRAME_INFO_S *pFrameinfo)
{
    if(NULL == pFrameinfo)
    {
        MK_LOG(AS_LOG_WARNING," Handle finish frame ,the Frame info is NULL");
        return;
    }
    if (pFrameinfo->PacketQueue.empty())
    {
        MK_LOG(AS_LOG_WARNING," Handle finish frame ,the Frame queue is empty");
        return;
    }

    if (NULL == m_pRtpFrameHandler)
    {
        MK_LOG(AS_LOG_WARNING," Handle finish frame ,the RTP Handle is NULL");
        return;
    }

    m_pRtpFrameHandler->handleRtpFrame(pFrameinfo->PayloadType,pFrameinfo->PacketQueue);

    return;
}

void mk_rtp_frame_organizer::releaseRtpPacket(RTP_FRAME_INFO_S *pFrameinfo)
{
    if(NULL == pFrameinfo)
    {
        return;
    }

    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.find(pFrameinfo->unTimestamp);

    if(iter == m_RtpFrameMap.end())
    {
        return;
    }


    while (0 < pFrameinfo->PacketQueue.size())
    {
        if (NULL != pFrameinfo->PacketQueue[0].pRtpMsgBlock)
        {
            mk_rtsp_service::instance().free_rtp_recv_buf(pFrameinfo->PacketQueue[0].pRtpMsgBlock);
        }
        pFrameinfo->PacketQueue.pop_front();
    }
    pFrameinfo->PacketQueue.clear();

    m_RtpFrameMap.erase(iter);
    m_RtpFrameFreeList.push_back(pFrameinfo);


    return;
}
RTP_FRAME_INFO_S* mk_rtp_frame_organizer::InsertFrame(uint8_t PayloadType,uint32_t  unTimestamp)
{
    if(0 == m_RtpFrameFreeList.size())
    {
        return NULL;
    }
    RTP_FRAME_INFO_S* pFrame = m_RtpFrameFreeList.front();
    m_RtpFrameFreeList.pop_front();
    pFrame->bMarker = false;
    pFrame->PayloadType = PayloadType;
    pFrame->unTimestamp = unTimestamp;
    m_RtpFrameMap.insert(RTP_FRAME_MAP_S::value_type(unTimestamp,pFrame));
    return pFrame;
}

RTP_FRAME_INFO_S* mk_rtp_frame_organizer::GetSmallFrame()
{
    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.begin();
    RTP_FRAME_INFO_S *pCurFramInfo = NULL;
    RTP_FRAME_INFO_S *pSmallFramInfo = NULL;
    for(;iter != m_RtpFrameMap.end();++iter)
    {
        pCurFramInfo = iter->second;
        if(NULL == pCurFramInfo)
        {
            continue;
        }
        if(NULL == pSmallFramInfo)
        {
            pSmallFramInfo = pCurFramInfo;
            continue;
        }


        if(pSmallFramInfo->unTimestamp >  pCurFramInfo->unTimestamp)
        {
            pSmallFramInfo = pCurFramInfo;
        }
    }

    return pSmallFramInfo;
}

