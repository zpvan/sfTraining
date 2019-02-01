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

#ifndef HWCOMPOSER_CONTEXT_H
#define HWCOMPOSER_CONTEXT_H

#include <hardware/hwcomposer.h>
#include <pthread.h>
//#include "MsTypes.h"
#include "disp_api.h"
#include <apiGOP.h>
#include <apiXC.h>
#include <apiXC_DWIN.h>

#define MAX_HWWINDOW_COUNT 2
#define MAX_OVERLAY_LAYER_NUM MS_MAX_WINDOW
#define MAX_SIDEBAND_LAYER_NUM 9
#define MAX_NUM_LAYERS 32
#define MAX_DEBUG_LAYERCOUNT 50
#define MAX_UIOVERLAY_COUNT 1
#define GAMECURSOR_COUNT 2

#define DISPLAYMODE_NORMAL                          0
#define DISPLAYMODE_LEFTRIGHT                       1
#define DISPLAYMODE_TOPBOTTOM                       2
#define DISPLAYMODE_TOPBOTTOM_LA                    3
#define DISPLAYMODE_NORMAL_LA                       4
#define DISPLAYMODE_TOP_LA                          5
#define DISPLAYMODE_BOTTOM_LA                       6
#define DISPLAYMODE_LEFT_ONLY                       7
#define DISPLAYMODE_RIGHT_ONLY                      8
#define DISPLAYMODE_TOP_ONLY                        9
#define DISPLAYMODE_BOTTOM_ONLY                     10
#define DISPLAYMODE_LEFTRIGHT_FR                    11 //Duplicate Left to right and frame sequence output
#define DISPLAYMODE_NORMAL_FR                       12 //Normal frame sequence output
#define DISPLAYMODE_NORMAL_FP                       13
#define DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY          14
#define DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY       15
#define DISPLAYMODE_NORMAL_LA_HIGH_QUALITY          16
#define DISPLAYMODE_TOP_LA_HIGH_QUALITY             17
#define DISPLAYMODE_BOTTOM_LA_HIGH_QUALITY          18
#define DISPLAYMODE_LEFTRIGHT_FULL                  19
#define DISPLAYMODE_FRAME_ALTERNATIVE_LLRR          20
#define DISPLAYMODE_MAX                             21

#define MSTAR_DESK_DISPLAY_MODE                      "mstar.desk-display-mode"

#define HWCURSOR_ICON 0x01
#define HWCURSOR_POSITION 0x02
#define HWCURSOR_ALPHA 0x04
#define HWCURSOR_SHOW 0x08
#define HWCURSOR_HIDE 0x10

#define RES_4K2K_WIDTH 3840
#define RES_4K2K_HEIGHT 2160
enum {
    VIDEO_MAIN_WINDOW = 0,
    VIDEO_SUB_WINDOW = 1,
    GRAPHIC_HW_WINDOW0 = 2,
    GRAPHIC_HW_WINDOW1 = 3,
    GRAPHIC_HW_WINDOW2 = 4,
};

struct LayerIndexRange {
    int minLayerIndex;
    int maxLayerIndex;
};

struct HWWindowInfo_t {
    unsigned int gop;
    unsigned int gop_dest;
    bool bEnableGopAFBC;
    bool bSupportGopAFBC;
    bool bAFBCNeedReset;
    unsigned int gwin;
    int channel;    // miu
    unsigned int fbid; // if property changed, recreate fbid with new property
    unsigned int subfbid;
    MS_PHY physicalAddr;
    MS_PHY mainFBphysicalAddr;
    MS_PHY subFBphysicalAddr;
    unsigned int bufferWidth;
    unsigned int bufferHeight;
    unsigned int srcX;
    unsigned int srcY;
    unsigned int srcWidth;
    unsigned int srcHeight;
    unsigned int dstX;
    unsigned int dstY;
    unsigned int dstWidth;
    unsigned int dstHeight;
    int format;
    unsigned int stride;
    int blending;
    int planeAlpha;

    int acquireFenceFd;
    int releaseFenceFd; // TODO: release fence if set() failed anyway
    int releaseSyncTimelineFd;
    int releaseFenceValue;

    int layerIndex; //index to primaryContent->hwLayers[layerIndex], if prepare() set layerIndex to -1, set() will disable this window
    bool bFrameBufferTarget;
    LayerIndexRange layerRangeInSF;
    // the ionBufferSharedFd is the fd ,represent  the ionbuffer which will be shown by the GOP.
    // To avoid the ionbuffer modified by others during the GOP showing it,
    // we should hold on the  ionBufferSharedFd and close it after the ionbuffer's GOP show ended.
    int ionBufferSharedFd;
};

struct hwc_overlay_info_t {
    hwc_layer_1_t layer;
    int SrcWidth;
    int SrcHeight;
    bool bInitMVOP;
    bool bInUsed;
    MS_DispFrameFormat stdff;
    int FbWidth;
    int FbHeight;
    const void *handle;
    bool bInited;
    int zorder;
};

struct LayerCache {
    size_t layerCount;
    int GopOverlayCount;
    int fbLayerCount;
    int fbZ;
    int cursorLayerCount;
    buffer_handle_t hnd[MAX_NUM_LAYERS];
    hwc_rect_t sourceCrop[MAX_NUM_LAYERS];
    hwc_rect_t displayFrame[MAX_NUM_LAYERS];
    int32_t compositionType[MAX_NUM_LAYERS];
    uint8_t planeAlpha[MAX_NUM_LAYERS];
    int32_t blending[MAX_NUM_LAYERS];
    int32_t activeDegree[MAX_NUM_LAYERS];
    int32_t VendorUsageBits[MAX_NUM_LAYERS];
    bool    bCursorLayer[MAX_NUM_LAYERS];
};

struct BootLogoObj {
    unsigned int gop;
    int closeThreshold;  //by frame
    unsigned int frameCountAfterBoot;
    bool bGopClosed;
};

enum  LayerType {
    NORMAL_LAYER = 0,
    MM_OVERLAY_LAYER = 1,
    TV_OVERLAY_LAYER = 2,
    NW_OVERLAY_LAYER = 3,
    GOP_OVERLAY_LAYER = 4,
    FB_TARGET_LAYER =5,
    HW_CURSOR_LAYER =6,
    SIDEBAND_LAYER =7,
};

// layer attribute for dump SurfaceFlinger
struct layer_attr {
   int layerType;
   unsigned int memType;
   int sidebandType;
};

struct HwCursorInfo_t {
    int layerIndex; //index to primaryContent->hwLayers[layerIndex], if prepare() set layerIndex to -1, set() will disable this window
    int compositionType;
    unsigned int gop;
    unsigned int gop_dest;
    unsigned int gwin;
    unsigned int gwinPosAlignment;
    unsigned int fbid;
    int channel;    // miu
    bool bInitialized;
    //position factor
    int positionX;
    int positionY;
    //stretch blit factor
    uintptr_t srcAddr;
    int srcX;
    int srcY;
    int srcWidth;
    int srcHeight;
    int srcStride;
    int dstWidth;
    int dstHeight;
    //gop stretch setting
    int gop_srcWidth;
    int gop_srcHeight;
    int gop_dstWidth;
    int gop_dstHeight;
    int planeAlpha;
    int releaseSyncTimelineFd;
    int releaseFenceFd;
    int acquireFenceFd;
    unsigned char operation;
    int gameCursorIndex;
    uintptr_t pCtx;
};

struct hwc_context_t {
    hwc_composer_device_1_t device;
    /* our private state goes below here */

    // primary display attributes
    int32_t vsync_period;
    int32_t width;
    int32_t height;
    int32_t xdpi;
    int32_t ydpi;
    // mstar attributes
    bool vsync_off;
    int panelWidth;
    int panelHeight;
    int panelHStart;
    int ursaVersion;
    int panelLinkType;
    int panelLinkExtType;
    int osdc_enable;
    int panelType;
    int mirrorModeH;
    int mirrorModeV;
    // sometimes osdWidht/osdHeight is not the same as width height
    // osdWidth/osdHeight is report to the android system when
    // SurfaceFlinger.getDisplayInfo is called
    // width height means the size of frambuffer Surface used by
    // the eglwindow
    int osdWidth;
    int osdHeight;
    // sometimes only show part of the frambuffer,then
    // need the displayRegionWidth/displayRegionHeight
    // describe the region
    int displayRegionWidth;
    int displayRegionHeight;

    pthread_t               vsync_thread;
    const hwc_procs_t       *procs;

    HWWindowInfo_t HWWindowInfo[MAX_HWWINDOW_COUNT];
    HWWindowInfo_t lastHWWindowInfo[MAX_HWWINDOW_COUNT];

    bool stbTarget;
    // 1.if stbTarget is true, set bRGBColorSpace as false;
	//    if stbTarget is false, set bRGBColorSpace as true.
	// 2.if the property mstar.RGBColorSpace is set, set the
	//    bRGBColorSpace as mstar.ColorSpace.bRGB
	bool bRGBColorSpace;

// start of video things
    gralloc_module_t *gralloc;
    bool bSendMMDispInfo;
    bool bSendTVDispInfo;
    int overlay_num;
    hwc_overlay_info_t overlay[MAX_OVERLAY_LAYER_NUM];
    unsigned char u8SizeChanged[MAX_OVERLAY_LAYER_NUM];
    bool bIsForceClose[MAX_OVERLAY_LAYER_NUM];
    hwc_rect_t TVdisplayFrame;
    hwc_overlay_info_t sideband[MAX_SIDEBAND_LAYER_NUM];
// end of video things

    /* some case ,the timing changed,we need to
       record the current timing
    */
    int curTimingWidth;
    int curTimingHeight;
    /* the displayFrameFTScaleRatio_w =  curTimingWidth/FramebufferTarget.displayFrame.width
       the displayFrameFTHeightRatio_h = curTimingHeight/FramebufferTarget.displayFrame.height
    */
    float displayFrameFTScaleRatio_w;
    float displayFrameFTScaleRatio_h;
    int requiredGopOverlycount;
    int lastLayerCounts;
    int curLayerCounts;
    unsigned long frameCount;
    LayerCache lastFrameLayersInfo;
    int current3DMode;
    bool b3DModeChanged;
    unsigned int memAvailable;
    BootLogoObj bootlogo;
    bool bGopHasInit[MAX_HWWINDOW_COUNT];
    GOP_LayerConfig stLayerSetting;
    // gop Count which will be used by HWC
    int gopCount;
    uintptr_t captureScreenAddr;
    uintptr_t captureScreenLen;
    SCALER_DIP_WIN enDIPWin;
    bool bAFBCFramebuffer;
    bool bAFBCNeedReset;
    bool bFramebufferNeedRedraw;
    //debug information
    unsigned long HwcomposerDebugType;
    layer_attr debug_LayerAttr[MAX_DEBUG_LAYERCOUNT];
    unsigned int debug_LayerCount;


    //hwcursor
    HwCursorInfo_t hwCusorInfo[GAMECURSOR_COUNT];
    HwCursorInfo_t lastHwCusorInfo[GAMECURSOR_COUNT];
    bool hwcursor_fbNeedRedraw;

    //gamecurcor
    bool gameCursorEnabled;
    bool gameCursorChanged; //gamecursor switch to hwcursor, or hwcursor switch to gamecursor

    //cursor buffer addr, this can be used by hwcursor and gamecursor
    uintptr_t CursorPhyAddr[GAMECURSOR_COUNT];
    uintptr_t CursorVaddr[GAMECURSOR_COUNT];
};
#endif // HWCOMPOSER_CONTEXT_H
