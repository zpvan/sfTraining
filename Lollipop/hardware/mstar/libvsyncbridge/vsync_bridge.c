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

#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <mmap.h>
#include <UFO.h>
#include <MsTypesEx.h>
#include <MsCommon.h>
#include <MsIRQ.h>
#include <MsOS.h>
#include <drvMMIO.h>
#include <drvSEM.h>
#include <drvMIU.h>
#include <drvSYS.h>
#include <apiVDEC_EX.h>
#include <drvMVOP.h>
#include <drvXC_IOPort.h>
#include <apiPNL.h>
#include <drvGPIO.h>
#include <drvMMIO.h>
#include <drvSERFLASH.h>
#include <apiXC.h>
#include <stdio.h>

#include "MsFrmFormat.h"
#include "vsync_bridge.h"
#include <hardware/hwcomposer.h>
#include "dynamic_scale.h"
#include "VideoSetWrapper.h"
#include "dip.h"

#include "detile_yuv.h"
#if defined(BUILD_MSTARTV_MI)
#include "mi_display.h"
static void timingToWidthHeight(MI_DISPLAY_OutputTiming_e eOutputTiming, int *width, int *height);
#endif

#define NUM_MIU 3
#define OPEN_LOG_BY_PROPERTY        1

#if SUPPORT_FRAME_CONVERT_CHIP
#define ENABLE_FRAME_CONVERT        1
#else
#define ENABLE_FRAME_CONVERT        0
#endif
#define DISPQ_FIRST_R_INDEX         1
#define HANDLE_CROP_FIRST           0
#define TIME_DISPLAY_INIT_REF   800 // display initial reference duration time: 800 ms
#define WAIT_BOBMODE_FRAME      8 // disable bob mode after waiting for 8 frames. This is magic number, xc has no comment to how to calculate.
#define WAIT_FRAMECNT_UNMUTE    3   // wait for 3 Frames to un-mute automatically in case the APK do not init overlay and remove overlay
#define SUPPORT_HW_DISPLAY_MAX  2
#define CHECK_STREAM_ID(x) (x < SUPPORT_HW_DISPLAY_MAX) ? 1 : 0

// FIXME: remove these after utopia public header release.
MS_BOOL             __attribute__((weak))   MApi_XC_Is_SupportSWDS();
MS_U8               __attribute__((weak))   MApi_XC_GetSWDSIndex();
E_APIXC_ReturnValue __attribute__((weak))   MApi_XC_Get_VirtualBox_Info();
MS_BOOL    __attribute__((weak))   MApi_XC_Get_FD_Mask_byWin(SCALER_WIN eWindow);
void    __attribute__((weak))   MApi_XC_set_FD_Mask_byWin(MS_BOOL bEnable, SCALER_WIN eWindow);

#define IS_XC_SUPPORT_SWDS  (MApi_XC_Is_SupportSWDS && MApi_XC_Is_SupportSWDS())
#define IS_XC_FDMASK_API_READY    (MApi_XC_Get_FD_Mask_byWin && MApi_XC_set_FD_Mask_byWin)

#if OPEN_LOG_BY_PROPERTY
static MS_U32 bMsLogON = FALSE;
#endif

static MS_PHY u32SHMAddr[2] = {0, 0};
static MS_PHY u32FrameBufAddr = 0;
static MS_PHY u32SubFrameBufAddr = 0;
static MS_PHY u32DSAddr[2] = {0, 0};
static MS_U32 u32DSSize[2] = {0, 0};
static MS_U8  u8MaxDSIdx[2] = {0, 0};
static MS_U8  u8SaveBandwidthMode[2] = {0, 0};
static MS_PHY u32MiuStart[NUM_MIU + 1];
static MS_U8  u8Freeze[2];
static MS_U8  u8Rolling[2] = {0, 0};
#if ENABLE_FRAME_CONVERT
static MS_U8  u8FrcMode[2] = {E_FRC_NORMAL, E_FRC_NORMAL};
static MS_S8  s8CvtValue[2];
static MS_U8  u8CvtCount[2];
static MS_U8  u8Interlace[2];
static MS_U8  u8FrcDropMode[2];
static MS_U8  u8UpdateFrcMode[2];
#endif
static int panelWidth[2] = {1920, 1920};
static int panelHeight[2] = {1080, 1080};
#if BUILD_MSTARTV_MI
static int prevPanelWidth[2] = {1920, 1920};
static int prevPanelHeight[2] = {1080, 1080};
#endif
static MS_U32 u32CropLeft[2];
static MS_U32 u32CropRight[2];
static MS_U32 u32CropTop[2];
static MS_U32 u32CropBottom[2];
static int u32OsdWidth = 1920, u32OsdHeight= 1080;
static MS_HDRInfo stHDRInfo[2];
static MS_BOOL bIsMVOPSWCrop = FALSE;
static MS_U8  u8MirrorMode = 0;
static MS_WINDOW CurrentCropWin[2];
static MS_FreezeMode stFreezeMode[2] = {0, 0};
static MS_BOOL bCapImmediate[2] = {0, 0};
static MS_BufSlotInfo sBufSlotInfo[2];
static MS_U16 u16FrameCount[2] = {0};
static MS_U8 u8FDMaskEnable[2] = {0};
static MS_U8  u8BobModeCnt[2]= {0, 0};
static MS_U8 u8BlankVideoMute[2]= {0, 0};
static MS_BOOL bIsMVOPAutoGenBlack = FALSE;

#if SUPPORT_10BIT_CHIP
#define SUPPORT_10BIT
#endif

#ifdef SUPPORT_10BIT
static MS_U8 u8Is10Bit[2];
#endif

//#define VSYNC_BRIDGE_DEBUG
#ifdef VSYNC_BRIDGE_DEBUG
    #define VSYNC_BRIDGE_PRINT  ALOGD
#else
    #define VSYNC_BRIDGE_PRINT
#endif

static pthread_mutex_t sLock[2] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
#define VSYNC_BRIDGE_LOCK(id)  {    \
    VSYNC_BRIDGE_PRINT("[%d]Entar %s[%d]", id, __FUNCTION__, __LINE__);    \
    pthread_mutex_lock(&sLock[id]); }
#define VSYNC_BRIDGE_UNLOCK(id)  {    \
    VSYNC_BRIDGE_PRINT("[%d]Leave %s[%d]", id, __FUNCTION__, __LINE__);    \
    pthread_mutex_unlock(&sLock[id]);}

#define UPDATE_SHM_ADDR {   \
        if (dff->u32SHMAddr) {  \
            u32SHMAddr[0] =  u32SHMAddr[1] = dff->u32SHMAddr;   \
        }   \
    }


typedef struct
{
    union
    {
        struct
        {
            MS_U8 u4DSIndex0 : 4;
            MS_U8 u4DSIndex1 : 4;
        };
        MS_U8 u8DSIndex;
    };
}DS_Index;

static MS_U8  u8SizeChange[2];
static MS_U8  u8VsyncBridgeEnable[2];
static MS_U8  u8VsyncBridgeFirstFire[2];
static MS_U8  u8Captured[2];
static ds_internal_data_structure ds_int_data[2];
static DS_Index ds_curr_index[2];
static MS_U8  ds_next_index[2];
static MS_WINDOW OutputWin[2];
static MS_WINDOW_RATIO SrcCropWinRatio[2];
static MS_U16 u16SrcWidth[2];
static MS_U16 u16SrcHeight[2];
static stVBDispCrop ExternalWin[2];
static MS_U16 u16FrameRate[2];
static MS_U32 CodecType[2];
static MS_U32 u32RollingTime[2];
static MS_U8  u8SeamlessDSEnable[2];
static MS_U8  u8Interlace[2];
static MS_U8  u8MCUMode[2];
static MS_U8  u8FieldCtrl[2];
static MS_U8  u8Abort[2];

#define ABORT_WAIT_INIT_DONE      (1 << 0)
#define ABORT_WAIT_DISP_SHOW      (1 << 1)
#define ABORT_WAIT_DISP_CAPTURE      (1 << 2)

#define UPDATE_VIDEO_INFO        1

static Vsync_Bridge_ARC_Type eARC_Type = E_AR_DEFAULT;
static MS_DispFrameFormat lastdff[2];
static void *DipHandle[2] = {NULL,};

static MS_U32 u32FrameCount = 0;
static MS_BOOL bInterlaceOut = FALSE;

static void *callback_handle[2];
static FrameDoneCallBack frame_done_callback[2] = {NULL,};
static pthread_t monitor_thread[2];

#define FRAME_DURATION(x) (1000000 / ((x) ? (x) : 30000))

MS_U32 vsync_bridge_GetSHMIndex(MS_DispFrameFormat *dff)
{
    if (dff == NULL) {
        ALOGD("MS_DispFrameFormat NULL");
        return 0;
    }

    /************************************************************
    2017.0601
    Display queue sharememory index as the same as stream id now.
    It is mean we need to layout as the same count of sharememory area
    for display queue just like vdec.
    But if vdec can decode 16 bitstreams and mstar just only 2 mvop.
    Why we need to layout 16 area of display queue sharememory ?

    Even if 2 or 3 bitstream probably to use the same mvop.  We can just
    layout 2 or 3 more display queue area is enough for special case. In fact,
    the case wont be offen.

    It will more cases be going N-Way display and few streams
    use mvop if running the 16 streams playing at the same time.
    eg. 1st~15th are going N-way and 16th going mvop display. Why we need to
    layout 16 area display queue for just 1 mvop & 1 stream used ? Because it's
    coding easy ? it's totally doesnt make sense.

    For now. The index of display queue is 0-3. So if 1st ~4th use N-way. 5th
    can not use mvop dispaly because the inde of sharememory was
    set 0-3. it's really fucning stupid design....

    Conclusion: i think the count and index of display queue sharememory should
    be independent. Timothy

    ************************************************************/
    return dff->u32StreamUniqueId ? 1 : 0 ;
}

void *vsync_bridge_monitor_thread(void *arg)
{
    if (arg == NULL) return NULL;
    MS_DispFrameFormat *dff = (MS_DispFrameFormat *)arg;
    MS_U32 ShmIndex = vsync_bridge_GetSHMIndex(dff);
     int index = dff->OverlayID ? 1 : 0;

    ALOGD("[%d] enter vsync_bridge_monitor_thread", index);

    while (u8VsyncBridgeEnable[index]) {

        MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[ShmIndex] + (ShmIndex * sizeof(MCU_DISPQ_INFO)));

        VSYNC_BRIDGE_LOCK(index);

        if (pSHM && pSHM->u8McuDispSwitch) {

            // update current W ptr DS index, when it is pause state
            if (ds_int_data[index].bIsInitialized
                &&(pSHM->u8McuDispQWPtr == pSHM->u8McuDispQRPtr)
                && !pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8Tog_Time
                && (ds_curr_index[index].u8DSIndex != pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8DSIndex)) {

                pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u16CropLeft = CurrentCropWin[index].x;
                pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u16CropTop = CurrentCropWin[index].y;
                pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u16CropRight = pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u16Width
                                                            - CurrentCropWin[index].width
                                                            - CurrentCropWin[index].x;
                pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u16CropBottom = pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u16Height
                                                            - CurrentCropWin[index].height
                                                            - CurrentCropWin[index].y;

                pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8DSIndex = ds_curr_index[index].u8DSIndex;

                ALOGD("vsync_bridge_ds_thread DS_Idx=0x%x, DS_Idx1=0x%x, u8DSIndex = 0x%x, overlayID=%d",
                    ds_curr_index[index].u4DSIndex0 , ds_curr_index[index].u4DSIndex1,
                    pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8DSIndex, index);
            }

            // check if frame show done
            if (sBufSlotInfo[index].u8InQueueCnt > 1) {
                int i;
                for (i = 0; i < MAX_VSYNC_BRIDGE_DISPQ_NUM; i++) {
                    if (sBufSlotInfo[index].sBufSlot[i].eBufState == MS_BS_IN_QUEUE) {
                        if (pSHM->u8McuDispQRPtr != sBufSlotInfo[index].sBufSlot[i].u8CurIndex
                            && pSHM->McuDispQueue[sBufSlotInfo[index].sBufSlot[i].u8CurIndex].u8Tog_Time == 0) {
                            sBufSlotInfo[index].sBufSlot[i].eBufState = MS_BS_NONE;
                            sBufSlotInfo[index].u8InQueueCnt--;
                            if (frame_done_callback[index] != NULL) {
                                frame_done_callback[index](callback_handle[index], sBufSlotInfo[index].sBufSlot[i].u8CurIndex);
                            }
                            if (bMsLogON)
                                ALOGD("mointor Overlay[%d] W %d, R %d, Cur %d, disp_cnt = %d", index,
                                      pSHM->u8McuDispQWPtr, pSHM->u8McuDispQRPtr, sBufSlotInfo[index].sBufSlot[i].u8CurIndex,
                                      pSHM->McuDispQueue[sBufSlotInfo[index].sBufSlot[i].u8CurIndex].u16DispCnt);
                        }
                    }
                }
            }

        }

        VSYNC_BRIDGE_UNLOCK(index);

        usleep(u16FrameRate[index] ? (1000000000 / u16FrameRate[index] / 2) : 16000);
    }

    ALOGD("[%d] leave vsync_bridge_monitor_thread", index);

    return NULL;
}

inline static int isInDefaultAspecRatioMode(void)
{
    return (eARC_Type == E_AR_DEFAULT);
}

static void vsync_bridge_adjust_win_l(MS_WINDOW *pDispWin, float fRatio)
{
    MS_U16 u16Temp;

    ALOGD("Disp fRatio = %f", fRatio);
    u16Temp = (MS_U32)pDispWin->height * fRatio;
    if (u16Temp <= pDispWin->width) {
        u16Temp = (u16Temp / 2) * 2;
        pDispWin->x += (pDispWin->width - u16Temp) / 2;
        pDispWin->width = u16Temp;
    } else {
        u16Temp = (MS_U32)pDispWin->width / fRatio;
        u16Temp = (u16Temp / 2) * 2;
        pDispWin->y += (pDispWin->height - u16Temp) / 2;
        pDispWin->height = u16Temp;
    }
}

/* adjust disp window with NativeWindow scalingMode */
int vsync_bridge_adjust_win(MS_DispFrameFormat *dff, MS_WINDOW *pDispWin, float fRatio)
{
    int ret = false;
    if (dff->u32ScalingMode == NATIVE_WINDOW_SCALING_MODE_NO_SCALE_CROP) {
        vsync_bridge_adjust_win_l(pDispWin, fRatio);
        ret = true;
    }
    return ret;
}

static void vsync_bridge_adjust_crop_l(MS_DispFrameFormat *dff, MS_WINDOW *crop, int width, int height)
{
    MS_U32 cropW, cropH;

    cropW = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width -
            dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft -
            dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
    cropH = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height -
            dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop -
            dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;

    // the MVOP already setting as u32CropLeft and u32CropTop, x and y must 0
    crop->x = 0;
    crop->y = 0;
    crop->width = cropW;
    crop->height = cropH;
}

/* adjust crop with NativeWindow scalingMode */
int vsync_bridge_adjust_crop(MS_DispFrameFormat *dff, MS_WINDOW *crop, int width, int height)
{
    int ret = false;

    if (dff->u32ScalingMode == NATIVE_WINDOW_SCALING_MODE_SCALE_CROP) {
        MS_U32 dispW, dispH;
        MS_U32 cropW, cropH;
        MS_U32 expectedCropW;

        dispW = width;
        dispH = height;
        cropW = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width -
                dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft -
                dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
        cropH = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height -
                dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop -
                dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;

        //ALOGI("oriHW: %d, %d", dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width, dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height);
        //ALOGI("cropWH: %d %d, dispWH: %d %d", cropW, cropH, dispW, dispH);
        expectedCropW = cropH * dispW/dispH;
        //ALOGI("expectedCropW: %d", expectedCropW);
        if (expectedCropW <= cropW) {
            MS_U32 diff = cropW - expectedCropW;
            crop->x = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft + diff/2;
            crop->width = expectedCropW;
            crop->y = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop;
            crop->height = cropH;
        } else {
            MS_U32 expectedCropH = cropW * dispH/dispW;
            MS_U32 diff = cropH - expectedCropH;
            //ALOGI("expectedCropH: %d", expectedCropH);
            crop->y = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop +diff/2;
            crop->height = expectedCropH;
            crop->x = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft;
            crop->width = cropW;
        }
        ret = true;
    } else {
        vsync_bridge_adjust_crop_l(dff, crop, width, height);
    }

    return ret;
}

static int recal_clip_region(const hwc_rect_t *pos, hwc_rect_t *disp, int screen_w, int screen_h, int img_w, int img_h, int rotation, hwc_rect_t *clip)
{
    int rc = 0;
    int temp_x = pos->left, temp_y = pos->top, temp_w = pos->right - pos->left, temp_h = pos->bottom - pos->top;
    int clip_x = 0, clip_y = 0, clip_w, clip_h;
    int obj_w, obj_h;
    int x = temp_x, y = temp_y, w = temp_w, h = temp_h;

    if (rotation == 90 || rotation == 270) {
        obj_w = img_h;
        obj_h = img_w;
        clip_w = img_h;
        clip_h = img_w;
    } else {
        obj_w = img_w;
        obj_h = img_h;
        clip_w = img_w;
        clip_h = img_h;
    }

    if (w == 0 || h == 0) {
        rc = -1;
    } else
        if (x >= 0 && y >= 0 && (x + w) <= screen_w && (y + h) <= screen_h) {
        // none clip region path
    } else {
        // clip region path
        if ((x + (int)w) < 0) {
            rc = -1;
        }
        if ((x + (int)w) > (int)screen_w) {
            if (x >= 0) {
                clip_w = (screen_w - x) * obj_w / w;
                temp_w = screen_w - x;
            } else {
                clip_w = screen_w * obj_w / w;
                temp_w = screen_w;
            }
        } else {
            if (x >= 0) {
                clip_w = w * obj_w / w;
            } else {
                clip_w = (x + w) * obj_w / w;
                temp_w = x + w;
            }
        }

        if ((y + (int)h) < 0) {
            rc = -1;
        }
        if ((y + (int)h) > (int)screen_h) {
            if (y >= 0) {
                clip_h = (screen_h - y) * obj_h / h;
                temp_h = screen_h - y;
            } else {
                clip_h = screen_h * obj_h / h;
                temp_h = screen_h;
            }
        } else {
            if (y >= 0) {
                clip_h = h * obj_h / h;
            } else {
                clip_h = (y + h) * obj_h / h;
                temp_h = y + h;
            }
        }

        if (x >= (int)screen_w) {
            rc = -1;
        } else
            if (x < 0) {
            clip_x = (-x) * (int)obj_w / (int)w;
            temp_x = 0;
        }

        if (y >= (int)screen_h) {
            rc = -1;
        } else
            if (y < 0) {
            clip_y = (-y) * (int)obj_h / (int)h;
            temp_y = 0;
        }
    }

    if (rotation == 90 || rotation == 270) {
        obj_w = clip_x;
        clip_x = clip_y;
        clip_y = obj_w;
        obj_w = clip_w;
        clip_w = clip_h;
        clip_h = obj_w;
    }

    disp->left = temp_x;
    disp->right = temp_x + temp_w;
    disp->top = temp_y;
    disp->bottom = temp_y + temp_h;

    clip->left = clip_x;
    clip->right = clip_x + clip_w;
    clip->top = clip_y;
    clip->bottom = clip_y + clip_h;

    return rc;
}

#define DISP_FRAME_TO_MI(dff, stDispFrameFormat) \
do { \
        memset(&(stDispFrameFormat), 0, sizeof(stDispFrameFormat)); \
        (stDispFrameFormat).u16SizeofStructure = sizeof(stDispFrameFormat); \
        (stDispFrameFormat).u32OverlayID = (dff)->OverlayID; \
        (stDispFrameFormat).sFrames[0] = *(MI_VIDEO_FrameFormat_t *)(&(dff)->sFrames[0]); \
        (stDispFrameFormat).sFrames[1] = *(MI_VIDEO_FrameFormat_t *)(&(dff)->sFrames[1]); \
        (stDispFrameFormat).u64Pts = (dff)->u64Pts; \
        (stDispFrameFormat).u32FrameRate = (dff)->u32FrameRate; \
        (stDispFrameFormat).u8AspectRate = (dff)->u8AspectRate; \
        (stDispFrameFormat).CodecType = (dff)->CodecType; \
        (stDispFrameFormat).u8Interlace = (dff)->u8Interlace; \
        (stDispFrameFormat).u8FrcMode = (dff)->u8FrcMode; \
        (stDispFrameFormat).u83DMode = (dff)->u83DMode; \
        (stDispFrameFormat).u8BottomFieldFirst = (dff)->u8BottomFieldFirst; \
        (stDispFrameFormat).u8FreezeThisFrame = (dff)->u8FreezeThisFrame; \
        (stDispFrameFormat).u8CurIndex = (dff)->u8CurIndex; \
        (stDispFrameFormat).u32SHMAddr = (dff)->u32SHMAddr; \
        (stDispFrameFormat).u8ToggleTime = (dff)->u8ToggleTime; \
        (stDispFrameFormat).u8IsNoWaiting = (dff)->u8IsNoWaiting; \
        (stDispFrameFormat).u32AspectWidth = (dff)->u32AspectWidth; \
        (stDispFrameFormat).u32AspectHeight = (dff)->u32AspectHeight; \
        (stDispFrameFormat).u32ScalingMode = (dff)->u32ScalingMode; \
        (stDispFrameFormat).u8MCUMode = (dff)->u8MCUMode; \
        (stDispFrameFormat).u8SeamlessDS = (dff)->u8SeamlessDS; \
        (stDispFrameFormat).u8FBLMode = (dff)->u8FBLMode; \
        (stDispFrameFormat).u8FieldCtrl = (dff)->u8FieldCtrl; \
        (stDispFrameFormat).u8ApplicationType = (dff)->u8ApplicationType; \
        (stDispFrameFormat).u8Reser[0] = (dff)->u8Reser[0]; \
        (stDispFrameFormat).u8Reser[1] = (dff)->u8Reser[1]; \
        (stDispFrameFormat).u8Reser[2] = (dff)->u8Reser[2]; \
        (stDispFrameFormat).u32Reser = (dff)->u32Reser; \
} while(0)

static MS_BOOL is_dolby_vision(const MS_DispFrameFormat *dff)
{
#if defined(UFO_XC_SET_DSINFO_V0) && (API_XCDS_INFO_VERSION == 2)
    if (dff->CodecType == MS_CODEC_TYPE_HEVC_DV && dff->sHDRInfo.stDolbyHDRInfo.phyHDRRegAddr) {
        return TRUE;
    } else
#endif
    {
        return FALSE;
    }
}

static void update_ds_table(MS_DispFrameFormat *dff, MS_U8 u8FrameIndex, MS_BOOL bLock, MS_BOOL bSizeChange)
{
    MS_U8 u8NewDSIndex = 0;
    ds_recal_structure ds_structure;
    // MStar Android Patch Begin
    // fix coverity HIGH_IMPACT: UNINIT
    memset(&ds_structure, 0x00, sizeof(ds_structure));
    // MStar Android Patch End
    MS_U16 u16Width = dff->sFrames[u8FrameIndex].u32Width
                      - dff->sFrames[u8FrameIndex].u32CropLeft
                      - dff->sFrames[u8FrameIndex].u32CropRight;
    MS_U16 u16Height = dff->sFrames[u8FrameIndex].u32Height
                       - dff->sFrames[u8FrameIndex].u32CropTop
                       - dff->sFrames[u8FrameIndex].u32CropBottom;
    XC_SETWIN_INFO stXC_SetWin_Info;
    memset(&stXC_SetWin_Info, 0, sizeof(XC_SETWIN_INFO));

    if (IS_XC_SUPPORT_SWDS) {

        XC_ApiStatus stXCStatus;
        MVOP_Timing stMVOPTiming;

        if (dff->OverlayID) {
            MDrv_MVOP_SubGetOutputTiming(&stMVOPTiming);
        } else {
            MDrv_MVOP_GetOutputTiming(&stMVOPTiming);
        }

        // get scaler information
        if (MApi_XC_GetStatus(&stXCStatus, (SCALER_WIN) dff->OverlayID) == FALSE) {
            ALOGD("MApi_XC_GetStatus failed.\n");
        }

        stXC_SetWin_Info.enInputSourceType = stXCStatus.enInputSourceType;

        if (dff->OverlayID) {
            stXC_SetWin_Info.stCapWin.x = MDrv_MVOP_SubGetHStart();
            stXC_SetWin_Info.stCapWin.y = MDrv_MVOP_SubGetVStart();
        } else {
            stXC_SetWin_Info.stCapWin.x = MDrv_MVOP_GetHStart();
            stXC_SetWin_Info.stCapWin.y = MDrv_MVOP_GetVStart();
        }

        stXC_SetWin_Info.stCapWin.width = u16Width;
        stXC_SetWin_Info.stCapWin.height = u16Height;

        stXC_SetWin_Info.stDispWin.width = stXCStatus.stDispWin.width;
        stXC_SetWin_Info.stDispWin.height = stXCStatus.stDispWin.height;

        // Timing
        stXC_SetWin_Info.bHDuplicate = stMVOPTiming.bHDuplicate;
        stXC_SetWin_Info.u16InputVTotal = stMVOPTiming.u16V_TotalCount;
        stXC_SetWin_Info.bInterlace = stMVOPTiming.bInterlace;
        if (stXC_SetWin_Info.bInterlace) {
            stXC_SetWin_Info.u16InputVFreq = (stMVOPTiming.u16ExpFrameRate * 2) / 100;
        } else {
            stXC_SetWin_Info.u16InputVFreq = stMVOPTiming.u16ExpFrameRate / 100;
        }
        stXC_SetWin_Info.u16DefaultHtotal = stXCStatus.u16DefaultHtotal;
        stXC_SetWin_Info.u8DefaultPhase = stXCStatus.u8DefaultPhase;

        // customized scaling //post
        stXC_SetWin_Info.bHCusScaling = stXCStatus.bHCusScaling;
        stXC_SetWin_Info.u16HCusScalingSrc = stXCStatus.u16HCusScalingSrc; //stXC_SetWin_Info.stCapWin.width
        stXC_SetWin_Info.u16HCusScalingDst = stXCStatus.u16HCusScalingDst;
        stXC_SetWin_Info.bVCusScaling = stXCStatus.bVCusScaling;
        stXC_SetWin_Info.u16VCusScalingSrc = stXCStatus.u16VCusScalingSrc; //stXC_SetWin_Info.stCapWin.height
        stXC_SetWin_Info.u16VCusScalingDst = stXCStatus.u16VCusScalingDst;

        // 9 lattice
        stXC_SetWin_Info.bDisplayNineLattice = stXCStatus.bDisplayNineLattice;

        //add // force disable prescaling
        stXC_SetWin_Info.bPreHCusScaling = TRUE;
        stXC_SetWin_Info.bPreVCusScaling = TRUE;
        stXC_SetWin_Info.u16PreHCusScalingSrc = stXC_SetWin_Info.stCapWin.width;
        if (u8SaveBandwidthMode[dff->OverlayID] && (u16Width >= 3840)) {
            // for saving BW, 4K video H prescaling down 1/2
            stXC_SetWin_Info.u16PreHCusScalingDst = stXC_SetWin_Info.stCapWin.width / 2;
        } else if (u16Width >= 3840) {
            // exceed 3840, prescaling down to 3840
            stXC_SetWin_Info.u16PreHCusScalingDst = 3840;
        } else {
            stXC_SetWin_Info.u16PreHCusScalingDst = stXC_SetWin_Info.stCapWin.width;
        }
        stXC_SetWin_Info.u16PreVCusScalingSrc = stXC_SetWin_Info.stCapWin.height;
        // height exceed 2160, prescaling down to 2160
        if (u16Height >= 2160) {
            stXC_SetWin_Info.u16PreVCusScalingDst = 2160;
        } else {
            stXC_SetWin_Info.u16PreVCusScalingDst = stXC_SetWin_Info.stCapWin.height;
        }

        if (bSizeChange) {
            ALOGD("[%s,%5d] DispW:%d DispH:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.stDispWin.width, stXC_SetWin_Info.stDispWin.height);

            ALOGD("[%s,%5d] Cap.x:%d Cap.y:%d CapW:%d CapH:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.stCapWin.x, stXC_SetWin_Info.stCapWin.y,
                stXC_SetWin_Info.stCapWin.width, stXC_SetWin_Info.stCapWin.height);
            ALOGD("[%s,%5d] enInputSourceType:%d \n",__func__,__LINE__,stXC_SetWin_Info.enInputSourceType);
            ALOGD("[%s,%5d] u16InputVTotal:%d  u16InputVFreq:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.u16InputVTotal, stXC_SetWin_Info.u16InputVFreq);
            ALOGD("[%s,%5d] bInterlace:%d  u16DefaultHtotal:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.bInterlace, stXC_SetWin_Info.u16DefaultHtotal);
            //prescaling
            ALOGD("[%s,%5d] bPreHCusScaling:%d  bPreVCusScaling:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.bPreHCusScaling, stXC_SetWin_Info.bPreVCusScaling);
            ALOGD("[%s,%5d] u16PreHCusScalingSrc:%d  u16PreHCusScalingDst:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.u16PreHCusScalingSrc, stXC_SetWin_Info.u16PreHCusScalingDst);
            ALOGD("[%s,%5d] u16PreVCusScalingSrc:%d  u16PreVCusScalingSrc:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.u16PreVCusScalingSrc, stXC_SetWin_Info.u16PreVCusScalingDst);
            //postscaling
            ALOGD("[%s,%5d] bHCusScaling:%d  bVCusScaling:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.bHCusScaling, stXC_SetWin_Info.bVCusScaling);
            ALOGD("[%s,%5d] u16HCusScalingSrc:%d  u16HCusScalingDst:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.u16HCusScalingSrc, stXC_SetWin_Info.u16HCusScalingDst);
            ALOGD("[%s,%5d] u16VCusScalingSrc:%d  u16VCusScalingDst:%d \n", __func__, __LINE__,
                stXC_SetWin_Info.u16VCusScalingSrc, stXC_SetWin_Info.u16VCusScalingDst);
        }
    }else {

        ds_int_data[dff->OverlayID].pDSXCData->bSaveBandwidthMode = u8SaveBandwidthMode[dff->OverlayID];
    }

    if ((ExternalWin[dff->OverlayID].u8SetFromExt) && (!u8SizeChange[dff->OverlayID])) {
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.x = ExternalWin[dff->OverlayID].u16CropX;
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.y = ExternalWin[dff->OverlayID].u16CropY;
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.width = ExternalWin[dff->OverlayID].u16CropWidth;
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.height = ExternalWin[dff->OverlayID].u16CropHeight;

        if (IS_XC_SUPPORT_SWDS) {
            stXC_SetWin_Info.stCropWin.x = ExternalWin[dff->OverlayID].u16CropX;
            stXC_SetWin_Info.stCropWin.y = ExternalWin[dff->OverlayID].u16CropY;
            stXC_SetWin_Info.stCropWin.width = ExternalWin[dff->OverlayID].u16CropWidth;
            stXC_SetWin_Info.stCropWin.height = ExternalWin[dff->OverlayID].u16CropHeight;
        }

        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.x = ExternalWin[dff->OverlayID].u16DispX + g_IPanel.HStart();
        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.y = ExternalWin[dff->OverlayID].u16DispY + g_IPanel.VStart();
        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.width = ExternalWin[dff->OverlayID].u16DispWidth;
        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.height = ExternalWin[dff->OverlayID].u16DispHeight;

        if (IS_XC_SUPPORT_SWDS) {
            stXC_SetWin_Info.stDispWin.x = ExternalWin[dff->OverlayID].u16DispX;
            stXC_SetWin_Info.stDispWin.y = ExternalWin[dff->OverlayID].u16DispY;
            stXC_SetWin_Info.stDispWin.width = ExternalWin[dff->OverlayID].u16DispWidth;
            stXC_SetWin_Info.stDispWin.height = ExternalWin[dff->OverlayID].u16DispHeight;
        }
        ALOGD("Set DISP/CROP FromExt!!!");
    } else {

        /* NativeWindow scalingMode is only applied in Default Zoom Mode */
        MS_WINDOW CropWin;
        MS_WINDOW DispWin;
        MS_FrameFormat stFrame;
        MS_VideoInfo vInfo;
        stFrame = dff->sFrames[u8FrameIndex];
        memcpy(&DispWin, &OutputWin[dff->OverlayID], sizeof(MS_WINDOW));

        char cropValue[PROPERTY_VALUE_MAX] = {0};
        property_get("mstar.crop.x", cropValue, "0");
        int x = atoi(cropValue);
        property_get("mstar.crop.y", cropValue, "0");
        int y = atoi(cropValue);
        property_get("mstar.crop.width", cropValue, "0");
        int width = atoi(cropValue);
        property_get("mstar.crop.height", cropValue, "0");
        int height = atoi(cropValue);
        if (x != 0 || y != 0 || width != 0 || height != 0) {
            if (dff->CodecType == MS_CODEC_TYPE_MVC) {
                vInfo.u16HorSize = stFrame.u32Width;
                vInfo.u16VerSize = stFrame.u32Height;
                vInfo.u16CropRight = 0;
                vInfo.u16CropLeft = 0;
                vInfo.u16CropBottom = 0;
                vInfo.u16CropTop = 0;
            } else {
                vInfo.u16HorSize = stFrame.u32Width;
                vInfo.u16VerSize = stFrame.u32Height;
                vInfo.u16CropRight = vInfo.u16HorSize - (x + width);
                vInfo.u16CropLeft = x;
                vInfo.u16CropBottom = vInfo.u16VerSize - (y + height);
                vInfo.u16CropTop = y;
            }
        } else {
            if (dff->CodecType == MS_CODEC_TYPE_MVC) {
                vInfo.u16HorSize = stFrame.u32Width - stFrame.u32CropRight - stFrame.u32CropLeft;
                vInfo.u16VerSize = stFrame.u32Height - stFrame.u32CropBottom - stFrame.u32CropTop;
                vInfo.u16CropRight = 0;
                vInfo.u16CropLeft = 0;
                vInfo.u16CropBottom = 0;
                vInfo.u16CropTop = 0;
            } else {
                vInfo.u16HorSize = stFrame.u32Width;
                vInfo.u16VerSize = stFrame.u32Height;
                vInfo.u16CropRight = stFrame.u32CropRight;
                vInfo.u16CropLeft = stFrame.u32CropLeft;
                vInfo.u16CropBottom = stFrame.u32CropBottom;
                vInfo.u16CropTop = stFrame.u32CropTop;
            }
        }

        CropWin.x = vInfo.u16CropLeft;
        CropWin.y = vInfo.u16CropTop;
        CropWin.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
        CropWin.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;

        vsync_bridge_CalAspectRatio(&DispWin, &CropWin, dff, TRUE, FALSE, 0, 0);

        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.x = CropWin.x;
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.y = CropWin.y;
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.width = CropWin.width;
        ds_int_data[dff->OverlayID].pDSXCData->stCropWin.height = CropWin.height;

        if (bSizeChange) {
            Wrapper_SurfaceVideoSizeChange(dff->OverlayID, DispWin.x,
                DispWin.y, DispWin.width, DispWin.height, u16SrcWidth[dff->OverlayID],
                u16SrcHeight[dff->OverlayID], bLock);
        }

        if (u8MirrorMode & E_VOPMIRROR_HORIZONTALL) {
            DispWin.x = panelWidth[dff->OverlayID] - (DispWin.x + DispWin.width);
        }
        if (u8MirrorMode & E_VOPMIRROR_VERTICAL) {
            DispWin.y = panelHeight[dff->OverlayID] - (DispWin.y + DispWin.height);
        }
        if (bSizeChange && (u8MirrorMode & E_VOPMIRROR_HVBOTH)) {
            ALOGD("DS MVOP Mirror %d [%d %d %d %d]", u8MirrorMode, DispWin.x, DispWin.y, DispWin.width, DispWin.height);
        }

        if (IS_XC_SUPPORT_SWDS) {
            stXC_SetWin_Info.stCropWin.x = CropWin.x;
            stXC_SetWin_Info.stCropWin.y = CropWin.y;
            stXC_SetWin_Info.stCropWin.width = CropWin.width;
            stXC_SetWin_Info.stCropWin.height = CropWin.height;
        }

        hwc_rect_t disp_old;
        hwc_rect_t disp_new = {0,0,0,0};
        hwc_rect_t crop = {0,0,0,0};

        disp_old.left = (short)DispWin.x;
        disp_old.top = (short)DispWin.y;
        disp_old.right = (short)DispWin.x + (short)DispWin.width;
        disp_old.bottom = (short)DispWin.y + (short)DispWin.height;

        if (recal_clip_region(&disp_old, &disp_new, panelWidth[dff->OverlayID], panelHeight[dff->OverlayID],
                                       CropWin.width, CropWin.height, 0, &crop) == 0) {

            crop.left += CropWin.x;
            crop.right += CropWin.x;
            crop.top += CropWin.y;
            crop.bottom += CropWin.y;

            if (bSizeChange) {
                ALOGD("disp_old[%d %d %d %d], disp_new[%d %d %d %d], crop[%d %d %d %d]",
                    disp_old.left,  disp_old.top, disp_old.right, disp_old.bottom,
                    disp_new.left,  disp_new.top, disp_new.right, disp_new.bottom,
                    crop.left,  crop.top, crop.right, crop.bottom);
            }

            ds_int_data[dff->OverlayID].pDSXCData->stCropWin.x = crop.left;
            ds_int_data[dff->OverlayID].pDSXCData->stCropWin.y = crop.top;
            ds_int_data[dff->OverlayID].pDSXCData->stCropWin.width = crop.right - crop.left;
            ds_int_data[dff->OverlayID].pDSXCData->stCropWin.height = crop.bottom - crop.top;

            if (IS_XC_SUPPORT_SWDS) {
                stXC_SetWin_Info.stCropWin.x = crop.left;
                stXC_SetWin_Info.stCropWin.y = crop.top;
                stXC_SetWin_Info.stCropWin.width = crop.right - crop.left;
                stXC_SetWin_Info.stCropWin.height = crop.bottom - crop.top;
            }

            DispWin.x = disp_new.left;
            DispWin.y = disp_new.top;
            DispWin.width = disp_new.right - disp_new.left;
            DispWin.height = disp_new.bottom - disp_new.top;

        } else {
            ALOGD("recal_clip_region error");
        }

        #ifdef MSTAR_NAPOLI
        if (panelWidth[dff->OverlayID] == 3840 && (g_IPanel.DefaultVFreq() == 0 || g_IPanel.DefaultVFreq() == 600)) {
            // napoli 4K2K XC limitation, convert to 2K2K then pass to FRC
            DispWin.x /= 2;
            DispWin.width /= 2;
        }
        #endif

        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.x = DispWin.x + g_IPanel.HStart();
        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.y = DispWin.y + g_IPanel.VStart();
        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.width = DispWin.width;
        ds_int_data[dff->OverlayID].pDSXCData->stDispWin.height = DispWin.height;

        if (IS_XC_SUPPORT_SWDS) {
            stXC_SetWin_Info.stDispWin.x = DispWin.x;
            stXC_SetWin_Info.stDispWin.y = DispWin.y;
            stXC_SetWin_Info.stDispWin.width = DispWin.width;
            stXC_SetWin_Info.stDispWin.height = DispWin.height;
        }

    }

    if (!IS_XC_SUPPORT_SWDS) {
        ds_curr_index[dff->OverlayID].u8DSIndex = ds_next_index[dff->OverlayID];

        ds_structure.CodedWidth = dff->sFrames[u8FrameIndex].u32Width
                                    - dff->sFrames[u8FrameIndex].u32CropRight;
        ds_structure.CodedHeight = dff->sFrames[u8FrameIndex].u32Height
                                    - dff->sFrames[u8FrameIndex].u32CropBottom;

        ds_structure.DSBufIdx = ds_curr_index[dff->OverlayID].u8DSIndex;
        ds_structure.pDSIntData = &ds_int_data[dff->OverlayID];
        ds_structure.CropRight = dff->sFrames[u8FrameIndex].u32CropRight;
    }

    if (g_IPanel.Width()) {
        ds_int_data[dff->OverlayID].pDSXCData->u32PNL_Width = g_IPanel.Width();
    }

    // disable force index !!!!!!!!!!!!!!!!!
    MApi_XC_Set_DSForceIndex(0, 0, (SCALER_WIN)dff->OverlayID);

    if (IS_XC_SUPPORT_SWDS) {

        if (bSizeChange) {
            ALOGD("[%s,%5d] SW DS !!! \n", __func__, __LINE__);
            ALOGD("[%s,%5d] stXC_SetWin_Info.stCapWin.width%d  stXC_SetWin_Info.stCapWin.height:%d\n",
                __func__, __LINE__, stXC_SetWin_Info.stCapWin.width, stXC_SetWin_Info.stCapWin.height);
            ALOGD("[%s,%5d] stXC_SetWin_Info.stCropWin.width%d  stXC_SetWin_Info.stCropWin.height:%d\n",
                __func__, __LINE__, stXC_SetWin_Info.stCropWin.width, stXC_SetWin_Info.stCropWin.height);
            ALOGD("[%s,%5d] MApi_XC_GetDynamicScalingStatus():%d \n", __func__, __LINE__, MApi_XC_GetDynamicScalingStatus());
        }

        if (MApi_XC_GetDynamicScalingStatus()) {
#ifdef UFO_XC_SET_DSINFO_V0
            if (bIsMVOPSWCrop || is_dolby_vision(dff)) {

                XC_DS_INFO stXCDSInfo = {0, };

                stXCDSInfo.u32ApiDSInfo_Version = API_XCDS_INFO_VERSION;
                stXCDSInfo.u16ApiDSInfo_Length = sizeof(XC_DS_INFO);
                stXCDSInfo.u32MFCodecInfo = dff->sDispFrmInfoExt.u32MFCodecInfo;
#if (API_XCDS_INFO_VERSION == 1)
                if (sizeof(XC_DS_HDRInfo) == sizeof(MS_HDRInfo)) {
                    memcpy(&stXCDSInfo.stHDRInfo, &dff->sHDRInfo, sizeof(XC_DS_HDRInfo));
                } else {
                    ALOGE("MS_HDRInfo/XC_DS_HDRInfo size is not match");
                }
#elif (API_XCDS_INFO_VERSION == 2)
                stXCDSInfo.stHDRInfo.u32FrmInfoExtAvail = dff->sHDRInfo.u32FrmInfoExtAvail;

                if (sizeof(XC_DS_ColorDescription) == sizeof(MS_ColorDescription)) {
                    memcpy(&stXCDSInfo.stHDRInfo.stColorDescription, &dff->sHDRInfo.stColorDescription, sizeof(XC_DS_ColorDescription));
                } else {
                    ALOGE("XC_DS_ColorDescription/MS_ColorDescription size is not match");
                }

                if (sizeof(XC_DS_MasterColorDisplay) == sizeof(MS_MasterColorDisplay)) {
                    memcpy(&stXCDSInfo.stHDRInfo.stMasterColorDisplay, &dff->sHDRInfo.stMasterColorDisplay, sizeof(XC_DS_MasterColorDisplay));
                } else {
                    ALOGE("XC_DS_MasterColorDisplay/MS_MasterColorDisplay size is not match");
                }
                stXCDSInfo.stHDRInfo.u8CurrentIndex = dff->sHDRInfo.stDolbyHDRInfo.u8CurrentIndex;
                stXCDSInfo.stHDRInfo.phyRegAddr = dff->sHDRInfo.stDolbyHDRInfo.phyHDRRegAddr;
                stXCDSInfo.stHDRInfo.u32RegSize = dff->sHDRInfo.stDolbyHDRInfo.u32HDRRegSize;
                stXCDSInfo.stHDRInfo.phyLutAddr = dff->sHDRInfo.stDolbyHDRInfo.phyHDRLutAddr;
                stXCDSInfo.stHDRInfo.u32LutSize = dff->sHDRInfo.stDolbyHDRInfo.u32HDRLutSize;
                stXCDSInfo.stHDRInfo.bDMEnable = dff->sHDRInfo.stDolbyHDRInfo.u8DMEnable;
                stXCDSInfo.stHDRInfo.bCompEnable = dff->sHDRInfo.stDolbyHDRInfo.u8CompEnable;
#endif
                MApi_XC_SetDSInfo(&stXCDSInfo, sizeof(XC_DS_INFO), (SCALER_WIN)dff->OverlayID);
            }
#endif

            if (MApi_XC_SetWindow(&stXC_SetWin_Info, sizeof(XC_SETWIN_INFO), (SCALER_WIN)dff->OverlayID) == FALSE) {
                ALOGD("MApi_XC_SetWindow failed!\n");
            }
        }

        if (u8FrameIndex == 1) {
            ds_curr_index[dff->OverlayID].u4DSIndex1 = MApi_XC_GetSWDSIndex((SCALER_WIN)dff->OverlayID);
        } else {
            ds_curr_index[dff->OverlayID].u4DSIndex0 = MApi_XC_GetSWDSIndex((SCALER_WIN)dff->OverlayID);
        }
    } else {
        ALOGD("[%s,%5d] Original DS !!! \n", __func__, __LINE__);

        dynamic_scale(&ds_structure);
        ds_next_index[dff->OverlayID] = (ds_next_index[dff->OverlayID] + 1) % u8MaxDSIdx[dff->OverlayID];
    }

}


static void vsync_bridge_ds_set_index(MS_DispFrameFormat *dff, MS_BOOL bLock)
{
    // !!! this function must in mutex protection !!!
    UPDATE_SHM_ADDR;

    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff)*sizeof(MCU_DISPQ_INFO)));
    MS_U16 u16Width = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width
                      - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft
                      - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
    MS_U16 u16Height = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height
                       - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop
                       - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;

    if (pSHM && pSHM->u8McuDispSwitch && ds_int_data[dff->OverlayID].bIsInitialized && Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
        if ((u8SizeChange[dff->OverlayID] || ExternalWin[dff->OverlayID].u8SetFromExt)) {

            update_ds_table(dff, 0, bLock, TRUE);
            if ((ExternalWin[dff->OverlayID].u8SetFromExt) && (!u8SizeChange[dff->OverlayID])) {
                ExternalWin[dff->OverlayID].u8SetFromExt = 0;
            } else {
                u8SizeChange[dff->OverlayID] = 0;
            }

            ALOGD("vsync_bridge_render_frame pts = %lld, DS_Idx=0x%x, DS_Idx1=0x%x, u8DSIndex = 0x%x, overlayID=%d",
                dff->u64Pts, ds_curr_index[dff->OverlayID].u4DSIndex0 , ds_curr_index[dff->OverlayID].u4DSIndex1,
                pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8DSIndex, dff->OverlayID);

            ALOGD(" W %d, R %d, Cur %d", pSHM->u8McuDispQWPtr, pSHM->u8McuDispQRPtr, dff->u8CurIndex);

            MsOS_FlushMemory();
        }
    }
}

#ifdef BUILD_MSTARTV_MI
static void timingToWidthHeight(MI_DISPLAY_OutputTiming_e eOutputTiming, int *width, int *height) {
    switch (eOutputTiming) {
        case E_MI_DISPLAY_OUTPUT_TIMING_720X480_60P:
        case E_MI_DISPLAY_OUTPUT_TIMING_720X480_60I:
            *width=720;
            *height=480;
            break;
        case E_MI_DISPLAY_OUTPUT_TIMING_720X576_50I:
        case E_MI_DISPLAY_OUTPUT_TIMING_720X576_50P:
            *width=720;
            *height=576;
            break;
        case E_MI_DISPLAY_OUTPUT_TIMING_1280X720_50P:
        case E_MI_DISPLAY_OUTPUT_TIMING_1280X720_60P:
            *width=1280;
            *height=720;
            break;
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_50I:
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_60I:
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_24P:
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_25P:
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_30P:
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_50P:
        case E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_60P:
            *width=1920;
            *height=1080;
            break;
        case E_MI_DISPLAY_OUTPUT_TIMING_3840X2160_30P:
        case E_MI_DISPLAY_OUTPUT_TIMING_3840X2160_60P:
            *width=3840;
            *height=2160;
            break;
        default:
            *width=3840;
            *height=2160;
            break;
    }
}
#endif

void vsync_bridge_CalAspectRatio(MS_WINDOW *pDispWin, MS_WINDOW *pCropWin, MS_DispFrameFormat *dff, MS_BOOL bCalAspect, MS_BOOL bUseDefaultArc, int panel_width, int panel_height)
{
    MS_U16 u16Temp;
    MS_U32 u32CalWidth;
    MS_U32 u32CalHeight;
    MS_FrameFormat *sFrame;

    if ((pDispWin == NULL) || (pCropWin == NULL) || (dff == NULL)) return;

    Vsync_Bridge_ARC_Type eArcType = ((dff->u8ApplicationType & MS_APP_TYPE_IMAGE) || bUseDefaultArc) ? E_AR_DEFAULT: eARC_Type;

    if (bCalAspect) {
        float fRatio = 1.0;

        sFrame = &dff->sFrames[MS_VIEW_TYPE_CENTER];
        u32CalWidth = (sFrame->u32Width - sFrame->u32CropRight - sFrame->u32CropLeft);
        u32CalHeight = (sFrame->u32Height - sFrame->u32CropBottom - sFrame->u32CropTop);

        if (dff->u32AspectWidth && dff->u32AspectHeight
            && (dff->CodecType != MS_CODEC_TYPE_MVC)) {
            fRatio = (float)dff->u32AspectWidth/(float)dff->u32AspectHeight;
        } else if (u32CalWidth && u32CalHeight) {
            fRatio = (float)u32CalWidth/(float)u32CalHeight;
        }

        if (bMsLogON || !u8VsyncBridgeEnable[dff->OverlayID]) {
            ALOGD("vsync_bridge_CalAspectRatio type = %d", eArcType);
        }

        switch (eArcType) {

        case E_AR_DEFAULT:
#if (ENABLE_SF_SOURCE_CROP == 1)
            if (!(dff->u8ApplicationType & MS_APP_TYPE_IMAGE)
                && (SrcCropWinRatio[dff->OverlayID].width != 0)
                && (SrcCropWinRatio[dff->OverlayID].height != 0)) {
                pCropWin->x = sFrame->u32CropLeft + (u32CalWidth * SrcCropWinRatio[dff->OverlayID].x);
                pCropWin->y = sFrame->u32CropTop + (u32CalHeight * SrcCropWinRatio[dff->OverlayID].y);
                pCropWin->width = (u32CalWidth * SrcCropWinRatio[dff->OverlayID].width);
                pCropWin->height = (u32CalHeight * SrcCropWinRatio[dff->OverlayID].height);
            }
#endif
            break;
        case E_AR_AUTO:
            {
#if (KEEP_ASPECT_RATE == 1)
                vsync_bridge_adjust_win_l(pDispWin, fRatio);
#endif
            }
            break;

        case E_AR_16x9:
            {
                u16Temp = ((MS_U32)pDispWin->height * 16) / 9;
                if (u16Temp <= pDispWin->width) {  // H:V >= 16:9
                    pDispWin->x += (pDispWin->width - u16Temp) / 2;
                    pDispWin->width = u16Temp;
                } else { // H:V <= 16:9
                    u16Temp = ((MS_U32)pDispWin->width * 9) / 16;
                    pDispWin->y += (pDispWin->height - u16Temp) / 2;
                    pDispWin->height = u16Temp;
                }
            }
            break;

        case E_AR_4x3:
            {
                u16Temp = ((MS_U32)pDispWin->height * 4) / 3;
                if (u16Temp <= pDispWin->width) {  // H:V >= 4:3
                    pDispWin->x += (pDispWin->width - u16Temp) / 2;
                    pDispWin->width = u16Temp;
                } else { // H:V <= 4:3
                    u16Temp = ((MS_U32)pDispWin->width * 3) / 4;
                    pDispWin->y += (pDispWin->height - u16Temp) / 2;
                    pDispWin->height = u16Temp;
                }
            }
            break;

        case E_AR_14x9:
            {
                u16Temp = ((MS_U32)pDispWin->height * 14) / 9;
                if (u16Temp <= pDispWin->width) {  // H:V >= 4:3
                    pDispWin->x += (pDispWin->width - u16Temp) / 2;
                    pDispWin->width = u16Temp;
                } else { // H:V <= 14:9
                    u16Temp = ((MS_U32)pDispWin->width * 9) / 14;
                    pDispWin->y += (pDispWin->height - u16Temp) / 2;
                    pDispWin->height = u16Temp;
                }
            }
            break;

        case E_AR_Zoom1:
            {
                MS_U16 x0, y0,x1, y1;
                x0 = (MS_U16)(((MS_U32)pCropWin->width * 50) / 1000);
                y0 = (MS_U16)(((MS_U32)pCropWin->height * 60) / 1000);
                x0 &= ~0x1;
                y0 &= ~0x1;
                x1 = (MS_U16)(((MS_U32)pCropWin->width * 50) / 1000);
                y1 = (MS_U16)(((MS_U32)pCropWin->height * 60) / 1000);
                x1 &= ~0x1;
                y1 &= ~0x1;

                pCropWin->width = pCropWin->width - (x0 + x1);
                pCropWin->height = pCropWin->height - (y0 + y1);

                pCropWin->x = pCropWin->x + x0;
                pCropWin->y = pCropWin->y + y0;
            }
            break;

        case E_AR_Zoom2:
            {
                MS_U16 x0, y0,x1, y1;
                x0 = (MS_U16)(((MS_U32)pCropWin->width * 100) / 1000);
                y0 = (MS_U16)(((MS_U32)pCropWin->height * 120) / 1000);
                x0 &= ~0x1;
                y0 &= ~0x1;
                x1 = (MS_U16)(((MS_U32)pCropWin->width * 100) / 1000);
                y1 = (MS_U16)(((MS_U32)pCropWin->height * 120) / 1000);
                x1 &= ~0x1;
                y1 &= ~0x1;

                pCropWin->width = pCropWin->width - (x0 + x1);
                pCropWin->height = pCropWin->height - (y0 + y1);

                pCropWin->x = pCropWin->x + x0;
                pCropWin->y = pCropWin->y + y0;
            }
            break;

        case E_AR_JustScan:
            if (pCropWin->width <= pDispWin->width && pCropWin->height <= pDispWin->height) {
                pDispWin->x += (pDispWin->width - pCropWin->width) / 2;
                pDispWin->width = pCropWin->width;
                pDispWin->y += (pDispWin->height - pCropWin->height) / 2;
                pDispWin->height = pCropWin->height;
            }
            break;

        case E_AR_Panorama:
        case E_AR_DotByDot:
            break;

            // E_AR_Subtitle ,E_AR_movie,E_AR_Zoom3 is add  for CUS 36 ,please don't modify.
        case E_AR_Subtitle:
            u16Temp = ((MS_U32)pDispWin->height * 2) / 25;
            pCropWin->y += u16Temp;
            pCropWin->height = pCropWin->height - u16Temp;
            break;
        case E_AR_Movie:
            u16Temp = ((MS_U32)pDispWin->height * 2) / 25;
            pCropWin->y += u16Temp;
            pCropWin->height = pCropWin->height - 2*u16Temp;
            break;

        case E_AR_Personal:
            u16Temp = ((MS_U32)pDispWin->height * 3) / 50;
            pCropWin->y += u16Temp;
            pCropWin->height = pCropWin->height - 2*u16Temp;
            break;

        default:
            {
                #if (KEEP_ASPECT_RATE == 1)
                vsync_bridge_adjust_win_l(pDispWin, fRatio);
                #endif
            }
            break;
        }
        float Wratio = 1.0f, Hratio = 1.0f;

 #if BUILD_MSTARTV_MI
        prevPanelWidth[dff->OverlayID] = panelWidth[dff->OverlayID];
        prevPanelHeight[dff->OverlayID] = panelHeight[dff->OverlayID];
 #endif
        panelWidth[dff->OverlayID] = g_IPanel.Width();
        panelHeight[dff->OverlayID] = g_IPanel.Height();
        if (vsync_bridge_GetInterlaceOut()) {
            panelHeight[dff->OverlayID] = panelHeight[dff->OverlayID] << 1;
        }
#ifdef BUILD_MSTARTV_MI
        if ((panel_width == 0) || (panel_height== 0)) {
            MI_DISPLAY_HANDLE hDispHandle = MI_HANDLE_NULL;
            MI_DISPLAY_VideoTiming_t stCurrentVideoTiming;
            MI_DISPLAY_InitParams_t stInitParams;
            INIT_ST(stCurrentVideoTiming);
            INIT_ST(stInitParams);
            if (MI_DISPLAY_Init(&stInitParams) == MI_OK) {
                if (MI_DISPLAY_GetDisplayHandle(0, &hDispHandle) != MI_OK) {
                    MI_DISPLAY_Open(0, &hDispHandle);
                }
            }
            MI_DISPLAY_GetOutputTiming(hDispHandle, &stCurrentVideoTiming);
            timingToWidthHeight(stCurrentVideoTiming.eVideoTiming, &panelWidth[dff->OverlayID], &panelHeight[dff->OverlayID]);
            MI_DISPLAY_Close(hDispHandle);
            MI_DISPLAY_DeInit();
            ALOGV("timing %d , width %d,height %d \n", stCurrentVideoTiming.eVideoTiming, panelWidth[dff->OverlayID], panelHeight[dff->OverlayID]);
        } else {
            panelWidth[dff->OverlayID] = panel_width;
            panelHeight[dff->OverlayID] = panel_height;
        }
#endif

        if (u32OsdWidth && u32OsdHeight) {
            Wratio = (float)panelWidth[dff->OverlayID] / u32OsdWidth;
            Hratio = (float)panelHeight[dff->OverlayID] / u32OsdHeight;
        }

        if (bMsLogON || !u8VsyncBridgeEnable[dff->OverlayID]) {
            ALOGD("panel [%d %d] osd [%d %d] ratio [%f %f]", panelWidth[dff->OverlayID],
                panelHeight[dff->OverlayID], u32OsdWidth, u32OsdHeight, Wratio, Hratio);
        }

        // apply to Width
        if (Wratio > 0) {
            if (pDispWin->x < 0) {
                pDispWin->x = 0;
            } else {
                pDispWin->x *= Wratio;
            }

            pDispWin->width *= Wratio;
            if (pDispWin->width > panelWidth[dff->OverlayID]) {
                pDispWin->width = panelWidth[dff->OverlayID];
            }
        }
        // apply to Height
        if (Hratio > 0) {
            if (pDispWin->y < 0) {
                pDispWin->y = 0;
            } else {
                pDispWin->y *= Hratio;
            }

            pDispWin->height *= Hratio;
            if (pDispWin->height > panelHeight[dff->OverlayID]) {
                pDispWin->height = panelHeight[dff->OverlayID];
            }
        }
        char value[PROPERTY_VALUE_MAX] = {0};
        if (property_get("mstar.video.pos", value, "0"))
        {
            if (!strcmp(value,"1"))
            {
                pDispWin->width *= 2;
                pDispWin->height *= 2;
            }
            else if (!strcmp(value,"2"))
            {
                pDispWin->x -= pDispWin->width;
                pDispWin->width *= 2;
                pDispWin->height *= 2;
            }
            else if (!strcmp(value,"3"))
            {
                pDispWin->y -= pDispWin->height;
                pDispWin->width *= 2;
                pDispWin->height *= 2;
            }
            else if (!strcmp(value,"4"))
            {
                pDispWin->x -= pDispWin->width;
                pDispWin->y -= pDispWin->height;
                pDispWin->width *= 2;
                pDispWin->height *= 2;
            }
        }
        if (bMsLogON || !u8VsyncBridgeEnable[dff->OverlayID]) {
            ALOGD("apply ratio, disp_win[%d %d %d %d] crop_win[%d %d %d %d]",
                  pDispWin->x, pDispWin->y, pDispWin->width, pDispWin->height,
                  pCropWin->x, pCropWin->y, pCropWin->width, pCropWin->height);
        }
        memcpy(&CurrentCropWin[dff->OverlayID ? 1 : 0], pCropWin, sizeof(MS_WINDOW));
    } else {
#if BUILD_MSTARTV_SN
        MS_U16 org_panel_w = panelWidth[dff->OverlayID];
        MS_U16 org_panel_h = panelHeight[dff->OverlayID];
#elif BUILD_MSTARTV_MI
        MS_U16 org_panel_w = prevPanelWidth[dff->OverlayID];
        MS_U16 org_panel_h = prevPanelHeight[dff->OverlayID];
#endif
        MS_U16 new_panel_w = g_IPanel.Width();
        MS_U16 new_panel_h = g_IPanel.Height();
 #ifdef BUILD_MSTARTV_MI
        if ((panel_width == 0) || (panel_height== 0)) {
            MI_DISPLAY_HANDLE hDispHandle = MI_HANDLE_NULL;
            MI_DISPLAY_VideoTiming_t stCurrentVideoTiming;
            MI_DISPLAY_InitParams_t stInitParams;
            INIT_ST(stCurrentVideoTiming);
            INIT_ST(stInitParams);
            MI_DISPLAY_Init(&stInitParams);
            if (MI_DISPLAY_GetDisplayHandle(0, &hDispHandle) != MI_OK) {
                MI_DISPLAY_Open(0, &hDispHandle);
            }
            MI_DISPLAY_GetOutputTiming(hDispHandle, &stCurrentVideoTiming);
            timingToWidthHeight(stCurrentVideoTiming.eVideoTiming, &new_panel_w, &new_panel_h);
            MI_DISPLAY_Close(hDispHandle);
            MI_DISPLAY_DeInit();
            ALOGV("timing %d , width %d,height %d \n", stCurrentVideoTiming.eVideoTiming, new_panel_w, new_panel_w);
        } else {
            new_panel_w = panel_width;
            new_panel_h = panel_height;
        }
#endif

        if ((org_panel_w != new_panel_w) || (org_panel_h != new_panel_h)) {
            float Wratio = (float)new_panel_w / org_panel_w;
            float Hratio = (float)new_panel_h / org_panel_h;

            if (bMsLogON || !u8VsyncBridgeEnable[dff->OverlayID]) {
                ALOGD("panel timing change when 3D enable [%d %d] [%d %d]",
                    new_panel_w, new_panel_h, org_panel_w, org_panel_h);
            }
            // apply to Width
            pDispWin->x *= Wratio;

            pDispWin->width *= Wratio;
            if (pDispWin->width > new_panel_w) {
                pDispWin->width = new_panel_w;
            }

            // apply to Height
            pDispWin->y *= Hratio;

            pDispWin->height *= Hratio;
            if (pDispWin->height > new_panel_h) {
                pDispWin->height = new_panel_h;
            }

        }
    }
}

enum {
    MMAP_CACHED,
    MMAP_NONCACHED,
    MMAP_BOTH
};

#define INDEX_NOT_FOUND -1
static int vsync_bridge_buf_slot_get_index(const MS_DispFrameFormat *dff, int checkIdx)
{
    // !!! this function must in mutex protection !!!

    int i;
    for (i = 0; i < MAX_VSYNC_BRIDGE_DISPQ_NUM; i++) {
        if (sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].eBufState == MS_BS_IN_QUEUE) {
            if (!checkIdx) {
                return i;
            } else if (dff->u32UniqueId) {
                // has u32UniqueId, use it. for VDEC3 display, N stream use the same disp queue
                if (sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].u32UniqueId == dff->u32UniqueId) {
                    return i;
                }
            } else if (sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].u32UniqueId == dff->sFrames[0].u32Idx) {
                // no u32UniqueId, use vdec frame buffer index
                return i;
            }
        }
    }
    return INDEX_NOT_FOUND;
}

static void vsync_bridge_buf_slot_add_index(const MS_DispFrameFormat *dff)
{
    // !!! this function must in mutex protection !!!

    int i;
    for (i = 0; i < MAX_VSYNC_BRIDGE_DISPQ_NUM; i++) {
        if (sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].eBufState == MS_BS_NONE) {
            sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].eBufState = MS_BS_IN_QUEUE;
            sBufSlotInfo[dff->OverlayID ? 1 : 0].u8InQueueCnt++;
            if (dff->u32UniqueId) {
                sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].u32UniqueId = dff->u32UniqueId;
            } else {
                sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].u32UniqueId = dff->sFrames[0].u32Idx;
            }
            sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[i].u8CurIndex = dff->u8CurIndex;
            break;
        }
    }
}

static void vsync_bridge_buf_slot_clear(int id)
{
    // !!! this function must in mutex protection !!!

    memset(&sBufSlotInfo[id ? 1 : 0], 0, sizeof(sBufSlotInfo[id ? 1 : 0]));
}

static void MiuMapper_Init(void)
{
    const mmap_info_t *mmapInfo;
    int i;

    u32MiuStart[0] = 0;
    for (i = 1; i <= NUM_MIU; i++) {
        u32MiuStart[i] = 0xFFFFFFFF;
    }

    mmapInfo = mmap_get_info("MIU_INTERVAL");
    if (mmapInfo) {
        u32MiuStart[1] = mmapInfo->size;
    }
    mmapInfo = mmap_get_info("MIU_INTERVAL2");
    if (mmapInfo) {
        u32MiuStart[2] = mmapInfo->size;
    }
}

static MS_PHY MiuMapper_GetRelativeAddress(MS_PHY addr)
{
    int i;
    for (i = 0; i < NUM_MIU; i++)
    {
        if (addr < u32MiuStart[i + 1])
            break;
    }

    return addr - u32MiuStart[i];
}

static MS_U32 MiuMapper_GetMiuNo(MS_PHY addr)
{
    int i;
    for (i = 0; i < NUM_MIU; i++)
    {
        if (addr < u32MiuStart[i + 1])
            break;
    }

    return i;
}

static MS_PHY vsync_device_remap_range(char *range, MS_U32 *pu32MMAPSize, int cachedMode)
{
    const mmap_info_t *mmapInfo;
    MS_U32 u32TotalSize = 0;
    MS_PHY u32StartAddr = 0;

    mmapInfo = mmap_get_info(range);
    if (mmapInfo) {
        u32StartAddr = mmapInfo->addr;
        u32TotalSize = mmapInfo->size;

        if (MMAP_CACHED != cachedMode) {
            MsOS_MPool_Mapping(MiuMapper_GetMiuNo(u32StartAddr),
                MiuMapper_GetRelativeAddress(u32StartAddr), u32TotalSize, 1);
        }

        if (MMAP_NONCACHED != cachedMode) {
            MsOS_MPool_Mapping(MiuMapper_GetMiuNo(u32StartAddr),
                MiuMapper_GetRelativeAddress(u32StartAddr), u32TotalSize, 0);
        }
    }

    if (pu32MMAPSize) {
        *pu32MMAPSize = u32TotalSize;
    }
    ALOGI("%s: 0x%lx, 0x%lx, Start: 0x%lx, size: 0x%lx", range, u32MiuStart[1], u32MiuStart[2], u32StartAddr, u32TotalSize);
    return u32StartAddr;
}

void vsync_bridge_init(void)
{
    const mmap_info_t *mmapInfo;
    static MS_BOOL bIsInitalized = FALSE;

    if (bIsInitalized) {
        return;
    }
    bIsInitalized = TRUE;

    mmap_init();
    MDrv_SYS_GlobalInit();
    MiuMapper_Init();

    u32SHMAddr[0] = vsync_device_remap_range("E_MMAP_ID_VDEC_CPU", 0, MMAP_NONCACHED) + (VSYNC_BRIGE_SHM_ADDR);
    u32SHMAddr[1] = u32SHMAddr[0];
    u32DSAddr[0] = vsync_device_remap_range("E_MMAP_ID_XC_DS", &u32DSSize[0], MMAP_NONCACHED);
    u8MaxDSIdx[0] = (u32DSSize[0]/DS_BUFFER_DEPTH_EX/DS_COMMAND_BYTE_LEN);
    if (u8MaxDSIdx[0] > DS_BUFFER_NUM_EX)    u8MaxDSIdx[0] = DS_BUFFER_NUM_EX;

    u32DSAddr[1] = vsync_device_remap_range("E_MMAP_ID_XC_SUB_DS", &u32DSSize[1], MMAP_NONCACHED);
    u8MaxDSIdx[1] = (u32DSSize[1]/DS_BUFFER_DEPTH_EX/DS_COMMAND_BYTE_LEN);
    if (u8MaxDSIdx[1] > DS_BUFFER_NUM_EX)    u8MaxDSIdx[1] = DS_BUFFER_NUM_EX;

    u32FrameBufAddr = vsync_device_remap_range("E_MMAP_ID_VDEC_FRAMEBUFFER", 0, MMAP_BOTH);
    vsync_device_remap_range("E_MMAP_ID_VDEC_BITSTREAM", 0, MMAP_NONCACHED);
    u32SubFrameBufAddr = vsync_device_remap_range("E_MMAP_ID_VDEC_SUB_FRAMEBUFFER", 0, MMAP_BOTH);
    vsync_device_remap_range("E_MMAP_ID_VDEC_SUB_BITSTREAM", 0, MMAP_NONCACHED);
    vsync_device_remap_range("E_MMAP_ID_VDEC_SHARE_MEM", 0, MMAP_NONCACHED);

    vsync_bridge_buf_slot_clear(0);
    vsync_bridge_buf_slot_clear(1);

    MDrv_SYS_GlobalInit();

    XC_INITDATA XC_InitData;
    // xc not init.
    memset((void*)&XC_InitData, 0, sizeof(XC_InitData));
    MApi_XC_Init(&XC_InitData, 0);
    MApi_PNL_IOMapBaseInit();

    MS_PHY u32MLAddr;
    MS_U32 u32MLSize;
    u32MLAddr = vsync_device_remap_range("E_MMAP_ID_XC_MLOAD", &u32MLSize, MMAP_NONCACHED);
    MApi_XC_MLoad_Init(u32MLAddr, u32MLSize);

    memset(&ds_int_data, 0, sizeof(ds_internal_data_structure));

#ifdef UFO_MVOP_GET_IS_MVOP_AUTO_GEN_BLACK
    MVOP_Handle stMvopHd;
    MVOP_DrvMirror eMirrorMode = E_VOPMIRROR_NONE;
    stMvopHd.eModuleNum = E_MVOP_MODULE_MAIN;

    MDrv_MVOP_GetCommand(&stMvopHd, E_MVOP_CMD_GET_IS_MVOP_AUTO_GEN_BLACK, &bIsMVOPAutoGenBlack, sizeof(MS_BOOL));
#elif SUPPORT_HW_AUTO_GEN_BLACK_CHIP
    bIsMVOPAutoGenBlack = TRUE;
#else
    bIsMVOPAutoGenBlack = FALSE;
#endif
}

#if ENABLE_FRAME_CONVERT
void vsync_bridge_decide_frc_mode(MS_DispFrameFormat *dff, MS_U32 u32OutputFrmRate)
{
    MS_U32 u32FrameRate = dff->u32FrameRate;
    MS_U8 u8Id = dff->OverlayID ? 1 : 0;
    MS_U8 u8NextFrcMode = E_FRC_NORMAL;

    u8Interlace[u8Id] = dff->u8Interlace ? 1 : 0;

    if (u32OutputFrmRate) {
        if (dff->u8Interlace) {   // Only Interlace Mode
            if ( (u32FrameRate<<1) == u32OutputFrmRate) {   // Ex: 30x2 = 60i
                u8NextFrcMode = E_FRC_NORMAL;
            } else if (u32FrameRate == 60000) {   // Input 120i
                if (u32OutputFrmRate == 50000) {
                    // 120i -> 50i
                    u8NextFrcMode = E_FRC_120I_50I;
                } else if (u32OutputFrmRate == 60000) {
                    // 120i -> 60i
                    u8NextFrcMode = E_FRC_HALF_I;
                }
            } else if (u32FrameRate == 50000) {   // Input 100i
                if (u32OutputFrmRate == 60000) {
                    // 100i -> 60i
                    u8NextFrcMode = E_FRC_100I_60I;
                } else if (u32OutputFrmRate == 50000) {
                    // 100i -> 50i
                    u8NextFrcMode = E_FRC_HALF_I;
                }
            } else if (u32FrameRate >= 29970
                       && u32FrameRate <= 30000) {
                // 60i -> 50i
                if (u32OutputFrmRate == 50000) {
                    u8NextFrcMode = E_FRC_NTSC2PAL;
                } else if (u32OutputFrmRate == 30000) {
                    u8NextFrcMode = E_FRC_HALF_I;
                } else if (u32OutputFrmRate == 25000) {
                    u8NextFrcMode = E_FRC_120I_50I; // 30 fps interlace = 60i, 60i to 25i == 120i to 50i
                } else if (u32OutputFrmRate == 24000) {
                    u8NextFrcMode = E_FRC_30_24;
                }
            } else if (u32FrameRate == 25000) {   // Input 50i
                if (u32OutputFrmRate == 60000) {   // 50i -> 60i
                    u8NextFrcMode = E_FRC_PAL2NTSC;
                } else if ((u32OutputFrmRate == 30000)) {
                    u8NextFrcMode = E_FRC_25_30;
                }
            } else if (u32FrameRate >= 23970
                    && u32FrameRate <= 24000) {

                if (u32OutputFrmRate == 60000) {   // 48i -> 60i
                    u8NextFrcMode = E_FRC_32PULLDOWN;
                } else if (u32OutputFrmRate == 50000) {   // 48i -> 50i
                    u8NextFrcMode = E_FRC_NORMAL;
                } else if (u32OutputFrmRate == 30000) {   // 48i -> 30i
                     u8NextFrcMode = E_FRC_24_30;
                }
            }
        } else {   // Only Progressive Mode
            if (u32FrameRate == u32OutputFrmRate) {   // Ex: 25p = 25p
                u8NextFrcMode = E_FRC_NORMAL;
            } else if ((u32FrameRate << 1) == u32OutputFrmRate) {   // Ex: 25p x 2 = 50p
                u8NextFrcMode = E_FRC_DISP_2X;
            } else if ((u32FrameRate >> 1) == u32OutputFrmRate) {   // Ex: half p
                u8NextFrcMode = E_FRC_HALF_P;
            } else if (u32FrameRate >= 59900
                    && u32FrameRate <= 60000) {
                if (u32OutputFrmRate == 50000) {   // 60p -> 50p
                    u8NextFrcMode = E_FRC_60P_50P;
                } else if (u32OutputFrmRate == 30000) {
                    u8NextFrcMode = E_FRC_HALF_P;
                } else if (u32OutputFrmRate == 25000) {
                    u8NextFrcMode = E_FRC_60_25;
                } else if (u32OutputFrmRate == 24000) {
                    u8NextFrcMode = E_FRC_60_24;
                }
            } else if (u32FrameRate == 50000) {
                if (u32OutputFrmRate == 60000) {   // 50p -> 60p
                    u8NextFrcMode = E_FRC_50P_60P;
                } else if (u32OutputFrmRate == 30000) {   // 50p -> 30p
                    u8NextFrcMode = E_FRC_50_30;
                } else if (u32OutputFrmRate == 24000) {   // 50p -> 24p
                    u8NextFrcMode = E_FRC_50_24;
                }
            } else if (u32FrameRate >= 29970
                       && u32FrameRate <= 30000) {
                // 30p -> 50p
                if (u32OutputFrmRate == 50000) {
                    u8NextFrcMode = E_FRC_NTSC2PAL;
                } else if ((u32OutputFrmRate == 60000)) {
                    u8NextFrcMode = E_FRC_DISP_2X;
                } else if (u32OutputFrmRate == 25000) {
                    u8NextFrcMode = E_FRC_60P_50P;  // 30p -> 25p, 60p -> 50p, the same rule
                } else if (u32OutputFrmRate == 24000) {
                    u8NextFrcMode = E_FRC_30_24;
                }
            } else if (u32FrameRate == 25000) {
                // 25p -> 60p
                if (u32OutputFrmRate == 60000) {
                    u8NextFrcMode = E_FRC_PAL2NTSC;
                } else if ((u32OutputFrmRate == 30000)) {
                    u8NextFrcMode = E_FRC_25_30;
                }
            } else if (u32FrameRate >= 23970
                    && u32FrameRate <= 24000) {

                if (u32OutputFrmRate == 60000) {
                    u8NextFrcMode = E_FRC_32PULLDOWN;
                } else if (u32OutputFrmRate == 50000) {
                    u8NextFrcMode = E_FRC_24_50;
                } else if (u32OutputFrmRate == 30000) {   // 24p -> 30p
                    u8NextFrcMode = E_FRC_24_30;
                }
            }
        }
    }

    u8FrcMode[u8Id] = u8NextFrcMode;
    s8CvtValue[u8Id] = 0;
    u8CvtCount[u8Id] = 0;
    u8FrcDropMode[u8Id] = E_FRC_DROP_FRAME;

    if (dff->CodecType == MS_CODEC_TYPE_RV8
        || dff->CodecType == MS_CODEC_TYPE_RV9) {
        // rmvb is variable framerate, don't do FRC
        u8FrcMode[u8Id] = E_FRC_NORMAL;
    }

    ALOGD("u32FrameRate=%d, u32OutputFrmRate=%d, u8FrcMode[%d] = %d",
        (int)u32FrameRate, (int)u32OutputFrmRate, u8Id, u8FrcMode[u8Id]);
}

void vsync_bridge_convert_frame_rate(MS_U8 u8Id, DISP_FRM_INFO *dfi)
{
    MS_U8 interlace = u8Interlace[u8Id];
    MS_S8 frc_toggle = s8CvtValue[u8Id];

    if (u8FrcMode[u8Id] == E_FRC_NORMAL) return;

    switch (u8FrcMode[u8Id]) {
    case E_FRC_32PULLDOWN:
        if (interlace) { // 48I->60I
            if (frc_toggle == 0) {
                dfi->u8TB_toggle = 0;   //TBT
                dfi->u8Tog_Time  = 3;
            } else if (frc_toggle == 1) {
                dfi->u8TB_toggle = 1;   //BT
                dfi->u8Tog_Time  = 2;
            } else if (frc_toggle == 2) {
                dfi->u8TB_toggle = 1;   //BTB
                dfi->u8Tog_Time  = 3;
            } else {
                dfi->u8TB_toggle = 0;   //TB
                dfi->u8Tog_Time  = 2;
                frc_toggle = -1;
            }
        } else { // 24P -> 60P
            if (frc_toggle == 0) {
                dfi->u8Tog_Time = 3;
            } else {
                dfi->u8Tog_Time = 2;
                frc_toggle = -1;
            }
        }
        frc_toggle++;
        break;
    case E_FRC_PAL2NTSC:  // 25P->60P //50I->60I
        // 5 vsync repeat 1
        if (frc_toggle == 2) {
            dfi->u8Tog_Time = 3;
        } else if (frc_toggle == 4) {
            dfi->u8Tog_Time = 3;
            frc_toggle = -1;
        } else {
            dfi->u8Tog_Time = 2;
        }

        frc_toggle++;
        break;
    case E_FRC_NTSC2PAL:
        if (interlace) { //60I->50I
            if (u8FrcDropMode[u8Id] == E_FRC_DROP_FIELD) {
                frc_toggle += dfi->u8Tog_Time;

                // 5 field drop 1 field
                if (frc_toggle > 5) {
                    if (dfi->u8Tog_Time == 3) { // TBT or BTB
                        dfi->u8Tog_Time = 2;
                    } else {  // TB or BT
                        dfi->u8Tog_Time = 1;
                    }

                    frc_toggle -= 1;
                    frc_toggle %= 5;
                }
            }
        } else { // 30P->50P
            if (frc_toggle == 2) {
                frc_toggle = -1;
                dfi->u8Tog_Time = 1;
            } else {
                dfi->u8Tog_Time = 2;
            }

            frc_toggle++;
        }
        break;
    case E_FRC_DISP_2X: //double
        {
            dfi->u8Tog_Time <<= 1;
        }

        frc_toggle++;
        break;
    case E_FRC_30_24:
        if (dfi->u8Interlace) //60i to 24i
        {
            if (u8CvtCount[u8Id] % 2) {
                dfi->u8TB_toggle = 1;
            } else {
                dfi->u8TB_toggle = 0;
            }

            if (frc_toggle == 4)
            {
                dfi->u8Tog_Time = 0;
                frc_toggle = -1;

            } else {
                dfi->u8Tog_Time = 1;
            }

            u8CvtCount[u8Id] += dfi->u8Tog_Time;

        }
        else    // 30p to 24p
        {
            if (frc_toggle == 4) {
                dfi->u8Tog_Time = 0;
                frc_toggle = -1;
            }
        }
        frc_toggle++;

        break;
    case E_FRC_24_50: //24P->50P 48I->50I
        {
            dfi->u8Tog_Time = 2;
        }

        frc_toggle++;
        break;
    case E_FRC_50P_60P: //50P->60P
        if (frc_toggle == 4) {
            dfi->u8Tog_Time = 2;
            frc_toggle = -1;
        }

        frc_toggle++;
        break;
    case E_FRC_HALF_I:
        {
            if (frc_toggle == 0) {
                dfi->u8TB_toggle = 0; // T
                dfi->u8Tog_Time = 1;
            } else { // frc_toggle == 1
                dfi->u8TB_toggle = 1; // B
                dfi->u8Tog_Time = 1;
                frc_toggle = -1;
            }
            frc_toggle++;
        }
        break;
    case E_FRC_120I_50I:
        {
            if (frc_toggle == 5) {
                dfi->u8Tog_Time = 0; // drop this frame before insert to disaplay queue
                frc_toggle = -1;
            } else if (u8CvtCount[u8Id] == 0) {
                dfi->u8TB_toggle = 0; // T
                dfi->u8Tog_Time = 1;
                u8CvtCount[u8Id] = 1 ;
            } else if (u8CvtCount[u8Id] == 1) {
                dfi->u8TB_toggle = 1; // B
                dfi->u8Tog_Time = 1;
                u8CvtCount[u8Id] = 0;
            }
            frc_toggle++;
        }
        break;
    case E_FRC_100I_60I:
        {
            if (frc_toggle == 4) {
                // Display one frame
                frc_toggle = -1;
            } else if ((frc_toggle % 2) == 0) {
                dfi->u8TB_toggle = 0; // T
                dfi->u8Tog_Time = 1;
            } else if ((frc_toggle % 2) == 1) {
                dfi->u8TB_toggle = 1; // B
                dfi->u8Tog_Time = 1;
            }
            frc_toggle++;
        }
        break;


    case E_FRC_60_24: //60P->24P
        if (interlace) //120i to 24i
        {
            // not support
        }
        else    // 60p to 24p
        {
            if (frc_toggle == 0 || frc_toggle == 2)
            {
                dfi->u8Tog_Time = 1;
            }
            else
            {
                dfi->u8Tog_Time = 0;
            }

            if (frc_toggle == 4) {
                frc_toggle = -1;
            }
        }
        frc_toggle++;
        break;

    case E_FRC_60_25: //60P->25P
        if (interlace) {
            // not support
        } else {
            if (frc_toggle == 0 || frc_toggle == 2 || frc_toggle == 5
                || frc_toggle == 7 || frc_toggle == 10)
            {
                dfi->u8Tog_Time = 1;
            }
            else
            {
                dfi->u8Tog_Time = 0;
            }

            if (frc_toggle == 11) {
                frc_toggle = -1;
            }
        }
        frc_toggle++;
        break;

    case E_FRC_HALF_P: //half p
        if (interlace) {
            // not support
        } else {
            if (frc_toggle == 1)
            {
                dfi->u8Tog_Time = 0;
                frc_toggle = -1;
            }
            else
            {
                dfi->u8Tog_Time = 1;
            }
        }
        frc_toggle++;
        break;

    case E_FRC_60P_50P:
        if (interlace) {
            // not support
        } else {
            if (frc_toggle == 5)
            {
                dfi->u8Tog_Time = 0;
                frc_toggle = -1;
            }
            else
            {
                dfi->u8Tog_Time = 1;
            }
        }
        frc_toggle++;
        break;

    case E_FRC_50_30:
        if (interlace) {
            // not support
        } else {
            if (frc_toggle == 0 || frc_toggle == 2 || frc_toggle == 4)
            {
                dfi->u8Tog_Time = 1;
            }
            else
            {
                dfi->u8Tog_Time = 0;
            }

            if (frc_toggle == 4) {
                frc_toggle = -1;
            }
        }
        frc_toggle++;
        break;

    case E_FRC_50_24:
        if (interlace) {
            // not support
        } else {
            if ((frc_toggle & 0x01) == 0)
            {
                dfi->u8Tog_Time = 1;
            }
            else
            {
                dfi->u8Tog_Time = 0;
            }

            if (frc_toggle == 24) {
                dfi->u8Tog_Time = 0;
                frc_toggle = -1;
            }
        }
        frc_toggle++;
        break;


    default:
        frc_toggle++;
        break;
    }

    s8CvtValue[u8Id] = frc_toggle;
}

static MS_BOOL vsync_bridge_frc_drop(MS_U8 u8Id)
{
    if (u8FrcMode[u8Id] == E_FRC_60_25 || u8FrcMode[u8Id] == E_FRC_60_24
        || u8FrcMode[u8Id] == E_FRC_50_30 || u8FrcMode[u8Id] == E_FRC_50_24
        || u8FrcMode[u8Id] == E_FRC_HALF_P || u8FrcMode[u8Id] == E_FRC_60P_50P
        || u8FrcMode[u8Id] == E_FRC_30_24) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static void vsync_bridge_set_frc_mode(MS_DispFrameFormat *dff)
{
#ifdef UFO_MVOP_GET_IS_XC_GEN_TIMING
    // Get is XC gen timing or not, if XC gen timing, enable SW FRC
    MVOP_Handle stMvopHd;
    MS_BOOL bIsXCGenTiming = FALSE;

    if (dff->OverlayID == 0) {
        stMvopHd.eModuleNum = E_MVOP_MODULE_MAIN;
    } else {
        stMvopHd.eModuleNum = E_MVOP_MODULE_SUB;
    }

    MDrv_MVOP_GetCommand(&stMvopHd, E_MVOP_CMD_GET_IS_XC_GEN_TIMING, &bIsXCGenTiming, sizeof(bIsXCGenTiming));

    if (bIsXCGenTiming) {
        vsync_bridge_decide_frc_mode(dff, MApi_XC_GetOutputVFreqX100() * 10);
    } else {
        u8FrcMode[dff->OverlayID ? 1 : 0] = E_FRC_NORMAL;
    }
#else
#ifdef MSTAR_CLIPPERS
    vsync_bridge_decide_frc_mode(dff, g_IPanel.DefaultVFreq() * 100);
#else   // MSTAR_MONET or other
    MVOP_Timing stMVOPTiming;

    if (dff->OverlayID) {
        MDrv_MVOP_SubGetOutputTiming(&stMVOPTiming);
    } else {
        MDrv_MVOP_GetOutputTiming(&stMVOPTiming);
    }

    if (stMVOPTiming.u16ExpFrameRate != dff->u32FrameRate) {
        if (stMVOPTiming.bInterlace) {
            vsync_bridge_decide_frc_mode(dff, stMVOPTiming.u16ExpFrameRate * 2);
        } else {
            vsync_bridge_decide_frc_mode(dff, stMVOPTiming.u16ExpFrameRate);
        }
    } else {
        u8FrcMode[dff->OverlayID ? 1 : 0] = E_FRC_NORMAL;
    }
#endif
#endif
}
#endif

void vsync_bridge_open(MS_DispFrameFormat *dff, int panelW, int panelH)
{
    char value[PROPERTY_VALUE_MAX];
#if OPEN_LOG_BY_PROPERTY

    if (property_get("ms.vsync_bridge.log", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) {
        bMsLogON = TRUE;
#if BUILD_FOR_MI
    MI_VIDEO_SetDebugLevel(MI_DBG_ALL);
#endif
    }
#endif

#ifdef MSTAR_KAISER
    if (!dff->u8MCUMode) {
        Wrapper_Video_setMute(dff->OverlayID, 1, 0);
    }
#endif

    VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
    panelWidth[dff->OverlayID ? 1 : 0] = panelW;
    panelHeight[dff->OverlayID ? 1 : 0] = panelH;
    VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);

    u8MCUMode[dff->OverlayID ? 1 : 0] = dff->u8MCUMode;
    u8FieldCtrl[dff->OverlayID ? 1 : 0] = 0;

    MVOP_Handle stMvopHd;
    MVOP_DrvMirror eMirrorMode = E_VOPMIRROR_NONE;

    if (dff->OverlayID == 0) {
        stMvopHd.eModuleNum = E_MVOP_MODULE_MAIN;
    } else {
        stMvopHd.eModuleNum = E_MVOP_MODULE_SUB;
    }

    MDrv_MVOP_GetCommand(&stMvopHd, E_MVOP_CMD_GET_MIRROR_MODE, &eMirrorMode, sizeof(MVOP_DrvMirror));

    u8MirrorMode = (eMirrorMode == E_VOPMIRROR_NONE) ? 0 : eMirrorMode;

#ifdef UFO_MVOP_GET_IS_SW_CROP
    MDrv_MVOP_GetCommand(&stMvopHd, E_MVOP_CMD_GET_IS_SW_CROP, &bIsMVOPSWCrop, sizeof(MS_BOOL));
#endif

#if 0
    if (dff->OverlayID == 0) {
        VDEC_StreamId VdecStreamId;
        VdecStreamId.u32Version = dff->u32VdecStreamVersion;
        VdecStreamId.u32Id = dff->u32VdecStreamId;
        VDEC_EX_DynmcDispPath stDynmcDispPath;
        stDynmcDispPath.bConnect = TRUE;
        stDynmcDispPath.eMvopPath = E_VDEC_EX_DISPLAY_PATH_MVOP_MAIN;
        MApi_VDEC_EX_SetControl((VDEC_StreamId *)&VdecStreamId, E_VDEC_EX_USER_CMD_SET_DYNAMIC_DISP_PATH, (MS_U32)&stDynmcDispPath);
    }
#endif
    vsync_bridge_render_frame(dff, VSYNC_BRIDGE_INIT);
    VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
    u8Freeze[dff->OverlayID ? 1 : 0] = 0;
    bCapImmediate[dff->OverlayID ? 1 : 0] = 0;
    stFreezeMode[dff->OverlayID ? 1 : 0] = 0;
    u8VsyncBridgeFirstFire[dff->OverlayID ? 1 : 0] = 0;
    u8BobModeCnt[dff->OverlayID ? 1 : 0] = 0;
    u8BlankVideoMute[dff->OverlayID ? 1 : 0] = 0;

    //eARC_Type = E_AR_DEFAULT;
    u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0] = 1;
    u8Captured[dff->OverlayID ? 1 : 0] = 2;
    if (dff->u8SeamlessDS && Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
        u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 1;
    } else {
        u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 0;
    }
#if ENABLE_FRAME_CONVERT
    u8UpdateFrcMode[dff->OverlayID] = 1;
#endif

    pthread_create(&monitor_thread[dff->OverlayID ? 1 : 0], NULL, vsync_bridge_monitor_thread, dff);

    VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);

    ALOGD("panelWidth = %d, panelHeight = %d, [%d %d %d]", panelWidth[dff->OverlayID ? 1 : 0],
        panelHeight[dff->OverlayID ? 1 : 0], g_IPanel.Width(), g_IPanel.Height(), g_IPanel.DefaultVFreq());

    vsync_bridge_wait_disp_show(dff);    // wait R index to make sure video is show on disp

#ifdef MSTAR_KAISER
    if (!dff->u8MCUMode) {
        Wrapper_Video_setMute(dff->OverlayID, 0, 0);
    }
#endif
}

#ifdef SUPPORT_10BIT
static void vsync_bridge_set_mvop_10bit(MS_DispFrameFormat *dff, Vsync_Bridge_Mode eMode)
{
    MVOP_EVDFeature stEVDFeature;
    MVOP_Handle stMvopHd;

    stEVDFeature.bEnableEVD = ((dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat ==
        MS_COLOR_FORMAT_10BIT_TILE) || (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat ==
        MS_COLOR_FORMAT_HW_EVD)) ? TRUE : FALSE;

    if (dff->OverlayID == 0) {
        stMvopHd.eModuleNum = E_MVOP_MODULE_MAIN;
    } else {
        stMvopHd.eModuleNum = E_MVOP_MODULE_SUB;
    }

    if (eMode == VSYNC_BRIDGE_INIT) {
        if (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_10BIT_TILE) {
            stEVDFeature.eEVDBit[0] = E_MVOP_EVD_10BIT;
            stEVDFeature.eEVDBit[1] = E_MVOP_EVD_10BIT;
            u8Is10Bit[dff->OverlayID ? 1 : 0] = 1;
            MDrv_MVOP_SetCommand(&stMvopHd, E_MVOP_CMD_SET_EVD_FEATURE, &stEVDFeature);
        } else {
            u8Is10Bit[dff->OverlayID ? 1 : 0] = 0;
        }
    } else if ((eMode == VSYNC_BRIDGE_UPDATE) && u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]) {
        // check if change between 10_bit ans non-10_bit
        if (!u8Is10Bit[dff->OverlayID ? 1 : 0]
            && (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_10BIT_TILE)) {
            // non-10 bit to 10 bit
            stEVDFeature.eEVDBit[0] = E_MVOP_EVD_10BIT;
            stEVDFeature.eEVDBit[1] = E_MVOP_EVD_10BIT;
            u8Is10Bit[dff->OverlayID ? 1 : 0] = 1;
            MDrv_MVOP_SetCommand(&stMvopHd, E_MVOP_CMD_SET_EVD_FEATURE, &stEVDFeature);
        } else if (u8Is10Bit[dff->OverlayID ? 1 : 0]
            && (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat != MS_COLOR_FORMAT_10BIT_TILE)) {
            // 10 bit to non-10 bit
            stEVDFeature.eEVDBit[0] = E_MVOP_EVD_8BIT;
            stEVDFeature.eEVDBit[1] = E_MVOP_EVD_8BIT;
            u8Is10Bit[dff->OverlayID ? 1 : 0] = 0;
            MDrv_MVOP_SetCommand(&stMvopHd, E_MVOP_CMD_SET_EVD_FEATURE, &stEVDFeature);
        }
    }
}
#endif

void vsync_bridge_close(unsigned char Overlay_ID, MS_DispFrameFormat *dff)
{
    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff)*sizeof(MCU_DISPQ_INFO)));

    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0);
    frame_done_callback[Overlay_ID ? 1 : 0] = NULL;

    if (pSHM) {
#ifdef SUPPORT_10BIT
        // FIXME: SW Patch for Monet MVOP 10 bit HW auto gen black bug
        if (lastdff[Overlay_ID].sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_10BIT_TILE) {
            lastdff[Overlay_ID].sFrames[MS_VIEW_TYPE_CENTER].eColorFormat = MS_COLOR_FORMAT_HW_EVD;
            vsync_bridge_set_mvop_10bit(&lastdff[Overlay_ID], VSYNC_BRIDGE_UPDATE);
        }
#endif
        pSHM->u8McuDispSwitch = 0;
        memset(pSHM, 0, sizeof(MCU_DISPQ_INFO));
        MsOS_FlushMemory();
    } else {
        ALOGE("vsync_bridge_close, pSHM is NULL!");
    }

    vsync_bridge_buf_slot_clear(Overlay_ID ? 1 : 0);

    MApi_XC_FreezeImg(FALSE, (SCALER_WIN)(Overlay_ID ? 1 : 0));
    u8Freeze[Overlay_ID ? 1 : 0] = 0;
    stFreezeMode[Overlay_ID ? 1 : 0] = 0;
    bCapImmediate[Overlay_ID ? 1 : 0] = 0;
    u8VsyncBridgeEnable[Overlay_ID ? 1 : 0] = 0;
    u8MCUMode[Overlay_ID ? 1 : 0] = 0;
    u8VsyncBridgeFirstFire[Overlay_ID ? 1 : 0] = 0;
    u8BobModeCnt[Overlay_ID ? 1 : 0] = 0;
    u8FieldCtrl[Overlay_ID ? 1 : 0] = 0;
    u8BlankVideoMute[Overlay_ID ? 1 : 0] = 0;
    if (DipHandle[Overlay_ID ? 1 : 0]) {
        DIP_DeInit(DipHandle[Overlay_ID ? 1 : 0]);
        DipHandle[Overlay_ID ? 1 : 0] = NULL;
    }
    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);
    pthread_join(monitor_thread[Overlay_ID ? 1 : 0], NULL);
}

#ifdef BLOCK_UNTIL_DISP_INIT_DONE
MS_BOOL vsync_bridge_wait_init_done(MS_DispFrameFormat *dff)
{
    unsigned int u32Cnt = 0;

    if (!u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0] && !dff->u8IsNoWaiting) {
        u32Cnt = 4000;
        ALOGD("Display Path INIT BEGIN");
        u32RollingTime[dff->OverlayID ? 1 : 0] = MsOS_GetSystemTime();
        u8Abort[dff->OverlayID ? 1 : 0] &= ~ABORT_WAIT_INIT_DONE;

        while (--u32Cnt) {
            if (u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]
                || (u8Abort[dff->OverlayID ? 1 : 0] & ABORT_WAIT_INIT_DONE)) {
                break;
            }
            usleep(1000);
        }
        ALOGD("Display Path INIT END: %d", u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0] != 0);
        ALOGD("[Time] display_init: %d, ref: %d", (MsOS_GetSystemTime() - u32RollingTime[dff->OverlayID ? 1 : 0]), TIME_DISPLAY_INIT_REF);
    } else if (u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0] && dff->u8FreezeThisFrame) {

        u32Cnt = 2000;
        ALOGD("Wait video captured BEGIN");
        u8Abort[dff->OverlayID ? 1 : 0] &= ~ABORT_WAIT_DISP_CAPTURE;

        if (dff->u8FreezeThisFrame == MS_FREEZE_MODE_VIDEO) {
            while (--u32Cnt) {
                if (!u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]
                    || (u8Abort[dff->OverlayID ? 1 : 0] & ABORT_WAIT_DISP_CAPTURE)) {
                    break;
                }
                usleep(1000);
            }
            if (bCapImmediate[dff->OverlayID ? 1 : 0]) {
                unsigned int DelayTime = 0;
                MVOP_Timing stMVOPTiming;

                MDrv_MVOP_GetOutputTiming(&stMVOPTiming);
                if (stMVOPTiming.u16ExpFrameRate) {
                    DelayTime = (1000000 / stMVOPTiming.u16ExpFrameRate);
                }
                // delay one more vsync time
                usleep((DelayTime * 1000));
            }
        } else {
            while (--u32Cnt) {
                if ((u8Captured[dff->OverlayID ? 1 : 0] == 0)
                    || (u8Abort[dff->OverlayID ? 1 : 0] & ABORT_WAIT_DISP_CAPTURE)) break;
                usleep(1000);
            }
        }
        ALOGD("Wait video captured END: %d", u8Captured[dff->OverlayID ? 1 : 0] == 0);
    }

    if (u32Cnt == 0) {
        return FALSE;
    }

    return TRUE;
}

void vsync_bridge_signal_abort_waiting(unsigned char Overlay_ID)
{
    u8Abort[Overlay_ID ? 1 : 0] = 0xFF;
}
#endif

void vsync_bridge_set_mvop(MS_DispFrameFormat *dff)
{
    MVOP_Result bresult;
    MVOP_InputCfg dc_param;
    MVOP_VidStat stMvopVidSt;
    XC_MUX_PATH_INFO PathInfo;
    MS_S16 s16PathId = -1;
    MS_FrameFormat *sFrame = &dff->sFrames[MS_VIEW_TYPE_CENTER];
    MS_U32 u32YBufAddr = sFrame->sHWFormat.u32LumaAddr;
    MS_U32 u32UVBufAddr = sFrame->sHWFormat.u32ChromaAddr;
    MS_U32 u32Width = sFrame->u32Width;
    MS_U32 u32Height = sFrame->u32Height;
    MS_U32 u32Pitch = sFrame->sHWFormat.u32LumaPitch;
    SCALER_WIN enWin = (SCALER_WIN)dff->OverlayID;

    if (enWin == MAIN_WINDOW) {
        ALOGD("MDrv_MVOP_Init() 0x%x", (int)u32YBufAddr);
        MDrv_MVOP_Init();
        MDrv_MVOP_Enable(FALSE);
        MDrv_MVOP_MiuSwitch(MiuMapper_GetMiuNo(u32YBufAddr));

    } else {
        ALOGD("MDrv_MVOP_SubInit() 0x%x", (int)u32YBufAddr);
        MDrv_MVOP_SubInit();
        MDrv_MVOP_SubEnable(FALSE);
        MDrv_MVOP_SubMiuSwitch(MiuMapper_GetMiuNo(u32YBufAddr));
    }

    u32YBufAddr = MiuMapper_GetRelativeAddress(u32YBufAddr);
    u32UVBufAddr = MiuMapper_GetRelativeAddress(u32UVBufAddr);

    memset(&dc_param, 0, sizeof(MVOP_InputCfg));
    memset(&stMvopVidSt, 0, sizeof(MVOP_VidStat));

    if (sFrame->eColorFormat == MS_COLOR_FORMAT_YUYV) {
        dc_param.u16HSize = u32Width;
        dc_param.u16VSize = u32Height;
        dc_param.u32YOffset = u32YBufAddr;
        dc_param.u32UVOffset = u32UVBufAddr;
        dc_param.bSD = 1;
        dc_param.bYUV422 = 1;
        dc_param.bProgressive = 1;//dff->u8Interlace ? 0 : 1;
        dc_param.bUV7bit = 0;
        dc_param.bDramRdContd = 1;
        dc_param.bField = 0;
        dc_param.b422pack = 1;
        dc_param.u16StripSize = u32Pitch;
    } else {
        dc_param.u16HSize = u32Width;
        dc_param.u16VSize = u32Height;
        dc_param.u32YOffset = u32YBufAddr;
        dc_param.u32UVOffset = u32UVBufAddr;
        dc_param.bSD = 1;
        dc_param.bYUV422 = 0;
        dc_param.bProgressive = 1;//dff->u8Interlace ? 0 : 1;
        dc_param.bUV7bit = 0;
        dc_param.bDramRdContd = 0;
        dc_param.bField = 0;
        dc_param.b422pack = 0;
        dc_param.u16StripSize = u32Pitch;
    }

    if (enWin == MAIN_WINDOW) {
        if (dff->CodecType == MS_CODEC_TYPE_MVC) {
            MVOP_BaseAddInput tBaseAddInput;
            MVOP_Handle tMVOPHandle;
            tMVOPHandle.eModuleNum = E_MVOP_MODULE_MAIN;
            tBaseAddInput.eView = E_MVOP_2ND_VIEW;
            tBaseAddInput.u32YOffset = MiuMapper_GetRelativeAddress(dff->sFrames[1].sHWFormat.u32LumaAddr);
            tBaseAddInput.u32UVOffset = MiuMapper_GetRelativeAddress(dff->sFrames[1].sHWFormat.u32ChromaAddr);
            MDrv_MVOP_SetCommand(&tMVOPHandle, E_MVOP_CMD_SET_BASE_ADD_MULTI_VIEW, (void *)&tBaseAddInput);
            // Need to call E_MVOP_CMD_SET_FIRE_MVOP because MDrv_MVOP_SetInputCfg will do this
            bresult = MDrv_MVOP_SetInputCfg(MVOP_INPUT_DRAM_3DLR, &dc_param);
        } else {
            bresult = MDrv_MVOP_SetInputCfg(MVOP_INPUT_DRAM, &dc_param);
        }
        ALOGD("MDrv_MVOP_SetInputCfg() with result=%d", bresult);
    } else {
        bresult = MDrv_MVOP_SubSetInputCfg(MVOP_INPUT_DRAM, &dc_param);
        ALOGD("MDrv_MVOP_SubSetInputCfg() with result=%d", bresult);
    }

    stMvopVidSt.u16HorSize = u32Width;
    stMvopVidSt.u16VerSize = u32Height;
    if (dff->u32FrameRate == 0) {
        ALOGD("Error FrameRate!!! Set u16FrameRate to 30000");
        stMvopVidSt.u16FrameRate = 30000;
    } else {
        stMvopVidSt.u16FrameRate = dff->u32FrameRate;
    }

    stMvopVidSt.u8AspectRate = dff->u8AspectRate;
    stMvopVidSt.u8Interlace  = 0;//dff->u8Interlace ? 1 : 0;

    ALOGD("stMvopVidSt: %d %d %d %d %d %d %d",
          stMvopVidSt.u16HorSize,
          stMvopVidSt.u16VerSize,
          stMvopVidSt.u16FrameRate,
          stMvopVidSt.u8AspectRate,
          stMvopVidSt.u8Interlace,
          stMvopVidSt.u16HorOffset,
          stMvopVidSt.u16VerOffset);

    if (enWin == MAIN_WINDOW) {
        bresult = MDrv_MVOP_SetOutputCfg(&stMvopVidSt, FALSE);
        ALOGD("MDrv_MVOP_SetOutputCfg() with result=%d!", bresult);

        MDrv_MVOP_EnableBlackBG();
        MDrv_MVOP_Enable(TRUE);
    } else {
        bresult = MDrv_MVOP_SubSetOutputCfg(&stMvopVidSt, FALSE);
        ALOGD("MDrv_MVOP_SubSetOutputCfg() with result=%d", bresult);

        MDrv_MVOP_SubEnableBlackBG();
        MDrv_MVOP_SubEnable(TRUE);
    }

}

static MS_BOOL vsync_bridge_SupportDS(unsigned char Overlay_ID, MS_DispFrameFormat *dff)
{
    MS_BOOL ret = FALSE;
    int Current3DMode = 0;
    int CurrentPIPMode = 0;
    if (dff == NULL) return ret;

    if (Wrapper_get_Status(Overlay_ID) & E_STATE_3D) {
        Current3DMode = 1;
    }
    if (Wrapper_get_Status(Overlay_ID) & (E_STATE_POP | E_STATE_PIP)) {
        CurrentPIPMode = 1;
    }

#ifdef ENABLE_DYNAMIC_SCALING
    MS_BOOL bDisableDS = FALSE;
    char value[PROPERTY_VALUE_MAX];

    if (property_get("ms.disable.ds", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) {
        bDisableDS = TRUE;
    }

#ifndef ENABLE_SUB_DYNAMIC_SCALING
    if (property_get("mstar.enable.sub.ds", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) {
        // enable sub DS by setprop
    } else if (Overlay_ID == 1) {
        // disable sub DS
        bDisableDS = TRUE;
    }
#endif
    if (
        !bDisableDS
#ifndef ENABLE_INTERLACE_DS
        && !dff->u8Interlace
#endif
#if defined(MSTAR_NAPOLI)
        && !((g_IPanel.Width() == 3840)
            && (u8VsyncBridgeEnable[1]))    // main win can not support DS if enable sub win in NAPOLI chip set
#endif
#if defined(MSTAR_CLIPPERS)
        // This is a SW patch for clippers interlace mm play shaking when open DS by HW issue.
        && !(dff->u8Interlace)
#endif
        && !((dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width > DS_MAX_WIDTH) || (dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height > DS_MAX_HEIGHT))
        && !(Current3DMode)
        && !(CurrentPIPMode)
        && !(dff->u8MCUMode)
        && !(dff->u8ApplicationType & MS_APP_TYPE_GAME)
        && (dff->CodecType != MS_CODEC_TYPE_MVC)) {
        ret = TRUE;
    }
#endif
    return ret;
}

static int vsync_bridge_set_DS(MCU_DISPQ_INFO *pSHM, MS_DispFrameFormat *dff)
{
    if ((pSHM == NULL) || (dff == NULL)) return -1;

    ds_init_structure ds_init_info;
    memset(&ds_int_data[dff->OverlayID], 0, sizeof(ds_internal_data_structure));
    ds_curr_index[dff->OverlayID].u8DSIndex = 0;
    ds_next_index[dff->OverlayID] = 0;

#ifdef MSTAR_NIKE
    ds_init_info.ChipID = DS_NIKE;
#elif defined(MSTAR_NAPOLI)
    ds_init_info.ChipID = DS_NAPOLI;
#elif defined(MSTAR_MADISON)
    ds_init_info.ChipID = DS_MADISON;
#elif defined(MSTAR_MONACO)
    ds_init_info.ChipID = DS_MONACO;
#elif defined(MSTAR_KAISER)
    ds_init_info.ChipID = DS_KAISER;
#elif defined(MSTAR_CLIPPERS)
    ds_init_info.ChipID = DS_CLIPPERS;
#elif defined(MSTAR_MUJI)
    ds_init_info.ChipID = DS_MUJI;
#else
    ds_init_info.ChipID = DS_EDISON;
#endif

    ds_init_info.MaxWidth = DS_MAX_WIDTH;
    ds_init_info.MaxHeight = DS_MAX_HEIGHT;
    ds_init_info.pDSIntData = (ds_internal_data_structure *)&ds_int_data[dff->OverlayID];
    ds_init_info.pDSBufBase = (unsigned char *)MsOS_PA2KSEG1(u32DSAddr[dff->OverlayID]);
    ds_init_info.DSBufSize = u32DSSize[dff->OverlayID];
    ds_init_info.pDSXCData = (unsigned char *)pSHM + ((SUPPORT_HW_DISPLAY_MAX - vsync_bridge_GetSHMIndex(dff)) * sizeof(MCU_DISPQ_INFO));
    ds_init_info.u8EnhanceMode = DS_ENHANCE_MODE_RESET_DISP_WIN;
    ds_init_info.DS_Depth = vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUFFER_DEPTH);
    memset(ds_init_info.pDSXCData, 0, sizeof(ds_xc_data_structure));

    if (bMsLogON) ALOGD("[%d] ds_init_info.pDSXCData = 0x%p-0x%x ds_init_info.DSBufSize = 0x%d", dff->OverlayID, ds_init_info.pDSXCData, MsOS_PA2KSEG1(ds_init_info.pDSXCData),ds_init_info.DSBufSize);

    if (IS_XC_SUPPORT_SWDS) {
        ds_init_info.pDSIntData->bIsInitialized = 1;
        ds_init_info.pDSIntData->pDSXCData = (ds_xc_data_structure *) ds_init_info.pDSXCData;
    } else {
        dynamic_scale_init(&ds_init_info);
    }

    return 0;
}

#define vsync_bridge_seq_change(id, dff, forceDS) \
{   \
    VSYNC_BRIDGE_UNLOCK(id);    \
    Wrapper_seq_change(id, dff, forceDS);   \
    VSYNC_BRIDGE_LOCK(id);    \
}

#define vsync_bridge_video_set_mute(id, mute, lock_mute) \
{   \
    VSYNC_BRIDGE_UNLOCK(id);    \
    Wrapper_Video_setMute(id, mute, lock_mute);   \
    VSYNC_BRIDGE_LOCK(id);    \
}

void vsync_bridge_render_frame(MS_DispFrameFormat *dff, Vsync_Bridge_Mode eMode)
{
    UPDATE_SHM_ADDR;

    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff)*sizeof(MCU_DISPQ_INFO)));
    Vsync_Bridge_Mode eModeBak = eMode;

#if HANDLE_CROP_FIRST
    dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width
                                                - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
    dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight = 0;
    dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height
                                                - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;
    dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom = 0;
#endif

    // video wall

    MS_BOOL bVideoMute = FALSE;
    MS_BOOL bSeqChange = FALSE;
    MS_BOOL bVdecReset = FALSE;
    MS_U32 lumaAddr = MiuMapper_GetRelativeAddress(dff->sFrames[MS_VIEW_TYPE_CENTER].sHWFormat.u32LumaAddr);
    MS_U32 chromAddr = MiuMapper_GetRelativeAddress(dff->sFrames[MS_VIEW_TYPE_CENTER].sHWFormat.u32ChromaAddr);
    MS_U32 lumaAddr2 = MiuMapper_GetRelativeAddress(dff->sFrames[1].sHWFormat.u32LumaAddr);
    MS_U32 chromAddr2 = MiuMapper_GetRelativeAddress(dff->sFrames[1].sHWFormat.u32ChromaAddr);
    MS_U16 u16Width = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width
                      - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft
                      - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
    MS_U16 u16Height = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height
                       - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop
                       - dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;


    if (u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]
        && (u8MCUMode[dff->OverlayID ? 1 : 0] != dff->u8MCUMode)) {

        ALOGD("MCU mode not match!");
        return;
    }

    if (dff->u8MCUMode) {

        if (eMode == VSYNC_BRIDGE_INIT) {
            u16SrcWidth[dff->OverlayID] = (MS_U16)dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width;
            u16SrcHeight[dff->OverlayID] = (MS_U16)dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height;
            u32CropLeft[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft;
            u32CropRight[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
            u32CropTop[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop;
            u32CropBottom[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;
        }
        VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);

         if (!u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]) {
            ALOGD("MCU vsync_bridge_render_frame: [%d]switch not on !", dff->OverlayID);
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
            return;
        }

        if (bMsLogON) {
            ALOGD("MCU render_frame [%d] pts = %lld, luma=0x%x, chroma=0x%x, B_F = %d, Time = %d, Interlace = %d, Frc = %d",
                  dff->OverlayID, dff->u64Pts, (unsigned int)lumaAddr, (unsigned int)chromAddr, dff->u8BottomFieldFirst,
                  dff->u8ToggleTime, dff->u8Interlace, dff->u8FrcMode);
        }

        dff->u8CurIndex = 0;

        // source resolution change
        if ((u16SrcWidth[dff->OverlayID] != dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width)
            || (u16SrcHeight[dff->OverlayID] != dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height)) {

            u16SrcWidth[dff->OverlayID] = (MS_U16)dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width;
            u16SrcHeight[dff->OverlayID] = (MS_U16)dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height;
            u32CropLeft[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft;
            u32CropRight[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
            u32CropTop[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop;
            u32CropBottom[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;
            vsync_bridge_seq_change(dff->OverlayID, dff, 0);
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
            return;
        }

        if ((dff->CodecType == MS_CODEC_TYPE_MVC) && (dff->OverlayID == 0)) {
            MVOP_BaseAddInput tBaseAddInput;
            MVOP_Handle tMVOPHandle;
            MS_BOOL     bEnable = TRUE;

            tMVOPHandle.eModuleNum = E_MVOP_MODULE_MAIN;
            tBaseAddInput.eView = E_MVOP_MAIN_VIEW;
            tBaseAddInput.u32YOffset = lumaAddr;
            tBaseAddInput.u32UVOffset = chromAddr;
            MDrv_MVOP_SetCommand(&tMVOPHandle, E_MVOP_CMD_SET_BASE_ADD_MULTI_VIEW, (void *)&tBaseAddInput);
            tBaseAddInput.eView = E_MVOP_2ND_VIEW;
            tBaseAddInput.u32YOffset = lumaAddr2;
            tBaseAddInput.u32UVOffset = chromAddr2;
            MDrv_MVOP_SetCommand(&tMVOPHandle, E_MVOP_CMD_SET_BASE_ADD_MULTI_VIEW, (void *)&tBaseAddInput);
            MDrv_MVOP_SetCommand(&tMVOPHandle, E_MVOP_CMD_SET_FIRE_MVOP, (void *)&bEnable);
        } else {
            if (dff->OverlayID == 0) {
                MDrv_MVOP_SetBaseAdd(lumaAddr, chromAddr, 1, 0);
            } else {
                MDrv_MVOP_SubSetBaseAdd(lumaAddr, chromAddr, 1, 0);
            }
        }

        if (dff->u8ApplicationType & MS_APP_TYPE_IMAGE) {
            // update image real region without black area to SN for 3D auto detect
            Wrapper_SurfaceVideoSizeChange(dff->OverlayID,
                dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft,
                dff->sFrames[MS_VIEW_TYPE_CENTER].u32CropTop,
                u16Width,
                u16Height,
                0x55AA, 0x55AA, 1);
        }

        VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
        return;
    }


    VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);

    if (u8Rolling[dff->OverlayID ? 1 : 0]) {
#ifdef MSTAR_CLIPPERS
        if (dff->u8Interlace)
            dff->u8SeamlessDS = 0;  // This is a SW patch for clippers interlace mm play shaking when open DS by HW issue.
#endif
    }

    if (pSHM && u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]) {

        if ((pSHM->u8McuDispSwitch == 0)
            && u8VsyncBridgeFirstFire[dff->OverlayID ? 1 : 0]) {
            bVdecReset = bSeqChange = TRUE;
            if (u8Rolling[dff->OverlayID ? 1 : 0]) {
                ALOGD("[Time] display_get_new_frame - prepare to set mvop & xc -->u8SeamlessDS %d", dff->u8SeamlessDS);
                u32RollingTime[dff->OverlayID ? 1 : 0] = MsOS_GetSystemTime();
            }
            if (stHDRInfo[dff->OverlayID ? 1 : 0].u32FrmInfoExtAvail) {
                Wrapper_SetHdrMetadata((int)dff->OverlayID, &dff->sHDRInfo);
                memcpy(&stHDRInfo[dff->OverlayID ? 1 : 0], &dff->sHDRInfo, sizeof(MS_HDRInfo));
            } else {
            memset(&stHDRInfo[dff->OverlayID ? 1 : 0], 0, sizeof(MS_HDRInfo));
            }

            Vsync_Bridge_SWITCH_TYPE bEnableDS = VSYNC_BRIDGE_NONE;
            Vsync_Bridge_SWITCH_TYPE bEnableSeamlessDS = VSYNC_BRIDGE_NONE;
            if (u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] && !dff->u8SeamlessDS) {
                bEnableSeamlessDS = VSYNC_BRIDGE_OFF;
            } else if (!u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] && dff->u8SeamlessDS) {
                bEnableSeamlessDS = VSYNC_BRIDGE_ON;
            }
            if (Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0) && !vsync_bridge_SupportDS(dff->OverlayID ? 1 : 0, dff)) {
                bEnableDS = VSYNC_BRIDGE_OFF;
            } else if (!Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0) && vsync_bridge_SupportDS(dff->OverlayID ? 1 : 0, dff)) {
                bEnableDS = VSYNC_BRIDGE_ON;
            }
            if ((bEnableDS != VSYNC_BRIDGE_NONE) || (u8Interlace[dff->OverlayID] != dff->u8Interlace)) {  // I/P switch
                if ((!u8Rolling[dff->OverlayID ? 1 : 0]) && (stFreezeMode[dff->OverlayID ? 1 : 0] != MS_FREEZE_MODE_VIDEO)) {
                    bVideoMute = TRUE;
                    vsync_bridge_video_set_mute(dff->OverlayID, 1, 1);
                }
                if (u8Interlace[dff->OverlayID] != dff->u8Interlace) dff->u8SeamlessDS = 0;  // need to reset XC
            }
            if (bMsLogON) ALOGD("Need change seamlessDS %d EnableDS %d preInterlace %d now %d", bEnableSeamlessDS, bEnableDS, u8Interlace[dff->OverlayID], dff->u8Interlace);

            if ((u8Rolling[dff->OverlayID ? 1 : 0]) || (bVideoMute) || (stFreezeMode[dff->OverlayID ? 1 : 0] == MS_FREEZE_MODE_VIDEO)) {   // I/P switch
                if (u8Freeze[dff->OverlayID ? 1 : 0]) {
                    u8Freeze[dff->OverlayID ? 1 : 0] = 0;
                    MApi_XC_FreezeImg(FALSE, (SCALER_WIN)(dff->OverlayID ? 1 : 0));
                }
            }

            int bForceDS = 0;
            int bSeamlessPlay = 0;
            if (bEnableDS == VSYNC_BRIDGE_ON) { // enable DS
                vsync_bridge_set_DS(pSHM, dff);
                bForceDS = 1;
                dff->u8SeamlessDS = 0;
                if (bEnableSeamlessDS == VSYNC_BRIDGE_ON)
                    u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 1;
                else if (bEnableSeamlessDS == VSYNC_BRIDGE_OFF)
                    u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 0;
            } else if (bEnableDS == VSYNC_BRIDGE_OFF) { // disable DS
                Wrapper_Video_closeDS(dff->OverlayID, 0, 1);
                u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 0;
                dff->u8SeamlessDS = 0;
            } else {    // keep the same DS state
                MS_BOOL IsSameFrameRate= FALSE;
                if ((!dff->u8Interlace) && Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
                    IsSameFrameRate = TRUE;
                } else if (u16FrameRate[dff->OverlayID] == dff->u32FrameRate) {
                    IsSameFrameRate = TRUE;
                }
                if ((CodecType[dff->OverlayID] == dff->CodecType) && IsSameFrameRate
                    && (u8Interlace[dff->OverlayID] == dff->u8Interlace)
                    && !Wrapper_Video_need_seq_change(dff->OverlayID ? 1 : 0,
                    dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width,
                    dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height)) {

                    if (Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
                        // DS mode can handle width / height changed case.
                        bSeqChange = FALSE;
                    } else if ((u16SrcWidth[dff->OverlayID] == dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width)
                        && (u16SrcHeight[dff->OverlayID] == dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height)) {
                        // non-DS, do not seq change if the width and height is the same. Otherwise , need to be set seq change.
                        bSeqChange = FALSE;
                    }
                }
                if (Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
                    if (bEnableSeamlessDS == VSYNC_BRIDGE_ON)
                        u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 1;
                    else if (bEnableSeamlessDS == VSYNC_BRIDGE_OFF)
                        u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 0;
                    else
                        bSeamlessPlay = 1;

                    if (bMsLogON) ALOGD("Keep DS");

#if UPDATE_VIDEO_INFO
                    MS_VideoInfo vInfo;
                    MS_FrameFormat stFrame = dff->sFrames[MS_VIEW_TYPE_CENTER];
                    if (dff->CodecType == MS_CODEC_TYPE_MVC) {
                        vInfo.u16HorSize = stFrame.u32Width - stFrame.u32CropRight - stFrame.u32CropLeft;
                        vInfo.u16VerSize = stFrame.u32Height - stFrame.u32CropBottom - stFrame.u32CropTop;
                        vInfo.u16CropRight = 0;
                        vInfo.u16CropLeft = 0;
                        vInfo.u16CropBottom = 0;
                        vInfo.u16CropTop = 0;
                    } else {
                        vInfo.u16HorSize = stFrame.u32Width;
                        vInfo.u16VerSize = stFrame.u32Height;
                        vInfo.u16CropRight = stFrame.u32CropRight;
                        vInfo.u16CropLeft = stFrame.u32CropLeft;
                        vInfo.u16CropBottom = stFrame.u32CropBottom;
                        vInfo.u16CropTop = stFrame.u32CropTop;
                    }
                    Wrapper_UpdateModeSize(dff->OverlayID, vInfo.u16HorSize, vInfo.u16VerSize, vInfo.u16CropLeft, vInfo.u16CropRight, vInfo.u16CropTop, vInfo.u16CropBottom);
#endif
                } else {
                    dff->u8SeamlessDS = 0;
                    u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 0;
                    if (bMsLogON) ALOGD("Keep Non-DS");
                }
            }

            if (bSeamlessPlay && !bVideoMute) {
                if (u8Freeze[dff->OverlayID ? 1 : 0]) u8Freeze[dff->OverlayID ? 1 : 0]++;
            }
            if (bSeqChange) {
                if (!(Wrapper_get_Status(dff->OverlayID ? 1 : 0) & E_STATE_3D) &&
                    ((dff->CodecType == MS_CODEC_TYPE_MVC) && (dff->u83DMode != MS_3DMODE_SIDEBYSIDE))) {
                        VSYNC_BRIDGE_UNLOCK(dff->OverlayID);
                        Wrapper_Enable3D(dff->OverlayID ? 1 : 0);
                } else if ((Wrapper_get_Status(dff->OverlayID ? 1 : 0) & E_STATE_3D) &&
                    !((dff->CodecType == MS_CODEC_TYPE_MVC) && (dff->u83DMode != MS_3DMODE_SIDEBYSIDE))) {
                    VSYNC_BRIDGE_UNLOCK(dff->OverlayID);
                    Wrapper_Disable3D(dff->OverlayID ? 1 : 0);
                }

                dff->u8SeamlessDS = 0;
                if (!bVideoMute) {
                    vsync_bridge_video_set_mute(dff->OverlayID, 1, 1);
                    bVideoMute = TRUE;
                }
                vsync_bridge_seq_change(dff->OverlayID, dff, bForceDS);
                if (IS_XC_SUPPORT_SWDS && (u16FrameRate[dff->OverlayID] != dff->u32FrameRate)) {
                    // framerate change needs update HV/start
                    MApi_XC_SetCaptureWindowHstart(dff->OverlayID ?
                        MDrv_MVOP_SubGetHStart() : MDrv_MVOP_GetHStart(),
                        (SCALER_WIN)dff->OverlayID);
                    MApi_XC_SetCaptureWindowVstart(dff->OverlayID ?
                        MDrv_MVOP_SubGetVStart() : MDrv_MVOP_GetVStart(),
                        (SCALER_WIN)dff->OverlayID);
                }
            }
            ALOGD("[Time] display_set_mvop&xc: %d", (MsOS_GetSystemTime() - u32RollingTime[dff->OverlayID ? 1 : 0]));
            eMode = VSYNC_BRIDGE_INIT;
        } else if (u8Interlace[dff->OverlayID] != dff->u8Interlace) {
            if (stFreezeMode[dff->OverlayID ? 1 : 0] != MS_FREEZE_MODE_VIDEO) {
                bVideoMute = TRUE;
                vsync_bridge_video_set_mute(dff->OverlayID, 1, 1);
            }
            // switch off first
            pSHM->u8McuDispSwitch = 0;
            dff->u8SeamlessDS = 0;
            bSeqChange = TRUE;
            MsOS_FlushMemory();

            vsync_bridge_seq_change(dff->OverlayID, dff, 0);
            eMode = VSYNC_BRIDGE_INIT;
        } else if (Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
            if ((dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width > DS_MAX_WIDTH)
                || (dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height > DS_MAX_HEIGHT)
                || (Wrapper_Video_need_seq_change(dff->OverlayID ? 1 : 0,
                    dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width,
                    dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height))) {
                // Size change exceed DS_MAX_WIDTH or DS_MAX_HEIGHT, disable DS
                if (stFreezeMode[dff->OverlayID ? 1 : 0] != MS_FREEZE_MODE_VIDEO) {
                    bVideoMute = TRUE;
                    vsync_bridge_video_set_mute(dff->OverlayID, 1, 1);
                }
                u8SeamlessDSEnable[dff->OverlayID ? 1 : 0] = 0;

                // switch off first
                pSHM->u8McuDispSwitch = 0;
                dff->u8SeamlessDS = 0;
                bSeqChange = TRUE;
                MsOS_FlushMemory();
                if (!vsync_bridge_SupportDS(dff->OverlayID ? 1 : 0, dff)) {
                    Wrapper_Video_closeDS(dff->OverlayID, 0, 1);
                }
                vsync_bridge_seq_change(dff->OverlayID, dff, 0);

                // clear dispQ
                memset(pSHM, 0, sizeof(MCU_DISPQ_INFO));
                eMode = VSYNC_BRIDGE_INIT;
            }
        } else {
            // source resolution change and DS not enable
            if ((u16SrcWidth[dff->OverlayID] != dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width)
                || (u16SrcHeight[dff->OverlayID] != dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height)) {
                u16SrcWidth[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width;
                u16SrcHeight[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height;
                u8SizeChange[dff->OverlayID] = 1;
                int ForceDS = 0;

                if (stFreezeMode[dff->OverlayID ? 1 : 0] != MS_FREEZE_MODE_VIDEO) {
                    bVideoMute = TRUE;
                    vsync_bridge_video_set_mute(dff->OverlayID, 1, 1);
                }
                // switch off first
                pSHM->u8McuDispSwitch = 0;
                dff->u8SeamlessDS = 0;              // If DS not enable, Reset XC Window
                MsOS_FlushMemory();
                if (vsync_bridge_SupportDS(dff->OverlayID ? 1 : 0, dff)) {
                    ForceDS = 1;
                }
                vsync_bridge_seq_change(dff->OverlayID, dff, ForceDS);
                bSeqChange = TRUE;

                // clear dispQ
                memset(pSHM, 0, sizeof(MCU_DISPQ_INFO));
                eMode = VSYNC_BRIDGE_INIT;
            }
        }
    }
    if (bCapImmediate[dff->OverlayID ? 1 : 0] && stFreezeMode[dff->OverlayID ? 1 : 0] == 0) {
        bVideoMute = TRUE;
    }

    if (bVideoMute) ALOGD("Video Mute ON");

    if (pSHM) {

        MS_U8 u8WIndex = pSHM->u8McuDispQWPtr;
        MS_U32 u32Cnt = 10;

#ifdef SUPPORT_10BIT
        vsync_bridge_set_mvop_10bit(dff, eMode);
#endif

        if (eMode == VSYNC_BRIDGE_INIT) {
            ALOGD("VSYNC_BRIDGE_INIT clear dispQ");
            memset(pSHM, 0, sizeof(MCU_DISPQ_INFO));
            vsync_bridge_buf_slot_clear(dff->OverlayID ? 1 : 0);

            u8WIndex = DISPQ_FIRST_R_INDEX - 1;
            u8Captured[dff->OverlayID ? 1 : 0] = 2;
            u16SrcWidth[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width;
            u16SrcHeight[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height;
            u16FrameRate[dff->OverlayID] = (MS_U16)dff->u32FrameRate;
            CodecType[dff->OverlayID] = dff->CodecType;
            u8Interlace[dff->OverlayID] = dff->u8Interlace;
            if (bSeqChange && dff->sHDRInfo.u32FrmInfoExtAvail) {
                memcpy(&stHDRInfo[dff->OverlayID ? 1 : 0], &dff->sHDRInfo, sizeof(MS_HDRInfo));
            }

            pSHM->u2MirrorMode = u8MirrorMode;

            if (MApi_XC_IsCurrentRequest_FrameBufferLessMode() || MApi_XC_IsCurrentFrameBufferLessMode()) {
                pSHM->u1FBLMode = 1;
            } else {
                pSHM->u1FBLMode = 0;
            }

            if (dff->u8FrcMode == MS_FRC_32PULLDOWN) {
                pSHM->u8DisableFDMask = 1;
            } else if (!dff->u8Interlace) {
                pSHM->u8DisableFDMask = 1;
            }

#if ENABLE_FRAME_CONVERT
            u8UpdateFrcMode[dff->OverlayID] = 1;
#endif

            if (eModeBak == VSYNC_BRIDGE_INIT) {
                VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
                return;
            }
        } else if ((eMode == VSYNC_BRIDGE_UPDATE) && (!u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0])) {
            ALOGD("vsync_bridge_render_frame: [%d]switch not on !", dff->OverlayID);
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
            return;
        }

        if (lumaAddr == 0) {
            ALOGD("Invalid DispQ !!!");
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
            return;
        }

        if (dff->u8FrcMode == MS_FRC_32PULLDOWN) {
            pSHM->u8DisableFDMask = 1;
        }

        if (stHDRInfo[dff->OverlayID ? 1 : 0].u32FrmInfoExtAvail != dff->sHDRInfo.u32FrmInfoExtAvail
            || memcmp(&stHDRInfo[dff->OverlayID ? 1 : 0].stColorDescription, &dff->sHDRInfo.stColorDescription, sizeof(MS_ColorDescription))
            || memcmp(&stHDRInfo[dff->OverlayID ? 1 : 0].stMasterColorDisplay, &dff->sHDRInfo.stMasterColorDisplay, sizeof(MS_MasterColorDisplay))) {
            // HDR info changed
            Wrapper_SetHdrMetadata((int)dff->OverlayID, &dff->sHDRInfo);
            memcpy(&stHDRInfo[dff->OverlayID ? 1 : 0], &dff->sHDRInfo, sizeof(MS_HDRInfo));
        }

        if (dff->u8FreezeThisFrame && !u8Freeze[dff->OverlayID ? 1 : 0]) {

            unsigned int u32Cnt = 100;
            while(--u32Cnt && pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8Tog_Time != 0) {
                usleep(10000);
            }

            if (dff->u8FreezeThisFrame == MS_FREEZE_MODE_BLANK_VIDEO) {

                u8BlankVideoMute[dff->OverlayID ? 1 : 0] = 1;
                vsync_bridge_video_set_mute(dff->OverlayID ? 1 : 0, 1, 0);

#ifdef SUPPORT_10BIT
                // FIXME: SW Patch for Monet MVOP 10 bit HW auto gen black bug
                if (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_10BIT_TILE) {
                    dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat = MS_COLOR_FORMAT_HW_EVD;
                    vsync_bridge_set_mvop_10bit(dff, eMode);
                }
#endif

                // u8FreezeThisFrame is MS_FREEZE_MODE_BLANK_VIDEO
                // set u8McuDispSwitch to generate black video
                pSHM->u8McuDispSwitch = 0;
                MsOS_FlushMemory();
                VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
                return;
            }

            ALOGD("[%d]MApi_XC_FreezeImg, u8FreezeThisFrame = %d", dff->OverlayID, dff->u8FreezeThisFrame);
            MApi_XC_FreezeImg(TRUE, (SCALER_WIN)(dff->OverlayID ? 1 : 0));
            bCapImmediate[dff->OverlayID ? 1 : 0] = FALSE;
            if (MApi_XC_IsFreezeImg((SCALER_WIN)(dff->OverlayID ? 1 : 0)) == FALSE) {   // This is for hw limitation
                bCapImmediate[dff->OverlayID ? 1 : 0] = TRUE;
            }
            u8Freeze[dff->OverlayID ? 1 : 0] = 2;
            u8Captured[dff->OverlayID ? 1 : 0] = 2;

            Dip_Output_Info dipOutput = {E_FMT_ARGB8888, 0, 0, NULL, NULL};
            Dip_ErrType dipError;

            int x, y, width, height;

            MS_WINDOW CaptureCropWin;
            MS_WINDOW CaptureDispWin;
            CaptureDispWin = OutputWin[dff->OverlayID];

            vsync_bridge_CalAspectRatio(&CaptureDispWin, &CaptureCropWin, dff, TRUE, TRUE, 0, 0);
            x = CaptureDispWin.x;
            y = CaptureDispWin.y;
            width = CaptureDispWin.width;
            height = CaptureDispWin.height;

            stFreezeMode[dff->OverlayID ? 1 : 0] = dff->u8FreezeThisFrame;
            if (dff->u8FreezeThisFrame == MS_FREEZE_MODE_VIDEO) {
	            if (DIP_InitCaptureOneFrame(&DipHandle[dff->OverlayID ? 1 : 0], dipOutput, x, y, width, height,  OutputWin[dff->OverlayID].width,  OutputWin[dff->OverlayID].height) == E_OK) {
	                if (bCapImmediate[dff->OverlayID ? 1 : 0]) {
	                    DIP_CaptureOneFrame(DipHandle[dff->OverlayID ? 1 : 0], &width, &height, NULL, TRUE, 0);
	                }
	            } else {
	                if (bCapImmediate[dff->OverlayID ? 1 : 0]) {    // both of freeze and capture was failed, need to mute
	                    vsync_bridge_video_set_mute(dff->OverlayID ? 1 : 0, 1, 0);
                        u8BlankVideoMute[dff->OverlayID ? 1 : 0] = 1;
	                }
	                stFreezeMode[dff->OverlayID ? 1 : 0] = 0;
	                ALOGE("Error: capture dip init fail");
	            }

	            if (!bCapImmediate[dff->OverlayID ? 1 : 0]) {
	                memset(pSHM, 0, sizeof(MCU_DISPQ_INFO));
	                MsOS_FlushMemory();
	            }
	#ifdef MSTAR_CLIPPERS
	            else {
	                stFreezeMode[dff->OverlayID ? 1 : 0] = 0;
	            }
	#endif
            }
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);

            return;
        }


#if ENABLE_FRAME_CONVERT
        if (u16FrameRate[dff->OverlayID] != (MS_U16)dff->u32FrameRate) {
            u8UpdateFrcMode[dff->OverlayID] = 1;
            u16FrameRate[dff->OverlayID] = dff->u32FrameRate;
        }
        if (u8UpdateFrcMode[dff->OverlayID]) {
            vsync_bridge_set_frc_mode(dff);
            u8UpdateFrcMode[dff->OverlayID] = 0;
        }
#endif

        if (bMsLogON)
            ALOGD("render_frame [%d] pts = %lld, luma=0x%x, chroma=0x%x, Interlace = %d, B_F = %d, TG = %d, Last_TG = %d, FieldCtrl:%d u8SeamlessDS %d",
                  dff->OverlayID, dff->u64Pts, (unsigned int)lumaAddr, (unsigned int)chromAddr, dff->u8Interlace,
                  dff->u8BottomFieldFirst, dff->u8ToggleTime, pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8Tog_Time, dff->u8FieldCtrl, dff->u8SeamlessDS);

        while ((((u8WIndex + 1)%MAX_VSYNC_BRIDGE_DISPQ_NUM) == pSHM->u8McuDispQRPtr) && u32Cnt--) {
            ALOGE("Display Queue Full !! Waiting... [u32SHMAddr = 0x%x]", (int)u32SHMAddr[vsync_bridge_GetSHMIndex(dff)]);
            //usleep(1000);
        }

        if (dff->u8LowLatencyMode && pSHM->u8McuDispSwitch
            && (pSHM->u8McuDispQWPtr != pSHM->u8McuDispQRPtr)) {

            ALOGD("render_frame [%d] drop pts = %lld, luma=0x%x, chroma=0x%x, Interlace = %d, B_F = %d, TG = %d, Last_TG = %d, FieldCtrl:%d",
                dff->OverlayID, dff->u64Pts, (unsigned int)lumaAddr, (unsigned int)chromAddr, dff->u8Interlace,
                dff->u8BottomFieldFirst, dff->u8ToggleTime, pSHM->McuDispQueue[pSHM->u8McuDispQWPtr].u8Tog_Time, dff->u8FieldCtrl);
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
            return;
        }

        u8WIndex = (u8WIndex + 1)%MAX_VSYNC_BRIDGE_DISPQ_NUM;

        pSHM->McuDispQueue[u8WIndex].u32LumaAddr0 = lumaAddr;
        pSHM->McuDispQueue[u8WIndex].u32ChromaAddr0 = chromAddr;

        if (dff->CodecType == MS_CODEC_TYPE_MVC) {
            #if defined(MSTAR_EDISON) || defined(MSTAR_KAISER)
            pSHM->McuDispQueue[u8WIndex].u16Width = (u16Width + 15) & ~15;
            #else
            pSHM->McuDispQueue[u8WIndex].u16Width = (u16Width + 1) & ~1;
            #endif
            pSHM->McuDispQueue[u8WIndex].u16Height = u16Height;
        } else {
            #if defined(MSTAR_EDISON) || defined(MSTAR_KAISER)
            pSHM->McuDispQueue[u8WIndex].u16Width = (dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width + 15) & ~15;
            #else
            pSHM->McuDispQueue[u8WIndex].u16Width = (dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width + 1) & ~1;
            #endif
            pSHM->McuDispQueue[u8WIndex].u16Height = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height;
        }

        pSHM->McuDispQueue[u8WIndex].u32Idx = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Idx;
        pSHM->McuDispQueue[u8WIndex].u32PriData = dff->sFrames[MS_VIEW_TYPE_CENTER].u32PriData;


        pSHM->McuDispQueue[u8WIndex].u16Pitch = dff->sFrames[MS_VIEW_TYPE_CENTER].sHWFormat.u32LumaPitch;
        pSHM->McuDispQueue[u8WIndex].u32CodecType = dff->CodecType;
        pSHM->McuDispQueue[u8WIndex].u8Interlace = dff->u8Interlace ? 1 : 0;

        if (u8BobModeCnt[dff->OverlayID] || (u8FieldCtrl[dff->OverlayID] != dff->u8FieldCtrl)) {
                ALOGD("DISP_ONE_FIELD = 0x%d", (dff->u8FieldCtrl ? TRUE : FALSE));
                VDEC_StreamId VdecStreamId;
                VdecStreamId.u32Version = dff->u32VdecStreamVersion;
                VdecStreamId.u32Id = dff->u32VdecStreamId;

                ALOGD("MApi_VDEC_EX_SetControl @1 DISP_ONE_FIELD 0x%d", dff->u8FieldCtrl);
                if (dff->u8FieldCtrl) {
                    MApi_VDEC_EX_SetControl((VDEC_StreamId *) &VdecStreamId, E_VDEC_EX_USER_CMD_DISP_ONE_FIELD, TRUE);
                } else {
                    MApi_VDEC_EX_SetControl((VDEC_StreamId *) &VdecStreamId, E_VDEC_EX_USER_CMD_DISP_ONE_FIELD, FALSE);
                }
        }

        if (u8FieldCtrl[dff->OverlayID] != dff->u8FieldCtrl) {
            // enable BOB mode to prevent FF/RF playback De-interlace garbage.
            if (!IS_XC_SUPPORT_SWDS && ds_int_data[dff->OverlayID].bIsInitialized) {
                ds_xc_data_structure *pDSXCData = ds_int_data[dff->OverlayID].pDSXCData;
                // set 1 to trigger update DS command table.
                u8SizeChange[dff->OverlayID] = 1;
                pDSXCData->u8BobMode = (dff->u8FieldCtrl ? TRUE : FALSE);
            } else {
                if(dff->u8FieldCtrl) {
                    if (!u8BobModeCnt[dff->OverlayID]) {
                        // Do not enable twice bob mode inner magic frames
                        int ret = MApi_XC_SetBOBMode(TRUE, dff->OverlayID);
                        ALOGD("MApi_XC_SetBOBMode @1 ret:%d enable:%d win:%d", ret, TRUE,dff->OverlayID);
                    }
                    u8BobModeCnt[dff->OverlayID] = 1;
                }
            }
            u8FieldCtrl[dff->OverlayID] = dff->u8FieldCtrl;
        }

        if (u8BobModeCnt[dff->OverlayID] && (!dff->u8FieldCtrl)) {
            u8BobModeCnt[dff->OverlayID]++;
            if (u8BobModeCnt[dff->OverlayID] > WAIT_BOBMODE_FRAME) {
                int ret = MApi_XC_SetBOBMode(FALSE, dff->OverlayID);
                u8BobModeCnt[dff->OverlayID] = 0;
				ALOGD("MApi_XC_SetBOBMode @1 ret:%d enable:%d win:%d", ret, FALSE,dff->OverlayID);
            }
        }

        pSHM->McuDispQueue[u8WIndex].u8FieldCtrl = dff->u8FieldCtrl;

        if (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_YUYV) {
            pSHM->McuDispQueue[u8WIndex].u8ColorFormat = 1; //422
        } else if (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_10BIT_TILE) {
            pSHM->McuDispQueue[u8WIndex].u8ColorFormat = 2; //420 10 bits
            pSHM->McuDispQueue[u8WIndex].u16Pitch1 = dff->sFrames[1].sHWFormat.u32LumaPitch;
            pSHM->McuDispQueue[u8WIndex].u8RangeMapY = dff->sFrames[1].u8RangeMapY;
            pSHM->McuDispQueue[u8WIndex].u8RangeMapUV = dff->sFrames[1].u8RangeMapUV;
            pSHM->McuDispQueue[u8WIndex].u32LumaAddr1 = lumaAddr2;
            pSHM->McuDispQueue[u8WIndex].u32ChromaAddr1 = chromAddr2;
        } else if (dff->sFrames[MS_VIEW_TYPE_CENTER].eColorFormat == MS_COLOR_FORMAT_HW_32x32) {
            pSHM->McuDispQueue[u8WIndex].u8ColorFormat = 3; //420 (32x32)
        } else {
            pSHM->McuDispQueue[u8WIndex].u8ColorFormat = 0; //420
        }

        if (u8Interlace[dff->OverlayID] != dff->u8Interlace) {
            ALOGD("WARNING [%d] pts = %lld, luma=0x%x, chroma=0x%x, BoT_First = %d, TogTime = %d, Interlace = %d",
                  dff->OverlayID, dff->u64Pts, (unsigned int)lumaAddr, (unsigned int)chromAddr, dff->u8BottomFieldFirst,
                  dff->u8ToggleTime, dff->u8Interlace);
        }

        if (dff->CodecType == MS_CODEC_TYPE_VC1_ADV || dff->CodecType == MS_CODEC_TYPE_VC1_MAIN) {
            pSHM->McuDispQueue[u8WIndex].u8RangeMapY = dff->sFrames[MS_VIEW_TYPE_CENTER].u8RangeMapY;
            pSHM->McuDispQueue[u8WIndex].u8RangeMapUV = dff->sFrames[MS_VIEW_TYPE_CENTER].u8RangeMapUV;
        }

        if (dff->CodecType == MS_CODEC_TYPE_MVC) {
            pSHM->McuDispQueue[u8WIndex].u8FrameNum = 2;
            if ((u8MirrorMode == (MS_U8)E_MVOP_MIRROR_HVBOTH) && (dff->u83DMode == MS_3DMODE_SIDEBYSIDE)) {
                // MVOP mirror and MVC side by side, swap L R
                pSHM->McuDispQueue[u8WIndex].u32LumaAddr0 = lumaAddr2;
                pSHM->McuDispQueue[u8WIndex].u32ChromaAddr0 = chromAddr2;
                pSHM->McuDispQueue[u8WIndex].u32LumaAddr1 = lumaAddr;
                pSHM->McuDispQueue[u8WIndex].u32ChromaAddr1 = chromAddr;
            } else {
                pSHM->McuDispQueue[u8WIndex].u32LumaAddr1 = lumaAddr2;
                pSHM->McuDispQueue[u8WIndex].u32ChromaAddr1 = chromAddr2;
            }
        }

        if (dff->u8ToggleTime == 0) {
            pSHM->McuDispQueue[u8WIndex].u8TB_toggle = 0;
            pSHM->McuDispQueue[u8WIndex].u8Tog_Time = 1;

        } else if (dff->u8Interlace) {   //interlace
            pSHM->McuDispQueue[u8WIndex].u8TB_toggle = dff->u8BottomFieldFirst ? 1 : 0;    //0, TOP then BOTTOM, 1 BOT then TOP
            pSHM->McuDispQueue[u8WIndex].u8Tog_Time = dff->u8ToggleTime;

            if ((dff->u8ToggleTime == 1) && (dff->sFrames[MS_VIEW_TYPE_CENTER].eFieldType == MS_FIELD_TYPE_BOTH)) {
                // prevent de-interlace wrong...
                pSHM->McuDispQueue[u8WIndex].u8Tog_Time = 2;
            }
        } else {
                pSHM->McuDispQueue[u8WIndex].u8TB_toggle = 0;
                pSHM->McuDispQueue[u8WIndex].u8Tog_Time = 1;
        }

        pSHM->McuDispQueue[u8WIndex].u1BottomFieldFirst = dff->u8BottomFieldFirst;

        pSHM->McuDispQueue[u8WIndex].u2Luma0Miu = MiuMapper_GetMiuNo(dff->sFrames[0].sHWFormat.u32LumaAddr);
        pSHM->McuDispQueue[u8WIndex].u2Chroma0Miu = MiuMapper_GetMiuNo(dff->sFrames[0].sHWFormat.u32ChromaAddr);
        pSHM->McuDispQueue[u8WIndex].u2Luma1Miu = MiuMapper_GetMiuNo(dff->sFrames[1].sHWFormat.u32LumaAddr);
        pSHM->McuDispQueue[u8WIndex].u2Chroma1Miu = MiuMapper_GetMiuNo(dff->sFrames[1].sHWFormat.u32ChromaAddr);

#if ENABLE_FRAME_CONVERT
        if (vsync_bridge_frc_drop(dff->OverlayID ? 1 : 0)) {
            pSHM->u5FRCMode = E_FRC_NORMAL; // handle outside
            vsync_bridge_convert_frame_rate(dff->OverlayID, &pSHM->McuDispQueue[u8WIndex]);
            if (bMsLogON) {
                ALOGD("FRC u8Tog_Time = %d", pSHM->McuDispQueue[u8WIndex].u8Tog_Time);
            }
            if (pSHM->McuDispQueue[u8WIndex].u8Tog_Time == 0) {
                // drop !!!
                VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
                return;
            }
        } else {
            pSHM->u5FRCMode = u8FrcMode[dff->OverlayID];
        }
#endif

        if (dff->phyVsyncBridgeExtAddr) {
            // has vsync_bridge EXT addr
            MCU_DISPQ_INFO_EXT *pDispQ_Ext = (MCU_DISPQ_INFO_EXT *)MsOS_PA2KSEG1(dff->phyVsyncBridgeExtAddr +
                (vsync_bridge_GetSHMIndex(dff) * sizeof(MCU_DISPQ_INFO_EXT)));

            if (pDispQ_Ext && (pDispQ_Ext->u32Version <= VSYNC_BRIDGE_EXT_VERSION)) {
                // vsync bridge extened header --> vbeh magic number
                pDispQ_Ext->u8Pattern[0] = 'v';
                pDispQ_Ext->u8Pattern[1] = 'b';
                pDispQ_Ext->u8Pattern[2] = 'e';
                pDispQ_Ext->u8Pattern[3] = 'h';
                pDispQ_Ext->u32Version = VSYNC_BRIDGE_EXT_VERSION;
                memcpy(&pDispQ_Ext->McuDispQueue[u8WIndex], &dff->sDispFrmInfoExt, sizeof(DISP_FRM_INFO_EXT));
            } else {
                ALOGE("pDispQ_Ext is NULL");
            }
        }

        if (ds_int_data[dff->OverlayID].bIsInitialized && Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
            // source resolution change
            if ((u16SrcWidth[dff->OverlayID] != dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width)
                || (u16SrcHeight[dff->OverlayID] != dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height)) {
                u16SrcWidth[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Width;
                u16SrcHeight[dff->OverlayID] = dff->sFrames[MS_VIEW_TYPE_CENTER].u32Height;
                u8SizeChange[dff->OverlayID] = 1;
            }

            if (!pSHM->u8McuDispSwitch || u8SizeChange[dff->OverlayID]
                || ExternalWin[dff->OverlayID].u8SetFromExt || is_dolby_vision(dff)) {

                update_ds_table(dff, 0, TRUE, (!pSHM->u8McuDispSwitch || u8SizeChange[dff->OverlayID]
                    || ExternalWin[dff->OverlayID].u8SetFromExt));
                if ((ExternalWin[dff->OverlayID].u8SetFromExt) && (!u8SizeChange[dff->OverlayID])) {
                    ExternalWin[dff->OverlayID].u8SetFromExt = 0;
                } else {
                    u8SizeChange[dff->OverlayID] = 0;
                }
            }
            //ALOGD("vsync_bridge_render_frame pts = %lld, lumaAddr=0x%x, DS_Idx=0x%x, overlayID=%d", dff->u64Pts,lumaAddr, ds_curr_index, dff->OverlayID);
            pSHM->McuDispQueue[u8WIndex].u8DSIndex = ds_curr_index[dff->OverlayID].u8DSIndex;
        }

        pSHM->McuDispQueue[u8WIndex].u16CropLeft = CurrentCropWin[dff->OverlayID ? 1 : 0].x;
        pSHM->McuDispQueue[u8WIndex].u16CropTop = CurrentCropWin[dff->OverlayID ? 1 : 0].y;
        pSHM->McuDispQueue[u8WIndex].u16CropRight = pSHM->McuDispQueue[u8WIndex].u16Width
                                                    - CurrentCropWin[dff->OverlayID ? 1 : 0].width
                                                    - CurrentCropWin[dff->OverlayID ? 1 : 0].x;
        pSHM->McuDispQueue[u8WIndex].u16CropBottom = pSHM->McuDispQueue[u8WIndex].u16Height
                                                    - CurrentCropWin[dff->OverlayID ? 1 : 0].height
                                                    - CurrentCropWin[dff->OverlayID ? 1 : 0].y;

        pSHM->u8McuDispQWPtr = u8WIndex;
        dff->u8CurIndex = u8WIndex;

        int buf_slot = vsync_bridge_buf_slot_get_index(dff, true);
        if (buf_slot == INDEX_NOT_FOUND) {
            vsync_bridge_buf_slot_add_index(dff);
        } else {
            // the same frame in queue twice, update u8CurIndex
            ALOGD("the same frame in queue twice update u8WIndex:%d u8CurIndex:%d ", u8WIndex, dff->u8CurIndex);
            sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[buf_slot].u8CurIndex = dff->u8CurIndex;
        }

        if (pSHM->u8McuDispSwitch == 0) {
            pSHM->u8DispQueNum = MAX_VSYNC_BRIDGE_DISPQ_NUM;

            if (dff->u8Interlace) {
                pSHM->McuDispQueue[u8WIndex].u8FieldCtrl = 1;

                VDEC_StreamId VdecStreamId;
                VdecStreamId.u32Version = dff->u32VdecStreamVersion;
                VdecStreamId.u32Id = dff->u32VdecStreamId;

                ALOGD("MApi_VDEC_EX_SetControl @2 DISP_ONE_FIELD 0x%d", pSHM->McuDispQueue[u8WIndex].u8FieldCtrl);
                if (pSHM->McuDispQueue[u8WIndex].u8FieldCtrl) {
                    MApi_VDEC_EX_SetControl((VDEC_StreamId *) &VdecStreamId, E_VDEC_EX_USER_CMD_DISP_ONE_FIELD, TRUE);
                } else {
                    MApi_VDEC_EX_SetControl((VDEC_StreamId *) &VdecStreamId, E_VDEC_EX_USER_CMD_DISP_ONE_FIELD, FALSE);
                }

                // enable BOB mode to prevent FF/RF playback De-interlace garbage.
                if (!IS_XC_SUPPORT_SWDS && ds_int_data[dff->OverlayID].bIsInitialized
                    && Wrapper_Video_DS_State(dff->OverlayID ? 1 : 0)) {
                    ds_xc_data_structure *pDSXCData = ds_int_data[dff->OverlayID].pDSXCData;
                    // set 1 to trigger update DS command table.
                    u8SizeChange[dff->OverlayID] = 1;
                    pDSXCData->u8BobMode = (pSHM->McuDispQueue[u8WIndex].u8FieldCtrl ? TRUE : FALSE);
                } else {
                     int ret = MApi_XC_SetBOBMode(pSHM->McuDispQueue[u8WIndex].u8FieldCtrl ? TRUE : FALSE, dff->OverlayID);
					 ALOGD("MApi_XC_SetBOBMode @2 ret:%d enable:%d win:%d", ret, (pSHM->McuDispQueue[u8WIndex].u8FieldCtrl ? TRUE : FALSE),dff->OverlayID);
                     if (pSHM->McuDispQueue[u8WIndex].u8FieldCtrl) {
                        u8BobModeCnt[dff->OverlayID] = 1;
                    }
                }
                u8FieldCtrl[dff->OverlayID] = pSHM->McuDispQueue[u8WIndex].u8FieldCtrl;
            }

#ifdef MSTAR_KAISER
            if (dff->u8Interlace) {
                ALOGD("Interlace repeat last field due to FD_Mask delay count");
                pSHM->u8ToggleMethod = 1;
                pSHM->McuDispQueue[u8WIndex].u8Tog_Time = 6;
            }
#endif

            pSHM->u8McuDispSwitch = 1;
            u32FrameCount = 0;
            u8VsyncBridgeFirstFire[dff->OverlayID ? 1 : 0] = 1;

            MS_BOOL bFdMASKOFF = FALSE;

            if ((bVideoMute && (!u8Rolling[dff->OverlayID ? 1 : 0])) || (stFreezeMode[dff->OverlayID ? 1 : 0] == MS_FREEZE_MODE_VIDEO)
                || (bVideoMute && bCapImmediate[dff->OverlayID ? 1 : 0])) {
                if (!bIsMVOPAutoGenBlack) {
                    if (dff->u8Interlace) {
                        // False fdmask let XC to fill buffer with new setting, then un-mute
                        vsync_bridge_set_fdmask(dff, FALSE, FALSE);
                        bFdMASKOFF = TRUE;
                        // delay to fill XC buffer with new setting, then un-mute
                        if (g_IPanel.DefaultVFreq()) {
                            usleep(4 * 10000000 / g_IPanel.DefaultVFreq());
                        } else {
                            usleep(200000);
                        }
                    } else {
                        if (g_IPanel.DefaultVFreq()) {
                            usleep(1 * 10000000 / g_IPanel.DefaultVFreq());
                        } else {
                            usleep(200000);
                        }
                    }
                }
            }
            if ((bVideoMute) && (u8BlankVideoMute[dff->OverlayID ? 1 : 0] == 0)) {
                vsync_bridge_video_set_mute(dff->OverlayID, 0, 1);
                ALOGD("Video Mute OFF");
            }
                if (bFdMASKOFF) {
                    vsync_bridge_set_fdmask(dff, TRUE, FALSE);

            }

            stFreezeMode[dff->OverlayID ? 1 : 0] = 0;
            bCapImmediate[dff->OverlayID ? 1 : 0] = FALSE;
        }
        u32FrameCount++;
        if ((u8BlankVideoMute[dff->OverlayID ? 1 : 0])) {
            u8BlankVideoMute[dff->OverlayID ? 1 : 0]++;
        }
        if (dff != &lastdff[dff->OverlayID])
            memcpy(&lastdff[dff->OverlayID], dff, sizeof(MS_DispFrameFormat));

        MsOS_FlushMemory();

        if (u8Freeze[dff->OverlayID ? 1 : 0]) {
            u8Freeze[dff->OverlayID ? 1 : 0]--;
            if (u8Freeze[dff->OverlayID ? 1 : 0] == 0) {
                ALOGD("[%d]MApi_XC_UnFreezeImg", dff->OverlayID );
                MApi_XC_FreezeImg(FALSE, (SCALER_WIN)(dff->OverlayID ? 1 : 0));
            }

            if (!dff->u8Interlace) {
                pSHM->u8DisableFDMask = 1;
            } else {
                pSHM->u8DisableFDMask = 0;
            }
        }

        if ((u8BlankVideoMute[dff->OverlayID ? 1 : 0]) > WAIT_FRAMECNT_UNMUTE) {
            if (dff->u8Interlace) vsync_bridge_set_fdmask(dff, FALSE, FALSE);
            vsync_bridge_video_set_mute(dff->OverlayID, 0, 1);
            if (dff->u8Interlace) vsync_bridge_set_fdmask(dff, TRUE, FALSE);
            u8BlankVideoMute[dff->OverlayID ? 1 : 0] = 0;
        }
    } else {
        ALOGE("vsync_bridge_render_frame, pSHM is NULL!");
    }

    VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);

}


void vsync_bridge_wait_frame_done(MS_DispFrameFormat *dff)
{
    unsigned long u32Cnt = 0, u32Cnt2 = 0;
    volatile MCU_DISPQ_INFO *pSHM = (volatile  MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff) * sizeof(MCU_DISPQ_INFO)));

    VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
    int buf_slot = vsync_bridge_buf_slot_get_index(dff, true);
    if (buf_slot == INDEX_NOT_FOUND) {
        // no need to wait
        VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
        return;
    }
    dff->u8CurIndex = sBufSlotInfo[dff->OverlayID ? 1 : 0].sBufSlot[buf_slot].u8CurIndex;
    VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);

    if (dff->u32FrameRate) {
        u32Cnt = (1000000/dff->u32FrameRate)*4;
    } else {
        u32Cnt = 200;
    }

    if (bMsLogON)   u32Cnt2 = u32Cnt;

    if (pSHM && u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]) {

        if (dff->u8MCUMode) {

            MS_PHY u32YOffset = 0, u32UVOffset = 0;
            MS_PHY lumaAddr = MiuMapper_GetRelativeAddress(dff->sFrames[MS_VIEW_TYPE_CENTER].sHWFormat.u32LumaAddr);
            MS_PHY chromAddr = MiuMapper_GetRelativeAddress(dff->sFrames[MS_VIEW_TYPE_CENTER].sHWFormat.u32ChromaAddr);

            while (--u32Cnt) {
                if (dff->OverlayID == 0) {
                    MDrv_MVOP_GetBaseAdd(&u32YOffset, &u32UVOffset);
                } else {
                    MDrv_MVOP_SubGetBaseAdd(&u32YOffset, &u32UVOffset);
                }
                u32YOffset <<= 3;
                u32UVOffset <<= 3;
                if (lumaAddr != u32YOffset && chromAddr != u32UVOffset) {
                    break;
                }
                usleep(1000);
            }
        } else {
            while (--u32Cnt && ((pSHM->u8McuDispQRPtr == dff->u8CurIndex) || (pSHM->McuDispQueue[dff->u8CurIndex].u8Tog_Time))) {
                usleep(1000);
            }
        }

        if (!u32Cnt)   ALOGE("[%d]Wait frame done timeout!! ", dff->OverlayID);

        if (bMsLogON)
            ALOGD("Overlay[%d] W %d, R %d, Cur %d, disp_cnt = %d, wait time = %dms",dff->OverlayID,
                  pSHM->u8McuDispQWPtr, pSHM->u8McuDispQRPtr, dff->u8CurIndex,
                  pSHM->McuDispQueue[dff->u8CurIndex].u16DispCnt, (int)(u32Cnt2 - u32Cnt));
    } else if (!pSHM) {
        ALOGE("vsync_bridge_wait_frame_done pSHM is NULL");
    }

}

int vsync_bridge_is_open(MS_DispFrameFormat *dff)
{
    int value = 0;
    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff) * sizeof(MCU_DISPQ_INFO)));
    VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
    if (pSHM && u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]) {
        value = 1;
    }
    VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
    return value;
}


int vsync_bridge_ds_init(MS_DispFrameFormat *dff)
{
    int ret = -1;
    if (!u32DSAddr[dff->OverlayID] || !u32DSSize[dff->OverlayID]) {
        ALOGE("DS not support due to MIU setting!");
        return ret;
    }

    UPDATE_SHM_ADDR;

    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff) * sizeof(MCU_DISPQ_INFO)));

    if (!pSHM) {
        ALOGE("vsync_bridge_ds_init fail u32SHMAddr = 0x%x", (int)u32SHMAddr[vsync_bridge_GetSHMIndex(dff)]);
        return ret;
    }

    VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
    ret = vsync_bridge_set_DS(pSHM, dff);
    VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
    return ret;
}

void vsync_bridge_ds_deinit(unsigned char Overlay_ID)
{
    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0);
    memset(&ds_int_data[Overlay_ID], 0, sizeof(ds_internal_data_structure));
    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);
}

void vsync_bridge_set_dispinfo(unsigned char Overlay_ID, MS_WINDOW_RATIO newCropWinRatio, const MS_WinInfo *pVinfo)
{
    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0);

    OutputWin[Overlay_ID] = pVinfo->disp_win;
    // SrcCropWin only use when zoom mode as E_AR_DEFAULT
    SrcCropWinRatio[Overlay_ID] = newCropWinRatio;

    if (pVinfo->osdWidth && pVinfo->osdHeight) {
        u32OsdWidth = pVinfo->osdWidth;
        u32OsdHeight = pVinfo->osdHeight;
    }

    Wrapper_SetDispWin(Overlay_ID, OutputWin[Overlay_ID].x, OutputWin[Overlay_ID].y, OutputWin[Overlay_ID].width, OutputWin[Overlay_ID].height, NULL);
    if (ds_int_data[Overlay_ID].bIsInitialized) {
            u8SizeChange[Overlay_ID] = 1;
            if ((OutputWin[Overlay_ID].width) && (OutputWin[Overlay_ID].height)) {
                // it will work when pSHM->u8McuDispSwitch is ON !!
                vsync_bridge_ds_set_index(&lastdff[Overlay_ID], TRUE);
            }
    }

#if ENABLE_FRAME_CONVERT
    u8UpdateFrcMode[Overlay_ID ? 1 : 0] = 1;
#endif

    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);
}

void vsync_bridge_ds_set_ext_DipCropWin(stVBDispCrop* ptr)
{
    VSYNC_BRIDGE_LOCK(ptr->u8Win ? 1 : 0);
    ExternalWin[ptr->u8Win] = *ptr;
    if ((ExternalWin[ptr->u8Win].u16DispWidth) && (ExternalWin[ptr->u8Win].u16DispHeight)) {
        if (ds_int_data[ptr->u8Win].pDSXCData->u8MVOPMirror == (MS_U8)E_MVOP_MIRROR_HVBOTH) {
            MS_U16 u16Width = lastdff[ptr->u8Win].sFrames[MS_VIEW_TYPE_CENTER].u32Width
                              -  lastdff[ptr->u8Win].sFrames[MS_VIEW_TYPE_CENTER].u32CropLeft
                              -  lastdff[ptr->u8Win].sFrames[MS_VIEW_TYPE_CENTER].u32CropRight;
            MS_U16 u16Height =  lastdff[ptr->u8Win].sFrames[MS_VIEW_TYPE_CENTER].u32Height
                               -  lastdff[ptr->u8Win].sFrames[MS_VIEW_TYPE_CENTER].u32CropTop
                               -  lastdff[ptr->u8Win].sFrames[MS_VIEW_TYPE_CENTER].u32CropBottom;

            ExternalWin[ptr->u8Win].u16CropX = u16Width- (ExternalWin[ptr->u8Win].u16CropX + ExternalWin[ptr->u8Win].u16CropWidth);
            ExternalWin[ptr->u8Win].u16CropY = u16Height - (ExternalWin[ptr->u8Win].u16CropY + ExternalWin[ptr->u8Win].u16CropHeight);

            ExternalWin[ptr->u8Win].u16DispX = panelWidth[ptr->u8Win] - (ExternalWin[ptr->u8Win].u16DispX + ExternalWin[ptr->u8Win].u16DispWidth);
            ExternalWin[ptr->u8Win].u16DispY = panelHeight[ptr->u8Win] - (ExternalWin[ptr->u8Win].u16DispY + ExternalWin[ptr->u8Win].u16DispHeight);
        }
        ALOGD("EXT corp x: %d, y: %d, w: %d, h: %d, Disp x: %d, y: %d, w: %d, h: %d",
        ExternalWin[ptr->u8Win].u16CropX, ExternalWin[ptr->u8Win].u16CropY, ExternalWin[ptr->u8Win].u16CropWidth, ExternalWin[ptr->u8Win].u16CropHeight,
        ExternalWin[ptr->u8Win].u16DispX, ExternalWin[ptr->u8Win].u16DispY, ExternalWin[ptr->u8Win].u16DispWidth, ExternalWin[ptr->u8Win].u16DispHeight);
        ExternalWin[ptr->u8Win].u8SetFromExt = 1;

        OutputWin[ptr->u8Win].x = ExternalWin[ptr->u8Win].u16DispX;
        OutputWin[ptr->u8Win].y = ExternalWin[ptr->u8Win].u16DispY;
        OutputWin[ptr->u8Win].width = ExternalWin[ptr->u8Win].u16DispWidth;
        OutputWin[ptr->u8Win].height = ExternalWin[ptr->u8Win].u16DispHeight;
        eARC_Type = E_AR_DEFAULT;
        if (ds_int_data[ptr->u8Win].bIsInitialized) {
            vsync_bridge_ds_set_index(&lastdff[ptr->u8Win], TRUE);
        }
    }
    VSYNC_BRIDGE_UNLOCK(ptr->u8Win ? 1 : 0);
}

void vsync_bridge_ds_enable(unsigned char u8Enable, unsigned char Overlay_ID)
{
    XC_DynamicScaling_Info stDS_Info;
    memset(&stDS_Info, 0, sizeof(XC_DynamicScaling_Info));

    ALOGD("vsync_bridge_ds_enable [%d][%d]", Overlay_ID, u8Enable);
    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0);

    if (u8Enable) {
        stDS_Info.u32DS_Info_BaseAddr = u32DSAddr[Overlay_ID];
        stDS_Info.u8MIU_Select = MiuMapper_GetMiuNo(u32DSAddr[Overlay_ID]);
        stDS_Info.u8DS_Index_Depth    = DS_BUFFER_DEPTH_EX;
        stDS_Info.bOP_DS_On           = 1;
        stDS_Info.bIPS_DS_On          = 0;
        stDS_Info.bIPM_DS_On          = 1;

        ALOGD("u32DSAddr = 0x%x u32DSSize = 0x%x, u8MaxDSIdx = 0x%x", (unsigned int)stDS_Info.u32DS_Info_BaseAddr,
            (unsigned int)u32DSSize[Overlay_ID], u8MaxDSIdx[Overlay_ID]);

        MApi_XC_SetDynamicScaling(&stDS_Info, sizeof(XC_DynamicScaling_Info), (SCALER_WIN)Overlay_ID);
    } else {
        MApi_XC_SetDynamicScaling(&stDS_Info, sizeof(XC_DynamicScaling_Info), (SCALER_WIN)Overlay_ID);

        memset(&ds_int_data[Overlay_ID], 0, sizeof(ds_internal_data_structure));
    }

    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);
}

MS_U64 vsync_bridge_get_cmd(Vsync_Bridge_Get_Cmd eCmd)
{
    MS_U64 u64Rtn = 0;

    switch (eCmd) {
    case VSYNC_BRIDGE_GET_DISPQ_SHM_ADDR:
        u64Rtn = u32SHMAddr[0];
        break;
    case VSYNC_BRIDGE_GET_SUB_DISPQ_SHM_ADDR:
        u64Rtn = u32SHMAddr[1];
        break;
    case VSYNC_BRIDGE_GET_DS_BUF_ADDR:
        u64Rtn = u32DSAddr[0];
        break;
    case VSYNC_BRIDGE_GET_DS_BUF_SIZE:
        u64Rtn = u32DSSize[0];
        break;
    case VSYNC_BRIDGE_GET_DS_SUB_BUF_ADDR:
        u64Rtn = u32DSAddr[1];
        break;
    case VSYNC_BRIDGE_GET_DS_XC_ADDR:
        u64Rtn = u32SHMAddr[0] + 2*sizeof(MCU_DISPQ_INFO);
        break;
    case VSYNC_BRIDGE_GET_SUB_DS_XC_ADDR:
        u64Rtn = u32SHMAddr[1] + 2*sizeof(MCU_DISPQ_INFO);
        break;
    case VSYNC_BRIDGE_GET_MIU_INTERVAL:
        u64Rtn = u32MiuStart[1];
        break;
    case VSYNC_BRIDGE_GET_FRAME_BUF_ADDR:
        u64Rtn = u32FrameBufAddr;
        break;
    case VSYNC_BRIDGE_GET_SUB_FRAME_BUF_ADDR:
        u64Rtn = u32SubFrameBufAddr;
        break;
    case VSYNC_BRIDGE_GET_DS_BUFFER_DEPTH:
        if (IS_XC_SUPPORT_SWDS) {
            u64Rtn = DS_BUFFER_DEPTH_EX;
        } else {
            u64Rtn = DS_BUFFER_DEPTH;
        }
#ifdef MSTAR_KAISER
        u64Rtn = DS_BUFFER_DEPTH_EX;
#endif
        break;
    default:
        break;
    }
    return u64Rtn;
}

MS_U32 vsync_bridge_set_aspectratio(unsigned char Overlay_ID, Vsync_Bridge_ARC_Type enARCType)
{
    ALOGD("vsync_bridge_set_aspectratio [%d] %d",Overlay_ID, enARCType);

    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0)

    eARC_Type = enARCType;

    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);

    return 0;
}

MS_U32 vsync_bridge_ds_update_table(unsigned char Overlay_ID)
{
    ALOGD("vsync_bridge_ds_update_table [%d]",Overlay_ID);

    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0)

    if (ds_int_data[Overlay_ID].bIsInitialized) {
        u8SizeChange[Overlay_ID] = 1;
        vsync_bridge_ds_set_index(&lastdff[Overlay_ID], FALSE);
    }

    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);

    return 0;
}

int vsync_bridge_capture_video(unsigned char u8Mode, unsigned int OverlayID, int left, int top, int *width, int *height, int *crop_left, int *crop_top, int *crop_right, int *crop_bottom, void * data, int stride)
{
    int rtn = 0;

    VSYNC_BRIDGE_LOCK(OverlayID ? 1 : 0);
    if (!u8VsyncBridgeEnable[OverlayID ? 1 : 0] || !u8Captured[OverlayID ? 1 : 0]
        || (u8Mode == MS_FREEZE_MODE_FREEZE_ONLY) || (u8Mode == MS_FREEZE_MODE_FREEZE_MUTE)) {

        ALOGE("vsync_bridge_capture_video return fail %d %d %d",u8Mode,
            u8VsyncBridgeEnable[OverlayID ? 1 : 0], u8Captured[OverlayID ? 1 : 0]);
        u8Abort[OverlayID ? 1 : 0] |= ABORT_WAIT_DISP_CAPTURE;
        u8Captured[OverlayID ? 1 : 0] = 0;
        VSYNC_BRIDGE_UNLOCK(OverlayID ? 1 : 0);
        return -1;
    }

    *crop_left = OutputWin[OverlayID ? 1 : 0].x;
    *crop_top = OutputWin[OverlayID ? 1 : 0].y;
    *crop_right = OutputWin[OverlayID ? 1 : 0].x + OutputWin[OverlayID ? 1 : 0].width;
    *crop_bottom = OutputWin[OverlayID ? 1 : 0].y + OutputWin[OverlayID ? 1 : 0].height;

    if (u8Mode == MS_FREEZE_MODE_BLANK_VIDEO) {
        if (data == NULL) {
            *crop_left = 0;
            *crop_top = 0;
            *crop_right = 1;
            *crop_bottom = 1;
            *width = 1;
            *height = 1;
        } else {
            memset(data, 0, 4);  // black AGRB8888
        }
        rtn = 0;
    } else {
        // trigger to caputre
        if (DipHandle[OverlayID ? 1 : 0]) {
            if (data) {
                if(!bCapImmediate[OverlayID ? 1 : 0]) {
                    // Dip hw capture
                    rtn = DIP_CaptureOneFrame(DipHandle[OverlayID ? 1 : 0], width, height, data, TRUE, stride);
                }
                // copy to mali
                rtn = DIP_CaptureOneFrame(DipHandle[OverlayID ? 1 : 0], width, height, data, FALSE, stride);
            } else {
                 // data is NULL, jsut report width / height
                 rtn = DIP_CaptureOneFrame(DipHandle[OverlayID ? 1 : 0], width, height, data, FALSE, stride);
            }
            *crop_left = 0;
            *crop_top = 0;
            *crop_right = *width;
            *crop_bottom = *height;
            ALOGD("CaptureOneFrame display width %d height %d, crop %d %d %d %d, stride %d", *width, *height, *crop_left, *crop_top, *crop_right, *crop_bottom, stride);
            if ((rtn >= 0) && data) {  // capture completed
                DIP_DeInit(DipHandle[OverlayID ? 1 : 0]);
                DipHandle[OverlayID ? 1 : 0] = NULL;
                if (!bCapImmediate[OverlayID ? 1 : 0]) {
                    u8Abort[OverlayID ? 1 : 0] |= ABORT_WAIT_DISP_CAPTURE;
                }
            }
        } else {
            u8Abort[OverlayID ? 1 : 0] |= ABORT_WAIT_DISP_CAPTURE;
            rtn = -1;
        }
    }
    if (u8Captured[OverlayID ? 1 : 0]) u8Captured[OverlayID ? 1 : 0]--;
    VSYNC_BRIDGE_UNLOCK(OverlayID ? 1 : 0);
    return rtn;
}

void vsync_bridge_wait_disp_show(MS_DispFrameFormat *dff)
{
    unsigned long u32Cnt = 0, u32Cnt2;
    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[vsync_bridge_GetSHMIndex(dff)] + (vsync_bridge_GetSHMIndex(dff)*sizeof(MCU_DISPQ_INFO)));

    if (dff->u8MCUMode) {
        return;
    }

    if (dff->u32FrameRate) {
        u32Cnt = (1000000 / dff->u32FrameRate) * 10;
    } else {
        u32Cnt = 200;
    }

    if (pSHM && u8VsyncBridgeEnable[dff->OverlayID ? 1 : 0]) {
        unsigned char u8WaitDispIndex = DISPQ_FIRST_R_INDEX;
        unsigned long u32Cnt2;

        if (dff->u8Interlace) {
            vsync_bridge_set_fdmask(dff, FALSE, TRUE);
        }
        if (!bIsMVOPAutoGenBlack) {
            #define DISPQ_SECOND_R_INDEX      2
            if (dff->u8Interlace) {
                u32Cnt *= 3;
                u8WaitDispIndex = DISPQ_SECOND_R_INDEX;
            }
        }

        u32Cnt2 = u32Cnt;
        u8Abort[dff->OverlayID ? 1 : 0] &= ~ABORT_WAIT_DISP_SHOW;

        while (--u32Cnt && (pSHM->u8McuDispQRPtr != u8WaitDispIndex)) {
            if (u8Abort[dff->OverlayID ? 1 : 0] & ABORT_WAIT_DISP_SHOW) {
                return;
            }
            usleep(1000);
        }

        if (!u32Cnt) {
            ALOGE("[%d]Wait frame done timeout!! ", dff->OverlayID);
            VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
            int buf_slot = vsync_bridge_buf_slot_get_index(dff, false);
            VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
            if (buf_slot == INDEX_NOT_FOUND) {
                // Timeout !!! No frame in display queue, add one to prevent black screen.
                vsync_bridge_render_frame(dff, VSYNC_BRIDGE_UPDATE);
            }
        }

        ALOGD("Overlay[%d] wait_disp_show W %d, R %d, Cur %d, wait time = %dms",dff->OverlayID,
            pSHM->u8McuDispQWPtr, pSHM->u8McuDispQRPtr, dff->u8CurIndex, (int)(u32Cnt2 - u32Cnt));

        if (dff->u32FrameRate) {
            usleep(1000000000/dff->u32FrameRate);
        }
        if (dff->u8Interlace) {
            vsync_bridge_set_fdmask(dff, TRUE, TRUE);
        }
    } else if (!pSHM) {
        ALOGE("vsync_bridge_wait_disp_show pSHM is NULL");
    }

}

MS_BOOL vsync_bridge_GetInterlaceOut(void)
{
    return bInterlaceOut;
}

void vsync_bridge_SetInterlaceOut(MS_BOOL bIsInterlaceOut)
{
    bInterlaceOut = bIsInterlaceOut;
    ALOGD("set interlace out: %d", bIsInterlaceOut);
}

MS_BOOL vsync_bridge_CheckInterlaceOut(MS_WINDOW *pDispWin)
{
    MS_BOOL Ret = FALSE;
    if (pDispWin == NULL) {
        return Ret;
    }

    if (vsync_bridge_GetInterlaceOut()) {
        pDispWin->height = pDispWin->height >> 1;
        pDispWin->y = pDispWin->y >> 1;
        Ret = TRUE;
    }
    return Ret;
}

void vsync_bridge_SetSavingBandwidthMode(unsigned char bSaveMode, unsigned char Overlay_ID)
{
    VSYNC_BRIDGE_LOCK(Overlay_ID);

    ALOGD("[%d]Saving Band width Mode %d", Overlay_ID, bSaveMode);
    u8SaveBandwidthMode[Overlay_ID] = bSaveMode;

    if (ds_int_data[Overlay_ID].bIsInitialized) {
        u8SizeChange[Overlay_ID] = 1;
        vsync_bridge_ds_set_index(&lastdff[Overlay_ID], TRUE);
    }

    VSYNC_BRIDGE_UNLOCK(Overlay_ID);
}

void vsync_bridge_SetBlackScreenFlag(unsigned char Overlay_ID, MS_BOOL bFlag)
{
    VSYNC_BRIDGE_LOCK(Overlay_ID);
    u8Rolling[Overlay_ID] = bFlag;
    if (u8Rolling[Overlay_ID]) {
        vsync_bridge_buf_slot_clear(Overlay_ID);
    }
    VSYNC_BRIDGE_UNLOCK(Overlay_ID);
    ALOGD("Set Win %d Rolling mode %d", Overlay_ID, bFlag);
}

void vsync_bridge_update_status(unsigned char Overlay_ID)
{
    //FIXME: the pSHM should use vsync_bridge_GetSHMIndex(dff)
    MCU_DISPQ_INFO *pSHM = (MCU_DISPQ_INFO *)MsOS_PA2KSEG1(u32SHMAddr[Overlay_ID ? 1 : 0] + ((MS_U32)Overlay_ID * sizeof(MCU_DISPQ_INFO)));
    VSYNC_BRIDGE_LOCK(Overlay_ID);
    // If in pause state, we must update FBL mode here.
    if (pSHM && pSHM->u8McuDispSwitch) {
        if (MApi_XC_IsCurrentRequest_FrameBufferLessMode() || MApi_XC_IsCurrentFrameBufferLessMode()) {
            pSHM->u1FBLMode = 1;
        } else {
            pSHM->u1FBLMode = 0;
        }
    }
    VSYNC_BRIDGE_UNLOCK(Overlay_ID);

#if ENABLE_FRAME_CONVERT
    u8UpdateFrcMode[Overlay_ID ? 1 : 0] = 1;
#endif
}


MS_FreezeMode vsync_bridge_GetLastFrameFreezeMode(unsigned char Overlay_ID)
{
    return stFreezeMode[Overlay_ID];
}

void vsync_bridge_set_frame_done_callback(unsigned char Overlay_ID, void *pHandle, FrameDoneCallBack callback)
{
    VSYNC_BRIDGE_LOCK(Overlay_ID);
    callback_handle[Overlay_ID] = pHandle;
    frame_done_callback[Overlay_ID] = callback;
    VSYNC_BRIDGE_UNLOCK(Overlay_ID);
}

void vsync_bridge_set_fdmask(MS_DispFrameFormat *dff, MS_BOOL bEnable, MS_BOOL bLockMutex)
{
    if (dff == NULL) return;

    if (IS_XC_FDMASK_API_READY) {
        if (bLockMutex) VSYNC_BRIDGE_LOCK(dff->OverlayID ? 1 : 0);
        ALOGD("vsync_bridge_set_fdmask %d", bEnable);
        if (bEnable) {
            if ((u8FDMaskEnable[dff->OverlayID ? 1 : 0] == 0) && dff->u8Interlace) {
                MApi_XC_set_FD_Mask_byWin(1, (SCALER_WIN)dff->OverlayID);
                u8FDMaskEnable[dff->OverlayID ? 1 : 0] = 1;
            }
        } else {
            MApi_XC_set_FD_Mask_byWin(0, (SCALER_WIN)dff->OverlayID);
            u8FDMaskEnable[dff->OverlayID ? 1 : 0] = 0;
        }
        if (bLockMutex) VSYNC_BRIDGE_UNLOCK(dff->OverlayID ? 1 : 0);
    }
}

void vsync_bridge_update_output_timing(unsigned char Overlay_ID, Vsync_Bridge_Mode eMode)
{
#if ENABLE_FRAME_CONVERT
    u8UpdateFrcMode[Overlay_ID ? 1 : 0] = 1;
#endif
    u8SizeChange[Overlay_ID] = 1;

#ifdef SUPPORT_10BIT
    vsync_bridge_set_mvop_10bit(&lastdff[Overlay_ID], eMode);
#endif
}

void vsync_bridge_CalAspectRatio_WithTimingChange(unsigned char Overlay_ID, MS_WINDOW *pDispWin, MS_WINDOW *pCropWin, MS_BOOL bCalAspect, MS_BOOL bUseDefaultArc, int panel_width, int panel_height)
{
    vsync_bridge_CalAspectRatio(pDispWin, pCropWin, &lastdff[Overlay_ID], TRUE, FALSE, panel_width, panel_height);
}

MS_BOOL vsync_bridge_is_hw_auto_gen_black(void)
{
    return bIsMVOPAutoGenBlack;
}

void vsync_bridge_SetBlankVideo(unsigned char Overlay_ID,unsigned char Count)
{
    VSYNC_BRIDGE_LOCK(Overlay_ID ? 1 : 0);
    u8BlankVideoMute[Overlay_ID ? 1 : 0] = Count;
    VSYNC_BRIDGE_UNLOCK(Overlay_ID ? 1 : 0);
}
