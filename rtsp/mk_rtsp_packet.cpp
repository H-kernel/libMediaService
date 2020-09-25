/*
 * RtspPacket.cpp
 *
 *  Created on: 2016-1-12
 *      Author:
 */
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mk_rtsp_packet.h"
#include "mk_media_common.h"
using namespace std;
uint32_t mk_rtsp_packet::m_unRtspCseq = 1;

string mk_rtsp_packet::m_strRtspMethods[] =
{
        string("DESCRIBE"),
        string("SETUP"),
        string("TEARDOWN"),
        string("PLAY"),
        string("PAUSE"),
        string("OPTIONS"),
        string("ANNOUNCE"),
        string("GET_PARAMETER"),
        string("SET_PARAMETER"),
        string("REDIRECT"),
        string("RECORD"),

        RTSP_VERSION
};

string mk_rtsp_packet::m_strRtspHeaders[] =
{
    string("CSeq"),
    string("User-Agent"),
    string("Session"),
    string("Range"),
    string("Scale"),
    string("Speed"),
    string("Content-Length"),
    string("Transport"),
    string("RTP-Info"),
    string("Content-Type"),
    string("WWW-Authenticate"),
    string("Authorization"),     
};

uint32_t mk_rtsp_packet::m_ulRtspStatusCode[] =
{
    RTSP_STATUS_CONTINUE,
    RTSP_STATUS_OK,
    RTSP_STATUS_CREATED,
    RTSP_STATUS_LOW_ON_STORAGE_SPACE,
    RTSP_STATUS_MULTIPLE_CHOICES,
    RTSP_STATUS_MOVED_PERMANENTLY,
    RTSP_STATUS_MOVED_TEMPORARILY,
    RTSP_STATUS_SEE_OTHER,
    RTSP_STATUS_NOT_MODIFIED,
    RTSP_STATUS_USE_PROXY,
    RTSP_STATUS_BAD_REQUEST,
    RTSP_STATUS_UNAUTHORIZED,
    RTSP_STATUS_PAYMENT_REQUIRED,
    RTSP_STATUS_FORBIDDEN,
    RTSP_STATUS_NOT_FOUND,
    RTSP_STATUS_METHOD,
    RTSP_STATUS_NOT_ACCEPTABLE,
    RTSP_STATUS_PROXY_AUTH_REQUIRED,
    RTSP_STATUS_REQ_TIME_OUT,
    RTSP_STATUS_GONE,
    RTSP_STATUS_LENGTH_REQUIRED,
    RTSP_STATUS_PRECONDITION_FAILED,
    RTSP_STATUS_REQ_ENTITY_2LARGE,
    RTSP_STATUS_REQ_URI_2LARGE,
    RTSP_STATUS_UNSUPPORTED_MTYPE,
    RTSP_STATUS_PARAM_NOT_UNDERSTOOD,
    RTSP_STATUS_CONFERENCE_NOT_FOUND,
    RTSP_STATUS_BANDWIDTH,
    RTSP_STATUS_SESSION,
    RTSP_STATUS_STATE,
    RTSP_STATUS_INVALID_HEADER_FIELD,
    RTSP_STATUS_INVALID_RANGE,
    RTSP_STATUS_RONLY_PARAMETER,
    RTSP_STATUS_AGGREGATE,
    RTSP_STATUS_ONLY_AGGREGATE,
    RTSP_STATUS_TRANSPORT,
    RTSP_STATUS_UNREACHABLE,
    RTSP_STATUS_INTERNAL,
    RTSP_STATUS_NOT_IMPLEMENTED,
    RTSP_STATUS_BAD_GATEWAY,
    RTSP_STATUS_SERVICE,
    RTSP_STATUS_GATEWAY_TIME_OUT,
    RTSP_STATUS_VERSION,
    RTSP_STATUS_UNSUPPORTED_OPTION,

    0
};
string mk_rtsp_packet::m_strRtspStatusCode[] =
{
    string("100 Continue"),
    string("200 OK"),
    string("201 Created"),
    string("250 Low on Storage Space"),
    string("300 Multiple Choices"),
    string("301 Moved Permanently"),
    string("302 Moved Temporarily"),
    string("303 See Other"),
    string("304 Not Modified"),
    string("305 Use Proxy"),
    string("400 Bad Request"),
    string("401 Unauthorized"),
    string("402 Payment Required"),
    string("403 Forbidden"),
    string("404 Not Found"),
    string("405 Method Not Allowed"),
    string("406 Not Acceptable"),
    string("407 Proxy Authentication Required"),
    string("408 Request Time-out"),
    string("410 Gone"),
    string("411 Length Required"),
    string("412 Precondition Failed"),
    string("413 Request Entity Too Large"),
    string("414 Request URI Too Large"),
    string("415 Unsupported Media Type"),
    string("451 Parameter Not Understood"),
    string("452 Conference Not Found"),
    string("453 Not Enough Bandwidth"),
    string("454 Session Not Found"),
    string("455 Method Not Valid in This State"),
    string("456 Header Field Not Valid for Resource"),
    string("457 Invalid Range"),
    string("458 Parameter Is Read-Only"),
    string("459 Aggregate Operation no Allowed"),
    string("460 Only Aggregate Operation Allowed"),
    string("461 Unsupported Transport"),
    string("462 Destination Unreachable"),
    string("500 Internal Server Error"),
    string("501 Not Implemented"),
    string("502 Bad Gateway"),
    string("503 Service Unavailable"),
    string("504 Gateway Time-out"),
    string("505 RTSP Version not Supported"),
    string("551 Option not supported"),

    string("Unknown")
};

mk_rtsp_packet::mk_rtsp_packet()
{
    memset(&m_RtspCommonInfo, 0x0, sizeof(m_RtspCommonInfo));
    m_RtspCommonInfo.MethodIndex     = RtspIllegalMethod;
    m_RtspCommonInfo.StatusCodeIndex = RtspNotAcceptedStatus;


    m_dSpeed        = 0;
    m_dScale        = 0;


    m_strRtpInfo      = "";
    m_ulContenLength  = 0;
    m_strContentType  = "";
    m_strAuthenticate = "";
    m_strAuthorization= "";
}

mk_rtsp_packet::~mk_rtsp_packet()
{
}


int32_t mk_rtsp_packet::checkRtsp(const char* pszRtsp, uint32_t unRtspSize, uint32_t &unMsgLen)
{
    int32_t nMethodsIndex = 0;
    for (; nMethodsIndex < RtspIllegalMethod; nMethodsIndex++)
    {
        if (0 == strncmp(pszRtsp,
                m_strRtspMethods[nMethodsIndex].c_str(),
                m_strRtspMethods[nMethodsIndex].size()))
        {
            break;
        }
    }

    if (nMethodsIndex >= RtspIllegalMethod)
    {
        MK_LOG(AS_LOG_WARNING,"rtsp method index:[%d] invalid.", nMethodsIndex)
        return AS_ERROR_CODE_FAIL;
    }

    string strRtspMsg;
    strRtspMsg.append(pszRtsp, unRtspSize);
    string::size_type endPos = strRtspMsg.find(RTSP_END_MSG);
    if (string::npos == endPos)
    {
        if (RTSP_MSG_LENGTH <= unRtspSize)
        {
            MK_LOG(AS_LOG_WARNING,"msg len [%d] is too int32_t.", unRtspSize);
            return AS_ERROR_CODE_FAIL;
        }
        else
        {
            unMsgLen = 0;
            return AS_ERROR_CODE_OK;
        }
    }

    unMsgLen = (uint32_t) endPos;
    unMsgLen += RTSP_END_MSG.size();


    endPos = strRtspMsg.find("Content-Length:");
    if (string::npos == endPos)
    {
        endPos = strRtspMsg.find("Content-length:");
        if (string::npos == endPos)
        {
            return AS_ERROR_CODE_OK;
        }
        else
        {
            char* pMsg = (char*)pszRtsp;
            pMsg[endPos + strlen("Content-")] = 'L';
        }
    }


    string strContentLen = strRtspMsg.substr(endPos + strlen("Content-Length:"));
    string::size_type endLine = strRtspMsg.find(RTSP_END_LINE);
    if (string::npos == endLine)
    {
        MK_LOG(AS_LOG_WARNING,"parse Content-Length fail.");
        return AS_ERROR_CODE_FAIL;
    }

    std::string strLength = strContentLen.substr(0, endLine);
    mk_rtsp_packet::trimString(strLength);
    uint32_t unContentLen = (uint32_t)atoi(strLength.c_str());

    MK_LOG(AS_LOG_INFO,"need to read extra content: %d.", unContentLen);
    unMsgLen += unContentLen;

    return AS_ERROR_CODE_OK;
}


uint32_t mk_rtsp_packet::getRtspCseqNo()
{
    return m_unRtspCseq++;
}
std::string& mk_rtsp_packet::getMethodString(enRtspMethods method)
{
    return m_strRtspMethods[method];
}

void mk_rtsp_packet::setRtspStatusCode(uint32_t unRespCode)
{
    if (RtspNotAcceptedStatus <= unRespCode)
    {
        return;
    }

    m_RtspCommonInfo.StatusCodeIndex = unRespCode;
    return;
}

uint32_t mk_rtsp_packet::getRtspStatusCodeIndex() const
{
    return m_RtspCommonInfo.StatusCodeIndex;
}

uint32_t  mk_rtsp_packet::getRtspStatusCode() const
{
    if(RtspNotAcceptedStatus <= m_RtspCommonInfo.StatusCodeIndex) {
        return AS_ERROR_CODE_OK;
    }
    return m_ulRtspStatusCode[m_RtspCommonInfo.StatusCodeIndex];
}

std::string& mk_rtsp_packet::getRtspStatusString() const
{
    if(RtspNotAcceptedStatus <= m_RtspCommonInfo.StatusCodeIndex) {
        return m_strRtspStatusCode[RtspNotAcceptedStatus];
    }
    return m_strRtspStatusCode[m_RtspCommonInfo.StatusCodeIndex];
}

int32_t mk_rtsp_packet::parse(const char* pszRtsp, uint32_t unRtspSize)
{
    if ((NULL == pszRtsp) || (0 == unRtspSize))
    {
        return AS_ERROR_CODE_FAIL;
    }

    string strRtspLine;
    int32_t nOffset   = 0;

    int32_t nReadSize = readRtspLine(pszRtsp, strRtspLine);
    if (0 >= nReadSize)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if (0 != parseRtspMethodLine(strRtspLine))
    {
        MK_LOG(AS_LOG_WARNING,"parse rtsp method line fail.");
        return AS_ERROR_CODE_FAIL;
    }
    nOffset += nReadSize;

    int32_t nRtspHeaderIndex = 0;
    while ((uint32_t)nOffset < unRtspSize)
    {
        nReadSize = readRtspLine(pszRtsp + nOffset, strRtspLine);
        if (0 >= nReadSize)
        {
            return AS_ERROR_CODE_FAIL;
        }

        if (0 == strRtspLine.size())
        {
            break;
        }
        nOffset += nReadSize;

        nRtspHeaderIndex = parseRtspHeaderIndex(strRtspLine);
        if (0 > nRtspHeaderIndex)
        {
            return AS_ERROR_CODE_FAIL;
        }

        if (RtspNotAcceptedHeader == nRtspHeaderIndex)
        {
            continue;
        }

        if (0 != parseRtspHeaderValue(nRtspHeaderIndex, strRtspLine))
        {
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(0 != m_ulContenLength)
    {
        string strRtspMsg;
        strRtspMsg.append(pszRtsp, unRtspSize);
        string::size_type endPos = strRtspMsg.find(RTSP_END_MSG);
        if (string::npos == endPos)
        {
            MK_LOG(AS_LOG_WARNING,"rtsp msg is error.");
            return AS_ERROR_CODE_FAIL;
        }
        m_strContent = strRtspMsg.substr(endPos+RTSP_END_MSG.size());
    }

    MK_LOG(AS_LOG_DEBUG,"parse rtsp message success.");
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_packet::parseRtspMethodLine(const string& strLine)
{
    MK_LOG(AS_LOG_DEBUG,"parse rtsp method: %s", strLine.c_str());

    int32_t nMethodsIndex = 0;
    for (; nMethodsIndex < RtspIllegalMethod; nMethodsIndex++)
    {
        if (0 == strncmp(strLine.c_str(),
                         m_strRtspMethods[nMethodsIndex].c_str(),
                         m_strRtspMethods[nMethodsIndex].size()))
        {
            MK_LOG(AS_LOG_DEBUG,"parse rtsp method: %s success", m_strRtspMethods[nMethodsIndex].c_str());
            break;
        }
    }

    m_RtspCommonInfo.MethodIndex = (uint32_t)nMethodsIndex;
    if (RtspIllegalMethod == nMethodsIndex)
    {
        MK_LOG(AS_LOG_WARNING,"parse rtsp method line[%s] fail, invalid methods.",
                strLine.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    if (strLine.length() <= (m_strRtspMethods[nMethodsIndex].size() + 1))
    {
        MK_LOG(AS_LOG_DEBUG,"parse rtsp method ,no response code.");
        return AS_ERROR_CODE_FAIL;
    }

    string strLeast = strLine.substr(m_strRtspMethods[nMethodsIndex].size() + 1);
    trimString(strLeast);
    if (RtspResponseMethod == nMethodsIndex)
    {
        uint32_t unStatus = 0;
        for (; unStatus < RtspNotAcceptedStatus; unStatus++)
        {
            if (0 == strncmp(strLeast.c_str(),
                             m_strRtspStatusCode[unStatus].c_str(),
                             RTSP_STATUS_CODE_LENGTH))
            {
                break;
            }
        }

        if (RtspNotAcceptedStatus <= unStatus)
        {
            MK_LOG(AS_LOG_INFO,"not accepted status code[%s].", strLeast.c_str());
        }

        m_RtspCommonInfo.StatusCodeIndex = unStatus;
        MK_LOG(AS_LOG_DEBUG,"parse status code[%s].", m_strRtspStatusCode[unStatus].c_str());
        return AS_ERROR_CODE_OK;
    }

    string::size_type nPos = strLeast.find(RTSP_VERSION);
    if (string::npos == nPos)
    {
        MK_LOG(AS_LOG_WARNING,"parse request url fail[%s], not rtsp version.", strLeast.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    string strUrl = strLeast.substr(0, nPos);
    trimString(strUrl);
    if (strUrl.size() > RTSP_MAX_URL_LENGTH)
    {
        MK_LOG(AS_LOG_WARNING,"rtsp request url [%s] length[%d] invalid.",
                strUrl.c_str(),
                strUrl.size());
        return AS_ERROR_CODE_FAIL;
    }

    memcpy(m_RtspCommonInfo.RtspUrl, strUrl.c_str(), strUrl.size());
    MK_LOG(AS_LOG_DEBUG,"parse request url [%s].", strUrl.c_str());

    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_packet::parseRtspHeaderIndex(const std::string& strLine) const
{
    int32_t nHeaderIndex = 0;
    for (; nHeaderIndex < RtspNotAcceptedHeader; nHeaderIndex++)
    {
        if (0 == strncasecmp(strLine.c_str(),
                         m_strRtspHeaders[nHeaderIndex].c_str(),
                         m_strRtspHeaders[nHeaderIndex].size()))
        {
            MK_LOG(AS_LOG_DEBUG,"parse rtsp header: %s.", m_strRtspHeaders[nHeaderIndex].c_str());
            break;
        }
    }

    return nHeaderIndex;
}


int32_t mk_rtsp_packet::parseRtspHeaderValue(int32_t nHeaderIndex, const std::string& strLine)
{
    if ((0 > nHeaderIndex) || (RtspNotAcceptedHeader <= nHeaderIndex))
    {
        return AS_ERROR_CODE_FAIL;
    }

    string strValue = strLine.substr(m_strRtspHeaders[nHeaderIndex].size(),
                                    strLine.size() - m_strRtspHeaders[nHeaderIndex].size());

    string::size_type nPos = strValue.find(":");
    if (string::npos == nPos)
    {
        return AS_ERROR_CODE_FAIL;
    }
    strValue = strValue.substr(nPos + 1);
    trimString(strValue);

    int32_t nRet = 0;
    switch(nHeaderIndex)
    {
    case RtspCseqHeader:
    {
        m_RtspCommonInfo.Cseq = strtoul(strValue.c_str(), NULL, 0);
        MK_LOG(AS_LOG_DEBUG,"parsed Cseq: [%u]", m_RtspCommonInfo.Cseq);
        break;
    }
    case RtspSessionHeader:
    {
        nPos = strValue.find(";");
        if (string::npos != nPos)
        {
            strValue = strValue.substr(0,nPos);
        }
        trimString(strValue);
        m_RtspCommonInfo.SessionID = strtoull(strValue.c_str(), NULL, 0);
        MK_LOG(AS_LOG_DEBUG,"parsed SessionID:[%s] [%lld]",strValue.c_str(), m_RtspCommonInfo.SessionID);
        break;
    }

    case RtspRangeHeader:
    {
        m_strRange = strValue;
        MK_LOG(AS_LOG_DEBUG,"parsed Range: [%s]", m_strRange.c_str());
        break;
    }
    case RtspSpeedHeader:
    {
        m_dSpeed    = atof(strValue.c_str());
        MK_LOG(AS_LOG_DEBUG,"parsed Speed: [%f]", m_dSpeed);
        break;
    }
    case RtspScaleHeader:
    {
        m_dScale    = atof(strValue.c_str());
        MK_LOG(AS_LOG_DEBUG,"parsed Scale: [%f]", m_dScale);
        break;
    }
    case RtspContentLengthHeader:
    {
        m_ulContenLength    = atoi(strValue.c_str());
        MK_LOG(AS_LOG_DEBUG,"parsed Content length: [%d]", m_ulContenLength);
        break;
    }
    case RtspContentType:
    {
        if (0 == strValue.size())
        {
            nRet = -1;
            break;
        }
        m_strContentType = strValue;
        break;
    }
    case RtspTransPortHeader:
    {
        if (0 == strValue.size())
        {
            nRet = -1;
            break;
        }
        m_strTransPort = strValue;
        break;
    }
    case RtspRtpInfoHeader:
    {
        if (0 == strValue.size())
        {
            nRet = -1;
            break;
        }

        m_strRtpInfo = strValue;
        break;
    }
    case RtspAuthenticate:
    {
        if (0 == strValue.size())
        {
            nRet = -1;
            break;
        }

        m_strAuthenticate = strValue;
        break;
    }
    case RtspAuthorization:
    {
        if (0 == strValue.size())
        {
            nRet = -1;
            break;
        }

        m_strAuthorization = strValue;
        break;
    }
    default:
        break;
    }

    return nRet;
}

int32_t mk_rtsp_packet::parseRtspUrl(const std::string &strUrl, RTSP_URL_INFO &urlInfo) const
{
    string::size_type nPos = strUrl.find(RTSP_URL_PROTOCOL);
    if (string::npos == nPos)
    {
        MK_LOG(AS_LOG_WARNING,"parse rtsp url[%s] fail, can't find(%s).",
                strUrl.c_str(),
                RTSP_URL_PROTOCOL.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    string strLeastUrl = strUrl.substr(RTSP_URL_PROTOCOL.size());
    nPos = strLeastUrl.find_first_of("/");
    if (string::npos == nPos)
    {
        MK_LOG(AS_LOG_WARNING,"parse rtsp url[%s] fail, can't find (/).",
                strUrl.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    string strIp = strLeastUrl.substr(0, nPos);
    strLeastUrl = strLeastUrl.substr(nPos + 1);

    nPos = strIp.find("@");
    if (string::npos != nPos)
    {
        strIp = strIp.substr(nPos + 1);
    }

    nPos = strIp.find(":");
    if (string::npos == nPos)
    {
        urlInfo.Port = RTSP_DEFAULT_PORT;
    }
    else
    {
        urlInfo.Port = (uint16_t)atoi(strIp.substr(nPos + 1).c_str());
        strIp        = strIp.substr(0, nPos);
    }

    urlInfo.Ip  = (uint32_t)inet_addr(strIp.c_str());
    urlInfo.Ip  = ntohl(urlInfo.Ip);
    if ((0 == urlInfo.Ip) || (0 == urlInfo.Port))
    {
        MK_LOG(AS_LOG_WARNING,"parse url[%s] fail, ip[%s] or port[%d] invalid.",
                strUrl.c_str(),
                strIp.c_str(),
                urlInfo.Port);

        return AS_ERROR_CODE_FAIL;
    }

    nPos = strLeastUrl.find(".");
    if (string::npos == nPos)
    {
        urlInfo.ContentId = strLeastUrl;
        return AS_ERROR_CODE_OK;
    }
    else
    {
        strLeastUrl = strLeastUrl.substr(0, nPos);
        if (RTSP_MAX_DEVID_LENGTH - 1 < strLeastUrl.size())
        {
            MK_LOG(AS_LOG_WARNING,"parse url[%s] fail, devid[%s] invalid.",
                    strUrl.c_str(),
                    strLeastUrl.c_str());

            return AS_ERROR_CODE_FAIL;
        }
        urlInfo.ContentId = strLeastUrl;
    }

    MK_LOG(AS_LOG_DEBUG,"parse url[%s] success.", strUrl.c_str());
    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_packet::readRtspLine(const char* pszMsg, std::string &strLine) const
{
    strLine.clear();
    if (NULL == pszMsg)
    {
        return AS_ERROR_CODE_FAIL;
    }

    char *pEndPos = strstr((char*)pszMsg, RTSP_END_LINE.c_str());
    if (NULL == pEndPos)
    {
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nLength = pEndPos - pszMsg;
    strLine.append(pszMsg, (uint32_t)nLength);

    nLength     += (int32_t)RTSP_END_LINE.size();

    MK_LOG(AS_LOG_DEBUG,"read rtsp line: %s", strLine.c_str());

    return nLength;
}


void mk_rtsp_packet::trimString(std::string& srcString)
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


std::string mk_rtsp_packet::uint64ToStr(uint64_t num) const
{
    char szData[64] = {0};
    snprintf(szData, 64, "%lld", num);
    return szData;
}


std::string mk_rtsp_packet::uint32ToStr(uint32_t num) const
{
    char szData[64] = { 0 };
    snprintf(szData, 64, "%u", num);
    return szData;
}


std::string mk_rtsp_packet::double2Str(double num) const
{
    char szData[16] =  { 0 };
    snprintf(szData, 16, "%6.6f", num);
    return szData;
}


uint32_t mk_rtsp_packet::getCseq() const
{
    return m_RtspCommonInfo.Cseq;
}


void mk_rtsp_packet::setCseq(uint32_t unCseq)
{
    m_RtspCommonInfo.Cseq = unCseq;
    return;
}

double mk_rtsp_packet::getSpeed() const
{
    return m_dSpeed;
}

void mk_rtsp_packet::setSpeed(double dSpeed)
{
    m_dSpeed = dSpeed;
    return;
}

double mk_rtsp_packet::getScale() const
{
    return m_dScale;
}

void mk_rtsp_packet::setScale(double dScale)
{
    m_dScale = dScale;
    return;
}


void mk_rtsp_packet::getRtspUrl(std::string &strUrl) const
{
    strUrl.clear();
    strUrl.append(m_RtspCommonInfo.RtspUrl, strlen(m_RtspCommonInfo.RtspUrl));

    return;
}


void mk_rtsp_packet::setRtspUrl(const std::string &strUrl)
{
    memset(m_RtspCommonInfo.RtspUrl, 0x0, RTSP_MAX_URL_LENGTH);

    if (RTSP_MAX_URL_LENGTH < strUrl.size())
    {
        MK_LOG(AS_LOG_WARNING,"set rtsp url fail, rtsp url [%s] length[%d] invalid.",
                strUrl.c_str(),
                strUrl.size());
    }
    else
    {
        (void)strncpy(m_RtspCommonInfo.RtspUrl, strUrl.c_str(), strUrl.size());
    }
    return;
}


uint64_t mk_rtsp_packet::getSessionID() const
{
    return m_RtspCommonInfo.SessionID;
}

void mk_rtsp_packet::setSessionID(uint64_t ullSessionID)
{
    m_RtspCommonInfo.SessionID = ullSessionID;
    return;
}


uint32_t mk_rtsp_packet::getMethodIndex() const
{
    return m_RtspCommonInfo.MethodIndex;
}


void mk_rtsp_packet::setMethodIndex(uint32_t unMethodIndex)
{
    m_RtspCommonInfo.MethodIndex = unMethodIndex;
    return;
}


bool mk_rtsp_packet::isResponse() const
{
    return (m_RtspCommonInfo.MethodIndex == (uint32_t)RtspResponseMethod);
}


int32_t mk_rtsp_packet::generateRtspResp(std::string& strResp)
{
    if (RtspNotAcceptedStatus <= m_RtspCommonInfo.StatusCodeIndex)
    {
        return AS_ERROR_CODE_FAIL;
    }

    strResp.clear();

    uint32_t unStatusIndex = m_RtspCommonInfo.StatusCodeIndex;

    strResp = RTSP_VERSION + " " + m_strRtspStatusCode[unStatusIndex] + RTSP_END_LINE;


    generateCommonHeader(strResp);


    generateAcceptedHeader(strResp);


    if ("" != m_strRtpInfo)
    {
        strResp += m_strRtspHeaders[RtspRtpInfoHeader];
        strResp += ": ";
        strResp += m_strRtpInfo;
        strResp += RTSP_END_LINE;
    }
    /* WWW-Authenticate */
    if ("" != m_strAuthenticate)
    {
        strResp += m_strRtspHeaders[RtspAuthenticate];
        strResp += ": ";
        strResp += m_strAuthenticate;
        strResp += RTSP_END_LINE;
    }

    strResp += RTSP_END_LINE;

    return AS_ERROR_CODE_OK;
}


int32_t mk_rtsp_packet::generateRtspReq(std::string& strReq)
{
    if (RtspIllegalMethod <= m_RtspCommonInfo.MethodIndex)
    {
        return AS_ERROR_CODE_FAIL;
    }

    strReq.clear();

    strReq = m_strRtspMethods[m_RtspCommonInfo.MethodIndex] + " " + m_RtspCommonInfo.RtspUrl;
    strReq += " ";
    strReq += RTSP_VERSION;
    strReq += RTSP_END_LINE;

    generateCommonHeader( strReq);

    generateAcceptedHeader(strReq);
    /* Authorization */
    if ("" != m_strAuthorization)
    {
        strReq += m_strRtspHeaders[RtspAuthorization];
        strReq += ": ";
        strReq += m_strAuthorization;
        strReq += RTSP_END_LINE;
    }

    strReq += RTSP_END_LINE;

    return AS_ERROR_CODE_OK;
}

void mk_rtsp_packet::generateCommonHeader(std::string& strRtsp)
{
    // SessionID
    strRtsp += m_strRtspHeaders[RtspSessionHeader];
    strRtsp += ": ";
    strRtsp += uint64ToStr(m_RtspCommonInfo.SessionID);
    strRtsp += RTSP_END_LINE;

    // CSeq
    strRtsp += m_strRtspHeaders[RtspCseqHeader];
    strRtsp += ": ";
    strRtsp += uint32ToStr(m_RtspCommonInfo.Cseq);
    strRtsp += RTSP_END_LINE;

    if ("" != m_strRange)
    {
        strRtsp += m_strRtspHeaders[RtspRangeHeader];
        strRtsp += ": ";
        strRtsp += m_strRange;
        strRtsp += RTSP_END_LINE;
    }

    // User-Agent
    strRtsp += m_strRtspHeaders[RtspUserAgentHeader];
    strRtsp += ": ";
    strRtsp += RTSP_USER_AGENT;
    strRtsp += RTSP_END_LINE;

    return;
}

void mk_rtsp_packet::generateAcceptedHeader(std::string& strRtsp)
{
    if (0 != m_dSpeed)
    {
        strRtsp += m_strRtspHeaders[RtspSpeedHeader];
        strRtsp += ": ";
        strRtsp += double2Str(m_dSpeed);
        strRtsp += RTSP_END_LINE;
    }

    if (0 != m_dScale)
    {
        strRtsp += m_strRtspHeaders[RtspScaleHeader];
        strRtsp += ": ";
        strRtsp += double2Str(m_dScale);
        strRtsp += RTSP_END_LINE;
    }

    // Transport
    if("" != m_strTransPort) {
        strRtsp += m_strRtspHeaders[RtspTransPortHeader];
        strRtsp += ": ";
        strRtsp += m_strTransPort;
        strRtsp += RTSP_END_LINE;
    }

    return;
}


int32_t mk_rtsp_packet::getRangeTime(uint32_t &unTimeType,
                              uint32_t &unStartTime,
                              uint32_t &unStopTime) const
{
    if ("" == m_strRange)
    {
        unTimeType  = RELATIVE_TIME;
        unStartTime = 0;
        unStopTime  = 0;

        MK_LOG(AS_LOG_INFO,"no range para in rtsp packet.");
        return 1;
    }

    string::size_type nStartPos = m_strRange.find("clock=");
    if (string::npos != nStartPos)
    {
        unTimeType                  = ABSOLUTE_TIME;
        string strTimeRange         = m_strRange.substr(nStartPos);
        string::size_type nStopPos  = strTimeRange.find_first_of("-");
        if (string::npos == nStopPos)
        {
            MK_LOG(AS_LOG_WARNING,"get range time fail, no \"-\" field in [%s].",
                    m_strRange.c_str());
            return AS_ERROR_CODE_FAIL;
        }

        string strStartTime = strTimeRange.substr(strlen("clock="), nStopPos);
        string strStopTime  = strTimeRange.substr(nStopPos + 1);
        trimString(strStartTime);
        trimString(strStopTime);

        struct tm rangeTm;
        memset(&rangeTm, 0x0, sizeof(rangeTm));
        char* pRet = strptime(strStartTime.c_str(), "%Y%m%dT%H%M%S", &rangeTm);
        if (NULL == pRet)
        {
            MK_LOG(AS_LOG_WARNING,"get range time fail, start time format invalid in [%s].",
                    m_strRange.c_str());
            return AS_ERROR_CODE_FAIL;
        }
        unStartTime = (uint32_t) mktime(&rangeTm);
        unStopTime  = 0;

        if ("" != strStopTime)
        {
            memset(&rangeTm, 0x0, sizeof(rangeTm));
            pRet = strptime(strStopTime.c_str(), "%Y%m%dT%H%M%S", &rangeTm);
            if (NULL == pRet)
            {
                MK_LOG(AS_LOG_WARNING,"get range time fail, stop time format invalid in [%s].",
                        m_strRange.c_str());
                return AS_ERROR_CODE_FAIL;
            }
            unStopTime = (uint32_t) mktime(&rangeTm);
        }

        return AS_ERROR_CODE_OK;
    }

    nStartPos = m_strRange.find("npt=");
    if (string::npos == nStartPos)
    {
        MK_LOG(AS_LOG_WARNING,"get range fail, time type not accepted[%s].",
                         m_strRange.c_str());
        return AS_ERROR_CODE_FAIL;
    }


    unTimeType = RELATIVE_TIME;
    string strTimeRange = m_strRange.substr(nStartPos);
    string::size_type nStopPos = strTimeRange.find_first_of("-");
    if (string::npos == nStopPos)
    {
        MK_LOG(AS_LOG_WARNING,"get range start time fail, no \"-\" field in [%s].",
                m_strRange.c_str());
        return AS_ERROR_CODE_FAIL;
    }

    string strStartTime = strTimeRange.substr(strlen("npt="), nStopPos);
    string strStopTime = strTimeRange.substr(nStopPos + 1);
    trimString(strStartTime);
    trimString(strStopTime);


    unStartTime = (uint32_t) atoi(strStartTime.c_str());
    unStopTime = 0;

    if ("" != strStopTime)
    {
        unStopTime = (uint32_t) atoi(strStopTime.c_str());
    }

    return AS_ERROR_CODE_OK;
}

void mk_rtsp_packet::setRangeTime(uint32_t unTimeType,
                                    uint32_t unStartTime,
                                    uint32_t unStopTime)
{
    char strTime[32] = { 0 };
    if (ABSOLUTE_TIME == unTimeType)
    {
        time_t rangeTime = (time_t) unStartTime;
        struct tm tmv;

        (void) localtime_r(&rangeTime, &tmv);
        (void) snprintf(strTime, 32, "%04d%02d%02dT%02d%02d%02dZ", tmv.tm_year + 1900,
                tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

        m_strRange = "clock=";
        m_strRange += strTime;
        m_strRange += "-";

        if (0 != unStopTime)
        {
            rangeTime = (time_t) unStopTime;
            (void) localtime_r(&rangeTime, &tmv);
            (void) snprintf(strTime, 32, "%04d%02d%02dT%02d%02d%02dZ", tmv.tm_year + 1900,
                    tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

            m_strRange += strTime;
        }
        return;
    }

    m_strRange = "npt=";
    if (0 == unStartTime)
    {
        m_strRange += "now";
    }
    else
    {
        (void) snprintf(strTime, 32, "%u", unStartTime);
        m_strRange += strTime;
    }
    m_strRange += "-";

    if (0 != unStopTime)
    {
        (void) snprintf(strTime, 32, "%u", unStopTime);
        m_strRange += strTime;
    }

    return;
}
void mk_rtsp_packet::getTransPort(std::string& strTransPort) const
{
    strTransPort = m_strTransPort;
    return;
}
void mk_rtsp_packet::setTransPort(std::string& strTransPort)
{
    m_strTransPort = strTransPort;
    return;
}

uint32_t mk_rtsp_packet::getContentLength() const
{
    return m_ulContenLength;
}
void  mk_rtsp_packet::getContentType(std::string &strContentType) const
{
    strContentType = m_strContentType;
}
void  mk_rtsp_packet::getContent(std::string &strContent) const
{
    strContent = m_strContent;
}
void     mk_rtsp_packet::SetContent(std::string &strContent)
{
    m_strContent = strContent;
}

int32_t mk_rtsp_packet::getAuthenticate(std::string &strAuthenticate)
{
    if(0 == m_strAuthenticate.length()) {
        return AS_ERROR_CODE_FAIL;
    }

    strAuthenticate = m_strAuthenticate;
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_packet::setAuthenticate(std::string &strAuthenticate)
{
    m_strAuthenticate = strAuthenticate;
}

int32_t mk_rtsp_packet::getAuthorization(std::string &strAuthorization)
{
    if(0 == m_strAuthorization.length()) {
        return AS_ERROR_CODE_FAIL;
    }

    strAuthorization = m_strAuthorization;
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_packet::setAuthorization(std::string &strAuthorization)
{
    m_strAuthorization = strAuthorization;
}
void mk_rtsp_packet::getRtpInfo(std::string &strRtpInfo) const
{
    strRtpInfo = m_strRtpInfo;
    return;
}

void mk_rtsp_packet::setRtpInfo(const std::string &strRtpInfo)
{
    m_strRtpInfo = strRtpInfo;
    return;
}

void mk_rtsp_packet::setRtpInfo(const std::string &strRtpInfoUrl,
                            const uint32_t &unSeq,
                            const uint32_t &unTimestamp)
{
    m_strRtpInfo.clear();
    m_strRtpInfo = strRtpInfoUrl;
    m_strRtpInfo += ";seq=";
    m_strRtpInfo += uint32ToStr(unSeq);
    m_strRtpInfo += ";rtptime=";
    m_strRtpInfo += uint32ToStr(unTimestamp);
    return;
}
