/*
 * RtspTeardownMessage.h
 *
 *  Created on: 2016-5-20
 *      Author:
 */

#ifndef RTSPTEARDOWNMESSAGE_H_
#define RTSPTEARDOWNMESSAGE_H_

#include "svs_rtsp_message.h"

class CRtspTeardownMessage: public CRtspMessage
{
public:
    CRtspTeardownMessage();
    virtual ~CRtspTeardownMessage();

    int32_t encodeMessage(std::string &strMessage);
};

#endif /* RTSPTEARDOWNMESSAGE_H_ */
