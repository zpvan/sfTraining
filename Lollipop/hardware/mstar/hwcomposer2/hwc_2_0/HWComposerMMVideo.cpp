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
#include "HWComposerMMVideo.h"
#include <sys/stat.h>
#define __CLASS__ "HWComposerMMVideo"

#define NOTIFY_TO_SET_DS defined(MSTAR_CLIPPERS) || defined(MSTAR_KANO) || defined(MSTAR_CURRY)

#if NOTIFY_TO_SET_DS
#include "VideoSetWrapper.h"
#endif


#define OVERLAY_TO_WININFO(Overlay, stWinInfo) \
    do { \
            memset(&stWinInfo, 0, sizeof(MS_WinInfo));  \
            (stWinInfo).disp_win.x = (Overlay).displayFrame.left; \
            (stWinInfo).disp_win.y = (Overlay).displayFrame.top;  \
            (stWinInfo).disp_win.width = (Overlay).displayFrame.right \
                                        - (Overlay).displayFrame.left;\
            (stWinInfo).disp_win.height = (Overlay).displayFrame.bottom   \
                                        - (Overlay).displayFrame.top; \
            if (((int)(Overlay).sourceCropf.right <= (Overlay).FbWidth)   \
                && ((int)(Overlay).sourceCropf.bottom <= (Overlay).FbHeight)) {   \
                (stWinInfo).crop_win.x = (((float)(Overlay).SrcWidth * 10 * \
                                            ((Overlay).sourceCropf.left / (float)(Overlay).FbWidth)) + 5) / 10;  \
                (stWinInfo).crop_win.y = (((float)(Overlay).SrcHeight * 10 * \
                                            ((Overlay).sourceCropf.top / (float)(Overlay).FbHeight)) + 5) / 10;   \
                (stWinInfo).crop_win.width = (((float)((Overlay).SrcWidth * 10 * \
                                              (((Overlay).sourceCropf.right  \
                                            - (Overlay).sourceCropf.left) / (float)(Overlay).FbWidth))) + 5) / 10; \
                (stWinInfo).crop_win.height = (((float)((Overlay).SrcHeight * 10 * \
                                              (((Overlay).sourceCropf.bottom  \
                                            - (Overlay).sourceCropf.top) / (float)(Overlay).FbHeight))) + 5) / 10; \
            } else {    \
                (stWinInfo).crop_win.x = (Overlay).sourceCropf.left;  \
                (stWinInfo).crop_win.y = (Overlay).sourceCropf.top;   \
                (stWinInfo).crop_win.width = (Overlay).sourceCropf.right  \
                                            - (Overlay).sourceCropf.left; \
                (stWinInfo).crop_win.height = (Overlay).sourceCropf.bottom\
                                            - (Overlay).sourceCropf.top;  \
            }   \
            (stWinInfo).osdWidth = (Overlay).osdRegion.right      \
                                    - (Overlay).osdRegion.left;   \
            (stWinInfo).osdHeight = (Overlay).osdRegion.bottom    \
                                    - (Overlay).osdRegion.top;    \
            (stWinInfo).transform = (Overlay).transform;  \
            (stWinInfo).zorder = (Overlay).zorder;    \
        } while(0)


static bool operator==(const hwc_rect_t& lhs, const hwc_rect_t& rhs) {
    return lhs.left == rhs.left &&
            lhs.top == rhs.top &&
            lhs.right == rhs.right &&
            lhs.bottom == rhs.bottom;
}

static bool operator==(const hwc_frect_t& lhs, const hwc_frect_t& rhs) {
    return lhs.left == rhs.left &&
            lhs.top == rhs.top &&
            lhs.right == rhs.right &&
            lhs.bottom == rhs.bottom;
}

template <typename T>
static inline bool operator!=(const T& lhs, const T& rhs)
{
    return !(lhs == rhs);
}

HWComposerMMVideo::HWComposerMMVideo(HWCDisplayDevice& display)
    :mDisplay(display)
{
    init();
    /* initialize the gralloc */
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&mGralloc);
}
HWComposerMMVideo::~HWComposerMMVideo() {
    for(int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        MS_Video_Disp_DeInit((MS_Win_ID)i);
    }
}

void HWComposerMMVideo::getHardwareRenderGeometry(int &width, int &height) {
    int minSize = sizeof(MS_DispFrameFormat);
    int rowBytes;

    width       = HARDWARE_RENDER_WIDTH;
    rowBytes    = width * 4;
    height      = (minSize + rowBytes - 1) / rowBytes;
}

void HWComposerMMVideo::get_panel_ratio(hwc_win *win, bool recheck) {

    static float Wratio = 1.0f, Hratio = 1.0f;
    bool check = false;

    if (check == false || recheck) {
        dictionary *sysINI = NULL, *modelINI = NULL, *panelINI = NULL;
        int osdWidth = 0, osdHeight = 0;
        int ret = 0;

        ret = tvini_init(&sysINI, &modelINI, &panelINI);
        if (ret == -1) {
            DLOGE("tvini_init: get ini fail!!!\n");
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
            timingToWidthHeight(stCurrentVideoTiming.eVideoTiming,&mDisplay.mPanelWidth,&mDisplay.mPanelHeight);
            MI_DISPLAY_Close(hDispHandle);
            MI_DISPLAY_DeInit();
            DLOGD("timing %d , width %d,height %d \n",stCurrentVideoTiming.eVideoTiming,mDisplay.mPanelWidth,mDisplay.mPanelHeight);
        } else {
            mDisplay.mPanelWidth = g_IPanel.Width();
            mDisplay.mPanelHeight = g_IPanel.Height();
        }
#else
#if defined(MSTAR_NAPOLI) || defined(MSTAR_MONACO) || defined(MSTAR_CLIPPERS)
        if (g_IPanel.Width()) {
            mDisplay.mPanelWidth = g_IPanel.Width();
            mDisplay.mPanelHeight = g_IPanel.Height();
        } else {
            mDisplay.mPanelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
            mDisplay.mPanelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
        }
    #else
        mDisplay.mPanelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
        mDisplay.mPanelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
    #endif
#endif

        Wratio = (float)mDisplay.mPanelWidth / osdWidth;
        Hratio = (float)mDisplay.mPanelHeight / osdHeight;
        DLOGD("panel Width %d, Height %d", mDisplay.mPanelWidth, mDisplay.mPanelHeight);
        DLOGD("osd Width %d, Height %d", osdWidth, osdHeight);
        check = true;
        tvini_free(panelINI, modelINI, sysINI);
    }

    DLOGD("@@@@PANEL/OSD W ratio : %f, H ratio : %f", Wratio, Hratio);
    if (check == true) {
        // apply to Width
        if (Wratio > 1.0f) {
            if (win->x < 0) {
                win->x = 0;
            } else {
                win->x *= Wratio;
            }

            win->width *= Wratio;
            if (win->width > mDisplay.mPanelWidth) {
                win->width = mDisplay.mPanelWidth;
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
            if (win->height > mDisplay.mPanelHeight) {
                win->height = mDisplay.mPanelHeight;
            }
        }
        DLOGD("@@@@ apply ratio, x=%d, y=%d, width=%d, height=%d",
              win->x, win->y, win->width, win->height);
    }
}

void HWComposerMMVideo::mmVideoSizeChangedNotify() {
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
                DLOGD("hwc_reSetPanelSize mstar.hwc.ds  value = %d \n",atoi(valueMW));
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

void HWComposerMMVideo::mmVideoCheckDownScaleXC() {
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

static void processSignal(int signo) {
    DLOGE("Signal is %d\n", signo);
}

void HWComposerMMVideo::create_pipe() {
    mode_t mode = 0777;

    unlink(FIFO_MM);
    if (mkfifo(FIFO_MM, mode) < 0)
        DLOGE("failed to make mm fifo : %s", strerror(errno));

    unlink(FIFO_TV);
    if (mkfifo(FIFO_TV, mode) < 0)
        DLOGE("failed to make tv fifo : %s", strerror(errno));

    signal(SIGPIPE, processSignal);
}

void HWComposerMMVideo::init() {
    create_pipe();

    for(int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        MS_Video_Disp_Init((MS_Win_ID)i);
    }
}
#ifdef SUPPORT_TUNNELED_PLAYBACK
int HWComposerMMVideo::mmSidebandPrepare(std::vector<std::shared_ptr<HWCLayer>>& layers) {
    bool bExist[MAX_SIDEBAND_LAYER_NUM];

    for (uint32_t i = 0; i < MAX_SIDEBAND_LAYER_NUM; i++) {
        bExist[i] = false;
    }

    for (auto& layer:layers) {

        if (layer->getLayerCompositionType() != HWC2_COMPOSITION_SIDEBAND)
            continue;

        sideband_handle_t *sidebandStream = (sideband_handle_t *)layer->getSideband();
        if (sidebandStream && (sidebandStream->sideband_type == TV_INPUT_SIDEBAND_TYPE_MM_XC || sidebandStream->sideband_type == TV_INPUT_SIDEBAND_TYPE_MM_GOP)) {
            MS_WinInfo stWinInfo;
            uint32_t idx, null_idx = MAX_SIDEBAND_LAYER_NUM;
            uint32_t cmd;
            bool changed = false;

            for (idx = 0; idx < MAX_SIDEBAND_LAYER_NUM; idx++) {
                if (mSideband[idx].handle == NULL && null_idx == MAX_SIDEBAND_LAYER_NUM)
                    null_idx = idx;
                if (mSideband[idx].handle == layer->getSideband()) {
                    bExist[idx] = true;
                    break;
                }
            }

            if (idx == MAX_SIDEBAND_LAYER_NUM) {
                if (null_idx == MAX_SIDEBAND_LAYER_NUM) {
                    DLOGE("SideBand Full: %d", MAX_SIDEBAND_LAYER_NUM);
                    continue;
                }
                idx = null_idx;
                memset(&mSideband[idx], 0, sizeof(HwcOverlayInfo));
            }

            if ((mSideband[idx].displayFrame != layer->getLayerDisplayFrame())
                || (mSideband[idx].sourceCropf != layer->getLayerSourceCrop())
                || (mSideband[idx].zorder != layer->getIndex())) {

                if (mSideband[idx].bInited) {
                    DLOGD("[%d][%p] MM SIDEBAND RESIZE src crop{%f %f %f %f}->{%f %f %f %f} dst{%d %d %d %d}->{%d %d %d %d} z{%d}->{%d}"
                          , idx, mSideband[idx].handle
                          , mSideband[idx].sourceCropf.left, mSideband[idx].sourceCropf.top, mSideband[idx].sourceCropf.right, mSideband[idx].sourceCropf.bottom
                          , layer->getLayerSourceCrop().left, layer->getLayerSourceCrop().top, layer->getLayerSourceCrop().right, layer->getLayerSourceCrop().bottom
                          , mSideband[idx].displayFrame.left, mSideband[idx].displayFrame.top, mSideband[idx].displayFrame.right, mSideband[idx].displayFrame.bottom
                          , layer->getLayerDisplayFrame().left, layer->getLayerDisplayFrame().top, layer->getLayerDisplayFrame().right, layer->getLayerDisplayFrame().bottom
                          , mSideband[idx].zorder, layer->getIndex());
                }
                changed = true;
            }

            mSideband[idx].handle = layer->getSideband();
            bExist[idx] = true;
            mSideband[idx].sourceCropf = layer->getLayerSourceCrop();
            mSideband[idx].displayFrame = layer->getLayerDisplayFrame();
            mSideband[idx].zorder = layer->getIndex();

            OVERLAY_TO_WININFO(mSideband[idx], stWinInfo);

            if (mSideband[idx].bInited == false) {
                cmd = E_SIDEBAND_COMMAND_INIT;
                write(sidebandStream->pipe_in[WRITE_END], &cmd, sizeof(cmd));
                write(sidebandStream->pipe_in[WRITE_END], &stWinInfo, sizeof(stWinInfo));
                mSideband[idx].bInited = true;
                DLOGD("[%d][%p] MM SIDEBAND INIT crop{%f %f %f %f} dst{%d %d %d %d}"
                    , idx, mSideband[idx].handle
                    , layer->getLayerSourceCrop().left, layer->getLayerSourceCrop().top, layer->getLayerSourceCrop().right, layer->getLayerSourceCrop().bottom
                    , layer->getLayerDisplayFrame().left, layer->getLayerDisplayFrame().top, layer->getLayerDisplayFrame().right, layer->getLayerDisplayFrame().bottom);
            } else if (changed) {
                cmd = E_SIDEBAND_COMMAND_RESIZE;
                write(sidebandStream->pipe_in[WRITE_END], &cmd, sizeof(cmd));
                write(sidebandStream->pipe_in[WRITE_END], &stWinInfo, sizeof(stWinInfo));
            }
        }
    }

    for (uint32_t i = 0; i < MAX_SIDEBAND_LAYER_NUM; i++) {
        if (mSideband[i].handle && bExist[i] == false) {
            uint32_t cmd;
            MS_WinInfo stWinInfo = {0,};
            sideband_handle_t *sidebandStream = (sideband_handle_t *)mSideband[i].handle;
            DLOGD("[%d][%p] MM SIDEBAND REMOVE", i, mSideband[i].handle);
            cmd = E_SIDEBAND_COMMAND_DEINIT;
            write(sidebandStream->pipe_in[WRITE_END], &cmd, sizeof(cmd));
            write(sidebandStream->pipe_in[WRITE_END], &stWinInfo, sizeof(stWinInfo));
            mSideband[i].handle = NULL;
        }
    }

    return 0;
}
#endif

void HWComposerMMVideo::setOsdRegion(hwc_rect_t rect) {
    mOsdRegion = rect;
}

hwc2_error_t HWComposerMMVideo::present(std::vector<std::shared_ptr<HWCLayer>>& layers, int32_t* outRetireFence) {

    // configure the MM TV OVERLAY
    bool isAnyMMOverlay = false;
    bool isAnyTVOverlay = false;

    int prev_overlay_num = mOverlayNum;
    mOverlayNum = 0;
    HwcOverlayInfo prev_overlay[MAX_OVERLAY_LAYER_NUM];

#ifdef SUPPORT_TUNNELED_PLAYBACK
    mmSidebandPrepare(layers);
#endif

    {
        std::unique_lock <std::mutex> lock(mMutex);
        memcpy(&prev_overlay, &mOverlay, sizeof(prev_overlay));
        memset(&mOverlay, 0, sizeof(prev_overlay));

        for (auto layer:layers) {

            DLOGD_IF(HWC2_DEBUGTYPE_SHOW_VIDEO_LAYERS,"Video layer :%s",layer->dump().c_str());
            if (layer->getIndex() >= 0 ) {
                private_handle_t *hnd = (private_handle_t *)layer->getLayerBuffer().getBuffer();
                if (hnd == nullptr) {
                    return HWC2_ERROR_NOT_VALIDATED;
                }

                //dump_layer(k, layer);
#ifdef SUPPORT_TUNNELED_PLAYBACK
                if (layer->getLayerCompositionType() == HWC2_COMPOSITION_SIDEBAND) {
                    continue;
                }
#endif

                int MM_TV_NW_OVERLAY_MASK = private_handle_t::PRIV_FLAGS_MM_OVERLAY|private_handle_t::PRIV_FLAGS_TV_OVERLAY|private_handle_t::PRIV_FLAGS_NW_OVERLAY;
                if (hnd && (hnd->mstaroverlay&MM_TV_NW_OVERLAY_MASK)) {
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

                        mGralloc->lock(mGralloc, (buffer_handle_t)hnd, (GRALLOC_USAGE_SW_READ_MASK), 0, 0, 0, 0, &vaddr);
                        if (vaddr != NULL) {
                            MS_DispFrameFormat *dff = (MS_DispFrameFormat*)vaddr;

                            if (dff->OverlayID >= (MAX_OVERLAY_LAYER_NUM)) {
                                DLOGE("hwc overlay number exceed limit");
                            } else {
                                mOverlay[dff->OverlayID].bInUsed = true;
                                if (!prev_overlay[dff->OverlayID].bInUsed && dff->u8FreezeThisFrame) {
                                    // invalid, do not init overlay
                                    DLOGE("DO NOT init overlay");
                                    mOverlay[dff->OverlayID].bInUsed = false;
                                }
                                mOverlay[dff->OverlayID].sourceCropf = layer->getLayerSourceCrop();
                                mOverlay[dff->OverlayID].displayFrame = layer->getLayerDisplayFrame();
                                memcpy(&mOverlay[dff->OverlayID].stdff, dff, sizeof(MS_DispFrameFormat));
                                getHardwareRenderGeometry(mOverlay[dff->OverlayID].FbWidth, mOverlay[dff->OverlayID].FbHeight);

                                mOverlay[dff->OverlayID].SrcWidth = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
                                mOverlay[dff->OverlayID].SrcHeight = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop
                                                                    - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;

                                mOverlayNum++;
                            }
                        } else {
                            DLOGE("[%d] vaddr == NULL", __LINE__);
                        }
                        mGralloc->unlock(mGralloc, (buffer_handle_t)hnd);
                    } else {
                        if ((bSendMMDispInfo == false || bSendTVDispInfo == false) &&
                            layer->getLayerDisplayFrame().left == mTVdisplayFrame.left &&
                            layer->getLayerDisplayFrame().top == mTVdisplayFrame.top &&
                            layer->getLayerDisplayFrame().right == mTVdisplayFrame.right &&
                            layer->getLayerDisplayFrame().bottom == mTVdisplayFrame.bottom) {
                            //DLOGE("same display info as before");
                            continue;
                        }

                        // save previous frame
                        mTVdisplayFrame.left = layer->getLayerDisplayFrame().left;
                        mTVdisplayFrame.top = layer->getLayerDisplayFrame().top;
                        mTVdisplayFrame.right = layer->getLayerDisplayFrame().right;
                        mTVdisplayFrame.bottom = layer->getLayerDisplayFrame().bottom;

                        win.x = layer->getLayerDisplayFrame().left;
                        win.y = layer->getLayerDisplayFrame().top;
                        win.width = layer->getLayerDisplayFrame().right - layer->getLayerDisplayFrame().left;
                        win.height = layer->getLayerDisplayFrame().bottom - layer->getLayerDisplayFrame().top;
                        win.osdRegion = mOsdRegion;
                        DLOGD("@@@@x=%d, y=%d, width=%d, height=%d", win.x, win.y, win.width, win.height);
                        get_panel_ratio(&win, false);

                        if (hnd->mstaroverlay == private_handle_t::PRIV_FLAGS_TV_OVERLAY) {
                            // TODO in the future
                        }
                        fd = open(fifo, O_RDWR | O_NONBLOCK);
                        if (fd < 0) {
                            DLOGE("failed to open fifo(%s), err=%s", fifo, strerror(errno));
                        } else {
                            //clear fifo
                            hwc_win tmpwin;
                            while (read(fd, (char*)&tmpwin, sizeof(hwc_win)) > 0);

                            if (write(fd, (char*)&win, sizeof(hwc_win)) < 0) {
                                DLOGE("failed to write data : %s", strerror(errno));
                            }
                            close(fd);
                        }

                        fp = fopen(file, "w+");
                        if (!fp) {
                            DLOGE("fail to open file(%s), err=%s", file, strerror(errno));
                            continue;
                        }

                        fwrite(&win, sizeof(hwc_win), 1, fp);
                        fclose(fp);
                    }
                }
            }
        }
    }

    // Overlay Change, remove overlay first
    //DLOGD("Overlay Change, prev_overlay_num = %d, ctx->overlay_num = %d", prev_overlay_num, ctx->overlay_num);
    for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
        if (prev_overlay[i].bInUsed != mOverlay[i].bInUsed) {

            DLOGD("Overlay[%d], bInUsed = %d, bIsForceClose = %d", i, mOverlay[i].bInUsed, bIsForceClose[i]);
            if (!mOverlay[i].bInUsed) {     // remove overlay
                MS_Video_Disp_Close((MS_Win_ID)i);
                u8SizeChanged[i] = 0;
            } else {                            // init overlay
                MS_WinInfo stWinInfo;

                OVERLAY_TO_WININFO(mOverlay[i], stWinInfo);
                MS_Video_Disp_Open((MS_Win_ID)i, stWinInfo, &mOverlay[i].stdff);
            }
        }
    }

    if (mOverlayNum) {

        for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
            if (mOverlay[i].bInUsed && prev_overlay[i].bInUsed) {
                if (memcmp(&mOverlay[i].displayFrame, &prev_overlay[i].displayFrame, sizeof(hwc_rect_t))
                    || memcmp(&mOverlay[i].sourceCropf, &prev_overlay[i].sourceCropf, sizeof(hwc_frect_t))) {

                    DLOGE("Overlay %d size changeed src {%d %d} -> {%d, %d} crop{%f %f %f %f}->{%f %f %f %f} dst{%d %d %d %d}->{%d %d %d %d}"
                          , i, prev_overlay[i].SrcWidth, prev_overlay[i].SrcHeight, mOverlay[i].SrcWidth, mOverlay[i].SrcHeight
                          , prev_overlay[i].sourceCropf.left, prev_overlay[i].sourceCropf.top, prev_overlay[i].sourceCropf.right, prev_overlay[i].sourceCropf.bottom
                          , mOverlay[i].sourceCropf.left, mOverlay[i].sourceCropf.top, mOverlay[i].sourceCropf.right, mOverlay[i].sourceCropf.bottom
                          , prev_overlay[i].displayFrame.left, prev_overlay[i].displayFrame.top, prev_overlay[i].displayFrame.right, prev_overlay[i].displayFrame.bottom
                          , mOverlay[i].displayFrame.left, mOverlay[i].displayFrame.top, mOverlay[i].displayFrame.right, mOverlay[i].displayFrame.bottom);

#if HWC_DS_APPLY_SIZE_CHANGE_IMMEDIATELY
                    if (Wrapper_Video_DS_State(i) || (MS_Video_Disp_Get_State((MS_Win_ID)i) < MS_STATE_OPENING))
#else
                    if (MS_Video_Disp_Get_State((MS_Win_ID)i) < MS_STATE_OPENING)
#endif
                    {
                        MS_WinInfo stWinInfo;

                        OVERLAY_TO_WININFO(mOverlay[i], stWinInfo);
                        MS_Video_Disp_Resize((MS_Win_ID)i, stWinInfo, &mOverlay[i].stdff);
                        u8SizeChanged[i] = 0;
                    } else {
                        u8SizeChanged[i] = SIZE_CHANGE_THRESHOLD_MAX;  // prevent continuous size change makes screen blink
                    }
                }
            }
        }

        for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
            if (u8SizeChanged[i] && mOverlay[i].bInUsed && !bIsForceClose[i]
                && !memcmp(&mOverlay[i].displayFrame, &prev_overlay[i].displayFrame, sizeof(hwc_rect_t))) {

                if (u8SizeChanged[i] == SIZE_CHANGE_THRESHOLD_MIN) {
                    DLOGD("Overlay [%d] do size change!", i);

                    MS_WinInfo stWinInfo;

                    OVERLAY_TO_WININFO(mOverlay[i], stWinInfo);
                    MS_Video_Disp_Resize((MS_Win_ID)i, stWinInfo, &mOverlay[i].stdff);

                    u8SizeChanged[i] = 0;
                } else {
                    u8SizeChanged[i]--;
                }
            }
        }
#ifndef RENDER_IN_QUEUE_BUFFER
        for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
            if (mOverlay[i].bInUsed && (prev_overlay[i].layer.handle != (mOverlay[i].layer.handle)) {
                MS_Video_Disp_Flip(MS_Win_ID(mOverlay[i].stdff.OverlayID), &mOverlay[i].stdff);
            }
        }
#endif
    }

    bSendMMDispInfo = !isAnyMMOverlay;
    bSendTVDispInfo = !isAnyTVOverlay;
    if (isAnyMMOverlay == false)
        remove(FILE_MM);
    if (isAnyTVOverlay == false)
        remove(FILE_TV);
    return HWC2_ERROR_NONE;
}
