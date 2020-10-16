#ifndef __MK_RTSP_RTP_FRAME_ORGANIZER_INCLUDE_H__
#define __MK_RTSP_RTP_FRAME_ORGANIZER_INCLUDE_H__

#include <deque>
#include <list>
#include <map>
#include "as.h"

typedef enum
{
    RTP_H264_NALU_TYPE_UNDEFINED    = 0,
    RTP_H264_NALU_TYPE_IDR          = 5,
    RTP_H264_NALU_TYPE_SEI          = 6,
    RTP_H264_NALU_TYPE_SPS          = 7,
    RTP_H264_NALU_TYPE_PPS          = 8,
    RTP_H264_NALU_TYPE_STAP_A       = 24,
    RTP_H264_NALU_TYPE_STAP_B       = 25,
    RTP_H264_NALU_TYPE_MTAP16       = 26,
    RTP_H264_NALU_TYPE_MTAP24       = 27,
    RTP_H264_NALU_TYPE_FU_A         = 28,
    RTP_H264_NALU_TYPE_FU_B         = 29,
    RTP_H264_NALU_TYPE_END
}RTP_H264_NALU_TYPE;

/**
 * Table 7-1: NAL unit type codes
 */
typedef enum  {
    RTP_HEVC_NAL_TRAIL_N    = 0,
    RTP_HEVC_NAL_TRAIL_R    = 1,
    RTP_HEVC_NAL_TSA_N      = 2,
    RTP_HEVC_NAL_TSA_R      = 3,
    RTP_HEVC_NAL_STSA_N     = 4,
    RTP_HEVC_NAL_STSA_R     = 5,
    RTP_HEVC_NAL_RADL_N     = 6,
    RTP_HEVC_NAL_RADL_R     = 7,
    RTP_HEVC_NAL_RASL_N     = 8,
    RTP_HEVC_NAL_RASL_R     = 9,
    RTP_HEVC_NAL_VCL_N10    = 10,
    RTP_HEVC_NAL_VCL_R11    = 11,
    RTP_HEVC_NAL_VCL_N12    = 12,
    RTP_HEVC_NAL_VCL_R13    = 13,
    RTP_HEVC_NAL_VCL_N14    = 14,
    RTP_HEVC_NAL_VCL_R15    = 15,
    RTP_HEVC_NAL_BLA_W_LP   = 16,
    RTP_HEVC_NAL_BLA_W_RADL = 17,
    RTP_HEVC_NAL_BLA_N_LP   = 18,
    RTP_HEVC_NAL_IDR_W_RADL = 19,
    RTP_HEVC_NAL_IDR_N_LP   = 20,
    RTP_HEVC_NAL_CRA_NUT    = 21,
    RTP_HEVC_NAL_IRAP_VCL22 = 22,
    RTP_HEVC_NAL_IRAP_VCL23 = 23,
    RTP_HEVC_NAL_RSV_VCL24  = 24,
    RTP_HEVC_NAL_RSV_VCL25  = 25,
    RTP_HEVC_NAL_RSV_VCL26  = 26,
    RTP_HEVC_NAL_RSV_VCL27  = 27,
    RTP_HEVC_NAL_RSV_VCL28  = 28,
    RTP_HEVC_NAL_RSV_VCL29  = 29,
    RTP_HEVC_NAL_RSV_VCL30  = 30,
    RTP_HEVC_NAL_RSV_VCL31  = 31,
    RTP_HEVC_NAL_VPS        = 32,
    RTP_HEVC_NAL_SPS        = 33,
    RTP_HEVC_NAL_PPS        = 34,
    RTP_HEVC_NAL_AUD        = 35,
    RTP_HEVC_NAL_EOS_NUT    = 36,
    RTP_HEVC_NAL_EOB_NUT    = 37,
    RTP_HEVC_NAL_FD_NUT     = 38,
    RTP_HEVC_NAL_SEI_PREFIX = 39,
    RTP_HEVC_NAL_SEI_SUFFIX = 40,
}RTP_HEVC_NALU_TYPE;

typedef struct
{
    //byte 0
    uint8_t TYPE:5;
    uint8_t NRI:2;
    uint8_t F:1;
} H264_NALU_HEADER; /**//* 1 BYTES */

typedef struct
{
    //byte 0
    uint8_t LATERID0:1;
    uint8_t TYPE:6;
    uint8_t F:1;
    //byte 1
    uint8_t TID:3;
    uint8_t LATERID1:5;
} H265_NALU_HEADER; /**//* 2 BYTES */


typedef struct
{
    //byte 0
    uint8_t TYPE:5;
    uint8_t NRI:2;
    uint8_t F:1;
} FU_INDICATOR; /**//* 1 BYTES */

typedef struct
{
    //byte 0
    uint8_t TYPE:5;
    uint8_t R:1;
    uint8_t E:1;
    uint8_t S:1;
} FU_HEADER; /**//* 1 BYTES */

#define MAX_RTP_FRAME_CACHE_NUM     5
#define MAX_RTP_SEQ                 65535

typedef struct _stRTP_PACK_INFO_S
{
    uint16_t      usSeq;
    uint32_t      unTimestamp;
    bool          bMarker;
    char*         pRtpMsgBlock;
    uint32_t      len;
}RTP_PACK_INFO_S;
typedef std::deque<RTP_PACK_INFO_S>       RTP_PACK_QUEUE;
typedef struct _stRTP_FRAME_INFO_S
{
    uint32_t        unTimestamp;
    bool            bMarker;
    uint8_t         PayloadType;
    RTP_PACK_QUEUE  PacketQueue;
}RTP_FRAME_INFO_S;
typedef std::map<uint32_t,RTP_FRAME_INFO_S*> RTP_FRAME_MAP_S;
typedef std::list<RTP_FRAME_INFO_S*> RTP_FRAME_LIST_S;


#define INVALID_RTP_SEQ     (0x80000000)

class mk_rtp_frame_handler
{
public:
    mk_rtp_frame_handler(){}

    virtual ~mk_rtp_frame_handler(){}

    virtual void handleRtpFrame(uint8_t PayloadType,RTP_PACK_QUEUE &rtpFrameList) = 0;
};

class mk_rtp_frame_organizer
{
public:
    mk_rtp_frame_organizer();
    virtual ~mk_rtp_frame_organizer();

    int32_t init(mk_rtp_frame_handler* pHandler, uint32_t unMaxFrameCache = MAX_RTP_FRAME_CACHE_NUM);

    int32_t insertRtpPacket(char* pRtpBlock,uint32_t len);

    void release();

    void getRtpPacketStatInfo(uint32_t &totalPackNum,uint32_t &lostRtpPacketNum,uint32_t &lostFrameNum,uint32_t &disorderSeqCounts);

    void updateTotalRtpPacketNum();
    void updateLastRtpSeq(uint32_t uSeq, bool bRelease);
private:
    int32_t insert(RTP_FRAME_INFO_S *pFrameinfo,const RTP_PACK_INFO_S &info);

    int32_t insertRange(RTP_FRAME_INFO_S *pFrameinfo ,const RTP_PACK_INFO_S &info);

    void checkFrame();

    void handleFinishedFrame(RTP_FRAME_INFO_S *pFrameinfo);

    void releaseRtpPacket(RTP_FRAME_INFO_S *pFrameinfo);
    RTP_FRAME_INFO_S* InsertFrame(uint8_t PayloadType,uint32_t  unTimestamp);
    RTP_FRAME_INFO_S* GetSmallFrame();
private:
    uint32_t                 m_unMaxCacheFrameNum;
    mk_rtp_frame_handler*    m_pRtpFrameHandler;

    RTP_FRAME_MAP_S          m_RtpFrameMap;
    RTP_FRAME_LIST_S         m_RtpFrameFreeList;

    bool                     m_bFirstRtpPacket;
    uint32_t                 m_unTotalRtpPacketNum;
    uint32_t                 m_unLostRtpPacketNum;      //丢包
    uint32_t                 m_unLostFrameNum;          //丢帧
    uint32_t                 m_unDisorderSeqCounts;     //乱序次数
    uint32_t                 m_unLastRtpSeq;
};

#endif /* __MK_RTSP_RTP_FRAME_ORGANIZER_INCLUDE_H__ */
