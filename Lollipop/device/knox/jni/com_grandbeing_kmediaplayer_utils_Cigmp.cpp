
#define LOG_TAG "Cigmp-JNI"

#include <stdio.h>
#include <jni.h>
#include <stdlib.h>
#include <assert.h>
#include "android/log.h"
#include "igmp.h"
#include "GbIgmp.h"

#define USE_CIGMP 1

#define GLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define GLOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ----------------------------------------------------------------------------

using namespace android;
// using android::GbIgmp;

// ----------------------------------------------------------------------------

struct fields_t {
    jfieldID context;
};

static fields_t gFields;

//-----------------------------------------------------------------------------

static void setCigmp(JNIEnv *env, jobject thiz, const CIGMP *p_cigmp)
{
    // GLOGE("setCigmp In");
    CIGMP *old = (CIGMP *)env->GetLongField(thiz, gFields.context);
    if (old != NULL) {
        // GLOGD("delete old cigmp");
        delete old;
        old = NULL;
    }
    env->SetLongField(thiz, gFields.context, (jlong)p_cigmp);
    // GLOGE("setCigmp Out");
}

static CIGMP *getCigmp(JNIEnv *env, jobject thiz)
{
    // GLOGE("getCigmp In");
    return (CIGMP *)env->GetLongField(thiz, gFields.context);
}

static void setGbIgmp(JNIEnv *env, jobject thiz, const sp<GbIgmp>& p_cigmp)
{
    // GLOGE("setGbIgmp In");
    GbIgmp *old = (GbIgmp *)env->GetLongField(thiz, gFields.context);
    if (p_cigmp.get()) {
        p_cigmp->incStrong((void*)setGbIgmp);
    }
    if (old != 0) {
        old->decStrong((void*)setGbIgmp);
    }
    env->SetLongField(thiz, gFields.context, (jlong)p_cigmp.get());
    // GLOGE("setGbIgmp Out");
}

static sp<GbIgmp> getGbIgmp(JNIEnv *env, jobject thiz)
{
    // GLOGE("getGbIgmp In");
    GbIgmp* const p_cigmp = (GbIgmp*)env->GetLongField(thiz, gFields.context);
    return sp<GbIgmp>(p_cigmp);
}

//-----------------------------------------------------------------------------

//------jni method----------------------------------------------------------------------------------

/*
 * Class:     com_grandbeing_kmediaplayer_utils_Cigmp
 * Method:    native_init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_init(JNIEnv *env, jclass clazz)
{
    // GLOGE("native init");
    gFields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (gFields.context == NULL) {
        // TODO throw exception
        return;
    }
}

/*
 * Class:     com_grandbeing_kmediaplayer_utils_Cigmp
 * Method:    native_setup
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_setup(JNIEnv *env, jobject obj)
{
    // GLOGE("native_setup");

    // CIGMP begin
    // CIGMP *p_cigmp = new CIGMP();
    // setCigmp(env, obj, p_cigmp);
    // CIGMP end

    /**
     * 申请native边的资源
     */
    sp<GbIgmp> p_gbIgmp = new GbIgmp();
    setGbIgmp(env, obj, p_gbIgmp);
}

/*
 * Class:     com_grandbeing_kmediaplayer_utils_Cigmp
 * Method:    native_open
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_open(JNIEnv *env, jobject obj, jstring localAddr, jstring multicastAddr)
{
    // GLOGE("native_open");

    // CIGMP begin
    // CIGMP *p_cigmp = getCigmp(env, obj);
    // if (p_cigmp != NULL)
    // {
    //     const char *la = env->GetStringUTFChars(localAddr, NULL);
    //     const char *ma = env->GetStringUTFChars(multicastAddr, NULL);
    //     // GLOGD("open [la, ma]=[%s, %s]", la, ma);
    //     p_cigmp->Open(la, ma);
    // }
    // CIGMP end
    
    sp<GbIgmp> p_gbIgmp = getGbIgmp(env, obj);
    if (p_gbIgmp != NULL)
    {
        const char *la = env->GetStringUTFChars(localAddr, NULL);
        const char *ma = env->GetStringUTFChars(multicastAddr, NULL);
        // GLOGD("open [la, ma]=[%s, %s]", la, ma);
        p_gbIgmp->Open(la, ma);
    }
}

/*
 * Class:     com_grandbeing_kmediaplayer_utils_Cigmp
 * Method:    native_memberShip
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_memberShip(JNIEnv *env, jobject obj)
{
    // GLOGE("native_memberShip");

    // CIGMP begin
    // CIGMP *p_cigmp = getCigmp(env, obj);
    // if (p_cigmp != NULL)
    // {
    //     // GLOGD("Cigmp_native_memberShip");
    //     p_cigmp->MemberShip();
    // }
    // CIGMP end

    sp<GbIgmp> p_gbIgmp = getGbIgmp(env, obj);
    if (p_gbIgmp != NULL)
    {
        // GLOGD("Cigmp_native_memberShip");
        p_gbIgmp->MemberShip();
    }
}

/*
 * Class:     com_grandbeing_kmediaplayer_utils_Cigmp
 * Method:    native_close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_close(JNIEnv *env, jobject obj)
{
    // GLOGE("native_close");

    // CIGMP begin
    // CIGMP *p_cigmp = getCigmp(env, obj);
    // if (p_cigmp != NULL)
    // {
    //     p_cigmp->Close();
    // }
    // CIGMP end

    sp<GbIgmp> p_gbIgmp = getGbIgmp(env, obj);
    if (p_gbIgmp != NULL)
    {
        // GLOGE("p_gbIgmp->Close() begin");
        p_gbIgmp->Close();
        // GLOGE("p_gbIgmp->Close() end");
        /**
         * 释放native边的资源
         */
        // GLOGE("delete p_gbIgmp begin");
        setGbIgmp(env, obj, 0);
        // GLOGE("delete p_gbIgmp end");
    }
}

//------dynamic register----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
        {"native_init",          "()V",                                        (void *)Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_init},
        {"native_setup",         "()V",                                        (void *)Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_setup},
        {"native_open",          "(Ljava/lang/String;Ljava/lang/String;)V",    (void *)Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_open},
        {"native_memberShip",    "()V",                                        (void *)Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_memberShip},
        {"native_close",         "()V",                                        (void *)Java_com_grandbeing_kmediaplayer_utils_Cigmp_native_close},
};

int register_com_grandbeing_kmediaplayer_utils_Cigmp(JNIEnv *env) {
    jclass clazz = env->FindClass("com/grandbeing/kmediaplayer/utils/Cigmp");
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

    if (register_com_grandbeing_kmediaplayer_utils_Cigmp(env) < 0) {
        GLOGE("ERROR: Cigmp native registration failed");
        return -1;
    }
    // GLOGE("JNI_OnLoad Out");
    return JNI_VERSION_1_4;
}