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

/* android header */
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>


#include <utils/Errors.h>
#include <gralloc_priv.h>
#include <sync/sync.h>
//#include <sync/sw_sync.h>
#include <sw_sync.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <assert.h>
#include <stdlib.h>

#include <utils/Timers.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <cutils/properties.h>

#include "hwcomposer_context.h"
#include "hwcomposer_util.h"
#include "hwcomposer_cursor.h"
#include "hwcursordev.h"

#define ALIGN(_val_,_align_) ((_val_ + _align_ - 1) & (~(_align_ - 1)))
#define GWIN_ALIGNMENT 2

#ifdef ENABLE_GAMECURSOR
int gameCursor_init(struct hwc_context_t* ctx, int32_t devNo) {
    int res = hwcursordev_init(ctx, devNo);
    if (res == -1)
        ALOGD("mmapInfo == NULL");
    return res;
}

int gameCursor_deinit(struct hwc_context_t* ctx, int32_t devNo) {
    int res = hwcursordev_deinit(ctx, devNo);
    if (res == 1) {
        ctx->hwCusorInfo[devNo].operation = 0;
    }
    return 0;
}

int gameCursor_show(struct hwc_context_t* ctx, int32_t devNo)
{
    ctx->hwCusorInfo[devNo].operation |= HWCURSOR_SHOW;
    return 0;
}

int gameCursor_hide(struct hwc_context_t* ctx, int32_t devNo)
{
    ctx->hwCusorInfo[devNo].operation |= HWCURSOR_HIDE;
    return 0;
}

int gameCursor_setAlpha(struct hwc_context_t* ctx, int32_t devNo, float alpha)
{
    ctx->hwCusorInfo[devNo].planeAlpha = alpha*255;
    ctx->hwCusorInfo[devNo].operation |= HWCURSOR_ALPHA;
    return 0;
}

int gameCursor_redrawFb(struct hwc_context_t* ctx, int32_t devNo, int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight)
{
    ctx->hwCusorInfo[devNo].srcAddr = ctx->CursorPhyAddr[devNo];
    ctx->hwCusorInfo[devNo].srcX = srcX;
    ctx->hwCusorInfo[devNo].srcY = srcY;
    ctx->hwCusorInfo[devNo].srcWidth = srcWidth;
    ctx->hwCusorInfo[devNo].srcHeight = srcHeight;
    ctx->hwCusorInfo[devNo].srcStride = stride;
    ctx->hwCusorInfo[devNo].dstWidth = ALIGN(dstWidth,GWIN_ALIGNMENT);
    ctx->hwCusorInfo[devNo].dstHeight = dstHeight;
    ctx->hwCusorInfo[devNo].gop_srcWidth = ctx->displayRegionWidth ;
    ctx->hwCusorInfo[devNo].gop_srcHeight = ctx->displayRegionHeight;
    ctx->hwCusorInfo[devNo].gop_dstWidth = ctx->curTimingWidth;
    ctx->hwCusorInfo[devNo].gop_dstHeight = ctx->curTimingHeight;

    ctx->hwCusorInfo[devNo].operation |= HWCURSOR_ICON;

    return 0;
}

int gameCursor_setPosition(struct hwc_context_t* ctx, int32_t devNo, float positionX, float positionY)
{
    ctx->hwCusorInfo[devNo].positionX = positionX;
    ctx->hwCusorInfo[devNo].positionY = positionY;
    ctx->hwCusorInfo[devNo].operation |= HWCURSOR_POSITION;
    return 0;
}

int gameCursor_doTransaction(struct hwc_context_t* ctx, int32_t devNo)
{
    if (!ctx->hwCusorInfo[devNo].bInitialized) {
        return 0;
    }

    ctx->hwCusorInfo[devNo].acquireFenceFd = -1;
    ctx->hwCusorInfo[devNo].releaseFenceFd = -1;
    hwcursordev_commitTransaction(devNo);
    return 0;
}
#endif

int hwCursor_redrawFb(struct hwc_context_t* ctx, int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight)
{
    if (!ctx->hwCusorInfo[0].bInitialized) {
        return 0;
    }
    ctx->hwCusorInfo[0].srcAddr = ctx->CursorPhyAddr[0];
    ctx->hwCusorInfo[0].srcX = srcX;
    ctx->hwCusorInfo[0].srcY = srcY;
    ctx->hwCusorInfo[0].srcWidth = srcWidth;
    ctx->hwCusorInfo[0].srcHeight = srcHeight;
    ctx->hwCusorInfo[0].srcStride = stride;
    ctx->hwCusorInfo[0].dstWidth = ALIGN(dstWidth,GWIN_ALIGNMENT);
    ctx->hwCusorInfo[0].dstHeight = dstHeight;
    ctx->hwCusorInfo[0].gop_srcWidth = ctx->displayRegionWidth ;
    ctx->hwCusorInfo[0].gop_srcHeight = ctx->displayRegionHeight;
    ctx->hwCusorInfo[0].gop_dstWidth = ctx->curTimingWidth;
    ctx->hwCusorInfo[0].gop_dstHeight = ctx->curTimingHeight;

    ctx->hwCusorInfo[0].operation |= HWCURSOR_ICON;

    //ALOGD("hwCursor_redrawFb:[%d,%d, %d,%d,%d,%d,%d], pid=%d, tid=%d", srcX,srcY,srcWidth,srcHeight,stride,dstWidth,dstHeight,getpid(),gettid());
    return 0;
}

int hwCursor_display(hwc_context_t* ctx) {
    HwCursorInfo_t &hwcursorinfo = ctx->hwCusorInfo[0];
    HwCursorInfo_t &lastcursorinfo = ctx->lastHwCusorInfo[0];
    if (hwcursorinfo.layerIndex < 0) {
        if (lastcursorinfo.layerIndex >= 0) {
            //close gwin
            if (hwcursorinfo.operation & HWCURSOR_SHOW) {
                hwcursorinfo.operation &=~HWCURSOR_SHOW;
            }
            hwcursorinfo.operation |= HWCURSOR_HIDE;
        } else {
            return 0;
        }
    } else {
        if (lastcursorinfo.layerIndex < 0) {
            //open gwin
            if (hwcursorinfo.operation & HWCURSOR_HIDE) {
                hwcursorinfo.operation &=~HWCURSOR_HIDE;
            }
            hwcursorinfo.operation |= HWCURSOR_SHOW;
        }
    }
    if (hwcursorinfo.operation) {
        hwcursordev_commitTransaction(0);
    }
    return 0;

}
int hwCursor_setCursorPositionAsync(hwc_context_t* ctx, int x_pos, int y_pos) {
    if ((!ctx->hwCusorInfo[0].bInitialized)||(ctx->gameCursorEnabled)) {
        return 0;
    }
    ctx->hwCusorInfo[0].positionX = x_pos;
    ctx->hwCusorInfo[0].positionY = y_pos;
    ctx->hwCusorInfo[0].operation |= HWCURSOR_POSITION;

    hwcursordev_commitTransaction(0);
    return 0;
}
int hwCursor_set_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent) {
    int result = android::NO_ERROR;
    HwCursorInfo_t &hwcursorinfo = ctx->hwCusorInfo[0];
    HwCursorInfo_t &lastcursorinfo = ctx->lastHwCusorInfo[0];
    if (hwcursorinfo.layerIndex >= 0) {
        if (!hwcursorinfo.bInitialized) {
            hwcursordev_init(ctx, 0);
            hwcursorinfo.bInitialized = true;
        }
        hwc_layer_1_t &layer = primaryContent->hwLayers[hwcursorinfo.layerIndex];
        private_handle_t *handle = (private_handle_t *)layer.handle;
        hwcursorinfo.acquireFenceFd = layer.acquireFenceFd;
        hwcursorinfo.planeAlpha = layer.planeAlpha;
        layer.acquireFenceFd = -1;
        if (layer.compositionType == HWC_CURSOR_OVERLAY) {
            //check timing changed
            if ((lastcursorinfo.gop_dstWidth!= ctx->curTimingWidth) ||
               (lastcursorinfo.gop_dstHeight != ctx->curTimingHeight) ||
               (lastcursorinfo.gop_srcWidth != ctx->displayRegionWidth) ||
               (lastcursorinfo.gop_srcHeight != ctx->displayRegionHeight) ||
               (ctx->hwcursor_fbNeedRedraw)) {
                if (handle->size <= HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4) {
                    memcpy((void*)ctx->CursorVaddr[0], handle->base, handle->size);
                }
                ctx->hwCusorInfo[0].srcAddr = ctx->CursorPhyAddr[0];
                ctx->hwCusorInfo[0].srcX = (int)layer.sourceCropf.left;
                ctx->hwCusorInfo[0].srcY = (int)layer.sourceCropf.top;
                ctx->hwCusorInfo[0].srcWidth = (int)layer.sourceCropf.right - (int)layer.sourceCropf.left;
                ctx->hwCusorInfo[0].srcHeight = (int)layer.sourceCropf.bottom - (int)layer.sourceCropf.top;
                ctx->hwCusorInfo[0].dstWidth = ALIGN((layer.displayFrame.right - layer.displayFrame.left),GWIN_ALIGNMENT);
                ctx->hwCusorInfo[0].dstHeight = layer.displayFrame.bottom - layer.displayFrame.top;
                ctx->hwCusorInfo[0].srcStride = handle->byte_stride;

                ctx->hwCusorInfo[0].gop_srcWidth = ctx->displayRegionWidth ;
                ctx->hwCusorInfo[0].gop_srcHeight = ctx->displayRegionHeight;
                ctx->hwCusorInfo[0].gop_dstWidth = ctx->curTimingWidth;
                ctx->hwCusorInfo[0].gop_dstHeight = ctx->curTimingHeight;
                ctx->hwCusorInfo[0].positionX = layer.displayFrame.left;
                ctx->hwCusorInfo[0].positionY = layer.displayFrame.top;
                ctx->hwCusorInfo[0].operation |= HWCURSOR_ICON;
            }
       }
    }

    hwcursorinfo.releaseFenceFd = -1;

    result = hwCursor_display(ctx);  // work thread will signal releaseFence in last composition

    if (result == android::NO_ERROR) {
        if (hwcursorinfo.layerIndex >= 0) {
            hwc_layer_1_t &layer = primaryContent->hwLayers[hwcursorinfo.layerIndex];
            layer.releaseFenceFd = hwcursorinfo.releaseFenceFd;
        }
    }

    return result;
}

bool layerSupportCursorOverlay(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent,hwc_layer_1_t &layer, size_t i) {
    char property[PROPERTY_VALUE_MAX] = {0};
    if (!(layer.flags & HWC_IS_CURSOR_LAYER)) {
        return false;
    }

    private_handle_t *handle = (private_handle_t *)layer.handle;
    if (!handle) {
        ALOGW("layer %u: handle is NULL", i);
        return false;
    }

    // format
    if ( handle->format != HAL_PIXEL_FORMAT_RGBA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_BGRA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_RGBX_8888 &&
         handle->format != HAL_PIXEL_FORMAT_RGB_565 ) {
        ALOGV("\tlayer %u: format not supported", i);
        return false;
    }
    // transform
    if (layer.transform != 0) {
        ALOGV("layer %u: has rotation", i);
        return false;
    }

    unsigned int dst_width = layer.displayFrame.right - layer.displayFrame.left;
    unsigned int dst_height = layer.displayFrame.bottom - layer.displayFrame.top;
    if ((dst_width > 128) || (dst_height > 128)) {
        ALOGV("layer dispframe w=%d, h=%d", dst_width, dst_height);
        return false;
    }

    if (ctx->gameCursorEnabled) {
        return false;
    }

    if (property_get("mstar.desk-enable-hwcursor", property, "1") > 0) {
         if (0 == atoi(property)) {
            return false;
        }
    }

    //check STR suspend
    if (property_get("mstar.str.suspending", property, "0") > 0) {
        if (atoi(property) != 0) {
            return false;
        }
    }

    if (property_get("mstar.videoadvert.finished", property, "1") > 0) {
         if (0 == atoi(property)) {
            return false;
        }
    }
    // check 3D mode
    if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
        if (DISPLAYMODE_NORMAL_FP == atoi(property)) {
            return false;
        }
    }
    return true;
}

bool checkCursorFbNeedRedraw(hwc_context_t* ctx,hwc_display_contents_1_t *primaryContent) {
    LayerCache &LastFrameLayersInfo = ctx->lastFrameLayersInfo;
    // HwCursorInfo_t &lastcursorinfo = ctx->lastHwCusorInfo[0];
    bool bCursorCountDirty = false;
    bool bhndDirty = false;
    bool bCropDirty = false;
    bool bFrameDirty = false;
    bool bAlphaDirty = false;
    bool bBlendingDirty = false;
    bool bCursorFbNeedRedraw = false;

    for (int i=primaryContent->numHwLayers-2; i>=0 && i<MAX_NUM_LAYERS; i--) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];
        if (layer.compositionType == HWC_CURSOR_OVERLAY) {
            if (LastFrameLayersInfo.cursorLayerCount == 0) {
                bCursorCountDirty = true;
            } else {
                for (int j=LastFrameLayersInfo.layerCount-1; j>=0; j--) {
                    if(LastFrameLayersInfo.bCursorLayer[j]) {
                        private_handle_t *hnd = (private_handle_t*)layer.handle;
                        if (hnd != LastFrameLayersInfo.hnd[j]) {
                          bhndDirty =  true;
                        }
#if 0
                        if (!rect_eq(layer.sourceCrop,LastFrameLayersInfo.sourceCrop[j])) {
                          bCropDirty = true;
                        }

#endif
                        if (layer.planeAlpha != LastFrameLayersInfo.planeAlpha[j]) {
                          bAlphaDirty = true;
                        }

                        if (layer.blending != LastFrameLayersInfo.blending[j]) {
                          bBlendingDirty = true;
                        }
                        break;
                    }
                }
            }

            if (bCursorCountDirty || bhndDirty || bCropDirty || bAlphaDirty || bBlendingDirty || bFrameDirty) {
              bCursorFbNeedRedraw = true;
            }
            if (ctx->b3DModeChanged) {
              bCursorFbNeedRedraw = true;
            }
            if (ctx->gameCursorChanged) {
              bCursorFbNeedRedraw = true;
              ctx->gameCursorChanged= false;
            }
            //ALOGD("check cursor redraw: %d,%d,%d,%d,%d,%d,%d",bCursorCountDirty,bhndDirty,bCropDirty, bAlphaDirty, bBlendingDirty, bFrameDirty,bCursorFbNeedRedraw);
        }
    }
    return bCursorFbNeedRedraw;
}
