#include "client_jni.h"
#include "../client_sdk/client_log.h"
#include "../android/androidclient.h"

#ifdef __cplusplus
extern "C" {
#endif
	static gim::AndroidClient s_cli;
	JNIEXPORT jint JNICALL Java_com_gim_client_init(JNIEnv *env, jobject jcli, jobject jlstn)
	{
		if(s_cli.initJniEnv(env, jlstn) < 0)
			return -1;

		return s_cli.init();
	}
	JNIEXPORT jint JNICALL Java_com_gim_client_stop(JNIEnv *, jobject)
	{
		return s_cli.stop();
	}
	JNIEXPORT jint JNICALL Java_com_gim_client_login(JNIEnv* env, jobject job, jstring jsrvip, jint srvport, 
		jstring jcliver, jint enc,jstring jcid, jstring jpwd)
	{
		int ret = -1;
		const char* ip = env->GetStringUTFChars(jsrvip, NULL);
		const char* version = env->GetStringUTFChars(jcliver, NULL);
		const char* cid = env->GetStringUTFChars(jcid, NULL);
		const char* pwd = env->GetStringUTFChars(jpwd, NULL);
		if (ip && version && cid && pwd)
		{
			ret = s_cli.login(ip, srvport, cid, pwd, enc, version);
		}
		env->ReleaseStringUTFChars(jsrvip, ip);
		env->ReleaseStringUTFChars(jcliver, version);
		env->ReleaseStringUTFChars(jcid, cid);
		env->ReleaseStringUTFChars(jpwd, pwd);
		return 0;
	}
	JNIEXPORT jint JNICALL Java_com_gim_client_disconnect(JNIEnv *env, jobject job, jstring jcid)
	{
		int ret = -1;
		const char* cid = env->GetStringUTFChars(jcid, NULL);
		if (cid)
		{
			ret = s_cli.disconnect(cid);
			env->ReleaseStringUTFChars(jcid, cid);
		}
		return 0;
	}
	JNIEXPORT jint JNICALL Java_com_gim_client_sendMessage(JNIEnv *env, jobject, jstring jjson)
	{
		int ret = -1;
		const char* json = env->GetStringUTFChars(jjson, NULL);
		if (json)
		{
			ret = s_cli.sendMessageWithJson(json);
			env->ReleaseStringUTFChars(jjson, json);
		}
		return 0;
	}

#ifdef __cplusplus
}
#endif
