/*
 * RtspDescribeReq.cpp
 *
 *  Created on: 2016-5-19
 *      Author:
 */
#include <sstream>
#include "ms_engine_svs_retcode.h"
#include "svs_log.h"
#include "svs_rtsp_describe_message.h"
#include "svs_rtsp_protocol.h"

CRtspDescribeMessage::CRtspDescribeMessage()
{
    m_unMethodType = RTSP_METHOD_DESCRIBE;
    m_strSDP = "";
}

CRtspDescribeMessage::~CRtspDescribeMessage()
{
}

int32_t CRtspDescribeMessage::decodeMessage(CRtspPacket& objRtspPacket)
{
    int32_t nRet = CRtspMessage::decodeMessage(objRtspPacket);
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_WARNING,"decode rtsp play message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    objRtspPacket.getContent(m_strSDP);

    return AS_ERROR_CODE_OK;
}
int32_t CRtspDescribeMessage::encodeMessage(std::string &strMessage)
{
    strMessage.clear();

    // ֱ�ӵ��ø������CSeq��User-Agent
    if (AS_ERROR_CODE_OK != CRtspMessage::encodeMessage(strMessage))
    {
        AS_LOG(AS_LOG_WARNING,"encode rtsp describe request message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (RTSP_MSG_RSP == getMsgType())
    {
        // ��Ӧ��Ϣ
        if (RTSP_SUCCESS_OK == m_unStatusCode)
        {
            // 200 OK��Ӧ����ҪЯ��SDP��Ϣ.
            // Content-Length
            if (0 == m_strSDP.length())
            {
                AS_LOG(AS_LOG_WARNING,"encode rtsp describe message fail, no sdp info.");

                return AS_ERROR_CODE_FAIL;
            }

            strMessage += RTSP_TOKEN_STR_CONTENT_LENGTH;
            std::stringstream strContentLength;
            strContentLength << m_strSDP.length();    // ��������������\r\n
            strMessage += strContentLength.str();
            strMessage += RTSP_END_TAG;

            // Content-Type
            strMessage += RTSP_TOKEN_STR_CONTENT_TYPE;
            strMessage += RTSP_CONTENT_SDP;
            strMessage += RTSP_END_TAG;

            // End of Rtsp
            strMessage += RTSP_END_TAG;

            // ���׷��SDP��Ϣ
            strMessage += m_strSDP;
        }
        else
        {
            // ����������û��SDP
            // End of Rtsp
            strMessage += RTSP_END_TAG;
        }
    }
    else
    {
        // ������Ϣ
        // Accept
        strMessage += RTSP_TOKEN_STR_ACCEPT;
        strMessage += RTSP_CONTENT_SDP;
        strMessage += RTSP_END_TAG;

        //end
        strMessage += RTSP_END_TAG;
    }
    AS_LOG(AS_LOG_DEBUG,"encode rtsp describe message:\n%s",
                     strMessage.c_str());
    return AS_ERROR_CODE_OK;
}

void CRtspDescribeMessage::setSdp(const std::string &strSdp)
{
    m_strSDP    = strSdp;
    return;
}

std::string CRtspDescribeMessage::getSdp() const
{
    return m_strSDP;
}
