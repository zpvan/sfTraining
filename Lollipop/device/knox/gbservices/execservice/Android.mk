LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libgbexecservice

LOCAL_SRC_FILES:= \
    GbExecService.cpp \
    IGbExecService.cpp \
    GbExec.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \
    libbinder \
    libcutils

LOCAL_C_INCLUDES += \
    $(TOP)/device/grandbeing/ipd8000l/libraries/gbservices/include

include $(BUILD_SHARED_LIBRARY)