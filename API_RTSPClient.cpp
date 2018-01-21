#include "API_RTSPClient.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#define REQUEST_STREAMING_OVER_TCP False
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE (640*480*3)
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

//IplImage* image; //local 

static unsigned rtspClientCount = 0;
static LPGetFramePtr   n_pRtspCall = nullptr;
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);


void subsessionAfterPlaying(void* clientData); 
void subsessionByeHandler(void* clientData); 
void streamTimerHandler(void* clientData);
  

// The main streaming routine (for each "rtsp://" URL):
//void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);


// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);


// A function that outputs a string that identifies each stream (for debugging output). Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient)
{
 return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}
// A function that outputs a string that identifies each subsession (for debugging output). Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
 return env << subsession.mediumName() << "/" << subsession.codecName();
}
void usage(UsageEnvironment& env, char const* progName)
{
	 //env << "Usage: " << progName << "  ... \n";
	 //env << "\t(where each  is a \"rtsp://\" URL)\n";
}



void setupNextSubsession(RTSPClient* rtspClient) 
{
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL)
    {
      if (!scs.subsession->initiate())
     {
        env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
        setupNextSubsession(rtspClient);
     } else {
      //env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) {
	     env << "client port " << scs.subsession->clientPortNum();
      } else {
	     //env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
      }
      //env << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}



void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
{
  do
   {
      UsageEnvironment& env = rtspClient->envir(); // alias
      StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    //env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    
    if (scs.session == NULL) 
    {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } 
    else if (!scs.session->hasSubsessions())
    {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  }while (0);

      // An unrecoverable error occurred with this stream.
      shutdownStream(rtspClient);
}




void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    //env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      //env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    //env << ")\n";


    //----------------add ......H264

    //..........

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

if (0 == strncmp(scs.subsession->mediumName(),"video",5))
    {
       do
       {
          if (0 ==  strcmp(scs.subsession->codecName(),"H264"))
          {
              scs.subsession->sink = H264MediaSink::createNew(env,*scs.subsession,scs.subsession->fmtp_spropparametersets(),rtspClient->url());
          }
       }
       while(0);
    }

    if (scs.subsession->sink == NULL)
    {
        //env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
       //<< "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }
    //env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    //-------------------------------------
    //-----------------------
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}


void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs;

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    //env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "PLAY...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}



void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}



void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  //if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
   // exit(exitCode);
  //}
}


// Implementation of "ourRTSPClient":
ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}
// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


H264MediaSink* H264MediaSink::createNew(UsageEnvironment& env, MediaSubsession& subsession,char const* sPropParameterSetsStr, char const* streamId) {
  return new H264MediaSink(env, subsession,sPropParameterSetsStr, streamId);
}

H264MediaSink::H264MediaSink(UsageEnvironment& env, MediaSubsession& subsession,char const* sPropParameterSetsStr, char const* streamId)
:MediaSink(env), 
  fSubsession(subsession),
  bFirstFrame(False),
  extraDataSize(0),
  allSize(0),
  m_pDecode(nullptr)
{
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  fSPropParameterSetsStr[0] = sPropParameterSetsStr;

  //memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);
  fReceiveBufferAV  = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE + 100];

  m_pDecode = new API::H264DecodeDev;

  if(m_pDecode != nullptr)
  {
      m_pDecode->InitializeDev(n_pRtspCall,IMAGE_WIDTH,IMAGE_HEIGHT);
  }

}
H264MediaSink::~H264MediaSink(){
  delete[] fReceiveBuffer;
  delete[] fReceiveBufferAV;
  delete[] fStreamId;
  if (m_pDecode != nullptr){
    delete   m_pDecode;
    m_pDecode = nullptr;  
  }
}
int  H264MediaSink::SetParameterStr(u_int8_t* fReceiveBuffer,unsigned& frameSize)
{
    extraDataSize = 0 ;
    unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};
    if (nullptr == fReceiveBuffer){
       LLERROR("fReceiveBuffer  is nullptr");
       return -1;
    }
    if (nullptr == fSPropParameterSetsStr[0]){
        LLERROR("SPropParameter  is nullptr");
       return -2;
    }
    
    if (!bFirstFrame)
    {
      unsigned numSPropRecords;
      SPropRecord* sPropRecords = parseSPropParameterSets(fSPropParameterSetsStr[0], numSPropRecords);  
      for (unsigned i = 0; i < numSPropRecords; i++)
      {
          if (sPropRecords[i].sPropLength == 0) continue;
          u_int8_t nal_unit_type = (sPropRecords[i].sPropBytes[0])&0x1F;
          if (nal_unit_type == 7)//SPS
          {
              memcpy(fReceiveBufferAV + extraDataSize,start_code,4);
              extraDataSize += 4;
              memcpy(fReceiveBufferAV + extraDataSize,sPropRecords[i].sPropBytes,sPropRecords[i].sPropLength);
              extraDataSize += sPropRecords[i].sPropLength;
              //LLDEBUG("Set  ParameterStr ...  sps...   ");
          } 
          else if (nal_unit_type == 8)//PPS
          {
              memcpy(fReceiveBufferAV + extraDataSize,start_code,4);
              extraDataSize += 4;
              memcpy(fReceiveBufferAV + extraDataSize,sPropRecords[i].sPropBytes,sPropRecords[i].sPropLength);
              extraDataSize += sPropRecords[i].sPropLength;
              //LLDEBUG("Set ParameterStr ...  pps...    ");
          }

      }

      bFirstFrame = True;
      delete[] sPropRecords;
    }
    memcpy(fReceiveBufferAV + extraDataSize,start_code,4);
    memcpy(fReceiveBufferAV + extraDataSize + 4,fReceiveBuffer,frameSize);

    if(bFirstFrame && extraDataSize)
    {
       return 4 + extraDataSize;
    }
    extraDataSize = 0; 

    return 4;//4
}
void H264MediaSink::afterGettingFrame(void* clientData, unsigned frameSize, 
  unsigned numTruncatedBytes,
  struct timeval presentationTime,
  unsigned durationInMicroseconds)
{
  H264MediaSink* sink = (H264MediaSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void H264MediaSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
          struct timeval presentationTime) {
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
  if (0 ==  strcmp(fSubsession.codecName(),"H264"))
  {
      int     SetSize = SetParameterStr(fReceiveBuffer,frameSize);
      m_pDecode->ProcessDecode(fReceiveBufferAV,SetSize + frameSize,fStreamId);
  }
  //Then continue, to request the next frame of data:
  continuePlaying();
}
Boolean H264MediaSink::continuePlaying()
{
  if (fSource == NULL) return False; // sanity check (should not happen)
  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}



static API_RTSPSession* g_This = nullptr;
API_RTSPSession::API_RTSPSession():m_rtspUrl(""),
m_rtspClient(nullptr),
eventLoopWatchVariable(0),
m_scheduler(nullptr),
m_env(nullptr)
{
    g_This= this;
}
API_RTSPSession::~API_RTSPSession()
{
}
int API_RTSPSession::startRTSPClient(char const* progName, char const* rtspURL, int debugLevel)
{
/*	 std::cout << "Begin RTSP client" << "\n";
	 m_progName              = progName;
	 m_rtspUrl               = rtspURL;
	 m_debugLevel            = debugLevel;
	 eventLoopWatchVariable  = 0;
	 rtsp_fun();*/
	 return 0;
}
int API_RTSPSession::stopRTSPClient()
{
	 eventLoopWatchVariable = 1;
	 return 0;
}
void API_RTSPSession::rtsp_fun()
{
	   //::startRTSP(m_progName.c_str(), m_rtspUrl.c_str(), m_ndebugLever);
/*  	 TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  	 UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
  	 if (openURL(*env, m_progName.c_str(), m_rtspUrl.c_str(), m_debugLevel) == 0)
  	 {
    	 	m_nStatus = 1;
    	 	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    	 
    	 	m_running = false;
    	 	eventLoopWatchVariable = 0;
    	 
    	 	if (m_rtspClient)
    	 	{
    	 		shutdownStream(m_rtspClient,0);
    	 	}
    	 	m_rtspClient = NULL;
  	 }
  	 env->reclaim(); 
  	 env = NULL;
  	 delete scheduler; 
  	 scheduler = NULL;
  	 m_nStatus = 2;*/
}
int API_RTSPSession::openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel)
{
	   m_rtspClient = ourRTSPClient::createNew(env, rtspURL, debugLevel, progName);

  	 if (m_rtspClient == NULL) 
  	 {
      	env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
      	return -1;
  	 }
  	 //++rtspClientCount;
  	 //((RTSPClient*)m_rtspClient)->m_nID = 5;//设置ID   @leger  
  	 m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE); //begin 建立rtsp会话
  	 return 0;
}

int API_RTSPSession::Initialize(std::string& rtspUrl,int& nIndex)
{
    if (rtspUrl != "")
    {
       m_rtspUrl = rtspUrl;
    }
    else
    {
      return -1;
    }
    if (nIndex < 0)
    {
      return -2;
    }
    nSessionId = nIndex;
    //LLDEBUG("RTSPSession Init  end ..");
    return 0;
}
void* API_RTSPSession::StreamTimerThread(void* arg)
{
   API_RTSPSession* pThis = (API_RTSPSession*)arg;
   while(true)
   {
      sleep(3);//每3秒更新
   }
   return nullptr;
}
int  API_RTSPSession::startRTSPClient(LPGetFramePtr pCallBack,const char* rtspUrl)
{
    cv::namedWindow("VideoVerify");

    std::string   m_url = rtspUrl;
    int  a = 0;
    Initialize(m_url,a);

    if(0 != Initlive555())
    {
        LLERROR("Initlive555 failed! Please Try again");
        return -1;
    }
    if(rtspClientCount < 1)
    {
        n_pRtspCall = pCallBack;
    }
    eventLoopWatchVariable = 0;
    //pthread_attr_t attr;
    //pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);//PTHREAD_CREATE_JOINABLE  PTHREAD_CREATE_DETACHED
    pthread_create(&nSessionPid,NULL,live555_Thread,this);
    void*  run_status;
    pthread_join(nSessionPid,&run_status);
    return 0;
}
int  API_RTSPSession::stopRTSP()
{
    //LLDEBUG("Connect closed : %d",nSessionId);
    eventLoopWatchVariable = 1;
/*    if (m_rtspClient != nullptr)
    {
      shutdownStream(m_rtspClient);
    }*/
    if (m_env != nullptr)
    {
      Boolean bEnv = m_env->reclaim();
      /*只能调用reclaim  (回收)函数，不能直接删除，(析构函数为virtual的)
      返回值为true时，说明已经delete对象.
      m_env->liveMediaPriv = nullptr;
      m_env->groupsockPriv = nullptr;
      */
      if (bEnv == True)
      {
         //LLDEBUG("... stop ..");
      }
      m_env = nullptr;
    }
    if (m_scheduler != nullptr)
    {
      delete  m_scheduler; 
      m_scheduler = nullptr;
    }
    return 0;
}
int API_RTSPSession::Initlive555(){
    m_scheduler  = BasicTaskScheduler::createNew();
    if (nullptr == m_scheduler){
       LLERROR("BasicTaskScheduler create failed");
       return -1;
    }
    m_env        = BasicUsageEnvironment::createNew(*m_scheduler);
    if (nullptr == m_env){
      LLERROR("BasicUsageEnvironment create failed");
      return -2;
    }
    m_scheduler->scheduleDelayedTask(3000, Play_video, (void*)this);
    //LLDEBUG("live555 pthread_exit");
    //pthread_exit(nullptr);
    return 0;
}
void* API_RTSPSession::live555_Thread(void* arg)
{
    API_RTSPSession*   pThis = (API_RTSPSession*)arg;
    //TaskScheduler&  Schedu   = pThis->m_env->taskScheduler();
    //Schedu.doEventLoop(&(pThis->eventLoopWatchVariable));
    pThis->live555_loop();
    //pthread_exit(nullptr); //2018-01-04
}
void API_RTSPSession::live555_loop()
{
    //LLDEBUG("live555_loop");
    m_env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    return ;
}
//static funtion -----------------------------
void API_RTSPSession::Play_video(void* clientData)
{
    API_RTSPSession*  pThis = (API_RTSPSession*)clientData;
    pThis->openURL(*(pThis->m_env),"TestRtsp", pThis->m_rtspUrl.c_str());
    return ;
}
//---------------------------------------------
int API_RTSPSession::openURL(UsageEnvironment& env, char const* progName, char  const* rtspURL)
{
    //LLDEBUG("openURL  ");
    m_rtspClient  = ourRTSPClient::createNew(env, rtspURL, 0, progName);
    if(m_rtspClient == nullptr){
        env << "Failed to create a RTSP  client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return -1;
    }
    if(strlen(rtspURL) > 40)
    {
        //LLDEBUG("Authenticator ... ");
        std::string  adminStr;
        std::string  PassStr;
        SetAdminPassWd(rtspURL,adminStr,PassStr);
        Authenticator authenticator(adminStr.c_str(),PassStr.c_str(),True);

        m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE,&authenticator);
        rtspClientCount++;
    }
    else
    {
        m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
        rtspClientCount++;
    } //begin   建立rtsp会话
    return 0;
}//--------------------------

void API_RTSPSession::CallBackdata(u_int8_t* pImage ,int& frame,const char* CameraIp)
{
    API_RTSPSession* pThis = g_This;
    if(pImage != nullptr)
    {
      //LLDEBUG("Image data ");
      pThis->m_DataMutex.Lock();
      pThis->unchar2IplImg(pImage,frame,640,480);
      pThis->m_DataMutex.UnLock();
    }
}
void API_RTSPSession::IplImg2unchar(const char* filePath, unsigned char *pImg, int nW, int nH)//@leger 2017-07-22
{
    if(nullptr == filePath)return;
    IplImage* image = cvLoadImage(filePath);
    if (nullptr == image) return;
    for (int j = 0; j< nH; j++) 
    {
       memcpy(&pImg[j*nW*3],&image->imageData[(nH-j-1)*nW*3],nW*3);
    }
    cvReleaseImage(&image);
    return ;
}
void  API_RTSPSession::unchar2IplImg(u_int8_t* pImage,int& frame,int nW,int nH)
{
    IplImage* image = cvCreateImageHeader(cvSize(nW,nH),IPL_DEPTH_8U,3);
    cvSetData(image,pImage,nW*3);
    //cv::Mat  mat_img = cv::cvarrToMat(image);
    //char  nameimage[64]={0};
    //sprintf(nameimage,"%d.bmp",frame);
    cvShowImage("VideoVerify", image);
    //cvSaveImage(nameimage,image);
    //cv::imshow("VideoVerify", mat_img);
    cv::waitKey(1);

    cvReleaseImageHeader(&image);
    //cv::destroyWindow("VideoVerify");
}

void API_RTSPSession::SetAdminPassWd(char const* rtspUrl,std::string& adminStr,std::string& Passwd)
{
  int  nUrlIdex = 0;
  int  idex = 0;
  char user[64]={0};
  
  //const char* str = "rtsp://admin1234:admin123456@192.168.200.65:554/h264/ch1/sub/av_stream";
  while(rtspUrl != nullptr)
  {
    if (*rtspUrl == '/'){
      nUrlIdex++;
    }
    if (nUrlIdex > 1){
      rtspUrl++;
      if(*rtspUrl != '@')
      {
        user[idex] = *rtspUrl;
        idex++;
      }else{
        break;
      }
    }
    else{
      rtspUrl++;
    } 
  }
  user[idex]='\0';
  //printf(" User and Pass: %s\n",user);

  char admin[64]={0};
  int  aIndex = 0;
  for(int i = 0;i < strlen(user);i++)
  {
    if(user[i] != ':')
    {
      admin[i] = user[i];
      aIndex++;
    }
    else
    {
      break;
    }
  }
  admin[aIndex]='\0';
  //printf(" user:  %s\n",admin);

  adminStr = admin;

  char passWd[64]={0};
  int  nPass = 0;
  int  pIdex = 0;
  for (int i = 0; i < strlen(user); ++i)
  {
    if(nPass)
    {
      passWd[pIdex]= user[i];
      pIdex++;
    }
    if(user[i] == ':')
    {
      nPass = 1;
    }
  }
  passWd[pIdex] = '\0';
  //printf("Passwd: %s\n",passWd);

  Passwd = passWd;
}

//-----------------------------
