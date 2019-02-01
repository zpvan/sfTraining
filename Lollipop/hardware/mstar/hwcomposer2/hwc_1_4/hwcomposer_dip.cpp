//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2015 MStar Semiconductor, Inc. All rights reserved.
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

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <utils/Errors.h>
#include <assert.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <fcntl.h>


/* mstar header */
#include <mmap.h>
#include <apiXC.h>
#include <drvCMAPool.h>
#include <vector>

#include "hwcomposer_dip.h"
#include "hwcomposer_gop.h"
// caculate based on frame rate
#define DIP_RETRY_TIMES 10

typedef struct {
    uintptr_t u32MemoryPhyAddress;
    uintptr_t u32MemorySize;
    unsigned int u16CaptureWidth;
    unsigned int u16CaptureHeight;
    int captureWithOsd;
} capture_info;

static pthread_t dip_thread;
static pthread_mutex_t mMutexMain;
static pthread_mutex_t mMutexSub;
static pthread_cond_t main_cond;
static pthread_cond_t sub_cond;
static CMA_Pool_Alloc_Param mCMA_Alloc_PARAM;
static CMA_Pool_Free_Param mCMA_Pool_Free_Param;
static bool mCMAEnable;
static int captureScreenMutexId = -1;
std::vector<hwc_capture_data_t> mCaptureData;

static int do_dip_capture_screen(hwc_context_t* ctx) {
    int bResult = FALSE;
    //One frame mode
    if(E_APIXC_RET_OK == MApi_XC_DIP_CapOneFrameFast(ctx->enDIPWin)) {
        ALOGD("[captureScreen]:sucess!!!");
        bResult = TRUE;
    } else {
        ALOGE("[captureScreen]:fail");
        bResult = FALSE;
    }
    return bResult;
}

static void *dip_work_thread(void *data) {
    hwc_context_t *ctx = (hwc_context_t*)data;
    while (true) {
        pthread_mutex_lock(&mMutexSub);
        pthread_cond_wait(&sub_cond, &mMutexSub);
        do_dip_capture_screen(ctx);
        pthread_mutex_unlock(&mMutexSub);
        pthread_mutex_lock(&mMutexMain);
        //tell main thread,have done
        pthread_cond_signal(&main_cond);
        pthread_mutex_unlock(&mMutexMain);
    }
    return NULL;
}

void DIP_init(hwc_context_t* ctx) {
    const mmap_info_t* mmapInfo = NULL;
    mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_DIP_AN_CAPTURESCREEN");
    if (mmapInfo == NULL) {
        ALOGE("[capture screen]: There is no E_DIP_AN_CAPTURESCREEN,using E_DFB_JPD_WRITE");
        mmapInfo = mmap_get_info("E_DFB_JPD_WRITE");
    }
    if (mmapInfo != NULL) {
        ALOGD("[capture screen]: Capture screen memory cmahid = %d,miuno = %d, addr = %p,size = %lx",mmapInfo->cmahid,mmapInfo->miuno,mmapInfo->addr,mmapInfo->size);
        MS_BOOL ret = TRUE;
        mCMAEnable = false;
        if (mmapInfo->cmahid > 0) {
            CMA_Pool_Init_Param CMA_Pool_Init_PARAM;
            //CMA_POOL_INIT
            CMA_Pool_Init_PARAM.heap_id = mmapInfo->cmahid;
            CMA_Pool_Init_PARAM.flags = CMA_FLAG_MAP_VMA;
            ret = MApi_CMA_Pool_Init(&CMA_Pool_Init_PARAM);
            if (ret ==  FALSE) {
                ALOGE("gfx_Init MApi_CMA_Pool_Init error");
            }
            //CMA_Pool_GetMem
            mCMA_Alloc_PARAM.pool_handle_id = CMA_Pool_Init_PARAM.pool_handle_id;
            if (mmapInfo->miuno == 0) {
                mCMA_Alloc_PARAM.offset_in_pool = mmapInfo->addr - CMA_Pool_Init_PARAM.heap_miu_start_offset;
            } else if (mmapInfo->miuno == 1) {
                mCMA_Alloc_PARAM.offset_in_pool = mmapInfo->addr - mmap_get_miu0_offset() - CMA_Pool_Init_PARAM.heap_miu_start_offset;
            } else if (mmapInfo->miuno == 2) {
                mCMA_Alloc_PARAM.offset_in_pool = mmapInfo->addr - mmap_get_miu1_offset() - CMA_Pool_Init_PARAM.heap_miu_start_offset;
            }
            mCMA_Alloc_PARAM.length = mmapInfo->size;
            mCMA_Alloc_PARAM.flags = CMA_FLAG_VIRT_ADDR;
            mCMA_Pool_Free_Param.pool_handle_id = CMA_Pool_Init_PARAM.pool_handle_id;
            mCMA_Pool_Free_Param.offset_in_pool = mmapInfo->addr - CMA_Pool_Init_PARAM.heap_miu_start_offset;
            mCMA_Pool_Free_Param.length = mmapInfo->size;
            mCMAEnable = true;
        }else {
            if (mmapInfo->miuno == 0) {
                ret = MsOS_MPool_Mapping(0, mmapInfo->addr,  mmapInfo->size, 1);
            } else if (mmapInfo->miuno == 1) {
                ret = MsOS_MPool_Mapping(1, (mmapInfo->addr - mmap_get_miu0_offset()),  mmapInfo->size, 1);
            } else if (mmapInfo->miuno == 2) {
                ret = MsOS_MPool_Mapping(2, (mmapInfo->addr - mmap_get_miu1_offset()),  mmapInfo->size, 1);
            }
            if (ret) {
                void* vaddr = NULL;
                 if (mmapInfo->miuno == 0) {
                    vaddr = (void*)MsOS_PA2KSEG1(mmapInfo->addr);
                } else if (mmapInfo->miuno == 1) {
                    vaddr = (void*)MsOS_PA2KSEG1(mmapInfo->addr | mmap_get_miu0_offset());
                } else if (mmapInfo->miuno == 2) {
                    vaddr = (void*)MsOS_PA2KSEG1(mmapInfo->addr | mmap_get_miu1_offset());
                }
                if (vaddr != NULL) {
                    memset(vaddr, 0x0, mmapInfo->size);
                } else {
                    ALOGE("get the DIP memory virtual addr failed!!!!!");
                }
                ctx->captureScreenAddr = (uintptr_t)vaddr;
            }
        }
        ctx->captureScreenLen = (uintptr_t)mmapInfo->size;
        ALOGD("[capture screen]: ctx->captureScreenAddr = %llx,ctx->captureScreenLen = %llx",(long long unsigned int)ctx->captureScreenAddr,(long long unsigned int)ctx->captureScreenLen);
    }
    pthread_cond_init(&main_cond, NULL);
    pthread_cond_init(&sub_cond, NULL);
    pthread_mutex_init(&mMutexMain, NULL);
    pthread_mutex_init(&mMutexSub, NULL);

    pthread_create(&dip_thread, NULL, dip_work_thread, (void*)ctx);
    if (captureScreenMutexId < 0) {
        captureScreenMutexId = MsOS_CreateNamedMutex((MS_S8 *)"SkiaDecodeMutex");

        if (captureScreenMutexId < 0) {
            ALOGE("[capture screen]: create named mutex SkiaDecodeMutex failed.");
        }
    }
}

static int xc_dip_prepare(hwc_context_t* ctx,capture_info stCaptureInfo) {
    ctx->enDIPWin  = MAX_DIP_WINDOW;
    XC_SETWIN_INFO stXC_SetWin_Info;
    ST_XC_DIP_WINPROPERTY stDIPWinProperty;
    ST_XC_DIP_CHIPCAPS stDipChipCaps={(SCALER_DIP_WIN)(ctx->enDIPWin), 0};
    for (int i = 0; i < MAX_DIP_WINDOW ; i++) {
        if ((E_APIXC_RET_OK == MApi_XC_DIP_QueryResource((SCALER_DIP_WIN)i)) && (E_APIXC_RET_OK == MApi_XC_DIP_GetResource((SCALER_DIP_WIN)i))) {
            ctx->enDIPWin = (SCALER_DIP_WIN)i;
            stDipChipCaps.eWindow = ctx->enDIPWin;
            if(E_APIXC_RET_OK != MApi_XC_GetChipCaps(E_XC_DIP_CHIP_CAPS, (MS_U32 *)(&stDipChipCaps), sizeof(ST_XC_DIP_CHIPCAPS))) {
                ALOGE("[capture screen]: MApi_XC_GetChipCaps return fail for DIP[%d]\n",ctx->enDIPWin);
            }
            if((MS_BOOL)(stDipChipCaps.u32DipChipCaps & DIP_CAP_SCALING_DOWN)) {
                break;
            } else {
                ALOGE("[capture screen]: DIP[%u] doesn't support scaling down\n",ctx->enDIPWin);
            }
        }
    }

    if (ctx->enDIPWin == MAX_DIP_WINDOW) {
        ALOGE("[capture screen]: MApi_XC_DIP_GetResource Failed!!!\n");
        return  FALSE;
    }

    //enDIPWin = (SCALER_DIP_WIN)1;
    ALOGD("[capture screen]:dip win = %d",ctx->enDIPWin);
    if(E_APIXC_RET_OK != MApi_XC_DIP_InitByDIP(ctx->enDIPWin)) {
        ALOGE("[capture screen]: MApi_XC_DIP_InitByDIP Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(ctx->enDIPWin);
        return  FALSE;
    }

    MApi_XC_DIP_SetOutputDataFmt(DIP_DATA_FMT_ARGB8888, ctx->enDIPWin);
    MApi_XC_DIP_SetAlpha(0xFF, ctx->enDIPWin);
    MApi_XC_DIP_FrameRateCtrl(TRUE, 1, 1, ctx->enDIPWin);
    EN_XC_DIP_OP_CAPTURE type = stCaptureInfo.captureWithOsd? E_XC_DIP_OP2:E_XC_DIP_VOP2;
    if(E_APIXC_RET_OK != MApi_XC_DIP_SetOutputCapture(TRUE, type, ctx->enDIPWin)) {//Set to capture Video(no OSD).
        ALOGE("[capture screen]: MApi_XC_DIP_SetOutputCapture Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(ctx->enDIPWin);
        return  FALSE;
    }
    MApi_XC_DIP_EnableR2Y(FALSE, ctx->enDIPWin);

    if (!ctx->bRGBColorSpace) {
        MApi_XC_DIP_EnableY2R(TRUE, ctx->enDIPWin);//OP is yuv, so enable Y2R
    } else {
        MApi_XC_DIP_EnableY2R(FALSE, ctx->enDIPWin);//OP is rgb, so Disable Y2R
    }
    MApi_XC_DIP_SwapRGB(TRUE,E_XC_DIP_RGB_SWAPTO_BGR,ctx->enDIPWin);
    //We get video data from op2,since clippers,it doesn't care about current scan mode.
    //Always progressive mode.
    MApi_XC_DIP_SetSourceScanType(DIP_SCAN_MODE_PROGRESSIVE, ctx->enDIPWin);
    MApi_XC_DIP_EnaInterlaceWrite(FALSE, ctx->enDIPWin); // auto mode, do not de-interlace, return TOP/Bottom field to APP
    int opTimingWidth,opTimingHeight;
    GOP_getCurOPTiming(&opTimingWidth,&opTimingHeight,NULL);
    memset(&stXC_SetWin_Info,0,sizeof(XC_SETWIN_INFO));
    stXC_SetWin_Info.enInputSourceType = INPUT_SOURCE_TV;//INPUT_SOURCE_STORAGE;
    stXC_SetWin_Info.stCapWin.x = 0;
    stXC_SetWin_Info.stCapWin.y = 0;
    stXC_SetWin_Info.stCapWin.width = opTimingWidth;//stCaptureInfo.u16CaptureWidth;//Capture display window
    stXC_SetWin_Info.stCapWin.height = opTimingHeight;//stCaptureInfo.u16CaptureHeight;
    stXC_SetWin_Info.bPreHCusScaling = TRUE;
    stXC_SetWin_Info.u16PreHCusScalingSrc = opTimingWidth;//stCaptureInfo.u16CaptureWidth;
    stXC_SetWin_Info.u16PreHCusScalingDst = stCaptureInfo.u16CaptureWidth;
    stXC_SetWin_Info.bPreVCusScaling = TRUE;
    stXC_SetWin_Info.u16PreVCusScalingSrc = opTimingHeight;//stCaptureInfo.u16CaptureHeight;
    stXC_SetWin_Info.u16PreVCusScalingDst = stCaptureInfo.u16CaptureHeight;
    stXC_SetWin_Info.bInterlace = FALSE;

    ALOGD("[capture screen]: ---enInputSourceType=%u\n", stXC_SetWin_Info.enInputSourceType);
    ALOGD("[capture screen]: ---SetDIPWin x,y,w,h=%u,%u,%u,%u\n", stXC_SetWin_Info.stCapWin.x, stXC_SetWin_Info.stCapWin.y,
                                stXC_SetWin_Info.stCapWin.width, stXC_SetWin_Info.stCapWin.height);
    ALOGD("[capture screen]: ---stDispWin x,y,w,h=%u,%u,%u,%u\n",  stXC_SetWin_Info.stDispWin.x, stXC_SetWin_Info.stDispWin.y,
                                stXC_SetWin_Info.stDispWin.width, stXC_SetWin_Info.stDispWin.height);
    ALOGD("[capture screen]: ---stCropWin x,y,w,h=%u,%u,%u,%u\n", stXC_SetWin_Info.stCropWin.x, stXC_SetWin_Info.stCropWin.y,
                                stXC_SetWin_Info.stCropWin.width, stXC_SetWin_Info.stCropWin.height);

    ALOGD("[capture screen]: ---bInterlace, bHDuplicate, u16InputVFreq,u16InputVTotal, u16DefaultHtotal, u8DefaultPhase=%u,%u,%u,%u,%u,%u\n", stXC_SetWin_Info.bInterlace, stXC_SetWin_Info.bHDuplicate,
                                stXC_SetWin_Info.u16InputVFreq, stXC_SetWin_Info.u16InputVTotal, stXC_SetWin_Info.u16DefaultHtotal, stXC_SetWin_Info.u8DefaultPhase);

    ALOGD("[capture screen]: ---bHCusScaling[%u] src,dst =%u,%u\n",  stXC_SetWin_Info.bHCusScaling, stXC_SetWin_Info.u16HCusScalingSrc, stXC_SetWin_Info.u16HCusScalingDst);
    ALOGD("[capture screen]: ---bVCusScaling[%u] src,dst =%u,%u\n",  stXC_SetWin_Info.bVCusScaling, stXC_SetWin_Info.u16VCusScalingSrc, stXC_SetWin_Info.u16VCusScalingDst);

    ALOGD("[capture screen]: ---bDisplayNineLattice=%u\n", stXC_SetWin_Info.bDisplayNineLattice);

    ALOGD("[capture screen]: ---PreScalingH[%u] src,dst =%u,%u\n", stXC_SetWin_Info.bPreHCusScaling, stXC_SetWin_Info.u16PreHCusScalingSrc, stXC_SetWin_Info.u16PreHCusScalingDst);
    ALOGD("[capture screen]: ---PreScalingV[%u] src,dst =%u,%u\n",stXC_SetWin_Info.bPreVCusScaling, stXC_SetWin_Info.u16PreVCusScalingSrc, stXC_SetWin_Info.u16PreVCusScalingDst);
    ALOGD("[capture screen]: ---u16DefaultPhase=%u\n", stXC_SetWin_Info.u16DefaultPhase);


    if(E_APIXC_RET_OK != MApi_XC_DIP_SetWindow(&stXC_SetWin_Info, sizeof(XC_SETWIN_INFO), ctx->enDIPWin)) {//Set DIP clip window AND scaling
        ALOGE("[capture screen]: MApi_XC_DIP_SetWindow Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(ctx->enDIPWin);
        return  FALSE;
    }
    memset(&stDIPWinProperty,0,sizeof(ST_XC_DIP_WINPROPERTY));
    stDIPWinProperty.u8BufCnt = ctx->captureScreenLen / stCaptureInfo.u32MemorySize;
    stDIPWinProperty.u16Width = stCaptureInfo.u16CaptureWidth;
    stDIPWinProperty.u16Height = stCaptureInfo.u16CaptureHeight;
    stDIPWinProperty.u32BufStart = stCaptureInfo.u32MemoryPhyAddress;
    stDIPWinProperty.u32BufEnd = stCaptureInfo.u32MemoryPhyAddress + stCaptureInfo.u32MemorySize;
    stDIPWinProperty.u16Pitch = stCaptureInfo.u16CaptureWidth * ARGB8888_LEN;
    stDIPWinProperty.enSource = SCALER_DIP_SOURCE_TYPE_OP_CAPTURE;

    ALOGD("[capture screen]: ---DIP bufcount=%u, output w,h=%u,%u, u32BufStart, u32BufEnd = 0x%lx,0x%lx, enSource= %u\n", stDIPWinProperty.u8BufCnt, stDIPWinProperty.u16Width,  stDIPWinProperty.u16Height, stDIPWinProperty.u32BufStart, stDIPWinProperty.u32BufEnd, stDIPWinProperty.enSource);

    if(E_APIXC_RET_OK != MApi_XC_DIP_SetDIPWinProperty(&stDIPWinProperty, ctx->enDIPWin)) {
        ALOGE("[capture screen]: MApi_XC_DIP_SetDIPWinProperty Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(ctx->enDIPWin);
        return  FALSE;
    }
    return TRUE;
}

int DIP_releaseCaptureScreenMemory(hwc_context_t* ctx) {
    ALOGD("[capture screen]: DIP_releaseCaptureScreenMemory!!!!");
    for (int i=0; i<mCaptureData.size(); i++) {
        hwc_capture_data_t data = mCaptureData.at(i);
        if (data.address != NULL) {
            ALOGD("[capture screen]:free  data[%d]address!!!!!!!%lx",i,(unsigned long)data.address);
            free((void*)data.address);
        }
    }
    mCaptureData.clear();
    if (mCMAEnable) {
        MS_BOOL ret = MApi_CMA_Pool_PutMem(&mCMA_Pool_Free_Param);
        if (ret == FALSE) {
            ALOGE("[capture screen]: DIP_releaseCaptureScreenMemory MApi_CMA_Pool_PutMem error!!!!");
            return FALSE;
        }
        ALOGD("[capture screen]: DIP_releaseCaptureScreenMemory MApi_CMA_Pool_PutMem success!!!!");
    }
    if (!(MsOS_UnlockMutex(captureScreenMutexId, 0))) {
        ALOGE("[capture screen]: unlock captureScreen Mem Mutex failed!!");
        return FALSE;
    }
    return TRUE;
}

int DIP_startCaptureScreenWithClip(hwc_context_t* ctx,
                                                        size_t numData, hwc_capture_data_t* captureData,
                                                        int width, int height, int withOsd) {
    int result = DIP_startCaptureScreen(ctx,width,height,withOsd);
    //add debug infomation :fill buffer with random number
    char property[PROPERTY_VALUE_MAX] = {0};
    property_get("mstar.debug.capturedata", property, "0");
    if (strcmp(property, "2") == 0) {
        for (int i=0; i<height; i++) {
            uint8_t* date = ((uint8_t *)ctx->captureScreenAddr)+i*width*4;
            memset(date, rand() % 256, width*4);
            result = 1;
        }
    }
    if (result) {
        if (captureData) {
            for (size_t i = 0; i < numData;i++) {
                //apply gop's scale ratio
                float wRatio = (float)width / ctx->displayRegionWidth;
                float hRatio = (float)height / ctx->displayRegionHeight;
                ALOGD("[capture screen]:wRatio = %f,hRatio = %f",wRatio,hRatio);
                captureData[i].region.left = captureData[i].region.left * wRatio + 0.5;
                captureData[i].region.top = captureData[i].region.top * hRatio + 0.5;
                captureData[i].region.right = captureData[i].region.right * wRatio  + 0.5;
                captureData[i].region.bottom = captureData[i].region.bottom * hRatio + 0.5;
                int dataWidth = captureData[i].region.right - captureData[i].region.left;
                int dataHeight = captureData[i].region.bottom - captureData[i].region.top;
                captureData[i].address = (uintptr_t)malloc(dataWidth * dataHeight * ARGB8888_LEN);
                if (captureData[i].address != NULL) {
                    memset((void*)captureData[i].address,0,dataWidth * dataHeight * ARGB8888_LEN);
                    void* src = (void*)(ctx->captureScreenAddr + ARGB8888_LEN * width * captureData[i].region.top
                                                + ARGB8888_LEN * captureData[i].region.left);
                    void* dest = (void*)(captureData[i].address);
                    int line_num = captureData[i].region.bottom - captureData[i].region.top;
                    unsigned long capture_range_original_line_size = ARGB8888_LEN * width;
                    unsigned long capture_range_required_line_size = ARGB8888_LEN * (captureData[i].region.right - captureData[i].region.left);
                    ALOGD("[capture screen]:clip [%d,%d-%d,%d] from rawdata begin,adress = %lx",captureData[i].region.left,captureData[i].region.top,
                        captureData[i].region.right,captureData[i].region.bottom,(unsigned long)captureData[i].address);
                    if (ctx->captureScreenLen == dataWidth * dataHeight * ARGB8888_LEN) {
                        memcpy((void*)captureData[i].address, (void*)ctx->captureScreenAddr, width * height * ARGB8888_LEN);
                    } else {
                        for (int i = 0; i < line_num; i++) {
                            memcpy(dest, src, capture_range_required_line_size);
                            dest = (void*)((char *)dest + capture_range_required_line_size);
                            src = (void*)((char *)src + capture_range_original_line_size);
                        }
                    }
                    ALOGD("[capture screen]:clip [%d,%d-%d,%d] from rawdata end,adress = %lx",captureData[i].region.left,captureData[i].region.top,
                        captureData[i].region.right,captureData[i].region.bottom,(unsigned long)captureData[i].address);
                    mCaptureData.push_back(captureData[i]);

                    //add debug infomation :save capture data to /data/capture/
                    if (strcmp(property, "3") == 0) {
                        char filename[100];
                        int num  = -1;
                        while (++num < 10000) {
                            snprintf(filename, 100, "%s/%s_%d,%d--%d,%d_%04d.raw",
                                    "/data/capture","captureData", captureData[i].region.left,captureData[i].region.top,
                                    captureData[i].region.right,captureData[i].region.bottom,num);

                            if (access(filename, F_OK) != 0) {
                                break;
                            }
                        }
                        ALOGD("[capture screen]:captureData filename  = %s",filename);
                        int fd_p = open(filename, O_EXCL | O_CREAT | O_WRONLY, 0644);
                        if (fd_p < 0) {
                            ALOGD("[capture screen]:%s  can't open",filename);
                            return 0;
                        }
                        write(fd_p, (void *)captureData[i].address, dataWidth*dataHeight*ARGB8888_LEN);
                        close(fd_p);
                    }
                }
            }
        }
    }
    return result;
}

int DIP_startCaptureScreen(hwc_context_t* ctx, int width, int height,int withOsd) {
    capture_info stCaptureInfo;
    unsigned int u32MemorySize= ARGB8888_LEN * width * height;//25165824;//
    int bResult = TRUE;
    bool bCaptureSuccess = FALSE;
    ALOGD("[capture screen]:Start_Capture \n");

    if (!(MsOS_LockMutex(captureScreenMutexId, 0))) {
        ALOGE("[capture screen] lock captureScreen Mem Mutex failed!!");
        return FALSE;
    }

    if (mCMAEnable) {
        MS_BOOL ret = MApi_CMA_Pool_GetMem(&mCMA_Alloc_PARAM);
        if (ret == FALSE) {
            ALOGE("[capture screen]: gfx_Init MApi_CMA_Pool_GetMem error");
            if (!(MsOS_UnlockMutex(captureScreenMutexId, 0))) {
                ALOGE("[capture screen] unlock captureScreen Mem Mutex failed!!");
                return FALSE;
            }
            return FALSE;
        }
        //store
        void* vaddr = (void*)mCMA_Alloc_PARAM.virt_addr;
        ctx->captureScreenAddr = (uintptr_t)vaddr;
    }
    memset((void*)ctx->captureScreenAddr,0x0,ctx->captureScreenLen);
    memset(&stCaptureInfo, 0x00, sizeof(capture_info));
    stCaptureInfo.u32MemorySize = u32MemorySize;
    stCaptureInfo.u16CaptureWidth = width;
    stCaptureInfo.u16CaptureHeight = height;
    stCaptureInfo.u32MemoryPhyAddress = MsOS_VA2PA(ctx->captureScreenAddr);
    stCaptureInfo.captureWithOsd = withOsd;
    if(stCaptureInfo.u32MemoryPhyAddress == 0) {
        ALOGE("[capture screen]: Get Capture MemoryPhyAddress Failed!!!\n");
        if (!(MsOS_UnlockMutex(captureScreenMutexId, 0))) {
            ALOGE("[capture screen] unlock captureScreen Mem Mutex failed!!");
            return FALSE;
        }
        return FALSE;
    }

    stCaptureInfo.u32MemoryPhyAddress = (stCaptureInfo.u32MemoryPhyAddress + (MEM_ADDR_ALIGNMENT - 1))&(~(MEM_ADDR_ALIGNMENT - 1));
    stCaptureInfo.u32MemoryPhyAddress = (stCaptureInfo.u32MemoryPhyAddress + (DIP_BUS_ALIGNMENT_FACTOR-1)) & (~(DIP_BUS_ALIGNMENT_FACTOR-1));
    ctx->captureScreenAddr = MsOS_PA2KSEG1(stCaptureInfo.u32MemoryPhyAddress);
    ALOGD("[capture screen]:u32MemoryPhyAddress %llx u32MemorySize %llx\n",(long long unsigned int)stCaptureInfo.u32MemoryPhyAddress,(long long unsigned int)stCaptureInfo.u32MemorySize );
    bResult = xc_dip_prepare(ctx,stCaptureInfo);
    if(bResult == FALSE) {
        ALOGE("[capture screen]: XC DIP init Failed!!!\n");
        if (!(MsOS_UnlockMutex(captureScreenMutexId, 0))) {
            ALOGE("[capture screen] unlock captureScreen Mem Mutex failed!!");
            return FALSE;
        }
        return FALSE;
    }
    //bCaptureSuccess = do_dip_capture_screen(ctx);
    pthread_mutex_lock(&mMutexSub);
    //lock main mutex,because sometimes,sub thread maybe do fast before main have not waited.
    pthread_mutex_lock(&mMutexMain);
    //signal dip thread,
    pthread_cond_signal(&sub_cond);
    pthread_mutex_unlock(&mMutexSub);

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec = time(NULL)+1;
    int ret = pthread_cond_timedwait(&main_cond, &mMutexMain,&ts);
    if(ret == ETIMEDOUT) {
        ALOGE("[capture screen]: Capture Screen time out!!!!!!!!");
        if (!(MsOS_UnlockMutex(captureScreenMutexId, 0))) {
            ALOGE("[capture screen] unlock captureScreen Mem Mutex failed!!");
            return FALSE;
        }
        bCaptureSuccess = FALSE;
    } else {
        bCaptureSuccess = TRUE;
    }
    pthread_mutex_unlock(&mMutexMain);
    if (bCaptureSuccess) {
        MS_U16 u16IntStatus = MApi_XC_DIP_GetIntStatus(ctx->enDIPWin);
        ALOGD("[capture screen]: CaptureDone u16IntStatus %d \n",u16IntStatus);
        int index = 1;
        //Because BW problem,we need waitting for two vsync to get correct status
        while (u16IntStatus == 0 && index < DIP_RETRY_TIMES) {
            usleep(16*1000);
            u16IntStatus = MApi_XC_DIP_GetIntStatus(ctx->enDIPWin);
            ALOGD("[capture screen]: delay %d ms, CaptureDone u16IntStatus %d \n",16*index,u16IntStatus);
            if (u16IntStatus != 0) {
                break;
            }
            index ++;
        }
        if(u16IntStatus != 0) {
            MApi_XC_DIP_ClearInt(u16IntStatus, ctx->enDIPWin);
        }
    }
    MApi_XC_DIP_Ena(false, ctx->enDIPWin);
    MApi_XC_DIP_ReleaseResource(ctx->enDIPWin);
    ctx->enDIPWin  = MAX_DIP_WINDOW;
    return bCaptureSuccess;
}
