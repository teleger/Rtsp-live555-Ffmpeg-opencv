#include "API_H264Decode.hpp"
//----------------------------ffmpeg　　h264解码类
API::H264DecodeDev::H264DecodeDev():
m_pcodec(nullptr),
m_pCodeCtx(nullptr),
m_pFrame(nullptr),//m_pCodecParserCtx(nullptr),
m_nFrameNumber(0),
m_nWidth(0),
m_nHeight(0)
{
}
API::H264DecodeDev::~H264DecodeDev(){
  if (m_pFrame != nullptr)
  {
    av_frame_free(&m_pFrame);
    m_pFrame = nullptr;
  }
  if (m_pcodec != nullptr)
  {
    avcodec_close(m_pCodeCtx);
    av_free(m_pcodec);
    m_pcodec = nullptr;
  }
/*  if (m_pCodecParserCtx != nullptr)
  {
    av_parser_close(m_pCodecParserCtx);
    m_pCodecParserCtx = nullptr;
  }*/
}
bool API::H264DecodeDev::InitializeDev(LPGetFramePtr  pCallBack,int width,int height){
   if (nullptr == pCallBack){
      LLERROR("H264  CallBack_Ptr is nullptr");
      return false;
    }
    if (0 == width || 0 == height){
      LLERROR("H264  width or height is zero");
      return false;
    }
    m_pCallBack = pCallBack;

    m_nWidth    = width;
    m_nHeight   = height;

    m_pcodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(m_pcodec != nullptr)
    {
       m_pcodec->capabilities |= CODEC_CAP_DELAY;
       m_pCodeCtx = avcodec_alloc_context3(m_pcodec);
       //m_pCodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
       if(m_pCodeCtx != nullptr)
       {
          //if((nullptr ==  m_pCodeCtx->extradata) || (0 == m_pCodeCtx->extradata_size))
          //m_pCodeCtx->width =   m_nWidth;
          //m_pCodeCtx->height =  m_nHeight;
          //m_pCodeCtx->codec_id =  AV_CODEC_ID_H264;
          //m_pCodeCtx->codec_type = AVMEDIA_TYPE_VIDEO;
          //m_pCodeCtx->pix_fmt =   AV_PIX_FMT_YUV420P;
          //m_pCodecParserCtx = av_parser_init(AV_CODEC_ID_H264);
          //if(m_pCodecParserCtx != nullptr)
          // {
                if(avcodec_open2(m_pCodeCtx,m_pcodec,nullptr) < 0)
                {
                    LLERROR("avcodec_open2  failed ");
                    return false;
                }
                else
                {
                    m_pFrame = av_frame_alloc();
                    //avcodec_send_packet(m_pCodeCtx, nullptr);
                    //avcodec_flush_buffers(m_pCodeCtx);
                    av_init_packet(&m_Packet);
                    //LLDEBUG("Init Decode ok"); 
                    return true;
                }
           // }
           /* else
            {
                LLERROR("av_parser_init failed");
                return false;
            }*/
        }
        else
        {
          LLERROR("avcodec_alloc_context3 failed");
          return false;
        }
    }
    else
    {
       LLERROR("avcodec_find_decoder failed");
       return false;
    }
    return false;
}

bool API::H264DecodeDev::ProcessDecode(uint8_t* pBuffer,int nSize,const char* fStreamId) //rtsp方式获取数据流回调函数
{
    bool            bReturn = true;
    m_Packet.data           = pBuffer;
    m_Packet.size           = nSize;

    int getpicture = avcodec_send_packet(m_pCodeCtx, &m_Packet);//avcodec_flush_buffers 
    if(getpicture == 0)
    {
        getpicture = avcodec_receive_frame(m_pCodeCtx, m_pFrame);
        av_packet_unref(&m_Packet);
        if (getpicture == 0)
        {
            bReturn = true;
            ++m_nFrameNumber;
            //LLDEBUG("FrameNum:%d",m_nFrameNumber);
            int   nDataSize = FrameChangeRGB24(m_pFrame, nullptr); //获取每帧长度
            if(nDataSize)
            {
                try
                {
                    unsigned char*   pImage = new unsigned char[nDataSize];
                    if(pImage != nullptr)
                    {
                          memset(pImage,0,nDataSize);
                          FrameChangeRGB24(m_pFrame,&pImage);
                          RedChangeBlue(pImage,m_nWidth,m_nHeight);
                          if(m_pCallBack != nullptr)
                          {
                              m_pCallBack(pImage,m_nFrameNumber,fStreamId); 
                              delete[] pImage;
                          }      
                    }
                }
                catch(const std::bad_alloc& e)
                {
                    bReturn = false;
                    LLERROR("new malloc exception:%s",e.what());
                    return bReturn;
                }
                catch(const std::exception& e)
                {
                    bReturn = false;
                    LLERROR("other  exception: %s",e.what());
                    return bReturn;
                }
            }
        }
    }
    return bReturn;
}

//RGB24红蓝调换
void API::H264DecodeDev::RedChangeBlue(uint8_t* pRGB, int nWidth, int nHeight)
{
  for (int x = 0; x < nHeight; ++x)
  {
    for (int y = 0; y < nWidth; ++y)
    {
      int   nSize = (x * nWidth + y) * 3;
      uint8_t uTemp = pRGB[nSize + 0];
      pRGB[nSize + 0] = pRGB[nSize + 2];
      pRGB[nSize + 2] = uTemp;
    }
  }
}
int API::H264DecodeDev::FrameChangeRGB24(AVFrame* pFrame, uint8_t** pRGB)
{
    int   iSize = m_nWidth * m_nHeight * 3;
    if(pRGB == nullptr)
    {
        return iSize;
    }
    struct SwsContext* img_convert_ctx = nullptr;
    int linesize[4] = { 3 * m_nWidth, 0, 0, 0 };
    //LLDEBUG("Codex width: %d ,pFrame width :%d , pFrame height: %d",m_pCodeCtx->width,pFrame->width,pFrame->height);
    img_convert_ctx = sws_getContext(pFrame->width, pFrame->height, AV_PIX_FMT_YUV420P, m_nWidth, m_nHeight, AV_PIX_FMT_RGB24, SWS_POINT, nullptr, nullptr, nullptr);
    if(img_convert_ctx != nullptr)
    {
      sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pFrame->height, (uint8_t**)pRGB, linesize);
      sws_freeContext(img_convert_ctx);
    }
    return 0;
}