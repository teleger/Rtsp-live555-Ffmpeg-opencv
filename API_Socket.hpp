#ifndef			_API_SOCKET_HPP_
#define			_API_SOCKET_HPP_

#include "API_Define.hpp"

#define		NEURALNETWORK_NULL	0
#define		NEURALNETWORK_BEGIN	1
#define		NEURALNETWORK_END	2

//API命名空间
namespace API{


//套接字类
class API_Socket
{
	public:
		API_Socket();				//构造函数
		virtual ~API_Socket();			//虚析构函数

	private:	//内部成员变量
		int		m_nSocket;		//主套接字
		LPSOCKETCALLBACKEX	m_pCallBack;	//回调函数
		void*		m_zParam;		//回调函数参数

	private:	//内部成员函数
		virtual void Listening();		//监听中
		virtual bool SendRecv(int nSocket, void* pBuffer, int nLength, bool bSend);	//发送或接收

	public:		//静态成员函数
		static void* Thread_Socket(void* zParam);			//线程函数
		static void* Thread_SubSocket(void* zParam);			//子线程函数

	public:		//导出成员函数
		virtual bool CreateServer(LPSOCKETCALLBACKEX pCallBack, int nPort, void* zParam, bool bWait = false);		//创建Socket服务端
		virtual bool CreateClient(LPSOCKETCALLBACKEX pCallBack, const char* strServer, int nPort, void* zParam);		//创建Socket客户端
		virtual bool Recv(int nSocket, void* pBuffer, int nLength);	//接收
		virtual bool Send(int nSocket, void* pBuffer, int nLength);	//发送
		virtual void CloseSocket();					//关闭套接字
};
};
#endif//_API_SOCKET_HPP_
