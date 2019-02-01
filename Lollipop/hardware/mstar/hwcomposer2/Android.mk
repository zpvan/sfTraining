#<MStar Software>
#******************************************************************************
# MStar Software
# Copyright (c) 2010 - 2015 MStar Semiconductor, Inc. All rights reserved.
# All software, firmware and related documentation herein ("MStar Software") are
# intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
# law, including, but not limited to, copyright law and international treaties.
# Any use, modification, reproduction, retransmission, or republication of all
# or part of MStar Software is expressly prohibited, unless prior written
# permission has been granted by MStar.
#
# By accessing, browsing and/or using MStar Software, you acknowledge that you
# have read, understood, and agree, to be bound by below terms ("Terms") and to
# comply with all applicable laws and regulations:
#
# 1. MStar shall retain any and all right, ownership and interest to MStar
#    Software and any modification/derivatives thereof.
#    No right, ownership, or interest to MStar Software and any
#    modification/derivatives thereof is transferred to you under Terms.
#
# 2. You understand that MStar Software might include, incorporate or be
#    supplied together with third party's software and the use of MStar
#    Software may require additional licenses from third parties.
#    Therefore, you hereby agree it is your sole responsibility to separately
#    obtain any and all third party right and license necessary for your use of
#    such third party's software.
#
# 3. MStar Software and any modification/derivatives thereof shall be deemed as
#    MStar's confidential information and you agree to keep MStar's
#    confidential information in strictest confidence and not disclose to any
#    third party.
#
# 4. MStar Software is provided on an "AS IS" basis without warranties of any
#    kind. Any warranties are hereby expressly disclaimed by MStar, including
#    without limitation, any warranties of merchantability, non-infringement of
#    intellectual property rights, fitness for a particular purpose, error free
#    and in conformity with any international standard.  You agree to waive any
#    claim against MStar for any loss, damage, cost or expense that you may
#    incur related to your use of MStar Software.
#    In no event shall MStar be liable for any direct, indirect, incidental or
#    consequential damages, including without limitation, lost of profit or
#    revenues, lost or damage of data, and unauthorized system use.
#    You agree that this Section 4 shall still apply without being affected
#    even if MStar Software has been modified by MStar in accordance with your
#    request or instruction for your use, except otherwise agreed by both
#    parties in writing.
#
# 5. If requested, MStar may from time to time provide technical supports or
#    services in relation with MStar Software to you for your use of
#    MStar Software in conjunction with your or your customer's product
#    ("Services").
#    You understand and agree that, except otherwise agreed by both parties in
#    writing, Services are provided on an "AS IS" basis and the warranty
#    disclaimer set forth in Section 4 above shall apply.
#
# 6. Nothing contained herein shall be construed as by implication, estoppels
#    or otherwise:
#    (a) conferring any license or right to use MStar name, trademark, service
#        mark, symbol or any other identification;
#    (b) obligating MStar or any of its affiliates to furnish any person,
#        including without limitation, you and your customers, any assistance
#        of any kind whatsoever, or any information; or
#    (c) conferring any license or right under any intellectual property right.
#
# 7. These terms shall be governed by and construed in accordance with the laws
#    of Taiwan, R.O.C., excluding its conflict of law rules.
#    Any and all dispute arising out hereof or related hereto shall be finally
#    settled by arbitration referred to the Chinese Arbitration Association,
#    Taipei in accordance with the ROC Arbitration Law and the Arbitration
#    Rules of the Association by three (3) arbitrators appointed in accordance
#    with the said Rules.
#    The place of arbitration shall be in Taipei, Taiwan and the language shall
#    be English.
#    The arbitration award shall be final and binding to both parties.
#
#******************************************************************************
#<MStar Software>

ifeq ($(filter madison kaiser napoli,$(TARGET_BOARD_PLATFORM)),)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
ifeq ($(USE_HWC2),true)
LOCAL_SRC_FILES := hwc_2_0/hwcomposer.cpp\
    hwc_2_0/HWComposerDevice.cpp \
    hwc_2_0/HWCDisplayDevice.cpp \
    hwc_2_0/HWCLayer.cpp \
    hwc_2_0/HWComposerOSD.cpp \
    hwc_2_0/HWComposerGop.cpp \
    hwc_2_0/HWComposerDip.cpp \
    hwc_2_0/HWComposerTVVideo.cpp \
    hwc_2_0/HWComposerMMVideo.cpp \
    hwc_2_0/HWCCursor.cpp \
    hwc_2_0/HWComposerCursorDev.cpp \
    hwc_2_0/debug.cpp

else
LOCAL_SRC_FILES := hwc_1_4/hwcomposer.cpp \
    hwc_1_4/hwcomposer_platform.cpp \
    hwc_1_4/hwcomposerMMVideo.cpp \
    hwc_1_4/hwcomposerTVVideo.cpp \
    hwc_1_4/hwcomposerGraphic.cpp \
    hwc_1_4/hwcomposer_util.cpp \
    hwc_1_4/hwcomposer_gop.cpp \
    hwc_1_4/hwcomposer_dip.cpp
endif

LOCAL_C_INCLUDES := \
    $(TARGET_UTOPIA_LIBS_DIR)/include \
    device/mstar/common/libraries/mutils \
    external/iniparser \
    hardware/mstar/gralloc2 \
    hardware/mstar/libvsyncbridge \
    hardware/mstar/omx/ms_codecs/video/mm_asic/include \
    $(TARGET_TVAPI_LIBS_DIR)/include \
    hardware/mstar/libfbdev2 \
    device/mstar/common/libraries/resourcemanager/include \
    system/core/libsync \
    system/core/include/sync \
    $(TARGET_MI_LIBS_DIR)/include \
    $(TARGET_MI_LIBS_DIR)/include/datatype \
    hardware/mstar/libfbdev2 \
    hardware/mstar/tv_input \
    hardware/libhardware/include/hardware

ifneq ($(filter 21 22 23 24, $(PLATFORM_SDK_VERSION)),)
    LOCAL_C_INCLUDES += external/libcxx/include
else
    LOCAL_C_INCLUDES += \
        bionic \
        bionic/libstdc++/include \
        external/stlport/stlport
    LOCAL_STATIC_LIBRARIES := libstlport_static
endif

LOCAL_CFLAGS := \
    -DLOG_TAG=\"hwcomposer\" \
    -DMSOS_TYPE_LINUX

ifeq ($(MALI_AFBC_GRALLOC),1)
    LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC
endif

ifeq ($(TARGET_BOARD_PLATFORM),monaco)
    LOCAL_CFLAGS += -DMSTAR_MONACO
endif
ifeq ($(TARGET_BOARD_PLATFORM),muji)
    LOCAL_CFLAGS += -DMSTAR_MUJI
endif
ifeq ($(TARGET_BOARD_PLATFORM),clippers)
    LOCAL_CFLAGS += -DMSTAR_CLIPPERS
endif
ifeq ($(TARGET_BOARD_PLATFORM),messi)
    LOCAL_CFLAGS += -DMSTAR_MESSI
endif
ifeq ($(TARGET_BOARD_PLATFORM),kano)
    LOCAL_CFLAGS += -DMSTAR_KANO
endif
ifeq ($(TARGET_BOARD_PLATFORM),maserati)
    LOCAL_CFLAGS += -DMSTAR_MASERATI
endif
ifeq ($(TARGET_BOARD_PLATFORM),curry)
    LOCAL_CFLAGS += -DMSTAR_CURRY
endif

ifeq ($(ENABLE_STR),true)
    LOCAL_CFLAGS += -DENABLE_STR
endif


HAVE_CURSOR := false
ifeq ($(ENABLE_GAMECURSOR),true)
    HAVE_CURSOR := true
    LOCAL_CFLAGS += -DENABLE_GAMECURSOR
endif

ifneq ($(filter 23 24, $(PLATFORM_SDK_VERSION)),)
    LOCAL_CFLAGS += -DENABLE_DOUBLEGAMECURSOR
endif
ifeq ($(ENABLE_HWC14_CURSOR),true)
    HAVE_CURSOR := true
    LOCAL_CFLAGS += -DENABLE_HWC14_CURSOR
endif

ifeq ($(ENABLE_SPECIFIED_VISIBLEREGION),true)
    LOCAL_CFLAGS += -DENABLE_SPECIFIED_VISIBLEREGION
endif

ifneq ($(USE_HWC2),true)
ifeq ($(HAVE_CURSOR),true)
    LOCAL_SRC_FILES += hwc_1_4/hwcursordev.cpp \
        hwc_1_4/hwcomposer_cursor.cpp
endif
else
    LOCAL_CFLAGS += -DUSE_HWC2
endif

ifeq ($(ENABLE_HWC14_SIDEBAND),true)
    LOCAL_CFLAGS += -DENABLE_HWC14_SIDEBAND
endif

ifneq ($(filter 22 23 24, $(PLATFORM_SDK_VERSION)),)
    LOCAL_CFLAGS += -DENABLE_HWC14_SIDEBAND
endif

ifeq ($(filter 19 20 21, $(PLATFORM_SDK_VERSION)),)
    LOCAL_CFLAGS += -DSUPPORT_TUNNELED_PLAYBACK
endif

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libEGL \
    libcutils \
    libhardware \
    libhardware_legacy \
    libiniparser \
    libmutils \
    libutopia \
    libvsyncbridge \
    libutils \
    libsync \
    libfbdev \
    libmi

ifneq ($(filter 21 22 23 24, $(PLATFORM_SDK_VERSION)),)
    LOCAL_MODULE_RELATIVE_PATH := hw
else
    LOCAL_MODULE_RELATIVE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
endif

ifneq ($(MAX_VIRTUAL_DISPLAY_DIMENSION),)
    LOCAL_CFLAGS += -DMAX_VIRTUAL_DISPLAY_DIMENSION=$(MAX_VIRTUAL_DISPLAY_DIMENSION)
else
    LOCAL_CFLAGS += -DMAX_VIRTUAL_DISPLAY_DIMENSION=0
endif

include $(BUILD_SHARED_LIBRARY)
endif
