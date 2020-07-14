/*
 * RtspOptionsResp.cpp
 *
 *  Created on: 2016-5-19
 *      Author:
 */
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "svs_rtsp_defs.h"
#include "svs_rtsp_options_message.h"
#include "svs_rtsp_protocol.h"

CRtspOptionsMessage::CRtspOptionsMessage()
{
    m_unMethodType     = RTSP_METHOD_OPTIONS;
    m_strRange         = "";
}
CRtspOptionsMessage::~CRtspOptionsMessage()
{

}

void CRtspOptionsMessage::setRange(const std::string &strRange)
{
    m_strRange = strRange;
    return;
}

int32_t CRtspOptionsMessage::encodeMessage(std::string &strMessage)
{
    strMessage.clear();

    if (AS_ERROR_CODE_OK != mk_rtsp_message::encodeMessage(strMessage))
    {
        AS_LOG(AS_LOG_WARNING,"encode rtsp options message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_MSG_RSP == getMsgType())
    {
        if ("" != m_strSession)
        {
            strMessage += RTSP_TOKEN_STR_RANGE;

            if ("" == m_strRange)
            {
                strMessage += "npt=now-";
            }
            else
            {
                strMessage += m_strRange;
            }
            strMessage += RTSP_END_TAG;
        }

        // Public
        strMessage += RTSP_TOKEN_STR_PUBLIC;
        strMessage += "OPTIONS, DESCRIBE, SETUP, TEARDOWN, GET_PARAMETER, PLAY, PAUSE, ANNOUNCE";
        strMessage += RTSP_END_TAG;
    }

    // End
    strMessage += RTSP_END_TAG;

    AS_LOG(AS_LOG_DEBUG,"encode rtsp options message:\n%s",
                strMessage.c_str());
    return AS_ERROR_CODE_OK;
}
