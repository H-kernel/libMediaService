/*
 * RtspDescribeReq.h
 *
 *  Created on: 2016-5-19
 *      Author:
 */

#ifndef RTSPDESCRIBEREQ_H_
#define RTSPDESCRIBEREQ_H_

#include "svs_rtsp_message.h"

class CRtspDescribeMessage: public CRtspMessage
{
public:
    CRtspDescribeMessage();
    virtual ~CRtspDescribeMessage();

    void setSdp(const std::string &strSdp);

    std::string getSdp() const;

    int32_t encodeMessage(std::string &strMessage);

    int32_t decodeMessage(CRtspPacket& objRtspPacket);
private:
    std::string     m_strSDP;
};

#endif /* RTSPDESCRIBEREQ_H_ */
