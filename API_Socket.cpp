#include "API_Define.hpp"
#include "API_Socket.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

typedef struct _SP_API_SOCKET_DATA
{
    API::API_Socket*	pApiSocket;
	struct sockaddr_in	addrClient;
	int			nSocket;
}SP_API_SOCKET_DATA, *PSP_API_SOCKET_DATA;

//构造函数
API::API_Socket::API_Socket()
{
	m_pCallBack = nullptr;
	m_zParam = nullptr;
	m_nSocket = 0;
}

//虚析构函数
API::API_Socket::~API_Socket()
{
}

//监听中
void API::API_Socket::Listening()
{
	while(m_nSocket)
	{
		PSP_API_SOCKET_DATA	pData = new SP_API_SOCKET_DATA;
		pData->pApiSocket = this;
		socklen_t		nAddrLength = sizeof(struct sockaddr);
		//LLPRINT("正在监听...\n");
		pData->nSocket = accept(m_nSocket, (struct sockaddr*)&(pData->addrClient), &nAddrLength);
		if(pData->nSocket > 0)
		{
			pthread_t	nPid;
			if(pthread_create(&nPid, nullptr, Thread_SubSocket, (void*)pData))
			{
				delete pData;
				LLERROR("创建套接字子线程失败");
			}
		}
	};
}

//发送或接收
bool API::API_Socket::SendRecv(int nSocket, void* pBuffer, int nLength, bool bSend)
{
	int	nCurrLength = nLength;
	int	nAction = 0;
	char*	pStr = (char*)pBuffer;
	//循环接收数据
	do{
		if(bSend)
		{
			nAction = send(nSocket, (char*)(pStr + nLength - nCurrLength), nCurrLength, 0);
		}
		else
		{
			nAction = recv(nSocket, (char*)(pStr + nLength - nCurrLength), nCurrLength, 0);
		}
		if(nAction == -1)
		{
			return false;
		}
		nCurrLength -= nAction;
	} while(nAction > 0 && nCurrLength);
	if(nCurrLength)
	{
		return false;
	}
	else
	{
		return true;
	}
}

//线程函数
void* API::API_Socket::Thread_Socket(void* zParam)
{
	API_Socket*	pThis = (API_Socket*)zParam;
	pThis->Listening();
	return nullptr;
}

//子线程函数
void* API::API_Socket::Thread_SubSocket(void* zParam)
{
	//获取数据并释放内存
	PSP_API_SOCKET_DATA	pData = (PSP_API_SOCKET_DATA)zParam;
	API_Socket*		pThis = pData->pApiSocket;
	int			nSocket = pData->nSocket;
	delete pData;
	//调用回调函数
	if(pThis->m_pCallBack)
	{
		pThis->m_pCallBack(pThis, nSocket, pThis->m_zParam);
	}
	return nullptr;
}

//创建Socket服务端
bool API::API_Socket::CreateServer(LPSOCKETCALLBACKEX pCallBack, int nPort, void* zParam, bool bWait)
{
	//保存参数
	m_pCallBack = pCallBack;
	m_zParam = zParam;

	//准备创建套接字
	m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in      addrServer;
        addrServer.sin_addr.s_addr = htonl(INADDR_ANY);		//目标IP
        addrServer.sin_family = AF_INET;
        addrServer.sin_port = htons((unsigned short)nPort);	//连接端口
        if(m_nSocket > 0)
        {
			//LLDEBUG("创建套接字成功");
		int	nEnable = 1;
		setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nEnable, sizeof(int));
		if(bind(m_nSocket, (struct sockaddr*)&addrServer, sizeof(struct sockaddr)) == 0)
		{
			//LLDEBUG("绑定套接字成功");
			if(listen(m_nSocket, 10) == 0)
			{
				//判断是否等待
				if(bWait)
				{
					//监听中
					Listening();
					return true;
				}
				else
				{
					pthread_t	nPid;
					if(!pthread_create(&nPid, nullptr, Thread_Socket, (void*)this))
					{
						return true;
					}
				}
			}
		}
		else
		{
			LLERROR("绑定套接字失败 error=%d", errno);
		}
        }
	else
	{
		LLERROR("创建套接字失败");
	}
	return false;
}

//创建Socket客户端
bool API::API_Socket::CreateClient(LPSOCKETCALLBACKEX pCallBack, const char* strServer, int nPort, void* zParam)
{
	//保存参数
	m_pCallBack = pCallBack;
	m_zParam = zParam;
	PSP_API_SOCKET_DATA	pData = new SP_API_SOCKET_DATA;

	m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        //struct sockaddr_in      addrServer;
        pData->addrClient.sin_addr.s_addr = inet_addr(strServer);	//目标IP
        pData->addrClient.sin_family = AF_INET;
        pData->addrClient.sin_port = htons((unsigned short)nPort);	//连接端口
        if(m_nSocket > 0)
        {
		pData->pApiSocket = this;
		pData->nSocket = m_nSocket;
		//开始连接
                int     nConnect = connect(m_nSocket, (struct sockaddr*)&(pData->addrClient), sizeof(struct sockaddr));
                if(!nConnect)
                {
			pthread_t	nPid;
			if(pthread_create(&nPid, nullptr, Thread_SubSocket, (void*)pData) == 0)
			{
				//LLDEBUG("创建套接字线程成功");
				return true;
			}
			else
			{
				LLERROR("创建套接字线程失败");
			}
                }
        }
	delete pData;
        return false;
}

//接收
bool API::API_Socket::Recv(int nSocket, void* pBuffer, int nLength)
{
	return SendRecv(nSocket, pBuffer, nLength, false);
}

//发送
bool API::API_Socket::Send(int nSocket, void* pBuffer, int nLength)
{
	return SendRecv(nSocket, pBuffer, nLength, true);
}

//关闭套接字
void API::API_Socket::CloseSocket()
{
	close(m_nSocket);
}
