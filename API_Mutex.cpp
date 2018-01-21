#include "API_Define.hpp"
#include "API_Mutex.hpp"
#include <pthread.h>

//构造函数
API::API_Mutex::API_Mutex()
{
    pthread_mutexattr_t	MutexAttr;
    memset(&MutexAttr,0,sizeof(pthread_mutexattr_t));
    pthread_mutexattr_init(&MutexAttr);
    pthread_mutexattr_settype(&MutexAttr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&m_Mutex, &MutexAttr);
}

//虚析构函数
API::API_Mutex::~API_Mutex()
{
	pthread_mutex_destroy(&m_Mutex);
}

//加锁
void API::API_Mutex::Lock()
{
	pthread_mutex_lock(&m_Mutex);
}

//解锁
void API::API_Mutex::UnLock()
{
	pthread_mutex_unlock(&m_Mutex);
}
