#ifndef _mov_file_buffer_h_
#define _mov_file_buffer_h_
#include "mov-buffer.h"
#include <stdio.h>
#if defined(__cplusplus)
extern "C" {
#endif
const struct mov_buffer_t* mov_file_buffer(void);
#if defined(__cplusplus)
}
#endif
#endif