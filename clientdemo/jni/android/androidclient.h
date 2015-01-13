#include "../client_sdk/client.h"
#include "../client_sdk/err_no.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <jni.h>

namespace gim
{
	class JObjGarbage
	{
	public:
		JObjGarbage(JNIEnv* env, bool local = true)
			:m_env(env), m_localRef(local)
		{
			
		};
		~JObjGarbage()
		{
			for (std::vector<jobject>::iterator it=m_objs.begin(); it!=m_objs.end(); ++it)
			{
				if (m_localRef)
				{
					m_env->DeleteLocalRef(*it);
				}
				else
				{
					m_env->DeleteGlobalRef(*it);
				}
			}
		}
		void push(jobject obj)
		{
			m_objs.push_back(obj);
		}
	private:
		std::vector<jobject> m_objs;
		bool m_localRef;
		JNIEnv* m_env;
	};



#define JNI_LISTENER_PATH "com/gim/listener"


	class AndroidClient
		:public Client
	{
	public:
		AndroidClient()
			:m_jvm(NULL),
			m_env(NULL),
			m_invokeEnv(NULL),
			m_jcLstn(NULL),
			m_jobLstn(NULL),
			m_pGarbage(NULL)
		{

		}
		~AndroidClient()
		{
			if (m_pGarbage)
			{
				delete m_pGarbage;
			}
		}
		int sendMessageWithJson(const char* json);
		int initJniEnv(JNIEnv* env, jobject objLstn);
		virtual int handleMessage(const std::string& msg);
	private:

		//invoked in client thread
		JNIEnv* jniInvokeEnv()
		{
			if (!m_invokeEnv)
			{
				m_jvm->AttachCurrentThread(&m_invokeEnv, 0);
			}
			return m_invokeEnv;
		}
		void detachJavaThrd()
		{
			if (m_invokeEnv)
			{
				int ret = m_jvm->DetachCurrentThread();
				m_invokeEnv = NULL;
			}
		}
	private:
		JavaVM *m_jvm;
		JNIEnv* m_env;
		JNIEnv* m_invokeEnv;
		jclass m_jcLstn;

		jobject m_jobLstn;

		jmethodID m_hmidHandleMessage;
		JObjGarbage* m_pGarbage;
	};


};

