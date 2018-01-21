#ifndef			_API_H264_DECODE_HPP_
#define			_API_H264_DECODE_HPP_
#include 		"API_Define.hpp"
#include 		<exception>
typedef void(*LPGetFramePtr)(u_int8_t* pImage ,int& frame,const char* CameraIp);
extern "C"
{
	#include <libavutil/opt.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/common.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/mathematics.h>
	#include <libavutil/samplefmt.h>
	#include <libswscale/swscale.h>
	#include <libavformat/avformat.h> 
};
//API命名空间
namespace API
{
	class H264DecodeDev{
	public:
		H264DecodeDev();
		virtual ~H264DecodeDev();
	private:
		AVCodec 			        *m_pcodec;
		AVCodecContext              *m_pCodeCtx;
		AVFormatContext				*m_pFormat;
		//AVCodecParserContext		*m_pCodecParserCtx;
		AVFrame 					*m_pFrame;
		AVPacket        			m_Packet;
	private:
		LPGetFramePtr  				m_pCallBack;
	private:
		int 						m_nWidth;
		int 						m_nHeight;	
	public://ffmpeg 
		virtual bool 				InitializeDev(LPGetFramePtr  pCallBack,int width,int height);//
		int 						FrameChangeRGB24(AVFrame* pFrame, uint8_t** pRGB);
		void 						RedChangeBlue(uint8_t* pRGB, int nWidth, int nHeight);
		virtual bool 				ProcessDecode(uint8_t* pBuffer,int nSize,const char* fStreamId);
	public:
		int  						m_nFrameNumber;
	};
};
#endif