#ifndef _AS_NAL_SPS_PARSE_H_
#define _AS_NAL_SPS_PARSE_H_
#include "as.h"

typedef enum
{
    MK_H264_NALU_TYPE_UNDEFINED    = 0,
    MK_H264_NALU_TYPE_IDR          = 5,
    MK_H264_NALU_TYPE_SEI          = 6,
    MK_H264_NALU_TYPE_SPS          = 7,
    MK_H264_NALU_TYPE_PPS          = 8,
    MK_H264_NALU_TYPE_STAP_A       = 24,
    MK_H264_NALU_TYPE_STAP_B       = 25,
    MK_H264_NALU_TYPE_MTAP16       = 26,
    MK_H264_NALU_TYPE_MTAP24       = 27,
    MK_H264_NALU_TYPE_FU_A         = 28,
    MK_H264_NALU_TYPE_FU_B         = 29,
    MK_H264_NALU_TYPE_END
}MK_H264_NALU_TYPE;

/**
 * Table 7-1: NAL unit type codes
 */
typedef enum  {
    MK_HEVC_NAL_TRAIL_N    = 0,
    MK_HEVC_NAL_TRAIL_R    = 1,
    MK_HEVC_NAL_TSA_N      = 2,
    MK_HEVC_NAL_TSA_R      = 3,
    MK_HEVC_NAL_STSA_N     = 4,
    MK_HEVC_NAL_STSA_R     = 5,
    MK_HEVC_NAL_RADL_N     = 6,
    MK_HEVC_NAL_RADL_R     = 7,
    MK_HEVC_NAL_RASL_N     = 8,
    MK_HEVC_NAL_RASL_R     = 9,
    MK_HEVC_NAL_VCL_N10    = 10,
    MK_HEVC_NAL_VCL_R11    = 11,
    MK_HEVC_NAL_VCL_N12    = 12,
    MK_HEVC_NAL_VCL_R13    = 13,
    MK_HEVC_NAL_VCL_N14    = 14,
    MK_HEVC_NAL_VCL_R15    = 15,
    MK_HEVC_NAL_BLA_W_LP   = 16,
    MK_HEVC_NAL_BLA_W_RADL = 17,
    MK_HEVC_NAL_BLA_N_LP   = 18,
    MK_HEVC_NAL_IDR_W_RADL = 19,
    MK_HEVC_NAL_IDR_N_LP   = 20,
    MK_HEVC_NAL_CRA_NUT    = 21,
    MK_HEVC_NAL_IRAP_VCL22 = 22,
    MK_HEVC_NAL_IRAP_VCL23 = 23,
    MK_HEVC_NAL_RSV_VCL24  = 24,
    MK_HEVC_NAL_RSV_VCL25  = 25,
    MK_HEVC_NAL_RSV_VCL26  = 26,
    MK_HEVC_NAL_RSV_VCL27  = 27,
    MK_HEVC_NAL_RSV_VCL28  = 28,
    MK_HEVC_NAL_RSV_VCL29  = 29,
    MK_HEVC_NAL_RSV_VCL30  = 30,
    MK_HEVC_NAL_RSV_VCL31  = 31,
    MK_HEVC_NAL_VPS        = 32,
    MK_HEVC_NAL_SPS        = 33,
    MK_HEVC_NAL_PPS        = 34,
    MK_HEVC_NAL_AUD        = 35,
    MK_HEVC_NAL_EOS_NUT    = 36,
    MK_HEVC_NAL_EOB_NUT    = 37,
    MK_HEVC_NAL_FD_NUT     = 38,
    MK_HEVC_NAL_SEI_PREFIX = 39,
    MK_HEVC_NAL_SEI_SUFFIX = 40,
}MK_HEVC_NALU_TYPE;

typedef struct
{
    //byte 0
    uint8_t TYPE:5;
    uint8_t NRI:2;
    uint8_t F:1;
} MK_H264_NALU_HEADER; /**//* 1 BYTES */

typedef struct
{
    //byte 0
    uint8_t LATERID0:1;
    uint8_t TYPE:6;
    uint8_t F:1;
    //byte 1
    uint8_t TID:3;
    uint8_t LATERID1:5;
} MK_H265_NALU_HEADER; /**//* 2 BYTES */

int mk_h264_decode_sps(char * buf, unsigned int nLen, int &width, int &height, int &fps);

int mk_hevc_decode_sps(char * buf, unsigned int nLen, int &width, int &height, int &fps);

#endif
