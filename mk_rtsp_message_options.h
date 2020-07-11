/*
 * RtspOptionsResp.h
 *
 *  Created on: 2016-5-19
 *      Author:
 */

#ifndef RTSPOPTIONSRESP_H_
#define RTSPOPTIONSRESP_H_

#include "svs_rtsp_message.h"

// RTSP OPTIONS
class CRtspOptionsMessage: public CRtspMessage
{
public:
    CRtspOptionsMessage();
    virtual ~CRtspOptionsMessage();

    void setRange(const std::string &strRange);

    int32_t encodeMessage(std::string &strMessage);
private:
    std::string     m_strRange;
};

#endif /* RTSPOPTIONSRESP_H_ */
