#ifndef __RTSPDEFS_H__
#define __RTSPDEFS_H__

#define RTSP_CONNECTION_DEFAULT 2000 

#define DEF_RTSP_SEND_MSG_TRY_INTERVAL 50

#define DEF_RTSP_SEND_MSG_WAIT_TIME    (1 * UNIT_SECOND2MS)

#define MAX_RTSP_MSG_LEN             4096

#define MAX_BYTES_PER_RECEIVE        524288 /*(512*1024) */

#define RTSP_INTERLEAVE_FLAG         0x24

#define RTSP_INTERLEAVE_HEADER_LEN   4

#define RTSP_INTERLEAVE_FLAG_LEN     1

#define RTSP_INTERLEAVE_CHID_LEN     1

#define RTSP_INTERLEAVE_DATA_AMMOUNT_LEN 2

#define RTSP_SUCCESS_CODE            200

#define RTSP_REDIRECT_CODE           302

#define RTSP_INVALID_RANGE           457

#define RTSP_PROTOCOL_VERSION        "RTSP/1.0"

#define RTSP_PROTOCOL_HEADER         "rtsp://"

#define RTSP_END_TAG                 "\r\n"

#define MAX_DIGIT_LEN                128

#define MAX_RTSP_TRANSPORT_LEN       1024

#define SIGN_SEMICOLON               ";"

#define SIGN_SLASH                   "/"

#define SIGN_COLON                   ":"

#define SIGN_H_LINE                  "-"

#define SIGN_MINUS                   SIGN_H_LINE

#define RTSP_TRANSPORT_RTP           "RTP"

#define RTSP_TRANSPORT_PROFILE_AVP   "AVP"

#define RTSP_TRANSPORT_MP2T          "MP2T"

#define RTSP_TRANSPORT_TS_OVER_RTP   "MP2T/RTP"

#define RTSP_TRANSPORT_TCP           "TCP"

#define RTSP_TRANSPORT_SPEC_SPLITER  SIGN_SLASH

#define RTSP_TRANSPORT_CLIENT_PORT   "client_port="

#define RTSP_TRANSPORT_SERVER_PORT   "server_port="

#define RTSP_TRANSPORT_SOURCE        "source="

#define RTSP_TRANSPORT_DESTINATIION  "destination="

#define RTSP_TRANSPORT_INTERLEAVED   "interleaved="

#define RTSP_TRANSPORT_SSRC          "ssrc="

#define RTSP_TRANSPORT_UNICAST       "unicast"

#define RTSP_TRANSPORT_MULTICAST     "multicast"

#define RTSP_TRANSPORT_TTL           "ttl="

#define RTSP_RANGE_NPT               "npt="

#define RTSP_RANGE_CLOCK             "clock="

#define RTSP_RANGE_NOW               "now"

#define RTSP_RANGE_BEGINNING         "beginning"

#define RTSP_RANGE_UTC_SEC_DOT       "."

#define RTSP_PLAY_SEEK               "playseek="

#define RTSP_RANGE_SPLITER           SIGN_H_LINE

#define RTSP_RANGE_PRECISION         6

#define RTSP_PLAY_SEEK_TIME_LEN      14

#define URL_SPLITER                  SIGN_SLASH

#define RTSP_TOKEN_STR_CSEQ          "CSeq: "
#define RTSP_TOKEN_STR_ACCEPT        "Accept: "
#define RTSP_TOKEN_STR_USERAGENT     "User-Agent: "
#define RTSP_TOKEN_STR_DATE          "Date: "
#define RTSP_TOKEN_STR_SERVER        "Server: "
#define RTSP_TOKEN_STR_PUBLIC        "Public: "
#define RTSP_TOKEN_STR_SESSION       "Session: "
#define RTSP_TOKEN_STR_CONTENT_LENGTH "Content-Length: "
#define RTSP_TOKEN_STR_CONTENT_TYPE  "Content-Type: "
#define RTSP_TOKEN_STR_CONTENT_BASE  "Content-Base: "
#define RTSP_TOKEN_STR_LOCATION      "Location: "
#define RTSP_TOKEN_STR_TRANSPORT     "Transport: "
#define RTSP_TOKEN_STR_RANGE         "Range: "
#define RTSP_TOKEN_STR_SCALE         "Scale: "
#define RTSP_TOKEN_STR_SPEED         "Speed: "
#define RTSP_TOKEN_STR_RTPINFO       "RTP-Info: "


#define RTSP_CONTENT_SDP              "application/sdp"
#define RTSP_CONTENT_PARAM            "text/parameters"

#define RTSP_MAX_TIME_LEN                 128

#define CURTIMESTR(s, len) \
{\
    struct tm tms;\
    time_t vtime;\
    time (&vtime);\
    gmtime_r (&vtime, &tms);\
    strftime (s, len, "%a %b %d %H:%M:%S %Y GMT", &tms);\
}


enum RTSP_PT_TYPE_VALUE
{
    RTSP_PT_TYPE_PCMU  = 0,
    RTSP_PT_TYPE_PCMA  = 8,
    RTSP_PT_TYPE_MJPEG = 26,
    RTSP_PT_TYPE_PS    = 96,
    RTSP_PT_TYPE_MPEG4 = 97,
    RTSP_PT_TYPE_H264  = 98,
    RTSP_PT_TYPE_H265  = 99,
    RTSP_PT_TYPE_AAC   = 100,
    RTSP_PT_TYPE_MAX   = 0xFF,
};

#endif /*__RTSPDEFS_H__ */
