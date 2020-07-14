/*
 * RtspPauseMessage.cpp
 *
 *  Created on: 2016-5-20
 *      Author:
 */
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "svs_rtsp_pause_message.h"

CRtspPauseMessage::CRtspPauseMessage()
{
    m_unMethodType     = RTSP_METHOD_PAUSE;
}

CRtspPauseMessage::~CRtspPauseMessage()
{
}


int32_t CRtspPauseMessage::encodeMessage(std::string &strMessage)
{
    strMessage.clear();

    // ֱ�ӵ��ø������CSeq��User-Agent
    if (AS_ERROR_CODE_OK != mk_rtsp_message::encodeMessage(strMessage))
    {
        AS_LOG(AS_LOG_WARNING,"encode rtsp pause message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    // End
    strMessage += RTSP_END_TAG;

    AS_LOG(AS_LOG_DEBUG,"encode rtsp pause message:\n%s", strMessage.c_str());
    return AS_ERROR_CODE_OK;
}
