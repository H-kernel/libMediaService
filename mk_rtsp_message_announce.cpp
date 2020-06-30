/*
 * RtspAnnounceMessage.cpp
 *
 *  Created on: 2016-5-22
 *      Author:
 */
#include <sstream>
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "svs_rtsp_announce_message.h"

CRtspAnnounceMessage::CRtspAnnounceMessage()
{
    m_unMethodType     = RTSP_METHOD_ANNOUNCE;
}

CRtspAnnounceMessage::~CRtspAnnounceMessage()
{
}

int32_t CRtspAnnounceMessage::decodeMessage(CRtspPacket& objRtspPacket)
{
    int32_t nRet = CRtspMessage::decodeMessage(objRtspPacket);
    if (AS_ERROR_CODE_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"decode rtsp Announce message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if( m_strBody.length() == 0 )
    {
        SVS_LOG(SVS_LOG_WARNING,"decode rtsp Announce message fail, body is empty.");
        return AS_ERROR_CODE_FAIL;
    }
    return AS_ERROR_CODE_OK ;
}

int32_t CRtspAnnounceMessage::encodeMessage(std::string &strMessage)
{
    strMessage.clear();

    if (AS_ERROR_CODE_OK != CRtspMessage::encodeMessage(strMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"encode rtsp announce message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_MSG_REQ == getMsgType())
    {
        //Content-Length: 24
        std::string strQuit = "HW_ANNOUNCE_QUIT:2002";
        strMessage += RTSP_TOKEN_STR_CONTENT_LENGTH;

        std::stringstream strContentLength;
        strContentLength << strQuit.length();
        strMessage += strContentLength.str();
        strMessage += RTSP_END_TAG;

        //Content-Type: text/parameters
        strMessage += RTSP_TOKEN_STR_CONTENT_TYPE;
        strMessage += RTSP_CONTENT_PARAM;
        strMessage += RTSP_END_TAG;

        // End
        strMessage += RTSP_END_TAG;

        // Body
        strMessage += strQuit;
    }
    else
    {
        // End
        strMessage += RTSP_END_TAG;
    }

    SVS_LOG(SVS_LOG_DEBUG,"encode rtsp options message:\n%s",
                strMessage.c_str());
    return AS_ERROR_CODE_OK;
}
