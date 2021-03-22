/*
 * RtspPacket.h
 *
 *  Created on: 2016-1-12
 *      Author:
 */

#ifndef RTSPPACKET_H_
#define RTSPPACKET_H_

#include <string>
#include "as.h"

#define RTSP_VERSION             string("RTSP/1.0")
#define RTSP_END_LINE            string("\r\n")
#define RTSP_END_MSG             string("\r\n\r\n")
#define RTSP_USER_AGENT          string("H.Kernel 2020.06.08")
#define RTSP_URL_PROTOCOL        string("rtsp://")
#define RTSP_DEFAULT_PORT        554

#ifndef _RTSP_MSG_TYPE_
#define _RTSP_MSG_TYPE_
#define RTSP_MSG_REQ        0
#define RTSP_MSG_RSP        1
#endif

enum PLAY_TIME_TYPE
{
    ABSOLUTE_TIME,
    RELATIVE_TIME
};

enum _enRTSP_LENGTH_DEFINE
{
    RTSP_MAX_DEVID_LENGTH      = 33,
    RTSP_MSG_LENGTH            = 2048,
    RTSP_MAX_URL_LENGTH        = 512,
    RTSP_SESSION_LENGTH        = 32,
    RTSP_STATUS_CODE_LENGTH    = 3
};

typedef enum _enRtspMethods
{
    RtspDescribeMethod      = 0,
    RtspSetupMethod         = 1,
    RtspTeardownMethod      = 2,
    RtspPlayMethod          = 3,
    RtspPauseMethod         = 4,
    RtspOptionsMethod       = 5,
    RtspAnnounceMethod      = 6,
    RtspGetParameterMethod  = 7,
    RtspSetParameterMethod  = 8,
    RtspRedirectMethod      = 9,
    RtspRecordMethod        = 10,

    RtspResponseMethod      = 11,

    RtspIllegalMethod
}enRtspMethods;

enum _enRtspHeaders
{
    RtspCseqHeader          = 0,
    RtspUserAgentHeader     = 1,
    RtspSessionHeader       = 2,
    RtspRangeHeader         = 3,
    RtspScaleHeader         = 4,
    RtspSpeedHeader         = 5,
    RtspContentLengthHeader = 6,
    RtspTransPortHeader     = 7,
    RtspRtpInfoHeader       = 8,
    RtspContentType         = 9,
    RtspAuthenticate        = 10,
    RtspAuthorization       = 11,

    RtspNotAcceptedHeader
};


enum _enRtspStatusCode
{
    RtspStatus_Index_100          = 0,        // 100 Continue
    RtspStatus_Index_200          = 1,        // 200 OK
    RtspStatus_Index_201          = 2,        // 201 Created
    RtspStatus_Index_250          = 3,        // 250 Low on Storage Space
    RtspStatus_Index_300          = 4,        // 300 Multiple Choices
    RtspStatus_Index_301          = 5,        // 301 Moved Permanently
    RtspStatus_Index_302          = 6,        // 302 Moved Temporarily
    RtspStatus_Index_303          = 7,        // 303 See Other
    RtspStatus_Index_304          = 8,        // 304 Not Modified
    RtspStatus_Index_305          = 9,        // 305 Use Proxy
    RtspStatus_Index_400          = 10,       // 400 Bad Request
    RtspStatus_Index_401          = 11,       // 401 Unauthorized
    RtspStatus_Index_402          = 12,       // 402 Payment Required
    RtspStatus_Index_403          = 13,       // 403 Forbidden
    RtspStatus_Index_404          = 14,       // 404 Not Found
    RtspStatus_Index_405          = 15,       // 405 Method Not Allowed
    RtspStatus_Index_406          = 16,       // 406 Not Acceptable
    RtspStatus_Index_407          = 17,       // 407 Proxy Authentication Required
    RtspStatus_Index_408          = 18,       // 408 Request Time-out
    RtspStatus_Index_410          = 19,       // 410 Gone
    RtspStatus_Index_411          = 20,       // 411 Length Required
    RtspStatus_Index_412          = 21,       // 412 Precondition Failed
    RtspStatus_Index_413          = 22,       // 413 Request Entity Too Large
    RtspStatus_Index_414          = 23,       // 414 Request URI Too Large
    RtspStatus_Index_415          = 24,       // 415 Unsupported Media Type
    RtspStatus_Index_451          = 25,       // 451 Parameter Not Understood
    RtspStatus_Index_452          = 26,       // 452 Conference Not Found
    RtspStatus_Index_453          = 27,       // 453 Not Enough Bandwidth
    RtspStatus_Index_454          = 28,       // 454 Session Not Found
    RtspStatus_Index_455          = 29,       // 455 Method Not Valid in This State
    RtspStatus_Index_456          = 30,       // 456 Header Field Not Valid for Resource
    RtspStatus_Index_457          = 31,       // 457 Invalid Range
    RtspStatus_Index_458          = 32,       // 458 Parameter Is Read-Only
    RtspStatus_Index_459          = 33,       // 459 Aggregate Operation no Allowed
    RtspStatus_Index_460          = 34,       // 460 Only Aggregate Operation Allowed
    RtspStatus_Index_461          = 35,       // 461 Unsupported Transport
    RtspStatus_Index_462          = 36,       // 462 Destination Unreachable
    RtspStatus_Index_500          = 37,       // 500 Internal Server Error
    RtspStatus_Index_501          = 38,       // 501 Not Implemented
    RtspStatus_Index_502          = 39,       // 502 Bad Gateway
    RtspStatus_Index_503          = 40,       // 503 Service Unavailable
    RtspStatus_Index_504          = 41,       // 504 Gateway Time-out
    RtspStatus_Index_505          = 42,       // 505 RTSP Version not Supported
    RtspStatus_Index_551          = 43,       // 551 Option not supported

    RtspNotAcceptedStatus
};


/** RTSP handling */
enum RTSPStatusCode {
    RTSP_STATUS_CONTINUE             = 100,
    RTSP_STATUS_OK                   = 200,
    RTSP_STATUS_CREATED              = 201,
    RTSP_STATUS_LOW_ON_STORAGE_SPACE = 250,
    RTSP_STATUS_MULTIPLE_CHOICES     = 300,
    RTSP_STATUS_MOVED_PERMANENTLY    = 301,
    RTSP_STATUS_MOVED_TEMPORARILY    = 302,
    RTSP_STATUS_SEE_OTHER            = 303,
    RTSP_STATUS_NOT_MODIFIED         = 304,
    RTSP_STATUS_USE_PROXY            = 305,
    RTSP_STATUS_BAD_REQUEST          = 400,
    RTSP_STATUS_UNAUTHORIZED         = 401,
    RTSP_STATUS_PAYMENT_REQUIRED     = 402,
    RTSP_STATUS_FORBIDDEN            = 403,
    RTSP_STATUS_NOT_FOUND            = 404,
    RTSP_STATUS_METHOD               = 405,
    RTSP_STATUS_NOT_ACCEPTABLE       = 406,
    RTSP_STATUS_PROXY_AUTH_REQUIRED  = 407,
    RTSP_STATUS_REQ_TIME_OUT         = 408,
    RTSP_STATUS_GONE                 = 410,
    RTSP_STATUS_LENGTH_REQUIRED      = 411,
    RTSP_STATUS_PRECONDITION_FAILED  = 412,
    RTSP_STATUS_REQ_ENTITY_2LARGE    = 413,
    RTSP_STATUS_REQ_URI_2LARGE       = 414,
    RTSP_STATUS_UNSUPPORTED_MTYPE    = 415,
    RTSP_STATUS_PARAM_NOT_UNDERSTOOD = 451,
    RTSP_STATUS_CONFERENCE_NOT_FOUND = 452,
    RTSP_STATUS_BANDWIDTH            = 453,
    RTSP_STATUS_SESSION              = 454,
    RTSP_STATUS_STATE                = 455,
    RTSP_STATUS_INVALID_HEADER_FIELD = 456,
    RTSP_STATUS_INVALID_RANGE        = 457,
    RTSP_STATUS_RONLY_PARAMETER      = 458,
    RTSP_STATUS_AGGREGATE            = 459,
    RTSP_STATUS_ONLY_AGGREGATE       = 460,
    RTSP_STATUS_TRANSPORT            = 461,
    RTSP_STATUS_UNREACHABLE          = 462,
    RTSP_STATUS_INTERNAL             = 500,
    RTSP_STATUS_NOT_IMPLEMENTED      = 501,
    RTSP_STATUS_BAD_GATEWAY          = 502,
    RTSP_STATUS_SERVICE              = 503,
    RTSP_STATUS_GATEWAY_TIME_OUT     = 504,
    RTSP_STATUS_VERSION              = 505,
    RTSP_STATUS_UNSUPPORTED_OPTION   = 551,
};

typedef enum _enRTSP_NAT_TYPE
{
    RTSP_NAT_TYPE_RTP,
    RTSP_NAT_TYPE_RTCP,

    RTSP_INVALID_NAT_TYPE
} RTSP_NAT_TYPE;

typedef enum _enRTSP_XPLAYINFO_TYPE
{
    RTSP_XPLAYINFO_TYPE_EOS,
    RTSP_XPLAYINFO_TYPE_BOS,
    RTSP_XPLAYINFO_TYPE_CLOSE,

    RTSP_XPLAYINFO_INVALID_TYPE
} RTSP_XPLAYINFO_TYPE;


typedef struct _stRtspUrlInfo
{
    uint32_t        Ip;
    uint16_t        Port;
    std::string     ContentId;
} RTSP_URL_INFO;

typedef struct _stRtspCommonInfo
{
    uint32_t        MethodIndex;
    char            RtspUrl[RTSP_MAX_URL_LENGTH];
    uint64_t        SessionID;
    uint32_t        Cseq;
    uint32_t        StatusCodeIndex;
} RTSP_COMMON_INFO;



class mk_rtsp_packet
{
public:
    mk_rtsp_packet();

    virtual ~mk_rtsp_packet();

    static int32_t  checkRtsp(const char* pszRtsp, uint32_t unRtspSize, uint32_t &unMsgLen);

    static uint32_t getRtspCseqNo();

    static std::string& getMethodString(enRtspMethods method);

    int32_t   parse(const char* pszRtsp, uint32_t unRtspSize);

    uint32_t  getCseq() const;

    void      setCseq(uint32_t unCseq);

    double    getSpeed() const;

    void      setSpeed(double dSpeed);

    double    getScale() const;

    void      setScale(double dScale);

    void      getRtspUrl(std::string &strUrl) const;

    void      setRtspStatusCode(uint32_t unRespCode);

    uint32_t  getRtspStatusCodeIndex() const;

    uint32_t  getRtspStatusCode() const;

    std::string& getRtspStatusString() const;

    void      setRtspUrl(const std::string &strUrl);

    int32_t   parseRtspUrl(const std::string &strUrl, RTSP_URL_INFO &urlInfo) const;

    uint64_t  getSessionID() const;

    void      setSessionID(uint64_t ullSessionID);

    uint32_t  getMethodIndex() const;

    void      setMethodIndex(uint32_t unMethodIndex);

    bool      isResponse() const;

    void      getRtpInfo(std::string &strRtpInfo) const;

    void      setRtpInfo(const std::string &strRtpInfo);

    void      setRtpInfo(const std::string &strRtpInfoUrl,const uint32_t &unSeq,const uint32_t &unTimestamp);

    int32_t   getRangeTime(uint32_t &unTimeType,uint32_t &unStartTime,uint32_t &unStopTime) const;

    void      setRangeTime(uint32_t unTimeType,uint32_t unStartTime,uint32_t unStopTime);

    void      getTransPort(std::string& strTransPort) const;

    void      setTransPort(std::string& strTransPort);

    uint32_t  getContentLength() const;

    void      getContentType(std::string &strContentType) const;

    void      getContent(std::string &strContent) const;

    void      SetContent(std::string &strContent);
    /* WWW-Authenticate */
    int32_t   getAuthenticate(std::string &strAuthenticate);

    void      setAuthenticate(std::string &strAuthenticate);
    /* Authorization */
    int32_t   getAuthorization(std::string &strAuthorization);

    void      setAuthorization(std::string &strAuthorization);

    int32_t   generateRtspResp(std::string& strResp);

    int32_t   generateRtspReq(std::string& strReq);

private:
    void      generateCommonHeader(std::string& strRtsp);

    void      generateAcceptedHeader(std::string& strRtsp);

    int32_t   parseRtspMethodLine(const std::string& strLine);

    int32_t   parseRtspHeaderIndex(const std::string& strLine) const;

    int32_t   parseRtspHeaderValue(int32_t nHeaderIndex, const std::string& strLine);

    int32_t   readRtspLine(const char* pszMsg, std::string &strLine) const;
private:
    static void  trimString(std::string& srcString);

    std::string uint64ToStr(uint64_t num) const;

    std::string uint32ToStr(uint32_t num) const;

    std::string double2Str(double num) const;

	int32_t strparse2time(std::string& time, uint32_t& ulTime) const;

private:
    static std::string  m_strRtspMethods[];
    static std::string  m_strRtspHeaders[];
    static std::string  m_strRtspStatusCode[];
    static uint32_t     m_ulRtspStatusCode[];
    static uint32_t     m_unRtspCseq;

    RTSP_COMMON_INFO    m_RtspCommonInfo;
    uint32_t            m_ulContenLength;
    std::string         m_strContentType;
    std::string         m_strContent;

    std::string         m_strRange;
    double              m_dSpeed;
    double              m_dScale;
    
    std::string         m_strAuthenticate;
    std::string         m_strAuthorization;
    std::string         m_strRtpInfo;
    std::string         m_strTransPort;
};

#endif /* RTSPPACKET_H_ */
