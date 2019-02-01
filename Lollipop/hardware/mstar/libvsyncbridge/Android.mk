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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libvsyncbridge

LOCAL_SRC_FILES := \
    vsync_bridge.c \
    dynamic_scale.c \
    gop_display.c \
    disp_api.cpp \
    sideband.c

ifeq ($(BUILD_MSTARTV),sn)
    LOCAL_SRC_FILES += VideoSetWrapper_tvos.cpp
else ifeq ($(BUILD_MSTARTV),mi)
    LOCAL_SRC_FILES += VideoSetWrapper_mi.cpp
else ifeq ($(BUILD_MSTARTV),ddi)
    LOCAL_SRC_FILES += VideoSetWrapper_ddi.cpp
endif

LOCAL_C_INCLUDES := \
    $(TARGET_UTOPIA_LIBS_DIR)/include \
    device/mstar/common/libraries/mutils \
    hardware/mstar/gralloc \
    hardware/mstar/libfbdev \
    hardware/mstar/omx/ms_codecs/video/mm_asic/include \
    hardware/mstar/libstagefrighthw/include \
    hardware/mstar/libstagefrighthw/yuv \
    hardware/mstar/libstagefrighthw/dip \
    hardware/libhardware/include/hardware \
    hardware/mstar/synchronizer \
    hardware/mstar/tv_input \
    system/core/include/system \
    external/iniparser \
    $(TARGET_TVAPI_LIBS_DIR)/include \
    device/mstar/common/libraries/resourcemanager/include

LOCAL_CFLAGS := \
    -DLOG_TAG=\"vsyncbridge\" \
    -DMSOS_TYPE_LINUX
ifeq ($(TARGET_BOARD_PLATFORM),edison)
    LOCAL_CFLAGS += -DMSTAR_EDISON
else ifeq ($(TARGET_BOARD_PLATFORM),nike)
    LOCAL_CFLAGS += -DMSTAR_NIKE
else ifeq ($(TARGET_BOARD_PLATFORM),napoli)
    LOCAL_CFLAGS += -DMSTAR_NAPOLI
else ifeq ($(TARGET_BOARD_PLATFORM),monaco)
    LOCAL_CFLAGS += -DMSTAR_MONACO
else ifeq ($(TARGET_BOARD_PLATFORM),kaiser)
    LOCAL_CFLAGS += -DMSTAR_KAISER
else ifeq ($(TARGET_BOARD_PLATFORM),madison)
    LOCAL_CFLAGS += -DMSTAR_MADISON
else ifeq ($(TARGET_BOARD_PLATFORM),kano)
    LOCAL_CFLAGS += \
        -DMSTAR_KANO \
        -DMHDMITX_ENABLE=1
else ifeq ($(TARGET_BOARD_PLATFORM),curry)
    LOCAL_CFLAGS += \
        -DMSTAR_CURRY \
        -DMHDMITX_ENABLE=1
else ifeq ($(TARGET_BOARD_PLATFORM),clippers)
    LOCAL_CFLAGS += \
        -DMSTAR_CLIPPERS \
        -DMHDMITX_ENABLE=1
else ifeq ($(TARGET_BOARD_PLATFORM),muji)
    LOCAL_CFLAGS += -DMSTAR_MUJI
else ifeq ($(TARGET_BOARD_PLATFORM),monet)
    LOCAL_CFLAGS += -DMSTAR_MONET
endif

ifeq ($(TARGET_BOARD_PLATFORM),messi)
    LOCAL_CFLAGS += -DMSTAR_MESSI
endif

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libfbdev \
    libcutils \
    libutils \
    libmutils \
    libutopia \
    librmclient \
    libhardware \
    libvideoset \
    libiniparser \
    libstagefrighthw_dip \
    libthreedimensionmanager \
    libpicturemanager \
    libpipmanager \
    libstagefrighthw_dip

ifeq ($(BUILD_MSTARTV),sn)
    LOCAL_SHARED_LIBRARIES += \
    libtvmanager

    LOCAL_CFLAGS += -DBUILD_MSTARTV_SN
else ifeq ($(BUILD_MSTARTV),mi)
    LOCAL_C_INCLUDES += \
        $(TARGET_MI_LIBS_DIR)/include \
        $(TARGET_MI_LIBS_DIR)/include/datatype
    LOCAL_CFLAGS += -DBUILD_MSTARTV_MI
    LOCAL_SHARED_LIBRARIES += libmi
ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),cherry)
    #cherry project is special,it need open BUILD_FOR_STB but do not have Middleware
else ifeq ($(BUILD_FOR_STB),true)
    LOCAL_CFLAGS += -DBUILD_FOR_STB
endif
endif

include $(BUILD_SHARED_LIBRARY)

# ==============================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libmsfrmfmt

LOCAL_SRC_FILES := msfrmfmt.cpp

LOCAL_C_INCLUDES := \
    $(TARGET_UTOPIA_LIBS_DIR)/include \
    hardware/mstar/libvsyncbridge

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libui \
    libstagefright

include $(BUILD_SHARED_LIBRARY)
