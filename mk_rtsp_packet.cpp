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
#include "ms_engine_rtsp_packet.h"
#include "svs_log.h"
#include "ms_engine_common.h"
#include "ms_engine_def.h"
#include "ms_engine_vms_media.h"
using namespace std;
uint32_t CRtspPacket::m_unRtspCseq = 1;

string CRtspPacket::m_strRtspMethods[] =
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

string CRtspPacket::m_strRtspHeaders[] =
{
    string("CSeq"),
    string("User-Agent"),
    string("Session"),
    string("Range"),
    string("Scale"),
    string("Speed"),
    string("Content-Length"),
    string("Transport"),
    string("Content-Type"),

    string("RTP-Info"),
    string("x-NAT-Info"),
    string("X-PLAYINFO"),
    string("X-PLAYCTRL")
};

string CRtspPacket::m_strRtspStatusCode[] =
{
    string("200 OK"),
    string("300 Multiple Choices"),
    string("400 Bad Request"),
    string("401 Unauthorized"),
    string("404 Not Found"),
    string("406 Not Acceptable"),
    string("500 Internal Server Error"),

    string("Unknown")
};

CRtspPacket::CRtspPacket()
{
    memset(&m_RtspCommonInfo, 0x0, sizeof(m_RtspCommonInfo));
    m_RtspCommonInfo.MethodIndex     = RtspIllegalMethod;
    m_RtspCommonInfo.StatusCodeIndex = RtspNotAcceptedStatus;

    memset(&m_RtspNatInfo, 0x0, sizeof(m_RtspNatInfo));
    m_RtspNatInfo.NatType  = RTSP_INVALID_NAT_TYPE;

    m_dSpeed        = 0;
    m_dScale        = 0;

    m_strXPlayCtrl  = "";
    m_strXPlayInfo  = "";

    m_strRtpInfo    = "";
    m_ulContenLength= 0;
    m_strContentType= "";
}

CRtspPacket::~CRtspPacket()
{
}


int32_t CRtspPacket::checkRtsp(const char* pszRtsp, uint32_t unRtspSize, uint32_t &unMsgLen)
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
        return -1;
    }

    string strRtspMsg;
    strRtspMsg.append(pszRtsp, unRtspSize);
    string::size_type endPos = strRtspMsg.find(RTSP_END_MSG);
    if (string::npos == endPos)
    {
        if (RTSP_MSG_LENGTH <= unRtspSize)
        {
            AS_LOG(AS_LOG_WARNING,"msg len [%d] is too int32_t.", unRtspSize);
            return -1;
        }
        else
        {
            unMsgLen = 0;
            return 0;
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
            return 0;
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
        AS_LOG(AS_LOG_WARNING,"parse Content-Length fail.");
        return -1;
    }

    std::string strLength = strContentLen.substr(0, endLine);
    M_COMMON::trimString(strLength);
    uint32_t unContentLen = (uint32_t)atoi(strLength.c_str());

    AS_LOG(AS_LOG_INFO,"need to read extra content: %d.", unContentLen);
    unMsgLen += unContentLen;

    return 0;
}


uint32_t CRtspPacket::getRtspCseqNo()
{
    return m_unRtspCseq++;
}

void CRtspPacket::setRtspStatusCode(uint32_t unRespCode)
{
    if (RtspNotAcceptedStatus <= unRespCode)
    {
        return;
    }

    m_RtspCommonInfo.StatusCodeIndex = unRespCode;
    return;
}

uint32_t CRtspPacket::getRtspStatusCode() const
{
    return m_RtspCommonInfo.StatusCodeIndex;
}


int32_t CRtspPacket::parse(const char* pszRtsp, uint32_t unRtspSize)
{
    if ((NULL == pszRtsp) || (0 == unRtspSize))
    {
        return -1;
    }

    string strRtspLine;
    int32_t nOffset   = 0;

    int32_t nReadSize = readRtspLine(pszRtsp, strRtspLine);
    if (0 >= nReadSize)
    {
        return -1;
    }

    if (0 != parseRtspMethodLine(strRtspLine))
    {
        AS_LOG(AS_LOG_WARNING,"parse rtsp method line fail.");
        return -1;
    }
    nOffset += nReadSize;

    int32_t nRtspHeaderIndex = 0;
    while ((uint32_t)nOffset < unRtspSize)
    {
        nReadSize = readRtspLine(pszRtsp + nOffset, strRtspLine);
        if (0 >= nReadSize)
        {
            return -1;
        }

        if (0 == strRtspLine.size())
        {
            break;
        }
        nOffset += nReadSize;

        nRtspHeaderIndex = parseRtspHeaderIndex(strRtspLine);
        if (0 > nRtspHeaderIndex)
        {
            return -1;
        }

        if (RtspNotAcceptedHeader == nRtspHeaderIndex)
        {
            continue;
        }

        if (0 != parseRtspHeaderValue(nRtspHeaderIndex, strRtspLine))
        {
            return -1;
        }
    }

    if(0 != m_ulContenLength)
    {
        string strRtspMsg;
        strRtspMsg.append(pszRtsp, unRtspSize);
        string::size_type endPos = strRtspMsg.find(RTSP_END_MSG);
        if (string::npos == endPos)
        {
            AS_LOG(AS_LOG_WARNING,"rtsp msg is error.");
            return -1;
        }
        m_strContent = strRtspMsg.substr(endPos+RTSP_END_MSG.size());
    }

    AS_LOG(AS_LOG_DEBUG,"parse rtsp message success.");
    return 0;
}

int32_t CRtspPacket::parseRtspMethodLine(const string& strLine)
{
    AS_LOG(AS_LOG_DEBUG,"parse rtsp method: %s", strLine.c_str());

    int32_t nMethodsIndex = 0;
    for (; nMethodsIndex < RtspIllegalMethod; nMethodsIndex++)
    {
        if (0 == strncmp(strLine.c_str(),
                         m_strRtspMethods[nMethodsIndex].c_str(),
                         m_strRtspMethods[nMethodsIndex].size()))
        {
            AS_LOG(AS_LOG_DEBUG,"parse rtsp method: %s success", m_strRtspMethods[nMethodsIndex].c_str());
            break;
        }
    }

    m_RtspCommonInfo.MethodIndex = (uint32_t)nMethodsIndex;
    if (RtspIllegalMethod == nMethodsIndex)
    {
        AS_LOG(AS_LOG_WARNING,"parse rtsp method line[%s] fail, invalid methods.",
                strLine.c_str());
        return -1;
    }

    if (strLine.length() <= (m_strRtspMethods[nMethodsIndex].size() + 1))
    {
        AS_LOG(AS_LOG_DEBUG,"parse rtsp method ,no response code.");
        return -1;
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
            AS_LOG(AS_LOG_INFO,"not accepted status code[%s].", strLeast.c_str());
        }

        m_RtspCommonInfo.StatusCodeIndex = unStatus;
        AS_LOG(AS_LOG_DEBUG,"parse status code[%s].", m_strRtspStatusCode[unStatus].c_str());
        return 0;
    }

    string::size_type nPos = strLeast.find(RTSP_VERSION);
    if (string::npos == nPos)
    {
        AS_LOG(AS_LOG_WARNING,"parse request url fail[%s], not rtsp version.", strLeast.c_str());
        return -1;
    }

    string strUrl = strLeast.substr(0, nPos);
    trimString(strUrl);
    if (strUrl.size() > RTSP_MAX_URL_LENGTH)
    {
        AS_LOG(AS_LOG_WARNING,"rtsp request url [%s] length[%d] invalid.",
                strUrl.c_str(),
                strUrl.size());
        return -1;
    }

    memcpy(m_RtspCommonInfo.RtspUrl, strUrl.c_str(), strUrl.size());
    AS_LOG(AS_LOG_DEBUG,"parse request url [%s].", strUrl.c_str());

    return 0;
}


int32_t CRtspPacket::parseRtspHeaderIndex(const std::string& strLine) const
{
    int32_t nHeaderIndex = 0;
    for (; nHeaderIndex < RtspNotAcceptedHeader; nHeaderIndex++)
    {
        if (0 == strncasecmp(strLine.c_str(),
                         m_strRtspHeaders[nHeaderIndex].c_str(),
                         m_strRtspHeaders[nHeaderIndex].size()))
        {
            AS_LOG(AS_LOG_DEBUG,"parse rtsp header: %s.", m_strRtspHeaders[nHeaderIndex].c_str());
            break;
        }
    }

    return nHeaderIndex;
}


int32_t CRtspPacket::parseRtspHeaderValue(int32_t nHeaderIndex, const std::string& strLine)
{
    if ((0 > nHeaderIndex) || (RtspNotAcceptedHeader <= nHeaderIndex))
    {
        return -1;
    }

    string strValue = strLine.substr(m_strRtspHeaders[nHeaderIndex].size(),
                                    strLine.size() - m_strRtspHeaders[nHeaderIndex].size());

    string::size_type nPos = strValue.find(":");
    if (string::npos == nPos)
    {
        return -1;
    }
    strValue = strValue.substr(nPos + 1);
    trimString(strValue);

    int32_t nRet = 0;
    switch(nHeaderIndex)
    {
    case RtspCseqHeader:
    {
        m_RtspCommonInfo.Cseq = strtoul(strValue.c_str(), NULL, 0);
        AS_LOG(AS_LOG_DEBUG,"parsed Cseq: [%u]", m_RtspCommonInfo.Cseq);
        break;
    }
    case RtspSessionHeader:
    {
        m_RtspCommonInfo.SessionID = strtoull(strValue.c_str(), NULL, 0);
        AS_LOG(AS_LOG_DEBUG,"parsed SessionID: [%lld]", m_RtspCommonInfo.SessionID);
        break;
    }
    case RtspXNatInfoHeader:
    {
        if (0 == strValue.size())
        {
            nRet = -1;
            break;
        }

        nRet = parseNatInfo(strValue);
        break;
    }
    case RtspRangeHeader:
    {
        m_strRange = strValue;
        AS_LOG(AS_LOG_DEBUG,"parsed Range: [%s]", m_strRange.c_str());
        break;
    }
    case RtspSpeedHeader:
    {
        m_dSpeed    = atof(strValue.c_str());
        AS_LOG(AS_LOG_DEBUG,"parsed Speed: [%f]", m_dSpeed);
        break;
    }
    case RtspScaleHeader:
    {
        m_dScale    = atof(strValue.c_str());
        AS_LOG(AS_LOG_DEBUG,"parsed Scale: [%f]", m_dScale);
        break;
    }
    case RtspContentLengthHeader:
    {
        m_ulContenLength    = atoi(strValue.c_str());
        AS_LOG(AS_LOG_DEBUG,"parsed Content length: [%d]", m_ulContenLength);
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
    case RtspXPlayInfoHeader:
    {
        m_strXPlayInfo = strValue;
        AS_LOG(AS_LOG_DEBUG,"parsed XPlayInfo: [%s]", m_strXPlayInfo.c_str());
        break;
    }
    default:
        break;
    }

    return nRet;
}

int32_t CRtspPacket::parseRtspUrl(const std::string &strUrl, RTSP_URL_INFO &urlInfo) const
{
    string::size_type nPos = strUrl.find(RTSP_URL_PROTOCOL);
    if (string::npos == nPos)
    {
        AS_LOG(AS_LOG_WARNING,"parse rtsp url[%s] fail, can't find(%s).",
                strUrl.c_str(),
                RTSP_URL_PROTOCOL.c_str());
        return -1;
    }

    string strLeastUrl = strUrl.substr(RTSP_URL_PROTOCOL.size());
    nPos = strLeastUrl.find_first_of("/");
    if (string::npos == nPos)
    {
        AS_LOG(AS_LOG_WARNING,"parse rtsp url[%s] fail, can't find (/).",
                strUrl.c_str());
        return -1;
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
        AS_LOG(AS_LOG_WARNING,"parse url[%s] fail, ip[%s] or port[%d] invalid.",
                strUrl.c_str(),
                strIp.c_str(),
                urlInfo.Port);

        return -1;
    }

    nPos = strLeastUrl.find(".");
    if (string::npos == nPos)
    {
        urlInfo.ContentId = strLeastUrl;
        return 0;
    }
    else
    {
        strLeastUrl = strLeastUrl.substr(0, nPos);
        if (RTSP_MAX_DEVID_LENGTH - 1 < strLeastUrl.size())
        {
            AS_LOG(AS_LOG_WARNING,"parse url[%s] fail, devid[%s] invalid.",
                    strUrl.c_str(),
                    strLeastUrl.c_str());

            return -1;
        }
        urlInfo.ContentId = strLeastUrl;
    }

    AS_LOG(AS_LOG_DEBUG,"parse url[%s] success.", strUrl.c_str());
    return 0;
}


int32_t CRtspPacket::readRtspLine(const char* pszMsg, std::string &strLine) const
{
    strLine.clear();
    if (NULL == pszMsg)
    {
        return -1;
    }

    char *pEndPos = strstr((char*)pszMsg, RTSP_END_LINE.c_str());
    if (NULL == pEndPos)
    {
        return -1;
    }

    int32_t nLength = pEndPos - pszMsg;
    strLine.append(pszMsg, (uint32_t)nLength);

    nLength     += (int32_t)RTSP_END_LINE.size();

    AS_LOG(AS_LOG_DEBUG,"read rtsp line: %s", strLine.c_str());

    return nLength;
}


void CRtspPacket::trimString(std::string& srcString) const
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


std::string CRtspPacket::uint64ToStr(uint64_t num) const
{
    char szData[64] = {0};
    snprintf(szData, 64, "%lld", num);
    return szData;
}


std::string CRtspPacket::uint32ToStr(uint32_t num) const
{
    char szData[64] = { 0 };
    snprintf(szData, 64, "%u", num);
    return szData;
}


std::string CRtspPacket::double2Str(double num) const
{
    char szData[16] =  { 0 };
    snprintf(szData, 16, "%6.6f", num);
    return szData;
}

int32_t CRtspPacket::parseNatInfo(std::string& strNatInfo)
{

    if ( ';' != strNatInfo.at(strNatInfo.size() - 1) )
    {
        strNatInfo += ";";
    }

    AS_LOG(AS_LOG_DEBUG,"parse nat info: %s", strNatInfo.c_str());

    string::size_type nPos = strNatInfo.find_first_of(";");
    while (string::npos != nPos)
    {
        string strLine = strNatInfo.substr(0, nPos);
        strNatInfo.erase(0, nPos + 1);

        nPos = strLine.find_first_of("=");
        if (string::npos == nPos)
        {
            return -1;
        }
        string strKey   = strLine.substr(0, nPos);
        string strValue = strLine.substr(nPos + 1);
        trimString(strKey);
        trimString(strValue);

        do
        {
            if (0 == strncasecmp(strKey.c_str(), "type", sizeof("type")))
            {
                if ( 0 == strncasecmp( strValue.c_str(), "RTP", sizeof("RTP") ) )
                {
                    m_RtspNatInfo.NatType = RTSP_NAT_TYPE_RTP;
                    AS_LOG(AS_LOG_DEBUG,"nat type: RTP");
                    break;
                }

                if (0 == strncasecmp(strValue.c_str(), "RTCP", sizeof("RTCP")))
                {
                    m_RtspNatInfo.NatType = RTSP_NAT_TYPE_RTCP;
                    AS_LOG(AS_LOG_DEBUG,"nat type: RTCP");
                    break;
                }

                AS_LOG(AS_LOG_WARNING,"parse nat info fail, invalid nat type[%s].",
                        strValue.c_str());
                return -1;
            }

            if (0 == strncasecmp(strKey.c_str(), "local_addr", sizeof("local_addr")))
            {
                m_RtspNatInfo.LocalIp = (uint32_t)inet_addr(strValue.c_str());
                m_RtspNatInfo.LocalIp = ntohl(m_RtspNatInfo.LocalIp);
                if (0 == m_RtspNatInfo.LocalIp)
                {
                    return -1;
                }

                AS_LOG(AS_LOG_DEBUG,"local_addr: %s", strValue.c_str());
                break;
            }

            if (0 == strncasecmp(strKey.c_str(), "local_port", sizeof("local_port")))
            {
                m_RtspNatInfo.LocalPort = (uint16_t)atoi(strValue.c_str());
                if (0 == m_RtspNatInfo.LocalPort)
                {
                    return -1;
                }

                AS_LOG(AS_LOG_DEBUG,"local_port: %d", m_RtspNatInfo.LocalPort);
                break;
            }

            if (0 == strncasecmp(strKey.c_str(), "src_addr", sizeof("src_addr")))
            {
                m_RtspNatInfo.SrcIp = (uint32_t)inet_addr(strValue.c_str());
                m_RtspNatInfo.SrcIp = ntohl(m_RtspNatInfo.SrcIp);
                if (0 == m_RtspNatInfo.SrcIp)
                {
                    return -1;
                }

                AS_LOG(AS_LOG_DEBUG,"src_addr: %s", strValue.c_str());
                break;
            }

            if (0 == strncasecmp(strKey.c_str(), "src_port", sizeof("src_port")))
            {
                m_RtspNatInfo.SrcPort = (uint16_t) atoi(strValue.c_str());
                if (0 == m_RtspNatInfo.SrcPort)
                {
                    return -1;
                }

                AS_LOG(AS_LOG_DEBUG,"src_port: %d", m_RtspNatInfo.SrcPort);
                break;
            }

            AS_LOG(AS_LOG_WARNING,"parse fail, key[%s] invalid", strKey.c_str());
            return -1;
        }while(1);  //lint !e506

        nPos = strNatInfo.find_first_of(";");
    }

    return 0;
}


uint32_t CRtspPacket::getCseq() const
{
    return m_RtspCommonInfo.Cseq;
}


void CRtspPacket::setCseq(uint32_t unCseq)
{
    m_RtspCommonInfo.Cseq = unCseq;
    return;
}

double CRtspPacket::getSpeed() const
{
    return m_dSpeed;
}

void CRtspPacket::setSpeed(double dSpeed)
{
    m_dSpeed = dSpeed;
    return;
}

double CRtspPacket::getScale() const
{
    return m_dScale;
}

void CRtspPacket::setScale(double dScale)
{
    m_dScale = dScale;
    return;
}


void CRtspPacket::getRtspUrl(std::string &strUrl) const
{
    strUrl.clear();
    strUrl.append(m_RtspCommonInfo.RtspUrl, strlen(m_RtspCommonInfo.RtspUrl));

    return;
}


void CRtspPacket::setRtspUrl(const std::string &strUrl)
{
    memset(m_RtspCommonInfo.RtspUrl, 0x0, RTSP_MAX_URL_LENGTH);

    if (RTSP_MAX_URL_LENGTH < strUrl.size())
    {
        AS_LOG(AS_LOG_WARNING,"set rtsp url fail, rtsp url [%s] length[%d] invalid.",
                strUrl.c_str(),
                strUrl.size());
    }
    else
    {
        (void)strncpy(m_RtspCommonInfo.RtspUrl, strUrl.c_str(), strUrl.size());
    }
    return;
}


uint64_t CRtspPacket::getSessionID() const
{
    return m_RtspCommonInfo.SessionID;
}

RTSP_XPLAYINFO_TYPE CRtspPacket::getXPlayInfoType() const
{
    if ("" == m_strXPlayInfo)
    {
        return RTSP_XPLAYINFO_INVALID_TYPE;
    }

    string::size_type nPos = m_strXPlayInfo.find("CLOSE");
    if (string::npos != nPos)
    {
        return RTSP_XPLAYINFO_TYPE_CLOSE;
    }

    nPos = m_strXPlayInfo.find("EOS");
    if (string::npos != nPos)
    {
        return RTSP_XPLAYINFO_TYPE_EOS;
    }

    nPos = m_strXPlayInfo.find("BOS");
    if (string::npos != nPos)
    {
        return RTSP_XPLAYINFO_TYPE_BOS;
    }

    return RTSP_XPLAYINFO_INVALID_TYPE;
}

void CRtspPacket::setSessionID(uint64_t ullSessionID)
{
    m_RtspCommonInfo.SessionID = ullSessionID;
    return;
}


uint32_t CRtspPacket::getMethodIndex() const
{
    return m_RtspCommonInfo.MethodIndex;
}


void CRtspPacket::setMethodIndex(uint32_t unMethodIndex)
{
    m_RtspCommonInfo.MethodIndex = unMethodIndex;
    return;
}

void CRtspPacket::setXPlayCtrl(const std::string &strXPlayCtrl)
{
    m_strXPlayCtrl = strXPlayCtrl;
}

void CRtspPacket::setXPlayInfo(const std::string &strXPlayInfo)
{
    m_strXPlayInfo = strXPlayInfo;
}


bool CRtspPacket::isResponse() const
{
    return (m_RtspCommonInfo.MethodIndex == (uint32_t)RtspResponseMethod);
}


bool CRtspPacket::hasNetInfo() const
{
    return (m_RtspNatInfo.NatType != RTSP_INVALID_NAT_TYPE);
}


void CRtspPacket::getNatInfo(RTSP_NAT_INFO &info) const
{
    memcpy((char*)&info, &m_RtspNatInfo, sizeof(info));
    return;
}

void CRtspPacket::setNatInfo(const RTSP_NAT_INFO &info)
{
    memcpy((char*)&m_RtspNatInfo, &info, sizeof(info));
    return;
}


void CRtspPacket::clearNatInfo()
{
    m_RtspNatInfo.NatType  = RTSP_INVALID_NAT_TYPE;
    return;
}


int32_t CRtspPacket::generateRtspResp(std::string& strResp)
{
    if (RtspNotAcceptedStatus <= m_RtspCommonInfo.StatusCodeIndex)
    {
        return -1;
    }

    strResp.clear();

    uint32_t unStatusIndex = m_RtspCommonInfo.StatusCodeIndex;

    strResp = RTSP_VERSION + " " + m_strRtspStatusCode[unStatusIndex] + RTSP_END_LINE;


    generateCommonHeader(strResp);


    generateAcceptedHeader(strResp);


    generateNatInfo(strResp);

    if ("" != m_strRtpInfo)
    {
        strResp += m_strRtspHeaders[RtspRtpInfoHeader];
        strResp += ":";
        strResp += m_strRtpInfo;
        strResp += RTSP_END_LINE;
    }

    strResp += RTSP_END_LINE;

    return 0;
}


int32_t CRtspPacket::generateRtspReq(std::string& strReq)
{
    if (RtspIllegalMethod <= m_RtspCommonInfo.MethodIndex)
    {
        return -1;
    }

    strReq.clear();

    strReq = m_strRtspMethods[m_RtspCommonInfo.MethodIndex] + " " + m_RtspCommonInfo.RtspUrl;
    strReq += " ";
    strReq += RTSP_VERSION;
    strReq += RTSP_END_LINE;

    generateCommonHeader( strReq);

    generateAcceptedHeader(strReq);

    generateNatInfo(strReq);

    // X-PLAYCTRL
    if ((RtspSetParameterMethod == m_RtspCommonInfo.MethodIndex)
            && ("" != m_strXPlayCtrl))
    {
        strReq += m_strRtspHeaders[RtspXPlayCtrl];
        strReq += ":";
        strReq += m_strXPlayCtrl;
        strReq += RTSP_END_LINE;
    }

    // X-PLAYINFO
    if ((RtspSetParameterMethod == m_RtspCommonInfo.MethodIndex) && ("" != m_strXPlayInfo))
    {
        strReq += m_strRtspHeaders[RtspXPlayInfoHeader];
        strReq += ":";
        strReq += m_strXPlayInfo;
        strReq += RTSP_END_LINE;
    }

    strReq += RTSP_END_LINE;

    return 0;
}

void CRtspPacket::generateCommonHeader(std::string& strRtsp)
{
    // SessionID
    strRtsp += m_strRtspHeaders[RtspSessionHeader];
    strRtsp += ":";
    strRtsp += uint64ToStr(m_RtspCommonInfo.SessionID);
    strRtsp += RTSP_END_LINE;

    // CSeq
    strRtsp += m_strRtspHeaders[RtspCseqHeader];
    strRtsp += ":";
    strRtsp += uint32ToStr(m_RtspCommonInfo.Cseq);
    strRtsp += RTSP_END_LINE;

    if ("" != m_strRange)
    {
        strRtsp += m_strRtspHeaders[RtspRangeHeader];
        strRtsp += ":";
        strRtsp += m_strRange;
        strRtsp += RTSP_END_LINE;
    }

    // User-Agent
    strRtsp += m_strRtspHeaders[RtspUserAgentHeader];
    strRtsp += ":";
    strRtsp += RTSP_USER_AGENT;
    strRtsp += RTSP_END_LINE;

    return;
}

void CRtspPacket::generateAcceptedHeader(std::string& strRtsp)
{
    if (0 != m_dSpeed)
    {
        strRtsp += m_strRtspHeaders[RtspSpeedHeader];
        strRtsp += ":";
        strRtsp += double2Str(m_dSpeed);
        strRtsp += RTSP_END_LINE;
    }

    if (0 != m_dScale)
    {
        strRtsp += m_strRtspHeaders[RtspScaleHeader];
        strRtsp += ":";
        strRtsp += double2Str(m_dScale);
        strRtsp += RTSP_END_LINE;
    }

    return;
}

void CRtspPacket::generateNatInfo(std::string& strRtsp)
{
    if ( RTSP_INVALID_NAT_TYPE != m_RtspNatInfo.NatType)
    {
        char szData[RTSP_MSG_LENGTH] = {0};
        size_t nOffset      = 0;
        if (RTSP_NAT_TYPE_RTP == m_RtspNatInfo.NatType)
        {
            nOffset += snprintf(szData + nOffset,
                                RTSP_MSG_LENGTH - nOffset,
                                "x-NAT-Info:type=RTP;");        //lint !e737
        }
        else
        {
            nOffset += snprintf(szData + nOffset,
                                RTSP_MSG_LENGTH - nOffset,
                                "x-NAT-Info:type=RTCP;");       //lint !e737
        }

        struct in_addr localIp, srcIp;
        localIp.s_addr = htonl(m_RtspNatInfo.LocalIp);
        nOffset += snprintf(szData + nOffset, RTSP_MSG_LENGTH - nOffset,
                "local_addr=%s;local_port=%d;",
                inet_ntoa(localIp),
                m_RtspNatInfo.LocalPort); //lint !e737

        if ((0 != m_RtspNatInfo.SrcIp) &&(0 != m_RtspNatInfo.SrcPort))
        {
            srcIp.s_addr = htonl(m_RtspNatInfo.SrcIp);
            nOffset += snprintf(szData + nOffset,
                    RTSP_MSG_LENGTH - nOffset,
                    "src_addr=%s;src_port=%d",
                    inet_ntoa(srcIp), m_RtspNatInfo.SrcPort); //lint !e737
        }

        strRtsp += szData;
        strRtsp += RTSP_END_LINE;
    }

    return;
}

int32_t CRtspPacket::getRangeTime(uint32_t &unTimeType,
                              uint32_t &unStartTime,
                              uint32_t &unStopTime) const
{
    if ("" == m_strRange)
    {
        unTimeType  = RELATIVE_TIME;
        unStartTime = 0;
        unStopTime  = 0;

        AS_LOG(AS_LOG_INFO,"no range para in rtsp packet.");
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
            AS_LOG(AS_LOG_WARNING,"get range time fail, no \"-\" field in [%s].",
                    m_strRange.c_str());
            return -1;
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
            AS_LOG(AS_LOG_WARNING,"get range time fail, start time format invalid in [%s].",
                    m_strRange.c_str());
            return -1;
        }
        unStartTime = (uint32_t) mktime(&rangeTm);
        unStopTime  = 0;

        if ("" != strStopTime)
        {
            memset(&rangeTm, 0x0, sizeof(rangeTm));
            pRet = strptime(strStopTime.c_str(), "%Y%m%dT%H%M%S", &rangeTm);
            if (NULL == pRet)
            {
                AS_LOG(AS_LOG_WARNING,"get range time fail, stop time format invalid in [%s].",
                        m_strRange.c_str());
                return -1;
            }
            unStopTime = (uint32_t) mktime(&rangeTm);
        }

        return 0;
    }

    nStartPos = m_strRange.find("npt=");
    if (string::npos == nStartPos)
    {
        AS_LOG(AS_LOG_WARNING,"get range fail, time type not accepted[%s].",
                         m_strRange.c_str());
        return -1;
    }


    unTimeType = RELATIVE_TIME;
    string strTimeRange = m_strRange.substr(nStartPos);
    string::size_type nStopPos = strTimeRange.find_first_of("-");
    if (string::npos == nStopPos)
    {
        AS_LOG(AS_LOG_WARNING,"get range start time fail, no \"-\" field in [%s].",
                m_strRange.c_str());
        return -1;
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

    return 0;
}

void CRtspPacket::setRangeTime(uint32_t unTimeType,
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
void CRtspPacket::getTransPort(std::string& strTransPort) const
{
    strTransPort = m_strTransPort;
    return;
}
void CRtspPacket::setTransPort(std::string& strTransPort)
{
    m_strTransPort = strTransPort;
    return;
}

uint32_t CRtspPacket::getContentLength() const
{
    return m_ulContenLength;
}
void  CRtspPacket::getContentType(std::string &strContentType) const
{
    strContentType = m_strContentType;
}
void  CRtspPacket::getContent(std::string &strContent) const
{
    strContent = m_strContent;
}
void     CRtspPacket::SetContent(std::string &strContent)
{
    m_strContent = strContent;
}


void CRtspPacket::getRtpInfo(std::string &strRtpInfo) const
{
    strRtpInfo = m_strRtpInfo;
    return;
}

void CRtspPacket::setRtpInfo(const std::string &strRtpInfo)
{
    m_strRtpInfo = strRtpInfo;
    return;
}

void CRtspPacket::setRtpInfo(const std::string &strRtpInfoUrl,
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
