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

/* mstar header */
#include <iniparser.h>
#include <mmap.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <apiXC.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <map>
#include <vector>
#include <algorithm>

#include "hwcomposer_context.h"
#include "hwcomposerGraphic.h"
#include "hwcomposer_util.h"
#include "hwcomposer_gop.h"
#include "hwcomposer_dip.h"
#ifdef ENABLE_HWC14_CURSOR
#include "hwcomposer_cursor.h"
#endif
#ifdef ENABLE_HWC14_SIDEBAND
#include <hardware/tv_input.h>
#include <mstar_tv_input_sideband.h>
#include "sideband.h"
#endif

/*
 * main thread send composition to work thread through bufferqueue,
 * if pending compositions excceed queue count, skip earliest composition, release fence
 */
#define MAX_PENDING_QUEUE_COUNT 3
#define OSD_4K2K_WIDTH                  3840
#define OSD_4K2K_HEIGHT                 2160
#define OSD_2K1K_WIDTH                  1920
#define OSD_2K1K_HEIGHT                 1080
static pthread_t work_thread;
static pthread_mutex_t global_mutex;

// these are under mutex protection, never touch these outside mutex
// if work thread found queue empty, wait for main thread signal new pending item
static pthread_cond_t empty_cond;
// if main thread found queue full, wait for working thread signal new empty slot
static pthread_cond_t full_cond;
static HWWindowInfo_t pendingHWWindowInfoQueue[MAX_PENDING_QUEUE_COUNT][MAX_HWWINDOW_COUNT];
static int startPosition = 0;
static int queueLength = 0;

static void *graphic_work_thread(void *data);

int graphic_init_primary(hwc_context_t* ctx) {
    char property[PROPERTY_VALUE_MAX] = {0};
    dictionary *sysINI, *modelINI, *panelINI, *moduleINI, *hwcINI, *dfbrcFile;
    int width=0;
    int height=0;
    int fakefreq = 60;

    XC_GFX_init();

    if(openConfigFile(&sysINI, &modelINI, &panelINI, &moduleINI, &dfbrcFile, &hwcINI)== -1) {
        ALOGE("Open conifg file fail");
    }

    if (property_get("mstar.bootlogo.gop", property, NULL) > 0) {
        ctx->bootlogo.gop = atoi(property);
        if (property_get("mstar.close.bootlogo.gop.frame", property, NULL) > 0) {
            ctx->bootlogo.closeThreshold = atoi(property);
        }
        ctx->bootlogo.bGopClosed = false;
    } else {
        ctx->bootlogo.bGopClosed = true;
    }

    property_get("mstar.fakevsync.freq", property, "60");
    fakefreq = atoi(property);
    ALOGD("the mstar.fakevsync.freq is %d",fakefreq);
    ctx->vsync_period = 1000000000.0 / fakefreq;

    ctx->osdWidth = ctx->width = iniparser_getint(panelINI, "panel:osdWidth", 1920);
    ctx->osdHeight = ctx->height = iniparser_getint(panelINI, "panel:osdHeight", 1080);
    if (property_get("ro.sf.osd_size", property, NULL) > 0) {
        if ((0==strcmp(property,"1080p")) || (0==strcmp(property,"1080P"))) {
            ctx->osdWidth = ctx->width = 1920;
            ctx->osdHeight = ctx->height = 1080;
            ALOGD("the ro.sf.lcd_size is set as 1080p");
        } else if ((0==strcmp(property,"720p")) || (0==strcmp(property,"720P"))) {
            ctx->osdWidth = ctx->width = 1280;
            ctx->osdHeight = ctx->height = 720;
            ALOGD("the ro.sf.lcd_size is set as 720p");
        } else {
            ALOGD("warning!! wrong value is set to the ro.sf.lcd_size ");
        }
    }
    if (property_get("ro.sf.lcd_density", property, NULL) > 0) {
        ctx->xdpi = ctx->ydpi = atoi(property) * 1000;
    } else if (ctx->width <= 1920 && ctx->height <= 1080) {
        ctx->xdpi = ctx->ydpi = 240 * 1000;
    } else {
        ctx->xdpi = ctx->ydpi = 480 * 1000;
    }
    ctx->panelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
    ctx->panelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
    ctx->ursaVersion = iniparser_getint(moduleINI, "M_URSA:F_URSA_URSA_TYPE", 0);
    ctx->osdc_enable = iniparser_getboolean(moduleINI, "M_BACKEND:F_BACKEND_ENABLE_OSDC", 0);
    ctx->panelLinkType = iniparser_getint(panelINI, "panel:m_ePanelLinkType", 10);
    ctx->panelLinkExtType = iniparser_getint(panelINI, "panel:m_ePanelLinkExtType", 51);
    property_get("mstar.build.for.stb", property, "false");
    if (strcmp(property, "true") == 0) {
        ctx->stbTarget = true;
        ctx->bRGBColorSpace = false;
        ALOGD("the device is for STB");
    } else {
        ctx->stbTarget = false;
        ctx->bRGBColorSpace = true;
        ALOGD("the device is NOT for STB");
    }

    // for ctx->bRGBColorSpace ,mstar.ColorSpace.bRGB has higher priority
    // than mstar.build.for.stb. property mstar.build.for.stb is get firstly and then
    // get the mstar.colorspace.brgb

    memset(property,0,sizeof(property));
    if (property_get("mstar.colorspace.brgb",property,NULL) > 0) {
        if (strcmp(property,"true") == 0) {
            ctx->bRGBColorSpace = true;
            ALOGD("force the colorspace to be rgb");
        } else if (strcmp(property,"false") == 0) {
            ctx->bRGBColorSpace = false;
            ALOGD("force the colorspace to be yuv ");
        }
    }

    unsigned int gop_dest = GOP_getDestination(ctx);
    if(gop_dest == E_GOP_DST_FRC) {
        GOP_getCurOCTiming(&ctx->curTimingWidth, &ctx->curTimingHeight, &ctx->panelHStart);
    } else {
        GOP_getCurOPTiming(&ctx->curTimingWidth, &ctx->curTimingHeight, &ctx->panelHStart);
    }
    ALOGE("curTimingWidth =%d,curTimingHeight =%d, panelHStart=%d\n",ctx->curTimingWidth,ctx->curTimingHeight,ctx->panelHStart);

    if (ctx->stbTarget) {
        STB_setResolutionState();
    }

    ctx->displayRegionWidth = ctx->curTimingWidth;
    ctx->displayRegionHeight = ctx->curTimingHeight;

    if (!ctx->stbTarget) {
        if (ctx->panelWidth < ctx->osdWidth) {
            ctx->width = ALIGNMENT(ctx->panelWidth,16);
        }

        if (ctx->panelHeight < ctx->osdHeight) {
            ctx->height = ALIGNMENT(ctx->panelHeight,16);
        }
    }

    if ((ctx->width!=RES_4K2K_WIDTH)||(ctx->height!=RES_4K2K_HEIGHT)) {
        property_set("mstar.4k2k.2k1k.coexist","false");
    }
    getMirrorMode(modelINI, &ctx->mirrorModeH, &ctx->mirrorModeV);

    MApi_GOP_GWIN_ResetPool();
    std::map<int,int> mGopLayerMap;

    GOP_LayerConfig stLayerSetting;
    MS_U32 u32size = sizeof(GOP_LayerConfig);
    memset(&stLayerSetting,0,u32size);
    char layerStr[128] = "";
    stLayerSetting.u32LayerCounts = iniparser_getunsignedint(dfbrcFile, "DirectFBRC:DFBRC_LAYERCOUNTS", 0);
    for (unsigned int i = 0; i < stLayerSetting.u32LayerCounts; i++) {
        sprintf(layerStr,"DirectFBRC:DFBRC_LAYER%d_GOPINDEX",i);
        stLayerSetting.stGopLayer[i].u32GopIndex = iniparser_getunsignedint(dfbrcFile, layerStr, 0);
        stLayerSetting.stGopLayer[i].u32LayerIndex = i;
        //save gop and layer to map
        mGopLayerMap[stLayerSetting.stGopLayer[i].u32GopIndex] = i;
        ALOGD("%s",layerStr);
        ALOGD("the layer%d  gop index is %d",i,stLayerSetting.stGopLayer[i].u32GopIndex);
    }
    ctx->stLayerSetting = stLayerSetting;
    std::vector<int> HwcGop;

    //Get osd gop infomation form hwcomposer13.ini,and save layer to HwcGop temporary,it is used for sort.
    for (int i=0; i<MAX_HWWINDOW_COUNT; i++) {
        // default set the gopindex as 0xff
        ctx->HWWindowInfo[i].gop = 0xff;
        ctx->bGopHasInit[i] = false;

        char gopsetting[40];
        sprintf(gopsetting, "hwcomposer:HWC_GOP_IN_HWWINDOW%d", i);
        int gop = iniparser_getint(hwcINI, gopsetting, 0xff);
        if (gop!=0xff) {
            if ((mGopLayerMap.size() > 0)) {
                HwcGop.push_back(mGopLayerMap[gop]);
            } else {
                HwcGop.push_back(gop);
            }
        }
    }

    if (mGopLayerMap.size() > 0) {
        std::sort(HwcGop.begin(),HwcGop.end());
        //restore gop infomation to HwcGop
        for (int i=0; i<HwcGop.size(); i++) {
            for(std::map<int, int>::iterator it = mGopLayerMap.begin(); it != mGopLayerMap.end(); it++) {
                if (it->second == HwcGop.at(i)) {
                    ctx->HWWindowInfo[i].gop = HwcGop[i] = it->first;
                    break;
                }
            }
        }
    } else {
        for (int i=0; i<HwcGop.size(); i++) {
            ctx->HWWindowInfo[i].gop = HwcGop[i];
        }
    }

    if (HwcGop.size() > 0) {
       ctx->gopCount = HwcGop.size();
    } else {
      ALOGE("error !!! there is no gop for hwcomposer,maybe there is no hwcini file");
    }
#ifdef MALI_AFBC_GRALLOC
    GOP_CAP_AFBC_INFO sCaps;
    MApi_GOP_GetChipCaps(E_GOP_CAP_AFBC_SUPPORT, &sCaps, sizeof(GOP_CAP_AFBC_INFO));
#endif
    for (int i=0; i<ctx->gopCount; i++) {
        ctx->HWWindowInfo[i].gwin = GOP_getAvailGwinId(ctx->HWWindowInfo[i].gop);
        ALOGD("%s , HWWindowInfo[%d]  gop = %d, gwin = %d",__FUNCTION__,i,ctx->HWWindowInfo[i].gop,ctx->HWWindowInfo[i].gwin);

        if ((ctx->HWWindowInfo[i].gop == 0xff)||(ctx->HWWindowInfo[i].gwin == 0xff)) {
            ALOGE("warning !!! ,the HWWindowInfo[%d] has no gop&gwin maybe there is no  hwcini file",i);
        }
#ifdef MALI_AFBC_GRALLOC
        if (sCaps.GOP_AFBC_Support & GOP_getBitMask(ctx->HWWindowInfo[i].gop)) {
            ctx->HWWindowInfo[i].bSupportGopAFBC = true;
        } else
#endif
        {
            ctx->HWWindowInfo[i].bSupportGopAFBC = false;
        }
        ctx->HWWindowInfo[i].channel = E_GOP_SEL_MIU0;
        ctx->HWWindowInfo[i].fbid = INVALID_FBID;
        ctx->HWWindowInfo[i].releaseFenceFd = -1;
        ctx->HWWindowInfo[i].layerIndex = -1;
        ctx->HWWindowInfo[i].subfbid = INVALID_FBID;
        ctx->HWWindowInfo[i].gop_dest = GOP_getDestination(ctx);
        ctx->HWWindowInfo[i].releaseSyncTimelineFd= sw_sync_timeline_create();
        if (ctx->HWWindowInfo[i].releaseSyncTimelineFd < 0) {
            ALOGE("sw_sync_timeline_create failed");
        }
        ctx->HWWindowInfo[i].releaseFenceValue = 0;
        ctx->HWWindowInfo[i].ionBufferSharedFd = -1;
    }
#ifdef ENABLE_HWC14_CURSOR
    ctx->gameCursorEnabled = false;
    ctx->hwCusorInfo[0].bInitialized = false;
    ctx->hwCusorInfo[0].layerIndex = -1;
    ctx->hwCusorInfo[0].operation = 0;
    memcpy(&ctx->lastHwCusorInfo[0], &ctx->hwCusorInfo[0], sizeof(ctx->hwCusorInfo[0]));
#endif
    memcpy(ctx->lastHWWindowInfo, ctx->HWWindowInfo, sizeof(ctx->HWWindowInfo));

    CMDQ_and_MLoad_init();
    DIP_init(ctx);
    GFX_ClearFbBuffer();

    closeConfigFile(sysINI, modelINI, panelINI, moduleINI, dfbrcFile, hwcINI);

    pthread_cond_init(&empty_cond, NULL);
    pthread_cond_init(&full_cond, NULL);
    pthread_mutex_init(&global_mutex, NULL);

    pthread_create(&work_thread, NULL, graphic_work_thread, (void*)ctx);
    ALOGD("MStar graphic Config:\n");
    ALOGD("OSD WIDTH    : %d\n", ctx->osdWidth);
    ALOGD("OSD HEIGHT   : %d\n", ctx->osdHeight);
    ALOGD("PANEL WIDTH  : %d\n", ctx->panelWidth);
    ALOGD("PANEL HEIGHT : %d\n", ctx->panelHeight);
    ALOGD("DISP WIDTH   : %d\n", ctx->width);
    ALOGD("DISP HEIGHT  : %d\n", ctx->height);
    ALOGD("MIRROE MODE  H : %d,V : %d\n",ctx->mirrorModeH, ctx->mirrorModeV);
    ALOGD("URSA VERSION : %d\n", ctx->ursaVersion);
    return 0;
}

/* set the VendorUsageBits as VENDOR_CMA_CONTIGUOUS_MEMORY or VENDOR_CMA_DISCRETE_MEMORY
* if the VENDOR_CMA_CONTIGUOUS_MEMORY is set, but the mstar.maliion.conmem.location is not specified
* and the mstar.maliion.dismem.location is specified.Then will choose allocate discrete memory
* instead of contiguous memory. if mstar.maliion.dismem.location is still not specified,then the system
* heap will be choosed*/
static void setLayerVendorUsageBits(hwc_context_t* ctx,private_handle_t *handle,hwc_layer_1_t &layer,VENDOR_USAGE_BITS VendorUsageBits) {
    if (handle) {
        char  property[PROPERTY_VALUE_MAX] = {0};
        int miu_location = -1;
        switch (VendorUsageBits) {
            case VENDOR_CMA_CONTIGUOUS_MEMORY :
                {
                    if (property_get("mstar.maliion.conmem.location", property, NULL) > 0) {
                        miu_location = atoi(property);
                        if ((miu_location==2)&&(ctx->memAvailable&mem_type_cma_contiguous_miu2)) {
                            layer.VendorUsageBits |= (GRALLOC_USAGE_ION_CMA_MIU2|GRALLOC_USAGE_ION_CMA_CONTIGUOUS);
                        } else if ((miu_location==1)&&(ctx->memAvailable&mem_type_cma_contiguous_miu1)) {
                            layer.VendorUsageBits |= (GRALLOC_USAGE_ION_CMA_MIU1|GRALLOC_USAGE_ION_CMA_CONTIGUOUS);
                        } else if ((miu_location==0)&&(ctx->memAvailable&mem_type_cma_contiguous_miu0)) {
                            layer.VendorUsageBits |= (GRALLOC_USAGE_ION_CMA_MIU0|GRALLOC_USAGE_ION_CMA_CONTIGUOUS);
                        } else {
                            ALOGD("the mstar.maliion.conmem.location is in miu%d, but no available memory",miu_location);
                        }
                        #ifdef MALI_AFBC_GRALLOC
                        if (!ctx->HWWindowInfo[0].bSupportGopAFBC) {
                            layer.VendorUsageBits |= GRALLOC_USAGE_AFBC_OFF;
                        }
                        #endif
                    }
                }
            case VENDOR_CMA_DISCRETE_MEMORY:
                {
                    if (property_get("mstar.maliion.dismem.location", property, NULL) > 0) {
                        miu_location = atoi(property);
                        if ((miu_location==2)&&(ctx->memAvailable&mem_type_cma_discrete_miu2)) {
                            layer.VendorUsageBits |= GRALLOC_USAGE_ION_CMA_MIU2|GRALLOC_USAGE_ION_CMA_DISCRETE;
                        } else if ((miu_location==1)&&(ctx->memAvailable&mem_type_cma_discrete_miu1)) {
                            layer.VendorUsageBits |= GRALLOC_USAGE_ION_CMA_MIU1|GRALLOC_USAGE_ION_CMA_DISCRETE;
                        } else if ((miu_location==0)&&(ctx->memAvailable&mem_type_cma_discrete_miu0)) {
                            layer.VendorUsageBits |= GRALLOC_USAGE_ION_CMA_MIU0|GRALLOC_USAGE_ION_CMA_DISCRETE;
                        } else {
                            ALOGD("the mstar.maliion.dismem.location is in %d, but no available memory",miu_location);
                        }
                    }

                }
            default :
                {
                    layer.VendorUsageBits |= GRALLOC_USAGE_ION_SYSTEM_HEAP;
                    break;
                }
       }
    }
}

static bool layerSupportOverlay(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent,hwc_layer_1_t &layer, size_t i) {
    VENDOR_USAGE_BITS VendorUsageBits = VENDOR_CMA_DISCRETE_MEMORY;

    if (layer.flags & HWC_SKIP_LAYER) {
        ALOGV("layer %u: skipping", i);
        return false;
    }
    if (layer.compositionType == HWC_BACKGROUND) {
        ALOGV("\tlayer %u: background not supported", i);
        return false;
    }
    private_handle_t *handle = (private_handle_t *)layer.handle;
    if (!handle) {
        ALOGW("layer %u: handle is NULL", i);
        return false;
    }

    if (handle->mstaroverlay & private_handle_t::PRIV_FLAGS_MM_OVERLAY ||
        handle->mstaroverlay & private_handle_t::PRIV_FLAGS_TV_OVERLAY ||
        handle->mstaroverlay & private_handle_t::PRIV_FLAGS_NW_OVERLAY ) {
        return true;
    }

    if (ctx->current3DMode != DISPLAYMODE_NORMAL) {
        return false;
    }

    // scale
    unsigned int src_width = (unsigned int)layer.sourceCropf.right - (unsigned int)layer.sourceCropf.left;
    unsigned int src_height = (unsigned int)layer.sourceCropf.bottom - (unsigned int)layer.sourceCropf.top;
    unsigned int dst_width = layer.displayFrame.right - layer.displayFrame.left;
    unsigned int dst_height = layer.displayFrame.bottom - layer.displayFrame.top;
    // if need to do down scale ,not support
    // if sourceCropf.left is not 2 pixel alignment, not support
    if (src_width > dst_width || src_height > dst_height || (unsigned int)layer.sourceCropf.left&0x1) {
        ALOGV("\tlayer %u: upscale or 2 pxiel alignment not supported", i);
        return false;
    }

    // format
    if ( handle->format != HAL_PIXEL_FORMAT_RGBA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_BGRA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_RGBX_8888
#ifndef MALI_AFBC_GRALLOC
        &&handle->format != HAL_PIXEL_FORMAT_RGB_565
#endif
        ) {
        ALOGV("\tlayer %u: format not supported", i);
        return false;
    }
    // transform
    if (layer.transform != 0) {
        ALOGV("layer %u: has rotation", i);
        return false;
    }

    // when coming here ,the layer satisfied the basic conditions of GOPOVERLAY except the memory type
    // If the layer counts has been stable ,try to allocte contiguous memory for the proper layers.
    // if the layer count has not changed in the past 20 frames, the layers count is decided as stable
    // HardWare limited: On Each MIU only one layer can allocate contiguous memory.
    if ((ctx->requiredGopOverlycount < MAX_UIOVERLAY_COUNT)&&(ctx->frameCount>20)) {
        // only check the lowest zorder layer and the highest zorder layer excluding framebufferTarget layer
        // ALOGD("layer %d,handle %x,last handle %x",i,layer.handle,ctx->lastFrameLayersInfo.hnd[i]);
        if ((i==0)||(i==(primaryContent->numHwLayers-2))) {
            if (ctx->lastFrameLayersInfo.activeDegree[i]==MAX_ACTIVE_DEGREE) {
                ctx->requiredGopOverlycount ++;
                VendorUsageBits = VENDOR_CMA_CONTIGUOUS_MEMORY;
            } else if (ctx->lastFrameLayersInfo.activeDegree[i]==MIN_ACTIVE_DEGREE) {
                VendorUsageBits = VENDOR_CMA_DISCRETE_MEMORY;
            } else {
                if (ctx->lastFrameLayersInfo.VendorUsageBits[i]&GRALLOC_USAGE_ION_CMA_CONTIGUOUS) {
                    VendorUsageBits = VENDOR_CMA_CONTIGUOUS_MEMORY;
                } else if (ctx->lastFrameLayersInfo.VendorUsageBits[i]&GRALLOC_USAGE_ION_CMA_DISCRETE) {
                    VendorUsageBits = VENDOR_CMA_DISCRETE_MEMORY;
                }
            }
        }
    }
    setLayerVendorUsageBits(ctx,(private_handle_t *)layer.handle,layer,VendorUsageBits);

    // gralloc usage, currently only support contiguous memory
    unsigned int mem_type_cma_contiguous_mask = mem_type_cma_contiguous_miu0|mem_type_cma_contiguous_miu1|mem_type_cma_contiguous_miu2;
    unsigned int allocated_mem_type = (handle->memTypeInfo&mem_type_allocated_mask)>>16;
    if (!(allocated_mem_type&mem_type_cma_contiguous_mask)) {
        ALOGV("\tlayer %u: memory not supported", i);
        return false;
    }
    if (HWC_DEBUGTYPE_DISABLE_GOPOVERLAY &ctx->HwcomposerDebugType) {
        // force to render to the FramebufferTarget
        //ALOGD("%s:%d",__FUNCTION__,__LINE__);
        return false;
    }
    return true;
}

static bool isNotVideoLayerIndex(int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM], unsigned int layerIndex) {
    for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        if (layerIndex == videoLayerIndex[i])
            return false;
    }
    return true;
}

static void setCompositionTypeForAFBCLimitation (hwc_context_t* ctx,hwc_display_contents_1_t *primaryContent,int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM],bool has_fb) {
    //adjust layer Composition type if gop unsurpport afbc mode.
    //only count UI overlay, no video overlay
    int uiOverlayCount = 0;
    for (unsigned int i=0; i<(int)primaryContent->numHwLayers; i++) {
        if (primaryContent->hwLayers[i].compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
            uiOverlayCount++;
        }
    }
    if (uiOverlayCount != 0){
        bool bGopoverlayToFB = false;
        if (has_fb) {
            //have framebuffer target and ui overlay together.
            //if framebuffer target or ui overlay is 4k.rollback all ui overlays to framebuffer
            for (unsigned int i=0; i<(int)primaryContent->numHwLayers; i++) {
                unsigned int width = 1920,height = 1080;
                hwc_layer_1_t &layer = primaryContent->hwLayers[i];
                private_handle_t *hnd = (private_handle_t*)layer.handle;
                if ((layer.compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) &&
                    (hnd->internal_format & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK))){
                    width = hnd->width;
                    height = hnd->height;
                }
                if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
                    width = layer.displayFrame.right - layer.displayFrame.left;
                    height = layer.displayFrame.bottom - layer.displayFrame.top;
                }
                if (width == OSD_4K2K_WIDTH && height == OSD_4K2K_HEIGHT) {
                    bGopoverlayToFB = true;
                    break;
                }
            }
        } else {
            //have ui overlay only.
            //if has 4k ui overlay,rollback all ui overlays to framebuffer
            if (uiOverlayCount > 1) {
                for (unsigned int i=0; i<(int)primaryContent->numHwLayers; i++) {
                    hwc_layer_1_t &layer = primaryContent->hwLayers[i];
                    private_handle_t *hnd = (private_handle_t*)layer.handle;
                    if ((layer.compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) &&
                        (hnd->internal_format &(GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK))){
                        private_handle_t *handle = (private_handle_t *)primaryContent->hwLayers[i].handle;
                        if (handle->width == OSD_4K2K_WIDTH && handle->height == OSD_4K2K_HEIGHT) {
                            bGopoverlayToFB = true;
                            break;
                        }
                    }
                }
            }
        }
        if (bGopoverlayToFB) {
            for (unsigned int i=0; i<(int)primaryContent->numHwLayers; i++) {
                hwc_layer_1_t &layer = primaryContent->hwLayers[i];
                private_handle_t *hnd = (private_handle_t*)layer.handle;
                if ((layer.compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) &&
                    (hnd->internal_format &
                    (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK))) {
                    primaryContent->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                }
            }
        }
    }

    int currentHWWindowIndex = 0;
    int framebuffertargetHWWindowIndex = -1;
    for (unsigned int i=0; i<(int)primaryContent->numHwLayers; i++) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];
        private_handle_t *hnd = (private_handle_t*)layer.handle;
        if (layer.compositionType == HWC_OVERLAY) {
            if (isNotVideoLayerIndex(videoLayerIndex, i)) {
                //if layer's buffer format is afbc and current gop unsurpport afbc,rollback layer to FB
                if ((hnd->internal_format & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK))
                    && !ctx->HWWindowInfo[currentHWWindowIndex].bSupportGopAFBC) {
                    layer.compositionType = HWC_FRAMEBUFFER;
                    if (framebuffertargetHWWindowIndex == -1) {
                        framebuffertargetHWWindowIndex = currentHWWindowIndex++;
                    }
                } else {
                    currentHWWindowIndex++;
                }
            }
        } else if (layer.compositionType == HWC_FRAMEBUFFER) {
            if (framebuffertargetHWWindowIndex == -1) {
                framebuffertargetHWWindowIndex = currentHWWindowIndex++;
            }
        }
    }

    //adjust gop layer's type & FB target's buffer format if gop unsurpport afbc mode.
    int index = framebuffertargetHWWindowIndex != -1? framebuffertargetHWWindowIndex:0;
    //if framebuffer target is enable and it's gop unsurpport afbc ,turn off framebuffer's afbc.
    ctx->bAFBCFramebuffer = ctx->HWWindowInfo[index].bSupportGopAFBC;
    //UI overlay is not exist,we use one gop only.judge 3D display mode.
    //Hardware limitation,In AFBC mode,GOP not support all 3D mode.so we rollback
    //to linear buffer and using gop 3D.
    if (ctx->current3DMode == DISPLAYMODE_NORMAL_FP ||
        ctx->current3DMode == DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY ||
        ctx->current3DMode == DISPLAYMODE_FRAME_ALTERNATIVE_LLRR) {
        ctx->bAFBCFramebuffer = false;
    }
    if (HWC_DEBUGTYPE_DISABLE_FRAMEBUFFER_AFBC &ctx->HwcomposerDebugType) {
        // disable framebuffer afbc
        //ALOGD("%s:%d",__FUNCTION__,__LINE__);
        ctx->bAFBCFramebuffer = false;
    }
}

// this function set the layer composition by the layer attribute&ctx->gopCount
static void setCompositionTypeForLayersStageOne(hwc_context_t* ctx,hwc_display_contents_1_t *primaryContent,int &videoLayerNumber,int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM]) {
    int first_fb = 0, last_fb = 0;
    bool hasfb = false;
    for (int i=(primaryContent->numHwLayers-1); i>=0; i--) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];
        private_handle_t *handle = (private_handle_t *)layer.handle;
        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ctx->displayFrameFTScaleRatio_w = (float)ctx->curTimingWidth/(layer.displayFrame.right - layer.displayFrame.left);
            ctx->displayFrameFTScaleRatio_h = (float)ctx->curTimingHeight/(layer.displayFrame.bottom - layer.displayFrame.top);
            ctx->displayRegionWidth = layer.displayFrame.right - layer.displayFrame.left;
            ctx->displayRegionHeight = layer.displayFrame.bottom - layer.displayFrame.top;
            continue;
        }

#if defined(ENABLE_HWC14_SIDEBAND) || defined(SUPPORT_TUNNELED_PLAYBACK)
        if (layer.compositionType == HWC_SIDEBAND) {
            layer.hints |=  HWC_HINT_CLEAR_FB;
            sideband_handle_t *sidebandStream = (sideband_handle_t *)layer.sidebandStream;
            if (sidebandStream && sidebandStream->sideband_type != TV_INPUT_SIDEBAND_TYPE_MM_GOP) {
                if (videoLayerNumber < MAX_OVERLAY_LAYER_NUM) {
                    videoLayerIndex[videoLayerNumber++] = i;
                } else {
                    ALOGE("Error: only support %d video layers!", MAX_OVERLAY_LAYER_NUM);
                }
            }
            continue;
        }
#endif

        if (handle&&(((handle->memTypeInfo&mem_type_allocated_mask)>>16)!=(handle->memTypeInfo&mem_type_request_mask))) {
            // the request memtype is not available
            ctx->memAvailable &= ~(handle->memTypeInfo&mem_type_request_mask);
        }

#ifdef ENABLE_HWC14_CURSOR
        if(layerSupportCursorOverlay(ctx, primaryContent,layer, i)) {
            layer.compositionType = HWC_CURSOR_OVERLAY;
            continue;
        }
#endif

        if (layerSupportOverlay(ctx, primaryContent,layer, i)) {
            layer.compositionType = HWC_OVERLAY;
            if ( handle && (handle->mstaroverlay & private_handle_t::PRIV_FLAGS_MM_OVERLAY ||
                            handle->mstaroverlay & private_handle_t::PRIV_FLAGS_TV_OVERLAY ||
                            handle->mstaroverlay & private_handle_t::PRIV_FLAGS_NW_OVERLAY )) {
                if (videoLayerNumber < MAX_OVERLAY_LAYER_NUM) {
                    videoLayerIndex[videoLayerNumber++] = i;
                } else {
                    ALOGE("Error: only support %d video layers!", MAX_OVERLAY_LAYER_NUM);
                }
            }

            continue;
        }


        layer.compositionType = HWC_FRAMEBUFFER;
        // TODO: need to refine codes ,decide which layer need change the memory type
        // because kernel still can not allocate discerete memory ,temporary set it as VENDOR_CMA_CONTIGUOUS_MEMORY
        if (!(layer.VendorUsageBits&GRALLOC_USAGE_ION_MASK)) {
            setLayerVendorUsageBits(ctx,(private_handle_t *)layer.handle,layer,VENDOR_CMA_DISCRETE_MEMORY);
        }

        if (!hasfb) {
            hasfb = true;
            last_fb = i;
        }
        first_fb = i;
    }
    if (hasfb) {
        for (unsigned int i = first_fb+1; i < last_fb; i++) {
            if (primaryContent->hwLayers[i].compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
                primaryContent->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
            }
        }
    }

    int uiOverlayCount = 0; // only count UI overlay, no video overlay
    for (unsigned int i = 0; i < (int)primaryContent->numHwLayers; i++) {
        if (primaryContent->hwLayers[i].compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
            uiOverlayCount++;
        }
    }

    if (hasfb) {
        int needfbcount = uiOverlayCount - std::min(ctx->gopCount - 1, MAX_UIOVERLAY_COUNT);
        if (needfbcount > 0) {
            for (int i = first_fb-1; i >= 0 && needfbcount > 0; i--) {
                if (primaryContent->hwLayers[i].compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
                    primaryContent->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                    needfbcount--;
                }
            }
            if (needfbcount > 0) {
                for (unsigned int i = last_fb+1; i < (int)primaryContent->numHwLayers && needfbcount > 0; i++) {
                    if (primaryContent->hwLayers[i].compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
                        primaryContent->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                        needfbcount--;
                    }
                }
            }
            if (needfbcount > 0) {
                ALOGE("needfbcount too much");
            }
            // Maybe still has the case the framebufferTarget is not the lowest gop overlay
            // this will make the the gop alpha blend result different from gles blend..
        }
    } else {
        if (uiOverlayCount > std::min(ctx->gopCount , MAX_UIOVERLAY_COUNT)) {
            int needfbcount = uiOverlayCount - std::min(ctx->gopCount - 1, MAX_UIOVERLAY_COUNT);
            for (unsigned int i = 0; i < (int)primaryContent->numHwLayers && needfbcount > 0; i++) {
                if (primaryContent->hwLayers[i].compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
                    primaryContent->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                    needfbcount--;
                    hasfb = true;
                }
            }
            if (needfbcount > 0) {
                ALOGE("needfbcount too much");
            }
        }
    }
    // The GOP and video zorder will not be changed,GOP always on the video window.
    // so if there is ui layer under the video layer.
    // the video player need to  be set as HWC_HINT_CLEAR_FB
    // and the ui layer need to be set as HWC_FRAMEBUFFER compositiontype.
    for (int i = 0; i < videoLayerNumber; i++) {
        hwc_layer_1_t &videolayer = primaryContent->hwLayers[videoLayerIndex[i]];
        for (int j= videoLayerIndex[i] - 1; j>=0; j--) {
            hwc_layer_1_t &layer = primaryContent->hwLayers[j];
            private_handle_t *hnd = (private_handle_t *)layer.handle;
            if ((layer.compositionType==HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, (unsigned)j)) ||
                (layer.compositionType == HWC_FRAMEBUFFER)) {
                hasfb = true;
                layer.compositionType = HWC_FRAMEBUFFER;
                videolayer.hints |= HWC_HINT_CLEAR_FB;
            }
        }
    }
#ifdef MALI_AFBC_GRALLOC
    setCompositionTypeForAFBCLimitation(ctx, primaryContent, videoLayerIndex, hasfb);
#endif
}

static bool checkFrameBufferTargetNeedRedraw(hwc_context_t* ctx,hwc_display_contents_1_t *primaryContent) {
    // here have compute which UI layers will be HWC_OVERLY ,
    // here will check all the HWC_FRAMEBUFFER layers whether that
    // needs redraw
    LayerCache &LastFrameLayersInfo = ctx->lastFrameLayersInfo;
    bool bneedGlesBlend = true;
    bool bhndDirty = false;
    bool bsCropDirty = false;
    bool bdisFrameDirty = false;
    bool bTypeDirty = false;
    bool bAlphaDirty = false;
    bool bBlendingDirty = false;
    int fbLayerCount = 0;

    for (unsigned int i=0; i<(primaryContent->numHwLayers-1) && i<MAX_NUM_LAYERS; i++) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];

        // HWC_SKIP_LAYER layer will be force set as HWC_FRAMEBUFFER by SF
        // so need to do gles blend for all layer's with HWC_FRAMEBUFFER & HWC_HINT_CLEAR_FB
        if (layer.flags & HWC_SKIP_LAYER ) {
            bneedGlesBlend = true;
        }

#ifdef ENABLE_HWC14_CURSOR
        //if mstar.desk-enable-gamecursor set, the system cursor should't show
         if ((layer.flags & HWC_IS_CURSOR_LAYER)&&(ctx->gameCursorEnabled)) {
                layer.compositionType = HWC_OVERLAY;
                continue;
            }
#endif
        private_handle_t *hnd = (private_handle_t*)layer.handle;

        if (layer.compositionType == HWC_FRAMEBUFFER
            || ((layer.hints & HWC_HINT_CLEAR_FB) && layer.compositionType == HWC_OVERLAY)
#ifdef ENABLE_HWC14_SIDEBAND
            || ((layer.hints & HWC_HINT_CLEAR_FB) && layer.compositionType == HWC_SIDEBAND)
#endif
        ) {
            if (layer.compositionType == HWC_FRAMEBUFFER) {
                fbLayerCount ++ ;
            }

            if (hnd != LastFrameLayersInfo.hnd[i]) {
                bhndDirty =  true;
            }

            if (!rect_eq(layer.sourceCrop,LastFrameLayersInfo.sourceCrop[i])) {
                bsCropDirty = true;
            }

            if (!rect_eq(layer.displayFrame,LastFrameLayersInfo.displayFrame[i])) {
                bdisFrameDirty = true;
            }

            if (layer.compositionType != LastFrameLayersInfo.compositionType[i]) {
                bTypeDirty = true;
            }

            if (layer.planeAlpha != LastFrameLayersInfo.planeAlpha[i]) {
                bAlphaDirty = true;
            }

            if (layer.blending != LastFrameLayersInfo.blending[i]) {
                bBlendingDirty = true;
            }

            if (bhndDirty || bsCropDirty || bdisFrameDirty || bTypeDirty || bAlphaDirty || bBlendingDirty) {
                bneedGlesBlend = true;
            }
        }

        // calculate the activeDegree of the layer
        if (LastFrameLayersInfo.hnd[i]!=layer.handle) {
            if (LastFrameLayersInfo.activeDegree[i] < MAX_ACTIVE_DEGREE) {
                LastFrameLayersInfo.activeDegree[i]++;
            }
        } else {
            if (LastFrameLayersInfo.activeDegree[i] > MIN_ACTIVE_DEGREE) {
                LastFrameLayersInfo.activeDegree[i]--;
            }
        }
    }

    if (fbLayerCount!=LastFrameLayersInfo.fbLayerCount) {
        bneedGlesBlend = true;
    }
    if ((primaryContent->numHwLayers-1)!= LastFrameLayersInfo.layerCount) {
        bneedGlesBlend = true;
    }

    if (ctx->b3DModeChanged) {
        ALOGD("3D Mode changed need GlesBlend");
        bneedGlesBlend = true;
    }

    if (ctx->bFramebufferNeedRedraw) {
        bneedGlesBlend = true;
        ctx->bFramebufferNeedRedraw = false;
    }

    if (HWC_DEBUGTYPE_FORCE_GLESBLEND &ctx->HwcomposerDebugType) {
        // force Rerender to the FramebufferTarget,
        bneedGlesBlend = true;
    }
    return bneedGlesBlend;
}

static void AllocateHWWindowsVideoPlanesForLayers(hwc_context_t* ctx,hwc_display_contents_1_t *primaryContent,int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM],int videoLayerNumber) {
    bool hasfb = false;
    int first_fb = 0;
    int last_fb = 0;
    int framebuffertargetHWWindowIndex = -1;
    int framebuffertargetLayerIndex = -1;
    int currentHWWindowIndex = 0;
    for (unsigned int i=0; i<(int)primaryContent->numHwLayers; i++) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];
        private_handle_t *hnd = (private_handle_t*)layer.handle;
        if (layer.compositionType == HWC_OVERLAY
#ifdef ENABLE_HWC14_SIDEBAND
            || layer.compositionType == HWC_SIDEBAND
#endif
        ) {
            if (layer.compositionType == HWC_OVERLAY && isNotVideoLayerIndex(videoLayerIndex, i)) {
                ctx->HWWindowInfo[currentHWWindowIndex].layerIndex = i;
                currentHWWindowIndex++;
                ctx->debug_LayerAttr[i].layerType = GOP_OVERLAY_LAYER;
            }
        } else if (layer.compositionType == HWC_FRAMEBUFFER) {
            if (framebuffertargetHWWindowIndex == -1) {
                framebuffertargetHWWindowIndex = currentHWWindowIndex++;
            }
            if (!hasfb) {
                hasfb = true;
                first_fb = i;
            }
            last_fb = i;
        } else if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
                framebuffertargetLayerIndex = i;
        }
#ifdef ENABLE_HWC14_CURSOR
        else if (layer.compositionType == HWC_CURSOR_OVERLAY) {
            //the last HWWindowInfo for hwcursor
            ctx->hwCusorInfo[0].layerIndex = i;
            ctx->debug_LayerAttr[i].layerType = HW_CURSOR_LAYER;
        }
#endif
    }
    if (framebuffertargetHWWindowIndex != -1 && framebuffertargetLayerIndex != -1) {
        ctx->HWWindowInfo[framebuffertargetHWWindowIndex].layerIndex = framebuffertargetLayerIndex;
        ctx->HWWindowInfo[framebuffertargetHWWindowIndex].bFrameBufferTarget = true;
        ctx->HWWindowInfo[framebuffertargetHWWindowIndex].layerRangeInSF.minLayerIndex = first_fb;
        ctx->HWWindowInfo[framebuffertargetHWWindowIndex].layerRangeInSF.maxLayerIndex = last_fb;
    }

    for (int k=0; k<videoLayerNumber; k++) {
        if (videoLayerIndex[k] >= 0 ) {
            hwc_layer_1_t *layer = &primaryContent->hwLayers[videoLayerIndex[k]];
            private_handle_t *hnd = (private_handle_t*)layer->handle;
            if (hasfb && first_fb <= videoLayerIndex[k] && last_fb >= videoLayerIndex[k]) {
                layer->hints |=  HWC_HINT_CLEAR_FB;
            }
        }
    }
}

static void setCompositionTypeForLayersStageTwo(hwc_display_contents_1_t *primaryContent) {
    for (size_t i=0; i<primaryContent->numHwLayers; i++) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY
#ifdef ENABLE_HWC14_SIDEBAND
            || layer.compositionType == HWC_SIDEBAND
#endif
        ) {
            layer.hints &= ~HWC_HINT_CLEAR_FB;
        }
        if (layer.compositionType == HWC_FRAMEBUFFER) {
            layer.compositionType = HWC_FRAMEBUFFER_SKIP;
        }
    }
}

static void cacheLayerInfo(hwc_context_t* ctx,hwc_display_contents_1_t *primaryContent) {
    LayerCache &LastFrameLayersInfo = ctx->lastFrameLayersInfo;
    LastFrameLayersInfo.cursorLayerCount = 0;
    int fbLayerCount = 0;
    for (unsigned int i=0; i<(primaryContent->numHwLayers-1) && i<MAX_NUM_LAYERS; i++) {
         hwc_layer_1_t &layer = primaryContent->hwLayers[i];
         if (layer.compositionType == HWC_FRAMEBUFFER)
             fbLayerCount++;
         LastFrameLayersInfo.hnd[i] = layer.handle;
         LastFrameLayersInfo.sourceCrop[i] = layer.sourceCrop;
         LastFrameLayersInfo.displayFrame[i] = layer.displayFrame;
         LastFrameLayersInfo.compositionType[i] = layer.compositionType;
         LastFrameLayersInfo.planeAlpha[i] = layer.planeAlpha;
         LastFrameLayersInfo.blending[i] = layer.blending;
         LastFrameLayersInfo.VendorUsageBits[i] = layer.VendorUsageBits;
#ifdef ENABLE_HWC14_CURSOR
         if (layer.flags & HWC_IS_CURSOR_LAYER) {
             LastFrameLayersInfo.bCursorLayer[i] = true;
             LastFrameLayersInfo.cursorLayerCount++;
         } else
             LastFrameLayersInfo.bCursorLayer[i] = false;
#endif
    }
    LastFrameLayersInfo.fbLayerCount = fbLayerCount;
    LastFrameLayersInfo.layerCount = primaryContent->numHwLayers-1;
}
void graphic_prepare_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent, int &videoLayerNumber,int (&videoLayerIndex)[MAX_OVERLAY_LAYER_NUM])
{
    char  property[PROPERTY_VALUE_MAX] = {0};
    bool  hasBootAnimLayer = false;
    int   bootAnimLayerIdx = -1;
    if (property_get(HWCOMPOSER13_DEBUG_PROPERTY, property, NULL) > 0) {
        ctx->HwcomposerDebugType = strtoul(property,NULL,16);
        ALOGD("the HwcomposerDebugType is 0x%x",ctx->HwcomposerDebugType);
    }

    // reset requiredGopOverlycount as zero
    ctx->requiredGopOverlycount = 0;
    ctx->curLayerCounts = primaryContent->numHwLayers;

    // calculate the famecount after the layercount is stable
    if (ctx->lastLayerCounts == ctx->curLayerCounts) {
        ctx->frameCount++;
    } else {
        ctx->frameCount = 0;
        // here reset the layers activeDegree
        for (int i=0; i<ctx->curLayerCounts && i< MAX_NUM_LAYERS; i++) {
         ctx->lastFrameLayersInfo.activeDegree[i] = 0;
        }
        // here reset the mempool Available
        ctx->memAvailable=mem_type_mask;
    }
    ctx->lastLayerCounts = ctx->curLayerCounts;

    // initialize the HWWindowInfo & VideoPlanes
    for (int j=0; j<ctx->gopCount; j++) {
        ctx->HWWindowInfo[j].bEnableGopAFBC = false;
        ctx->HWWindowInfo[j].bAFBCNeedReset = false;
        ctx->HWWindowInfo[j].layerIndex = -1;
        ctx->HWWindowInfo[j].fbid = INVALID_FBID;
        ctx->HWWindowInfo[j].bFrameBufferTarget = false;
        ctx->HWWindowInfo[j].ionBufferSharedFd = -1;
    }
    ctx->bAFBCFramebuffer = false;
#ifdef ENABLE_HWC14_CURSOR
    ctx->hwCusorInfo[0].layerIndex = -1;
#endif
     ctx->debug_LayerCount = primaryContent->numHwLayers;
     for (unsigned int i=0; i<primaryContent->numHwLayers; i++) {
         hwc_layer_1_t &layer = primaryContent->hwLayers[i];
         // add debug info begin
#ifdef ENABLE_HWC14_SIDEBAND
         if (layer.compositionType == HWC_SIDEBAND) {
            ctx->debug_LayerAttr[i].layerType = SIDEBAND_LAYER;
            sideband_handle_t *sidebandStream = (sideband_handle_t *)layer.sidebandStream;
            if (sidebandStream) {
                ctx->debug_LayerAttr[i].sidebandType = sidebandStream->sideband_type;
            }
         } else
#endif
         {
             private_handle_t *handle = (private_handle_t *)layer.handle;
             if (handle) {
                 if (i<MAX_DEBUG_LAYERCOUNT) {
                     ctx->debug_LayerAttr[i].layerType = handle->mstaroverlay;
                     ctx->debug_LayerAttr[i].memType = handle->memTypeInfo;
                     // the last layer is the FramebufferTarget
                     if ((ctx->debug_LayerCount-1)==i) {
                         // LayerTypeToString[5] = ""
                         ctx->debug_LayerAttr[i].layerType= FB_TARGET_LAYER;
                     }
                 }
             } else {
                 if (i<MAX_DEBUG_LAYERCOUNT) {
                     ctx->debug_LayerAttr[i].layerType= NORMAL_LAYER;
                 }
             }
        }
         if ((i!=(primaryContent->numHwLayers-1))
             && strcmp(layer.name, "BootAnimation")==0) {
             hasBootAnimLayer = true;
             bootAnimLayerIdx = i;
         }
         // add debug info end
     }

     setCompositionTypeForLayersStageOne(ctx,primaryContent,videoLayerNumber,videoLayerIndex);
     AllocateHWWindowsVideoPlanesForLayers(ctx,primaryContent,videoLayerIndex,videoLayerNumber);
     bool bneedGlesBlend = checkFrameBufferTargetNeedRedraw(ctx,primaryContent);
     if (!bneedGlesBlend) {
         setCompositionTypeForLayersStageTwo(primaryContent);
     }

     showDebugInfo(ctx, HWC_DEBUGTYPE_SHOW_LAYERS, (void*)primaryContent);
#ifdef ENABLE_HWC14_CURSOR
     ctx->hwcursor_fbNeedRedraw = checkCursorFbNeedRedraw(ctx,primaryContent);
#endif
     // cache operation should behind checkCursorFbNeedRedraw
     cacheLayerInfo(ctx,primaryContent);
     // when bootanimation is showing,should not show  other layers
     if (hasBootAnimLayer) {
         for (int i=0; i<bootAnimLayerIdx; i++) {
             hwc_layer_1_t &layer = primaryContent->hwLayers[i];
             if (layer.compositionType==HWC_FRAMEBUFFER) {
                 layer.compositionType = HWC_OVERLAY;
             }
         }
     }
}
int graphic_commit_display(hwc_context_t* ctx) {
    pthread_mutex_lock(&global_mutex);

    while (queueLength >= MAX_PENDING_QUEUE_COUNT) {
        // exceed queue size, wait for empty slot
        pthread_cond_wait(&full_cond, &global_mutex);
    }

    int index = (startPosition + queueLength) % MAX_PENDING_QUEUE_COUNT;
    memcpy(pendingHWWindowInfoQueue[index], ctx->HWWindowInfo, sizeof(ctx->HWWindowInfo));
    queueLength++;

    pthread_cond_signal(&empty_cond);
    pthread_mutex_unlock(&global_mutex);

    return android::NO_ERROR;
}

int graphic_handle_display(hwc_context_t *ctx, HWWindowInfo_t (&HWWindowInfo)[MAX_HWWINDOW_COUNT]) {
    bool bReOrderVideoGraphicPlanes = false;

    showDebugInfo(ctx, HWC_DEBUGTYPE_SHOW_HWWINDOWINFO, (void*)HWWindowInfo);
    showDebugInfo(ctx, HWC_DEBUGTYPE_SHOW_FPS, NULL);

    for (int j=0; j<ctx->gopCount; j++) {
        if (HWWindowInfo[j].layerIndex >= 0) {
            int acquireFenceFd = HWWindowInfo[j].acquireFenceFd;
            if (acquireFenceFd >= 0 ) {
                int err = sync_wait(acquireFenceFd, 1000); // 1000 ms
                if (err<0) {
                    ALOGE("acquirefence sync_wait failed in work thread");
                }
                close(acquireFenceFd);
            }
        }
    }

    GOP_display(ctx, HWWindowInfo);
    // we should wait the register is update and then signal the release fence
    for (int j=0; j<ctx->gopCount; j++) {
        // trigger release fence
        sw_sync_timeline_inc(HWWindowInfo[j].releaseSyncTimelineFd,
                             (ctx->lastHWWindowInfo[j].releaseFenceFd >= 0 ? 1 : 0));
        if (ctx->lastHWWindowInfo[j].ionBufferSharedFd >= 0) {
            close(ctx->lastHWWindowInfo[j].ionBufferSharedFd);
        }
    }
    memcpy(ctx->lastHWWindowInfo, HWWindowInfo, sizeof(HWWindowInfo));
    return android::NO_ERROR;
}

static void *graphic_work_thread(void *data) {
    hwc_context_t *ctx = (hwc_context_t*)data;
    HWWindowInfo_t newHWWindowInfo[MAX_HWWINDOW_COUNT];
    // set thread name as graphic_workthread
    char thread_name[64] = "graphic_workthread";
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    while (true) {
        pthread_mutex_lock(&global_mutex);
        while (queueLength == 0) {
            pthread_cond_wait(&empty_cond, &global_mutex);
        }
        memcpy(newHWWindowInfo, pendingHWWindowInfoQueue[startPosition], sizeof(newHWWindowInfo));
        startPosition = (startPosition+1) % MAX_PENDING_QUEUE_COUNT;
        queueLength--;
        pthread_cond_signal(&full_cond); // tell main thread a new empty slot
        pthread_mutex_unlock(&global_mutex);
        graphic_handle_display(ctx, newHWWindowInfo);
    }
    return NULL;
}

int graphic_set_primary(hwc_context_t* ctx, hwc_display_contents_1_t *primaryContent) {
    for (int j=0; j<ctx->gopCount; j++) {
        HWWindowInfo_t &hwwindowinfo = ctx->HWWindowInfo[j];
        if (hwwindowinfo.layerIndex >= 0) {
            hwc_layer_1_t &layer = primaryContent->hwLayers[hwwindowinfo.layerIndex];
            private_handle_t *handle = (private_handle_t *)layer.handle;
            if (handle == NULL) {
                return android::UNKNOWN_ERROR;
            }
            hwwindowinfo.physicalAddr = handle->hw_base;
            if (hwwindowinfo.physicalAddr >= mmap_get_miu1_offset()) {
                hwwindowinfo.channel  = 2;
            } else if (hwwindowinfo.physicalAddr >= mmap_get_miu0_offset()) {
                hwwindowinfo.channel  = 1;
            } else {
                hwwindowinfo.channel = 0;
            }
            hwwindowinfo.format = handle->format;
            hwwindowinfo.stride = handle->byte_stride;
            hwwindowinfo.blending = layer.blending;
            hwwindowinfo.planeAlpha = layer.planeAlpha;
            hwwindowinfo.acquireFenceFd = layer.acquireFenceFd;
            layer.acquireFenceFd = -1;
#ifdef MALI_AFBC_GRALLOC
            if ((handle->internal_format & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK))
                && hwwindowinfo.bSupportGopAFBC) {
                hwwindowinfo.bEnableGopAFBC = true;
            }
            //Only have one AFBC Engineer,so we reset it by any HWInfo's status.
            if (ctx->bAFBCNeedReset) {
                hwwindowinfo.bAFBCNeedReset = true;
            }
#endif
            if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
#ifdef ENABLE_SPECIFIED_VISIBLEREGION
                hwwindowinfo.srcX = (unsigned int)layer.sourceCropf.left;
                hwwindowinfo.srcY = (unsigned int)layer.sourceCropf.top;
                hwwindowinfo.srcWidth =  layer.sourceCropf.right - layer.sourceCropf.left;
                hwwindowinfo.srcHeight =  layer.sourceCropf.bottom - layer.sourceCropf.top;

                hwwindowinfo.dstWidth = (hwwindowinfo.srcWidth)*ctx->displayFrameFTScaleRatio_w;
                hwwindowinfo.dstHeight = (hwwindowinfo.srcHeight)*ctx->displayFrameFTScaleRatio_h;

                hwwindowinfo.dstX = (unsigned int)layer.sourceCropf.left*ctx->displayFrameFTScaleRatio_w;
                hwwindowinfo.dstY = (unsigned int)layer.sourceCropf.top*ctx->displayFrameFTScaleRatio_h;
#else
                hwwindowinfo.srcX = (unsigned int)layer.displayFrame.left;
                hwwindowinfo.srcY = (unsigned int)layer.displayFrame.top;
                hwwindowinfo.srcWidth =  layer.displayFrame.right - layer.displayFrame.left;
                hwwindowinfo.srcHeight =  layer.displayFrame.bottom - layer.displayFrame.top;

                hwwindowinfo.dstWidth = ctx->curTimingWidth;
                hwwindowinfo.dstHeight = ctx->curTimingHeight;

                hwwindowinfo.dstX = 0;
                hwwindowinfo.dstY = 0;
#endif
                hwwindowinfo.bufferWidth = hwwindowinfo.srcWidth;
                hwwindowinfo.bufferHeight = hwwindowinfo.srcHeight;
            } else if (layer.compositionType == HWC_OVERLAY) {
                hwwindowinfo.bufferWidth = handle->width;
                hwwindowinfo.bufferHeight = handle->height;
                hwwindowinfo.srcX = (unsigned int)layer.sourceCropf.left;
                hwwindowinfo.srcY = (unsigned int)layer.sourceCropf.top;
                hwwindowinfo.srcWidth = (unsigned int)layer.sourceCropf.right - (unsigned int)layer.sourceCropf.left;
                hwwindowinfo.srcHeight = (unsigned int)layer.sourceCropf.bottom - (unsigned int)layer.sourceCropf.top;

                hwwindowinfo.dstX = layer.displayFrame.left*ctx->displayFrameFTScaleRatio_w + 0.5;
                hwwindowinfo.dstY = layer.displayFrame.top*ctx->displayFrameFTScaleRatio_h + 0.5;
                hwwindowinfo.dstWidth = (layer.displayFrame.right - layer.displayFrame.left)*ctx->displayFrameFTScaleRatio_w + 0.5;
                hwwindowinfo.dstHeight = (layer.displayFrame.bottom - layer.displayFrame.top)*ctx->displayFrameFTScaleRatio_h + 0.5;

            }
        }
    }
#ifdef MALI_AFBC_GRALLOC
    if (ctx->bAFBCNeedReset) {
        ctx->bAFBCNeedReset = false;
    }
#endif
    for (unsigned int i=0;i<primaryContent->numHwLayers;i++) {
        hwc_layer_1_t &layer = primaryContent->hwLayers[i];
        if (layer.acquireFenceFd!=-1) {
            sync_wait(layer.acquireFenceFd, 1000);
            close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
        }
    }

    for (int j=0; j<ctx->gopCount; j++) {
        HWWindowInfo_t &hwwindowinfo = ctx->HWWindowInfo[j];
        if (hwwindowinfo.layerIndex >= 0) {
            hwwindowinfo.releaseFenceValue++;
            char str[64];
            sprintf(str, "releaseFence %d", hwwindowinfo.releaseFenceValue);
            hwwindowinfo.releaseFenceFd = sw_sync_fence_create( hwwindowinfo.releaseSyncTimelineFd,str, hwwindowinfo.releaseFenceValue);
            if (hwwindowinfo.releaseFenceFd < 0) {
                ALOGE("fence dup failed: %s,str : %s", strerror(errno),str);
            }
            hwc_layer_1_t &layer = primaryContent->hwLayers[hwwindowinfo.layerIndex];
            private_handle_t *handle = (private_handle_t *)layer.handle;
            if (handle->share_fd >= 0) {
                hwwindowinfo.ionBufferSharedFd = dup(handle->share_fd);
                if (hwwindowinfo.ionBufferSharedFd < 0)
                    ALOGD("error dup handle->share_fd failed.....");
            }
        } else {
            hwwindowinfo.releaseFenceFd = -1;
        }
    }

    int result = graphic_commit_display(ctx);  // work thread will signal releaseFence in last composition

    if (result == android::NO_ERROR) {
        for (int j=0; j<ctx->gopCount; j++) {
            HWWindowInfo_t &hwwindowinfo = ctx->HWWindowInfo[j];
            if (hwwindowinfo.layerIndex >= 0) {
                hwc_layer_1_t &layer = primaryContent->hwLayers[hwwindowinfo.layerIndex];
                layer.releaseFenceFd = hwwindowinfo.releaseFenceFd;
                if (primaryContent->retireFenceFd < 0) {
                    primaryContent->retireFenceFd = dup(layer.releaseFenceFd);
                    if (primaryContent->retireFenceFd < 0) {
                        ALOGE("fence dup failed: %s", strerror(errno));
                    }
                } else {
                    int merged = sync_merge("primary", primaryContent->retireFenceFd, layer.releaseFenceFd);
                    if (merged < 0) {
                        ALOGE("fence merge failed: %s", strerror(errno));
                    } else {
                        close(primaryContent->retireFenceFd);
                        primaryContent->retireFenceFd = merged;
                    }
                }
            }
        }
    }

    return result;
}

void graphic_close_primary(hwc_context_t* ctx)
{
    // TODO: kill and join work thread
    // TODO: release fence, close timeline
    for (int j=0; j<ctx->gopCount; j++) {
        if (ctx->HWWindowInfo[j].fbid != INVALID_FBID) {
            MApi_GOP_GWIN_Delete32FB(ctx->HWWindowInfo[j].fbid);
        }
    }
}

int graphic_getCurOPTiming(int *ret_width, int *ret_height, int *ret_hstart) {
    GOP_getCurOPTiming(ret_width, ret_height, ret_hstart);
    return 0;
}

int graphic_recalc3DTransform(hwc_context_t* ctx,hwc_3d_param_t* params, hwc_rect_t frame) {
    int current3DMode = DISPLAYMODE_NORMAL;
    static int old3DMode = DISPLAYMODE_NORMAL;
    char property[PROPERTY_VALUE_MAX] = {0};
    if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
        current3DMode = atoi(property);
        if (current3DMode < 0 || current3DMode >= DISPLAYMODE_MAX) {
            current3DMode = DISPLAYMODE_NORMAL;
        }
    } else {
        current3DMode = DISPLAYMODE_NORMAL;
    }
    ctx->current3DMode = current3DMode;

    if (current3DMode != old3DMode) {
        ctx->b3DModeChanged= true;
        old3DMode = current3DMode;
    }
    switch (current3DMode) {
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_LEFTRIGHT_FR:
            //left
            params[0].isVisiable = 1;
            params[0].scaleX = 0.5f;
            params[0].scaleY = 1.0f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //right
            params[1].isVisiable = 1;
            params[1].scaleX = 0.5f;
            params[1].scaleY = 1.0f;
            params[1].TanslateX = (float)(frame.right - frame.left)/2;
            params[1].TanslateY = 0.0f;
            break;
        case DISPLAYMODE_LEFT_ONLY:
            //left
            params[0].isVisiable = 1;
            params[0].scaleX = 0.5f;
            params[0].scaleY = 1.0f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //right
            params[1].isVisiable = 0;
            break;
        case DISPLAYMODE_RIGHT_ONLY:
            //left
            params[0].isVisiable = 0;
            //right
            params[1].isVisiable = 1;
            params[1].scaleX = 0.5f;
            params[1].scaleY = 1.0f;
            params[1].TanslateX = (float)(frame.right - frame.left)/2;
            params[1].TanslateY = 0.0f;
            break;
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOPBOTTOM_LA:
            //top
            params[0].isVisiable = 1;
            params[0].scaleX = 1.0f;
            params[0].scaleY = 0.5f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //bottom
            params[1].isVisiable = 1;
            params[1].scaleX = 1.0f;
            params[1].scaleY = 0.5f;
            params[1].TanslateX = 0.0f;
            params[1].TanslateY = (float)(frame.bottom - frame.top)/2;
            break;
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_FRAME_ALTERNATIVE_LLRR:
            //top
            params[0].isVisiable = 1;
            params[0].scaleX = 1.0f;
            params[0].scaleY = 0.5f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //bottom
            params[1].isVisiable = 0;
            break;
        case DISPLAYMODE_BOTTOM_LA:
        case DISPLAYMODE_BOTTOM_ONLY:
            //top
            params[0].isVisiable = 0;
            //bottom
            params[1].isVisiable = 1;
            params[1].scaleX = 1.0f;
            params[1].scaleY = 0.5f;
            params[1].TanslateX = 0.0f;
            params[1].TanslateY = (float)(frame.bottom - frame.top)/2;
            break;
        default:
            //top
            params[0].isVisiable = 0;
            //bottom
            params[1].isVisiable = 0;
    }
    return 0;
}

unsigned int graphic_getGOPDestination(hwc_context_t* ctx)
{
    return GOP_getDestination(ctx);
}

bool graphic_checkGopTimingChanged(hwc_context_t* ctx)
{
    bool timing_changed = false;
    int OPTimingWidth = ctx->curTimingWidth;
    int OPTimingHeight = ctx->curTimingHeight;
    int PanelHstart = ctx->panelHStart;
    int OCTimingWidth = ctx->curTimingWidth;
    int OCTimingHeight = ctx->curTimingHeight;

    // check gop dest changed
    unsigned int gop_dest = GOP_getDestination(ctx);
    if (gop_dest!=ctx->HWWindowInfo[0].gop_dest) {
        if (gop_dest == E_GOP_DST_OP0) {
            OPTimingHeight = 0;
            OPTimingWidth = 0;
        } else if (gop_dest == E_GOP_DST_FRC) {
            OCTimingWidth = 0;
            OCTimingHeight = 0;
        }
        for (int i=0; i<ctx->gopCount; i++) {
            MApi_GOP_GWIN_SetGOPDst(ctx->HWWindowInfo[i].gop, (EN_GOP_DST_TYPE)gop_dest);
            ctx->HWWindowInfo[i].gop_dest = gop_dest;
        }
    }

    // check timing changed
    if (gop_dest == E_GOP_DST_OP0) {
        int width = 0;
        int height = 0;
        int hstart = 0;
        GOP_getCurOPTiming(&width, &height, &hstart);

        if ((width != OPTimingWidth)||(height != OPTimingHeight)||(hstart != PanelHstart)) {
            for (int i=0; i<ctx->gopCount; i++) {
                ST_GOP_TIMING_INFO stGopTiming;
                memset(&stGopTiming, 0, sizeof(ST_GOP_TIMING_INFO));
                MApi_GOP_GetConfigEx(ctx->HWWindowInfo[i].gop, E_GOP_TIMING_INFO, (MS_U32*)&stGopTiming);
                stGopTiming.u16DEHSize = width;
                stGopTiming.u16DEVSize = height;
                stGopTiming.u16DEHStart = hstart;
                MApi_GOP_SetConfigEx(ctx->HWWindowInfo[i].gop, E_GOP_TIMING_INFO, (MS_U32*)&stGopTiming);
                ALOGE("the new timing is %d :%d, hstar is %d \n",width,height,hstart);
            }
            OPTimingWidth = width;
            OPTimingHeight = height;
            PanelHstart = hstart;
            ctx->curTimingWidth = OPTimingWidth;
            ctx->curTimingHeight = OPTimingHeight;
            ctx->panelHStart = PanelHstart;
            timing_changed = true;
#ifdef MALI_AFBC_GRALLOC
            ctx->bAFBCNeedReset = true;
#endif
        }
    } else if (gop_dest == E_GOP_DST_FRC) {
        int width = 0;
        int height = 0;
        int hstart = 0;
        GOP_getCurOCTiming(&width,&height,&hstart);

        if ((width != OCTimingWidth)||(height != OCTimingHeight)) {
            OCTimingWidth = width;
            OCTimingHeight = height;
            ctx->curTimingWidth = OCTimingWidth;
            ctx->curTimingHeight = OCTimingHeight;
            ctx->panelHStart = PanelHstart;
            timing_changed = true;
#ifdef MALI_AFBC_GRALLOC
            ctx->bAFBCNeedReset = true;
#endif
        }
    }
   return timing_changed;
}

void getTravelingRes(int *width, int *height) {
    if (*width >= 720 && *width < 1280 && *height >= 480 && *height < 576) {
        *width = 720;
        *height = 480;
    } else if (*width >= 720 && *width < 1280 && *height >= 576 && *height < 720) {
        *width = 720;
        *height = 576;
    } else if (*width >= 1280 && *width < 1920 && *height >= 720 && *height < 1080) {
        *width = 1280;
        *height = 720;
    } else if (*width >= 1920 && *height >= 1080) {
        *width = 1920;
        *height = 1080;
    } else {
        ALOGD("Abnormal timing! Return 720*480, real timing is: %d*%d!", *width, *height);
        *width = 720;
        *height = 480;
    }
}

int graphic_captureScreenWithClip(hwc_context_t* ctx,
                                            size_t numData, hwc_capture_data_t* captureData,
                                            int* width, int* height, int withOsd) {
    char property[PROPERTY_VALUE_MAX] = {0};
    /* mstar.debug.capturedata ,capture debug option property
    * 1:skip dip flow
    * 2:fill buffer with pattern
    * 3:save raw data.
    */
    property_get("mstar.debug.capturedata", property, "0");
    if (strcmp(property, "1") == 0) {
        ALOGD("[capture screen]: dip scpture screen skip!!!!!");
        return FALSE;
    }
    graphic_getCurOPTiming(width,height,NULL);
    getTravelingRes(width,height);
    ALOGD("[capture screen]:startCapture ctx->captureScreenAddr = %llx,capture width = %d,height = %d"
        ,(long long unsigned int)ctx->captureScreenAddr,*width,*height);
    int ret = DIP_startCaptureScreenWithClip(ctx,numData,captureData,*width,*height,withOsd);
    //add debug infomation :save capture data to /data/capture/
    if (strcmp(property, "3") == 0) {
        char filename[100];
        int num  = -1;
        while (++num < 10000) {
            snprintf(filename, 100, "%s/%s_%04d.raw",
                     "/data/capture/","captureData",  num);

            if (access(filename, F_OK) != 0) {
                snprintf(filename, 100, "%s/%s_%04d.raw",
                         "/data/capture","captureData",  num);

                if (access(filename, F_OK) != 0)
                    break;
            }
        }
        ALOGD("[capture screen]:captureData filename  = %s",filename);
        int fd_p = open(filename, O_EXCL | O_CREAT | O_WRONLY, 0644);
        if (fd_p < 0) {
            ALOGD("[capture screen]:%s  can't open",filename);
            return 0;
        }
        write(fd_p, (void *)ctx->captureScreenAddr, (*width)*(*height)*4);
        close(fd_p);
    }
    return ret;
}

int graphic_startCapture(hwc_context_t* ctx, int* width, int* height, int withOsd) {
    char property[PROPERTY_VALUE_MAX] = {0};
    /* mstar.debug.capturedata ,capture debug option property
    * 1:skip dip flow
    * 2:fill buffer with pattern
    * 3:save raw data.
    */
    property_get("mstar.debug.capturedata", property, "0");
    if (strcmp(property, "1") == 0) {
        ALOGD("[capture screen]: dip scpture screen skip!!!!!");
        return FALSE;
    }
    graphic_getCurOPTiming(width,height,NULL);
    getTravelingRes(width,height);
    ALOGD("[capture screen]:startCapture ctx->captureScreenAddr = %llx,capture width = %d,height = %d"
        ,(long long unsigned int)ctx->captureScreenAddr,*width,*height);
    int ret = DIP_startCaptureScreen(ctx,*width,*height,withOsd);
    //add debug infomation :fill buffer with random number
    if (strcmp(property, "2") == 0) {
        for (int i=0; i<*height; i++) {
            uint8_t* date = ((uint8_t *)ctx->captureScreenAddr)+i*(*width)*4;
            memset(date, rand() % 256, (*width)*4);
            ret = 1;
        }
    }
    //add debug infomation :save capture data to /data/capture/
    if (strcmp(property, "3") == 0) {
        char filename[100];
        int num  = -1;
        while (++num < 10000) {
            snprintf(filename, 100, "%s/%s_%04d.raw",
                     "/data/capture/","captureData",  num);

            if (access(filename, F_OK) != 0) {
                snprintf(filename, 100, "%s/%s_%04d.raw",
                         "/data/capture","captureData",  num);

                if (access(filename, F_OK) != 0)
                    break;
            }
        }
        ALOGD("[capture screen]:captureData filename  = %s",filename);
        int fd_p = open(filename, O_EXCL | O_CREAT | O_WRONLY, 0644);
        if (fd_p < 0) {
            ALOGD("[capture screen]:%s  can't open",filename);
            return 0;
        }
        write(fd_p, (void *)ctx->captureScreenAddr, (*width)*(*height)*4);
        close(fd_p);
    }
    return ret;
}

int graphic_releaseCaptureScreenMemory(hwc_context_t* ctx) {
    return DIP_releaseCaptureScreenMemory(ctx);
}
