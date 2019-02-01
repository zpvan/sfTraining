LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	IGraphicBufferConsumer.cpp \
	IConsumerListener.cpp \
	BitTube.cpp \
	BufferItem.cpp \
	BufferItemConsumer.cpp \
	BufferQueue.cpp \
	BufferQueueConsumer.cpp \
	BufferQueueCore.cpp \
	BufferQueueProducer.cpp \
	BufferSlot.cpp \
	ConsumerBase.cpp \
	CpuConsumer.cpp \
	DisplayEventReceiver.cpp \
	GLConsumer.cpp \
	GraphicBufferAlloc.cpp \
	GuiConfig.cpp \
	IDisplayEventConnection.cpp \
	IGraphicBufferAlloc.cpp \
	IGraphicBufferProducer.cpp \
	IProducerListener.cpp \
	ISensorEventConnection.cpp \
	ISensorServer.cpp \
	ISurfaceComposer.cpp \
	ISurfaceComposerClient.cpp \
	LayerState.cpp \
	Sensor.cpp \
	SensorEventQueue.cpp \
	SensorManager.cpp \
	StreamSplitter.cpp \
	Surface.cpp \
	SurfaceControl.cpp \
	SurfaceComposerClient.cpp \
	SyncFeatures.cpp \

LOCAL_SHARED_LIBRARIES := \
	libbinder \
	libcutils \
	libEGL \
	libGLESv2 \
	libsync \
	libui \
	libutils \
	liblog


LOCAL_MODULE:= libgui

ifeq ($(TARGET_BOARD_PLATFORM), tegra)
	LOCAL_CFLAGS += -DDONT_USE_FENCE_SYNC
endif
ifeq ($(TARGET_BOARD_PLATFORM), tegra3)
	LOCAL_CFLAGS += -DDONT_USE_FENCE_SYNC
endif

# MStar Android Patch Begin
#ifneq ($(BUILD_MSTARTV),)
LOCAL_C_INCLUDES += \
        $(TARGET_UTOPIA_LIBS_DIR)/include \
        hardware/mstar/hwcomposer \
        hardware/mstar/libvsyncbridge \
        hardware/mstar/omx/ms_codecs/video/mm_asic/include
LOCAL_CFLAGS += -DBUILD_MSTARTV
LOCAL_SHARED_LIBRARIES += \
        libvsyncbridge
#endif
# MStar Android Patch End

include $(BUILD_SHARED_LIBRARY)

ifeq (,$(ONE_SHOT_MAKEFILE))
include $(call first-makefiles-under,$(LOCAL_PATH))
endif
