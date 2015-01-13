#include "androidclient.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include "../client_sdk/err_no.h"
#include "../client_sdk/client_log.h"
#include "common/ef_base64.h"
#include "json/reader.h"
#include <android/log.h>
#include "../client_sdk/client_log.h"

namespace gim
{
	int logprint(LogLevel lvl, const char* logbuf)
	{
		if (logbuf)
		{
			int l = (lvl == LOG_LEVEL_ERROR) ? ANDROID_LOG_ERROR : ANDROID_LOG_DEBUG;
			__android_log_print(l, "clientsdk", "%s", logbuf);
		}
		return 0;
	}
	int AndroidClient::sendMessageWithJson(const char* json)
	{
		if (!json)
		{
			return MY_JSON_ERROR;
		}
		Json::Reader reader;
		Json::Value root;

		if (!reader.parse(json, root))
		{
			SDK_LOG(LOG_LEVEL_ERROR, "json error !");
			return MY_JSON_ERROR;
		}

		GPeerMessage msg;
		std::string sn = root[JKEY_MSG_SN].asString();
		msg.to = root[JKEY_MSG_TO].asString();
		msg.data = root[JKEY_MSG_DATA].asString();
		std::string cid = root[JKEY_CID].asString();
		msg.from = cid;
		int ret = sendPeerMessage(cid, sn, msg);

		return ret;
	}
	int AndroidClient::handleMessage(const std::string& json)
	{
		SDK_LOG(LOG_LEVEL_TRACE, "handleMessage :%s", json.c_str());

		//std::string msg = ef::base64_encode(json);
		std::string msg = json;
		jstring jstr = jniInvokeEnv()->NewStringUTF(msg.c_str());
		jniInvokeEnv()->CallVoidMethod(m_jobLstn, m_hmidHandleMessage, jstr);
		if (jniInvokeEnv()->ExceptionCheck())
		{
			SDK_LOG(LOG_LEVEL_TRACE, "ExceptionCheck");
			jniInvokeEnv()->ExceptionDescribe();
			jniInvokeEnv()->ExceptionClear();
		}
		jniInvokeEnv()->DeleteLocalRef(jstr);
		detachJavaThrd();
		return 0;
	}

	int AndroidClient::initJniEnv(JNIEnv* env, jobject objLstn)
	{
		m_env = env;
		m_pGarbage = new JObjGarbage(m_env, false);

		m_jobLstn = env->NewGlobalRef(objLstn);
		m_pGarbage->push(m_jobLstn);

		m_env->GetJavaVM(&m_jvm);

		m_jcLstn = m_env->FindClass(JNI_LISTENER_PATH);
		if (NULL == m_jcLstn)
		{
			SDK_LOG(LOG_LEVEL_ERROR, "handlePushMessageReq find class RTClistener failed!");
			return MY_JNI_ERROR;
		}
		m_pGarbage->push(m_jcLstn);

		m_hmidHandleMessage = m_env->GetMethodID(m_jcLstn, "handleMessage", "(Ljava/lang/String;)V");
		if (NULL == m_hmidHandleMessage)
		{
			SDK_LOG(LOG_LEVEL_ERROR, "get jni method handleMessage fail!");
			return MY_JNI_ERROR;
		}

		return 0;
	}
};

