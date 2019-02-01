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

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hwcomposer.h>

#include <utils/Errors.h>
#include <gralloc_priv.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <assert.h>
#include <stdlib.h>
#include "hwcomposer_platform.h"
#include "hwcomposer_cursor.h"

/*****************************************************************************/

static int hwc_prepare(hwc_composer_device_1_t *dev,
                       size_t numDisplays, hwc_display_contents_1_t** displays) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int ret = 0;
    if (displays) {
        //platform_prepare_primary(ctx, displays[0]);
        for (int32_t i = numDisplays - 1; i >= 0; i--) {
            hwc_display_contents_1_t *list = displays[i];
            switch(i) {
                case HWC_DISPLAY_PRIMARY:
                    ret = platform_prepare_primary(ctx, list);
                    break;
                case HWC_DISPLAY_EXTERNAL:
                    ret = platform_prepare_external(ctx, list);
                    break;
                case HWC_DISPLAY_VIRTUAL:
                    ret = platform_prepare_virtual(ctx, list);
                    break;
                default:
                    ret = -EINVAL;
            }
        }
    }

    return ret;
}



static int hwc_set(hwc_composer_device_1_t *dev,
                   size_t numDisplays, hwc_display_contents_1_t** displays) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    int ret = 0;
    for (uint32_t i = 0; i < numDisplays; i++) {
        hwc_display_contents_1_t* list = displays[i];
        switch(i) {
            case HWC_DISPLAY_PRIMARY:
                ret = platform_set_primary(ctx, list);
                break;
            case HWC_DISPLAY_EXTERNAL:
                ret = platform_set_external(ctx, list);
                break;
            case HWC_DISPLAY_VIRTUAL:
                ret = platform_set_virtual(ctx, list);
                break;
            default:
                ret = -EINVAL;
        }
    }
    return ret;
}

static int hwc_eventControl(struct hwc_composer_device_1* dev, int disp,
                            int event, int enabled) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_eventControl(ctx, disp, event, enabled);
}

static int hwc_blank(struct hwc_composer_device_1* dev, int disp, int blank) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    ALOGI("TODO: blank/unblank display");

    switch (disp) {
        case HWC_DISPLAY_PRIMARY:
            // TODO: toggle primary display
            break;
        case HWC_DISPLAY_EXTERNAL:
            ALOGE("hwc_blank not support External Display");
            return -EINVAL;
            break;
        case HWC_DISPLAY_VIRTUAL:
                // TODO: toggle virtual display
                break;
        default:
            return -EINVAL;
    }
    return 0;
}

#ifdef ENABLE_HWC14_CURSOR
static int hwc_setPowerMode(struct hwc_composer_device_1* dev, int disp, int mode) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (disp) {
        case HWC_DISPLAY_PRIMARY:
            // TODO: toggle primary display
            break;
        case HWC_DISPLAY_EXTERNAL:
            ALOGE("hwc_setPowerMode not support External Display");
            return -EINVAL;
            break;
        case HWC_DISPLAY_VIRTUAL:
                // TODO: toggle virtual display
                break;
        default:
            return -EINVAL;
    }
    return 0;
}
int hwc_setActiveConfig(struct hwc_composer_device_1* dev, int disp, int index) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (disp) {
        case HWC_DISPLAY_PRIMARY:
            // TODO: toggle primary display
            break;
        case HWC_DISPLAY_EXTERNAL:
            // TODO: toggle primary display
            break;
        case HWC_DISPLAY_VIRTUAL:
            // TODO: toggle virtual display
            break;
        default:
            return -EINVAL;
    }
    return 0;
}
#endif
static int hwc_query(struct hwc_composer_device_1* dev, int what, int* value) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (what) {
        case HWC_BACKGROUND_LAYER_SUPPORTED:
            // we don't support the background layer yet
            value[0] = 0;
            break;
        case HWC_VSYNC_PERIOD:
            /*
             * Returns the vsync period in nanoseconds.
             *
             * This query is not used for HWC_DEVICE_API_VERSION_1_1 and later.
             * Instead, the per-display attribute HWC_DISPLAY_VSYNC_PERIOD is used.
             */
            value[0] = 1000000000.0 / 60;
            break;
        case HWC_DISPLAY_TYPES_SUPPORTED:
            // TODO: add virtual display
            value[0] = HWC_DISPLAY_PRIMARY_BIT;
            break;
        default:
            // unsupported query
            return -EINVAL;
    }
    return 0;
}

static void *hwc_event_thread(void *data) {
    return platform_event_thread(data);
}

static void hwc_registerProcs(struct hwc_composer_device_1* dev,
                              hwc_procs_t const* procs) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if (ctx->procs == NULL && procs != NULL) { // only start one thread
        ctx->procs = procs;
        int ret = pthread_create(&ctx->vsync_thread, NULL, hwc_event_thread, ctx);
        if (ret) {
            ALOGE("failed to start event thread: %s", strerror(ret));
        }
    } else {
        ALOGE("registerProcs failed or called twice!");
    }
}

static void hwc_dump(struct hwc_composer_device_1* dev, char *buff, int buff_len) {
    if (buff_len <= 0)
        return;
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    platform_dump(ctx, buff, buff_len);
}

static int hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp,
                                 uint32_t* configs, size_t* numConfigs) {
    if (*numConfigs == 0)
        return 0;
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    if (disp == HWC_DISPLAY_PRIMARY) {
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    } else if (disp == HWC_DISPLAY_EXTERNAL) {
        ALOGW("hwc_getDisplayConfigs not support External Display");
        return -EINVAL;
    } else if (disp == HWC_DISPLAY_VIRTUAL) {
        // TODO: connected?
        bool connected = false;
        if (connected) {
            configs[0] = 0;
            *numConfigs = 1;
            return 0;
        } else {
            return -EINVAL;
        }
    }
    return -EINVAL;
}

static int32_t platform_primary_attribute(hwc_context_t* ctx, const uint32_t attribute) {
    switch (attribute) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            return platform_getRefreshPeriod(ctx);
        case HWC_DISPLAY_WIDTH:
            return ctx->width;
        case HWC_DISPLAY_HEIGHT:
            return ctx->height;
        case HWC_DISPLAY_DPI_X:
            return ctx->xdpi;
        case HWC_DISPLAY_DPI_Y:
            return ctx->ydpi;
        case HWC_DISPLAY_OSDWIDTH:
            return ctx->osdWidth;
        case HWC_DISPLAY_OSDHEIGHT:
            return ctx->osdHeight;
        default:
            ALOGE("unknown display attribute %u", attribute);
            return -EINVAL;
    }
}

static int hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
                                    uint32_t config, const uint32_t* attributes, int32_t* values) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    switch (disp) {
        case HWC_DISPLAY_PRIMARY:
            for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
                values[i] = platform_primary_attribute(ctx, attributes[i]);
            }
            break;
        case HWC_DISPLAY_EXTERNAL:
            ALOGE("hwc_getDisplayAttributes not support External Display");
            return -EINVAL;
            break;
        case HWC_DISPLAY_VIRTUAL:
            // TODO: get virtal display attributes
            ALOGE("hwc_getDisplayAttributes not support Virtual Display");
            return -EINVAL;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static int hwc_device_close(struct hw_device_t *dev) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    // TODO: signal thread, join
//    pthread_kill(ctx->vsync_thread, SIGTERM);
//    pthread_join(ctx->vsync_thread, NULL);
    platform_close_primary(ctx);
    if (ctx) {
        free(ctx);
    }
    return 0;
}

static int64_t hwc_getRefreshPeriod(struct hwc_composer_device_1* dev) {
    // should call from the hwcomposer_platform.cpp
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_getRefreshPeriod(ctx);
}

static int hwc_getCurtiming(struct hwc_composer_device_1* dev,int *width, int *height) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_getCurOPTiming(ctx,width, height, NULL);
}

static int hwc_recalc3DTransform(struct hwc_composer_device_1* dev, hwc_3d_param_t* params, hwc_rect_t frame) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_recalc3DTransform(ctx,params,frame);
}

static int hwc_isStbTarget(struct hwc_composer_device_1* dev) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_isStbTarget(ctx);
}

static void hwc_captureScreen(struct hwc_composer_device_1* dev, uintptr_t* addr, int* width, int* height, int withOsd) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if(platform_captureScreen(ctx, width, height, withOsd)) {
        *addr = ctx->captureScreenAddr;
    } else {
        *addr = 0;
    }
}

static void hwc_captureScreenWithClip(struct hwc_composer_device_1* dev,
                                                    size_t numData, hwc_capture_data_t* captureData,
                                                    uintptr_t* addr, int* width, int* height, int withOsd) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if (platform_captureScreenWithClip(ctx, numData, captureData, width, height, withOsd)) {
        *addr = ctx->captureScreenAddr;
    } else {
        *addr = 0;
    }
}

static int hwc_releaseCaptureScreenMemory(struct hwc_composer_device_1* dev) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_releaseCaptureScreenMemory(ctx);
}

static int hwc_getMirrorMode(struct hwc_composer_device_1* dev, int* HMirror, int* VMirror) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_getMirrorMode(ctx,HMirror,VMirror);
}

static int hwc_isAFBCFramebuffer(struct hwc_composer_device_1* dev) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_isAFBCFramebuffer(ctx);
}


static int hwc_bRGBColorSpace(struct hwc_composer_device_1* dev) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return platform_isRGBColorSpace(ctx);
}

static int hwc_setCursorPositionAsync(struct hwc_composer_device_1 *dev, int disp, int x_pos, int y_pos) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    switch (disp) {
    case HWC_DISPLAY_PRIMARY:
        platform_setCursorPositionAsync(ctx, x_pos, y_pos);
        break;
    case HWC_DISPLAY_EXTERNAL:
    case HWC_DISPLAY_VIRTUAL:
        ALOGE("hwc_setCursorPositionAsync not support");
        return -EINVAL;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static int hwc_redrawHwCursorFb(struct hwc_composer_device_1 *dev,int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return hwCursor_redrawFb(ctx, srcX, srcY, srcWidth, srcHeight, stride, dstWidth, dstHeight);
}

#ifdef ENABLE_GAMECURSOR
#ifdef ENABLE_DOUBLEGAMECURSOR
static int hwc_gameCursorShowByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_show(ctx, devNo);
}

static int hwc_gameCursorHideByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_hide(ctx, devNo);
}

static int hwc_gameCursorSetAlphaByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo, float alpha)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_setAlpha(ctx, devNo, alpha);
}

static int hwc_gameCursorRedrawFbByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo, int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return gameCursor_redrawFb(ctx, devNo, srcX, srcY, srcWidth, srcHeight, stride, dstWidth, dstHeight);
}

static int hwc_gameCursorSetPositionByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo, float positionX, float positionY)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_setPosition(ctx, devNo, positionX, positionY);
}

static int hwc_gameCursorDoTransactionByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return gameCursor_doTransaction(ctx, devNo);
}

static int hwc_gameCursorGetFbVaddrByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo, intptr_t* vaddr)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if (!(ctx->hwCusorInfo[devNo].bInitialized)) {
        ALOGE("hwcursor module not init!!!");
        *vaddr = 0;
        return -1;
    }
    *vaddr = ctx->CursorVaddr[devNo];
    return 0;
}

static int hwc_gameCursorInitByDevNo(struct hwc_composer_device_1 *dev, int32_t devNo)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return gameCursor_init(ctx, devNo);
}
#endif
static int hwc_gameCursorShow(struct hwc_composer_device_1 *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_show(ctx, 0);
}

static int hwc_gameCursorHide(struct hwc_composer_device_1 *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_hide(ctx, 0);
}

static int hwc_gameCursorSetAlpha(struct hwc_composer_device_1 *dev, float alpha)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_setAlpha(ctx, 0, alpha);
}

static int hwc_gameCursorRedrawFb(struct hwc_composer_device_1 *dev, int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return gameCursor_redrawFb(ctx, 0, srcX, srcY, srcWidth, srcHeight, stride, dstWidth, dstHeight);
}

static int hwc_gameCursorSetPosition(struct hwc_composer_device_1 *dev, float positionX, float positionY)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    return gameCursor_setPosition(ctx, 0, positionX, positionY);
}

static int hwc_gameCursorDoTransaction(struct hwc_composer_device_1 *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return gameCursor_doTransaction(ctx, 0);
}

static int hwc_gameCursorGetFbVaddr(struct hwc_composer_device_1 *dev, intptr_t* vaddr)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    if (!(ctx->hwCusorInfo[0].bInitialized)) {
        ALOGE("hwcursor module not init!!!");
        *vaddr = 0;
        return -1;
    }
    *vaddr = ctx->CursorVaddr[0];
    return 0;
}

static int hwc_gameCursorInit(struct hwc_composer_device_1 *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;
    return gameCursor_init(ctx, 0);
}
#endif

/*****************************************************************************/
static int hwc_device_open(const struct hw_module_t* module, const char* name,
                           struct hw_device_t** device) {
    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        return -EINVAL;
    }
    struct hwc_context_t *dev;
    dev = (hwc_context_t*)malloc(sizeof(*dev));

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->device.common.tag = HARDWARE_DEVICE_TAG;
#ifdef ENABLE_HWC14_CURSOR
    dev->device.common.version = HWC_DEVICE_API_VERSION_1_4;
#else
    dev->device.common.version = HWC_DEVICE_API_VERSION_1_3;
#endif
    dev->device.common.module = const_cast<hw_module_t*>(module);
    dev->device.common.close = hwc_device_close;

    dev->device.prepare         = hwc_prepare;
    dev->device.set             = hwc_set;
    dev->device.eventControl    = hwc_eventControl;
#ifdef ENABLE_HWC14_CURSOR
    dev->device.setPowerMode    = hwc_setPowerMode;
    dev->device.setActiveConfig = hwc_setActiveConfig;
#else
    dev->device.blank           = hwc_blank;
#endif
    dev->device.query           = hwc_query;
    dev->device.registerProcs   = hwc_registerProcs;
    dev->device.dump            = hwc_dump;
    dev->device.getDisplayConfigs = hwc_getDisplayConfigs;
    dev->device.getDisplayAttributes = hwc_getDisplayAttributes;
    dev->device.getRefreshPeriod = hwc_getRefreshPeriod;
    dev->device.getCurOpTiming = hwc_getCurtiming;
    dev->device.recalc3DTransform = hwc_recalc3DTransform;
    dev->device.isStbTarget = hwc_isStbTarget;
    dev->device.captureScreen = hwc_captureScreen;
    dev->device.captureScreenWithClip = hwc_captureScreenWithClip;
    dev->device.releaseCaptureScreenMemory = hwc_releaseCaptureScreenMemory;
    dev->device.getMirrorMode = hwc_getMirrorMode;
    dev->device.bRGBColorSpace = hwc_bRGBColorSpace;
#ifdef MALI_AFBC_GRALLOC
    dev->device.isAFBCFramebuffer = hwc_isAFBCFramebuffer;
#endif
#ifdef ENABLE_HWC14_CURSOR
    dev->device.setCursorPositionAsync = hwc_setCursorPositionAsync;
    dev->device.redrawHwCursorFb = hwc_redrawHwCursorFb;
#endif
#ifdef ENABLE_GAMECURSOR
#ifdef ENABLE_DOUBLEGAMECURSOR
    dev->device.setGameCursorShowByDevNo = hwc_gameCursorShowByDevNo;
    dev->device.setGameCursorHideByDevNo = hwc_gameCursorHideByDevNo;
    dev->device.setGameCursorAlphaByDevNo = hwc_gameCursorSetAlphaByDevNo;
    dev->device.redrawGameCursorFbByDevNo = hwc_gameCursorRedrawFbByDevNo;
    dev->device.setGameCursorPositionByDevNo = hwc_gameCursorSetPositionByDevNo;
    dev->device.doGameCursorTransactionByDevNo = hwc_gameCursorDoTransactionByDevNo;
    dev->device.getGameCursorFbVaddrByDevNo = hwc_gameCursorGetFbVaddrByDevNo;
    dev->device.initGameCursorByDevNo = hwc_gameCursorInitByDevNo;
#endif
    dev->device.setGameCursorShow = hwc_gameCursorShow;
    dev->device.setGameCursorHide = hwc_gameCursorHide;
    dev->device.setGameCursorAlpha = hwc_gameCursorSetAlpha;
    dev->device.redrawGameCursorFb = hwc_gameCursorRedrawFb;
    dev->device.setGameCursorPosition = hwc_gameCursorSetPosition;
    dev->device.doGameCursorTransaction = hwc_gameCursorDoTransaction;
    dev->device.getGameCursorFbVaddr = hwc_gameCursorGetFbVaddr;
    dev->device.initGameCursor = hwc_gameCursorInit;
#endif

    platform_init_primary(dev);

    *device = &dev->device.common;
    return 0;
}

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: HWC_MODULE_API_VERSION_0_1,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: HWC_HARDWARE_MODULE_ID,
        name: "MStar hwcomposer module",
        author: "MStar",
        methods: &hwc_module_methods,
        dso: NULL,
        reserved: { 0 },
    }
};
