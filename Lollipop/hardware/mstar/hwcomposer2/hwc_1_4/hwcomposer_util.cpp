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
#define DFBRC_INI_FILENAME          "/config/dfbrc.ini"
#define HWCOMPOSER13_INI_FILENAME   "/system/etc/hwcomposer13.ini"

#include <cutils/log.h>
#include <gralloc_priv.h>
#include <cutils/properties.h>

#include <iniparser.h>
#include <tvini.h>
#include <mmap.h>
#include <drvSYS.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <drvGPIO.h>
#include <apiXC.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <drvCMDQ.h>

#include "hwcomposer_context.h"
#include "hwcomposer_util.h"
#ifdef ENABLE_HWC14_SIDEBAND
#include <hardware/tv_input.h>
#include <mstar_tv_input_sideband.h>
#endif

int openConfigFile(dictionary **ppSysINI, dictionary **ppModelINI, dictionary **ppPanelINI,
                               dictionary **ppModuleINI, dictionary **ppDfbrcFile, dictionary **ppHwcINI)
{
    char *pDfbrcName = NULL, *pModuleName = NULL;
    dictionary *pSysINI = NULL, *pModelINI = NULL, *pPanelINI = NULL, *pModuleINI = NULL, *pHwcINI = NULL, *pDfbrcINI = NULL;

    //load sys.ini, model.ini and panel.ini file
    if(tvini_init(ppSysINI, ppModelINI, ppPanelINI)==-1)
        return -1;
    pSysINI = *ppSysINI;
    pModelINI = *ppModelINI;
    pPanelINI = *ppPanelINI;

    //load hwcomposer13.ini file
    pHwcINI = iniparser_load(HWCOMPOSER13_INI_FILENAME);
    if (pHwcINI == NULL) {
        tvini_free(pSysINI, pModelINI, pPanelINI);
        ALOGE("Load %s failed!", HWCOMPOSER13_INI_FILENAME);
        return -1;
    }

    //load module.ini file
    pModuleName = iniparser_getstr(pModelINI, "module:m_pModuleName");
    pModuleINI = iniparser_load(pModuleName);
    if (pModuleINI == NULL) {
        tvini_free(pSysINI, pModelINI, pPanelINI);
        iniparser_freedict(pHwcINI);
        ALOGE("Load %s failed!", pModuleName);
        return -1;
    }
    ALOGD("Load module.ini file successfully and pModuleName is %s", pModuleName);

    //load dfbrc file
    pDfbrcName = iniparser_getstr(pModelINI, "dfbrc:m_pDFBrcName");
    if (pDfbrcName) {
        pDfbrcINI = iniparser_load(pDfbrcName);
        ALOGD("the dfbrcName name is %s",pDfbrcName);
    } else {
        pDfbrcINI = iniparser_load(DFBRC_INI_FILENAME);
    }
    if (pDfbrcINI == NULL) {
        ALOGE("Load DFBRC_INI_PATH_FILENAME fail");
        tvini_free(pSysINI, pModelINI, pPanelINI);
        iniparser_freedict(pHwcINI);
        iniparser_freedict(pModuleINI);
        return -1;
    }
    *ppModuleINI = pModuleINI;
    *ppDfbrcFile = pDfbrcINI;
    *ppHwcINI = pHwcINI;
    return 0;
}

void closeConfigFile(dictionary *pSysINI, dictionary *pModelINI, dictionary *pPanelINI,
                               dictionary *pModuleINI, dictionary *pDfbrcFile, dictionary *pHwcINI)
{
    if(pDfbrcFile!=NULL) {
        iniparser_freedict(pDfbrcFile);
    }
    if(pHwcINI!=NULL) {
        iniparser_freedict(pHwcINI);
    }
    if(pModuleINI!=NULL) {
        iniparser_freedict(pModuleINI);
    }
    tvini_free(pSysINI, pModelINI, pPanelINI);
}

void getMirrorMode(dictionary *pModelINI, int *mirrorModeH, int *mirrorModeV)
{
    int mirrorMode = iniparser_getboolean(pModelINI, "MISC_MIRROR_CFG:MIRROR_OSD", 0);
    int osdMirrorType = iniparser_getint(pModelINI, "MISC_MIRROR_CFG:MIRROR_OSD_TYPE", 0);
    if (mirrorMode == 1) {
        switch (osdMirrorType) {
            case 1:
                *mirrorModeH = 1;
                *mirrorModeV = 0;
                break;
            case 2:
                *mirrorModeH = 0;
                *mirrorModeV = 1;
                break;
            case 3:
                *mirrorModeH = 1;
                *mirrorModeV = 1;
                break;
            default:
                //if OSD mirror enable but not setting mirror type, then setting HV-mirror.
                ALOGE("Mirror Type = %d. it is incorrect! repair to HV-mirror\n",osdMirrorType);
                *mirrorModeH = 1;
                *mirrorModeV = 1;
                break;
        }
    } else {
        *mirrorModeH = 0;
        *mirrorModeV = 0;
    }
}

const char *memType2String(unsigned int memType) {
    switch (memType) {
        case mem_type_cma_contiguous_miu0:
            return "CMA_MIU0_CONTIGUOUS";
        case mem_type_cma_contiguous_miu1:
            return "CMA_MIU1_CONTIGUOUS";
        case mem_type_cma_contiguous_miu2:
            return "CMA_MIU2_CONTIGUOUS";
        case mem_type_cma_discrete_miu0:
            return "CMA_MIU0_DISCRETE";
        case mem_type_cma_discrete_miu1:
            return "CMA_MIU1_DISCRETE";
        case mem_type_cma_discrete_miu2:
            return "CMA_MIU2_DISCRETE";
        case mem_type_system_heap:
        default:
            return "SYSTEM_HEAP";
    }
}

const char *sidebandType2String(unsigned int sidebandType) {
    switch (sidebandType) {
#ifdef ENABLE_HWC14_SIDEBAND
        case TV_INPUT_SIDEBAND_TYPE_TV:
            return "SIDEBAND_TYPE_TV";
        case TV_INPUT_SIDEBAND_TYPE_MM_XC:
            return "SIDEBAND_TYPE_MM_XC";
        case TV_INPUT_SIDEBAND_TYPE_MM_GOP:
            return "SIDEBAND_TYPE_MM_GOP";
#endif
        default:
            return "SIDEBAND_UNKNOW";
    }
}

void STB_setResolutionState() {
    char property[PROPERTY_VALUE_MAX] = {0};
    char reslutionstate[PROPERTY_VALUE_MAX] = {0};

    if (property_get("mstar.resolution", property, NULL) > 0) {
        ALOGD("STB_setResolutionState resolution = %s\n",property);
        if ((strcmp(property,"HDMITX_RES_1920x1080p_60Hz") == 0)||
            (strcmp(property,"HDMITX_RES_1920x1080p_50Hz") == 0)||
            (strcmp(property,"HDMITX_RES_1920x1080p_30Hz")==0)||
            (strcmp(property,"HDMITX_RES_1920x1080p_25Hz") ==0)||
            (strcmp(property,"HDMITX_RES_1920x1080p_24Hz")==0)||
            (strcmp(property,"DACOUT_1080P_60")==0) ||
            (strcmp(property,"DACOUT_1080P_50")==0) ||
            (strcmp(property,"DACOUT_1080P_30")==0) ||
            (strcmp(property,"DACOUT_1080P_25")==0) ||
            (strcmp(property,"DACOUT_1080P_24")==0) ||
            (strcmp(property,"RAPTORS_RES_1920x1080p_60Hz")==0) ||
            (strcmp(property,"RAPTORS_RES_1920x1080p_50Hz")==0)) {
            strncpy(reslutionstate,"RESOLUTION_1080P",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_1920x1080i_60Hz") == 0) ||
                   (strcmp(property, "HDMITX_RES_1920x1080i_50Hz") == 0) ||
                   (strcmp(property, "HDMITX_RES_1920x1080i_30Hz") == 0) ||
                   (strcmp(property,"DACOUT_1080I_60") == 0) ||
                   (strcmp(property, "DACOUT_1080I_50") == 0) ||
                   (strcmp(property, "DACOUT_1080I_30") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_1080I",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_1280x720p_60Hz") == 0)||
                   (strcmp(property,"HDMITX_RES_1280x720p_50Hz") == 0)||
                   (strcmp(property,"DACOUT_720P_60") == 0)||
                   (strcmp(property,"DACOUT_720P_50") == 0)||
                   (strcmp(property,"RAPTORS_RES_1280x720p_60Hz") == 0)||
                   (strcmp(property,"RAPTORS_RES_1280x720p_50Hz") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_720P",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_720x576p") == 0)||
                   (strcmp(property,"DACOUT_576P_50") == 0)||
                   (strcmp(property,"RAPTORS_RES_720x576p_50Hz") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_576P",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_720x576i") == 0)||
                   (strcmp(property,"DACOUT_576I_50") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_576I",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_720x480p") == 0)||
                   (strcmp(property,"DACOUT_480P_60") == 0)||
                   (strcmp(property,"RAPTORS_RES_720x480p_60Hz") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_480P",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_720x480i") == 0)||
                   (strcmp(property,"DACOUT_480I_60") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_480I",PROPERTY_VALUE_MAX-1);
        } else if ((strcmp(property,"HDMITX_RES_4K2Kp_60Hz") == 0)||
                   (strcmp(property,"HDMITX_RES_4K2Kp_50Hz") == 0)||
                   (strcmp(property,"HDMITX_RES_4K2Kp_30Hz") == 0)||
                   (strcmp(property,"HDMITX_RES_4K2Kp_25Hz") == 0)||
                   (strcmp(property,"HDMITX_RES_4K2Kp_24Hz") == 0)||
                   (strcmp(property,"DACOUT_4K2KP_60") == 0)||
                   (strcmp(property,"DACOUT_4K2KP_50") == 0)||
                   (strcmp(property,"DACOUT_4K2KP_30") == 0)||
                   (strcmp(property,"DACOUT_4K2KP_25") == 0)||
                   (strcmp(property,"DACOUT_4K2KP_24") == 0)||
                   (strcmp(property,"RAPTORS_RES_4K2Kp_60Hz") == 0)||
                   (strcmp(property,"RAPTORS_RES_4K2Kp_50Hz") == 0)||
                   (strcmp(property,"RAPTORS_RES_4K2Kp_30Hz") == 0)||
                   (strcmp(property,"RAPTORS_RES_4K2Kp_25Hz") == 0)||
                   (strcmp(property,"RAPTORS_RES_4K2Kp_24Hz") == 0)) {
            strncpy(reslutionstate,"RESOLUTION_4K2KP",PROPERTY_VALUE_MAX-1);
        }
        property_set("mstar.resolutionState", reslutionstate);
    }
}
bool  rect_eq(hwc_rect_t rect1,hwc_rect_t rect2) {
    if ((rect1.left==rect2.left)&&
        (rect1.right==rect2.right)&&
        (rect1.top==rect2.top)&&
        (rect1.bottom==rect2.bottom)) {
        return  true;
    } else {
        return false;
    }
}

void showDebugInfo(hwc_context_t* ctx, unsigned long type, void *param)
{
    unsigned long debugmode = ctx->HwcomposerDebugType & type;
    switch(debugmode) {
        case HWC_DEBUGTYPE_SHOW_HWWINDOWINFO: {
            HWWindowInfo_t *HWWindowInfo = (HWWindowInfo_t *)param;
            for (int j=0; j<ctx->gopCount; j++) {
                ALOGD("last window %d: gop=%d gwin=%d miu=%d addr=%llx fbid=%d bufferW=%d bufferH=%d \
                format=%d stride=%d layerindex=%d srcx=%d srcY=%d srcWidth=%d srcHeight=%d dstX=%d dstY=%d dstWidth=%d dstHeight=%d\n",
                      j, ctx->lastHWWindowInfo[j].gop, ctx->lastHWWindowInfo[j].gwin,
                      ctx->lastHWWindowInfo[j].channel, ctx->lastHWWindowInfo[j].physicalAddr,
                      ctx->lastHWWindowInfo[j].fbid,
                      ctx->lastHWWindowInfo[j].bufferWidth, ctx->lastHWWindowInfo[j].bufferHeight,
                      ctx->lastHWWindowInfo[j].format, ctx->lastHWWindowInfo[j].stride,
                      ctx->lastHWWindowInfo[j].layerIndex,
                      ctx->lastHWWindowInfo[j].srcX,ctx->lastHWWindowInfo[j].srcY,
                      ctx->lastHWWindowInfo[j].srcWidth,ctx->lastHWWindowInfo[j].srcHeight,
                      ctx->lastHWWindowInfo[j].dstX,ctx->lastHWWindowInfo[j].dstY,
                      ctx->lastHWWindowInfo[j].dstWidth,ctx->lastHWWindowInfo[j].dstHeight);

                ALOGD("current window %d: gop=%d gwin=%d miu=%d addr=%llx fbid=%d bufferW=%d bufferH=%d \
                format=%d stride=%d layerindex=%d srcx=%d srcY=%d srcWidth=%d srcHeight=%d dstX=%d dstY=%d dstWidth=%d dstHeight=%d\n",
                      j, HWWindowInfo[j].gop, HWWindowInfo[j].gwin,
                      HWWindowInfo[j].channel, HWWindowInfo[j].physicalAddr,
                      HWWindowInfo[j].fbid,
                      HWWindowInfo[j].bufferWidth, HWWindowInfo[j].bufferHeight,
                      HWWindowInfo[j].format, HWWindowInfo[j].stride,
                      HWWindowInfo[j].layerIndex,
                      HWWindowInfo[j].srcX,HWWindowInfo[j].srcY,
                      HWWindowInfo[j].srcWidth,HWWindowInfo[j].srcHeight,
                      HWWindowInfo[j].dstX,HWWindowInfo[j].dstY,
                      HWWindowInfo[j].dstWidth,HWWindowInfo[j].dstHeight);
            }
        }
        break;

        case HWC_DEBUGTYPE_SHOW_FPS: {
            static int frames = 0;
            static struct timeval begin;
            static struct timeval end;
            if (frames==0) {
                gettimeofday(&begin,NULL);
                end = begin;
            }
            frames++;
            if (frames%100==0) {
                gettimeofday(&end,NULL);
                float fps = 100.0*1000/((end.tv_sec-begin.tv_sec)*1000+(end.tv_usec-begin.tv_usec)/1000);
                begin = end;
                ALOGI("from hwcomposer13================the fps is %f",fps);
            }
        }
        break;

        case HWC_DEBUGTYPE_SHOW_LAYERS: {
            hwc_display_contents_1_t *primaryContent = (hwc_display_contents_1_t *)param;
            ALOGD("the visible layer num is %d",primaryContent->numHwLayers);
            for (unsigned int i=0; i<(primaryContent->numHwLayers-1); i++) {
                hwc_layer_1_t &layer = primaryContent->hwLayers[i];
                ALOGD("layer[%d]:name: %s",i,layer.name);
                ALOGD("layer[%d]:compositionType:%d:hints:%d:flags:%d:handle:%p",i,layer.compositionType,layer.hints,layer.flags,layer.handle);
                ALOGD("layer[%d]:transform:%u blending:%d",i,layer.transform,layer.blending);
                ALOGD("layer[%d]:sourceCrop,left:%f right:%f,top:%f bottom:%f",i,layer.sourceCropf.left,layer.sourceCropf.right,layer.sourceCropf.top,layer.sourceCropf.bottom);
                ALOGD("layer[%d]:displayFrame,left:%d right:%d top:%d bottom:%d",i,layer.displayFrame.left,layer.displayFrame.right,layer.displayFrame.top,layer.displayFrame.bottom);
                ALOGD("layer[%d]:visibleRegion counts:%d Region left:%d right:%d top:%d bottom:%d:",i,layer.visibleRegionScreen.numRects,layer.visibleRegionScreen.rects[0].left,layer.visibleRegionScreen.rects[0].right,layer.visibleRegionScreen.rects[0].top,layer.visibleRegionScreen.rects[0].bottom);
                ALOGD("layer[%d]:planeAlpha:%d\n\n",i,layer.planeAlpha);
                }
            }
        break;
    }
}

void XC_GFX_init()
{
    MDrv_SYS_GlobalInit();
    mdrv_gpio_init();
    MsOS_MPool_Init();
    MsOS_Init();
    MApi_PNL_IOMapBaseInit();
    XC_INITDATA XC_InitData;

    GFX_Config geConfig;
    geConfig.u8Miu = 0;
    geConfig.u8Dither = FALSE;
    geConfig.u32VCmdQSize = 0;
    geConfig.u32VCmdQAddr = 0;
    geConfig.bIsHK = 1;
    geConfig.bIsCompt = FALSE;
    MApi_GFX_Init(&geConfig);

    memset((void*)&XC_InitData, 0, sizeof(XC_InitData));
    MApi_XC_Init(&XC_InitData, 0);
    // setup the gop driver local structure
    MApi_GOP_InitByGOP(NULL, 0);
}

void CMDQ_and_MLoad_init()
{
    const mmap_info_t* mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_MMAP_ID_CMDQ");
    if (mmapInfo != NULL) {
        ALOGD("miuno = %d, addr = %p,size = %lx",mmapInfo->miuno,mmapInfo->addr,mmapInfo->size);
        MS_BOOL ret = TRUE;
        MS_PHY miuOffset = 0;
        if (mmapInfo->miuno == 0) {
            ret = MsOS_MPool_Mapping(0, mmapInfo->addr,  mmapInfo->size, 1);
        } else if (mmapInfo->miuno == 1) {
            miuOffset = mmap_get_miu0_offset();
            ret = MsOS_MPool_Mapping(1, (mmapInfo->addr - mmap_get_miu0_offset()),  mmapInfo->size, 1);
        } else if (mmapInfo->miuno == 2) {
            miuOffset = mmap_get_miu1_offset();
            ret = MsOS_MPool_Mapping(2, (mmapInfo->addr - mmap_get_miu1_offset()),  mmapInfo->size, 1);
        }
        if (ret) {
            ret = MDrv_CMDQ_Init(mmapInfo->miuno);
            ret = MDrv_CMDQ_Get_Memory_Size((mmapInfo->addr - miuOffset), (mmapInfo->addr - miuOffset) + mmapInfo->size, mmapInfo->miuno);
        }
    } else {
        ALOGD("there is no E_MMAP_ID_CMDQ in mmap.ini");
    }

    mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_MMAP_ID_XC_MLOAD");
    if (mmapInfo != NULL) {
        MS_BOOL ret = TRUE;
        if (mmapInfo->miuno == 0) {
            ret = MsOS_MPool_Mapping_Dynamic(mmapInfo->miuno,mmapInfo->addr,mmapInfo->size,1);
        } else if (mmapInfo->miuno == 1) {
            ret = MsOS_MPool_Mapping_Dynamic(mmapInfo->miuno,(mmapInfo->addr - mmap_get_miu0_offset()),mmapInfo->size,1);
        } else if (mmapInfo->miuno == 2) {
            ret = MsOS_MPool_Mapping_Dynamic(mmapInfo->miuno,(mmapInfo->addr - mmap_get_miu1_offset()),mmapInfo->size,1);
        }
        if (ret) {
            MLG_TYPE mglresult = E_MLG_UNSUPPORTED;
            mglresult = MApi_XC_MLG_GetStatus();
            if (E_MLG_ENABLED == mglresult) {
                ALOGD("the menuload is enabled %s :%d",__FUNCTION__,__LINE__);
            } else {
                ALOGD("the menuload is not enabled or not support  %s %d",__FUNCTION__,__LINE__);
            }
        }
    } else {
        ALOGD("no E_MMAP_ID_XC_MLOAD in mmap.ini %s :%d",__FUNCTION__,__LINE__);
    }
}

void GFX_ClearFbBuffer()
{
    const mmap_info_t* mmapInfo = NULL;
    //clear GOP_FB use GE function
    unsigned int maskPhyAddr = 0;
    mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
    if (mmapInfo != NULL) {
        if (mmapInfo->cmahid > 0 ) {
            ALOGI("E_MMAP_ID_GOP_FB was integrated into CMA");
        } else {
            if (mmapInfo->miuno == 0) {
                maskPhyAddr = mmap_get_miu0_length() - 1;
                MApi_GFX_ClearFrameBufferByWord((mmapInfo->addr&maskPhyAddr), mmapInfo->size, 0);
            } else if (mmapInfo->miuno == 1) {
                maskPhyAddr = mmap_get_miu1_length() - 1;
                MApi_GFX_ClearFrameBufferByWord((mmapInfo->addr&maskPhyAddr)|mmap_get_miu0_offset(), mmapInfo->size, 0);
            } else if (mmapInfo->miuno == 2) {
                maskPhyAddr = mmap_get_miu2_length() - 1;
                MApi_GFX_ClearFrameBufferByWord((mmapInfo->addr&maskPhyAddr)|mmap_get_miu1_offset(), mmapInfo->size, 0);
            }
        }
    }
}

int HAL_PIXEL_FORMAT_to_MS_ColorFormat(int inFormat) {
    switch (inFormat) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return E_MS_FMT_ABGR8888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return E_MS_FMT_ABGR8888;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return E_MS_FMT_ARGB8888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return E_MS_FMT_RGB565;
        default:
            ALOGE("HAL_PIXEL_FORMAT_to_MS_ColorFormat failed");
            return E_MS_FMT_GENERIC;
    }
}

int HAL_PIXEL_FORMAT_byteperpixel(int inFormat) {
    switch (inFormat) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return 4;
        case HAL_PIXEL_FORMAT_RGB_565:
            return 2;
        default:
            ALOGE("HAL_PIXEL_FORMAT_byteperpixel failed");
            return 0;
    }
}
