LOCAL_PATH := $(call my-dir)

#--------------------------------------
# libgbigmp
#--------------------------------------
# include $(CLEAR_VARS)

# LOCAL_MODULE := libgbigmp

# LOCAL_SRC_FILES:= \
    

# include $(BUILD_SHARED_LIBRARY)

#--------------------------------------
# libgbigmpservice
#--------------------------------------
# include $(CLEAR_VARS)

# LOCAL_MODULE := libgbigmpservice

# LOCAL_SRC_FILES:= \
#   GbIgmpService.cpp \
#   IGbIgmpService.cpp \
#   IGbIgmp.cpp \
#   GbIgmp.cpp

# LOCAL_SHARED_LIBRARIES := \
#     libutils \
#     liblog \
#     libbinder \
#     libgbutils \
#     libcutils

# LOCAL_C_INCLUDES += \
#   $(TOP)/device/grandbeing/ipd8000l/libraries/gbutils

# include $(BUILD_SHARED_LIBRARY)

#--------------------------------------
# gbserver
#--------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := gbserver
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
    main_gbserver.cpp

# libutils ==> sp<>
# libbinder ==> binder
LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \
    libbinder \
    libgbigmpservice \
    libgbexecservice

LOCAL_C_INCLUDES += \
    $(TOP)/device/grandbeing/ipd8000l/libraries/gbutils \
    $(TOP)/device/grandbeing/ipd8000l/libraries/gbservices/include

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))