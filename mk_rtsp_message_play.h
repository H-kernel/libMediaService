/*
 * RtspPlayMessage.h
 *
 *  Created on: 2016-5-20
 *      Author:
 */

#ifndef RTSPPLAYMESSAGE_H_
#define RTSPPLAYMESSAGE_H_

#include "svs_rtsp_message.h"
// ý�岥�ŷ�Χ��ʱ���ʾ����
typedef enum
{
    RANGE_TYPE_NPT = 0, // NormalPlayTime(���ʱ��)
    RANGE_TYPE_UTC,     // UTC AbsoluteTime(UTC����ʱ��)
    RANGE_TYPE_SMPTE    // SMPTEʱ�䣬Ŀǰ�ݲ�֧��
}MEDIA_RANGE_TYPE_E;

// ��ʾý�岥�ŷ�Χ�Ľṹ��
typedef struct
{
    uint32_t  MediaBeginOffset; // ��ʼ����λ��(��λ:��)
    uint32_t  MediaEndOffset;   // ��������λ��(��λ:��)
    uint32_t  enRangeType;      // Range��ʱ������
}MEDIA_RANGE_S;

// �����ض��ļ���ʱ��ƫ����
#define OFFSET_BEGIN  ((uint32_t)-3) // ��ʾý���ʼ��λ��
#define OFFSET_CUR    ((uint32_t)-2) // ��ʾý�嵱ǰλ��
#define OFFSET_END    ((uint32_t)-1)  // ��ʾý�����λ��

class CRtspPlayMessage: public mk_rtsp_message
{
public:
    CRtspPlayMessage();

    virtual ~CRtspPlayMessage();

    void setSpeed(double nSpeed);

    double getSpeed() const;

    void setScale(double nScale);

    double getScale() const;

    bool hasRange() const;

    void setRange(const MEDIA_RANGE_S &stRange);

    void getRange(MEDIA_RANGE_S &stRange) const;

    void getRange(std::string &strRange) const;

    void setRtpInfo(const std::string &strRtpInfo);

    std::string getRtpInfo() const;

    int32_t decodeMessage(CRtspPacket& objRtspPacket);

    int32_t encodeMessage(std::string &strMessage);
private:
    int32_t encodeRangeField(std::string &strMessage);
private:
    double          m_dSpeed;
    double          m_dScale;

    bool            m_bHasRange;
    std::string     m_strRange;
    MEDIA_RANGE_S   m_stRange;
    std::string     m_strRtpInfo;
};
#endif /* RTSPPLAYMESSAGE_H_ */
