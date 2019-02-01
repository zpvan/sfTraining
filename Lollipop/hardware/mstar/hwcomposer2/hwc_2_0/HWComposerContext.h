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

#ifndef HWCOMPOSERCONTEXT_H
#define HWCOMPOSERCONTEXT_H

#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

#include "disp_api.h"
#include <apiGOP.h>
#include <apiXC.h>
#include <apiXC_DWIN.h>
#include <cutils/properties.h>
#include <utils/Timers.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <fcntl.h>
/* mstar header */
#include <gralloc_priv.h>
#include <iniparser.h>
#include <tvini.h>
#include <mmap.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <apiXC.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <drvGPIO.h>
#include <drvCMDQ.h>
#include <drvSYS.h>
#include <sw_sync.h>
#include <drvCMAPool.h>
#include <hardware/tv_input.h>
#include "sideband.h"
#include "debug.h"

#define UTILS_UNUSED __attribute__((unused))
#define MAX_OVERLAY_LAYER_NUM 2
#define MAX_SIDEBAND_LAYER_NUM 9
#define MAX_NUM_LAYERS 32
#define MAX_DEBUG_LAYERCOUNT 50
#define MAX_UIOVERLAY_COUNT 1
#define HWCURSOR_DSTFB_NUM 2
#define GWIN_ALIGNMENT 2
#define HWCURSOR_MAX_WIDTH 128
#define HWCURSOR_MAX_HEIGHT 128
#define BYTE_PER_PIXEL 4
#define GOP_TIMEOUT_CNT 100

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

#define INVALID_FBID    0xFF
#define MAX_ACTIVE_DEGREE (20)
#define MIN_ACTIVE_DEGREE (-20)
#define ALIGNMENT( value, base ) ((value) & ~((base) - 1))
#define DFBRC_INI_FILENAME          "/config/dfbrc.ini"
#define HWCOMPOSER13_INI_FILENAME   "/system/etc/hwcomposer13.ini"

//gflip ioctl
#define MDRV_GFLIP_IOC_MAGIC       ('2')
#define MDRV_GFLIP_IOC_GETVSYNC_NR 20
#define MDRV_GFLIP_IOC_GETVSYNC    _IOWR(MDRV_GFLIP_IOC_MAGIC,MDRV_GFLIP_IOC_GETVSYNC_NR,unsigned int)

typedef enum {
    VENDOR_CMA_CONTIGUOUS_MEMORY        = 0x00000001,
    VENDOR_CMA_DISCRETE_MEMORY          = 0x00000002,
} vendor_usage_bits_t;

template <typename T>
class CallbackInfo {
public:
    hwc2_callback_data_t data;
    T pointer;
};

class HWWindowInfo {
public:
    HWWindowInfo()
        :gop(0xFF),
        gop_dest(E_GOP_DST_OP0),
        bEnableGopAFBC(false),
        bSupportGopAFBC(false),
        bAFBCNeedReset(false),
        gwin(0xFF),
        channel(0),
        fbid(INVALID_FBID),
        subfbid(INVALID_FBID),
        physicalAddr(0),
        virtualAddr(0),
        mainFBphysicalAddr(0),
        subFBphysicalAddr(0),
        bufferWidth(0),
        bufferHeight(0),
        srcX(0),srcY(0),srcWidth(0),
        srcHeight(0),dstX(0),dstY(0),
        dstWidth(0),dstHeight(0),
        format(0),stride(0),
        blending(HWC2_BLEND_MODE_INVALID),
        planeAlpha(0),acquireFenceFd(-1),
        releaseFenceFd(-1),releaseSyncTimelineFd(-1),
        releaseFenceValue(-1),layerIndex(-1),bFrameBufferTarget(false),ionBufferSharedFd(-1)
        {}
    HWWindowInfo(const HWWindowInfo& value)
        :gop(value.gop),
        gop_dest(value.gop_dest),
        bEnableGopAFBC(value.bEnableGopAFBC),
        bSupportGopAFBC(value.bSupportGopAFBC),
        bAFBCNeedReset(value.bAFBCNeedReset),
        gwin(value.gwin),
        channel(value.channel),
        fbid(value.fbid),
        subfbid(value.subfbid),
        physicalAddr(value.physicalAddr),
        virtualAddr(value.virtualAddr),
        mainFBphysicalAddr(value.mainFBphysicalAddr),
        subFBphysicalAddr(value.subFBphysicalAddr),
        bufferWidth(value.bufferWidth),
        bufferHeight(value.bufferHeight),
        srcX(value.srcX),srcY(value.srcY),srcWidth(value.srcWidth),
        srcHeight(value.srcHeight),dstX(value.dstX),dstY(value.dstY),
        dstWidth(value.dstWidth),dstHeight(value.dstHeight),
        format(value.format),stride(value.stride),
        blending(value.blending),
        planeAlpha(value.planeAlpha),acquireFenceFd(value.acquireFenceFd),
        releaseFenceFd(value.releaseFenceFd),releaseSyncTimelineFd(value.releaseSyncTimelineFd),
        releaseFenceValue(value.releaseFenceValue),layerIndex(value.layerIndex),
        bFrameBufferTarget(value.bFrameBufferTarget),
        ionBufferSharedFd(value.ionBufferSharedFd)
        {}

    uint32_t gop;
    uint32_t gop_dest;
    bool bEnableGopAFBC;
    bool bSupportGopAFBC;
    bool bAFBCNeedReset;
    uint32_t gwin;
    int channel;    // miu
    uint32_t fbid; // if property changed, recreate fbid with new property
    uint32_t subfbid;
    uintptr_t physicalAddr;
    uintptr_t virtualAddr;
    uintptr_t mainFBphysicalAddr;
    uintptr_t subFBphysicalAddr;
    uint32_t bufferWidth;
    uint32_t bufferHeight;
    uint32_t srcX;
    uint32_t srcY;
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t dstX;
    uint32_t dstY;
    uint32_t dstWidth;
    uint32_t dstHeight;
    int32_t format;
    uint32_t stride;
    int32_t blending;
    int32_t planeAlpha;

    int32_t acquireFenceFd;
    int32_t releaseFenceFd; // TODO: release fence if set() failed anyway
    int32_t releaseSyncTimelineFd;
    int32_t releaseFenceValue;

    int32_t layerIndex; //index to primaryContent->hwLayers[layerIndex], if prepare() set layerIndex to -1, set() will disable this window
    bool bFrameBufferTarget;
    // the ionBufferSharedFd is the fd ,represent  the ionbuffer which will be shown by the GOP.
    // To avoid the ionbuffer modified by others during the GOP showing it,
    // we should hold on the  ionBufferSharedFd and close it after the ionbuffer's GOP show ended.
    int32_t ionBufferSharedFd;
    std::string toString() const {
        std::string output;

        const size_t BUFFER_SIZE = 500;
        char buffer[BUFFER_SIZE] = {};
        auto writtenBytes = snprintf(buffer, BUFFER_SIZE,
                "Hw window[%p] gop=%d gwin=%d miu=%d physicalAddr=%lx fbid=%x bufferW=%d bufferH=%d \
                format=%d stride=%d layerindex=%d bFrameBufferTarget=%d bEnableGopAFBC = %d \
                srcx=%d srcY=%d srcWidth=%d srcHeight=%d dstX=%d dstY=%d dstWidth=%d dstHeight=%d",
                this, gop, gwin, channel, (unsigned long)physicalAddr, fbid,
                bufferWidth, bufferHeight, format, stride, layerIndex,
                bFrameBufferTarget, bEnableGopAFBC,
                srcX,srcY, srcWidth,srcHeight,
                dstX,dstY, dstWidth,dstHeight);
        output.append(buffer, writtenBytes);
        return output;
    }
};

class HWCursorInfo {
public:
    HWCursorInfo()
        :layerIndex(-1),
        gop(0xFF),gop_dest(E_GOP_DST_OP0),
        gwin(0xFF),gwinPosAlignment(),
        fbid(INVALID_FBID),channel(0),
        bInitialized(false),positionX(0),
        positionY(0),srcAddr(0),
        srcX(0),srcY(0),srcWidth(0),srcHeight(0),
        srcStride(0),dstWidth(0),dstHeight(0),
        gop_srcWidth(0),gop_srcHeight(0),
        gop_dstWidth(0),gop_dstHeight(0),planeAlpha(0),
        releaseSyncTimelineFd(-1),releaseFenceFd(-1),
        acquireFenceFd(-1),operation(0),mHwCursorDstAddr(HWCURSOR_DSTFB_NUM)
    {}
    HWCursorInfo(const HWCursorInfo& value)
        :layerIndex(value.layerIndex),
        gop(value.gop),gop_dest(value.gop_dest),
        gwin(value.gwin),gwinPosAlignment(value.gwinPosAlignment),
        fbid(value.fbid),channel(value.channel),
        bInitialized(value.bInitialized),
        positionX(value.positionX),positionY(value.positionY),
        srcAddr(value.srcAddr),
        srcX(value.srcX),srcY(value.srcY),srcWidth(value.srcWidth),srcHeight(value.srcHeight),
        srcStride(value.srcStride),dstWidth(value.dstWidth),dstHeight(value.dstHeight),
        gop_srcWidth(value.gop_srcWidth),gop_srcHeight(value.gop_srcHeight),
        gop_dstWidth(value.gop_dstWidth),gop_dstHeight(value.gop_dstHeight),planeAlpha(value.planeAlpha),
        releaseSyncTimelineFd(value.releaseSyncTimelineFd),releaseFenceFd(value.releaseFenceFd),
        acquireFenceFd(value.acquireFenceFd),operation(value.operation),
        mHwCursorDstAddr(value.mHwCursorDstAddr)
    {}
    std::string toString() const {
        std::string output;

        const size_t BUFFER_SIZE = 300;
        char buffer[BUFFER_SIZE] = {};
        auto writtenBytes = snprintf(buffer, BUFFER_SIZE,
                "hwcursor[%p]: gop=%d gwin=%d miu=%d srcAddr=%llx fbid=%d stride=%d layerindex=%d \
                 positionX=%d positionY=%d opration=%x  \
                 srcX = %d,srcY = %d,srcWidth = %d,srcHeight = %d,dstWidth = %d,dstHeight  = %d \
                 strethwindow[%d,%d,%d,%d]\n",
                this,gop, gwin,
                channel, (long long unsigned int)srcAddr,
                fbid, 128*4, layerIndex,
                positionX,positionY,operation,
                srcX,srcY,srcWidth,srcHeight,dstWidth,dstHeight,
                gop_srcWidth,gop_srcHeight,
                gop_dstWidth,gop_dstHeight);
        output.append(buffer, writtenBytes);
        return output;
    }
    int layerIndex; //index to primaryContent->hwLayers[layerIndex], if prepare() set layerIndex to -1, set() will disable this window
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
    std::vector<uintptr_t> mHwCursorDstAddr;
};

class BootLogoObj {
public:
    unsigned int gop;
    int closeThreshold;  //by frame
    unsigned int frameCountAfterBoot;
    bool bGopClosed;
};

static const char *memType2String(unsigned int memType) {
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

static MS_U32 _SetFBFmt(MS_U16 pitch UTILS_UNUSED, MS_PHY addr UTILS_UNUSED, MS_U16 fmt UTILS_UNUSED) {
    // Set FB info to GE (For GOP callback fucntion use)
    return 1;
}

static MS_BOOL _XC_IsInterlace(void) {
    return 0;
}

static void _SetPQReduceBWForOSD(MS_U8 PqWin UTILS_UNUSED, MS_BOOL enable UTILS_UNUSED) {
}

static MS_U16 _XC_GetCapwinHStr(void) {
    return 0x80;
}

#endif // HWCOMPOSER_CONTEXT_H
