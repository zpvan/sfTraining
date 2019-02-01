#
# Copyright (C) 2010 ARM Limited. All rights reserved.
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# MStar: indicate our modifications
MSTAR=1

ifeq ($(filter madison kaiser napoli,$(TARGET_BOARD_PLATFORM)),) # MStar: filter certain projects to prevent build failure

LOCAL_PATH := $(call my-dir)

# Include platform specific makefiles
include $(if $(wildcard $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk), $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk,)

MALI_ARCHITECTURE_UTGARD?=0
MALI_ION?=1
GRALLOC_VSYNC_BACKEND?=default
DISABLE_FRAMEBUFFER_HAL?=0
MALI_SUPPORT_AFBC_WIDEBLK?=0
MALI_USE_YUV_AFBC_WIDEBLK?=0
GRALLOC_USE_ION_DMA_HEAP?=0
GRALLOC_USE_ION_COMPOUND_PAGE_HEAP?=0
GRALLOC_INIT_AFBC?=0

# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
include $(BUILD_SYSTEM)/version_defaults.mk

ifeq ($(TARGET_BOARD_PLATFORM), juno)
ifeq ($(MALI_MMSS), 1)
        GRALLOC_FB_SWAP_RED_BLUE = 0
    GRALLOC_DEPTH = GRALLOC_32_BITS
    MALI_ION = 1
    MALI_AFBC_GRALLOC = 1
    MALI_DISPLAY_VERSION = 550
    AFBC_YUV420_EXTRA_MB_ROW_NEEDED = 1
    GRALLOC_USE_ION_DMA_HEAP = 1
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), armboard_v7a)
ifeq ($(GRALLOC_MALI_DP),true)
    GRALLOC_FB_SWAP_RED_BLUE = 0
    MALI_ION = 1
    DISABLE_FRAMEBUFFER_HAL=1
    MALI_AFBC_GRALLOC = 1
    MALI_DISPLAY_VERSION = 550
    GRALLOC_USE_ION_DMA_HEAP=1
endif
endif

ifeq ($(MALI_ARCHITECTURE_UTGARD),1)
    # Utgard build settings
    MALI_LOCAL_PATH?=hardware/arm/mali
    GRALLOC_DEPTH?=GRALLOC_32_BITS
    GRALLOC_FB_SWAP_RED_BLUE?=1
    MALI_DDK_INCLUDES=$(MALI_LOCAL_PATH)/include $(MALI_LOCAL_PATH)/src/ump/include
    LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
    ifeq ($(MALI_ION),1)
        ALLOCATION_LIB := libion
        ALLOCATOR_SPECIFIC_FILES := alloc_ion.cpp gralloc_module_ion.cpp
    else
        ALLOCATION_LIB := libUMP
        ALLOCATOR_SPECIFIC_FILES := alloc_ump.cpp gralloc_module_ump.cpp
    endif
else
    # Midgard build settings
    MALI_LOCAL_PATH?=vendor/arm/mali6xx
    GRALLOC_DEPTH?=GRALLOC_16_BITS
    GRALLOC_FB_SWAP_RED_BLUE?=0
    MALI_DDK_INCLUDES=$(MALI_LOCAL_PATH)/include $(MALI_LOCAL_PATH)/kernel/include
    # MStar: LOCAL_MODULE_RELATIVE_PATH is used in Nougat instead of LOCAL_MODULE_PATH
    ifneq ($(MSTAR),1)
    LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/hw
    endif
    ifeq ($(MALI_ION),1)
        ifeq ($(MSTAR),1)
        ALLOCATOR_SPECIFIC_FILES := alloc_ion.cpp gralloc_module_ion.cpp ion.cpp
        else
        ALLOCATION_LIB := libion
        ALLOCATOR_SPECIFIC_FILES := alloc_ion.cpp gralloc_module_ion.cpp
        endif
    else
        ALLOCATION_LIB := libGLES_mali
        ALLOCATOR_SPECIFIC_FILES := alloc_ump.cpp gralloc_module_ump.cpp
    endif
endif

ifeq ($(MALI_AFBC_GRALLOC), 1)
AFBC_FILES = gralloc_buffer_priv.cpp
ifeq ($(MSTAR),1)
GRALLOC_ARM_NO_EXTERNAL_AFBC := 0
endif
else
MALI_AFBC_GRALLOC := 0
AFBC_FILES =
ifeq ($(MSTAR),1)
GRALLOC_ARM_NO_EXTERNAL_AFBC := 1
endif
endif

#If cropping should be enabled for AFBC YUV420 buffers
AFBC_YUV420_EXTRA_MB_ROW_NEEDED ?= 0

ifdef MALI_DISPLAY_VERSION
#if Mali display is available, should disable framebuffer HAL
DISABLE_FRAMEBUFFER_HAL := 1
#if Mali display is available, AFBC buffers should be initialised after allocation
GRALLOC_INIT_AFBC := 1
endif

LOCAL_PRELINK_MODULE := false


LOCAL_SHARED_LIBRARIES := libhardware liblog libcutils libGLESv1_CM $(ALLOCATION_LIB)
ifeq ($(MSTAR),1)
LOCAL_C_INCLUDES :=
LOCAL_CFLAGS := -DLOG_TAG=\"gralloc\" -DMALI_ION=$(MALI_ION) -DMALI_AFBC_GRALLOC=$(MALI_AFBC_GRALLOC) -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) -DDISABLE_FRAMEBUFFER_HAL=$(DISABLE_FRAMEBUFFER_HAL) -DMALI_SUPPORT_AFBC_WIDEBLK=$(MALI_SUPPORT_AFBC_WIDEBLK) -DMALI_USE_YUV_AFBC_WIDEBLK=$(MALI_USE_YUV_AFBC_WIDEBLK) -DAFBC_YUV420_EXTRA_MB_ROW_NEEDED=$(AFBC_YUV420_EXTRA_MB_ROW_NEEDED) -DGRALLOC_USE_ION_DMA_HEAP=$(GRALLOC_USE_ION_DMA_HEAP) -DGRALLOC_USE_ION_COMPOUND_PAGE_HEAP=$(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP) -DGRALLOC_INIT_AFBC=$(GRALLOC_INIT_AFBC)
else
LOCAL_C_INCLUDES := $(MALI_LOCAL_PATH) $(MALI_DDK_INCLUDES)
LOCAL_CFLAGS := -DLOG_TAG=\"gralloc\" -DSTANDARD_LINUX_SCREEN -DMALI_ION=$(MALI_ION) -DMALI_AFBC_GRALLOC=$(MALI_AFBC_GRALLOC) -D$(GRALLOC_DEPTH) -DMALI_ARCHITECTURE_UTGARD=$(MALI_ARCHITECTURE_UTGARD) -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) -DDISABLE_FRAMEBUFFER_HAL=$(DISABLE_FRAMEBUFFER_HAL) -DMALI_SUPPORT_AFBC_WIDEBLK=$(MALI_SUPPORT_AFBC_WIDEBLK) -DMALI_USE_YUV_AFBC_WIDEBLK=$(MALI_USE_YUV_AFBC_WIDEBLK) -DAFBC_YUV420_EXTRA_MB_ROW_NEEDED=$(AFBC_YUV420_EXTRA_MB_ROW_NEEDED) -DGRALLOC_USE_ION_DMA_HEAP=$(GRALLOC_USE_ION_DMA_HEAP) -DGRALLOC_USE_ION_COMPOUND_PAGE_HEAP=$(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP) -DGRALLOC_INIT_AFBC=$(GRALLOC_INIT_AFBC)
endif

ifdef GRALLOC_DISP_W
LOCAL_CFLAGS += -DGRALLOC_DISP_W=$(GRALLOC_DISP_W)
endif
ifdef GRALLOC_DISP_H
LOCAL_CFLAGS += -DGRALLOC_DISP_H=$(GRALLOC_DISP_H)
endif

ifdef MALI_DISPLAY_VERSION
LOCAL_CFLAGS += -DMALI_DISPLAY_VERSION=$(MALI_DISPLAY_VERSION)
endif

ifeq ($(wildcard system/core/libion/include/ion/ion.h),)
LOCAL_C_INCLUDES += system/core/include
LOCAL_CFLAGS += -DGRALLOC_OLD_ION_API
else
LOCAL_C_INCLUDES += system/core/libion/include
endif

ifeq ($(GRALLOC_FB_SWAP_RED_BLUE),1)
LOCAL_CFLAGS += -DGRALLOC_FB_SWAP_RED_BLUE
endif

ifeq ($(GRALLOC_ARM_NO_EXTERNAL_AFBC),1)
LOCAL_CFLAGS += -DGRALLOC_ARM_NO_EXTERNAL_AFBC=1
endif

ifdef PLATFORM_CFLAGS
LOCAL_CFLAGS += $(PLATFORM_CFLAGS)
endif

LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/hw
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/hw
ifeq ($(TARGET_BOARD_PLATFORM),)
LOCAL_MODULE := gralloc.default
else
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM)
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

ifeq ($(MSTAR),1)
LOCAL_ARM_MODE := arm
endif

LOCAL_SRC_FILES := \
    gralloc_module.cpp \
    alloc_device.cpp \
    $(ALLOCATOR_SPECIFIC_FILES) \
    framebuffer_device.cpp \
    format_chooser.cpp \
    format_chooser_blockinit.cpp \
    $(AFBC_FILES) \
    gralloc_vsync_${GRALLOC_VSYNC_BACKEND}.cpp

LOCAL_MODULE_OWNER := arm

# MStar: additional libs, include paths, and defines/flags
ifeq ($(MSTAR),1)

LOCAL_SHARED_LIBRARIES += \
    libc \
    libdl \
    libfbdev \
    libiniparser \
    libmutils \
    libutopia

LOCAL_C_INCLUDES += \
    $(TARGET_UTOPIA_LIBS_DIR)/include \
    external/iniparser \
    device/mstar/common/libraries/mutils \
    hardware/mstar/libfbdev2

LOCAL_CFLAGS += \
    -DMSTAR_GRALLOC \
    -DMSTAR_GRALLOC_NO_VSYNC \
    -DMSTAR_GRALLOC_FORMAT \
    -DMSTAR_GRALLOC_USAGE_PRIVATE \
    -DMSTAR_GRALLOC_AFBC \
    -DMSTAR_GRALLOC_CMA \
    -DMSTAR_FAKE_FBDEV \
    -DMSOS_TYPE_LINUX \
    -fpermissive \
    -Wall \
    -Wno-unused-parameter

ifeq ($(TARGET_BOARD_PLATFORM),muji)
LOCAL_CFLAGS += -DTARGET_BOARD_PLATFORM_MUJI
endif

ifeq ($(ENABLE_GRALLOC_LOCK_PHY_BUFFER), 1)
LOCAL_CFLAGS += -DENABLE_GRALLOC_LOCK_PHY_BUFFER
endif

ifeq ($(BUILD_FOR_STB),true)
LOCAL_CFLAGS += -DBUILD_FOR_STB
endif

# hw cursors, not used after lmr1
ifeq ($(ENABLE_HWCURSOR),true)
LOCAL_SRC_FILES += hwcursor_device.cpp
LOCAL_C_INCLUDES += hardware/mstar/libhwcursordev
LOCAL_CFLAGS += -DENABLE_HWCURSOR
LOCAL_SHARED_LIBRARIES += libhwcursordev
endif

endif # MSTAR

include $(BUILD_SHARED_LIBRARY)

endif
