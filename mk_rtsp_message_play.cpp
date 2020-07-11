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
#include "svs_rtsp_play_message.h"

CRtspPlayMessage::CRtspPlayMessage()
{
    m_unMethodType  = RTSP_METHOD_PLAY;
    m_dSpeed        = 0;
    m_dScale        = 0;

    m_bHasRange     = false;
    m_stRange.enRangeType = RANGE_TYPE_UTC;
    m_stRange.MediaBeginOffset = OFFSET_CUR;
    m_stRange.MediaEndOffset = OFFSET_END;

    m_strRtpInfo    = "";
    m_strRange      = "";
}

CRtspPlayMessage::~CRtspPlayMessage()
{
}


void CRtspPlayMessage::setSpeed(double nSpeed)
{
    m_dSpeed    = nSpeed;
    return;
}

double CRtspPlayMessage::getSpeed() const
{
    return  m_dSpeed;
}

void CRtspPlayMessage::setScale(double nScale)
{
    m_dScale = nScale;
    return;
}

double CRtspPlayMessage::getScale() const
{
    return m_dScale;
}

bool CRtspPlayMessage::hasRange() const
{
    return m_bHasRange;
}

void CRtspPlayMessage::setRange(const MEDIA_RANGE_S &stRange)
{
    m_bHasRange = true;
    m_stRange.enRangeType = stRange.enRangeType;
    m_stRange.MediaBeginOffset = stRange.MediaBeginOffset;
    m_stRange.MediaEndOffset   = stRange.MediaEndOffset;
    return;
}

void CRtspPlayMessage::getRange(MEDIA_RANGE_S &stRange) const
{
    stRange.enRangeType = m_stRange.enRangeType;
    stRange.MediaBeginOffset = m_stRange.MediaBeginOffset;
    stRange.MediaEndOffset = m_stRange.MediaEndOffset;

    return;
}

void CRtspPlayMessage::getRange(std::string &strRange) const
{
    strRange = m_strRange;
    return;
}

void CRtspPlayMessage::setRtpInfo(const string &strRtpInfo)
{
    m_strRtpInfo = strRtpInfo;
    return;
}

string CRtspPlayMessage::getRtpInfo() const
{
    return m_strRtpInfo;
}

int32_t CRtspPlayMessage::decodeMessage(CRtspPacket& objRtspPacket)
{
    int32_t nRet = CRtspMessage::decodeMessage(objRtspPacket);
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"decode rtsp play message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (RtspResponseMethod == objRtspPacket.getMethodIndex())
    {
        string strRtpInfo;

        // ����Ӧ��Ϣ�н���RTP-Info
        objRtspPacket.getRtpInfo(m_strRtpInfo);
        string::size_type endPos = m_strRtpInfo.find(RTSP_END_TAG);
        if (string::npos != endPos)
        {
            m_strRtpInfo = m_strRtpInfo.substr(0, endPos);
        }

        return AS_ERROR_CODE_OK;
    }

    // ����Scale

    m_dScale = objRtspPacket.getScale();

    // ����Speed
    m_dSpeed = objRtspPacket.getSpeed();

    // ����Range

    objRtspPacket.getRangeTime(m_stRange.enRangeType,
                                   m_stRange.MediaBeginOffset,
                                   m_stRange.MediaEndOffset);

    return AS_ERROR_CODE_OK;
}

int32_t CRtspPlayMessage::encodeMessage(std::string &strMessage)
{
    if (AS_ERROR_CODE_OK != CRtspMessage::encodeMessage(strMessage))
    {
        AS_LOG(AS_LOG_WARNING,"encode rtsp play message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    stringstream   strValue;
    if (RTSP_MSG_REQ == getMsgType())
    {
        // ������Ϣ
        // Scale
        if (0 != m_dScale)
        {
            strValue<<m_dScale;
            strMessage += RTSP_TOKEN_STR_SCALE;
            strMessage += strValue.str();
            strMessage += RTSP_END_TAG;
        }

        // Speed
        if (0 != m_dSpeed)
        {
            strValue.str("");
            strValue << m_dSpeed;
            strMessage += RTSP_TOKEN_STR_SPEED;
            strMessage += strValue.str();
            strMessage += RTSP_END_TAG;
        }

        if (m_bHasRange)
        {
            (void)encodeRangeField(strMessage);
        }
    }
    else
    {
        // ��Ӧ��Ϣ�������RTP_INFO����Ҫ����
        if ("" != m_strRtpInfo)
        {
            strMessage += RTSP_TOKEN_STR_RTPINFO;
            strMessage += m_strRtpInfo;
            strMessage += RTSP_END_TAG;
        }
    }

    // End
    strMessage += RTSP_END_TAG;

    AS_LOG(AS_LOG_DEBUG,"encode rtsp play message:\n%s", strMessage.c_str());
    return AS_ERROR_CODE_OK;
}

int32_t CRtspPlayMessage::encodeRangeField(std::string &strMessage)
{
    strMessage += RTSP_TOKEN_STR_RANGE;

    char strTime[32] = { 0 };
    if (RANGE_TYPE_UTC == m_stRange.enRangeType)
    {
        // clockʱ��
        time_t rangeTime = (time_t) m_stRange.MediaBeginOffset;
        struct tm tmv;

        (void) localtime_r(&rangeTime, &tmv);
        (void) snprintf(strTime, 32, "%04d%02d%02dT%02d%02d%02dZ", tmv.tm_year
                + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min,
                tmv.tm_sec);

        strMessage += RTSP_RANGE_CLOCK;
        strMessage += strTime;
        strMessage += SIGN_H_LINE;

        if (OFFSET_END != m_stRange.MediaEndOffset)
        {
            rangeTime = (time_t) m_stRange.MediaEndOffset;
            (void) localtime_r(&rangeTime, &tmv);
            (void) snprintf(strTime, 32, "%04d%02d%02dT%02d%02d%02dZ",
                    tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                    tmv.tm_hour, tmv.tm_min, tmv.tm_sec);

            strMessage += strTime;
        }

        strMessage += RTSP_END_TAG;
        return AS_ERROR_CODE_OK;
    }

    // nptʱ��
    strMessage += RTSP_RANGE_NPT;
    if (OFFSET_CUR == m_stRange.MediaBeginOffset)
    {
        strMessage += "0";
    }
    else if (OFFSET_BEGIN == m_stRange.MediaBeginOffset)
    {
        strMessage += "0";
    }
    else
    {
        (void) snprintf(strTime, 32, "%u", m_stRange.MediaBeginOffset);
        strMessage += strTime;
    }
    strMessage += SIGN_H_LINE;

    if (OFFSET_END != m_stRange.MediaEndOffset)
    {
        (void) snprintf(strTime, 32, "%u", m_stRange.MediaEndOffset);
        strMessage += strTime;
    }
    strMessage += RTSP_END_TAG;

    return AS_ERROR_CODE_OK;
}
