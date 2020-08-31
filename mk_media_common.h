#ifndef __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__
#define __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__

#include "libMediaService.h"

extern mk_log g_log;

#define MK_LOG(leve,format,...) \
    if(NULL != g_log) {         \
        g_log(__FILE__, __LINE__,leve,format,##__VA_ARGS__) \
    } 

#endif /* __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__ */