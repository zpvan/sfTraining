#define LOG_TAG "GbExec-JNI"

#include <stdio.h>
#include <jni.h>
#include <stdlib.h>
#include <assert.h>
#include "android/log.h"
#include "GbExec.h"

#define GLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define GLOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace android;

//------static ------------------------------------------------------------------------------------


//------jni method----------------------------------------------------------------------------------
/*
 * Class:     com_grandbeing_kmediaplayer_utils_GbExec
 * Method:    native_kill
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_grandbeing_kmediaplayer_utils_GbExec_native_kill(JNIEnv *env, jobject obj, jstring pid)
{
    sp<GbExec> p_gbExec = new GbExec();
    if (p_gbExec != NULL)
    {
        const char *i_pid = env->GetStringUTFChars(pid, NULL);
        // GLOGD("open [la, ma]=[%s, %s]", la, ma);
        p_gbExec->Kill(i_pid);
    }
}


//------dynamic register----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
        {"native_kill",          "(Ljava/lang/String;)V",                      (void *)Java_com_grandbeing_kmediaplayer_utils_GbExec_native_kill},
};

int register_com_grandbeing_kmediaplayer_utils_GbExec(JNIEnv *env) {
    jclass clazz = env->FindClass("com/grandbeing/kmediaplayer/utils/GbExec");
    if (clazz == NULL)
    {
        GLOGE("ERROR: FindClass failed");
        return -1;
    }

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods)/ sizeof(gMethods[0])) != JNI_OK)
    {
        GLOGE("ERROR: RegisterNatives failed");
        return -1;
    }

    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    // GLOGE("JNI_OnLoad In");
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        GLOGE("ERROR: GetEnv failed\n");
        return -1;
    }
    assert(env != NULL);

    if (register_com_grandbeing_kmediaplayer_utils_GbExec(env) < 0) {
        GLOGE("ERROR: GbExec native registration failed");
        return -1;
    }
    // GLOGE("JNI_OnLoad Out");
    return JNI_VERSION_1_4;
}