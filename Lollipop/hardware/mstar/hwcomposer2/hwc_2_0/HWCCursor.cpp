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
#define LOG_TAG "HWCCursor"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define __CLASS__ "HWCCursor"

#include "HWCCursor.h"

HWCCursor::HWCCursor(HWCDisplayDevice& display)
    :mDisplay(display),
    mMutex(GAMECURSOR_COUNT),
    mExitMutex(GAMECURSOR_COUNT),
    mCv(GAMECURSOR_COUNT),
    mDisplayQueue(GAMECURSOR_COUNT),
    bThreadHasExit(GAMECURSOR_COUNT,TRUE),
    mCursorLayerCount(0),
    mHWCursorInfo(),
    mLastHWCursorInfo(GAMECURSOR_COUNT,std::make_shared<HWCursorInfo>()),
    mCursorPhyAddr(GAMECURSOR_COUNT),mCursorVaddr(GAMECURSOR_COUNT)
{
    for (int i = 0;i < GAMECURSOR_COUNT;i++) {
        auto hwCur = std::make_shared<HWCursorInfo>();
        mHWCursorInfo.push_back(std::move(hwCur));
    }

    mCursorDevice = std::make_unique<HWComposerCursorDev>(display);
}

HWCCursor::~HWCCursor() {

}

int HWCCursor::init(int32_t devNo) {
    auto hwcursorinfo = mHWCursorInfo.at(devNo);
    if (hwcursorinfo->bInitialized) {
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwcursordev_init has done, devNo=%d", devNo);
        if (bThreadHasExit[devNo]) {
            bThreadHasExit[devNo] = FALSE;
            DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursor_workthread created again");
            DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "create work thread,devNo[%d]",devNo);
            auto t = std::make_shared<std::thread>(&HWCCursor::cursorDisplayWorkThread,this,devNo);
            t->detach();
        }
    }
    const mmap_info_t* mmapInfo = NULL;
    MS_PHY gopAddr = 0;
    MS_PHY gopLen = 0;
    MS_PHY hwCursorAddr = 0;
    MS_PHY hwCursorLen = HWCURSOR_MAX_WIDTH*HWCURSOR_MAX_HEIGHT*BYTE_PER_PIXEL*3;// 3:three framebuffer
    void * hwCursorVaddr = NULL;
    int miu_sel =  0;
    int cmaEnable = 0;
    CMA_Pool_Init_Param CMA_Pool_Init_PARAM;
    CMA_Pool_Alloc_Param CMA_Alloc_PARAM;

    mmapInfo = mmap_get_info("E_MMAP_ID_HW_CURSOR");
    if (mmapInfo == NULL) {
        mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
        if (mmapInfo == NULL)
            return -1;
        gopAddr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        gopLen = mmapInfo->size;
        hwCursorAddr = gopAddr + gopLen - GAMECURSOR_COUNT * ((hwCursorLen+4095)&~4095U);
    } else {
        hwCursorAddr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
    }
    if (mmapInfo->cmahid > 0) {
        cmaEnable = 1;
    }

    mCursorPhyAddr[devNo] = hwCursorAddr + devNo * hwCursorLen;
    for (int i=0; i<HWCURSOR_DSTFB_NUM; i++) {
        hwcursorinfo->mHwCursorDstAddr[i] = mCursorPhyAddr[devNo] + hwCursorLen/3 * (i + 1);
    }
    // get gop & gwin
    MS_U8 gopNum = MApi_GOP_GWIN_GetMaxGOPNum();
    hwcursorinfo->gop = gopNum - 1 - devNo;
    hwcursorinfo->gwin = mCursorDevice->getAvailGwinId(hwcursorinfo->gop);

    if (hwCursorAddr >= mmap_get_miu1_offset()) {
        miu_sel = 2;
    } else if (hwCursorAddr >= mmap_get_miu0_offset()) {
        miu_sel = 1;
    } else {
        miu_sel = 0;
    }

    hwcursorinfo->gop_dest = mCursorDevice->getDestination();
    hwcursorinfo->channel = miu_sel;
    hwcursorinfo->releaseFenceFd = -1;
    hwcursorinfo->releaseSyncTimelineFd= sw_sync_timeline_create();
    if (hwcursorinfo->releaseSyncTimelineFd < 0) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "sw_sync_timeline_create failed");
    }

    mCursorDevice->init(hwcursorinfo);

    if (cmaEnable==1) {
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "MMAP_GOP_FB was integrated into CMA init it!!");
        void* fbVaddr = fbdev_mmap(0, gopLen, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
        hwCursorVaddr =  (void*)((char*)fbVaddr + gopLen - (GAMECURSOR_COUNT-devNo) * ((hwCursorLen+4095)&~4095U));
    } else {
        unsigned int maskPhyAddr = mCursorDevice->getMaskPhyAddr(miu_sel);
        if (miu_sel==2) {
            if (!MsOS_MPool_Mapping(2, (hwCursorAddr & maskPhyAddr), ((hwCursorLen+4095)&~4095U), 1)) {
                DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwcursordev_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            hwCursorVaddr = (void*)MsOS_PA2KSEG1((hwCursorAddr & maskPhyAddr) | mmap_get_miu1_offset());
        }else if (miu_sel==1){
            if (!MsOS_MPool_Mapping(1, (hwCursorAddr & maskPhyAddr), ((hwCursorLen+4095)&~4095U), 1)) {
                DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwcursordev_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            hwCursorVaddr = (void*)MsOS_PA2KSEG1((hwCursorAddr & maskPhyAddr) | mmap_get_miu0_offset());
        }else {
            if (!MsOS_MPool_Mapping(0, (hwCursorAddr & maskPhyAddr), ((hwCursorLen+4095)&~4095U), 1)) {
                DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwcursordev_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            hwCursorVaddr = (void*)MsOS_PA2KSEG1(hwCursorAddr & maskPhyAddr);
        }
    }
    memset(hwCursorVaddr, 0x0, hwCursorLen);
    mCursorVaddr[devNo] = (uintptr_t)hwCursorVaddr;
    if (!hwcursorinfo->bInitialized) {
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "create work thread");
        bThreadHasExit[devNo] = FALSE;
        auto t = std::make_shared<std::thread>(&HWCCursor::cursorDisplayWorkThread,this,devNo);
        t->detach();
    }
    hwcursorinfo->bInitialized = true;
    return 0;
}

int HWCCursor::deinit(int32_t devNo){
    auto hwcursorinfo = mHWCursorInfo.at(devNo);
    if (hwcursorinfo->bInitialized) {
        if (bThreadHasExit[devNo] != TRUE) {
            std::unique_lock <std::mutex> lock(mExitMutex[devNo]);
            bThreadHasExit[devNo] = TRUE;
            mCv[devNo].notify_all();
        }
        MApi_GOP_GWIN_Enable(hwcursorinfo->gwin, FALSE);
        return 1;
    }
    return 0;
}

hwc2_error_t HWCCursor::present(std::vector<std::shared_ptr<HWCLayer>>& layers, int32_t* outRetireFence) {
    auto hwcursorinfo = mHWCursorInfo.at(0);
    auto lastcursorinfo = mLastHWCursorInfo.at(0);
    hwcursorinfo->layerIndex = -1;
    hwcursorinfo->releaseFenceFd = -1;
    if (layers.size() > 0) {
        auto layer = layers.at(0);

        DLOGD_IF(HWC2_DEBUGTYPE_SHOW_CURSOR_LAYERS,"Cursor layer :%s",layer->dump().c_str());
        hwcursorinfo->layerIndex = layer->getIndex();
        if (!hwcursorinfo->bInitialized) {
            init(0);
            hwcursorinfo->bInitialized = true;
        }
        buffer_handle_t hnd = layer->getLayerBuffer().getBuffer();
        if (hnd == nullptr) {
            return HWC2_ERROR_NOT_VALIDATED;
        }
        private_handle_t *handle = (private_handle_t *)hnd;
        hwcursorinfo->acquireFenceFd = layer->getLayerBuffer().getFence();
        hwcursorinfo->planeAlpha = layer->getLayerPlaneAlpha() * 0xFF + 0.5;
        //check timing changed
        if ((lastcursorinfo->gop_dstWidth!= mDisplay.mCurTimingWidth) ||
           (lastcursorinfo->gop_dstHeight != mDisplay.mCurTimingHeight) ||
           (lastcursorinfo->gop_srcWidth != mDisplay.mDisplayRegionWidth) ||
           (lastcursorinfo->gop_srcHeight != mDisplay.mDisplayRegionHeight) ||
           (checkCursorFbNeedRedraw(layers))) {
            if (handle->size <= HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4) {
                memcpy((void*)mCursorVaddr[0], handle->base, handle->size);
                #if 0
                {
                    char filename[100];
                    int num  = -1;
                    while (++num < 10000) {
                        snprintf(filename, 100, "%s/%s_%04d.raw",
                                "/data/cursor","mDstCursorVaddr",num);

                        if (access(filename, F_OK) != 0) {
                            break;
                        }
                    }
                    DLOGD("hwcursor filename  = %s",filename);
                    int fd_p = open(filename, O_EXCL | O_CREAT | O_WRONLY, 0644);
                    if (fd_p < 0) {
                        DLOGD("[capture screen]:%s  can't open",filename);
                        return HWC2_ERROR_NOT_VALIDATED;
                    }
                    //dump src buffer
                    write(fd_p, (void *)(mCursorVaddr[0]), HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4);
                    //dump dst 1 buffer
                    //write(fd_p, (void *)(mCursorVaddr[0] + HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4), HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4);
                    //dump dst 2 buffer
                    //write(fd_p, (void *)(mCursorVaddr[0] + HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4 * 2), HWCURSOR_MAX_WIDTH * HWCURSOR_MAX_HEIGHT * 4);
                    close(fd_p);
                }
                #endif
            }
            hwcursorinfo->srcAddr = mCursorPhyAddr[0];
            hwcursorinfo->srcX = (int)layer->getLayerSourceCrop().left;
            hwcursorinfo->srcY = (int)layer->getLayerSourceCrop().top;
            hwcursorinfo->srcWidth = (int)layer->getLayerSourceCrop().right - (int)layer->getLayerSourceCrop().left;
            hwcursorinfo->srcHeight = (int)layer->getLayerSourceCrop().bottom - (int)layer->getLayerSourceCrop().top;
            hwcursorinfo->dstWidth = ALIGN((layer->getLayerDisplayFrame().right - layer->getLayerDisplayFrame().left),GWIN_ALIGNMENT);
            hwcursorinfo->dstHeight = layer->getLayerDisplayFrame().bottom - layer->getLayerDisplayFrame().top;
            hwcursorinfo->srcStride = handle->byte_stride;

            hwcursorinfo->gop_srcWidth = mDisplay.mDisplayRegionWidth ;
            hwcursorinfo->gop_srcHeight = mDisplay.mDisplayRegionHeight;
            hwcursorinfo->gop_dstWidth = mDisplay.mCurTimingWidth;
            hwcursorinfo->gop_dstHeight = mDisplay.mCurTimingHeight;
            hwcursorinfo->positionX = layer->getLayerDisplayFrame().left;
            hwcursorinfo->positionY = layer->getLayerDisplayFrame().top;
            hwcursorinfo->operation |= HWCURSOR_ICON;
        }
    }
    auto result = display();  // work thread will signal releaseFence in last composition

    if (result == HWC2_ERROR_NONE) {
        if (hwcursorinfo->layerIndex >= 0) {
            auto layer = layers.at(0);
            layer->setLayerReleaseFence(hwcursorinfo->releaseFenceFd);
        }
    }

    return result;

}

hwc2_error_t HWCCursor::display() {
    auto hwcursorinfo = mHWCursorInfo.at(0);
    auto lastcursorinfo = mLastHWCursorInfo.at(0);
    if (hwcursorinfo->layerIndex < 0) {
        if (lastcursorinfo->layerIndex >= 0) {
            //close gwin
            if (hwcursorinfo->operation & HWCURSOR_SHOW) {
                hwcursorinfo->operation &=~HWCURSOR_SHOW;
            }
            hwcursorinfo->operation |= HWCURSOR_HIDE;
        } else {
            return HWC2_ERROR_NONE;
        }
    } else {
        if (lastcursorinfo->layerIndex < 0) {
            //open gwin
            if (hwcursorinfo->operation & HWCURSOR_HIDE) {
                hwcursorinfo->operation &=~HWCURSOR_HIDE;
            }
            hwcursorinfo->operation |= HWCURSOR_SHOW;
        }
    }
    if (hwcursorinfo->operation) {
        commitTransaction(0);
    }
    return HWC2_ERROR_NONE;
}

void HWCCursor::commitTransaction(int devNo) {
    std::unique_lock <std::mutex> lock(mMutex[devNo]);
    auto hwcursorinfo = mHWCursorInfo.at(devNo);
    auto tansaction = std::make_shared<HWCursorInfo>(*hwcursorinfo);
    DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "Main thread devNo[%d] push cursor info :%s",devNo,tansaction->toString().c_str());
    hwcursorinfo->operation = 0;
    mDisplayQueue[devNo].push(tansaction);
    mCv[devNo].notify_all();
}

void HWCCursor::cursorDisplayWorkThread(int devNo) {
    DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "present enter in work thread[%d],tid:%lx",devNo,std::this_thread::get_id());
    char thread_name[64] = {0};
    MS_BOOL bGameCursorExit = FALSE;
    snprintf(thread_name, 64, "hwCursor%d_thread", devNo);
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);
    while(true) {
        {
            std::unique_lock <std::mutex> lock(mExitMutex[devNo]);
            bGameCursorExit = bThreadHasExit[devNo];
        }
        if (bGameCursorExit) {
            DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursor_workthread quit devNo=%d",devNo);
            break;
        }
        std::unique_lock <std::mutex> lock(mMutex[devNo]);
        if (mDisplayQueue[devNo].empty()) {
            mCv[devNo].wait(lock);
        }
        if (mDisplayQueue[devNo].empty()) {
            continue;
        }
        auto hwcursorInfo = mDisplayQueue[devNo].front();
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "work thread:devNo[%d] get cursor info :%s",devNo,hwcursorInfo->toString().c_str());
        mCursorDevice->present(hwcursorInfo, devNo);
        mLastHWCursorInfo[devNo] = hwcursorInfo;
        mDisplayQueue[devNo].pop();
    }

}

hwc2_error_t HWCCursor::redrawFb(int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight) {
    if (!mHWCursorInfo[0]->bInitialized || (mDisplay.mGameCursorEnabled)) {
        return HWC2_ERROR_BAD_CONFIG;
    }
    mHWCursorInfo[0]->srcAddr = mCursorPhyAddr[0];
    mHWCursorInfo[0]->srcX = srcX;
    mHWCursorInfo[0]->srcY = srcY;
    mHWCursorInfo[0]->srcWidth = srcWidth;
    mHWCursorInfo[0]->srcHeight = srcHeight;
    mHWCursorInfo[0]->srcStride = stride;
    mHWCursorInfo[0]->dstWidth = ALIGN(dstWidth,GWIN_ALIGNMENT);
    mHWCursorInfo[0]->dstHeight = dstHeight;
    mHWCursorInfo[0]->gop_srcWidth = mDisplay.mDisplayRegionWidth ;
    mHWCursorInfo[0]->gop_srcHeight = mDisplay.mDisplayRegionHeight;
    mHWCursorInfo[0]->gop_dstWidth = mDisplay.mCurTimingWidth;
    mHWCursorInfo[0]->gop_dstHeight = mDisplay.mCurTimingHeight;

    mHWCursorInfo[0]->operation |= HWCURSOR_ICON;

    //DLOGW_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursor_redrawFb:[%d,%d, %d,%d,%d,%d,%d], pid=%d, tid=%d", srcX,srcY,srcWidth,srcHeight,stride,dstWidth,dstHeight,getpid(),gettid());
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCCursor::setCursorPositionAsync(int x_pos, int y_pos) {
    if ((!mHWCursorInfo[0]->bInitialized)||(mDisplay.mGameCursorEnabled)) {
        return HWC2_ERROR_BAD_CONFIG;
    }
    mHWCursorInfo[0]->positionX = x_pos;
    mHWCursorInfo[0]->positionY = y_pos;
    mHWCursorInfo[0]->operation |= HWCURSOR_POSITION;

    commitTransaction(0);
    return HWC2_ERROR_NONE;
}

std::string HWCCursor::dump() const {
    std::stringstream output;
    output << mLastHWCursorInfo[0]->toString() << "\n";

    return output.str();
}

bool HWCCursor::checkCursorFbNeedRedraw(std::vector<std::shared_ptr<HWCLayer>>& layers) {
    bool bCursorFbNeedRedraw = false;
    if (mCursorLayerCount == 0) {
        bCursorFbNeedRedraw = true;
    }
    mCursorLayerCount = layers.size();
    for (auto& layer:layers) {
        if (layer->isDirty()) {
            bCursorFbNeedRedraw = true;
        }
    }
    if (mDisplay.b3DModeChanged) {
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "3D Mode changed need redraw cursor");
        bCursorFbNeedRedraw = true;
    }
    if (mDisplay.mGameCursorChanged) {
      bCursorFbNeedRedraw = true;
      mDisplay.mGameCursorChanged= false;
    }
    return bCursorFbNeedRedraw;
}

#ifdef ENABLE_GAMECURSOR
hwc2_error_t HWCCursor::gameCursorShow(int32_t devNo) {
    mHWCursorInfo[devNo]->operation |= HWCURSOR_SHOW;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCCursor::gameCursorHide(int32_t devNo) {
    mHWCursorInfo[devNo]->operation |= HWCURSOR_HIDE;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCCursor::gameCursorSetAlpha(int32_t devNo, float alpha) {
    mHWCursorInfo[devNo]->planeAlpha = alpha*255;
    mHWCursorInfo[devNo]->operation |= HWCURSOR_ALPHA;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCCursor::gameCursorSetPosition(int32_t devNo, int32_t positionX, int32_t positionY) {
    mHWCursorInfo[devNo]->positionX = positionX;
    mHWCursorInfo[devNo]->positionY = positionY;
    mHWCursorInfo[devNo]->operation |= HWCURSOR_POSITION;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCCursor::gameCursorRedrawFb(int32_t devNo, int srcX, int srcY, int srcWidth,
                        int srcHeight, int stride,int dstWidth, int dstHeight) {
    mHWCursorInfo[devNo]->srcAddr = mCursorPhyAddr[devNo];
    mHWCursorInfo[devNo]->srcX = srcX;
    mHWCursorInfo[devNo]->srcY = srcY;
    mHWCursorInfo[devNo]->srcWidth = srcWidth;
    mHWCursorInfo[devNo]->srcHeight = srcHeight;
    mHWCursorInfo[devNo]->srcStride = stride;
    mHWCursorInfo[devNo]->dstWidth = ALIGN(dstWidth,GWIN_ALIGNMENT);
    mHWCursorInfo[devNo]->dstHeight = dstHeight;
    mHWCursorInfo[devNo]->gop_srcWidth = mDisplay.mDisplayRegionWidth ;
    mHWCursorInfo[devNo]->gop_srcHeight = mDisplay.mDisplayRegionHeight;
    mHWCursorInfo[devNo]->gop_dstWidth = mDisplay.mCurTimingWidth;
    mHWCursorInfo[devNo]->gop_dstHeight = mDisplay.mCurTimingHeight;

    mHWCursorInfo[devNo]->operation |= HWCURSOR_ICON;
    //DLOGW_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursor_redrawFb[%d]:[%d,%d, %d,%d,%d,%d,%d], pid=%d, tid=%d", devNo, srcX,srcY,srcWidth,srcHeight,stride,dstWidth,dstHeight,getpid(),gettid());
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCCursor::gameCursorDoTransaction(int32_t devNo) {
    if (!mHWCursorInfo[devNo]->bInitialized) {
        return HWC2_ERROR_BAD_CONFIG;
    }

    mHWCursorInfo[devNo]->acquireFenceFd = -1;
    mHWCursorInfo[devNo]->releaseFenceFd = -1;
    commitTransaction(devNo);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCCursor::gameCursorInit(int32_t devNo) {
    int res = init(devNo);
    if (res == -1) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "mmapInfo == NULL");
        return HWC2_ERROR_BAD_CONFIG;
    }
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCCursor::gameCursorDeinit(int32_t devNo) {
    int res = deinit(devNo);
    if (res == 1) {
        mHWCursorInfo[devNo]->operation = 0;
    }
    return HWC2_ERROR_NONE;

}

hwc2_error_t HWCCursor::gameCursorGetFbVaddrByDevNo(int32_t devNo, uintptr_t* vaddr) {
    if (!(mHWCursorInfo[devNo]->bInitialized)) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwcursor module not init!!!");
        *vaddr = 0;
        return HWC2_ERROR_BAD_CONFIG;
    }
    *vaddr = mCursorVaddr[devNo];
    return HWC2_ERROR_NONE;
}
#endif
