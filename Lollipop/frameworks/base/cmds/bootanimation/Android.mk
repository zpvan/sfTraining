LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bootanimation_main.cpp \
	AudioPlayer.cpp \
	BootAnimation.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

# MStar Android Patch Begin
ifeq ($(ENABLE_STR),true)
    LOCAL_CFLAGS += -DENABLE_STR
endif
# MStar Android Patch Begin, for C2_CMCC
ifeq ($(operator_project),cmcc)
    LOCAL_CFLAGS += -DOPERATOR_PROJECT_CMCC
endif
# MStar Android Patch End, for C2_CMCC
# MStar Android Patch End

LOCAL_C_INCLUDES += external/tinyalsa/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libandroidfw \
	libutils \
	libbinder \
    libui \
	libskia \
    libEGL \
    libGLESv1_CM \
    libgui \
    libtinyalsa

LOCAL_MODULE:= bootanimation

ifdef TARGET_32_BIT_SURFACEFLINGER
LOCAL_32_BIT_ONLY := true
endif

include $(BUILD_EXECUTABLE)
