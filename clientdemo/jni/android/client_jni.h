#include <jni.h>
#ifndef _CLIENT_JNI_H_
#define _CLIENT_JNI_H_
#ifdef __cplusplus
extern "C" {
#endif

	JNIEXPORT jint JNICALL Java_com_gim_client_init
		(JNIEnv *, jobject);

	JNIEXPORT jint JNICALL Java_com_gim_client_stop
		(JNIEnv *, jobject);

	JNIEXPORT jint JNICALL Java_com_gim_client_login
		(JNIEnv* env, jobject job, jstring jsrvip, jint srvport, jstring jcliver, jint enc, jstring jcid, jstring jpwd);

	JNIEXPORT jint JNICALL Java_com_gim_client_disconnect
		(JNIEnv *, jobject, jstring);

	JNIEXPORT jint JNICALL Java_com_gim_client_sendMessage
		(JNIEnv *, jobject, jstring);

#ifdef __cplusplus
}
#endif
#endif
