#ifndef __MOV_WRITE_FILE_H__
#define __MOV_WRITE_FILE_H__

#include "mk_media_common.h"
#include "libMediaService.h"
extern "C" {
#include "mov-writer.h"
#include "mov-format.h"
#include "mpeg4-avc.h"
#include "mpeg4-hevc.h"
#include "mpeg4-aac.h"
#include "mov-file-buffer.h"
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOV_WRITER_G711_SAMPLE  8000
#define MOV_WRITER_G711_CHANNEL 1
#define MOV_WRITER_G711_BITS    16



class mk_mov_file_writer
{
public:
	mk_mov_file_writer();
	virtual ~mk_mov_file_writer();

	int32_t open_writer(const char* path);

	int32_t close_writer();

	int32_t write_media_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens);
private:
    int32_t write_h264_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens);

	int32_t write_h265_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens);

	int32_t write_g711_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens);
private:
	
    char*                m_szMp4Path;
    FILE*                m_fp;
	mov_writer_t*        m_mov;
    
	/* info for video */
	int32_t              m_lVideoTrack;
	int32_t              m_lWidth;
    int32_t              m_lHeight;
	uint32_t             m_ulVideo_pts;
	uint32_t             m_ulVideo_dts;

	struct mpeg4_avc_t   m_avc;    // for H264
	struct mpeg4_hevc_t  m_hevc;   // for HEVC

	/* info for audio */
	int32_t              m_lAudioTrack;
	uint32_t             m_ulAudioChannels;
	uint32_t             m_ulAudioBitsPerSample;
	uint32_t             m_ulAudio_pts;
	uint32_t             m_ulAudio_dts;

    uint8_t              s_buffer[2 * 1024 * 1024];
    uint8_t              s_extra_data[64 * 1024];
};
#endif /* __MOV_WRITE_FILE_H__ */