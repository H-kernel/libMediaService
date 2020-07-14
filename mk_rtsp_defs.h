#ifndef __RTSPDEFS_H__
#define __RTSPDEFS_H__

#define RTSP_CONNECTION_DEFAULT 16 

#define RTP_RECV_BUF_SIZE       1500
#define RTP_RECV_BUF_COUNT      (100*RTSP_CONNECTION_DEFAULT)

#define FRAME_RECV_BUF_SIZE     (1024*1024)
#define FRAME_RECV_BUF_COUNT    (2*RTSP_CONNECTION_DEFAULT)

#define RTP_RTCP_START_PORT     10000
#define RTP_RTCP_PORT_COUNT     (4*RTSP_CONNECTION_DEFAULT)


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

#define RTSP_SERVER_AGENT            "AllCam adapter v1.0.0.0"

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

#define RTSP_VERSION                "RTSP/1.0"
#define RTSP_END_LINE               "\r\n"
#define RTSP_END_MSG                "\r\n\r\n"
#define RTSP_URL_PROTOCOL           "rtsp://"
#define RTSP_DEFAULT_PORT            554

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


typedef enum _enRtspMethodType
{
    RTSP_METHOD_OPTIONS = 0,
    RTSP_METHOD_DESCRIBE,
    RTSP_METHOD_SETUP,
    RTSP_METHOD_PLAY,
    RTSP_METHOD_RECORD,
    RTSP_METHOD_PAUSE,
    RTSP_METHOD_TEARDOWN,
    RTSP_METHOD_ANNOUNCE,
    RTSP_METHOD_GETPARAMETER,
    RTSP_METHOD_SETPARAMETER,

    RTSP_METHOD_RESPONSE,
    RTSP_REQ_METHOD_NUM
}RtspMethodType;


#define RTSP_METHOD_STRING \
{\
    "OPTIONS",\
    "DESCRIBE",\
    "SETUP",\
    "PLAY",\
    "RECORD",\
    "PAUSE",\
    "TEARDOWN",\
    "ANNOUNCE",\
    "GET_PARAMETER",\
    "SET_PARAMETER",\
    "RTSP/1.0"\
}


typedef enum
{
    RTSP_CONTINUE                           = 0,        /*100*/
    RTSP_SUCCESS_CREATED                    = 2,        /*201*/
    RTSP_SUCCESS_OK                         = 1,        /*200*/
    RTSP_SUCCESS_ACCEPTED                   = 3,        /*202*/
    RTSP_SUCCESS_NOCONTENT                  = 4,        /*203*/
    RTSP_SUCCESS_PARTIALCONTENT             = 5,        /*204*/
    RTSP_SUCCESS_LOWONSTORAGE               = 6,        /*250*/
    RTSP_MULTIPLE_CHOICES                   = 7,        /*300*/
    RTSP_REDIRECT_PERMMOVED                 = 8,        /*301*/
    RTSP_REDIRECT_TEMPMOVED                 = 9,        /*302*/
    RTSP_REDIRECT_SEEOTHER                  = 10,       /*303*/
    RTSP_REDIRECT_NOTMODIFIED               = 11,       /*304*/
    RTSP_USEPROXY                           = 12,       /*305*/
    RTSP_CLIENT_BAD_REQUEST                 = 13,       /*400*/
    RTSP_CLIENT_UNAUTHORIZED                = 14,       /*401*/
    RTSP_PAYMENT_REQUIRED                   = 15,       /*402*/
    RTSP_CLIENT_FORBIDDEN                   = 16,       /*403*/
    RTSP_CLIENT_NOTFOUND                    = 17,       /*404*/
    RTSP_CLIENT_METHOD_NOTALLOWED           = 18,       /*405*/
    RTSP_NOTACCEPTABLE                      = 19,       /*406*/
    RTSP_PROXY_AUTHENTICATION_REQUIRED      = 20,       /*407*/
    RTSP_REQUEST_TIMEOUT                    = 21,       /*408*/
    RTSP_CLIENT_CONFLICT                    = 22,       /*409*/
    RTSP_GONE                               = 23,       /*410*/
    RTSP_LENGTH_REQUIRED                    = 24,       /*411*/
    RTSP_PRECONDITION_FAILED                = 25,       /*412*/
    RTSP_REQUEST_ENTITY_TOO_LARGE           = 26,       /*413*/
    RTSP_REQUEST_URI_TOO_LARGE              = 27,       /*414*/
    RTSP_UNSUPPORTED_MEDIA_TYPE             = 28,       /*415*/
    RTSP_CLIENT_PARAMETER_NOTUNDERSTOOD     = 29,       /*451*/
    RTSP_CLIENT_CONFERENCE_NOTFOUND         = 30,       /*452*/
    RTSP_CLIENT_NOTENOUGH_BANDWIDTH         = 31,       /*453*/
    RTSP_CLIENT_SESSION_NOTFOUND            = 32,       /*454*/
    RTSP_CLIENT_METHOD_NOTVALID_INSTATE     = 33,       /*455*/
    RTSP_CLIENT_HEADER_FIELD_NOTVALID       = 34,       /*456*/
    RTSP_CLIENT_INVALID_RANGE               = 35,       /*457*/
    RTSP_CLIENT_READONLY_PARAMETER          = 36,       /*458*/
    RTSP_CLIENT_AGGREGATE_OPTION_NOTALLOWED = 37,       /*459*/
    RTSP_CLIENT_AGGREGATE_OPTION_ALLOWED    = 38,       /*460*/
    RTSP_CLIENT_UNSUPPORTED_TRANSPORT       = 39,       /*461*/
    RTSP_CLIENT_DESTINATION_UNREACHABLE     = 40,       /*462*/
    RTSP_SERVER_INTERNAL                    = 41,       /*500*/
    RTSP_SERVER_NOTIMPLEMENTED              = 42,       /*501*/
    RTSP_SERVER_BAD_GATEWAY                 = 43,       /*502*/
    RTSP_SERVICE_UNAVAILABLE                = 44,       /*503*/
    RTSP_SERVER_GATEWAY_TIMEOUT             = 45,       /*505*/
    RTSP_RTSP_VERSION_NOTSUPPORTED          = 46,       /*504*/
    RTSP_SERVER_OPTION_NOTSUPPORTED         = 47,       /*551*/

    RTSP_STATUS_CODES_BUTT
}RTSP_STATUS_CODE_E;


#define RTSP_CODE_STRING \
{\
    "100 Continue",\
    "200 OK",\
    "201 Created",\
    "202 Accepted",\
    "203 No Content",\
    "204 Partial Content",\
    "250 Low on Storage Space",\
    "300 Multiple Choices",\
    "301 Moved Permanently",\
    "302 Moved Temporarily",\
    "303 See Other",\
    "304 Not Modified",\
    "305 Use Proxy",\
    "400 Bad Request",\
    "401 Unauthorized",\
    "402 Payment Required",\
    "403 Forbidden",\
    "404 Not Found",\
    "405 Method Not Allowed",\
    "406 Not Acceptable",\
    "407 Proxy Authentication Required",\
    "408 Request Time-out",\
    "409 Conflict",\
    "410 Gone",\
    "411 Length Required",\
    "412 Precondition Failed",\
    "413 Request Entity Too Large",\
    "414 Request-URI Too Large",\
    "415 Unsupported Media Type",\
    "451 Parameter Not Understood",\
    "452 Conference Not Found",\
    "453 Not Enough Bandwidth",\
    "454 Session Not Found",\
    "455 Method Not Valid in this State",\
    "456 Header Field Not Valid For Resource",\
    "457 Invalid Range",\
    "458 Parameter Is Read-Only",\
    "459 Aggregate Option Not Allowed",\
    "460 Only Aggregate Option Allowed",\
    "461 Unsupported Transport",\
    "462 Destination Unreachable",\
    "500 Internal Server Error",\
    "501 Not Implemented",\
    "502 Bad Gateway",\
    "503 Service Unavailable",\
    "504 Gateway Timeout",\
    "505 RTSP Version not supported",\
    "551 Option Not Supported"\
}



#endif /*__RTSPDEFS_H__ */
