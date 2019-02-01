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
#undef LOG_TAG
#define LOG_TAG "HWComposerDip"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define __CLASS__ "HWComposerDip"

#include "HWComposerDip.h"

HWComposerDip::HWComposerDip(HWCDisplayDevice& device)
    :mDisplay(device),
    mCaptureScreenMutexId(-1),
    mCaptureScreenAddr(0),
    mCaptureScreenLen(0),
    mCaptureData(),
    mCMAEnable(false),
    mCMA_Alloc_PARAM(),
    mCMA_Pool_Free_Param(),
    enDIPWin(MAX_DIP_WINDOW)
{
    init();
}
HWComposerDip::~HWComposerDip() {

}

hwc2_error_t HWComposerDip::doDipCaptureScreen() {
    hwc2_error_t bResult = HWC2_ERROR_NONE;
    //One frame mode
    if(E_APIXC_RET_OK == MApi_XC_DIP_CapOneFrameFast(enDIPWin)) {
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[captureScreen]:sucess!!!");
        bResult = HWC2_ERROR_NONE;
    } else {
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[captureScreen]:fail");
        bResult = HWC2_ERROR_NOT_VALIDATED;
    }
    return bResult;
}

void HWComposerDip::init() {
    const mmap_info_t* mmapInfo = NULL;
    mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_DIP_AN_CAPTURESCREEN");
    if (mmapInfo == NULL) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: There is no E_DIP_AN_CAPTURESCREEN,using E_DFB_JPD_WRITE");
        mmapInfo = mmap_get_info("E_DFB_JPD_WRITE");
    }
    if (mmapInfo != NULL) {
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: Capture screen memory cmahid = %d,miuno = %d, addr = %p,size = %lx",mmapInfo->cmahid,mmapInfo->miuno,mmapInfo->addr,mmapInfo->size);
        MS_BOOL ret = TRUE;
        mCMAEnable = false;
        if (mmapInfo->cmahid > 0) {
            CMA_Pool_Init_Param CMA_Pool_Init_PARAM;
            //CMA_POOL_INIT
            CMA_Pool_Init_PARAM.heap_id = mmapInfo->cmahid;
            CMA_Pool_Init_PARAM.flags = CMA_FLAG_MAP_VMA;
            ret = MApi_CMA_Pool_Init(&CMA_Pool_Init_PARAM);
            if (ret ==  FALSE) {
                DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "gfx_Init MApi_CMA_Pool_Init error");
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
                    DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "get the DIP memory virtual addr failed!!!!!");
                }
                mCaptureScreenAddr = (uintptr_t)vaddr;
            }
        }
        mCaptureScreenLen = (uintptr_t)mmapInfo->size;
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ctx->captureScreenAddr = %lx,ctx->captureScreenLen = %lx",mCaptureScreenAddr,mCaptureScreenLen);
    }
    if (mCaptureScreenMutexId < 0) {
        mCaptureScreenMutexId = MsOS_CreateNamedMutex((MS_S8 *)"SkiaDecodeMutex");

        if (mCaptureScreenMutexId < 0) {
            DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: create named mutex SkiaDecodeMutex failed.");
        }
    }
}
hwc2_error_t HWComposerDip::releaseCaptureScreenMemory() {
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: DIP_releaseCaptureScreenMemory!!!!");
    for (int i=0; i<mCaptureData.size(); i++) {
        hwc_capture_data_t data = mCaptureData.at(i);
        if (data.address != NULL) {
            DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:free  data[%d]address!!!!!!!%lx",i,(unsigned long)data.address);
            free((void*)data.address);
        }
    }
    mCaptureData.clear();
    if (mCMAEnable) {
        MS_BOOL ret = MApi_CMA_Pool_PutMem(&mCMA_Pool_Free_Param);
        if (ret == FALSE) {
            DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: DIP_releaseCaptureScreenMemory MApi_CMA_Pool_PutMem error!!!!");
            return HWC2_ERROR_NOT_VALIDATED;
        }
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: DIP_releaseCaptureScreenMemory MApi_CMA_Pool_PutMem success!!!!");
    }
    if (!(MsOS_UnlockMutex(mCaptureScreenMutexId, 0))) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: unlock captureScreen Mem Mutex failed!!");
        return HWC2_ERROR_NOT_VALIDATED;
    }
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWComposerDip::startCaptureScreen(int* outWidth, int* outHeight, int withOsd) {

    mDisplay.getCurrentTiming(outWidth,outHeight);
    getTravelingRes(outWidth,outHeight);
    int width = *outWidth;
    int height = *outHeight;
    CaptureInfo stCaptureInfo;
    unsigned int u32MemorySize= ARGB8888_LEN * width * height;//25165824;//
    int bResult = TRUE;
    hwc2_error_t bCaptureSuccess = HWC2_ERROR_NONE;
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:Start_Capture \n");
    if (!(MsOS_LockMutex(mCaptureScreenMutexId, 0))) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen] lock captureScreen Mem Mutex failed!!");
        return HWC2_ERROR_NOT_VALIDATED;
    }

    if (mCMAEnable) {
        MS_BOOL ret = MApi_CMA_Pool_GetMem(&mCMA_Alloc_PARAM);
        if (ret == FALSE) {
            DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: gfx_Init MApi_CMA_Pool_GetMem error");
            if (!(MsOS_UnlockMutex(mCaptureScreenMutexId, 0))) {
                DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen] unlock captureScreen Mem Mutex failed!!");
                return HWC2_ERROR_NOT_VALIDATED;
            }
            return HWC2_ERROR_NOT_VALIDATED;
        }
        //store
        void* vaddr = (void*)mCMA_Alloc_PARAM.virt_addr;
        mCaptureScreenAddr = (uintptr_t)vaddr;
    }
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:startCapture mCaptureScreenAddr = %lx,capture width = %d,height = %d"
        ,mCaptureScreenAddr, width, height);
    memset((void*)mCaptureScreenAddr,0x0,mCaptureScreenLen);
    stCaptureInfo.u32MemorySize = u32MemorySize;
    stCaptureInfo.u16CaptureWidth = width;
    stCaptureInfo.u16CaptureHeight = height;
    stCaptureInfo.u32MemoryPhyAddress = MsOS_VA2PA(mCaptureScreenAddr);
    stCaptureInfo.captureWithOsd = withOsd;
    if(stCaptureInfo.u32MemoryPhyAddress == 0) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: Get Capture MemoryPhyAddress Failed!!!\n");
        if (!(MsOS_UnlockMutex(mCaptureScreenMutexId, 0))) {
            DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen] unlock captureScreen Mem Mutex failed!!");
            return HWC2_ERROR_NOT_VALIDATED;
        }
        return HWC2_ERROR_NOT_VALIDATED;
    }

    stCaptureInfo.u32MemoryPhyAddress = (stCaptureInfo.u32MemoryPhyAddress + (MEM_ADDR_ALIGNMENT - 1))&(~(MEM_ADDR_ALIGNMENT - 1));
    stCaptureInfo.u32MemoryPhyAddress = (stCaptureInfo.u32MemoryPhyAddress + (DIP_BUS_ALIGNMENT_FACTOR-1)) & (~(DIP_BUS_ALIGNMENT_FACTOR-1));
    mCaptureScreenAddr = MsOS_PA2KSEG1(stCaptureInfo.u32MemoryPhyAddress);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:u32MemoryPhyAddress %llx u32MemorySize %llx\n",(long long unsigned int)stCaptureInfo.u32MemoryPhyAddress,(long long unsigned int)stCaptureInfo.u32MemorySize );
    bResult = dipPrepare(stCaptureInfo);
    if(bResult == FALSE) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: XC DIP init Failed!!!\n");
        if (!(MsOS_UnlockMutex(mCaptureScreenMutexId, 0))) {
            DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen] unlock captureScreen Mem Mutex failed!!");
            return HWC2_ERROR_NOT_VALIDATED;
        }
        return HWC2_ERROR_NOT_VALIDATED;
    }
    bCaptureSuccess = doDipCaptureScreen();

    if (bCaptureSuccess == HWC2_ERROR_NONE) {
        MS_U16 u16IntStatus = MApi_XC_DIP_GetIntStatus(enDIPWin);
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: CaptureDone u16IntStatus %d \n",u16IntStatus);
        int index = 1;
        //Because BW problem,we need waitting for two vsync to get correct status
        while (u16IntStatus == 0 && index < DIP_RETRY_TIMES) {
            usleep(16*1000);
            u16IntStatus = MApi_XC_DIP_GetIntStatus(enDIPWin);
            DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: delay %d ms, CaptureDone u16IntStatus %d \n",16*index,u16IntStatus);
            if (u16IntStatus != 0) {
                break;
            }
            index ++;
        }
        if(u16IntStatus != 0) {
            MApi_XC_DIP_ClearInt(u16IntStatus, enDIPWin);
        }
    }
    MApi_XC_DIP_Ena(false, enDIPWin);
    MApi_XC_DIP_ReleaseResource(enDIPWin);
    enDIPWin  = MAX_DIP_WINDOW;
    return bCaptureSuccess;
}

hwc2_error_t HWComposerDip::startCaptureScreenWithClip(size_t numData, hwc_capture_data_t* captureData, int* outWidth, int* outHeight, int withOsd) {
    char property[PROPERTY_VALUE_MAX] = {0};
    /* mstar.debug.capturedata ,capture debug option property
        * 1:skip dip flow
        * 2:fill buffer with pattern
        * 3:save raw data.
        */
    property_get("mstar.debug.capturedata", property, "0");
    if (strcmp(property, "1") == 0) {
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: dip scpture screen skip!!!!!");
        return HWC2_ERROR_UNSUPPORTED;
    }
    auto result = startCaptureScreen(outWidth,outHeight,withOsd);
    int width = *outWidth;
    int height = *outHeight;
    //add debug infomation :fill buffer with random number
    property_get("mstar.debug.capturedata", property, "0");
    if (strcmp(property, "2") == 0) {
        for (int i=0; i<height; i++) {
            uint8_t* date = ((uint8_t *)mCaptureScreenAddr)+i*width*4;
            memset(date, rand() % 256, width*4);
            result = HWC2_ERROR_NONE;
        }
    }
    if (result == HWC2_ERROR_NONE) {
        if (captureData) {
            for (size_t i = 0; i < numData;i++) {
                //apply gop's scale ratio
                float wRatio = (float)width / mDisplay.mDisplayRegionWidth;
                float hRatio = (float)height / mDisplay.mDisplayRegionHeight;
                DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:wRatio = %f,hRatio = %f",wRatio,hRatio);
                captureData[i].region.left = captureData[i].region.left * wRatio + 0.5;
                captureData[i].region.top = captureData[i].region.top * hRatio + 0.5;
                captureData[i].region.right = captureData[i].region.right * wRatio  + 0.5;
                captureData[i].region.bottom = captureData[i].region.bottom * hRatio + 0.5;
                int dataWidth = captureData[i].region.right - captureData[i].region.left;
                int dataHeight = captureData[i].region.bottom - captureData[i].region.top;
                captureData[i].address = (uintptr_t)malloc(dataWidth * dataHeight * ARGB8888_LEN);
                if (captureData[i].address != NULL) {
                    memset((void*)captureData[i].address,0,dataWidth * dataHeight * ARGB8888_LEN);
                    void* src = (void*)(mCaptureScreenAddr + ARGB8888_LEN * width * captureData[i].region.top
                                                + ARGB8888_LEN * captureData[i].region.left);
                    void* dest = (void*)(captureData[i].address);
                    int line_num = captureData[i].region.bottom - captureData[i].region.top;
                    unsigned long capture_range_original_line_size = ARGB8888_LEN * width;
                    unsigned long capture_range_required_line_size = ARGB8888_LEN * (captureData[i].region.right - captureData[i].region.left);
                    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:clip [%d,%d-%d,%d] from rawdata begin,adress = %lx",captureData[i].region.left,captureData[i].region.top,
                        captureData[i].region.right,captureData[i].region.bottom,(unsigned long)captureData[i].address);
                    if (mCaptureScreenLen == dataWidth * dataHeight * ARGB8888_LEN) {
                        memcpy((void*)captureData[i].address, (void*)mCaptureScreenAddr, width * height * ARGB8888_LEN);
                    } else {
                        for (int i = 0; i < line_num; i++) {
                            memcpy(dest, src, capture_range_required_line_size);
                            dest = (void*)((char *)dest + capture_range_required_line_size);
                            src = (void*)((char *)src + capture_range_original_line_size);
                        }
                    }
                    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:clip [%d,%d-%d,%d] from rawdata end,adress = %lx",captureData[i].region.left,captureData[i].region.top,
                        captureData[i].region.right,captureData[i].region.bottom,(unsigned long)captureData[i].address);
                    mCaptureData.push_back(captureData[i]);

                    //add debug infomation :save clip data to /data/capture/
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
                        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:captureData filename  = %s",filename);
                        int fd_p = open(filename, O_EXCL | O_CREAT | O_WRONLY, 0644);
                        if (fd_p < 0) {
                            DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:%s  can't open",filename);
                            return result;
                        }
                        write(fd_p, (void *)captureData[i].address, dataWidth*dataHeight*ARGB8888_LEN);
                        close(fd_p);
                    }
                }
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
            DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:captureData filename  = %s",filename);
            int fd_p = open(filename, O_EXCL | O_CREAT | O_WRONLY, 0644);
            if (fd_p < 0) {
                DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:%s  can't open",filename);
                return result;
            }
            write(fd_p, (void *)mCaptureScreenAddr, width*height*4);
            close(fd_p);
        }
    }
    return result;
}
int HWComposerDip::dipPrepare(CaptureInfo stCaptureInfo) {
    enDIPWin  = MAX_DIP_WINDOW;
    XC_SETWIN_INFO stXC_SetWin_Info;
    ST_XC_DIP_WINPROPERTY stDIPWinProperty;
    ST_XC_DIP_CHIPCAPS stDipChipCaps={(SCALER_DIP_WIN)(enDIPWin), 0};
    for (int i = 0; i < MAX_DIP_WINDOW ; i++) {
        if ((E_APIXC_RET_OK == MApi_XC_DIP_QueryResource((SCALER_DIP_WIN)i)) && (E_APIXC_RET_OK == MApi_XC_DIP_GetResource((SCALER_DIP_WIN)i))) {
            enDIPWin = (SCALER_DIP_WIN)i;
            stDipChipCaps.eWindow = enDIPWin;
            if(E_APIXC_RET_OK != MApi_XC_GetChipCaps(E_XC_DIP_CHIP_CAPS, (MS_U32 *)(&stDipChipCaps), sizeof(ST_XC_DIP_CHIPCAPS))) {
                DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: MApi_XC_GetChipCaps return fail for DIP[%d]\n",enDIPWin);
            }
            if((MS_BOOL)(stDipChipCaps.u32DipChipCaps & DIP_CAP_SCALING_DOWN)) {
                break;
            } else {
                DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: DIP[%u] doesn't support scaling down\n",enDIPWin);
            }
        }
    }

    if (enDIPWin == MAX_DIP_WINDOW) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: MApi_XC_DIP_GetResource Failed!!!\n");
        return  FALSE;
    }

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]:dip win = %d",enDIPWin);
    if(E_APIXC_RET_OK != MApi_XC_DIP_InitByDIP(enDIPWin)) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: MApi_XC_DIP_InitByDIP Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(enDIPWin);
        return  FALSE;
    }

    MApi_XC_DIP_SetOutputDataFmt(DIP_DATA_FMT_ARGB8888, enDIPWin);
    MApi_XC_DIP_SetAlpha(0xFF, enDIPWin);
    MApi_XC_DIP_FrameRateCtrl(TRUE, 1, 1, enDIPWin);
    EN_XC_DIP_OP_CAPTURE type = stCaptureInfo.captureWithOsd? E_XC_DIP_OP2:E_XC_DIP_VOP2;
    if(E_APIXC_RET_OK != MApi_XC_DIP_SetOutputCapture(TRUE, type, enDIPWin)) {//Set to capture Video(no OSD).
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: MApi_XC_DIP_SetOutputCapture Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(enDIPWin);
        return  FALSE;
    }
    MApi_XC_DIP_EnableR2Y(FALSE, enDIPWin);

    if (!mDisplay.bRGBColorSpace) {
        MApi_XC_DIP_EnableY2R(TRUE, enDIPWin);//OP is yuv, so enable Y2R
    } else {
        MApi_XC_DIP_EnableY2R(FALSE, enDIPWin);//OP is rgb, so Disable Y2R
    }
    MApi_XC_DIP_SwapRGB(TRUE,E_XC_DIP_RGB_SWAPTO_BGR,enDIPWin);
    //We get video data from op2,since clippers,it doesn't care about current scan mode.
    //Always progressive mode.
    MApi_XC_DIP_SetSourceScanType(DIP_SCAN_MODE_PROGRESSIVE, enDIPWin);
    MApi_XC_DIP_EnaInterlaceWrite(FALSE, enDIPWin); // auto mode, do not de-interlace, return TOP/Bottom field to APP
    int opTimingWidth,opTimingHeight;
    mDisplay.getCurrentTiming(&opTimingWidth,&opTimingHeight);
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

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---enInputSourceType=%u\n", stXC_SetWin_Info.enInputSourceType);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---SetDIPWin x,y,w,h=%u,%u,%u,%u\n", stXC_SetWin_Info.stCapWin.x, stXC_SetWin_Info.stCapWin.y,
                                stXC_SetWin_Info.stCapWin.width, stXC_SetWin_Info.stCapWin.height);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---stDispWin x,y,w,h=%u,%u,%u,%u\n",  stXC_SetWin_Info.stDispWin.x, stXC_SetWin_Info.stDispWin.y,
                                stXC_SetWin_Info.stDispWin.width, stXC_SetWin_Info.stDispWin.height);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---stCropWin x,y,w,h=%u,%u,%u,%u\n", stXC_SetWin_Info.stCropWin.x, stXC_SetWin_Info.stCropWin.y,
                                stXC_SetWin_Info.stCropWin.width, stXC_SetWin_Info.stCropWin.height);

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---bInterlace, bHDuplicate, u16InputVFreq,u16InputVTotal, u16DefaultHtotal, u8DefaultPhase=%u,%u,%u,%u,%u,%u\n", stXC_SetWin_Info.bInterlace, stXC_SetWin_Info.bHDuplicate,
                                stXC_SetWin_Info.u16InputVFreq, stXC_SetWin_Info.u16InputVTotal, stXC_SetWin_Info.u16DefaultHtotal, stXC_SetWin_Info.u8DefaultPhase);

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---bHCusScaling[%u] src,dst =%u,%u\n",  stXC_SetWin_Info.bHCusScaling, stXC_SetWin_Info.u16HCusScalingSrc, stXC_SetWin_Info.u16HCusScalingDst);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---bVCusScaling[%u] src,dst =%u,%u\n",  stXC_SetWin_Info.bVCusScaling, stXC_SetWin_Info.u16VCusScalingSrc, stXC_SetWin_Info.u16VCusScalingDst);

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---bDisplayNineLattice=%u\n", stXC_SetWin_Info.bDisplayNineLattice);

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---PreScalingH[%u] src,dst =%u,%u\n", stXC_SetWin_Info.bPreHCusScaling, stXC_SetWin_Info.u16PreHCusScalingSrc, stXC_SetWin_Info.u16PreHCusScalingDst);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---PreScalingV[%u] src,dst =%u,%u\n",stXC_SetWin_Info.bPreVCusScaling, stXC_SetWin_Info.u16PreVCusScalingSrc, stXC_SetWin_Info.u16PreVCusScalingDst);
    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---u16DefaultPhase=%u\n", stXC_SetWin_Info.u16DefaultPhase);


    if(E_APIXC_RET_OK != MApi_XC_DIP_SetWindow(&stXC_SetWin_Info, sizeof(XC_SETWIN_INFO), enDIPWin)) {//Set DIP clip window AND scaling
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: MApi_XC_DIP_SetWindow Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(enDIPWin);
        return  FALSE;
    }
    memset(&stDIPWinProperty,0,sizeof(ST_XC_DIP_WINPROPERTY));
    stDIPWinProperty.u8BufCnt = mCaptureScreenLen / stCaptureInfo.u32MemorySize;
    stDIPWinProperty.u16Width = stCaptureInfo.u16CaptureWidth;
    stDIPWinProperty.u16Height = stCaptureInfo.u16CaptureHeight;
    stDIPWinProperty.u32BufStart = stCaptureInfo.u32MemoryPhyAddress;
    stDIPWinProperty.u32BufEnd = stCaptureInfo.u32MemoryPhyAddress + stCaptureInfo.u32MemorySize;
    stDIPWinProperty.u16Pitch = stCaptureInfo.u16CaptureWidth * ARGB8888_LEN;
    stDIPWinProperty.enSource = SCALER_DIP_SOURCE_TYPE_OP_CAPTURE;

    DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: ---DIP bufcount=%u, output w,h=%u,%u, u32BufStart, u32BufEnd = 0x%lx,0x%lx, enSource= %u\n", stDIPWinProperty.u8BufCnt, stDIPWinProperty.u16Width,  stDIPWinProperty.u16Height, stDIPWinProperty.u32BufStart, stDIPWinProperty.u32BufEnd, stDIPWinProperty.enSource);

    if(E_APIXC_RET_OK != MApi_XC_DIP_SetDIPWinProperty(&stDIPWinProperty, enDIPWin)) {
        DLOGE_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "[capture screen]: MApi_XC_DIP_SetDIPWinProperty Failed!!!\n");
        MApi_XC_DIP_ReleaseResource(enDIPWin);
        return  FALSE;
    }
    return TRUE;
}

void HWComposerDip::getTravelingRes(int *width, int *height) {
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
        DLOGD_IF(HWC2_DEBUGTYPE_DIP_CAPTURE, "Abnormal timing! Return 720*480, real timing is: %d*%d!", *width, *height);
        *width = 720;
        *height = 480;
    }
}
