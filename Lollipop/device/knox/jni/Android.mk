LOCAL_PATH := $(call my-dir)

# libgb_igmp_jni

include $(CLEAR_VARS)

LOCAL_MODULE := libgb_igmp_jni
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	com_grandbeing_kmediaplayer_utils_Cigmp.cpp

LOCAL_SHARED_LIBRARIES := \
    libgbutils \
    liblog \
    libgbigmpservice \
    libutils

LOCAL_C_INCLUDES += \
	$(TOP)/device/grandbeing/ipd8000l/libraries/gbutils\
	$(TOP)/device/grandbeing/ipd8000l/libraries/gbservices/include

include $(BUILD_SHARED_LIBRARY)

# libgb_exec_jni

include $(CLEAR_VARS)

LOCAL_MODULE := libgb_exec_jni
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	com_grandbeing_kmediaplayer_utils_GbExec.cpp

LOCAL_SHARED_LIBRARIES := \
    libgbutils \
    liblog \
    libgbexecservice \
    libutils

LOCAL_C_INCLUDES += \
	$(TOP)/device/grandbeing/ipd8000l/libraries/gbutils\
	$(TOP)/device/grandbeing/ipd8000l/libraries/gbservices/include

include $(BUILD_SHARED_LIBRARY)