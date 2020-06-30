/*
 * RtspPlayMessage.h
 *
 *  Created on: 2016-5-20
 *      Author:
 */

#ifndef RTSPPLAYMESSAGE_H_
#define RTSPPLAYMESSAGE_H_

#include "svs_rtsp_message.h"
// 媒体播放范围的时间表示类型
typedef enum
{
    RANGE_TYPE_NPT = 0, // NormalPlayTime(相对时间)
    RANGE_TYPE_UTC,     // UTC AbsoluteTime(UTC绝对时间)
    RANGE_TYPE_SMPTE    // SMPTE时间，目前暂不支持
}MEDIA_RANGE_TYPE_E;

// 表示媒体播放范围的结构体
typedef struct
{
    uint32_t  MediaBeginOffset; // 开始播放位置(单位:秒)
    uint32_t  MediaEndOffset;   // 结束播放位置(单位:秒)
    uint32_t  enRangeType;      // Range的时间类型
}MEDIA_RANGE_S;

// 定义特定的几个时间偏移量
#define OFFSET_BEGIN  ((uint32_t)-3) // 表示媒体最开始的位置
#define OFFSET_CUR    ((uint32_t)-2) // 表示媒体当前位置
#define OFFSET_END    ((uint32_t)-1)  // 表示媒体结束位置

class CRtspPlayMessage: public CRtspMessage
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
