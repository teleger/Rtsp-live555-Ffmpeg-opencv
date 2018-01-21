#ifndef		_API_MUTEX_HPP_
#define		_API_MUTEX_HPP_
#include    <pthread.h>
//互斥量
namespace API{
class API_Mutex
{
public:
		API_Mutex();				//构造函数
		virtual ~API_Mutex();			//虚析构函数
	private:
		pthread_mutex_t		m_Mutex;	//互斥量
	public:
		void Lock();				//加锁
        void UnLock();				//解锁
};
};
#endif//_API_MUTEX_HPP_
