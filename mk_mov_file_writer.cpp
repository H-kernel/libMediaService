#include "mk_mov_file_writer.h"

mk_mov_file_writer::mk_mov_file_writer()
{
    m_szMp4Path = NULL;
    m_fp        = NULL;
    m_mov       = NULL;

    m_lVideoTrack          = -1;
	m_lWidth               = 0;
    m_lHeight              = 0;
	m_ulVideo_pts          = 0;
    m_ulVideo_pts_last     = 0;

    memset(&m_avc,0,sizeof(struct mpeg4_avc_t));
    memset(&m_hevc,0,sizeof(struct mpeg4_hevc_t));

    m_lAudioTrack          = -1;
	m_ulAudioChannels      = MOV_WRITER_G711_CHANNEL;
	m_ulAudioBitsPerSample = MOV_WRITER_G711_BITS;
	m_ulAudio_pts          = 0;
    m_ulAudio_pts_last     = 0;
}

mk_mov_file_writer::~mk_mov_file_writer()
{
    if(NULL != m_szMp4Path)
    {
        delete[] m_szMp4Path;
        m_szMp4Path = NULL;
    }
    if(NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
    if(NULL != m_mov)
    {
        mov_writer_destroy(m_mov);
        m_mov = NULL;
    }
}

int32_t mk_mov_file_writer::open_writer(const char *path)
{
    m_szMp4Path = strdup(path);
    
    m_fp = fopen(m_szMp4Path, "wb+");
    if (NULL == m_fp)
    {
        MK_LOG(AS_LOG_ERROR, "open mp4path fail");
        return AS_ERROR_CODE_FAIL;
    }
    m_mov = mov_writer_create(mov_file_buffer(), m_fp, MOV_FLAG_FASTSTART);
    if(NULL == m_mov) {
        fclose(m_fp);
        m_fp = NULL;
        MK_LOG(AS_LOG_ERROR, "create mov writer fail");
        return AS_ERROR_CODE_FAIL;
    }
    return AS_ERROR_CODE_OK;
}

int32_t mk_mov_file_writer::close_writer()
{
    MK_LOG(AS_LOG_INFO, "mk_mov_file_writer close");
    if (NULL != m_mov)
    {
        MK_LOG(AS_LOG_INFO, " mov_writer_destroy");
        mov_writer_destroy(m_mov);
        m_mov = NULL;
    }
    if (NULL != m_fp)
    {
        fclose(m_fp);
        m_fp = NULL;
    }
    if (NULL != m_szMp4Path)
    {
        delete[] m_szMp4Path;
        m_szMp4Path = NULL;
    }
	return AS_ERROR_CODE_OK;
}

int32_t mk_mov_file_writer::write_media_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens)
{
    if(NULL == info) {
        return AS_ERROR_CODE_FAIL;
    }

    switch (info->type)
    {
        case MR_MEDIA_TYPE_H264:
        {
            return write_h264_frame(info,data,lens);
        }
        case MR_MEDIA_TYPE_H265:
        {
            return write_h265_frame(info,data,lens);
        }
        case MR_MEDIA_TYPE_G711A:
        case MR_MEDIA_TYPE_G711U:
        {
            return write_g711_frame(info,data,lens);
        }
        default:
            break;
    }
    return AS_ERROR_CODE_OK;
}

int32_t mk_mov_file_writer::write_h264_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens)
{
	if (NULL == m_mov)
	{
		MK_LOG(AS_LOG_ERROR, "write h264 frame, mov is NULL");
		return AS_ERROR_CODE_FAIL;
	}
	int vcl = 0, update = 0;
    uint32_t pts = 0;

    if(0 == m_ulVideo_pts) {
        m_ulVideo_pts = info->pts;
    }

    if(m_ulVideo_pts < info->pts) {
        pts = m_ulVideo_pts_last + info->pts - m_ulVideo_pts;
    }

    m_ulVideo_pts_last = pts;
    m_ulVideo_pts = info->pts;

    int n = h264_annexbtomp4(&m_avc, data , lens, s_buffer, sizeof(s_buffer), &vcl,&update);

    if (m_lVideoTrack < 0)
    {
        if (m_avc.nb_sps < 1 || m_avc.nb_pps < 1)
        {
            MK_LOG(AS_LOG_INFO,"write_h264_frame waiting for sps/pps.");
            return AS_ERROR_CODE_OK; // waiting for sps/pps
        }

        int extra_data_size = mpeg4_avc_decoder_configuration_record_save(&m_avc, s_extra_data, sizeof(s_extra_data));
        if (extra_data_size <= 0)
        {
            MK_LOG(AS_LOG_INFO,"write_h264_frame invalid AVCC.");
            return AS_ERROR_CODE_OK;
        }

        // TODO: waiting for key frame ???
        m_lVideoTrack = mov_writer_add_video(m_mov, MOV_OBJECT_H264, m_lWidth, m_lHeight, s_extra_data, extra_data_size);
        MK_LOG(AS_LOG_INFO,"write_h264_frame mov_writer_add_video :%d.",m_lVideoTrack);
        if (m_lVideoTrack < 0)
        {
            MK_LOG(AS_LOG_ERROR,"write_h264_frame mp4 track error");
            return AS_ERROR_CODE_FAIL;
        }		
    }

    return mov_writer_write(m_mov, m_lVideoTrack, s_buffer, n, pts, pts, 1 == vcl ? MOV_AV_FLAG_KEYFREAME : 0);
}

int32_t mk_mov_file_writer::write_h265_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens)
{
    if (NULL == m_mov)
	{
		MK_LOG(AS_LOG_ERROR, "write h265 frame,mov is NULL");
		return AS_ERROR_CODE_FAIL;
	}

    uint32_t pts = 0;

    if(0 == m_ulVideo_pts) {
        m_ulVideo_pts = info->pts;
    }

    if(m_ulVideo_pts < info->pts) {
        pts = m_ulVideo_pts_last + info->pts - m_ulVideo_pts;
    }

    m_ulVideo_pts_last = pts;
    m_ulVideo_pts = info->pts;

    int vcl = 0;
	int update = 0;
	int n = h265_annexbtomp4(&m_hevc, data, lens, s_buffer, sizeof(s_buffer), &vcl, &update);

	if (m_lVideoTrack < 0)
	{
		if (m_hevc.numOfArrays < 1)
		{
			//ctx->ptr = end;
            MK_LOG(AS_LOG_INFO,"write_h265_frame waiting for vps/sps/pps.");
			return AS_ERROR_CODE_OK; // waiting for vps/sps/pps
		}

		int extra_data_size = mpeg4_hevc_decoder_configuration_record_save(&m_hevc, s_extra_data, sizeof(s_extra_data));
		if (extra_data_size <= 0)
		{
			// invalid HVCC
			//assert(0);
			return AS_ERROR_CODE_OK;
		}

		// TODO: waiting for key frame ???
		m_lVideoTrack = mov_writer_add_video(m_mov, MOV_OBJECT_HEVC, m_lWidth, m_lHeight, s_extra_data, extra_data_size);
		if (m_lVideoTrack < 0) {
            MK_LOG(AS_LOG_ERROR,"write_h265_frame mp4 track error");
			return AS_ERROR_CODE_FAIL;
        }
	}
	
    return mov_writer_write(m_mov, m_lVideoTrack, s_buffer, n, pts, pts, 1 == vcl ? MOV_AV_FLAG_KEYFREAME : 0);
}

int32_t mk_mov_file_writer::write_g711_frame(MEDIA_DATA_INFO* info,char* data,uint32_t lens)
{
    if(m_lAudioTrack < 0) {
        if(MR_MEDIA_TYPE_G711A == info->code) {
            m_lAudioTrack = mov_writer_add_audio(m_mov, MOV_OBJECT_G711a, m_ulAudioChannels, m_ulAudioBitsPerSample, MOV_WRITER_G711_SAMPLE, NULL, 0);
        }
        else if(MR_MEDIA_TYPE_G711U == info->code) {
            m_lAudioTrack = mov_writer_add_audio(m_mov, MOV_OBJECT_G711u, m_ulAudioChannels, m_ulAudioBitsPerSample, MOV_WRITER_G711_SAMPLE, NULL, 0);
        }

        if(m_lAudioTrack < 0) {
            MK_LOG(AS_LOG_ERROR,"write_g711_frame mp4 track error");
			return AS_ERROR_CODE_FAIL;
        }
    }

    uint32_t pts = 0;
    uint32_t need = lens;
    char* ptr = data;

    if(0 == m_ulAudio_pts) {
        m_ulAudio_pts = info->pts;
    }

    if(m_ulAudio_pts < info->pts) {
        pts = m_ulAudio_pts_last + info->pts - m_ulAudio_pts;
    }

    while (0 < need)
	{
		int n = 320 < need ? 320 : need;
		mov_writer_write(m_mov, m_lAudioTrack, ptr, n, pts, pts, 0);
		pts  += n / 8;
		need -= n;
        ptr  += n;
	}

    m_ulAudio_pts_last = pts;
    m_ulAudio_pts = info->pts;

    return AS_ERROR_CODE_OK;
}