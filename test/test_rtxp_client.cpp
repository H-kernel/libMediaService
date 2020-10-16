#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include "as.h"
#include "../libMediaService.h"
typedef struct
{
    //byte 0
    uint8_t TYPE:5;
    uint8_t NRI:2;
    uint8_t F:1;
} NALU_HEADER; /**//* 1 BYTES */

typedef struct
{
    //byte 0
    uint8_t LATERID0:1;
    uint8_t TYPE:6;
    uint8_t F:1;
    //byte 1
    uint8_t TID:3;
    uint8_t LATERID1:5;
} NALU_HEADER_S;


#define RECV_DATA_BUF_SIZE (2*1024*1024)

#define _DUMP_WRITE

class rtxp_client
{
public:
    rtxp_client(std::string& strUrl)
    {
        m_hanlde = NULL;
        m_strUrl = strUrl;
#ifdef _DUMP_WRITE
        m_WriteFd = NULL;
#endif
    }
    virtual ~rtxp_client()
    {

    }
    int start()
    {
        m_hanlde = mk_create_client_handle((char*)m_strUrl.c_str(),rtxp_client_handle_status,this);
        if(NULL == m_hanlde) {
            printf("create client handle fail.url:[%s]\n",m_strUrl.c_str());
            return -1;
        }
        mk_create_rtsp_client_set_tcp(m_hanlde);
#ifdef _DUMP_WRITE
        m_WriteFd = fopen("./a.264","wb+");
        if(NULL == m_WriteFd) {
            return -1;
        }
#endif
        return mk_start_client_handle(m_hanlde,m_szBuf,RECV_DATA_BUF_SIZE,rtxp_client_handle_media,this);
    }
    void close()
    {
        if(NULL == m_hanlde) {
            return;
        }
        mk_stop_client_handle(m_hanlde);
#ifdef _DUMP_WRITE
        if(NULL != m_WriteFd) {
            fclose(m_WriteFd);
            m_WriteFd = NULL;
            printf("close file handle.");
        }
#endif
        return;
    }
    void get_client_rtp_stat_info(RTP_PACKET_STAT_INFO &info)
    {
        if(NULL == m_hanlde) {
            return;
        }
        mk_get_client_rtp_stat_info(m_hanlde,info);
        return;
    }
    int32_t handle_lib_media_data(MR_CLIENT client,MR_MEDIA_TYPE type,MR_MEDIA_CODE code,uint32_t pts,char* data,uint32_t len)
    {
        if(type == MR_MEDIA_TYPE_H264) {
#ifdef _DUMP_WRITE
            fwrite(data,len,1,m_WriteFd);
#endif
            NALU_HEADER* nalu = (NALU_HEADER*)&data[4];
            printf("H264 NALU:[%d]\n",nalu->TYPE);
        }
        else if(type == MR_MEDIA_TYPE_H265){
            fwrite(data,len,1,m_WriteFd);
            NALU_HEADER_S* nalu = (NALU_HEADER_S*)&data[4];
            printf("H265 NALU:[%d]\n",nalu->TYPE);
        }
        printf("data start:[0x%0x 0x%0x 0x%0x 0x%0x]\n",data[0],data[1],data[2],data[3]);
        return mk_recv_next_media_data(client,m_szBuf,RECV_DATA_BUF_SIZE);
    }
    int32_t hanlde_lib_status(MR_CLIENT client,MR_CLIENT_STATUS status)
    {
        if(MR_CLIENT_STATUS_CONNECTED == status) {
            printf("connected,url:[%s]\n",m_strUrl.c_str());
        }
        else if(MR_CLIENT_STATUS_HANDSHAKE == status) {
            printf("handshake,url:[%s]\n",m_strUrl.c_str());
        }
        else if(MR_CLIENT_STATUS_RUNNING == status) {
            printf("running,url:[%s]\n",m_strUrl.c_str());
        }
        else if(MR_CLIENT_STATUS_TEARDOWN == status) {
            printf("teardown,url:[%s]\n",m_strUrl.c_str());
            if(NULL != m_hanlde) {
                mk_destory_client_handle(m_hanlde);
                m_hanlde = NULL;
                delete this;
            }
        }
        else if(MR_CLIENT_STATUS_TIMEOUT == status) {
            printf("timeout,url:[%s]\n",m_strUrl.c_str());
            if(NULL != m_hanlde) {
                mk_stop_client_handle(m_hanlde);
            }
        }
        return 0;
    }
public:
    static int32_t rtxp_client_handle_status(MR_CLIENT client,MR_CLIENT_STATUS status,void* ctx)
    {
        rtxp_client* pClient = (rtxp_client*)ctx;
        return pClient->hanlde_lib_status(client,status);
    }
    static int32_t rtxp_client_handle_media(MR_CLIENT client,MR_MEDIA_TYPE type,MR_MEDIA_CODE code,uint32_t pts,char* data,uint32_t len,void* ctx)
    {
         rtxp_client* pClient = (rtxp_client*)ctx;
         return pClient->handle_lib_media_data(client,type,code,pts,data,len);
    }
private:
    std::string m_strUrl;
    MR_CLIENT   m_hanlde;
    char        m_szBuf[RECV_DATA_BUF_SIZE];
    FILE*       m_WriteFd;
};

static void lib_mk_log(const char* szFileName, int32_t lLine,int32_t lLevel, const char* format,va_list argp)
{
    char buf[1024];
    (void)::vsnprintf(buf, 1024, format, argp);
    buf[1023] = '\0';
    printf("%s:%d %s\n",szFileName,lLine,buf);    
}

int main(int argc,char* argv[])
{
    std::list<rtxp_client*> clientList;
    rtxp_client* pClient = NULL;
    std::string  strUrl;

    if(1 >= argc) {
        printf("need input rtsp/rtmp url\n");
        return -1;
    }

    if(0 != mk_lib_init(2,lib_mk_log)) {
        printf("init lib fail.\n");
        return -1;
    }
    int nCount = argc - 1;

    for(int i = 0;i < nCount;i++) {
        strUrl = argv[i+1];
        try {
            pClient = new rtxp_client(strUrl);
        }
        catch(...) {
            printf("create client:[%d] ,url:[%s] fail\n",i,strUrl.c_str());
            return -1;
        }
        clientList.push_back(pClient);
    }
    
    std::list<rtxp_client*>::iterator iter = clientList.begin();
    for(;iter != clientList.end();++iter) {
        pClient = *iter;
        if(0 != pClient->start()) {
            return -1;
        }
    }
    uint32_t count = 0;
    while(true) {
        count ++;
        if(count < 32)
        {
            std::list<rtxp_client*>::iterator iter = clientList.begin();
            for(;iter != clientList.end();++iter) {
                pClient = *iter;
               
            RTP_PACKET_STAT_INFO info ={0};
            pClient->get_client_rtp_stat_info(info);
            printf("ulTotalPackNum:[%d] ,ulLostRtpPacketNum:[%d] ,ulLostFrameNum:[%d] ,ulDisorderSeqCounts:[%d] \n"
                ,info.ulTotalPackNum
                ,info.ulLostRtpPacketNum
                ,info.ulLostFrameNum,info.ulDisorderSeqCounts);
             }
            sleep(1);
        }
        else{
            break;
        }
    }

    iter = clientList.begin();
    for(;iter != clientList.end();++iter) {
        pClient = *iter;
        pClient->close();
    }

    return 0;
}