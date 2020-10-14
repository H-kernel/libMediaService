#ifndef __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__
#define __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__

#include "as.h"
#include "libMediaService.h"

extern mk_log g_log;
static void mk_log_writer(const char* pszfile,uint32_t line,uint32_t level,const char* format,...)
{
    if(NULL != g_log) {
        va_list argp;
        va_start(argp, format);            
        g_log(pszfile, line,level,format,argp);
        va_end(argp);
    }
}

#define MK_LOG(leve,format,...) mk_log_writer(__FILE__, __LINE__,leve,format,##__VA_ARGS__);

#endif /* __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__ */