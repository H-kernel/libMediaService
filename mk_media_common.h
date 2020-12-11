#ifndef __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__
#define __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__

#include "as.h"
#include "libMediaService.h"

void mk_log_writer(const char* pszfile, uint32_t line, uint32_t level, const char* format, ...);

#define MK_LOG(leve,format,...) mk_log_writer(__FILE__, __LINE__,leve,format,##__VA_ARGS__);

#endif /* __MK_CLIENT_MEDIA_COMMON_INCLUDE_H__ */