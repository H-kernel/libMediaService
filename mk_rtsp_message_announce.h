/*
 * RtspAnnounceMessage.h
 *
 *  Created on: 2016-5-22
 *      Author:
 */

#ifndef RTSPANNOUNCEMESSAGE_H_
#define RTSPANNOUNCEMESSAGE_H_

#include "svs_rtsp_message.h"

class CRtspAnnounceMessage: public CRtspMessage
{
public:
    CRtspAnnounceMessage();
    virtual ~CRtspAnnounceMessage();
    int32_t encodeMessage(std::string &strMessage);
    int32_t decodeMessage(CRtspPacket& objRtspPacket);
};

#endif /* RTSPANNOUNCEMESSAGE_H_ */
