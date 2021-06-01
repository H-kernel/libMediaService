#include "mk_nal_sps_parse.h"
#include <math.h>

int Ue(char *pBuff, int nLen, int &nStartBit)
{
	//计算0bit的个数
	int nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;

	//计算结果
	long dwRet = 0;
	for (int i = 0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}


int Se(char *pBuff, int nLen, int &nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}


long u(int BitCount, char * buf, int &nStartBit)
{
	long dwRet = 0;
	for (int i = 0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

/**
* 解码SPS,获取视频图像宽、高信息
*
* @param buf SPS数据内容
* @param nLen SPS数据的长度
* @param width 图像宽度
* @param height 图像高度

* @成功则返回1 , 失败则返回0
*/
int mk_h264_decode_sps(char * buf, unsigned int nLen, int &width, int &height, int &fps)
{
	int StartBit = 0;
	fps = 0;
	//de_emulation_prevention(buf, &nLen);

	int forbidden_zero_bit = u(1, buf, StartBit);
	int nal_ref_idc = u(2, buf, StartBit);
	int nal_unit_type = u(5, buf, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, buf, StartBit);
		int constraint_set0_flag = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, buf, StartBit);
		int level_idc = u(8, buf, StartBit);

		int seq_parameter_set_id = Ue(buf, nLen, StartBit);
		int chroma_format_idc = 1;									//不存在，默认为1
		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			chroma_format_idc = Ue(buf, nLen, StartBit);
			if (chroma_format_idc == 3)
				int residual_colour_transform_flag = u(1, buf, StartBit);
			int bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
			int bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
			int seq_scaling_matrix_present_flag = u(1, buf, StartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
		int pic_order_cnt_type = Ue(buf, nLen, StartBit);
		if (pic_order_cnt_type == 0)
			int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, buf, StartBit);
			int offset_for_non_ref_pic = Se(buf, nLen, StartBit);
			int offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

			int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
			delete[] offset_for_ref_frame;
		}
		int num_ref_frames = Ue(buf, nLen, StartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
		int pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
		int pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

		width = (pic_width_in_mbs_minus1 + 1) * 16;
		//height = (pic_height_in_map_units_minus1 + 1) * 16;
		

		int frame_mbs_only_flag = u(1, buf, StartBit);
		if (!frame_mbs_only_flag)
			int mb_adaptive_frame_field_flag = u(1, buf, StartBit);

		height = (2 - frame_mbs_only_flag)* ((pic_height_in_map_units_minus1 + 1) * 16);

		int direct_8x8_inference_flag = u(1, buf, StartBit);
		int frame_cropping_flag = u(1, buf, StartBit);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset = Ue(buf, nLen, StartBit);
			int frame_crop_right_offset = Ue(buf, nLen, StartBit);
			int frame_crop_top_offset = Ue(buf, nLen, StartBit);
			int frame_crop_bottom_offset = Ue(buf, nLen, StartBit);

			unsigned int crop_unit_x;
			unsigned int crop_unit_y;
			if (0 == chroma_format_idc) // monochrome
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			else if (1 == chroma_format_idc) // 4:2:0
			{
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (2 == chroma_format_idc) // 4:2:2
			{
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			else // 3 == sps.chroma_format_idc   // 4:4:4
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}
			width -= crop_unit_x * (frame_crop_left_offset +frame_crop_right_offset);
			height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
		}
		int vui_parameter_present_flag = u(1, buf, StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, buf, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, buf, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, buf, StartBit);
					int sar_height = u(16, buf, StartBit);
				}
			}
			int overscan_info_present_flag = u(1, buf, StartBit);
			if (overscan_info_present_flag)
				int overscan_appropriate_flagu = u(1, buf, StartBit);
			int video_signal_type_present_flag = u(1, buf, StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, buf, StartBit);
				int video_full_range_flag = u(1, buf, StartBit);
				int colour_description_present_flag = u(1, buf, StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, buf, StartBit);
					int transfer_characteristics = u(8, buf, StartBit);
					int matrix_coefficients = u(8, buf, StartBit);
				}
			}
			int chroma_loc_info_present_flag = u(1, buf, StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
				int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
			}
			int timing_info_present_flag = u(1, buf, StartBit);
			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, buf, StartBit);
				int time_scale = u(32, buf, StartBit);
				fps = time_scale / (2 * num_units_in_tick);
			}
		}
		return AS_ERROR_CODE_OK;
	}
	else
		return AS_ERROR_CODE_FAIL;
}




#define __int64 long long   
#define LONG long   
#define WORD        unsigned short   
#define DWORD       unsigned int   
#define BYTE        unsigned char   
#define LPBYTE char*   
#define _TCHAR char   
typedef unsigned char uint8;   
   
typedef unsigned short uint16;   
   
typedef unsigned long uint32;   
typedef unsigned __int64 uint64;   
typedef signed char int8;   
typedef signed short int16;   
typedef signed long int32;   
typedef signed __int64 int64;   
typedef unsigned int UINT32;   
typedef  int INT32;   
       
class NALBitstream   
{   
    public:   
    NALBitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) {};   
    NALBitstream(void * data, int len) { Init(data, len); };   
    void Init(void * data, int len) { m_data = (LPBYTE)data; m_len = len; m_idx = 0; m_bits = 0; m_byte = 0; m_zeros = 0; };   
        
    BYTE GetBYTE()   
    {   
        //printf("m_idx=%d,m_len=%d\n", m_idx, m_len);   
        if ( m_idx >= m_len )   
            return 0;   
        BYTE b = m_data[m_idx++];   
       
        // to avoid start-code emulation, a byte 0x03 is inserted   
        // after any 00 00 pair. Discard that here.   
        if ( b == 0 )   
        {   
            m_zeros++;   
            if ( (m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03) )   
            {     
                m_idx++;   
                m_zeros=0;   
            }   
        }    
        else    
        {   
            m_zeros = 0;   
           
        }   
        return b;   
    };   
   
   
    UINT32 GetBit()    
    {   
        //printf("m_bits=%d\n", m_bits);   
        if (m_bits == 0)    
        {   
            m_byte = GetBYTE();   
            m_bits = 8;      
        }   
        m_bits--;   
        return (m_byte >> m_bits) & 0x1;   
    };   
       
    UINT32 GetWord(int bits)    
    {   
        UINT32 u = 0;   
        while ( bits > 0 )   
        {   
            u <<= 1;   
            u |= GetBit();   
            bits--;   
        }   
        return u;   
    };   
    UINT32 GetUE()    
    {   
   
        // Exp-Golomb entropy coding: leading zeros, then a one, then   
        // the data bits. The number of leading zeros is the number of   
        // data bits, counting up from that number of 1s as the base.   
        // That is, if you see   
        //      0001010   
        // You have three leading zeros, so there are three data bits (010)   
        // counting up from a base of 111: thus 111 + 010 = 1001 = 9   
        int zeros = 0;   
        while (m_idx < m_len && GetBit() == 0 ) zeros++;   
        return GetWord(zeros) + ((1 << zeros) - 1);   
    };   
   
   
    INT32 GetSE()   
    {   
       
        // same as UE but signed.   
        // basically the unsigned numbers are used as codes to indicate signed numbers in pairs   
        // in increasing value. Thus the encoded values   
        //      0, 1, 2, 3, 4   
        // mean   
        //      0, 1, -1, 2, -2 etc   
        UINT32 UE = GetUE();   
        bool positive = UE & 1;   
        INT32 SE = (UE + 1) >> 1;   
        if ( !positive )   
        {   
            SE = -SE;   
        }   
        return SE;   
    };   
   
   
    private:   
    LPBYTE m_data;   
    int m_len;   
    int m_idx;   
    int m_bits;   
    BYTE m_byte;   
    int m_zeros;   
};   
   
   
int  mk_hevc_decode_sps(char* buf, unsigned int nLen, int &width, int &height, int &fps)   
{   
    if (nLen < 20)   
    {    
        return -1;   
    }   
       
    NALBitstream bs(buf, nLen);   
       
    // seq_parameter_set_rbsp()   
    bs.GetWord(4);// sps_video_parameter_set_id   
    int sps_max_sub_layers_minus1 = bs.GetWord(3); // "The value of sps_max_sub_layers_minus1 shall be in the range of 0 to 6, inclusive."   
    if (sps_max_sub_layers_minus1 > 6)    
    {   
		return -1;
    }   
    bs.GetWord(1);// sps_temporal_id_nesting_flag   
    // profile_tier_level( sps_max_sub_layers_minus1 )  
    int  general_profile_idc = 0;
    int  general_level_idc = 0;
    {   
        bs.GetWord(2);// general_profile_space   
        bs.GetWord(1);// general_tier_flag   
        general_profile_idc = bs.GetWord(5);// general_profile_idc   
        bs.GetWord(32);// general_profile_compatibility_flag[32]   
        bs.GetWord(1);// general_progressive_source_flag   
        bs.GetWord(1);// general_interlaced_source_flag   
        bs.GetWord(1);// general_non_packed_constraint_flag   
        bs.GetWord(1);// general_frame_only_constraint_flag   
        bs.GetWord(44);// general_reserved_zero_44bits   
        general_level_idc   = bs.GetWord(8);// general_level_idc   
        uint8 sub_layer_profile_present_flag[6] = {0};   
        uint8 sub_layer_level_present_flag[6]   = {0};   
        for (int i = 0; i < sps_max_sub_layers_minus1; i++)    
        {   
            sub_layer_profile_present_flag[i]= bs.GetWord(1);   
            sub_layer_level_present_flag[i]= bs.GetWord(1);   
        }   
        if (sps_max_sub_layers_minus1 > 0)    
        {   
            for (int i = sps_max_sub_layers_minus1; i < 8; i++)    
            {   
                uint8 reserved_zero_2bits = bs.GetWord(2);   
            }   
        }   
        for (int i = 0; i < sps_max_sub_layers_minus1; i++)    
        {   
            if (sub_layer_profile_present_flag[i])    
            {   
                bs.GetWord(2);// sub_layer_profile_space[i]   
                bs.GetWord(1);// sub_layer_tier_flag[i]   
                bs.GetWord(5);// sub_layer_profile_idc[i]   
                bs.GetWord(32);// sub_layer_profile_compatibility_flag[i][32]   
                bs.GetWord(1);// sub_layer_progressive_source_flag[i]   
                bs.GetWord(1);// sub_layer_interlaced_source_flag[i]   
                bs.GetWord(1);// sub_layer_non_packed_constraint_flag[i]   
                bs.GetWord(1);// sub_layer_frame_only_constraint_flag[i]   
                bs.GetWord(44);// sub_layer_reserved_zero_44bits[i]   
            }   
            if (sub_layer_level_present_flag[i])    
            {   
                bs.GetWord(8);// sub_layer_level_idc[i]   
            }   
        }   
    }   
    uint32 sps_seq_parameter_set_id= bs.GetUE(); // "The  value  of sps_seq_parameter_set_id shall be in the range of 0 to 15, inclusive."   
    if (sps_seq_parameter_set_id > 15)    
    {   
		return -1;
    }   
    uint32 chroma_format_idc = bs.GetUE(); // "The value of chroma_format_idc shall be in the range of 0 to 3, inclusive."   
    if (sps_seq_parameter_set_id > 3)   
    {   
		return -1;
    }   
    if (chroma_format_idc == 3)    
    {   
        bs.GetWord(1);// separate_colour_plane_flag   
    }   
    width  = bs.GetUE(); // pic_width_in_luma_samples   
    height  = bs.GetUE(); // pic_height_in_luma_samples   
    if (bs.GetWord(1))    
    {// conformance_window_flag   
        bs.GetUE();  // conf_win_left_offset   
        bs.GetUE();  // conf_win_right_offset   
        bs.GetUE();  // conf_win_top_offset   
        bs.GetUE();  // conf_win_bottom_offset   
    }   
    uint32 bit_depth_luma_minus8= bs.GetUE();   
    uint32 bit_depth_chroma_minus8= bs.GetUE();   
    if (bit_depth_luma_minus8 != bit_depth_chroma_minus8)    
    {   
		return -1;
    }      
    return 0;   
}   
