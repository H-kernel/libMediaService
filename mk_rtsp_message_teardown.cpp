/*
 * RtspTeardownMessage.cpp
 *
 *  Created on: 2016-5-20
 *      Author:
 */
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "svs_rtsp_teardown_message.h"

CRtspTeardownMessage::CRtspTeardownMessage()
{
    m_unMethodType = RTSP_METHOD_TEARDOWN;
}

CRtspTeardownMessage::~CRtspTeardownMessage()
{
}

int32_t CRtspTeardownMessage::encodeMessage(std::string &strMessage)
{
    strMessage.clear();

    // ֱ�ӵ��ø������CSeq��User-Agent
    if (AS_ERROR_CODE_OK != CRtspMessage::encodeMessage(strMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"encode rtsp teardown message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    // End
    strMessage += RTSP_END_TAG;

    SVS_LOG(SVS_LOG_DEBUG,"encode rtsp teardown message:\n%s", strMessage.c_str());
    return AS_ERROR_CODE_OK;
}
