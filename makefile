reset

export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:./live/BasicUsageEnvironment/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:./live/groupsock/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:./live/liveMedia/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:./live/UsageEnvironment/include

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib

LiveADLL=live/BasicUsageEnvironment/libBasicUsageEnvironment.so
LiveBDLL=live/groupsock/libgroupsock.so
LiveCDLL=live/liveMedia/libliveMedia.so
LiveDDLL=live/UsageEnvironment/libUsageEnvironment.so

FFMPEG_DLL=Ffmpeg/libavcodec.so.57

#TARGET:xx
CXXLive555Flags=-lBasicUsageEnvironment -lgroupsock -lliveMedia -lUsageEnvironment
CXXFlags=-lpthread -std=c++11
CXXFfmpeg=lavcodec -lavformat -lswscale -lavutil -lswresample

#g++ main.cpp API_RTSPClient.cpp API_Socket.cpp API_Mutex.cpp API_H264Decode.cpp ${LiveADLL} ${LiveBDLL} ${LiveCDLL} ${LiveDDLL} ${FFMPEG_DLL} $(pkg-config --cflags --libs opencv) \
 -lBasicUsageEnvironment -lgroupsock -lliveMedia -lUsageEnvironment  -lpthread -std=c++11 \
 -lavcodec -lavformat -lswscale -lavutil -lswresample \
 -o Demo.run
TARGET=Demo.run

${TARGET}: main.o API_RTSPClient.o API_Socket.o API_Mutex.o API_H264Decode.o
	g++ main.cpp API_RTSPClient.cpp API_Socket.cpp API_Mutex.cpp API_H264Decode.cpp ${LiveADLL} ${LiveBDLL} ${LiveCDLL} ${LiveDDLL} ${FFMPEG_DLL} $(pkg-config --cflags --libs opencv) /
 -lBasicUsageEnvironment -lgroupsock -lliveMedia -lUsageEnvironment  -lpthread -std=c++11 /
 -lavcodec -lavformat -lswscale -lavutil -lswresample /
 -o Demo.run	

.PHONY:clean
clean:
	rm edit $(object)