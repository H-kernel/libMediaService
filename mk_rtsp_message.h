/*
 * RtspMessage.h
 *
 *  Created on: 2016-5-17
 *      Author:
 */

#ifndef RTSPMESSAGE_H_
#define RTSPMESSAGE_H_

#include "svs_ace_header.h"
#include "svs_rtsp_defs.h"
#include "ms_engine_rtsp_packet.h"

class CRtspMessage
{
public:
    CRtspMessage();

    virtual ~CRtspMessage();

    static void encodeCommonResp(uint32_t unStatusCode,
                                 uint32_t unCseq,
                                 uint32_t unSession,
                                 std::string &strMsg);
    uint32_t getMethodType() const;

    void setMsgType(uint32_t unMsgType);

    uint32_t getMsgType() const;

    std::string getSession() const;

    void setSession(const std::string &strSession);

    uint32_t getCSeq() const;

    void setCSeq(uint32_t unCSeq);

    void setStatusCode(uint32_t unCode);

    uint32_t getStatusCode() const;

    void setRtspUrl(const std::string &strUrl);

    std::string getRtspUrl() const;

    uint32_t getContentLength() const;

    std::string getContetType() const ;

    std::string getBody() const ;

    void setBody(std::string& strContentType,std::string& strContent);

    virtual int32_t decodeMessage(CRtspPacket& objRtspPacket);

    virtual int32_t encodeMessage(std::string &strMessage);
protected:
    uint32_t        m_unMethodType;
    uint32_t        m_unMsgType;
    uint32_t        m_unCSeq;
    std::string     m_strSession;
    uint32_t        m_unStatusCode;
    std::string     m_strRtspUrl;

    uint32_t        m_unContentLength;
    std::string     m_strContetType;
    std::string     m_strBody;

    CRtspPacket     m_RtspPacket;


};

#endif /* RTSPMESSAGE_H_ */
