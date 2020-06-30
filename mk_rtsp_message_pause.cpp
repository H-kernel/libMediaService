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
    if (RET_OK != CRtspMessage::encodeMessage(strMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"encode rtsp pause message fail.");
        return RET_FAIL;
    }

    // End
    strMessage += RTSP_END_TAG;

    SVS_LOG(SVS_LOG_DEBUG,"encode rtsp pause message:\n%s", strMessage.c_str());
    return RET_OK;
}
