#include "API_RTSPClient.hpp"

int main(int argc, char** argv)
{
  	avcodec_register_all();
    av_register_all();

    if (argc != 2)
    {
       std::cout<< "Please Input a rtspAddress" << std::endl;
       return -1;
    }
  	API_RTSPSession* pRtsp = new API_RTSPSession;
  	if(pRtsp == NULL)
  	{
  		std::cout<< "RTSPSession is null ,ERROR.." << std::endl;
  		return -1;
  	}
    //设置 回调函数 
    const char *rtsp = argv[1];  
    pRtsp->startRTSPClient(API_RTSPSession::CallBackdata,rtsp);
    
  	return 0;
}