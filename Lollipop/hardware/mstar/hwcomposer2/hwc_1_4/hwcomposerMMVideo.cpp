//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2016 MStar Semiconductor, Inc. All rights reserved.
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

#include <utils/Timers.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <cutils/properties.h>

/* mstar header */
#include <iniparser.h>
#include <mmap.h>
#include <drvSYS.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <drvMMIO.h>
#include <drvSEM.h>
#include <drvGPIO.h>
#include <drvXC_IOPort.h>
#include <apiXC.h>
#include <apiXC_Ace.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <drvSERFLASH.h>
#include <tvini.h>
#include <platform.h>

#include <sys/stat.h>
#include "hwcomposerMMVideo.h"
#ifdef SUPPORT_TUNNELED_PLAYBACK
#include <hardware/tv_input.h>
#include <mstar_tv_input_sideband.h>
#include "sideband.h"
#endif

#define NOTIFY_TO_SET_DS defined(MSTAR_CLIPPERS) || defined(MSTAR_KANO) || defined(MSTAR_CURRY)

#if NOTIFY_TO_SET_DS
#include "VideoSetWrapper.h"
#endif
pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
#define HWC_LOCK  pthread_mutex_lock(&sLock)
#define HWC_UNLOCK pthread_mutex_unlock(&sLock)

static void processSignal(int signo) {
    ALOGE("Signal is %d\n", signo);
}

void create_pipe() {
    mode_t mode = 0777;

    unlink(FIFO_MM);
    if (mkfifo(FIFO_MM, mode) < 0)
        ALOGE("failed to make mm fifo : %s", strerror(errno));

    unlink(FIFO_TV);
    if (mkfifo(FIFO_TV, mode) < 0)
        ALOGE("failed to make tv fifo : %s", strerror(errno));

    signal(SIGPIPE, processSignal);
}

void get_panel_ratio(hwc_context_t *dev, hwc_win *win, bool recheck) {

    static float Wratio = 1.0f, Hratio = 1.0f;
    bool check = false;

    if (check == false || recheck) {
        dictionary *sysINI = NULL, *modelINI = NULL, *panelINI = NULL;
        int osdWidth = 0, osdHeight = 0;
        int ret = 0;

        ret = tvini_init(&sysINI, &modelINI, &panelINI);
        if (ret == -1) {
            ALOGE("tvini_init: get ini fail!!!\n");
            return;
        }

        int width = win->osdRegion.right - win->osdRegion.left;
        int height = win->osdRegion.bottom - win->osdRegion.top;
        osdWidth = width == 0? iniparser_getint(panelINI, "panel:osdWidth", 1920):width;
        osdHeight = height == 0? iniparser_getint(panelINI, "panel:osdHeight", 1080):height;
#ifdef BUILD_FOR_STB
        // for MI STB play mvc need a correct panel size from timing
        if (BUILD_PLATFORM_MI == get_build_platform()) {
            MI_DISPLAY_HANDLE hDispHandle = MI_HANDLE_NULL;
            MI_DISPLAY_VideoTiming_t stCurrentVideoTiming;
            MI_DISPLAY_InitParams_t stInitParams;
            INIT_ST(stInitParams);
            if (MI_DISPLAY_Init(&stInitParams) == MI_OK) {
                if (MI_DISPLAY_GetDisplayHandle(0,&hDispHandle) != MI_OK) {
                    MI_DISPLAY_Open(0,&hDispHandle);
                }
            }
            MI_DISPLAY_GetOutputTiming(hDispHandle,&stCurrentVideoTiming);
            timingToWidthHeight(stCurrentVideoTiming.eVideoTiming,&dev->panelWidth,&dev->panelHeight);
            MI_DISPLAY_Close(hDispHandle);
            MI_DISPLAY_DeInit();
            ALOGD("timing %d , width %d,height %d \n",stCurrentVideoTiming.eVideoTiming,dev->panelWidth,dev->panelHeight);
        } else {
            dev->panelWidth = g_IPanel.Width();
            dev->panelHeight = g_IPanel.Height();
        }
#else
#if defined(MSTAR_NAPOLI) || defined(MSTAR_MONACO) || defined(MSTAR_CLIPPERS)
        if (g_IPanel.Width()) {
            dev->panelWidth = g_IPanel.Width();
            dev->panelHeight = g_IPanel.Height();
        } else {
            dev->panelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
            dev->panelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
        }
    #else
        dev->panelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
        dev->panelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
    #endif
#endif

        Wratio = (float)dev->panelWidth / osdWidth;
        Hratio = (float)dev->panelHeight / osdHeight;
        ALOGD("panel Width %d, Height %d", dev->panelWidth, dev->panelHeight);
        ALOGD("osd Width %d, Height %d", osdWidth, osdHeight);
        check = true;
        tvini_free(panelINI, modelINI, sysINI);
    }

    ALOGD("@@@@PANEL/OSD W ratio : %f, H ratio : %f", Wratio, Hratio);
    if (check == true) {
        // apply to Width
        if (Wratio > 1.0f) {
            if (win->x < 0) {
                win->x = 0;
            } else {
                win->x *= Wratio;
            }

            win->width *= Wratio;
            if (win->width > dev->panelWidth) {
                win->width = dev->panelWidth;
            }
        }
        // apply to Height
        if (Hratio > 1.0f) {
            if (win->y < 0) {
                win->y = 0;
            } else {
                win->y *= Hratio;
            }

            win->height *= Hratio;
            if (win->height > dev->panelHeight) {
                win->height = dev->panelHeight;
            }
        }
        ALOGD("@@@@ apply ratio, x=%d, y=%d, width=%d, height=%d",
              win->x, win->y, win->width, win->height);
    }
}

static void dump_layer(int i, hwc_layer_1_t const* l) {
    ALOGD("\tlayer=%d, type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%f,%f,%f,%f}, {%d,%d,%d,%d}",
          i,
          l->compositionType, l->flags, l->handle, l->transform, l->blending,
          l->sourceCropf.left,
          l->sourceCropf.top,
          l->sourceCropf.right,
          l->sourceCropf.bottom,
          l->displayFrame.left,
          l->displayFrame.top,
          l->displayFrame.right,
          l->displayFrame.bottom);
}

void mm_video_sizechanged_notify()
{
#if NOTIFY_TO_SET_DS
    if (BUILD_PLATFORM_MI == get_build_platform()) {
        /* for MI clippers output cvbs & hdmi switch,need check if we need open|close DS
            status:
            -1  => do nothing.
            0   => close DS.
            1   => restore DS.
            2   => notify to mw we've done.
        */
        #define OVERLAY_MAIN 0
        char valueDS[PROPERTY_VALUE_MAX];
        char valueMW[PROPERTY_VALUE_MAX];
        if ((property_get("ms.disable.ds", valueDS, NULL) <= 0) || (strcmp(valueDS, "1") || strcasecmp(valueDS, "true"))) {
            if ((property_get("mstar.hwc.ds", valueMW, NULL) > 0) && (strcmp(valueMW, "-1")  != 0)) {
                if ((strcmp(valueMW, "0")  == 0) && (Wrapper_Video_DS_State(OVERLAY_MAIN) == 1)) {
                    Wrapper_Video_disableDS(OVERLAY_MAIN,0);
                } else if ((strcmp(valueMW, "1")  == 0) && (Wrapper_Video_DS_State(OVERLAY_MAIN) == 0)) {
                    Wrapper_Video_disableDS(OVERLAY_MAIN,1);
                }
                ALOGD("hwc_reSetPanelSize mstar.hwc.ds  value = %d \n",atoi(valueMW));
            }
        }
        property_set("mstar.hwc.ds","2");
    }
#endif

    MS_WinInfo stWinInfo;
    MS_DispFrameFormat stdff;
    memset(&stWinInfo, 0, sizeof(MS_WinInfo));
    memset(&stdff, 0, sizeof(MS_DispFrameFormat));
    stWinInfo.load_last_setting = 1;
    for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        MS_Video_Disp_Resize((MS_Win_ID)i, stWinInfo, &stdff);
    }
}

void mm_video_init_primary(hwc_context_t* ctx)
{
    create_pipe();

    for(int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        MS_Video_Disp_Init((MS_Win_ID)i);
    }
}

void mm_video_close_primary(hwc_context_t* ctx)
{
    for(int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        MS_Video_Disp_DeInit((MS_Win_ID)i);
    }
}

void mm_video_CheckDownscaleXC()
{
    static bool bReduceBandWidth = false;
    char property[PROPERTY_VALUE_MAX];
    property_get("mstar.reduceBW.enable", property, "false");
    if (strcmp(property, "true") == 0) {
        property_get("mstar.EnterOpaqueUI",property,"false");
        if (strcmp(property,"true") == 0) {
            if (bReduceBandWidth==false) {
                MS_BOOL bEnable = 1;
                MS_Video_Disp_Set_Cmd(MS_MAIN_WINDOW, MS_DISP_CMD_SAVING_BW_MODE, &bEnable);
                bReduceBandWidth = true;
            }

        } else {
            if (bReduceBandWidth==true) {
                 MS_BOOL bEnable = 0;
                 MS_Video_Disp_Set_Cmd(MS_MAIN_WINDOW, MS_DISP_CMD_SAVING_BW_MODE, &bEnable);
                bReduceBandWidth = false;
            }
        }
    }
}

#define OVERLAY_TO_WININFO(Overlay, stWinInfo) \
    do { \
            memset(&stWinInfo, 0, sizeof(MS_WinInfo));  \
            (stWinInfo).disp_win.x = (Overlay).layer.displayFrame.left; \
            (stWinInfo).disp_win.y = (Overlay).layer.displayFrame.top;  \
            (stWinInfo).disp_win.width = (Overlay).layer.displayFrame.right \
                                        - (Overlay).layer.displayFrame.left;\
            (stWinInfo).disp_win.height = (Overlay).layer.displayFrame.bottom   \
                                        - (Overlay).layer.displayFrame.top; \
            if (((int)(Overlay).layer.sourceCropf.right <= (Overlay).FbWidth)   \
                && ((int)(Overlay).layer.sourceCropf.bottom <= (Overlay).FbHeight)) {   \
                (stWinInfo).crop_win.x = (((float)(Overlay).SrcWidth * 10 * \
                                            ((Overlay).layer.sourceCropf.left / (float)(Overlay).FbWidth)) + 5) / 10;  \
                (stWinInfo).crop_win.y = (((float)(Overlay).SrcHeight * 10 * \
                                            ((Overlay).layer.sourceCropf.top / (float)(Overlay).FbHeight)) + 5) / 10;   \
                (stWinInfo).crop_win.width = (((float)((Overlay).SrcWidth * 10 * \
                                              (((Overlay).layer.sourceCropf.right  \
                                            - (Overlay).layer.sourceCropf.left) / (float)(Overlay).FbWidth))) + 5) / 10; \
                (stWinInfo).crop_win.height = (((float)((Overlay).SrcHeight * 10 * \
                                              (((Overlay).layer.sourceCropf.bottom  \
                                            - (Overlay).layer.sourceCropf.top) / (float)(Overlay).FbHeight))) + 5) / 10; \
            } else {    \
                (stWinInfo).crop_win.x = (Overlay).layer.sourceCropf.left;  \
                (stWinInfo).crop_win.y = (Overlay).layer.sourceCropf.top;   \
                (stWinInfo).crop_win.width = (Overlay).layer.sourceCropf.right  \
                                            - (Overlay).layer.sourceCropf.left; \
                (stWinInfo).crop_win.height = (Overlay).layer.sourceCropf.bottom\
                                            - (Overlay).layer.sourceCropf.top;  \
            }   \
            (stWinInfo).osdWidth = (Overlay).layer.osdRegion.right      \
                                    - (Overlay).layer.osdRegion.left;   \
            (stWinInfo).osdHeight = (Overlay).layer.osdRegion.bottom    \
                                    - (Overlay).layer.osdRegion.top;    \
            (stWinInfo).transform = (Overlay).layer.transform;  \
            (stWinInfo).zorder = (Overlay).zorder;    \
        } while(0)

#define HARDWARE_RENDER_WIDTH                        (16)
static void getHardwareRenderGeometry(int &width, int &height)
{
    int minSize = sizeof(MS_DispFrameFormat);
    int rowBytes;

    width       = HARDWARE_RENDER_WIDTH;
    rowBytes    = width * 4;
    height      = (minSize + rowBytes - 1) / rowBytes;
}

#ifdef SUPPORT_TUNNELED_PLAYBACK
static int mm_sideband_prepare_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent)
{
    bool bExist[MAX_SIDEBAND_LAYER_NUM];

    for (uint32_t i = 0; i < MAX_SIDEBAND_LAYER_NUM; i++) {
        bExist[i] = false;
    }

    for (uint32_t i = 0; i < primaryContent->numHwLayers; i++) {
        hwc_layer_1_t *layer = &primaryContent->hwLayers[i];

        if (layer->compositionType != HWC_SIDEBAND)
            continue;

        sideband_handle_t *sidebandStream = (sideband_handle_t *)layer->sidebandStream;
        if (sidebandStream && (sidebandStream->sideband_type == TV_INPUT_SIDEBAND_TYPE_MM_XC || sidebandStream->sideband_type == TV_INPUT_SIDEBAND_TYPE_MM_GOP)) {
            MS_WinInfo stWinInfo;
            uint32_t idx, null_idx = MAX_SIDEBAND_LAYER_NUM;
            uint32_t cmd;
            bool changed = false;

            for (idx = 0; idx < MAX_SIDEBAND_LAYER_NUM; idx++) {
                if (ctx->sideband[idx].handle == NULL && null_idx == MAX_SIDEBAND_LAYER_NUM)
                    null_idx = idx;
                if (ctx->sideband[idx].handle == layer->sidebandStream) {
                    bExist[idx] = true;
                    break;
                }
            }

            if (idx == MAX_SIDEBAND_LAYER_NUM) {
                if (null_idx == MAX_SIDEBAND_LAYER_NUM) {
                    ALOGE("SideBand Full: %d", MAX_SIDEBAND_LAYER_NUM);
                    continue;
                }
                idx = null_idx;
                memset(&ctx->sideband[idx], 0, sizeof(hwc_overlay_info_t));
            }

            if (memcmp(&ctx->sideband[idx].layer.displayFrame, &layer->displayFrame, sizeof(hwc_rect_t))
                || memcmp(&ctx->sideband[idx].layer.sourceCropf, &layer->sourceCropf, sizeof(hwc_frect_t))
                || (ctx->sideband[idx].zorder != i)) {

                if (ctx->sideband[idx].bInited) {
                    ALOGD("[%d][%p] MM SIDEBAND RESIZE src crop{%f %f %f %f}->{%f %f %f %f} dst{%d %d %d %d}->{%d %d %d %d} z{%d}->{%d}"
                          , idx, ctx->sideband[idx].handle
                          , ctx->sideband[idx].layer.sourceCropf.left, ctx->sideband[idx].layer.sourceCropf.top, ctx->sideband[idx].layer.sourceCropf.right, ctx->sideband[idx].layer.sourceCropf.bottom
                          , layer->sourceCropf.left, layer->sourceCropf.top, layer->sourceCropf.right, layer->sourceCropf.bottom
                          , ctx->sideband[idx].layer.displayFrame.left, ctx->sideband[idx].layer.displayFrame.top, ctx->sideband[idx].layer.displayFrame.right, ctx->sideband[idx].layer.displayFrame.bottom
                          , layer->displayFrame.left, layer->displayFrame.top, layer->displayFrame.right, layer->displayFrame.bottom
                          , ctx->sideband[idx].zorder, i);
                }
                changed = true;
            }

            ctx->sideband[idx].handle = layer->sidebandStream;
            bExist[idx] = true;
            memcpy(&ctx->sideband[idx].layer, layer, sizeof(hwc_layer_1_t));
            ctx->sideband[idx].zorder = i;

            OVERLAY_TO_WININFO(ctx->sideband[idx], stWinInfo);

            if (ctx->sideband[idx].bInited == false) {
                cmd = E_SIDEBAND_COMMAND_INIT;
                write(sidebandStream->pipe_in[WRITE_END], &cmd, sizeof(cmd));
                write(sidebandStream->pipe_in[WRITE_END], &stWinInfo, sizeof(stWinInfo));
                ctx->sideband[idx].bInited = true;
                ALOGD("[%d][%p] MM SIDEBAND INIT crop{%f %f %f %f} dst{%d %d %d %d}"
                    , idx, ctx->sideband[idx].handle
                    , layer->sourceCropf.left, layer->sourceCropf.top, layer->sourceCropf.right, layer->sourceCropf.bottom
                    , layer->displayFrame.left, layer->displayFrame.top, layer->displayFrame.right, layer->displayFrame.bottom);
            } else if (changed) {
                cmd = E_SIDEBAND_COMMAND_RESIZE;
                write(sidebandStream->pipe_in[WRITE_END], &cmd, sizeof(cmd));
                write(sidebandStream->pipe_in[WRITE_END], &stWinInfo, sizeof(stWinInfo));
            }
        }
    }

    for (uint32_t i = 0; i < MAX_SIDEBAND_LAYER_NUM; i++) {
        if (ctx->sideband[i].handle && bExist[i] == false) {
            uint32_t cmd;
            MS_WinInfo stWinInfo = {0,};
            sideband_handle_t *sidebandStream = (sideband_handle_t *)ctx->sideband[i].handle;
            ALOGD("[%d][%p] MM SIDEBAND REMOVE", i, ctx->sideband[i].handle);
            cmd = E_SIDEBAND_COMMAND_DEINIT;
            write(sidebandStream->pipe_in[WRITE_END], &cmd, sizeof(cmd));
            write(sidebandStream->pipe_in[WRITE_END], &stWinInfo, sizeof(stWinInfo));
            ctx->sideband[i].handle = NULL;
        }
    }

    return 0;
}
#endif

int mm_video_prepare_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent,int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM],int videoLayerNumber)
{
    // configure the MM TV OVERLAY
    gralloc_module_t *gralloc = ctx->gralloc;
    bool isAnyMMOverlay = false;
    bool isAnyTVOverlay = false;

    int prev_overlay_num = ctx->overlay_num;
    ctx->overlay_num = 0;
    hwc_overlay_info_t prev_overlay[MAX_OVERLAY_LAYER_NUM];

#ifdef SUPPORT_TUNNELED_PLAYBACK
    mm_sideband_prepare_primary(ctx, primaryContent);
#endif

    HWC_LOCK;

    memcpy(&prev_overlay, &ctx->overlay, sizeof(prev_overlay));
    memset(&ctx->overlay, 0, sizeof(prev_overlay));

    for (int k=0; k<videoLayerNumber; k++) {
        if (videoLayerIndex[k] >= 0 ) {

            hwc_layer_1_t *layer = &primaryContent->hwLayers[videoLayerIndex[k]];
            private_handle_t *hnd = (private_handle_t*)layer->handle;

            //dump_layer(k, layer);
#ifdef SUPPORT_TUNNELED_PLAYBACK
            if (layer->compositionType == HWC_SIDEBAND) {
                continue;
            }
#endif

            int MM_TV_NW_OVERLAY_MASK = private_handle_t::PRIV_FLAGS_MM_OVERLAY|private_handle_t::PRIV_FLAGS_TV_OVERLAY|private_handle_t::PRIV_FLAGS_NW_OVERLAY;
            if (hnd && (hnd->mstaroverlay&MM_TV_NW_OVERLAY_MASK) && !(layer->flags & HWC_SKIP_LAYER)) {
                int fd;
                FILE *fp;
                char *fifo = NULL, *file = NULL;
                hwc_win win;

                if (hnd->mstaroverlay == private_handle_t::PRIV_FLAGS_MM_OVERLAY) {
                    isAnyMMOverlay = true;
                    fifo = (char*)FIFO_MM;
                    file = (char*)FILE_MM;
                } else if (hnd->mstaroverlay == private_handle_t::PRIV_FLAGS_TV_OVERLAY) {
                    isAnyTVOverlay = true;
                    fifo = (char*)FIFO_TV;
                    file = (char*)FILE_TV;
                }

                if ((hnd->mstaroverlay == private_handle_t::PRIV_FLAGS_NW_OVERLAY)) {
                    void *vaddr = NULL;

                    gralloc->lock(gralloc, (buffer_handle_t)hnd, (GRALLOC_USAGE_SW_READ_MASK), 0, 0, 0, 0, &vaddr);
                    if (vaddr != NULL) {
                        MS_DispFrameFormat *dff = (MS_DispFrameFormat*)vaddr;

                        if (dff->OverlayID >= (MAX_OVERLAY_LAYER_NUM)) {
                            ALOGE("hwc overlay number exceed limit");
                        } else {
                            ctx->overlay[dff->OverlayID].bInUsed = true;
                            if (!prev_overlay[dff->OverlayID].bInUsed && dff->u8FreezeThisFrame) {
                                // invalid, do not init overlay
                                ALOGE("DO NOT init overlay");
                                ctx->overlay[dff->OverlayID].bInUsed = false;
                            }

                            memcpy(&ctx->overlay[dff->OverlayID].layer, layer, sizeof(hwc_layer_1_t));
                            memcpy(&ctx->overlay[dff->OverlayID].stdff, dff, sizeof(MS_DispFrameFormat));
                            getHardwareRenderGeometry(ctx->overlay[dff->OverlayID].FbWidth, ctx->overlay[dff->OverlayID].FbHeight);

                            ctx->overlay[dff->OverlayID].SrcWidth = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
                            ctx->overlay[dff->OverlayID].SrcHeight = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;

                            ctx->overlay_num++;
                        }
                    } else {
                        ALOGE("[%d] vaddr == NULL", __LINE__);
                    }
                    gralloc->unlock(gralloc, (buffer_handle_t)hnd);
                } else {
                    if ((ctx->bSendMMDispInfo == false || ctx->bSendTVDispInfo == false) &&
                        layer->displayFrame.left == ctx->TVdisplayFrame.left &&
                        layer->displayFrame.top == ctx->TVdisplayFrame.top &&
                        layer->displayFrame.right == ctx->TVdisplayFrame.right &&
                        layer->displayFrame.bottom == ctx->TVdisplayFrame.bottom) {
                        //ALOGE("same display info as before");
                        continue;
                    }

                    // save previous frame
                    ctx->TVdisplayFrame.left = layer->displayFrame.left;
                    ctx->TVdisplayFrame.top = layer->displayFrame.top;
                    ctx->TVdisplayFrame.right = layer->displayFrame.right;
                    ctx->TVdisplayFrame.bottom = layer->displayFrame.bottom;

                    win.x = layer->displayFrame.left;
                    win.y = layer->displayFrame.top;
                    win.width = layer->displayFrame.right - layer->displayFrame.left;
                    win.height = layer->displayFrame.bottom - layer->displayFrame.top;
                    win.osdRegion = layer->osdRegion;
                    ALOGD("@@@@x=%d, y=%d, width=%d, height=%d", win.x, win.y, win.width, win.height);
                    get_panel_ratio(ctx, &win, false);

                    if (hnd->mstaroverlay == private_handle_t::PRIV_FLAGS_TV_OVERLAY) {
                        // TODO in the future
                    }
                    fd = open(fifo, O_RDWR | O_NONBLOCK);
                    if (fd < 0) {
                        ALOGE("failed to open fifo(%s), err=%s", fifo, strerror(errno));
                    } else {
                        //clear fifo
                        hwc_win tmpwin;
                        while (read(fd, (char*)&tmpwin, sizeof(hwc_win)) > 0);

                        if (write(fd, (char*)&win, sizeof(hwc_win)) < 0) {
                            ALOGE("failed to write data : %s", strerror(errno));
                        }
                        close(fd);
                    }

                    fp = fopen(file, "w+");
                    if (!fp) {
                        ALOGE("fail to open file(%s), err=%s", file, strerror(errno));
                        continue;
                    }

                    fwrite(&win, sizeof(hwc_win), 1, fp);
                    fclose(fp);
                }
            }
        }
    }
    HWC_UNLOCK;

    // Overlay Change, remove overlay first
    //ALOGD("Overlay Change, prev_overlay_num = %d, ctx->overlay_num = %d", prev_overlay_num, ctx->overlay_num);
    for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        if (prev_overlay[i].bInUsed != ctx->overlay[i].bInUsed) {
            hwc_layer_1_t *overlayer = &ctx->overlay[i].layer;

            ALOGD("Overlay[%d], bInUsed = %d, bIsForceClose = %d", i, ctx->overlay[i].bInUsed, ctx->bIsForceClose[i]);
            if (!ctx->overlay[i].bInUsed) {     // remove overlay
                MS_Video_Disp_Close((MS_Win_ID)i);
                ctx->u8SizeChanged[i] = 0;
            } else {                            // init overlay
                MS_WinInfo stWinInfo;

                OVERLAY_TO_WININFO(ctx->overlay[i], stWinInfo);
                MS_Video_Disp_Open((MS_Win_ID)i, stWinInfo, &ctx->overlay[i].stdff);
            }
        }
    }

    if (ctx->overlay_num) {

        for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
            if (ctx->overlay[i].bInUsed && prev_overlay[i].bInUsed) {
                if (memcmp(&ctx->overlay[i].layer.displayFrame, &prev_overlay[i].layer.displayFrame, sizeof(hwc_rect_t))
                    || memcmp(&ctx->overlay[i].layer.sourceCropf, &prev_overlay[i].layer.sourceCropf, sizeof(hwc_frect_t))) {

                    hwc_layer_1_t *overlayer = &ctx->overlay[i].layer;
                    ALOGE("Overlay %d size changeed src {%d %d} -> {%d, %d} crop{%f %f %f %f}->{%f %f %f %f} dst{%d %d %d %d}->{%d %d %d %d}"
                          , i, prev_overlay[i].SrcWidth, prev_overlay[i].SrcHeight, ctx->overlay[i].SrcWidth, ctx->overlay[i].SrcHeight
                          , prev_overlay[i].layer.sourceCropf.left, prev_overlay[i].layer.sourceCropf.top, prev_overlay[i].layer.sourceCropf.right, prev_overlay[i].layer.sourceCropf.bottom
                          , ctx->overlay[i].layer.sourceCropf.left, ctx->overlay[i].layer.sourceCropf.top, ctx->overlay[i].layer.sourceCropf.right, ctx->overlay[i].layer.sourceCropf.bottom
                          , prev_overlay[i].layer.displayFrame.left, prev_overlay[i].layer.displayFrame.top, prev_overlay[i].layer.displayFrame.right, prev_overlay[i].layer.displayFrame.bottom
                          , ctx->overlay[i].layer.displayFrame.left, ctx->overlay[i].layer.displayFrame.top, ctx->overlay[i].layer.displayFrame.right, ctx->overlay[i].layer.displayFrame.bottom);

#if HWC_DS_APPLY_SIZE_CHANGE_IMMEDIATELY
                    if (Wrapper_Video_DS_State(i) || (MS_Video_Disp_Get_State((MS_Win_ID)i) < MS_STATE_OPENING)){
#else
                    if (MS_Video_Disp_Get_State((MS_Win_ID)i) < MS_STATE_OPENING){
#endif
                        MS_WinInfo stWinInfo;

                        OVERLAY_TO_WININFO(ctx->overlay[i], stWinInfo);
                        MS_Video_Disp_Resize((MS_Win_ID)i, stWinInfo, &ctx->overlay[i].stdff);
                        ctx->u8SizeChanged[i] = 0;
                    } else {
                        ctx->u8SizeChanged[i] = SIZE_CHANGE_THRESHOLD_MAX;  // prevent continuous size change makes screen blink
                    }
                }
            }
        }

        for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
            if (ctx->u8SizeChanged[i] && ctx->overlay[i].bInUsed && !ctx->bIsForceClose[i]
                && !memcmp(&ctx->overlay[i].layer.displayFrame, &prev_overlay[i].layer.displayFrame, sizeof(hwc_rect_t))) {

                if (ctx->u8SizeChanged[i] == SIZE_CHANGE_THRESHOLD_MIN) {
                    ALOGD("Overlay [%d] do size change!", i);

                    MS_WinInfo stWinInfo;

                    OVERLAY_TO_WININFO(ctx->overlay[i], stWinInfo);
                    MS_Video_Disp_Resize((MS_Win_ID)i, stWinInfo, &ctx->overlay[i].stdff);

                    ctx->u8SizeChanged[i] = 0;
                } else {
                    ctx->u8SizeChanged[i]--;
                }
            }
        }
    }

    ctx->bSendMMDispInfo = !isAnyMMOverlay;
    ctx->bSendTVDispInfo = !isAnyTVOverlay;
    if (isAnyMMOverlay == false)
        remove(FILE_MM);
    if (isAnyTVOverlay == false)
        remove(FILE_TV);

    return 0;
}
