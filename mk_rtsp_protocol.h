/*
 * RtspMessageFactory.h
 *
 *  Created on: 2016-5-17
 *      Author:
 */

#ifndef RTSPMESSAGEFACTORY_H_
#define RTSPMESSAGEFACTORY_H_

#include <map>
#include "svs_rtsp_defs.h"
#include "svs_rtsp_message.h"
#include "ms_engine_rtsp_packet.h"

typedef std::map<uint32_t, uint32_t>    REQ_TYPE_MAP;
typedef REQ_TYPE_MAP::iterator          REQ_TYPE_MAP_ITER;

class mk_rtsp_protocol
{
public:
    virtual ~mk_rtsp_protocol();

    mk_rtsp_protocol();


    int32_t init() const;

    uint32_t getCseq();

    int32_t saveSendReq(uint32_t unCSeq, uint32_t unReqMethodType);

    int32_t IsParsable(const char* pMsgData, uint32_t unDataLen) const;

    int32_t DecodeRtspMessage(const char* pMsgData, uint32_t unDataLen, mk_rtsp_message *&pMsg);

private:

    int32_t parseRtspRequest(CRtspPacket& objRtspPacket, mk_rtsp_message *&pMessage) const;

    int32_t parseRtspResponse(CRtspPacket& objRtspPacket, mk_rtsp_message *&pMessage);
public:
    static std::string           m_RtspCode[];
    static std::string           m_strRtspMethod[];

    uint32_t               m_unCSeq;
    ACE_Thread_Mutex       m_CseqMutex;

    REQ_TYPE_MAP           m_CseqReqMap;
    ACE_Thread_Mutex       m_MapMutex;

};

#endif /* RTSPMESSAGEFACTORY_H_ */
