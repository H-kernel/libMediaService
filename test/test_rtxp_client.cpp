#include <stdlib.h>
#include <stdio.h>
#include <string>
 #include <string.h>
#include <list>
//#include "as.h"
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

typedef struct tagASUrl
{
    char     protocol[8 + 1];
    char     username[64 + 1];
    char     password[64 + 1];
    char     host[512 + 1];
    uint16_t port;
    char     path[512 + 1];
    char     uri[512 + 1];
    char     args[512 + 1];
}as_url_t;

#define RECV_DATA_BUF_SIZE (2*1024*1024)

#define _DUMP_WRITE

class rtxp_client
{
public:
    rtxp_client(std::string& strUrl)
    {
        m_hanlde = NULL;
        m_strUrl = strUrl;
        printf("rtxp_client.url:[%s]\n",m_strUrl.c_str());
#ifdef _DUMP_WRITE
        m_WriteFd = NULL;
        b_first   = true;
#endif
        memset(&m_url,0 ,sizeof(as_url_t));
        m_media_cb.ctx = this;
        m_media_cb.m_cb_status = rtxp_client_handle_status;
        m_media_cb.m_cb_data = rtxp_client_handle_media;
        m_media_cb.m_cb_buffer = rtxp_client_handle_buffer;
    }
    virtual ~rtxp_client()
    {

    }
    int start()
    {
        m_hanlde = mk_create_client_handle((char*)m_strUrl.c_str(),&m_media_cb,this);
        if(NULL == m_hanlde) {
            printf("create client handle fail.url:[%s]\n",m_strUrl.c_str());
            return -1;
        }
        mk_create_rtsp_client_set_tcp(m_hanlde);
        if(0 != as_parse_url(m_strUrl.c_str(),&m_url)) {
            printf("parse url:[%s] fail.\n",m_strUrl.c_str());
            return -1;
        }
        printf("parse url path:[%s].\n",m_url.path);
        return mk_start_client_handle(m_hanlde);
    }
    void close()
    {
        if(NULL == m_hanlde) {
            return;
        }
        mk_stop_client_handle(m_hanlde);
        return;
    }
    void get_client_rtp_stat_info(RTP_PACKET_STAT_INFO &info)
    {
        if(NULL == m_hanlde) {
            return;
        }
        mk_get_client_rtp_stat_info(m_hanlde,&info);
        return;
    }
    void get_client_rtsp_sdp_info(char* info,uint32_t size,uint32_t& len)
    {
        if(NULL == m_hanlde) {
            return;
        }
        mk_get_client_rtsp_sdp_info(m_hanlde,info,size,len);
        return;
    }
    int32_t handle_lib_media_data(MR_CLIENT client,MEDIA_DATA_INFO* dataInfo,uint32_t len)
    {
        if(dataInfo->type == MR_MEDIA_TYPE_H264) {
            NALU_HEADER* nalu = (NALU_HEADER*)&m_szBuf[4];
            printf("H264 NALU:[%d]\n",nalu->TYPE);
            if(b_first)
            {
                if(7 == nalu->TYPE)
                {
                    b_first =false;
#ifdef _DUMP_WRITE
                    std::string filename = "";
                    snprintf((char*)filename.c_str(),512,"%s%s",m_url.path,".264");
                     
                    m_WriteFd = fopen(filename.c_str(),"wb+");
                    if(NULL == m_WriteFd) {
                        return -1;
                    }
#endif
                }
                else{
                    return mk_recv_next_media_data(client);
                }
            }
#ifdef _DUMP_WRITE
            if(NULL != m_WriteFd)
            {
                fwrite(m_szBuf,len,1,m_WriteFd);
            }
#endif              
        }
        else if(dataInfo->type == MR_MEDIA_TYPE_H265){
            //fwrite(m_szBuf,len,1,m_WriteFd);
            NALU_HEADER_S* nalu = (NALU_HEADER_S*)&m_szBuf[4];
            printf("H265 NALU:[%d]\n",nalu->TYPE);
            if(b_first)
            {
                if(32 == nalu->TYPE)
                {
                    b_first =false;
#ifdef _DUMP_WRITE
                    std::string filename = "";
                    snprintf((char*)filename.c_str(),512,"%s%s",m_url.path,".265");
                     
                    m_WriteFd = fopen(filename.c_str(),"wb+");
                    if(NULL == m_WriteFd) {
                        return -1;
                    }
#endif
                }
                else{
                    return mk_recv_next_media_data(client);
                }
            }
#ifdef _DUMP_WRITE
            if(NULL != m_WriteFd)
            {
                fwrite(m_szBuf,len,1,m_WriteFd);
            }
#endif              
        }
        printf("data start:[0x%0x 0x%0x 0x%0x 0x%0x]\n",m_szBuf[0],m_szBuf[1],m_szBuf[2],m_szBuf[3]);
        return mk_recv_next_media_data(client);
    }
    int32_t hanlde_lib_status(MR_CLIENT client,MR_CLIENT_STATUS status)
    {
        if(MR_CLIENT_STATUS_CONNECTED == status) {
            printf("connected,url:[%s]\n",m_strUrl.c_str());
        }
        else if(MR_CLIENT_STATUS_HANDSHAKE == status) {
            printf("handshake,url:[%s]\n",m_strUrl.c_str());
            char sz[1024] = {0};
            uint32_t len = 0;
            this->get_client_rtsp_sdp_info(&sz[0],sizeof(sz),len);
            printf("handshake,sdp info:[%s],length[%d]\n",&sz[0],len);
        }
        else if(MR_CLIENT_STATUS_RUNNING == status) {
            printf("running,url:[%s]\n",m_strUrl.c_str());
        }
        else if(MR_CLIENT_STATUS_TEARDOWN == status) {
            printf("teardown,url:[%s]\n",m_strUrl.c_str());
            if(NULL != m_hanlde) {
                mk_destory_client_handle(m_hanlde);
                m_hanlde = NULL;
#ifdef _DUMP_WRITE
                if(NULL != m_WriteFd) {
                    fclose(m_WriteFd);
                    m_WriteFd = NULL;
                    printf("close file handle.");
                }
#endif
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
    char* handle_lib_buffer(MR_CLIENT client,uint32_t len,uint32_t& ulBufLen)
    {
        if(RECV_DATA_BUF_SIZE < len) {
            return NULL;
        }
        ulBufLen = RECV_DATA_BUF_SIZE;
        return &m_szBuf[0];
    }
    int32_t    as_parse_url(const char* url,as_url_t* info)
    {
        if(NULL == url) {
            return -1;
        }
        uint32_t len = 0;
        char* data = (char*)url;
        /* find protocol */
        char* idx = strstr(data,"://");
        if(idx == NULL) {
            strncpy(info->protocol,"unknow",8);
        }
        else {
            len = idx - data;
            if(len >= 8) {
                return -1;
            }
            strncpy(info->protocol,data,len);
            data =idx + 3;/* skip the '://' */
        }

        /* find username and password */
        idx = strrchr(data,'@');
        if(NULL != idx) {
            char* username = data;
            data = idx + 1; /* skip the '@' */
            *idx = '\0';
            char* password = strchr(username,':');
            if(NULL == password) {
                len = idx - username;
                strncpy(info->username,username,len);
            }
            else {
                len = password - username;
                strncpy(info->username,username,len);
                password += 1; /* skip the ':' */
                len = idx - password;
                strncpy(info->password,password,len);
            }
            *idx = '@';
        }

        /* find host and port*/
        idx = strchr(data,':');
        if(NULL == idx) {
            /* default port*/
            idx = strchr(data,'/');
            if(NULL == idx) {
                strncpy(info->host,data,512);
                return 0;
            }
            len = idx - data;
            if(len >= 512) {
                return -1;
            }
            strncpy(info->host,data,len);
            data = idx;
        }
        else {
            len = idx - data;
            if(len >= 512) {
                return -1;
            }
            strncpy(info->host,data,len);
            data =idx + 1;/* skip the ':' */

            /* parse the port */
            idx = strchr(data,'/');
            if(NULL != idx) {
                *idx = '\0';
            }
            info->port = atoi(data);
            if(NULL == idx) {
                return 0;
            }
            *idx = '/';
            data = idx;
        }
        strncpy(info->uri,data,512);

        idx = strchr(data,'/');
        if(NULL != idx) {
            data = idx + 1;
        }

        idx = strchr(data,'/');
        if(NULL != idx) {
            data = idx + 1;
        }
        
        /* parse the path */
        idx = strchr(data,'?');
        if(NULL == idx) {
            strncpy(info->path,data,512);
            info->args[0] = '\0';
        }
        else {
            len = idx - data;
            if(len >= 512) {
                return -1;
            }
            strncpy(info->path,data,len);
            data =idx + 1;/* skip the '?' */
            strncpy(info->args,data,512);
        }
        return 0;
    }
public:
    static int32_t rtxp_client_handle_status(MR_CLIENT client,MR_CLIENT_STATUS status,void* ctx)
    {
        rtxp_client* pClient = (rtxp_client*)ctx;
        return pClient->hanlde_lib_status(client,status);
    }
    static int32_t rtxp_client_handle_media(MR_CLIENT client,MEDIA_DATA_INFO* dataInfo,uint32_t len,void* ctx)
    {
         rtxp_client* pClient = (rtxp_client*)ctx;
         return pClient->handle_lib_media_data(client,dataInfo,len);
    }
    static char* rtxp_client_handle_buffer(MR_CLIENT client,uint32_t len,uint32_t& ulBufLen,void* ctx)
    {
        rtxp_client* pClient = (rtxp_client*)ctx;
        return pClient->handle_lib_buffer(client,len,ulBufLen);
    }
private:
    std::string m_strUrl;
    MR_CLIENT   m_hanlde;
    char        m_szBuf[RECV_DATA_BUF_SIZE];
    FILE*       m_WriteFd;
    MEDIA_CALL_BACK m_media_cb;
    bool   b_first;
    as_url_t    m_url;
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

    if(0 != mk_lib_init(2,lib_mk_log,600,100)) {
        printf("init lib fail.\n");
        return -1;
    }
    int nCount = argc - 1;
    printf("create client nCount:[%d] \n",nCount);
    for(int i = 0;i < nCount;i++) {
        strUrl = argv[i+1];
        try {
            pClient = new rtxp_client(strUrl);
        }
        catch(...) {
            printf("create client:[%d] ,url:[%s] fail\n",i,strUrl.c_str());
            return -1;
        }
        printf("create client:[%d] ,url:[%s]\n",i,strUrl.c_str());
        clientList.push_back(pClient);
    }
    printf("create client url:[%s]\n",strUrl.c_str());
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
        if(count < 10)
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