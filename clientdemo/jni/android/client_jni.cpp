#include "client_jni.h"
#include "androidclient.h"

#define C_STR(x) (gim::JstringToString(env, x).c_str())

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
		return s_cli.login(C_STR(jsrvip), srvport, C_STR(jcid), C_STR(jpwd), enc, C_STR(jcliver));
	}
	JNIEXPORT jint JNICALL Java_com_gim_client_disconnect(JNIEnv *env, jobject job, jstring jcid)
	{
		return s_cli.disconnect(C_STR(jcid));
	}
	JNIEXPORT jint JNICALL Java_com_gim_client_sendPeerMessage(JNIEnv *env, jobject, jstring jcid, jstring sn, jstring peercid, jstring data)
	{
		return s_cli.sendPeerMessage(C_STR(jcid), C_STR(sn), C_STR(peercid), C_STR(data));
	}

#ifdef __cplusplus
}
#endif
