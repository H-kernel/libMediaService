#ifndef __mk_mov_file_buffer_h__
#define __mk_mov_file_buffer_h__

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mov_file_cache_t
{
	FILE* fp;
	uint8_t ptr[800];
	unsigned int len;
	unsigned int off;
	uint64_t tell;
};

const struct mov_buffer_t* mov_file_cache_buffer(void);

const struct mov_buffer_t* mov_file_buffer(void);

#ifdef __cplusplus
}
#endif
#endif /* !__mk_mov_file_buffer_h__ */
