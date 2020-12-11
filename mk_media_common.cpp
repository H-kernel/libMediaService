#include "as.h"
#include "libMediaService.h"
#include "mk_media_common.h"

extern mk_log g_log;
void mk_log_writer(const char* pszfile,uint32_t line,uint32_t level,const char* format,...)
{
    if(NULL != g_log) {
        va_list argp;
        va_start(argp, format);            
        g_log(pszfile, line,level,format,argp);
        va_end(argp);
    }
}
