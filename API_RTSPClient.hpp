#ifndef		_API_RTSP_CLIENT_HPP_
#define		_API_RTSP_CLIENT_HPP_
#include 	"UsageEnvironment.hh"
#include 	 "liveMedia.hh"
#include 	"BasicUsageEnvironment.hh"
#include 	"API_Socket.hpp"
#include    "API_Mutex.hpp"
#include    "API_H264Decode.hpp"
#include 	<pthread.h>
#include    <iostream>
#include 	<string>
#include    <vector>
#include    <queue>
#include    <opencv2/opencv.hpp>

using namespace API;
#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096


// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState 
{
	public:
  		StreamClientState();
  	virtual ~StreamClientState();

	public:
	  MediaSubsessionIterator* iter;
	  MediaSession* session;
	  MediaSubsession* subsession;
	  TaskToken streamTimerTask;
	  double duration;
};

class ourRTSPClient: public RTSPClient
{
	public:
	  static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
					  int verbosityLevel = 0,
					  char const* applicationName = NULL,
					  portNumBits tunnelOverHTTPPortNum = 0);

	protected:
  		ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  	virtual ~ourRTSPClient();

	public:
  		StreamClientState scs;
};

class H264MediaSink: public MediaSink 
{
	public:
  	static H264MediaSink* createNew(UsageEnvironment& env,
			      MediaSubsession& subsession, // identifies the kind of data that's being received
			      char const*sPropParameterSetsStr,
			      char const* streamId = NULL); // identifies the stream itself (optional)
protected:
  H264MediaSink(UsageEnvironment& env, MediaSubsession& subsession,char const*sPropParameterSetsStr, char const* streamId);
  virtual ~H264MediaSink();
  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
				struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			 struct timeval presentationTime);
  int  SetParameterStr(u_int8_t* fReceiveBuffer,unsigned& frameSize);
	private:
  	// redefined virtual functions:
  	virtual Boolean continuePlaying();
private:
	u_int8_t* 			fReceiveBuffer;
	u_int8_t* 			fReceiveBufferAV;
	MediaSubsession& 	fSubsession;
	char* 				fStreamId;
	Boolean             bFirstFrame;
	int 				extraDataSize;
	int 				allSize;
	char const*  		fSPropParameterSetsStr[3];
	uint8_t 			inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
	char 				buf[1024];
private://ffmpeg
	API::H264DecodeDev	*m_pDecode;
};
class API_RTSPSession 
{
	public:
 			API_RTSPSession();
 	virtual ~API_RTSPSession();
 	public:
 		int 		Initialize(std::string& rtspUrl,int& nIndex);
 		int  		startRTSPClient(LPGetFramePtr pCallBack,const char* rtspUrl);
 		int 		Initlive555();
 		void 		live555_loop();	
 		int 		stopRTSP();
 		int 		openURL(UsageEnvironment& env, char const* progName, char const * rtspURL);
 		void 		SetAdminPassWd(char const* rtspUrl,std::string& adminStr,std::string& Passwd);
 		//virtual void 		taskInterrupKeepAlive(void *clientData,UsageEnvironment& env);
 	public:
		static  void*       StreamTimerThread(void* arg);
		static  void        Play_video(void* clientData);
		static  void*       live555_Thread(void* arg);
		static  void 		livenessTimeoutTask(void *clientData);
	 	//static  void 		SubStreamThread(void* arg);
	 	//static  void        Set_live555task(int Time,void* Task,void* arg);
	public:
		pthread_t 					nSessionPid;
		int 						nSessionId;
 		std::string 				m_rtspUrl;
 		ourRTSPClient*				m_rtspClient;
		volatile char     			eventLoopWatchVariable;
		TaskScheduler*        		m_scheduler;
		UsageEnvironment*           m_env;
 	int startRTSPClient(char const* progName, char const* rtspURL, int debugLevel);
 	int stopRTSPClient();
 	int openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel);

	 	pthread_t 				tid;
	 	bool 					m_running;
	 	std::string 			m_progName;
	 	int 					m_debugLevel;
	 	int 					m_nStatus;
	 	int 					m_nID;
	 	static void* 			rtsp_thread_fun(void *param);


	 	void  					unchar2IplImg(u_int8_t* pImage,int& frame,int nW,int nH);
	 	void 					rtsp_fun();
	 	void 					IplImg2unchar(const char* filePath, unsigned char *pImg, int nW, int nH);
	 	static void       		CallBackdata(u_int8_t* pImage ,int& frame,const char* CameraIp);
 public:
		API_Socket 					m_socketClient;
		API_Mutex  			 		m_DataMutex;
};

#endif
