/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaMetadataRetrieverJNI"

#include <assert.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <SkBitmap.h>
#include <media/IMediaHTTPService.h>
#include <media/mediametadataretriever.h>
#include <media/mediascanner.h>
#include <private/media/VideoFrame.h>

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "android_media_Utils.h"
#include "android_util_Binder.h"

// MStar Android Patch Begin
#include "autodetect.h"
#include "unicode/ucnv.h"
#include "unicode/ucsdet.h"
#include "unicode/ustring.h"
// MStar Android Patch End

using namespace android;

struct fields_t {
    jfieldID context;
    jclass bitmapClazz;  // Must be a global ref
    jfieldID nativeBitmap;
    jmethodID createBitmapMethod;
    jmethodID createScaledBitmapMethod;
    jclass configClazz;  // Must be a global ref
    jmethodID createConfigMethod;
};

static fields_t fields;
static Mutex sLock;
static const char* const kClassPathName = "android/media/MediaMetadataRetriever";

static void process_media_retriever_call(JNIEnv *env, status_t opStatus, const char* exception, const char *message)
{
    if (opStatus == (status_t) INVALID_OPERATION) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    } else if (opStatus != (status_t) OK) {
        if (strlen(message) > 230) {
            // If the message is too long, don't bother displaying the status code.
            jniThrowException( env, exception, message);
        } else {
            char msg[256];
            // Append the status code to the message.
            sprintf(msg, "%s: status = 0x%X", message, opStatus);
            jniThrowException( env, exception, msg);
        }
    }
}

static MediaMetadataRetriever* getRetriever(JNIEnv* env, jobject thiz)
{
    // No lock is needed, since it is called internally by other methods that are protected
    MediaMetadataRetriever* retriever = (MediaMetadataRetriever*) env->GetLongField(thiz, fields.context);
    return retriever;
}

static void setRetriever(JNIEnv* env, jobject thiz, MediaMetadataRetriever* retriever)
{
    // No lock is needed, since it is called internally by other methods that are protected
    MediaMetadataRetriever *old = (MediaMetadataRetriever*) env->GetLongField(thiz, fields.context);
    env->SetLongField(thiz, fields.context, (jlong) retriever);
}

static void
android_media_MediaMetadataRetriever_setDataSourceAndHeaders(
        JNIEnv *env, jobject thiz, jobject httpServiceBinderObj, jstring path,
        jobjectArray keys, jobjectArray values) {

    ALOGV("setDataSource");
    MediaMetadataRetriever* retriever = getRetriever(env, thiz);
    if (retriever == 0) {
        jniThrowException(
                env,
                "java/lang/IllegalStateException", "No retriever available");

        return;
    }

    if (!path) {
        jniThrowException(
                env, "java/lang/IllegalArgumentException", "Null pointer");

        return;
    }

    const char *tmp = env->GetStringUTFChars(path, NULL);
    if (!tmp) {  // OutOfMemoryError exception already thrown
        return;
    }

    String8 pathStr(tmp);
    env->ReleaseStringUTFChars(path, tmp);
    tmp = NULL;

    // Don't let somebody trick us in to reading some random block of memory
    if (strncmp("mem://", pathStr.string(), 6) == 0) {
        jniThrowException(
                env, "java/lang/IllegalArgumentException", "Invalid pathname");
        return;
    }

    // We build a similar KeyedVector out of it.
    KeyedVector<String8, String8> headersVector;
    if (!ConvertKeyValueArraysToKeyedVector(
            env, keys, values, &headersVector)) {
        return;
    }

    sp<IMediaHTTPService> httpService;
    if (httpServiceBinderObj != NULL) {
        sp<IBinder> binder = ibinderForJavaObject(env, httpServiceBinderObj);
        httpService = interface_cast<IMediaHTTPService>(binder);
    }

    process_media_retriever_call(
            env,
            retriever->setDataSource(
                httpService,
                pathStr.string(),
                headersVector.size() > 0 ? &headersVector : NULL),

            "java/lang/RuntimeException",
            "setDataSource failed");
}

static void android_media_MediaMetadataRetriever_setDataSourceFD(JNIEnv *env, jobject thiz, jobject fileDescriptor, jlong offset, jlong length)
{
    ALOGV("setDataSource");
    MediaMetadataRetriever* retriever = getRetriever(env, thiz);
    if (retriever == 0) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return;
    }
    if (!fileDescriptor) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    if (offset < 0 || length < 0 || fd < 0) {
        if (offset < 0) {
            ALOGE("negative offset (%lld)", (long long)offset);
        }
        if (length < 0) {
            ALOGE("negative length (%lld)", (long long)length);
        }
        if (fd < 0) {
            ALOGE("invalid file descriptor");
        }
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
    process_media_retriever_call(env, retriever->setDataSource(fd, offset, length), "java/lang/RuntimeException", "setDataSource failed");
}

template<typename T>
static void rotate0(T* dst, const T* src, size_t width, size_t height)
{
    memcpy(dst, src, width * height * sizeof(T));
}

template<typename T>
static void rotate90(T* dst, const T* src, size_t width, size_t height)
{
    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
            dst[j * height + height - 1 - i] = src[i * width + j];
        }
    }
}

template<typename T>
static void rotate180(T* dst, const T* src, size_t width, size_t height)
{
    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
            dst[(height - 1 - i) * width + width - 1 - j] = src[i * width + j];
        }
    }
}

template<typename T>
static void rotate270(T* dst, const T* src, size_t width, size_t height)
{
    for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
            dst[(width - 1 - j) * height + i] = src[i * width + j];
        }
    }
}

template<typename T>
static void rotate(T *dst, const T *src, size_t width, size_t height, int angle)
{
    switch (angle) {
        case 0:
            rotate0(dst, src, width, height);
            break;
        case 90:
            rotate90(dst, src, width, height);
            break;
        case 180:
            rotate180(dst, src, width, height);
            break;
        case 270:
            rotate270(dst, src, width, height);
            break;
    }
}

static jobject android_media_MediaMetadataRetriever_getFrameAtTime(JNIEnv *env, jobject thiz, jlong timeUs, jint option)
{
    ALOGV("getFrameAtTime: %lld us option: %d", timeUs, option);
    MediaMetadataRetriever* retriever = getRetriever(env, thiz);
    if (retriever == 0) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    // Call native method to retrieve a video frame
    VideoFrame *videoFrame = NULL;
    sp<IMemory> frameMemory = retriever->getFrameAtTime(timeUs, option);
    if (frameMemory != 0) {  // cast the shared structure to a VideoFrame object
        videoFrame = static_cast<VideoFrame *>(frameMemory->pointer());
    }
    if (videoFrame == NULL) {
        ALOGE("getFrameAtTime: videoFrame is a NULL pointer");
        return NULL;
    }

    ALOGV("Dimension = %dx%d and bytes = %d",
            videoFrame->mDisplayWidth,
            videoFrame->mDisplayHeight,
            videoFrame->mSize);

    jobject config = env->CallStaticObjectMethod(
                        fields.configClazz,
                        fields.createConfigMethod,
                        SkBitmap::kRGB_565_Config);

    uint32_t width, height;
    bool swapWidthAndHeight = false;
    if (videoFrame->mRotationAngle == 90 || videoFrame->mRotationAngle == 270) {
        width = videoFrame->mHeight;
        height = videoFrame->mWidth;
        swapWidthAndHeight = true;
    } else {
        width = videoFrame->mWidth;
        height = videoFrame->mHeight;
    }

    jobject jBitmap = env->CallStaticObjectMethod(
                            fields.bitmapClazz,
                            fields.createBitmapMethod,
                            width,
                            height,
                            config);
    if (jBitmap == NULL) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        ALOGE("getFrameAtTime: create Bitmap failed!");
        return NULL;
    }

    SkBitmap *bitmap =
            (SkBitmap *) env->GetLongField(jBitmap, fields.nativeBitmap);

    bitmap->lockPixels();
    rotate((uint16_t*)bitmap->getPixels(),
           (uint16_t*)((char*)videoFrame + sizeof(VideoFrame)),
           videoFrame->mWidth,
           videoFrame->mHeight,
           videoFrame->mRotationAngle);
    bitmap->unlockPixels();

    if (videoFrame->mDisplayWidth  != videoFrame->mWidth ||
        videoFrame->mDisplayHeight != videoFrame->mHeight) {
        uint32_t displayWidth = videoFrame->mDisplayWidth;
        uint32_t displayHeight = videoFrame->mDisplayHeight;
        if (swapWidthAndHeight) {
            displayWidth = videoFrame->mDisplayHeight;
            displayHeight = videoFrame->mDisplayWidth;
        }
        ALOGV("Bitmap dimension is scaled from %dx%d to %dx%d",
                width, height, displayWidth, displayHeight);
        jobject scaledBitmap = env->CallStaticObjectMethod(fields.bitmapClazz,
                                    fields.createScaledBitmapMethod,
                                    jBitmap,
                                    displayWidth,
                                    displayHeight,
                                    true);
        return scaledBitmap;
    }

    return jBitmap;
}

static jbyteArray android_media_MediaMetadataRetriever_getEmbeddedPicture(
        JNIEnv *env, jobject thiz, jint pictureType)
{
    ALOGV("getEmbeddedPicture: %d", pictureType);
    MediaMetadataRetriever* retriever = getRetriever(env, thiz);
    if (retriever == 0) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }
    MediaAlbumArt* mediaAlbumArt = NULL;

    // FIXME:
    // Use pictureType to retrieve the intended embedded picture and also change
    // the method name to getEmbeddedPicture().
    sp<IMemory> albumArtMemory = retriever->extractAlbumArt();
    if (albumArtMemory != 0) {  // cast the shared structure to a MediaAlbumArt object
        mediaAlbumArt = static_cast<MediaAlbumArt *>(albumArtMemory->pointer());
    }
    if (mediaAlbumArt == NULL) {
        ALOGE("getEmbeddedPicture: Call to getEmbeddedPicture failed.");
        return NULL;
    }

    jbyteArray array = env->NewByteArray(mediaAlbumArt->size());
    if (!array) {  // OutOfMemoryError exception has already been thrown.
        ALOGE("getEmbeddedPicture: OutOfMemoryError is thrown.");
    } else {
        const jbyte* data =
                reinterpret_cast<const jbyte*>(mediaAlbumArt->data());
        env->SetByteArrayRegion(array, 0, mediaAlbumArt->size(), data);
    }

    // No need to delete mediaAlbumArt here
    return array;
}

// MStar Android Patch Begin
static uint32_t checkPossibleEncodings(const char* s)
{
    uint32_t result = kEncodingAll;
    // if s contains a native encoding, then it was mistakenly encoded in utf8 as if it were latin-1
    // so we need to reverse the latin-1 -> utf8 conversion to get the native chars back
    uint8_t ch1, ch2;
    uint8_t* chp = (uint8_t *)s;

    while ((ch1 = *chp++)) {
        if (ch1 & 0x80) {
            ch2 = *chp++;
            ch1 = ((ch1 << 6) & 0xC0) | (ch2 & 0x3F);
            // ch1 is now the first byte of the potential native char

            ch2 = *chp++;
            if (ch2 & 0x80)
                ch2 = ((ch2 << 6) & 0xC0) | (*chp++ & 0x3F);
            // ch2 is now the second byte of the potential native char
            int ch = (int)ch1 << 8 | (int)ch2;
            result &= findPossibleEncodings(ch);
        }
        // else ASCII character, which could be anything
    }

    return result;
}

static char* RetrieverConvertValues(const char* enc, char* sourceData)
{
    char* buffer  = NULL;
    if (enc) {
        UErrorCode status = U_ZERO_ERROR;

        UConverter *conv = ucnv_open(enc, &status);
        if (U_FAILURE(status)) {
            ALOGE("could not create UConverter for %s", enc);
            return NULL;
        }
        UConverter *utf8Conv = ucnv_open("UTF-8", &status);
        if (U_FAILURE(status)) {
            ALOGE("could not create UConverter for UTF-8");
            ucnv_close(conv);
            return NULL;
        }

        // for each value string, convert from native encoding to UTF-8
        //for (int i = 0; i < mNames->size(); i++) {
            // first we need to untangle the utf8 and convert it back to the original bytes
            // since we are reducing the length of the string, we can do this in place
            uint8_t* src = (uint8_t *)sourceData;
            int len = strlen((char *)src);
            uint8_t* dest = src;

            uint8_t uch;
            while ((uch = *src++)) {
                if (uch & 0x80)
                    *dest++ = ((uch << 6) & 0xC0) | (*src++ & 0x3F);
                else
                    *dest++ = uch;
            }
            *dest = 0;

            // now convert from native encoding to UTF-8
            const char* source = sourceData;
            int targetLength = len * 3 + 1;
            buffer = new char[targetLength];
            // don't normally check for NULL, but in this case targetLength may be large
            if (!buffer) {
                ucnv_close(conv);
                ucnv_close(utf8Conv);
                return NULL;
            }
            char* target = buffer;
            ucnv_convertEx(utf8Conv, conv, &target, target + targetLength,
                    &source, (const char *)dest, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
            if (U_FAILURE(status)) {
                ALOGE("ucnv_convertEx failed: %d", status);
                //mValues->setEntry(i, "???");
            } else {
                // zero terminate
                ALOGD("End zero terminate,target=%p",target);
                *target = 0;
                //mValues->setEntry(i, buffer);
            }

            //delete[] buffer;
        //}

        ucnv_close(conv);
        ucnv_close(utf8Conv);
    }
    return buffer;
}
// MStar Android Patch End

static jobject android_media_MediaMetadataRetriever_extractMetadata(JNIEnv *env, jobject thiz, jint keyCode)
{
    ALOGV("extractMetadata");
    MediaMetadataRetriever* retriever = getRetriever(env, thiz);
    if (retriever == 0) {
        jniThrowException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }
    const char* value = retriever->extractMetadata(keyCode);
    if (!value) {
        ALOGV("extractMetadata: Metadata is not found");
        return NULL;
    }
    ALOGV("extractMetadata: value (%s) for keyCode(%d)", value, keyCode);

    // MStar Android Patch Begin
    int len = strlen(value);
    if ((len < 1024) && (len > 0)) {
        UErrorCode status = U_ZERO_ERROR;
        UCharsetDetector *csd = ucsdet_open(&status);
        const UCharsetMatch *ucm;
        const char *enc;
        char rawdata[len+1];
        memcpy(rawdata,value,len);
        rawdata[len] = 0;
        ucsdet_setText(csd, rawdata, len, &status);
        ucm = ucsdet_detect(csd, &status);
        if (!ucm) {
            ALOGV("failed to detect the charset.\n");
            return env->NewStringUTF(rawdata);
        }
        enc = ucsdet_getName(ucm, &status);
        ALOGV("the enc is:%s\n", enc);
        ucsdet_close(csd);
        if (strcmp(enc,"UTF-8") == 0 || enc == NULL) {
            return env->NewStringUTF(rawdata);
        } else {
            uint32_t encoding = kEncodingAll;
            char* utf8data = NULL;
            encoding &= checkPossibleEncodings(value);
            if (encoding & kEncodingAll) {
                utf8data = RetrieverConvertValues(enc, rawdata);
            }
            if (utf8data != NULL) {
                jobject metadata = env->NewStringUTF(utf8data);
                delete[] utf8data;
                return metadata;
            }
        }
    }
    // MStar Android Patch End

    return env->NewStringUTF(value);
}

static void android_media_MediaMetadataRetriever_release(JNIEnv *env, jobject thiz)
{
    ALOGV("release");
    Mutex::Autolock lock(sLock);
    MediaMetadataRetriever* retriever = getRetriever(env, thiz);
    delete retriever;
    setRetriever(env, thiz, (MediaMetadataRetriever*) 0);
}

static void android_media_MediaMetadataRetriever_native_finalize(JNIEnv *env, jobject thiz)
{
    ALOGV("native_finalize");
    // No lock is needed, since android_media_MediaMetadataRetriever_release() is protected
    android_media_MediaMetadataRetriever_release(env, thiz);
}

// This function gets a field ID, which in turn causes class initialization.
// It is called from a static block in MediaMetadataRetriever, which won't run until the
// first time an instance of this class is used.
static void android_media_MediaMetadataRetriever_native_init(JNIEnv *env)
{
    jclass clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == NULL) {
        return;
    }

    jclass bitmapClazz = env->FindClass("android/graphics/Bitmap");
    if (bitmapClazz == NULL) {
        return;
    }
    fields.bitmapClazz = (jclass) env->NewGlobalRef(bitmapClazz);
    if (fields.bitmapClazz == NULL) {
        return;
    }
    fields.createBitmapMethod =
            env->GetStaticMethodID(fields.bitmapClazz, "createBitmap",
                    "(IILandroid/graphics/Bitmap$Config;)"
                    "Landroid/graphics/Bitmap;");
    if (fields.createBitmapMethod == NULL) {
        return;
    }
    fields.createScaledBitmapMethod =
            env->GetStaticMethodID(fields.bitmapClazz, "createScaledBitmap",
                    "(Landroid/graphics/Bitmap;IIZ)"
                    "Landroid/graphics/Bitmap;");
    if (fields.createScaledBitmapMethod == NULL) {
        return;
    }
    fields.nativeBitmap = env->GetFieldID(fields.bitmapClazz, "mNativeBitmap", "J");
    if (fields.nativeBitmap == NULL) {
        return;
    }

    jclass configClazz = env->FindClass("android/graphics/Bitmap$Config");
    if (configClazz == NULL) {
        return;
    }
    fields.configClazz = (jclass) env->NewGlobalRef(configClazz);
    if (fields.configClazz == NULL) {
        return;
    }
    fields.createConfigMethod =
            env->GetStaticMethodID(fields.configClazz, "nativeToConfig",
                    "(I)Landroid/graphics/Bitmap$Config;");
    if (fields.createConfigMethod == NULL) {
        return;
    }
}

static void android_media_MediaMetadataRetriever_native_setup(JNIEnv *env, jobject thiz)
{
    ALOGV("native_setup");
    MediaMetadataRetriever* retriever = new MediaMetadataRetriever();
    if (retriever == 0) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    setRetriever(env, thiz, retriever);
}

// JNI mapping between Java methods and native methods
static JNINativeMethod nativeMethods[] = {
        {
            "_setDataSource",
            "(Landroid/os/IBinder;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
            (void *)android_media_MediaMetadataRetriever_setDataSourceAndHeaders
        },

        {"setDataSource",   "(Ljava/io/FileDescriptor;JJ)V", (void *)android_media_MediaMetadataRetriever_setDataSourceFD},
        {"_getFrameAtTime", "(JI)Landroid/graphics/Bitmap;", (void *)android_media_MediaMetadataRetriever_getFrameAtTime},
        {"extractMetadata", "(I)Ljava/lang/String;", (void *)android_media_MediaMetadataRetriever_extractMetadata},
        {"getEmbeddedPicture", "(I)[B", (void *)android_media_MediaMetadataRetriever_getEmbeddedPicture},
        {"release",         "()V", (void *)android_media_MediaMetadataRetriever_release},
        {"native_finalize", "()V", (void *)android_media_MediaMetadataRetriever_native_finalize},
        {"native_setup",    "()V", (void *)android_media_MediaMetadataRetriever_native_setup},
        {"native_init",     "()V", (void *)android_media_MediaMetadataRetriever_native_init},
};

// This function only registers the native methods, and is called from
// JNI_OnLoad in android_media_MediaPlayer.cpp
int register_android_media_MediaMetadataRetriever(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods
        (env, kClassPathName, nativeMethods, NELEM(nativeMethods));
}
