/*
 * RtspPacket.h
 *
 *  Created on: 2016-1-12
 *      Author:
 */

#ifndef RTSPPACKET_H_
#define RTSPPACKET_H_

#include <string>

#define RTSP_VERSION             string("RTSP/1.0")
#define RTSP_END_LINE            string("\r\n")
#define RTSP_END_MSG             string("\r\n\r\n")
#define RTSP_USER_AGENT          string("AllCam adapter/v1.0.0.0")
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

    RtspNotAcceptedHeader
};

enum _enRtspStatusCode
{
    RtspStatus_200          = 0,        // 200 OK
    RtspStatus_300          = 1,        // 300 Multiple Choices
    RtspStatus_400          = 2,        // 400 Bad Request
    RtspStatus_401          = 3,        // 400 Bad Request
    RtspStatus_404          = 4,        // 404 Not Found
    RtspStatus_406          = 5,        // 406 Not Acceptable
    RtspStatus_500          = 6,        // 500 Internal Server Error

    RtspNotAcceptedStatus
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

    int32_t   parse(const char* pszRtsp, uint32_t unRtspSize);

    uint32_t  getCseq() const;

    void      setCseq(uint32_t unCseq);

    double    getSpeed() const;

    void      setSpeed(double dSpeed);

    double    getScale() const;

    void      setScale(double dScale);

    void      getRtspUrl(std::string &strUrl) const;

    void      setRtspStatusCode(uint32_t unRespCode);

    uint32_t  getRtspStatusCode() const;

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

private:
    static std::string  m_strRtspMethods[];
    static std::string  m_strRtspHeaders[];
    static std::string  m_strRtspStatusCode[];
    static uint32_t     m_unRtspCseq;

    RTSP_COMMON_INFO    m_RtspCommonInfo;
    uint32_t            m_ulContenLength;
    std::string         m_strContentType;
    std::string         m_strContent;

    std::string         m_strRange;
    double              m_dSpeed;
    double              m_dScale;

    std::string         m_strRtpInfo;
    std::string         m_strTransPort;
};

#endif /* RTSPPACKET_H_ */
