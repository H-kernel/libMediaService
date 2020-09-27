#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include "../libMediaService.h"
typedef struct
{
    //byte 0
    uint8_t TYPE:5;
    uint8_t NRI:2;
    uint8_t F:1;
} NALU_HEADER; /**//* 1 BYTES */
#define RECV_DATA_BUF_SIZE (1024*1024)

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
        }
#endif
        return;
    }
    int32_t handle_lib_media_data(MR_CLIENT client,MR_MEDIA_TYPE type,uint32_t pts,char* data,uint32_t len)
    {
        if(type == MR_MEDIA_TYPE_H264) {
#ifdef _DUMP_WRITE
            fwrite(data,1,len,m_WriteFd);
#endif
            NALU_HEADER* nalu = (NALU_HEADER*)&data[4];
            printf("H264 NALU:[%d]\n",nalu->TYPE);
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
        return AS_ERROR_CODE_OK;
    }
public:
    static int32_t rtxp_client_handle_status(MR_CLIENT client,MR_CLIENT_STATUS status,void* ctx)
    {
        rtxp_client* pClient = (rtxp_client*)ctx;
        return pClient->hanlde_lib_status(client,status);
    }
    static int32_t rtxp_client_handle_media(MR_CLIENT client,MR_MEDIA_TYPE type,uint32_t pts,char* data,uint32_t len,void* ctx)
    {
         rtxp_client* pClient = (rtxp_client*)ctx;
         return pClient->handle_lib_media_data(client,type,pts,data,len);
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

    if(AS_ERROR_CODE_OK != mk_lib_init(2,lib_mk_log)) {
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

    while(true) {
        sleep(30);
        break;
    }

    iter = clientList.begin();
    for(;iter != clientList.end();++iter) {
        pClient = *iter;
        pClient->close();
    }

    return 0;
}