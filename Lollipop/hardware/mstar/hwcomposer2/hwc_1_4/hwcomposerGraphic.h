//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2014 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party's software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party's software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar's confidential information and you agree to keep MStar's
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer's product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

#ifndef HWCOMPOSER_GRAPHIC_H
#define HWCOMPOSER_GRAPHIC_H

#include "hwcomposer_context.h"

/*
  mstar.debug.hwcomposer13 = 0x1 : show the hwindow info every post primary
  mstar.debug.hwcomposer13 = 0x2 : show the hwcomposer13 fps
  mstar.debug.hwcomposer13 = 0x4 : disable gopoverlay, all layers will fallback to FrameBufferTarget
  mstar.debug.hwcomposer13 = 0x8 : show the zorder of the video plane & hwWindow
  mstar.debug.hwcomposer13 = 0x10 : show all the layers attributes
  mstar.debug.hwcomposer13 = 0x20 : force to update glesblend layer
  mstar.debug.hwcomposer13 = 0x40 : turn off framebuffer's afbc mode
*/
#define HWCOMPOSER13_DEBUG_PROPERTY                  "mstar.debug.hwcomposer13"
#define MAX_ACTIVE_DEGREE (20)
#define MIN_ACTIVE_DEGREE (-20)
#define ALIGNMENT( value, base ) ((value) & ~((base) - 1))

enum VENDOR_USAGE_BITS {
    VENDOR_CMA_CONTIGUOUS_MEMORY        = 0x00000001,
    VENDOR_CMA_DISCRETE_MEMORY          = 0x00000002,
};

int graphic_init_primary(hwc_context_t* ctx);
int graphic_set_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent);
void graphic_prepare_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent, int &videoLayerNumber,int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM]);
void graphic_close_primary(hwc_context_t* ctx);
unsigned int graphic_getGOPDestination(hwc_context_t* ctx);
int graphic_recalc3DTransform(hwc_context_t* ctx, hwc_3d_param_t* params, hwc_rect_t frame);
int graphic_getCurOPTiming(int *ret_width, int *ret_height, int *ret_hstart);
bool graphic_checkGopTimingChanged(hwc_context_t* ctx);
int graphic_startCapture(hwc_context_t* ctx, int* width, int* height, int withOsd);
int graphic_captureScreenWithClip(hwc_context_t* ctx, size_t numData, hwc_capture_data_t* captureData, int* width, int* height, int withOsd);
int graphic_releaseCaptureScreenMemory(hwc_context_t* ctx);
#endif // HWCOMPOSER_GRAPHIC_H
