LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libgbigmpservice

LOCAL_SRC_FILES:= \
    GbIgmpService.cpp \
    IGbIgmpService.cpp \
    IGbIgmp.cpp \
    GbIgmp.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \
    libbinder \
    libgbutils \
    libcutils

LOCAL_C_INCLUDES += \
    $(TOP)/device/grandbeing/ipd8000l/libraries/gbutils \
    $(TOP)/device/grandbeing/ipd8000l/libraries/gbservices/include

include $(BUILD_SHARED_LIBRARY)