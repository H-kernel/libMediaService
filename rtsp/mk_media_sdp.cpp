/*
 * RtspSdp.cpp
 *
 *  Created on: 2016-5-20
 *      Author:
 */
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include "mk_media_common.h"
#include "mk_media_sdp.h"
#include "mk_media_common.h"


using namespace std;


mk_media_sdp::mk_media_sdp()
{
    m_unSession     = 0;

    m_strUrl        = "";
    m_strConnAddr   = "";
    m_range         = "";
    m_strSessionName= "hx media kernel";
    m_enTransDirect = TRANS_DIRECTION_MAX;
    m_strSsrc       = "";
    m_strOwner      = "vds";

    m_enParseStatus = SDP_GLOBAL_INFO;

}

mk_media_sdp::~mk_media_sdp()
{
    SDP_MEDIA_INFO* info;
    while(0 < m_VideoInfoList.size())
    {
        info = m_VideoInfoList.front();
        m_VideoInfoList.pop_front();
        AS_DELETE(info);
    }
    while(0 < m_AudioInfoList.size())
    {
        info = m_AudioInfoList.front();
        m_AudioInfoList.pop_front();
        AS_DELETE(info);
    }
}

void mk_media_sdp::setUrl(const std::string &strUrl)
{
    m_strUrl = strUrl;
    return;
}

std::string mk_media_sdp::getUrl() const
{
    return m_strUrl;
}

void mk_media_sdp::setRange(const std::string &strRange)
{
    m_range = strRange;
    return;
}

std::string mk_media_sdp::getRange() const
{
    return m_range;
}

void mk_media_sdp::setSessionName(const std::string &strSessionName)
{
    m_strSessionName = strSessionName;
}

std::string mk_media_sdp::getSessionName() const
{
    return m_strSessionName;
}
void mk_media_sdp::setConnAddr(const std::string &strConnAddr)
{
    m_strConnAddr = strConnAddr;
}

std::string mk_media_sdp::getConnAddr() const
{
    return m_strConnAddr;
}


void mk_media_sdp::getVideoInfo(MEDIA_INFO_LIST &infoList)
{
    MEDIA_INFO_LIST::iterator iter = m_VideoInfoList.begin();
    for(; iter != m_VideoInfoList.end();++iter)
    {
        MK_LOG(AS_LOG_DEBUG,"get video info: ucMediaType:[%d],ucPayloadType:[%d],usPort:[%d]"
                            "strRtpmap:[%s],strFmtp:[%s],strControl:[%s]", 
                            (*iter)->ucMediaType,(*iter)->ucPayloadType,(*iter)->usPort,
                            (*iter)->strRtpmap.c_str(),(*iter)->strFmtp.c_str(),(*iter)->strControl.c_str());
        infoList.push_back(*iter);
    }

    return;
}

void mk_media_sdp::addVideoInfo(const SDP_MEDIA_INFO &info)
{
    SDP_MEDIA_INFO* stVideoInfo = NULL;
    stVideoInfo = AS_NEW(stVideoInfo);
    if(NULL == stVideoInfo) {
        return;
    }
    stVideoInfo->ucPayloadType = INVALID_PAYLOAD_TYPE;
    stVideoInfo->usPort        = 0;
    stVideoInfo->strRtpmap     = "";
    stVideoInfo->strFmtp       = "";
    stVideoInfo->strControl    = "";

    stVideoInfo->ucPayloadType = info.ucPayloadType;
    stVideoInfo->strRtpmap     = info.strRtpmap;
    stVideoInfo->strFmtp       = info.strFmtp;
    stVideoInfo->strControl    = info.strControl;
    stVideoInfo->usPort        = info.usPort;

    if ("" == stVideoInfo->strRtpmap)
    {

        switch(stVideoInfo->ucPayloadType)
        {
            case PT_TYPE_MJPEG:
            {
                stVideoInfo->strRtpmap = MJPEG_VIDEO_RTPMAP;
                break;
            }
            case PT_TYPE_PS:
            {
                stVideoInfo->strRtpmap = PS_VIDEO_RTPMAP;
                break;
            }
            case PT_TYPE_MPEG4:
            {
                stVideoInfo->strRtpmap = MPEG4_VIDEO_RTPMAP;
                break;
            }
            case PT_TYPE_H264:
            {
                stVideoInfo->strRtpmap = H264_VIDEO_RTPMAP;
                break;
            }
            case PT_TYPE_H265:
            {
                stVideoInfo->strRtpmap = H265_VIDEO_RTPMAP;
                break;
            }
            default:
            {
                stVideoInfo->strRtpmap = H264_VIDEO_RTPMAP;
                break;
            }
        }
    }

    m_VideoInfoList.push_back(stVideoInfo);

    return;
}

void mk_media_sdp::getAudioInfo(MEDIA_INFO_LIST &infoList)
{
    MEDIA_INFO_LIST::iterator iter = m_AudioInfoList.begin();
    SDP_MEDIA_INFO info;
    for(; iter != m_AudioInfoList.end();++iter)
    {
        MK_LOG(AS_LOG_DEBUG,"get audio info: ucMediaType:[%d],ucPayloadType:[%d],usPort:[%d]"
                            "strRtpmap:[%s],strFmtp:[%s],strControl:[%s]", 
                            (*iter)->ucMediaType,(*iter)->ucPayloadType,(*iter)->usPort,
                            (*iter)->strRtpmap.c_str(),(*iter)->strFmtp.c_str(),(*iter)->strControl.c_str());
        infoList.push_back(*iter);
    }

    return;
}

void mk_media_sdp::addAudioInfo(const SDP_MEDIA_INFO &info)
{
    SDP_MEDIA_INFO* stAudioInfo = NULL;
    stAudioInfo = AS_NEW(stAudioInfo);
    if(NULL == stAudioInfo) {
        return;
    }
    stAudioInfo->ucPayloadType = INVALID_PAYLOAD_TYPE;
    stAudioInfo->usPort        = 0;
    stAudioInfo->strRtpmap     = "";
    stAudioInfo->strFmtp       = "";
    stAudioInfo->strControl    = "";

    stAudioInfo->ucPayloadType = info.ucPayloadType;
    stAudioInfo->strRtpmap     = info.strRtpmap;
    stAudioInfo->strFmtp       = info.strFmtp;
    stAudioInfo->strControl    = info.strControl;
    stAudioInfo->usPort        = info.usPort;

    if ("" == stAudioInfo->strRtpmap)
    {
        if (PT_TYPE_PCMU == stAudioInfo->ucPayloadType)
        {
            stAudioInfo->strRtpmap = DEFAULT_AUDIO_PCMU;
        }

        if (PT_TYPE_PCMA == stAudioInfo->ucPayloadType)
        {
            stAudioInfo->strRtpmap = DEFAULT_AUDIO_PCMA;
        }
    }

    m_AudioInfoList.push_back(stAudioInfo);

    return;
}
void mk_media_sdp::setTransDirect(TRANS_DIRECTION enTransDirect)
{
    m_enTransDirect = enTransDirect;
}

TRANS_DIRECTION mk_media_sdp::getTransDirect()
{
    return m_enTransDirect;
}

void mk_media_sdp::setSsrc(std::string& strSsrc)
{
    m_strSsrc = strSsrc;
}


void mk_media_sdp::setOwner(std::string& strOwner)
{
    m_strOwner = strOwner;
}


int32_t mk_media_sdp::decodeSdp(const std::string   &strSdp)
{
    if ("" == strSdp)
    {
        MK_LOG(AS_LOG_WARNING,"no sdp info, decode fail.");

        return AS_ERROR_CODE_FAIL;
    }
    MK_LOG(AS_LOG_DEBUG,"start decode sdp:\n%s", strSdp.c_str());

    string strBuff;
    uint32_t nNextStart = 0;
    bool bRet = getNextLine(strSdp, nNextStart, strBuff);

    SDP_MEDIA_INFO *pMediaInfo = NULL;

    char *pszTmp = NULL;
    int32_t nRet = AS_ERROR_CODE_OK;
    bool bFirst = true;
    while (bRet && (AS_ERROR_CODE_OK == nRet))
    {
        //MK_LOG(AS_LOG_DEBUG,"parser line:%s", strBuff.c_str());
        pszTmp = const_cast<char*> (strBuff.c_str());
        JUMP_SPACE(pszTmp);

        if ('\0' == *pszTmp)
        {
            bRet = getNextLine(strSdp, nNextStart, strBuff);
            continue;
        }

        char cCode = *pszTmp;
        pszTmp++;
        JUMP_SPACE(pszTmp);

        if ('=' == *pszTmp)
        {
            pszTmp++;
            JUMP_SPACE(pszTmp);

            if (('v' != tolower(cCode)) && bFirst)
            {
                MK_LOG(AS_LOG_WARNING,"Version is not the first statement.");
                return AS_ERROR_CODE_FAIL;
            }

            switch (tolower(cCode))
            {
            case 's':
                m_strSessionName = pszTmp;
                break;
            case 'c':
                nRet = parseConnDesc(pszTmp);
                break;
            case 't':
                nRet = parseTimeDesc(pszTmp);
                break;
            case 'm':
                nRet = parseMediaDesc(pszTmp,pMediaInfo);
                break;
            case 'a':
                nRet = parseAttributes(pszTmp,pMediaInfo);
                break;
            case 'u':
                m_strUrl.clear();
                m_strUrl = pszTmp;
                break;;
            default:
                break;
            }

            bFirst = false;
        }
        else
        {
            MK_LOG(AS_LOG_WARNING,"parse sdp fail, invalid line[%s].", strBuff.c_str());
            return AS_ERROR_CODE_FAIL;
        }

        bRet = getNextLine(strSdp, nNextStart, strBuff);
    }

    if ((0 == m_VideoInfoList.size())
        && (0 == m_AudioInfoList.size()))
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp fail, no video/audio info.");

        return AS_ERROR_CODE_FAIL;
    }

    MK_LOG(AS_LOG_INFO,"decode sdp success.");
    return nRet;
}

int32_t mk_media_sdp::encodeSdp(std::string   &strSdp,int32_t isplayback,std::string timeRange,time_t startTime,time_t endTime)
{
    strSdp.clear();
    stringstream strValue;

    // v=0
    strSdp = "v=0";
    strSdp += SDP_END_TAG;

    // o=<username> <session id> <version> <network type> <address type> <address>
    strSdp += "o=";
    strSdp += m_strOwner;
    strSdp += " ";
    strValue<<m_unSession;
    strSdp += strValue.str() + " 0 IN IP4 ";
    if(0 < m_strConnAddr.length()) {
        strSdp += m_strConnAddr;
    }
    else {
        strSdp += "vds.allcam.com";
    }
    strSdp += SDP_END_TAG;

    // s=Unnamed
    strSdp += "s=";
    strSdp += m_strSessionName;
    strSdp += SDP_END_TAG;

    if(1 == isplayback)
    {
        strSdp += "u=";
        strSdp += m_strOwner;
        strSdp += ":255";
        strSdp += SDP_END_TAG;
    }

    // c=IN IP4 vds.allcam.com
    if(0 < m_strConnAddr.length()) {
        strSdp += "c=";
        strSdp += "IN IP4 ";
        strSdp += m_strConnAddr;
        strSdp += SDP_END_TAG;
    }

    // t=0 0
    if(1 == isplayback)
    {
        strSdp += "t=";

        std::ostringstream os;  
        os<<startTime;  
        string result;  
        std::istringstream is(os.str());  
        is>>result;  

        strSdp += result;
        strSdp += " ";

        std::ostringstream os_1;  
        os_1<<endTime;  
        string result_1;  
        std::istringstream is_1(os_1.str());  
        is_1>>result_1;
        
        strSdp += result_1;
        strSdp += SDP_END_TAG;
    }
    else{
        strSdp += "t=0 0";
        strSdp += SDP_END_TAG;
    }
    
    // if(1 == isplayback)
    // {
    //     strSdp += "a=";
    //     strSdp += m_range;
    //     strSdp += SDP_END_TAG;

    //     strSdp += "a=length:";
    //     strSdp += timeRange;
    //     strSdp += SDP_END_TAG;

    // }

    // m=video
    if (0 < m_VideoInfoList.size())
    {
        encodeMediaDesc(strSdp, SDP_VIDEO_INFO);
    }

    // m=audio
    if (0 < m_AudioInfoList.size())
    {
        encodeMediaDesc(strSdp, SDP_AUDIO_INFO);
    }

    if(TRANS_DIRECTION_RECVONLY == m_enTransDirect)
    {
        strSdp += "a=" + STR_SDP_RECVONLY_ATTR;
        strSdp += SDP_END_TAG;
    }
    else if(TRANS_DIRECTION_SENDONLY == m_enTransDirect)
    {
        strSdp += "a=" + STR_SDP_SENDONLY_ATTR;
        strSdp += SDP_END_TAG;
    }
    else if(TRANS_DIRECTION_SENDRECV == m_enTransDirect)
    {
        strSdp += "a=" + STR_SDP_SENDRECV_ATTR;
        strSdp += SDP_END_TAG;
    }

    ;
    if(0 < m_strSsrc.length()) {
        strSdp += "y=";
        strSdp += m_strSsrc;
        strSdp += SDP_END_TAG;
    }

    // SDP END
    //strSdp += SDP_END_TAG;
    MK_LOG(AS_LOG_DEBUG,"success to encode sdp:\n%s.", strSdp.c_str());
    return AS_ERROR_CODE_OK;
}

int32_t mk_media_sdp::encode200Sdp(std::string &strSdp, int32_t isplayback, std::string timeRange, long startTime, long endTime, std::string AudioOutputIp, uint16_t AudioOutputport)
{
    strSdp.clear();
    stringstream strValue;

    // v=0
    strSdp = "v=0";
    strSdp += SDP_END_TAG;

    // o=<username> <session id> <version> <network type> <address type> <address>
    strSdp += "o=";
    strSdp += m_strOwner;
    strSdp += " ";
    strValue<<m_unSession;
    strSdp += strValue.str() + " 0 IN IP4 ";
    if(0 < m_strConnAddr.length()) {
        strSdp += m_strConnAddr;
    }
    else {
        strSdp += "vds.allcam.com";
    }
    strSdp += SDP_END_TAG;

    // s=Unnamed
    strSdp += "s=";
    strSdp += "BroadCast";
    strSdp += SDP_END_TAG;

    if(1 == isplayback)
    {
        strSdp += "u=";
        strSdp += m_strOwner;
        strSdp += ":255";
        strSdp += SDP_END_TAG;
    }
    
    strSdp += "c=";
    strSdp += "IN IP4 ";
    strSdp += AudioOutputIp;
    strSdp += SDP_END_TAG;

    // t=0 0
    if(1 == isplayback)
    {
        strSdp += "t=";

        std::ostringstream os;  
        os<<startTime;  
        string result;  
        std::istringstream is(os.str());  
        is>>result;  

        strSdp += result;
        strSdp += " ";

        std::ostringstream os_1;  
        os_1<<endTime;  
        string result_1;  
        std::istringstream is_1(os_1.str());  
        is_1>>result_1;
        
        strSdp += result_1;
        strSdp += SDP_END_TAG;
    }
    else{
        strSdp += "t=0 0";
        strSdp += SDP_END_TAG;
    }
    
    std::ostringstream audioport; 
    audioport<<AudioOutputport;
    string audiostring;
    std::istringstream audioport2(audioport.str());
    audioport2>>audiostring;

    strSdp += "m=";
    strSdp += "audio ";
    strSdp += audiostring;
    strSdp += " RTP/AVP 8";
    strSdp += SDP_END_TAG;   

    strSdp += "a=";
    strSdp += "rtpmap:8 PCMA/8000";
    strSdp += SDP_END_TAG;  
    // if(1 == isplayback)
    // {
    //     strSdp += "a=";
    //     strSdp += m_range;
    //     strSdp += SDP_END_TAG;

    //     strSdp += "a=length:";
    //     strSdp += timeRange;
    //     strSdp += SDP_END_TAG;

    // }

    // m=audio
    //encodeMediaDesc(strSdp, SDP_AUDIO_INFO);


    if(TRANS_DIRECTION_RECVONLY == m_enTransDirect)
    {
        strSdp += "a=" + STR_SDP_RECVONLY_ATTR;
        strSdp += SDP_END_TAG;
    }
    else if(TRANS_DIRECTION_SENDONLY == m_enTransDirect)
    {
        strSdp += "a=" + STR_SDP_SENDONLY_ATTR;
        strSdp += SDP_END_TAG;
    }
    else if(TRANS_DIRECTION_SENDRECV == m_enTransDirect)
    {
        strSdp += "a=" + STR_SDP_SENDRECV_ATTR;
        strSdp += SDP_END_TAG;
    }

    if(0 < m_strSsrc.length()) {
        strSdp += "y=";
        strSdp += m_strSsrc;
        strSdp += SDP_END_TAG;
    }

    //if(m_bSetFormat) {
    //    strSdp += "f=";
     //   strSdp += m_str28181Format;
     //   strSdp += SDP_END_TAG;
    //}

    // SDP END
    //strSdp += SDP_END_TAG;
    MK_LOG(AS_LOG_DEBUG,"success to encode sdp:\n%s.", strSdp.c_str());
    return AS_ERROR_CODE_OK;
}
void mk_media_sdp::copy(mk_media_sdp& rtspSdp)
{
    m_strUrl           =  rtspSdp.getUrl();
    m_range            =  rtspSdp.getRange();
    m_strSessionName   =  rtspSdp.getSessionName();
    m_strConnAddr      =  rtspSdp.getConnAddr();

    rtspSdp.getVideoInfo(m_VideoInfoList);
    rtspSdp.getAudioInfo(m_AudioInfoList);
    m_enParseStatus    =   SDP_GLOBAL_INFO;
}

void mk_media_sdp::makeRtpmap(std::string& strRtpmap,uint8_t ucPT,uint32_t ulClockFre)
{
    char szBuf[STR_SDP_BUF_LEN+1]={0};
    std::string strName;
    switch(ucPT)
    {
        case PT_TYPE_PCMU:
        {
            strName = PCMU_AUDIO;
            break;
        }
        case PT_TYPE_PCMA:
        {
            strName = PCMA_AUDIO;
            break;
        }
        case PT_TYPE_AAC:
        {
            strName = AAC_AUDIO;
            break;
        }

        case PT_TYPE_MJPEG:
        {
            strName = MJPEG_VIDEO;
            break;
        }
        case PT_TYPE_MPEG4:
        {
            strName = MPEG4_VIDEO;
            break;
        }
        case PT_TYPE_H264:

        {
            strName = H264_VIDEO;
            break;
        }
        case PT_TYPE_H265:

        {
            strName = H265_VIDEO;
            break;
        }
        default:
        {
            MK_LOG(AS_LOG_WARNING,"the playload:[%d] not support.",ucPT);
            return;
        }
    }

    snprintf(szBuf,STR_SDP_BUF_LEN,"%d",ulClockFre);
    strRtpmap = strName + "/";
    strRtpmap += szBuf;
    return;
}

uint8_t mk_media_sdp::getPayloadTypeByRtpmap(std::string& strRtpmap)
{
    uint8_t PayloadType = PT_TYPE_MAX;
    MEDIA_INFO_LIST::iterator iter = m_VideoInfoList.begin();
    for(;iter != m_VideoInfoList.end(); ++iter) {
        if((*iter)->strRtpmap == strRtpmap) {
            PayloadType = (*iter)->ucPayloadType;
            return PayloadType;
        }
    }
    
    iter = m_AudioInfoList.begin();
    for(;iter != m_AudioInfoList.end(); ++iter) {
        if((*iter)->strRtpmap == strRtpmap) {
            PayloadType = (*iter)->ucPayloadType;
            return PayloadType;
        }
    }

    return PayloadType;
}
bool mk_media_sdp::getNextLine(const std::string &strSdpMsg,
                            uint32_t &nNextStart,
                            std::string  &strBuff) const
{
   uint32_t nLen = 0;
   const char *pszStart = strSdpMsg.c_str() + nNextStart;

   while (('\0' != *pszStart) && ('\n' != *pszStart) && ('\r' != *pszStart))
   {
       pszStart++;
       nLen++;
   }

   if (strSdpMsg.length() == static_cast <uint32_t>(pszStart - strSdpMsg.c_str()))
   {
       return false;
   }

   strBuff = strSdpMsg.substr(nNextStart, nLen);
   nNextStart += nLen;
   nNextStart++;
   return true;
}


int32_t mk_media_sdp::parseConnDesc(char *pszBuff)
{
	char *pszConnDesc = as_strsep(&pszBuff, " ");
    if ((NULL == pszConnDesc) || (NULL == pszBuff))
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail, no Connect type.");
        return AS_ERROR_CODE_FAIL;
    }

    JUMP_SPACE(pszBuff);
    pszConnDesc = as_strsep(&pszBuff, " ");
    if ((NULL == pszConnDesc) || (NULL == pszBuff))
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail, no IP Type.");
        return AS_ERROR_CODE_FAIL;
    }

    JUMP_SPACE(pszBuff);


    m_strConnAddr =pszBuff;
    return AS_ERROR_CODE_OK;
}

int32_t mk_media_sdp::parseTimeDesc(char *pszBuff)
{
    return AS_ERROR_CODE_OK;
}

int32_t mk_media_sdp::parseMediaDesc(char *pszBuff,SDP_MEDIA_INFO*& pMediaInfo)
{
    if (NULL == pszBuff)
    {
        MK_LOG(AS_LOG_WARNING,"input param for decoding sdp media is null.");
        return AS_ERROR_CODE_FAIL;
    }

    MK_LOG(AS_LOG_DEBUG,"start to decode media info: %s", pszBuff);
    char cType = (char)tolower(pszBuff[0]);
    if (('v' != cType) && ('a' != cType))
    {
        m_enParseStatus = SDP_GLOBAL_INFO;
        MK_LOG(AS_LOG_INFO,"ignore not accepted media info: %s", pszBuff);
        return AS_ERROR_CODE_OK;
    }

	char *pszMediaDesc = as_strsep(&pszBuff, " ");
    if ((NULL == pszMediaDesc) || (NULL == pszBuff))
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail, no media type.");
        return AS_ERROR_CODE_FAIL;
    }

    JUMP_SPACE(pszBuff);
    pszMediaDesc = as_strsep(&pszBuff, " ");
    if ((NULL == pszMediaDesc) || (NULL == pszBuff))
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail, no port.");
        return AS_ERROR_CODE_FAIL;
    }

    JUMP_SPACE(pszBuff);
    pszMediaDesc = as_strsep(&pszBuff, " ");
    if ((NULL == pszMediaDesc) || (NULL == pszBuff))
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail, no RTP/AVP field.");
        return AS_ERROR_CODE_FAIL;
    }

    JUMP_SPACE(pszBuff);
    if ('\0' == *pszBuff)
    {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail, no payload field.");
        return AS_ERROR_CODE_FAIL;
    }

    SDP_MEDIA_INFO* stMediaInfo;
    stMediaInfo = AS_NEW(stMediaInfo);
    if(NULL == stMediaInfo) {
        MK_LOG(AS_LOG_WARNING,"parse sdp media info fail,no memory.");
        return AS_ERROR_CODE_FAIL;
    }
    stMediaInfo->ucPayloadType = INVALID_PAYLOAD_TYPE;
    stMediaInfo->usPort        = 0;
    stMediaInfo->strRtpmap     = "";
    stMediaInfo->strFmtp       = "";
    stMediaInfo->strControl    = "";

    uint32_t unPt = (uint32_t)atoi(pszBuff);
    if ('v' == cType)
    {
        stMediaInfo->ucMediaType   = MEDIA_TYPE_VALUE_VIDEO;
        stMediaInfo->ucPayloadType = (uint8_t)unPt;
        m_enParseStatus = SDP_VIDEO_INFO;

        m_VideoInfoList.push_back(stMediaInfo);
        pMediaInfo = stMediaInfo;
    }
    else
    {
        stMediaInfo->ucMediaType   = MEDIA_TYPE_VALUE_AUDIO;
        stMediaInfo->ucPayloadType = (uint8_t)unPt;
        m_enParseStatus = SDP_AUDIO_INFO;

        m_AudioInfoList.push_back(stMediaInfo);
        pMediaInfo = stMediaInfo;
    }

    return AS_ERROR_CODE_OK;
}


int32_t mk_media_sdp::parseAttributes(char *pszBuff,SDP_MEDIA_INFO* pMediaInfo)
{
    if (NULL == pszBuff)
    {
        return AS_ERROR_CODE_FAIL;
    }

    switch(m_enParseStatus)
    {
    case SDP_VIDEO_INFO:
    case SDP_AUDIO_INFO:
        return parseMediaAttributes(pszBuff,pMediaInfo);
    default:
        return parseGlobalAttributes(pszBuff);
    }

    return AS_ERROR_CODE_OK;
}

int32_t mk_media_sdp::parseGlobalAttributes(char *pszBuff)
{
    if (NULL == pszBuff)
    {
        return AS_ERROR_CODE_FAIL;
    }


    // a=control:
    if (strlen(pszBuff) > STR_SDP_CONTROL_ATTR.length())
    {
        if (0 == strncmp(pszBuff, STR_SDP_CONTROL_ATTR.c_str(), STR_SDP_CONTROL_ATTR.length()))
        {
            m_strUrl = "";
            m_strUrl.append(pszBuff + STR_SDP_CONTROL_ATTR.length());
            return AS_ERROR_CODE_OK;
        }
    }

    //a=recvonly,a=sendonly,a=sendrecv
   if (strlen(pszBuff) > STR_SDP_RECVONLY_ATTR.length())
    {
        if (0 == strncmp(pszBuff, STR_SDP_RECVONLY_ATTR.c_str(), STR_SDP_RECVONLY_ATTR.length()))
        {
            m_enTransDirect = TRANS_DIRECTION_RECVONLY;
        }
        else if (0 == strncmp(pszBuff, STR_SDP_SENDONLY_ATTR.c_str(), STR_SDP_SENDONLY_ATTR.length()))
        {
            m_enTransDirect = TRANS_DIRECTION_SENDONLY;
        }
        else if (0 == strncmp(pszBuff, STR_SDP_SENDRECV_ATTR.c_str(), STR_SDP_SENDRECV_ATTR.length()))
        {
            m_enTransDirect = TRANS_DIRECTION_SENDRECV;
        }
        return AS_ERROR_CODE_OK;
    }

   return AS_ERROR_CODE_OK;
}

int32_t mk_media_sdp::parseMediaAttributes(char *pszBuff,SDP_MEDIA_INFO* pMediaInfo)
{
    if (NULL == pszBuff)
    {
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pMediaInfo)
    {
         return AS_ERROR_CODE_FAIL;
    }

    //a=recvonly,a=sendonly,a=sendrecv
   if (strlen(pszBuff) > STR_SDP_RECVONLY_ATTR.length())
    {
        if (0 == strncmp(pszBuff, STR_SDP_RECVONLY_ATTR.c_str(), STR_SDP_RECVONLY_ATTR.length()))
        {
            MK_LOG(AS_LOG_INFO,"parse sdp: a=recvonly.");
            m_enTransDirect = TRANS_DIRECTION_RECVONLY;
            return AS_ERROR_CODE_OK;
        }
        else if (0 == strncmp(pszBuff, STR_SDP_SENDONLY_ATTR.c_str(), STR_SDP_SENDONLY_ATTR.length()))
        {
            MK_LOG(AS_LOG_INFO,"parse sdp: a=sendonly.");
            m_enTransDirect = TRANS_DIRECTION_SENDONLY;
            return AS_ERROR_CODE_OK;
        }
        else if (0 == strncmp(pszBuff, STR_SDP_SENDRECV_ATTR.c_str(), STR_SDP_SENDRECV_ATTR.length()))
        {
            MK_LOG(AS_LOG_INFO,"parse sdp: a=sendrecv.");
            m_enTransDirect = TRANS_DIRECTION_SENDRECV;
            return AS_ERROR_CODE_OK;
        }
    }

    // a=rtpmap:
    if (strlen(pszBuff) > STR_SDP_RTPMAP.length())
    {
        if (0 == strncmp(pszBuff, STR_SDP_RTPMAP.c_str(), STR_SDP_RTPMAP.length()))
        {
            char *pszProfile = as_strsep(&pszBuff, " ");
            if ((NULL == pszProfile) || (NULL == pszBuff))
            {
                MK_LOG(AS_LOG_WARNING,"parse sdp rtpmap info fail.");
                return AS_ERROR_CODE_FAIL;
            }

            pMediaInfo->strRtpmap = "";
            pMediaInfo->strRtpmap.append(pszBuff);
            MK_LOG(AS_LOG_INFO,"parse sdp: a=rtpmap:%s.",pMediaInfo->strRtpmap.c_str());
            return AS_ERROR_CODE_OK;
        }
    }

    // a=fmtp:
    if (strlen(pszBuff) > STR_SDP_FMTP.length())
    {
        if (0 == strncmp(pszBuff, STR_SDP_FMTP.c_str(), STR_SDP_FMTP.length()))
        {
            char *pszProfile = as_strsep(&pszBuff, " ");
            if ((NULL == pszProfile) || (NULL == pszBuff))
            {
                MK_LOG(AS_LOG_WARNING,"parse sdp fmtp info fail.");
                return AS_ERROR_CODE_FAIL;
            }

            pMediaInfo->strFmtp = "";
            pMediaInfo->strFmtp.append(pszBuff);
            MK_LOG(AS_LOG_INFO,"parse sdp: a=fmtp:%s.",pMediaInfo->strFmtp.c_str());
            return AS_ERROR_CODE_OK;
        }
    }

    // a=control:
    if (strlen(pszBuff) > STR_SDP_CONTROL_ATTR.length())
    {
        if (0 == strncmp(pszBuff, STR_SDP_CONTROL_ATTR.c_str(), STR_SDP_CONTROL_ATTR.length()))
        {
            pMediaInfo->strControl = "";
            pMediaInfo->strControl.append(pszBuff + STR_SDP_CONTROL_ATTR.length());
            MK_LOG(AS_LOG_INFO,"parse sdp: a=control:%s.",pMediaInfo->strControl.c_str());
            return AS_ERROR_CODE_OK;
        }
    }

   //a=range
   if(0 == strncmp(pszBuff,"range",5))
   {
         m_range =  "";
         m_range.append(pszBuff);
         MK_LOG(AS_LOG_WARNING,"parseRange is [%s]",pszBuff);
         return AS_ERROR_CODE_OK;
   }

   return AS_ERROR_CODE_OK;
}


void mk_media_sdp::encodeMediaDesc(std::string &strSdp, uint32_t unStatus)
{
    stringstream strValue;
    SDP_MEDIA_INFO *pMediaInfo = NULL;
    MEDIA_INFO_LIST*pMediaInfoList =NULL;

    if (SDP_VIDEO_INFO == unStatus)
    {
        pMediaInfoList = &m_VideoInfoList;
    }
    else
    {
        pMediaInfoList = &m_AudioInfoList;
    }

    MEDIA_INFO_LIST::iterator iter = pMediaInfoList->begin();

    pMediaInfo = (SDP_MEDIA_INFO *)(*iter);

    if (SDP_VIDEO_INFO == unStatus)
    {
        strSdp += "m=video ";
        strValue.str("");
        strValue << (uint32_t)pMediaInfo->usPort;
        strSdp += strValue.str();
        strSdp += " ";
    }
    else
    {
        strSdp += "m=audio ";
        strValue.str("");
        strValue << (uint32_t)pMediaInfo->usPort;
        strSdp += strValue.str();
        strSdp += " ";
    }

    strSdp += SDP_TRANSPORT_RTP;
    strSdp += SDP_SIGN_SLASH;
    strSdp += SDP_TRANSPORT_PROFILE_AVP;

    for(;iter != pMediaInfoList->end();++iter)
    {
        pMediaInfo = (SDP_MEDIA_INFO *)&(*iter);

        // payload
        strValue.str("");
        strValue << (uint32_t)pMediaInfo->ucPayloadType;
        strSdp += " " + strValue.str();
    }
    strSdp += SDP_END_TAG;

    iter = pMediaInfoList->begin();

    for(;iter != pMediaInfoList->end();++iter)
    {
        pMediaInfo = (SDP_MEDIA_INFO *)&(*iter);

        strValue.str("");
        strValue << (uint32_t)pMediaInfo->ucPayloadType;

        //a=fmtp:99 profile-level-id=42A01E;packetization-mode=0
        if ("" != pMediaInfo->strFmtp)
        {
            strSdp += pMediaInfo->strFmtp;
            strSdp += SDP_END_TAG;
        }
        else
        {
            //a=rtpmap:99 H264/90000
            strSdp += "a=rtpmap:" + strValue.str() + " " + pMediaInfo->strRtpmap;
            strSdp += SDP_END_TAG;
        }

        //a=control:
        if ("" != pMediaInfo->strControl)
        {
            strSdp += "a=control:";
            strSdp += pMediaInfo->strControl;
            strSdp += SDP_END_TAG;
        }
        else if ("" != m_strUrl)
        {

            strSdp += "a=control:";
            strSdp += m_strUrl;
            strSdp += SDP_END_TAG;
        }

    }

    return;
}

