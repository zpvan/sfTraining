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

#include "hwcomposer_platform.h"

/* android header */
#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>

#include <hardware/hwcomposer.h>

#include <utils/Errors.h>
#include <gralloc_priv.h>
#include <sync/sync.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <assert.h>
#include <stdlib.h>

#include <utils/Timers.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#include <apiXC.h>
#include <apiGOP.h>

#include "hwcomposerMMVideo.h"
#include "hwcomposerTVVideo.h"
#include "hwcomposerGraphic.h"
#include "hwcomposer_util.h"
#ifdef ENABLE_HWC14_CURSOR
#include "hwcomposer_cursor.h"
#endif
extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                               const struct timespec *request,
                               struct timespec *remain);

//gflip ioctl
#define MDRV_GFLIP_IOC_MAGIC       ('2')
#define MDRV_GFLIP_IOC_GETVSYNC_NR 20
#define MDRV_GFLIP_IOC_GETVSYNC    _IOWR(MDRV_GFLIP_IOC_MAGIC,MDRV_GFLIP_IOC_GETVSYNC_NR,unsigned int)

// lock between main and vsync thread
static pthread_mutex_t vsync_mutex;
// protected by vsync_mutex
static bool vsync_enabled = false;
static int vsync_gop = 2; // this is dirty
static nsecs_t next_fake_vsync = 0;
static int lastOsdDisableState = 0;

int platform_prepare_external(hwc_context_t* ctx, hwc_display_contents_1_t *externalContent) {
    if (externalContent && externalContent->numHwLayers > 0) {
        for (int i=(externalContent->numHwLayers-1); i>=0; i--) {
            hwc_layer_1_t &layer = externalContent->hwLayers[i];
            layer.compositionType = HWC_FRAMEBUFFER;
        }
    }
    return 0;
}

int platform_set_external(hwc_context_t* ctx, hwc_display_contents_1_t *externalContent) {
    return 0;
}

int platform_prepare_virtual(hwc_context_t* ctx, hwc_display_contents_1_t *virtualContent) {

     if (virtualContent && virtualContent->numHwLayers > 0) {
        for (int i=(virtualContent->numHwLayers-1); i>=0; i--) {
            hwc_layer_1_t &layer = virtualContent->hwLayers[i];
            if (layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
                layer.compositionType = HWC_FRAMEBUFFER;
            }
        }
    }
    return 0;
}

int platform_set_virtual(hwc_context_t* ctx, hwc_display_contents_1_t *virtualContent) {
    if (virtualContent != NULL) {
        if (virtualContent->outbufAcquireFenceFd != -1) {
            sync_wait(virtualContent->outbufAcquireFenceFd, 1000);
            close(virtualContent->outbufAcquireFenceFd);
            virtualContent->outbufAcquireFenceFd = -1;
        }
        for (unsigned int i=0;i<virtualContent->numHwLayers;i++) {
            hwc_layer_1_t &layer = virtualContent->hwLayers[i];
            if (layer.acquireFenceFd!=-1) {
                sync_wait(layer.acquireFenceFd, 1000);
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
            }
        }
    }
    return 0;
}

int platform_prepare_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent) {
    int videoLayerNumber = 0;
    int videoLayerIndex[MAX_OVERLAY_LAYER_NUM];

    for (unsigned int i=0;i<MAX_OVERLAY_LAYER_NUM;i++) {
        videoLayerIndex[i] = -1;
    }

    //determin layer's compostion type,
    graphic_prepare_primary(ctx, primaryContent, videoLayerNumber, videoLayerIndex);

    mm_video_prepare_primary(ctx,primaryContent,videoLayerIndex,videoLayerNumber);

    tv_video_prepare_primary(ctx,primaryContent,videoLayerIndex,videoLayerNumber);

    return 0;
}

int platform_set_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent) {
#ifdef ENABLE_HWC14_CURSOR
    hwCursor_set_primary(ctx, primaryContent);
#endif
    graphic_set_primary(ctx, primaryContent);
    return 0;
}
int platform_init_primary(hwc_context_t* ctx) {

    graphic_init_primary(ctx);

    mm_video_init_primary(ctx);

    pthread_mutex_init(&vsync_mutex, NULL);
    vsync_enabled = false;

    /* initialize the gralloc */
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&ctx->gralloc);

    return 0;
}

int platform_close_primary(hwc_context_t* ctx) {

    graphic_close_primary(ctx);
    mm_video_close_primary(ctx);
    tv_video_close_primary(ctx);

    return 0;
}

int platform_eventControl(hwc_context_t* dev, int disp, int event, int enabled) {
    switch (event) {
        case HWC_EVENT_VSYNC:
            if (enabled != 0 && enabled != 1) {
                return -EINVAL;
            } else {
                pthread_mutex_lock(&vsync_mutex);
                vsync_enabled = enabled;
                pthread_mutex_unlock(&vsync_mutex);
                // ALOGI("toggle hw vsync %d", enabled);
                return 0;
            }
    }
    return -EINVAL;
}
void platform_dump(struct hwc_context_t* ctx, char *buff, int buff_len) {
    android::String8 result;
    static const char *LayerTypeToString[8] ={"Normal_Layer","MM_OVERLAY","TV_OVERLAY","NW_OVERLAY","GOP_OVERLAY","FrameBufferTarget", "HWCURSOR","SIDEBAND"};
    static const char *SidebandTypeToString[5] ={"invalid","SIDEBAND_TV","SIDEBAND_MM","SIDEBAND_MM_XC","SIDEBAND_MM_GOP"};
    long vsync_period = int64_t(1e9 * 100 / MApi_XC_GetOutputVFreqX100());
    result.appendFormat("vsync_period=%ld width=%d height=%d xdpi=%d ydpi=%d\n",
                        vsync_period, ctx->width, ctx->height, ctx->xdpi, ctx->ydpi);
    result.appendFormat("curTimingWidth=%d curTimingHeight=%d\n", ctx->curTimingWidth, ctx->curTimingHeight);
    result.appendFormat("panelWidth=%d panelHeight=%d panelHStart=%d\n",
                        ctx->panelWidth, ctx->panelHeight, ctx->panelHStart);
    for (int j=0; j<ctx->gopCount; j++) {
        result.appendFormat("window %d: gop=%d gwin=%d miu=%d physicalAddr=%lx fbid=%x bufferW=%d bufferH=%d \
                format=%d stride=%d layerindex=%d bFrameBufferTarget=%d bEnableGopAFBC = %d \
                srcx=%d srcY=%d srcWidth=%d srcHeight=%d dstX=%d dstY=%d dstWidth=%d dstHeight=%d\n",
                            j, ctx->lastHWWindowInfo[j].gop, ctx->lastHWWindowInfo[j].gwin,
                            ctx->lastHWWindowInfo[j].channel, (unsigned long)ctx->lastHWWindowInfo[j].physicalAddr,
                            ctx->lastHWWindowInfo[j].fbid,
                            ctx->lastHWWindowInfo[j].bufferWidth, ctx->lastHWWindowInfo[j].bufferHeight,
                            ctx->lastHWWindowInfo[j].format, ctx->lastHWWindowInfo[j].stride,
                            ctx->lastHWWindowInfo[j].layerIndex,
                            ctx->lastHWWindowInfo[j].bFrameBufferTarget,
                            ctx->lastHWWindowInfo[j].bEnableGopAFBC,
                            ctx->lastHWWindowInfo[j].srcX,ctx->lastHWWindowInfo[j].srcY,
                            ctx->lastHWWindowInfo[j].srcWidth,ctx->lastHWWindowInfo[j].srcHeight,
                            ctx->lastHWWindowInfo[j].dstX,ctx->lastHWWindowInfo[j].dstY,
                            ctx->lastHWWindowInfo[j].dstWidth,ctx->lastHWWindowInfo[j].dstHeight);
    }

#ifdef ENABLE_HWC14_CURSOR
    result.appendFormat("hwcursor: gop=%d gwin=%d miu=%d srcAddr=%llx fbid=%d stride=%d layerindex=%d \
    positionX=%d positionY=%d opration=%x strethwindow[%d,%d,%d,%d]\n",
                    ctx->lastHwCusorInfo[0].gop, ctx->lastHwCusorInfo[0].gwin,
                    ctx->lastHwCusorInfo[0].channel, (long long unsigned int)ctx->lastHwCusorInfo[0].srcAddr,
                    ctx->lastHwCusorInfo[0].fbid, 128*4, ctx->lastHwCusorInfo[0].layerIndex,
                    ctx->lastHwCusorInfo[0].positionX,ctx->lastHwCusorInfo[0].positionY,ctx->lastHwCusorInfo[0].operation,
                    ctx->lastHwCusorInfo[0].gop_srcWidth,ctx->lastHwCusorInfo[0].gop_srcHeight,
                    ctx->lastHwCusorInfo[0].gop_dstWidth,ctx->lastHwCusorInfo[0].gop_dstHeight);


#endif
    for (unsigned int j=0; j<(ctx->debug_LayerCount-1) && j<MAX_DEBUG_LAYERCOUNT; j++) {
        if (ctx->debug_LayerAttr[j].layerType == SIDEBAND_LAYER) {
            result.appendFormat("layer %d :layer type is %s ,sideband type is %s ,memType is %s\n",
                j,LayerTypeToString[ctx->debug_LayerAttr[j].layerType],
                sidebandType2String(ctx->debug_LayerAttr[j].sidebandType),
                memType2String((ctx->debug_LayerAttr[j].memType&mem_type_allocated_mask)>>16));
        } else {
            result.appendFormat("layer %d :layer type is %s memType is %s\n",j,LayerTypeToString[ctx->debug_LayerAttr[j].layerType],memType2String((ctx->debug_LayerAttr[j].memType&mem_type_allocated_mask)>>16));
        }
    }
    strlcpy(buff, result.string(), buff_len);
}

int platform_getCurOPTiming(hwc_context_t* ctx, int *ret_width, int *ret_height, int *ret_hstart) {
    if(ret_width != NULL) {
        *ret_width  = ctx->curTimingWidth;
    }
    if(ret_height != NULL) {
        *ret_height = ctx->curTimingHeight;
    }
    if(ret_hstart != NULL) {
        *ret_hstart = ctx->panelHStart;
    }
    return 0;//graphic_getCurOPTiming(ret_width,ret_height,ret_hstart);
}

int platform_recalc3DTransform(hwc_context_t* ctx, hwc_3d_param_t* params, hwc_rect_t frame) {
    return graphic_recalc3DTransform(ctx,params,frame);
}

int64_t platform_getRefreshPeriod(hwc_context_t* ctx) {
    char values[PROPERTY_VALUE_MAX] = {0};
    if (property_get("mstar.override.refresh.rate", values, NULL) > 0) {
        int overridefps = atoi(values);
        return int64_t(1e9 / overridefps);
    }
    int fps = MApi_XC_GetOutputVFreqX100() / 100;
    unsigned int gop_dest = graphic_getGOPDestination(ctx);
#ifdef MSTAR_MASERATI
    if(gop_dest == E_GOP_DST_OP_DUAL_RATE)
        fps *= 2;
    if ((fps > 120) || (fps < 24)) {
#else
    if ((fps > 60) || (fps < 24)) {
#endif
        ALOGD("the fps get from MApi_XC_GetOutputVFreqX100 is abnormal %d,force the fps to 60",fps);
        fps = 60;
    }
    return int64_t(1e9 / fps);
}

int platform_isStbTarget(hwc_context_t* ctx) {
    return ctx->stbTarget;
}

int platform_captureScreen(hwc_context_t* ctx, int* width, int* height, int withOsd) {
    return graphic_startCapture(ctx, width, height, withOsd);
}

int platform_captureScreenWithClip(hwc_context_t* ctx, size_t numData, hwc_capture_data_t* captureData,int* width, int* height, int withOsd) {
    return graphic_captureScreenWithClip(ctx, numData, captureData, width, height, withOsd);
}

int platform_releaseCaptureScreenMemory(hwc_context_t* ctx) {
    return graphic_releaseCaptureScreenMemory(ctx);
}

int platform_getMirrorMode(hwc_context_t* ctx, int* HMirror, int* VMirror) {
    *HMirror = ctx->mirrorModeH;
    *VMirror = ctx->mirrorModeV;
    return 0;
}

int platform_isAFBCFramebuffer(hwc_context_t* ctx) {
    return ctx->bAFBCFramebuffer;
}

bool platform_isRGBColorSpace(hwc_context_t* ctx) {
    return ctx->bRGBColorSpace;
}


#ifdef ENABLE_HWC14_CURSOR
int platform_setCursorPositionAsync(hwc_context_t* ctx, int x_pos, int y_pos) {
    return hwCursor_setCursorPositionAsync(ctx, x_pos, y_pos);
}
#endif

void* platform_event_thread(void *data) {
    struct hwc_context_t* ctx = (struct hwc_context_t*)data;
    int old3DMode = DISPLAYMODE_NORMAL;

    // TODO: add hw vsync, virtual display hotplug
    const char* gflip_path = "/dev/gflip";

    char thread_name[64] = "hwcVsyncThread";
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    nsecs_t cur_timestamp=0;
    int fd_vsync = -1;
    bool fakevsync = false;
    int fakefreq = 60;
    char property[PROPERTY_VALUE_MAX] = {0};
    int sdr2hdrEnable = 0;
    int r2yEnable = 0;

    // TODO: hw vsync have some problem,use sw vsync recently
    property_get("mstar.lvds-off", property, "false");
    if (strcmp(property, "true") == 0) {
        ctx->vsync_off = true;
        ALOGD("mstar.lvds-off is true, so gop forcewrite will be enable!");
    } else {
        ctx->vsync_off = false;
    }

    fd_vsync = open(gflip_path, O_RDWR);
    if (fd_vsync < 0) {
        ALOGE("FATAL:%s:not able to open file:%s, %s",  __FUNCTION__,
              gflip_path, strerror(errno));
        fakevsync = true;
    }

    // avoid ctx->procs->invalidate run too early ?
    // usleep(1000*1000);
    vsync_gop = ctx->HWWindowInfo[0].gop;
    while (true) {
        bool enabled;
        pthread_mutex_lock(&vsync_mutex);
        enabled = vsync_enabled;
        pthread_mutex_unlock(&vsync_mutex);

        if (enabled) {
            if (!fakevsync) {
#define UNLIKELY(exp) (__builtin_expect((exp) != 0, false))
                if (UNLIKELY(ioctl(fd_vsync, MDRV_GFLIP_IOC_GETVSYNC, &vsync_gop) != 0)) {
                    fakevsync = true;
                    ALOGE("ioctl failed, use fake vsync");
                } else {
                    cur_timestamp = systemTime(CLOCK_MONOTONIC);
                }
            }
            if (fakevsync) {
                cur_timestamp = systemTime(CLOCK_MONOTONIC);
                nsecs_t next_vsync = next_fake_vsync;
                nsecs_t sleep = next_vsync - cur_timestamp;
                if (sleep < 0) {
                    // we missed, find where the next vsync should be
                    sleep = (ctx->vsync_period - ((cur_timestamp - next_vsync) % ctx->vsync_period));
                    next_vsync = cur_timestamp + sleep;
                }
                next_fake_vsync = next_vsync + ctx->vsync_period;

                struct timespec spec;
                spec.tv_sec  = next_vsync / 1000000000;
                spec.tv_nsec = next_vsync % 1000000000;

                int err;
                do {
                    err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
                } while (err<0 && errno == EINTR);

                cur_timestamp = next_vsync;
            }
            // send timestamp to HAL
            // ALOGD("%s: timestamp %llu sent to HWC for %s", __FUNCTION__, cur_timestamp, "gflip");
            ctx->procs->vsync(ctx->procs, HWC_DISPLAY_PRIMARY, cur_timestamp);
        } else {
            usleep(16666);
        }
        // dynamically control vsync frequence
        if (property_get("mstar.fake.vsync.enable", property, "false") > 0) {
            if (strcmp(property, "true") == 0) {
                fakevsync =  true;
            } else {
                fakevsync =  false;
            }
        }
        if (fakevsync) {
            if (property_get("mstar.fakevsync.freq", property, "60") > 0) {
                fakefreq = atoi(property);
                if (ctx->vsync_period != (int32_t)(1000000000.0 / fakefreq)) {
                    ctx->vsync_period = 1000000000.0 / fakefreq;
                    ALOGD("Adjust vsync_period through mstar.fakevsync.freq to %d",fakefreq);
                }
            }
        }
        //check surfaceflinger service whether start running
        if(property_get("mstar.surfaceflinger.running",property, "false") > 0 ) {
            if (strcmp(property, "false") == 0) {
                continue;
            }
        }
        // check whether enable osd
        if (property_get("mstar.disable.osd", property, "0") > 0) {
            int OsdDisableState = atoi(property);
            if (lastOsdDisableState != OsdDisableState) {
                lastOsdDisableState = OsdDisableState;
                ALOGD("mstar.disable.osd state change");
                if (ctx->procs) {
                    ctx->procs->invalidate(ctx->procs);
                }
            }
        }
        // check 3D
        if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
            int new3DMode = atoi(property);
            if (new3DMode != old3DMode) {
#ifndef ENABLE_HWC14_CURSOR
                //hwcursor don't support DISPLAYMODE_NORMAL_FP,so using sw cursor
                static bool rollbackHwcursor = false;
                if ((new3DMode == DISPLAYMODE_NORMAL_FP)
                #ifdef MSTAR_MESSI
                    ||(new3DMode != DISPLAYMODE_NORMAL)
                #endif
                 ) {
                    int enableHwcursor = 0;
                    if (property_get("mstar.desk-enable-hwcursor", property, "1") > 0) {
                        enableHwcursor = atoi(property);
                    }
                    if (enableHwcursor) {
                        rollbackHwcursor = true;
                        property_set("mstar.desk-enable-hwcursor", "0");
                    }
                } else if ((old3DMode == DISPLAYMODE_NORMAL_FP)
                #ifdef MSTAR_MESSI
                        ||(old3DMode != DISPLAYMODE_NORMAL)
                #endif
                ) {
                    if (rollbackHwcursor) {
                        rollbackHwcursor = false;
                        property_set("mstar.desk-enable-hwcursor", "1");
                    }
                }
#endif
                old3DMode = new3DMode;
                ALOGD("the new3DMode is %d",new3DMode);

                for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
                    MS_Video_Disp_Set_Cmd((MS_Win_ID)i, MS_DISP_CMD_UPDATE_DISP_STATUS, NULL);
                }

                if (ctx->procs) {
                    ctx->procs->invalidate(ctx->procs);
                }
            }
        }
#ifdef ENABLE_HWC14_CURSOR
        // check enable game cursor
        if (property_get("mstar.desk-enable-gamecursor", property, "0") > 0) {
            int value = atoi(property);
            if (ctx->gameCursorEnabled != value) {
                ctx->gameCursorEnabled = value;
                ctx->gameCursorChanged = true;
                if (ctx->procs) {
                    ctx->procs->invalidate(ctx->procs);
                    // close gamecursor thread and gwin(devNo1) if disabled
                    if(value == 0) {
                        gameCursor_deinit(ctx, 1);
                    }
                }
            }
        }
#endif
        // check SDR --> HDR and RGB --> YUV
        {
            if (property_get("mstar.enable.sdr2hdr", property, "0") > 0) {
                int enable = atoi(property);
                 if (enable != sdr2hdrEnable) {
                    sdr2hdrEnable = enable;
                    ctx->bFramebufferNeedRedraw = true;
                    if (ctx->procs) {
                        ctx->procs->invalidate(ctx->procs);
                    }
                 }
            }
            if (property_get("mstar.enable.r2y", property, "0") > 0) {
                 int enable = atoi(property);
                 if (enable != r2yEnable) {
                    r2yEnable = enable;
                    ctx->bFramebufferNeedRedraw = true;
                    if (ctx->procs) {
                        ctx->procs->invalidate(ctx->procs);
                    }
                 }
            }
        }
        // check gop dest or timing changed
        unsigned int gop_dest = graphic_getGOPDestination(ctx);
        if(graphic_checkGopTimingChanged(ctx)) {
            if (ctx->procs) {
                ctx->procs->invalidate(ctx->procs);
            }
            if(gop_dest == E_GOP_DST_OP0) {
                mm_video_sizechanged_notify();
            }
        }
        // check whether down scale XC
        mm_video_CheckDownscaleXC();
    }
    return NULL;
}
