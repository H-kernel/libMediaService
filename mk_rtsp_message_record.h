/*
 * svs_rtsp_play_message.h
 *
 *  Created on: 2016-5-20
 *      Author:
 */

#ifndef RTSPRECORDMESSAGE_H_
#define RTSPRECORDMESSAGE_H_

#include "svs_rtsp_message.h"

class CRtspRecordMessage: public CRtspMessage
{
public:
    CRtspRecordMessage();

    virtual ~CRtspRecordMessage();

    int32_t decodeMessage(CRtspPacket& objRtspPacket);

    int32_t encodeMessage(std::string &strMessage);
private:
    int32_t encodeRangeField(std::string &strMessage);
private:
};
#endif /* RTSPPLAYMESSAGE_H_ */
