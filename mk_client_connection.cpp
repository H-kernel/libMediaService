#include "mk_client_connection.h"
#include "mk_media_common.h"
#include "mk_media_service.h"


mk_client_timer::mk_client_timer()
{

}
mk_client_timer::~mk_client_timer()
{

}
void mk_client_timer::onTrigger(void *pArg, ULONGLONG ullScales, TriggerStyle enStyle)
{
    mk_client_connection* pConnect = (mk_client_connection*)pArg;
    pConnect->check_client();
}

mk_client_connection::mk_client_connection()
{
    m_ulIndex        = 0;
    m_enType         = MK_CLIENT_TYPE_MAX;
    m_enStatus       = MR_CLIENT_STATUS_MAX;
    m_strurl         = "";
}
mk_client_connection::~mk_client_connection()
{
    
}
MK_CLIENT_TYPE mk_client_connection::client_type()
{
    return m_enType;
}
MR_CLIENT_STATUS mk_client_connection::get_status()
{
    return m_enStatus;
}
void mk_client_connection::set_url(const char* pszUrl)
{
    m_strurl = pszUrl;
}
void  mk_client_connection::set_status_callback(MEDIA_CALL_BACK* cb,void* ctx)
{
    m_ClientCallBack = cb;
    m_pClientCb  = ctx;
}
int32_t  mk_client_connection::start_recv()
{
    if(AS_ERROR_CODE_OK != this->start()) {
        return AS_ERROR_CODE_FAIL;
    }

    as_timer& pTimerMgr = mk_media_service::instance().get_client_check_timer();
    return pTimerMgr.registerTimer(&m_ClientCheckTimer,this,MK_CLIENT_TIMER_INTERVAL,enRepeated);
}
int32_t  mk_client_connection::stop_recv()
{
    as_timer& pTimerMgr = mk_media_service::instance().get_client_check_timer();
    pTimerMgr.cancelTimer(&m_ClientCheckTimer);
    this->stop();
    return AS_ERROR_CODE_OK;
}
int32_t  mk_client_connection::do_next_recv()
{
    return recv_next();
}
void     mk_client_connection::set_index(uint32_t ulIdx)
{
    m_ulIndex = ulIdx;
}
uint32_t mk_client_connection::get_index()
{
    return m_ulIndex;
}
void mk_client_connection::get_client_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo)
{
    this->get_rtp_stat_info(statinfo);
    return;
}
void mk_client_connection::get_client_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen)
{
    this->get_rtsp_sdp_info(info,lens,copylen);
    return;
}
void    mk_client_connection::handle_connection_media(MEDIA_DATA_INFO* dataInfo,uint32_t len)
{
    if((NULL == m_ClientCallBack) ||(NULL == m_ClientCallBack->m_cb_status)) {
        return;
    }

    m_ClientCallBack->m_cb_data(this,dataInfo,len,m_pClientCb);
}
void    mk_client_connection::handle_connection_status(MR_CLIENT_STATUS  enStatus)
{
    m_enStatus = enStatus;
    if((NULL == m_ClientCallBack) ||(NULL == m_ClientCallBack->m_cb_status)) {
        return;
    }
    m_ClientCallBack->m_cb_status(this,m_enStatus,m_pClientCb);
}

char*   mk_client_connection::handle_connection_databuf(uint32_t len,uint32_t& bufLen)
{
    if((NULL == m_ClientCallBack) ||(NULL == m_ClientCallBack->m_cb_status))
    {
        return NULL;
    }
    return m_ClientCallBack->m_cb_buffer(this,len,bufLen,m_pClientCb);
}

void    mk_client_connection::parse_media_info(MR_MEDIA_TYPE enType,uint32_t pts,char* buf,MEDIA_DATA_INFO &dataInfo)
{
    char c = buf[4];                
    uint32_t isKeyFrame = 0;
    if(MR_MEDIA_TYPE_H264 == enType)
    {
        int nalType = (c & 0x1f);
        if((7 == nalType)||(8 == nalType)||(6 == nalType)||(5 == nalType))
        {
            isKeyFrame = 1;
        }
        pts /= 90;
    }
    else if(MR_MEDIA_TYPE_H265 == enType)
    {
        int nalType = (c & 0x7E)>>1;
        if((nalType >= 16) && (nalType <=21))
        {
            isKeyFrame = 1;
        }     
        pts /= 90;   
    }    
    else if((MR_MEDIA_TYPE_G711A == enType)||(MR_MEDIA_TYPE_G711U == enType))
    {
        pts /= 8;   
    }

    dataInfo.type = enType;            
    dataInfo.isKeyFrame = isKeyFrame;
    dataInfo.pts = pts;
    return;
}