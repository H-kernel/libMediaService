/*
 * RtspPlayMessage.cpp
 *
 *  Created on: 2016-5-20
 *      Author:
 */
#include <sstream>
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "ms_engine_common.h"
#include "svs_rtsp_defs.h"
#include "svs_rtsp_record_message.h"

CRtspRecordMessage::CRtspRecordMessage()
{
    m_unMethodType  = RTSP_METHOD_RECORD;
}

CRtspRecordMessage::~CRtspRecordMessage()
{
}



int32_t CRtspRecordMessage::decodeMessage(CRtspPacket& objRtspPacket)
{
    int32_t nRet = CRtspMessage::decodeMessage(objRtspPacket);
    if (RET_OK != nRet)
    {
        SVS_LOG(SVS_LOG_WARNING,"decode rtsp play message fail.");
        return RET_FAIL;
    }

    return RET_OK;
}

int32_t CRtspRecordMessage::encodeMessage(std::string &strMessage)
{
    if (RET_OK != CRtspMessage::encodeMessage(strMessage))
    {
        SVS_LOG(SVS_LOG_WARNING,"encode rtsp play message fail.");
        return RET_FAIL;
    }

    return RET_OK;
}

int32_t CRtspRecordMessage::encodeRangeField(std::string &strMessage)
{

    strMessage += RTSP_END_TAG;

    return RET_OK;
}
