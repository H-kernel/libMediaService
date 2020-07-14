/*
 * RtspPauseMessage.h
 *
 *  Created on: 2016-5-20
 *      Author:
 */

#ifndef RTSPPAUSEMESSAGE_H_
#define RTSPPAUSEMESSAGE_H_

#include "svs_rtsp_message.h"

class CRtspPauseMessage: public mk_rtsp_message
{
public:
    CRtspPauseMessage();
    virtual ~CRtspPauseMessage();

    int32_t encodeMessage(std::string &strMessage);
};

#endif /* RTSPPAUSEMESSAGE_H_ */
