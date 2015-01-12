/*
 * Copyright (c) 2010
 * efgod@126.com.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */


#ifndef EF_THREAD_H
#define EF_THREAD_H



#ifdef _WIN32

#include <windows.h>


#else/*_WIN32*/

#include <pthread.h>
#include <semaphore.h>


#endif

#include "ef_btype.h"

namespace ef{

#ifdef _WIN32
	typedef HANDLE   THREADHANDLE;
	typedef DWORD   PID;
	typedef CRITICAL_SECTION MUTEX;
	typedef HANDLE   SEMAPHORE;
	typedef LPTHREAD_START_ROUTINE PTHREAD_FUNC;


#else
	typedef pthread_t  THREADHANDLE;
	typedef pid_t   PID;
	typedef pthread_mutex_t  MUTEX;
	typedef sem_t   SEMAPHORE;
	typedef void* (*PTHREAD_FUNC)(void*);

#endif

	int32 thread_create(THREADHANDLE* thd, void* attr, PTHREAD_FUNC start_routine, void* arg);
	int32 thread_join (THREADHANDLE* thd);
#ifdef _WIN32
	int32 thread_suspend(THREADHANDLE* thd);
	int32 thread_resume(THREADHANDLE* thd);
#endif
	int32 mutex_init (MUTEX* mtx);
	int32 mutex_take (MUTEX* mtx);
	int32 mutex_give (MUTEX* mtx);
	int32 mutex_destroy(MUTEX* mtx);
	int32 sem_init (SEMAPHORE* sem, int32 initcnt, int32 maxcnt);
	int32 sem_take (SEMAPHORE* sem, bool couldbreak = false);
	int32 sem_try_take (SEMAPHORE* sem);
	int32 sem_give (SEMAPHORE* sem);
	int32 sem_destroy (SEMAPHORE* sem);

	class auto_lock
	{
		public:
			auto_lock(MUTEX* lock):m_lock(lock){
				mutex_take(m_lock);
			}
			~auto_lock( ){
				mutex_give(m_lock);
			}
		protected:
		private:
			MUTEX* m_lock;
	};

	class Condition
	{
	public:
		Condition()
		{
			pthread_cond_init(&m_cond, NULL);
			pthread_mutex_init(&m_mutex, NULL);
		}
		~Condition()
		{
			pthread_mutex_destroy(&m_mutex);
			pthread_cond_destroy(&m_cond);
		}
		void signal()
		{
			pthread_mutex_lock(&m_mutex);
			pthread_cond_signal(&m_cond);
			pthread_mutex_unlock(&m_mutex);
		}
		void wait()
		{
			pthread_mutex_lock(&m_mutex);
			pthread_cond_wait(&m_cond, &m_mutex);
			pthread_mutex_unlock(&m_mutex);
		}
		void init();
	private:
		MUTEX m_mutex;
		pthread_cond_t m_cond;
	};

};

#endif/*BE_THREAD_H*/
