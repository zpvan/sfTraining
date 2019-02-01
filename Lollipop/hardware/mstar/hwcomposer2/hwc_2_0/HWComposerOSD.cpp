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
#define LOG_TAG "HWComposerOSD"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "HWComposerOSD.h"
#include <cutils/log.h>
#include <gralloc_priv.h>
#define __CLASS__ "HWComposerOSD"

HWComposerOSD::HWComposerOSD(HWCDisplayDevice& display)
    :mDisplay(display),
    mHwwininfo(),
    mlastHWWindowInfo()
{
    mGopDevice = std::make_unique<HWComposerGop>(display);
    init();
}

void HWComposerOSD::init() {

    char property[PROPERTY_VALUE_MAX] = {0};
    dictionary *sysINI, *modelINI, *panelINI, *moduleINI, *hwcINI, *dfbrcFile;
    int width=0;
    int height=0;
    int fakefreq = 60;

    if (mmap_init() != 0) {
        return;
    }
    initXcGfx();

    if(openConfigFile(&sysINI, &modelINI, &panelINI, &moduleINI, &dfbrcFile, &hwcINI)== -1) {
        DLOGE("Open conifg file fail");
    }

    if (property_get("mstar.bootlogo.gop", property, NULL) > 0) {
        mDisplay.mBootlogo.gop = atoi(property);
        if (property_get("mstar.close.bootlogo.gop.frame", property, NULL) > 0) {
            mDisplay.mBootlogo.closeThreshold = atoi(property);
        }
        DLOGD("bootlogo gop [%d],frame [%d]",mDisplay.mBootlogo.gop,mDisplay.mBootlogo.closeThreshold);
        mDisplay.mBootlogo.bGopClosed = false;
        mDisplay.mBootlogo.frameCountAfterBoot = 0;
    } else {
        mDisplay.mBootlogo.bGopClosed = true;
    }

    property_get("mstar.fakevsync.freq", property, "60");
    fakefreq = atoi(property);
    DLOGD("the mstar.fakevsync.freq is %d",fakefreq);
    mDisplay.mVsyncPeriod = 1000000000.0 / fakefreq;

    mDisplay.mOsdWidth = mDisplay.mWidth = iniparser_getint(panelINI, "panel:osdWidth", 1920);
    mDisplay.mOsdHeight = mDisplay.mHeight = iniparser_getint(panelINI, "panel:osdHeight", 1080);
    if (property_get("ro.sf.osd_size", property, NULL) > 0) {
        if ((0==strcmp(property,"1080p")) || (0==strcmp(property,"1080P"))) {
            mDisplay.mOsdWidth = mDisplay.mWidth = 1920;
            mDisplay.mOsdHeight = mDisplay.mHeight = 1080;
            DLOGD("the ro.sf.lcd_size is set as 1080p");
        } else if ((0==strcmp(property,"720p")) || (0==strcmp(property,"720P"))) {
            mDisplay.mOsdWidth = mDisplay.mWidth = 1280;
            mDisplay.mOsdHeight = mDisplay.mHeight = 720;
            DLOGD("the ro.sf.lcd_size is set as 720p");
        } else {
            DLOGD("warning!! wrong value is set to the ro.sf.lcd_size ");
        }
    }

    if (property_get("ro.sf.lcd_density", property, NULL) > 0) {
        mDisplay.mXdpi = mDisplay.mYdpi = atoi(property) * 1000;
    } else if (mDisplay.mWidth <= 1920 && mDisplay.mHeight <= 1080) {
        mDisplay.mXdpi = mDisplay.mYdpi = 240 * 1000;
    } else {
        mDisplay.mXdpi = mDisplay.mYdpi = 480 * 1000;
    }
    mDisplay.mPanelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
    mDisplay.mPanelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
    mDisplay.mUrsaVersion = iniparser_getint(moduleINI, "M_URSA:F_URSA_URSA_TYPE", 0);
    mDisplay.mOsdc_enable = iniparser_getboolean(moduleINI, "M_BACKEND:F_BACKEND_ENABLE_OSDC", 0);
    mDisplay.mPanelLinkType = iniparser_getint(panelINI, "panel:m_ePanelLinkType", 10);
    mDisplay.mPanelLinkExtType = iniparser_getint(panelINI, "panel:m_ePanelLinkExtType", 51);
    property_get("mstar.build.for.stb", property, "false");
    if (strcmp(property, "true") == 0) {
        mDisplay.bStbTarget = true;
        mDisplay.bRGBColorSpace = false;
        DLOGD("the device is for STB");
    } else {
        mDisplay.bStbTarget = false;
        mDisplay.bRGBColorSpace = true;
        DLOGD("the device is NOT for STB");
    }

    // for ctx->bRGBColorSpace ,mstar.ColorSpace.bRGB has higher priority
    // than mstar.build.for.stb. property mstar.build.for.stb is get firstly and then
    // get the mstar.colorspace.brgb

    memset(property,0,sizeof(property));
    if (property_get("mstar.colorspace.brgb",property,NULL) > 0) {
        if (strcmp(property,"true") == 0) {
            mDisplay.bRGBColorSpace = true;
            DLOGD("force the colorspace to be rgb");
        } else if (strcmp(property,"false") == 0) {
            mDisplay.bRGBColorSpace = false;
            DLOGD("force the colorspace to be yuv ");
        }
    }

    mDisplay.mGopDest = mGopDevice->getDestination();
    if(mDisplay.mGopDest == E_GOP_DST_FRC) {
        mGopDevice->getCurOCTiming(&mDisplay.mCurTimingWidth, &mDisplay.mCurTimingHeight, &mDisplay.mPanelHStart);
    } else {
        mGopDevice->getCurOPTiming(&mDisplay.mCurTimingWidth, &mDisplay.mCurTimingHeight, &mDisplay.mPanelHStart);
    }
    DLOGE("curTimingWidth =%d,curTimingHeight =%d, panelHStart=%d\n",mDisplay.mCurTimingWidth,mDisplay.mCurTimingHeight,mDisplay.mPanelHStart);

    if (mDisplay.bStbTarget) {
        setStbResolutionState();
    }

    mDisplay.mDisplayRegionWidth = mDisplay.mCurTimingWidth;
    mDisplay.mDisplayRegionHeight = mDisplay.mCurTimingHeight;

    if (!mDisplay.bStbTarget) {
        if (mDisplay.mPanelWidth < mDisplay.mOsdWidth) {
            mDisplay.mWidth = ALIGNMENT(mDisplay.mPanelWidth,16);
        }

        if (mDisplay.mPanelHeight < mDisplay.mOsdHeight) {
            mDisplay.mHeight = ALIGNMENT(mDisplay.mPanelHeight,16);
        }
    }

    if ((mDisplay.mWidth != RES_4K2K_WIDTH)||(mDisplay.mHeight != RES_4K2K_HEIGHT)) {
        property_set("mstar.4k2k.2k1k.coexist","false");
    }
    getMirrorMode(modelINI, &mDisplay.mMirrorModeH, &mDisplay.mMirrorModeV);

    MApi_GOP_GWIN_ResetPool();
    //first is gop index,second is layer index
    std::map<int,int> mGopLayerMap;


    MS_U32 u32size = sizeof(GOP_LayerConfig);
    memset(&mDisplay.stLayerSetting,0,u32size);
    char layerStr[128] = "";
    mDisplay.stLayerSetting.u32LayerCounts = iniparser_getunsignedint(dfbrcFile, "DirectFBRC:DFBRC_LAYERCOUNTS", 0);
    for (unsigned int i = 0; i < mDisplay.stLayerSetting.u32LayerCounts; i++) {
        sprintf(layerStr,"DirectFBRC:DFBRC_LAYER%d_GOPINDEX",i);
        mDisplay.stLayerSetting.stGopLayer[i].u32GopIndex = iniparser_getunsignedint(dfbrcFile, layerStr, 0);
        mDisplay.stLayerSetting.stGopLayer[i].u32LayerIndex = i;
        //save gop and layer to map
        mGopLayerMap[mDisplay.stLayerSetting.stGopLayer[i].u32GopIndex] = i;
        DLOGD("%s",layerStr);
        DLOGD("the layer%d  gop index is %d",i,mDisplay.stLayerSetting.stGopLayer[i].u32GopIndex);
    }
    std::vector<int> HwcGop;

    //Get osd gop infomation form hwcomposer13.ini,and save layer's index to HwcGop temporary,it is used for sort.
    int index = 0;
    char gopsetting[40];
    sprintf(gopsetting, "hwcomposer:HWC_GOP_IN_HWWINDOW%d", index);
    while (iniparser_getint(hwcINI, gopsetting, 0xff) != 0xff) {
        int gop = iniparser_getint(hwcINI, gopsetting, 0xff);
        if ((mGopLayerMap.size() > 0)) {
            HwcGop.push_back(mGopLayerMap[gop]);
        } else {
            HwcGop.push_back(gop);
        }
        index++;
        memset(gopsetting,0,40);
        sprintf(gopsetting, "hwcomposer:HWC_GOP_IN_HWWINDOW%d", index);
    }

    if (mGopLayerMap.size() > 0) {
        std::sort(HwcGop.begin(),HwcGop.end());
        //restore gop infomation to HwcGop
        for (int i=0; i<HwcGop.size(); i++) {
            for(auto it:mGopLayerMap) {
                if (it.second == HwcGop.at(i)) {
                    HwcGop[i] = it.first;
                    break;
                }
            }
        }
    }

    if (HwcGop.size() > 0) {
       mDisplay.mGopCount = HwcGop.size();
    } else {
       DLOGE("error !!! there is no gop for hwcomposer,maybe there is no hwcini file");
    }
#ifdef MALI_AFBC_GRALLOC
    GOP_CAP_AFBC_INFO sCaps;
    MApi_GOP_GetChipCaps(E_GOP_CAP_AFBC_SUPPORT, &sCaps, sizeof(GOP_CAP_AFBC_INFO));
#endif
    for (int i = 0;i < mDisplay.mGopCount;i++) {
        auto hwWin = std::make_shared<HWWindowInfo>();
        hwWin->gop = HwcGop.at(i);
        hwWin->gwin = mGopDevice->getAvailGwinId(hwWin->gop);
        DLOGD("%s , hwWin  gop = %d, gwin = %d",__FUNCTION__,hwWin->gop,hwWin->gwin);

        if ((hwWin->gop == 0xff)||(hwWin->gwin == 0xff)) {
            DLOGE("warning !!! ,the gopInfo has no gop&gwin maybe there is no  hwcini file");
        }
#ifdef MALI_AFBC_GRALLOC
        if (sCaps.GOP_AFBC_Support & mGopDevice->getBitMask(hwWin->gop)) {
            hwWin->bSupportGopAFBC = true;
        } else
#endif
        {
            hwWin->bSupportGopAFBC = false;
        }
        hwWin->channel = E_GOP_SEL_MIU0;
        hwWin->layerIndex = -1;
        hwWin->fbid = INVALID_FBID;
        hwWin->releaseFenceFd = -1;
        hwWin->acquireFenceFd = -1;
        hwWin->subfbid = INVALID_FBID;
        hwWin->gop_dest = mDisplay.mGopDest;
        hwWin->releaseSyncTimelineFd = sw_sync_timeline_create();
        if (hwWin->releaseSyncTimelineFd < 0) {
            DLOGE("sw_sync_timeline_create failed");
        }
        hwWin->releaseFenceValue = 0;
        hwWin->ionBufferSharedFd = -1;
        mHwwininfo.push_back(std::move(hwWin));
    }
    mlastHWWindowInfo.assign(mDisplay.mGopCount,std::make_shared<HWWindowInfo>());
    initCmdqAndMload();
    clearGfxFbBuffer();

    closeConfigFile(sysINI, modelINI, panelINI, moduleINI, dfbrcFile, hwcINI);


    DLOGD("create work thread");
    auto t = std::make_shared<std::thread>(&HWComposerOSD::osdDisplayWorkThread,this);
    t->detach();

    DLOGD("MStar graphic Config:\n");
    DLOGD("OSD WIDTH    : %d\n", mDisplay.mOsdWidth);
    DLOGD("OSD HEIGHT   : %d\n", mDisplay.mOsdHeight);
    DLOGD("PANEL WIDTH  : %d\n", mDisplay.mPanelWidth);
    DLOGD("PANEL HEIGHT : %d\n", mDisplay.mPanelHeight);
    DLOGD("DISP WIDTH   : %d\n", mDisplay.mWidth);
    DLOGD("DISP HEIGHT  : %d\n", mDisplay.mHeight);
    DLOGD("MIRROE MODE  H : %d,V : %d\n",mDisplay.mMirrorModeH, mDisplay.mMirrorModeV);
    DLOGD("URSA VERSION : %d\n", mDisplay.mUrsaVersion);
}

HWComposerOSD::~HWComposerOSD(){}

bool HWComposerOSD::checkGopTimingChanged() {
    bool timing_changed = false;
    int OPTimingWidth = mDisplay.mCurTimingWidth;
    int OPTimingHeight = mDisplay.mCurTimingHeight;
    int PanelHstart = mDisplay.mPanelHStart;
    int OCTimingWidth = mDisplay.mCurTimingWidth;
    int OCTimingHeight = mDisplay.mCurTimingHeight;

    // check gop dest changed
    unsigned int gop_dest = getGopDest();
    if (gop_dest != mHwwininfo[0]->gop_dest) {
        if (gop_dest == E_GOP_DST_OP0) {
            OPTimingHeight = 0;
            OPTimingWidth = 0;
        } else if (gop_dest == E_GOP_DST_FRC) {
            OCTimingWidth = 0;
            OCTimingHeight = 0;
        }
        for (int i=0; i<mDisplay.mGopCount; i++) {
            MApi_GOP_GWIN_SetGOPDst(mHwwininfo[i]->gop, (EN_GOP_DST_TYPE)gop_dest);
            mHwwininfo[i]->gop_dest = gop_dest;
        }
    }

    // check timing changed
    if (gop_dest == E_GOP_DST_OP0) {
        int width = 0;
        int height = 0;
        int hstart = 0;
        mGopDevice->getCurOPTiming(&width, &height, &hstart);

        if ((width != OPTimingWidth)||(height != OPTimingHeight)||(hstart != PanelHstart)) {
            for (int i=0; i<mDisplay.mGopCount; i++) {
                ST_GOP_TIMING_INFO stGopTiming;
                memset(&stGopTiming, 0, sizeof(ST_GOP_TIMING_INFO));
                MApi_GOP_GetConfigEx(mHwwininfo[i]->gop, E_GOP_TIMING_INFO, (MS_U32*)&stGopTiming);
                stGopTiming.u16DEHSize = width;
                stGopTiming.u16DEVSize = height;
                stGopTiming.u16DEHStart = hstart;
                MApi_GOP_SetConfigEx(mHwwininfo[i]->gop, E_GOP_TIMING_INFO, (MS_U32*)&stGopTiming);
                DLOGE("the new timing is %d :%d, hstar is %d \n",width,height,hstart);
            }
            OPTimingWidth = width;
            OPTimingHeight = height;
            PanelHstart = hstart;
            mDisplay.mCurTimingWidth = OPTimingWidth;
            mDisplay.mCurTimingHeight = OPTimingHeight;
            mDisplay.mPanelHStart = PanelHstart;
            timing_changed = true;
#ifdef MALI_AFBC_GRALLOC
            mDisplay.bAFBCNeedReset = true;
#endif
        }
    } else if (gop_dest == E_GOP_DST_FRC) {
        int width = 0;
        int height = 0;
        int hstart = 0;
        mGopDevice->getCurOCTiming(&width,&height,&hstart);

        if ((width != OCTimingWidth)||(height != OCTimingHeight)) {
            OCTimingWidth = width;
            OCTimingHeight = height;
            mDisplay.mCurTimingWidth = OCTimingWidth;
            mDisplay.mCurTimingHeight = OCTimingHeight;
            mDisplay.mPanelHStart = PanelHstart;
            timing_changed = true;
#ifdef MALI_AFBC_GRALLOC
            mDisplay.bAFBCNeedReset = true;
#endif
        }
    }
   return timing_changed;
}

uint32_t HWComposerOSD::getGopDest() {
    return mGopDevice->getDestination();
}

bool HWComposerOSD::isGopSupportAFBC(int index) {
    return mHwwininfo[index]->bSupportGopAFBC;
}

hwc2_error_t HWComposerOSD::present(std::vector<std::shared_ptr<HWCLayer>>& layers, int32_t* outRetireFence) {
    std::unique_lock <std::mutex> lock(mMutex);
    int count = layers.size();
    if (count > mDisplay.mGopCount) {
        DLOGE("present failed,HW window is more than gop count!!!!");
        return HWC2_ERROR_NOT_VALIDATED;
    }
    for (int i = 0;i < mDisplay.mGopCount;i++) {
        auto hwWin = mHwwininfo.at(i);
        hwWin->bFrameBufferTarget = false;
        hwWin->bEnableGopAFBC = false;
        hwWin->bAFBCNeedReset = false;
        hwWin->layerIndex = -1;
        hwWin->releaseFenceFd = -1;
        hwWin->ionBufferSharedFd = -1;
    }
    for (int i = 0;i < count;i++) {
        auto layer = layers.at(i);
        auto hwWin = mHwwininfo.at(i);
        DLOGD_IF(HWC2_DEBUGTYPE_SHOW_OSD_LAYERS,"OSD layer :%s",layer->dump().c_str());
        hwWin->layerIndex = layer->getIndex();
        buffer_handle_t hnd = layer->getLayerBuffer().getBuffer();
        if (hnd == nullptr) {
            return HWC2_ERROR_NOT_VALIDATED;
        }
        private_handle_t *handle = (private_handle_t *)hnd;
        hwWin->physicalAddr = handle->hw_base;
        hwWin->virtualAddr = (uintptr_t)handle->base;
        if (hwWin->physicalAddr >= mmap_get_miu1_offset()) {
            hwWin->channel  = 2;
        } else if (hwWin->physicalAddr >= mmap_get_miu0_offset()) {
            hwWin->channel  = 1;
        } else {
            hwWin->channel = 0;
        }
        hwWin->format = handle->format;
        hwWin->stride = handle->byte_stride;
        hwWin->blending = layer->getLayerBlendMode();
        hwWin->planeAlpha = layer->getLayerPlaneAlpha() * 0xFF + 0.5;
        hwWin->acquireFenceFd = layer->getLayerBuffer().getFence();
        hwWin->bFrameBufferTarget = layer->isClientTarget();
        //layer->getLayerBuffer().setFence(-1);
        // TODO:create release fence
        hwWin->releaseFenceValue++;
        char str[64];
        sprintf(str, "releaseFence %d", hwWin->releaseFenceValue);
        hwWin->releaseFenceFd = sw_sync_fence_create( hwWin->releaseSyncTimelineFd,str, hwWin->releaseFenceValue);
        DLOGD_IF(HWC2_DEBUGTYPE_FENCE,"create release fenceFd [%d][%s]",hwWin->releaseFenceFd,str);
        if (hwWin->releaseFenceFd < 0) {
            DLOGE("fence create failed: %s,str : %s", strerror(errno),str);
        }
        if (handle->share_fd >= 0) {
            hwWin->ionBufferSharedFd = dup(handle->share_fd);
            if (hwWin->ionBufferSharedFd < 0)
                DLOGE("error dup handle->share_fd failed.....");
        }
#ifdef MALI_AFBC_GRALLOC
        if ((handle->internal_format & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK))
            && hwWin->bSupportGopAFBC) {
            hwWin->bEnableGopAFBC = true;
        }
        //Only have one AFBC Engineer,so we reset it by any HWInfo's status.
        if (mDisplay.bAFBCNeedReset) {
            hwWin->bAFBCNeedReset = true;
        }
#endif
        hwWin->bufferWidth = handle->width;
        hwWin->bufferHeight = handle->height;
        hwWin->srcX = (unsigned int)layer->getLayerSourceCrop().left;
        hwWin->srcY = (unsigned int)layer->getLayerSourceCrop().top;
        hwWin->srcWidth = (unsigned int)layer->getLayerSourceCrop().right - (unsigned int)layer->getLayerSourceCrop().left;
        hwWin->srcHeight = (unsigned int)layer->getLayerSourceCrop().bottom - (unsigned int)layer->getLayerSourceCrop().top;

        hwWin->dstX = layer->getLayerDisplayFrame().left*mDisplay.getDisplayScaleRatioW() + 0.5;
        hwWin->dstY = layer->getLayerDisplayFrame().top*mDisplay.getDisplayScaleRatioH() + 0.5;
        hwWin->dstWidth = (layer->getLayerDisplayFrame().right - layer->getLayerDisplayFrame().left)*mDisplay.getDisplayScaleRatioW() + 0.5;
        hwWin->dstHeight = (layer->getLayerDisplayFrame().bottom - layer->getLayerDisplayFrame().top)*mDisplay.getDisplayScaleRatioH() + 0.5;
        DLOGD_IF(HWC2_DEBUGTYPE_SHOW_HWWINDOWINFO,"push : %s to queue in main thread!!!",hwWin->toString().c_str());
    }
    HWWindowInfo_data windowInfoPresent;
    for (auto win:mHwwininfo) {
        auto workHwWin = std::make_shared<HWWindowInfo>(*win);
        windowInfoPresent.push_back(std::move(workHwWin));
    }
    mDisplayQueue.push(windowInfoPresent);
    mCv.notify_all();
    // TODO:create retire fence
    int retireFenceFd = -1;
    for (int i = 0;i < count;i++) {
        auto layer = layers.at(i);
        auto hwWin = mHwwininfo.at(i);
        layer->setLayerReleaseFence(hwWin->releaseFenceFd);
        if (retireFenceFd < 0 && hwWin->bFrameBufferTarget) {
            retireFenceFd = layer->getLayerReleaseFence();
        }
    }
    *outRetireFence = retireFenceFd;
    return HWC2_ERROR_NONE;
}
void HWComposerOSD::osdDisplayWorkThread() {

    char thread_name[64] = "graphic_workthread";
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    while(true) {
        std::unique_lock <std::mutex> lock(mMutex);
        if (mDisplayQueue.empty()) {
            mCv.wait(lock);
        }
        HWWindowInfo_data windows = mDisplayQueue.front();
        display(windows);
        mDisplayQueue.pop();
    }
}

hwc2_error_t HWComposerOSD::display(HWWindowInfo_data& inHWwindowinfo) {
    // calc fps
    {
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
            DLOGD_IF(HWC2_DEBUGTYPE_SHOW_FPS,"from hwcomposer20================the fps is %f",fps);
        }
    }
    for (auto win:inHWwindowinfo) {
        if (win->layerIndex >= 0) {
            DLOGD_IF(HWC2_DEBUGTYPE_SHOW_HWWINDOWINFO,"get :%s in work thread",win->toString().c_str());
            int acquireFenceFd = win->acquireFenceFd;
            if (acquireFenceFd >= 0 ) {
                int err = sync_wait(acquireFenceFd, 1000); // 1000 ms
                if (err<0) {
                    DLOGE("acquirefence[%d] sync_wait failed in work thread",acquireFenceFd);
                }
                DLOGD_IF(HWC2_DEBUGTYPE_FENCE,"close acquireFenceFd : %d",acquireFenceFd);
                close(acquireFenceFd);
                win->acquireFenceFd = -1;
            }
        }
    }
    mGopDevice->display(inHWwindowinfo,mlastHWWindowInfo);
    // we should wait the register is update and then signal the release fence
    for (auto window:mlastHWWindowInfo) {
        // trigger release fence
        sw_sync_timeline_inc(window->releaseSyncTimelineFd,
                             (window->releaseFenceFd >= 0 ? 1 : 0));
        if (window->releaseFenceFd >= 0) {
            DLOGD_IF(HWC2_DEBUGTYPE_FENCE," trigger release fence fenceFd [%d]",window->releaseFenceFd);
        }
        if (window->ionBufferSharedFd >= 0) {
            close(window->ionBufferSharedFd);
        }
    }

    mlastHWWindowInfo = inHWwindowinfo;

    return HWC2_ERROR_NONE;
}

std::string HWComposerOSD::dump() const {
    std::stringstream output;
    output << "OSD mDisplayQueue size " << mDisplayQueue.size() << "\n";
    for (int j=0; j<mDisplay.mGopCount; j++) {
        output << "window " << j << ": " << mlastHWWindowInfo[j]->toString() << "\n";
    }

    return output.str();
}

void HWComposerOSD::getMirrorMode(dictionary *pModelINI, int *mirrorModeH, int *mirrorModeV) {
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
                DLOGE("Mirror Type = %d. it is incorrect! repair to HV-mirror\n",osdMirrorType);
                *mirrorModeH = 1;
                *mirrorModeV = 1;
                break;
        }
    } else {
        *mirrorModeH = 0;
        *mirrorModeV = 0;
    }
}

int HWComposerOSD::openConfigFile(dictionary **ppSysINI, dictionary **ppModelINI, dictionary **ppPanelINI,
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
        DLOGE("Load %s failed!", HWCOMPOSER13_INI_FILENAME);
        return -1;
    }

    //load module.ini file
    pModuleName = iniparser_getstr(pModelINI, "module:m_pModuleName");
    pModuleINI = iniparser_load(pModuleName);
    if (pModuleINI == NULL) {
        tvini_free(pSysINI, pModelINI, pPanelINI);
        iniparser_freedict(pHwcINI);
        DLOGE("Load %s failed!", pModuleName);
        return -1;
    }

    //load dfbrc file
    pDfbrcName = iniparser_getstr(pModelINI, "dfbrc:m_pDFBrcName");
    if (pDfbrcName) {
        pDfbrcINI = iniparser_load(pDfbrcName);
        DLOGD("the dfbrcName name is %s",pDfbrcName);
    } else {
        pDfbrcINI = iniparser_load(DFBRC_INI_FILENAME);
    }
    if (pDfbrcINI == NULL) {
        DLOGE("Load DFBRC_INI_PATH_FILENAME fail");
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

void HWComposerOSD::closeConfigFile(dictionary *pSysINI, dictionary *pModelINI, dictionary *pPanelINI,
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

void HWComposerOSD::initXcGfx()
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

void HWComposerOSD::setStbResolutionState() {
    char property[PROPERTY_VALUE_MAX] = {0};
    char reslutionstate[PROPERTY_VALUE_MAX] = {0};

    if (property_get("mstar.resolution", property, NULL) > 0) {
        DLOGD("STB_setResolutionState resolution = %s\n",property);
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
                   (strcmp(property,"DACOUT_1080I_30") == 0)) {
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

void HWComposerOSD::initCmdqAndMload()
{
    const mmap_info_t* mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_MMAP_ID_CMDQ");
    if (mmapInfo != NULL) {
        DLOGD("miuno = %d, addr = %p,size = %lx",mmapInfo->miuno,mmapInfo->addr,mmapInfo->size);
        MS_BOOL ret = TRUE;
        unsigned int miuOffset = 0;
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
        DLOGD("there is no E_MMAP_ID_CMDQ in mmap.ini");
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
                DLOGD("the menuload is enabled %s :%d",__FUNCTION__,__LINE__);
            } else {
                DLOGD("the menuload is not enabled or not support  %s %d",__FUNCTION__,__LINE__);
            }
        }
    } else {
        DLOGD("no E_MMAP_ID_XC_MLOAD in mmap.ini %s :%d",__FUNCTION__,__LINE__);
    }
}

void HWComposerOSD::clearGfxFbBuffer()
{
    const mmap_info_t* mmapInfo = NULL;
    //clear GOP_FB use GE function
    unsigned int maskPhyAddr = 0;
    mmapInfo = NULL;
    mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
    if (mmapInfo != NULL) {
        if (mmapInfo->cmahid > 0 ) {
            DLOGI("E_MMAP_ID_GOP_FB was integrated into CMA");
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
