/*
 * RtspOptionsResp.h
 *
 *  Created on: 2016-5-19
 *      Author:
 */

#ifndef __RTSP_GET_PARAMETER_RESP_H_
#define __RTSP_GET_PARAMETER_RESP_H_

#include "svs_rtsp_message.h"

// RTSP GET_PARAMTER«Î«Û
class CRtspGerParamMessage: public CRtspMessage
{
public:
    CRtspGerParamMessage();
    virtual ~CRtspGerParamMessage();
};

#endif /* __RTSP_GET_PARAMETER_RESP_H_ */
