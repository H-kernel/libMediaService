
#ifndef __MK_RTSP_RTP_HEAD_INCLUDE_H__
#define __MK_RTSP_RTP_HEAD_INCLUDE_H__
#include "as.h"

typedef struct
{
    /**//* byte 0 */
    uint8_t csrc_len:4;       /**//* expect 0 */
    uint8_t extension:1;      /**//* expect 1, see RTP_OP below */
    uint8_t padding:1;        /**//* expect 0 */
    uint8_t version:2;        /**//* expect 2 */
    /**//* byte 1 */
    uint8_t payload:7;        /**//* RTP_PAYLOAD_RTSP */
    uint8_t marker:1;         /**//* expect 1 */
    /**//* bytes 2, 3 */
    uint16_t seq_no;
    /**//* bytes 4-7 */
    uint32_t timestamp;
    /**//* bytes 8-11 */
    uint32_t ssrc;             /**//* stream number is used here. */
} RTP_FIXED_HEADER;

typedef struct
{
    /**//* byte 0 */
    uint8_t   Count:5;
    uint8_t   Padding:1;
    uint8_t   Version:2;
    /**//* byte 1 */
    uint8_t   PacketType;

    /**//* bytes 2, 3 */
    uint16_t  Length;
} RTCP_FIXED_HEADER;

#define RTP_INTERLEAVE_FLAG    '$'
#define KILO                  1024 



const uint32_t RTP_RTCP_DUMMY   = 0xFEEDFACE;
#define DUMMYSIZE    sizeof(uint32_t)

#endif /* __MK_RTSP_RTP_HEAD_INCLUDE_H__ */
