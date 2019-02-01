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

//#include "VideoSetWrapper.h"

#include <videoset/VideoSet.h>

#include <picturemanager/PictureManager.h>
#include <threedimensionmanager/ThreeDimensionManager.h>
#include <pipmanager/PipManager.h>
#include <pipmanager/PipManagerType.h>
#include "MsFrmFormat.h"
#include "vsync_bridge.h"
#include <cutils/properties.h>

#include "VideoSetWrapper.h"
#include "mi_common.h"
#include "mi_video.h"
#include "mi_display.h"
#include <apiPNL.h>
#include "mi_dispout.h"
#include "mi_dmx.h"
#include "mi_v3d.h"
#include "mi_dispout_datatype.h"
#include "mi_video_datatype.h"

// FIXME: Use driver API directly before MI API is ready.
#include "drvMVOP.h"

#define WIDTH_4K    (3840)
#define WIDTH_2K    (1920)
#define HEIGHT_2K   (2160)
#define HEIGHT_1K   (1080)

#define VINFO_TO_MI(stSetMode, pVinfo) \
    do { \
            memset(&stSetMode, 0, sizeof(MI_VIDEO_DispInfo_t));\
            (stSetMode).enCodecType = (pVinfo)->enCodecType;\
            (stSetMode).u32YAddress = (pVinfo)->u32YAddress;\
            (stSetMode).u32UVAddress = (pVinfo)->u32UVAddress;\
            (stSetMode).u8DataOnMIU = (pVinfo)->u8DataOnMIU;\
            (stSetMode).u16FrameRate = (pVinfo)->u16FrameRate;\
            (stSetMode).u8Interlace = (pVinfo)->u8Interlace;\
            (stSetMode).u16HorSize = (pVinfo)->u16HorSize;\
            (stSetMode).u16VerSize = (pVinfo)->u16VerSize;\
            (stSetMode).u16CropRight = (pVinfo)->u16CropRight;\
            (stSetMode).u16CropLeft = (pVinfo)->u16CropLeft;\
            (stSetMode).u16CropBottom = (pVinfo)->u16CropBottom;\
            (stSetMode).u16CropTop = (pVinfo)->u16CropTop;\
            (stSetMode).u8AspectRatio = (pVinfo)->u8AspectRatio;\
            (stSetMode).u8Is3DLayout = (pVinfo)->u8Is3DLayout;\
            (stSetMode).en3DLayout = (pVinfo)->en3DLayout;\
            (stSetMode).u8DS_Index_Depth = (pVinfo)->u8DS_Index_Depth;\
            (stSetMode).u8EnableDynamicScaling = (pVinfo)->u8EnableDynamicScaling;\
            (stSetMode).u32DSAddr = (pVinfo)->u32DSAddr;\
            (stSetMode).u32DSSize = (pVinfo)->u32DSSize;\
            (stSetMode).u32XCAddr = (pVinfo)->u32XCAddr;\
            (stSetMode).u8DS_MIU_Select = (pVinfo)->u8DS_MIU_Select;\
            (stSetMode).u32AspectWidth = (pVinfo)->u32AspectWidth;\
            (stSetMode).u32AspectHeight = (pVinfo)->u32AspectHeight;\
            (stSetMode).u8Reserve[0] = (pVinfo)->u8Reserve[0];\
            (stSetMode).u8Reserve[1] = (pVinfo)->u8Reserve[1];\
            (stSetMode).u8Reserve[2] = (pVinfo)->u8Reserve[2];\
        } while(0)



#define IS_4K_DS_MODE(pVinfo) ((pVinfo)->u8Reserve[1] & 0x01)
static sp<VideoSet> pVideoSet = NULL;
static sp<PictureManager> pPictureManager = NULL;
static sp<PipManager> pPipManager = NULL;

static int used[2] = {0};
static int WIN_Initialized[2] = {0};
static int DS_Enabled[2] = {0};
static int DS_Restore[2] = {0};
static MS_VideoInfo SetMode_Vinfo[2];
static MS_VideoInfo SetWin_Vinfo[2];
static MS_WINDOW VideoWin[2];
static MS_WINDOW XCWin[2] = {0};
static MI_BOOL mi_enable_ds = FALSE;
static MI_BOOL bLastVideoSetAvailable = FALSE;
static int State[2] = {0};
static MI_BOOL Mute_Status[2] = {0};
static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
static int MVOP_width[2] = {0};
static int MVOP_height[2] = {0};
static bool b2StageDS = 0;
#define VIDEOSET_LOCK  pthread_mutex_lock(&sLock)
#define VIDEOSET_UNLOCK pthread_mutex_unlock(&sLock)

static void (*fpSetAspectRatio)(int32_t u32Value) = NULL;
static void (*fpEnable3D)(int32_t u32Value) = NULL;
static void (*fpEnableHDR)(int32_t u32Value) = NULL;

class PicMgrListener : public PictureManagerListener {
public:
    PicMgrListener();
    ~PicMgrListener();
    void notify(int msg, int ext1, int ext2);
    void PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_SetAspectratio(int32_t ext1, int32_t ext2);
    void PostEvent_4K2KPhotoDisablePip(int32_t ext1, int32_t ext2);
    void PostEvent_4K2KPhotoDisablePop(int32_t ext1, int32_t ext2);
    void PostEvent_4K2KPhotoDisableDualview(int32_t ext1, int32_t ext2);
    void PostEvent_4K2KPhotoDisableTravelingmode(int32_t ext1, int32_t ext2);
};

void PicMgrListener::PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2)
{

}

void PicMgrListener::PostEvent_SetAspectratio(int32_t ext1, int32_t ext2)
{
    ALOGD("EV_SET_ASPECTRATIO %d",ext1);
    if (fpSetAspectRatio)   fpSetAspectRatio(ext1);
}
void PicMgrListener::PostEvent_4K2KPhotoDisablePip(int32_t ext1, int32_t ext2)
{

}
void PicMgrListener::PostEvent_4K2KPhotoDisablePop(int32_t ext1, int32_t ext2)
{

}
void PicMgrListener::PostEvent_4K2KPhotoDisableDualview(int32_t ext1, int32_t ext2)
{

}
void PicMgrListener::PostEvent_4K2KPhotoDisableTravelingmode(int32_t ext1, int32_t ext2)
{

}
void PicMgrListener::notify(int msg, int ext1, int ext2)
{

}

void PicMgrListener::PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2)
{
}

PicMgrListener::PicMgrListener()
{
}

PicMgrListener::~PicMgrListener()
{
}


class ThreeDMgrListener : public ThreeDimensionManagerListener {
public:
    ThreeDMgrListener();
    ~ThreeDMgrListener();
    void PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_Enable3D(int32_t ext1, int32_t ext2);
    void PostEvent_4k2kUnsupportDualView(int32_t ext1, int32_t ext2);
};

void ThreeDMgrListener::PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2)
{

}

void ThreeDMgrListener::PostEvent_Enable3D(int32_t ext1, int32_t ext2)
{
    if (!WIN_Initialized[0]) {
        return;
    }

    ALOGD("PostEvent_Enable3D %d %d", ext1, ext2);
    if (fpEnable3D)   fpEnable3D(ext1);

    VinfoReserve0 *videoflags = (VinfoReserve0 *)&SetMode_Vinfo[0].u8Reserve[0];
    if (videoflags->application_type == MS_APP_TYPE_IMAGE) {
        if (ext1 != 0) {
            SetMode_Vinfo[0].u8Is3DLayout = 1;
        } else {
            SetMode_Vinfo[0].u8Is3DLayout = 0;
        }
    } else {
        if (ext1 != 0) {
            // needs to turn of DS if user want to turn on 3D
            if ((pVideoSet != NULL) && DS_Enabled[0]) {
                ALOGD("Wrapper_Video_disableDS %d ", 0);
                if (pVideoSet->video_disableDS(0, 0) == 0) {
                    DS_Enabled[0] = 0;
                    DS_Restore[0] = 1;      // restore DS after disable 3D
                }
            }
        } else if (DS_Restore[0]) {
            if (pVideoSet != NULL) {
                if (pVideoSet->video_disableDS(0, 1) == 0) {
                    ALOGD("Wrapper_Video_disableDS restore DS ");
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
    }

    if (ext1 != 0) {
        State[0] |= E_STATE_3D;
    } else {
        State[0] &= ~E_STATE_3D;
    }
}

void ThreeDMgrListener::PostEvent_4k2kUnsupportDualView(int32_t ext1, int32_t ext2)
{

}
void ThreeDMgrListener::PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2)
{
}

ThreeDMgrListener::ThreeDMgrListener()
{
}

ThreeDMgrListener::~ThreeDMgrListener()
{
}

class PipMgrListener : public PipManagerListener {
public:
    PipMgrListener();
    ~PipMgrListener();
    void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    void dataCallback(PipTravelingDataInfo * info, int32_t enEngineType=0);
    void eventCallback(PipTravelingEventInfo* info, int32_t enEngineType=0);
    void PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_EnablePop(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_EnablePip(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_4k2kUnsupportPip(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_4k2kUnsupportPop(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_4k2kUnsupportTravelingMode(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_RefreshPreviewModeWindow(int32_t nEvt, int32_t ext1, int32_t ext2);
};

void PipMgrListener::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{

}

void PipMgrListener::dataCallback(PipTravelingDataInfo * info, int32_t enEngineType)
{

}

void PipMgrListener::eventCallback(PipTravelingEventInfo * info, int32_t enEngineType)
{

}

void PipMgrListener::PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2)
{

}

void PipMgrListener::PostEvent_EnablePop(int32_t nEvt, int32_t ext1, int32_t ext2)
{
    if (!WIN_Initialized[0]) {
        return;
    }

    if ((EV_PIPMANAGER)nEvt == EV_ENABLE_POP) {
        if (ext1 != 0) {
            // needs to turn of DS if user want to turn on pop
            if ((pVideoSet != NULL) && DS_Enabled[0]) {
                ALOGD("Wrapper_Video PipMgrListener::PostEvent_EnablePop disableDS %d ", 0);
                if (pVideoSet->video_disableDS(0, 0) == 0) {
                    DS_Enabled[0] = 0;
                    DS_Restore[0] = 1;
                }
            }
        } else if (DS_Restore[0]) {
            if (pVideoSet != NULL) {
                if (pVideoSet->video_disableDS(0, 1) == 0) {
                    ALOGD("Wrapper_Video PipMgrListener::PostEvent_EnablePop disableDS %d ", 1);
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
        if (ext1 != 0) {
            State[0] |= E_STATE_POP;
        } else {
            State[0] &= ~E_STATE_POP;
        }
    } else {
        ALOGD("Receive unknown event(%08d)\n", nEvt);
    }
}
void PipMgrListener::PostEvent_EnablePip(int32_t nEvt, int32_t ext1, int32_t ext2)
{
    if (!WIN_Initialized[0]) {
        return;
    }

    if ((EV_PIPMANAGER)nEvt == EV_ENABLE_PIP) {
        if (ext1 != 0) {
            // needs to turn of DS if user want to turn on pip
            if ((pVideoSet != NULL) && DS_Enabled[0]) {
                ALOGD("Wrapper_Video PipMgrListener::PostEvent_EnablePip disableDS %d ", 0);
                if (pVideoSet->video_disableDS(0, 0) == 0) {
                    DS_Enabled[0] = 0;
                    DS_Restore[0] = 1;
                }
            }
        } else if (DS_Restore[0]) {
            if (pVideoSet != NULL) {
                if (pVideoSet->video_disableDS(0, 1) == 0) {
                    ALOGD("Wrapper_Video_disableDS restore DS %d", 1);
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
        if (ext1 != 0) {
            State[0] |= E_STATE_PIP;
        } else {
            State[0] &= ~E_STATE_PIP;
        }
    } else {
        ALOGD("Receive unknown event(%08d)\n", nEvt);
    }
}
void PipMgrListener::PostEvent_4k2kUnsupportPip(int32_t nEvt, int32_t ext1, int32_t ext2)
{
}
void PipMgrListener::PostEvent_4k2kUnsupportPop(int32_t nEvt, int32_t ext1, int32_t ext2)
{
}
void PipMgrListener::PostEvent_4k2kUnsupportTravelingMode(int32_t nEvt, int32_t ext1, int32_t ext2)
{
}
void PipMgrListener::PostEvent_RefreshPreviewModeWindow(int32_t, int32_t, int32_t)
{
}
void PipMgrListener::PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2)
{
}

PipMgrListener::PipMgrListener()
{
}

PipMgrListener::~PipMgrListener()
{
}

class VdSetListener : public VideoSetListener {
public:
    VdSetListener();
    ~VdSetListener();
    virtual void PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2);
    virtual void PostEvent_Send_Video_Info(int32_t win,int32_t surface_x,int32_t surface_y,int32_t surface_width,int32_t surface_height,int32_t width,int32_t height,int32_t framerate,int32_t interlace,int32_t CodecType);
    virtual void PostEvent_Send_CropDisp_Win(int32_t win,int32_t crop_x,int32_t crop_y,int32_t crop_width,int32_t crop_height,int32_t disp_x,int32_t disp_y,int32_t disp_width,int32_t disp_height);
    virtual void PostEvent_Close_Win(int32_t win);
};
void VdSetListener::PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2)
{

}
void VdSetListener::PostEvent_Send_Video_Info(int32_t win,int32_t surface_x,int32_t surface_y,int32_t surface_width,int32_t surface_height,int32_t width,int32_t height,int32_t framerate,int32_t interlace,int32_t CodecType)
{
    //ALOGD("PostEvent_MM_PlayStatus_Notify");
}
void VdSetListener::PostEvent_Close_Win(int32_t win)
{
    //ALOGD("PostEvent_Close_Win = %d",win);
}

void VdSetListener::PostEvent_Send_CropDisp_Win(int32_t win,int32_t crop_x,int32_t crop_y,int32_t crop_width,int32_t crop_height,int32_t disp_x,int32_t disp_y,int32_t disp_width,int32_t disp_height)
{
    ALOGD("PostEvent_Send_CropDisp_Win %d,crop x=%d,y=%d,w=%d, h=%d || disp x=%d,y=%d,w=%d, h=%d",win,crop_x,crop_y,crop_width,crop_height,disp_x,disp_y,disp_width,disp_height);
    stVBDispCrop data;
    memset(&data, 0x00, sizeof(data));
    data.u16CropX = crop_x;
    data.u16CropY = crop_y;
    data.u16CropWidth = crop_width;
    data.u16CropHeight = crop_height;
    data.u16DispX = disp_x;
    data.u16DispY = disp_y;
    data.u16DispWidth = disp_width;
    data.u16DispHeight = disp_height;
    data.u8Win = win;
    vsync_bridge_ds_set_ext_DipCropWin(&data);
}

VdSetListener::VdSetListener()
{
}

VdSetListener::~VdSetListener()
{
}

static MI_HANDLE hVdecHandle[2] ={MI_HANDLE_NULL,MI_HANDLE_NULL};
static MI_HANDLE _hDisplay0Handle = MI_HANDLE_NULL;
static MI_WINDOW_HANDLE _hWinHandle[2];
static MI_DISPOUT_HANDLE _hDispoutHandle = MI_HANDLE_NULL;
static MI_V3D_HANDLE _hV3dhandle = MI_HANDLE_NULL;
static MI_BOOL bVideoModeSet[2] = {FALSE, FALSE};
static MI_BOOL bMVC_Enable3D = FALSE;
typedef enum {
    E_4K2K_MODE_LOCK_INALID =0,
    E_4K2K_MODE_LOCK_ON,
    E_4K2K_MODE_LOCK_OFF,
} En4K2K_Mode_e;

typedef enum
{
    /// HDR is stop.
    E_HDR_STOP,
    /// Open HDR is running.
    E_OPEN_HDR_RUNNING,
    /// Dolby HDR is running..
    E_DOLBY_HDR_RUNNING,
} MS_HDR_STATUS;

static En4K2K_Mode_e e4K2KMode = E_4K2K_MODE_LOCK_OFF;
#if (MI_DEBUG == 1)
#define trace()  ALOGE("%s @%d: TIME: %s.\n", __func__, __LINE__, __TIME__)
#define traceD(format, args...) ALOGE("%s @%d: TIME: %s: "#format, __func__, __LINE__, __TIME__, ##args)
#else
#define trace()
#define traceD(format, args...)
#endif

#define showCropWin(stCropWin)  traceD("Test: MI_DISPLAY_UpdateWindowInputRect() %d X %d @ (%d, %d)", stCropWin.u16Width, stCropWin.u16Height, stCropWin.u16X, stCropWin.u16Y)
#define DS_STATUS_SET(st) do { \
                    traceD("Set DS Status: %d.\n", st); \
                    DS_Enabled[enWin] = (st);\
                    } while (0)

extern "C" MI_BOOL MI_DEMO_DISP_Init(MI_HANDLE *phVidWin, MI_HANDLE hInputSrc);
extern "C" MI_BOOL MI_DEMO_DISP_Exit(void);

static void _mi_PostEvent_Enable3D(MI_HANDLE hV3dHandle, MI_U32 u32EventFlags, void *pUserData, MI_BOOL bEnable)
{
    MI_V3D_3D_Status_t  *pstStatus = (MI_V3D_3D_Status_t *)pUserData;
    ALOGD("%s @ line %d: PostEvent_Enable3D enable %d, 3dType %d\n", __func__, __LINE__,bEnable, pstStatus->e3dType);
    if (pstStatus == NULL) {
        ALOGE("Invalid parameters!!!\n");
        return;
    } else if ((pstStatus->e3dType > E_MI_V3D_NONE) && (pstStatus->e3dType < E_MI_V3D_TYPE_NUM)) {
        ALOGD("PostEvent_Enable3D Enable3D.\n");
        if (!bEnable) {
            return;
        }
    } else {
        ALOGD("PostEvent_Enable3D Disable3D.\n");
        if (bEnable) {
            return;
        }
    }

    if (bEnable == TRUE) {
        State[0] |= E_STATE_3D;
    } else {
        State[0] &= ~E_STATE_3D;
    }

    if (!WIN_Initialized[0]) {
        return;
    }

    if (fpEnable3D) {
        fpEnable3D(bEnable);
    }

    MI_U32 enWin = 0;
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&SetMode_Vinfo[0].u8Reserve[0];
    if (videoflags->application_type == MS_APP_TYPE_IMAGE) {
        SetMode_Vinfo[0].u8Is3DLayout = 1;
    } else {
        if (bEnable == TRUE) {
            // needs to turn off DS if user want to turn on 3D
            if (Wrapper_Video_closeDS(0, 0, 0) == 0) {
                DS_Enabled[0] = 0;
                DS_Restore[0] = 1;
            }
        } else {
            if (DS_Restore[0]) {
                if (Wrapper_Video_closeDS(0, 1, 0) == 0) {
                    vsync_bridge_ds_update_table(0);
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
        vsync_bridge_update_status(0);
    }
}

static void _mi_3D_callback(MI_HANDLE hV3dHandle, MI_U32 u32EventFlags, void *pUserData)
{
    if (u32EventFlags & E_MI_V3D_V3DCALLBACK_EVENT_PRE_3D_STATUS_CHANGE) {
        _mi_PostEvent_Enable3D(hV3dHandle, u32EventFlags, pUserData, TRUE);
    }else if (u32EventFlags & E_MI_V3D_V3DCALLBACK_EVENT_POST_3D_STATUS_CHANGE) {
        _mi_PostEvent_Enable3D(hV3dHandle, u32EventFlags, pUserData, FALSE);
    }
}

static void _mi_PostEvent_hdr(MI_DISPLAY_HdrEvent_e enHdrEvent)
{
    int enableHDR = E_HDR_STOP;
    switch (enHdrEvent)
    {
        case E_MI_DISPLAY_HDR_EVENT_STATE_DOLBY_TURN_ON:
            enableHDR = E_DOLBY_HDR_RUNNING;
        break;
        case E_MI_DISPLAY_HDR_EVENT_STATE_OPEN_TURN_ON:
            enableHDR = E_OPEN_HDR_RUNNING;
        break;
        case E_MI_DISPLAY_HDR_EVENT_STATE_DOLBY_TURN_OFF:
        case E_MI_DISPLAY_HDR_EVENT_STATE_OPEN_TURN_OFF:
            enableHDR = E_HDR_STOP;
        break;
        default:
            ALOGE("Unknown HDR event.");
        return;
    }

    if (fpEnableHDR)   fpEnableHDR(enableHDR);
}

static void _mi_hdr_callback(MI_WINDOW_HANDLE hWinHandle, MI_U32 u32EventFlags, void *pOutData, void *pUsrData)
{
    if (u32EventFlags == E_MI_DISPLAY_WINDOWCALLBACK_EVENT_HDR)
    {
        ALOGD("_mi_hdr_callback u32EventFlags: %d, HDR event: %d\n", u32EventFlags, (*(MI_DISPLAY_HdrEvent_e*)pOutData));
        _mi_PostEvent_hdr(*(MI_DISPLAY_HdrEvent_e*)pOutData);
    }
}

static MI_RESULT mi_get_v3d_handle(int enWin, MI_V3D_HANDLE *phV3d)
{
    MI_V3D_InitParams_t stInit;
    MI_RESULT eRet = MI_ERR_FAILED;

    if (_hV3dhandle == MI_HANDLE_NULL) {
        INIT_ST(stInit);
        MI_V3D_Init(&stInit);
        eRet = MI_V3D_GetHandle(enWin, &_hV3dhandle);
        if (eRet != MI_OK) {
            ALOGE("Fail to MI_V3D_GetHandle(...).\n");
            return eRet;
        }
    } else {
        eRet = MI_OK;
    }

    *phV3d = _hV3dhandle;
    ALOGE("MI_V3D_GetHandle().\n");
    return eRet;
}

static MI_RESULT mi_get_v3d_status(int enWin, MI_BOOL *pbEnbale)
{
    MI_RESULT eRet = MI_ERR_FAILED;
    MI_V3D_3dType_e e3dType = E_MI_V3D_TYPE_NUM;
    MI_V3D_HANDLE hV3d = MI_HANDLE_NULL;

    eRet = mi_get_v3d_handle(enWin, &hV3d);
    if (eRet == MI_OK) {
        eRet = MI_V3D_GetAttr(hV3d, E_MI_V3D_PARAMTYPE_3D_FORMAT, &e3dType);
        if (eRet == MI_OK) {
            if ((e3dType > E_MI_V3D_NONE) && (e3dType < E_MI_V3D_TYPE_NUM)) {
                *pbEnbale = TRUE;
                ALOGD("3D enable, DS should closed.\n");
            } else {
                *pbEnbale = FALSE;
                ALOGD("3D disable, DS should on.\n");
            }
            eRet = MI_OK;
        } else {
            ALOGE("Fail to Get 3D format: eRet: %d.\n", eRet);
        }
    } else {
        ALOGE("Fail to get v3d handle.\n");
    }

    return eRet;
}

static MI_BOOL mi_check_Pannel4K2K(void)
{
// FIXME: This should be rewritten if driver can tell us output timing.
#if defined(MSTAR_NAPOLI)
    MI_RESULT eRet = MI_ERR_FAILED;
    MI_VIDEO_DispInfo_t stDispInfo;
    MI_DISPOUT_PanelAttrType_e eAttrType = E_MI_DISPOUT_PANEL_ATTR_PANEL_INFO;
    MI_DISPOUT_PanelInfo_t stPanelInfo;
    INIT_ST(stPanelInfo);

    eRet = MI_DISPOUT_PanelGetAttr(_hDispoutHandle, eAttrType, &stPanelInfo);
    if (eRet == MI_OK) {
        if (stPanelInfo.u16Width >= WIDTH_4K) {
            return TRUE;
        }
    }
    ALOGI("%s @ line %d: Panel(%d X %d) start Get out timing.\n", __func__, __LINE__, stPanelInfo.u16Height, stPanelInfo.u16Width);
#endif
    return FALSE;
}

static void timingToWidthHeight(MI_DISPLAY_OutputTiming_e eOutputTiming,int *width,int *height) {
    switch(eOutputTiming)
        {
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

static void _mi_switchXCOpTiming(MI_DISPLAY_HANDLE hDispHandle, MI_U32 u32EventFlags, void *pOutData, void *pUserData)
{
    ALOGD("%s @ line %d: _mi_switchXCOpTiming. flag %u: \n", __func__, __LINE__, u32EventFlags);
    MI_DISPOUT_EnableMute(_hDispoutHandle, MI_DISPLAY_MUTE_FLAG_SURFACEFLINGER);
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&SetMode_Vinfo[0].u8Reserve[0];
    if (u32EventFlags == E_MI_DISPLAY_DISPLAYCALLBACK_EVENT_PRE_TIMING_CHANGE) {
        if (videoflags->application_type != MS_APP_TYPE_IMAGE) {
            if ((mi_enable_ds == TRUE) && (DS_Enabled[0] == TRUE)) {
                MI_RESULT eRet = MI_ERR_FAILED;
                MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
                MI_DISPLAY_DsInfo_t stDsInfo;
                INIT_ST(stDsInfo);
                stDsInfo.stVidDispInfo = *((MI_VideoDispInfo *)&SetMode_Vinfo[0]);
                stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_OFF;
                eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[0], eAttrType, &stDsInfo);
                if (eRet == MI_OK) {
                    DS_Enabled[0] = 0;
                }
            }
        }
    } else if (u32EventFlags ==  E_MI_DISPLAY_DISPLAYCALLBACK_EVENT_POST_TIMING_CHANGE) {
        int ret = -1;
        if (videoflags->application_type != MS_APP_TYPE_IMAGE) {
            MI_RESULT eRet = MI_ERR_FAILED;
            MI_DISPLAY_DsInfo_t stDsInfo;
            INIT_ST(stDsInfo);
            if ((mi_enable_ds == TRUE) && (DS_Enabled[0] == FALSE) && (SetMode_Vinfo[0].u8EnableDynamicScaling == 1)) {
                MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
                stDsInfo.stVidDispInfo = *((MI_VideoDispInfo *)&SetMode_Vinfo[0]);
                stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_RESTORE;
                eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[0], eAttrType, &stDsInfo);
                if (MI_OK != eRet) {
                    ALOGE("Fail to set WindowAttr.\n");
                } else {
                    ret = 0;
                }
            }

            MS_VideoInfo vinfo;
            memcpy(&vinfo, &SetMode_Vinfo[0], sizeof(MS_VideoInfo));
            MI_VIDEO_DispInfo_t stSetMode;
            VINFO_TO_MI(stSetMode, &vinfo);
            if (stDsInfo.eDsStatus == E_MI_DISPLAY_DS_STATUS_RESTORE) {
                if (IS_4K_DS_MODE(&vinfo)) {
                    stSetMode.u16HorSize = (vinfo.u16HorSize > 3840) ? vinfo.u16HorSize : 3840;
                    stSetMode.u16VerSize = (vinfo.u16VerSize > 2160) ? vinfo.u16VerSize : 2160;
                }
                else {
                    stSetMode.u16HorSize = (vinfo.u16HorSize > 1920) ? vinfo.u16HorSize : 1920;
                    stSetMode.u16VerSize = (vinfo.u16VerSize > 1088) ? vinfo.u16VerSize : 1088;
                }
            }
            eRet = MI_VIDEO_SetMode(hVdecHandle[0], &stSetMode);
            if (MI_OK != eRet) {
                ALOGE("Fail to set mode.\n");
            }
        }

        MI_RESULT eRet;
        MI_DISPLAY_VidWin_Rect_t stRect;
        MS_WINDOW DispWin;
        MS_WINDOW CropWin;
        int width = 0;
        int height = 0;

        DispWin.x = VideoWin[0].x;
        DispWin.y = VideoWin[0].y;
        DispWin.width = VideoWin[0].width;
        DispWin.height = VideoWin[0].height;
        CropWin.x = SetWin_Vinfo[0].u16CropLeft;
        CropWin.y = SetWin_Vinfo[0].u16CropTop;
        CropWin.width = SetWin_Vinfo[0].u16HorSize - SetWin_Vinfo[0].u16CropLeft - SetWin_Vinfo[0].u16CropRight;
        CropWin.height = SetWin_Vinfo[0].u16VerSize - SetWin_Vinfo[0].u16CropTop - SetWin_Vinfo[0].u16CropBottom;

        MI_DISPLAY_OutputTiming_e outTiming = *(MI_DISPLAY_OutputTiming_e*)pOutData;
        timingToWidthHeight(outTiming, &width, &height);
        vsync_bridge_CalAspectRatio_WithTimingChange(0, &DispWin, &CropWin, TRUE, FALSE, width, height);

        XCWin[0].x = DispWin.x;
        XCWin[0].y = DispWin.y;
        XCWin[0].width = DispWin.width;
        XCWin[0].height = DispWin.height;
        SetWin_Vinfo[0].u16CropLeft = CropWin.x;
        SetWin_Vinfo[0].u16CropTop = CropWin.y;
        SetWin_Vinfo[0].u16HorSize = CropWin.width + CropWin.x;
        SetWin_Vinfo[0].u16VerSize = CropWin.height + CropWin.y;
        SetWin_Vinfo[0].u16CropRight = 0;
        SetWin_Vinfo[0].u16CropBottom = 0;

        stRect.u16X = CropWin.x;
        stRect.u16Y = CropWin.y;
        stRect.u16Width = CropWin.width;
        stRect.u16Height= CropWin.height;
        if (SetWin_Vinfo[0].enCodecType == MS_CODEC_TYPE_MVC) {
            if (SetMode_Vinfo[0].u8Is3DLayout == 0) {
                stRect.u16Width = CropWin.width * 2;
            } else {
                stRect.u16Height = CropWin.height * 2;
            }
        }
        eRet = MI_DISPLAY_UpdateWindowInputRect(_hWinHandle[0], &stRect);
        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_SetInputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }
        showCropWin(stRect);

        stRect.u16X = DispWin.x;
        stRect.u16Y =  DispWin.y;
        stRect.u16Width = DispWin.width;
        stRect.u16Height = DispWin.height;

        eRet = MI_DISPLAY_UpdateWindowOutputRect(_hWinHandle[0], &stRect);
        ALOGD("UpdateWindowOutputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_SetOutputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }

        eRet = MI_DISPLAY_ApplyWindowRect(_hWinHandle[0]);
        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_ApplySetting.\n");
        }

        if (ret == 0) {
            DS_Enabled[0] = 1;
        }

        vsync_bridge_update_output_timing(0, VSYNC_BRIDGE_INIT);
    }
    MI_DISPOUT_DisableMute(_hDispoutHandle, MI_DISPLAY_MUTE_FLAG_SURFACEFLINGER);
}

static void _mi_display_init(void)
{
    //MI_DEMO_VIDEO_Init();
    MI_RESULT errCode, eRet;

    if (hVdecHandle[0] == MI_HANDLE_NULL && hVdecHandle[1] == MI_HANDLE_NULL) {
        MI_VIDEO_InitParams_t stVidInitParams;
        INIT_ST(stVidInitParams);
        errCode = MI_VIDEO_Init(&stVidInitParams);

        MI_DISPLAY_InitParams_t stDisInitParams;
        INIT_ST(stDisInitParams);
        MI_U8 u8DispIndex = 0;

        eRet = MI_DISPLAY_Init(&stDisInitParams);
        if (eRet != MI_OK)  {
            ALOGE("Fail to to MI_DISPLAY_Init.\n");
        }
        eRet = MI_DISPLAY_GetDisplayHandle(u8DispIndex, &_hDisplay0Handle);
        if (eRet != MI_OK) {
            ALOGE("Fail to to MI_DISPLAY_GetDisplayHandle.\n");
            eRet = MI_DISPLAY_Open(u8DispIndex, &_hDisplay0Handle);
            if (eRet != MI_OK) {
                ALOGE("Fail to to MI_DISPLAY_Open.\n");
            }
        }
        // mi outputtiming mute /unmute.
        MI_Display_DisplayCallbackParams_t stDisplayCallbackParams;
        stDisplayCallbackParams.u32EventFlags = E_MI_DISPLAY_DISPLAYCALLBACK_EVENT_ALL;
        stDisplayCallbackParams.pfEventCallback = _mi_switchXCOpTiming;
        eRet = MI_DISPLAY_RegisterDisplayCallback(_hDisplay0Handle,&stDisplayCallbackParams);
        if (eRet != MI_OK)
            ALOGE("mi register disp callback failed \n");
    }

    MI_DISPOUT_InitParams_t stInitParam;
    INIT_ST(stInitParam);
    eRet = MI_DISPOUT_Init(&stInitParam);
    if (eRet == MI_OK) {
        eRet = MI_DISPOUT_GetHandle(E_MI_DISPOUT_DEVICE_PANEL , 0, &_hDispoutHandle);
        if (eRet != MI_OK) {
            MI_DISPOUT_OpenParams_t  stOpenParams;
            INIT_ST(stOpenParams);
            stOpenParams.eDeviceType = E_MI_DISPOUT_DEVICE_PANEL;
            stOpenParams.u32DeviceId = 0;
            eRet = MI_DISPOUT_Open(&stOpenParams, &_hDispoutHandle);
            if (eRet != MI_OK) {
                _hDispoutHandle = MI_HANDLE_NULL;
                ALOGE("\033[31m Error 0x%x: [%s][%d] main failed to MI_DISPOUT_Open()!\033[m\n", eRet, __FUNCTION__, __LINE__);
            }
        }
    } else {
        _hDispoutHandle = MI_HANDLE_NULL;
        ALOGE("\033[31m Error 0x%x: [%s][%d] main failed to MI_DISPOUT_Init()!\033[m\n", eRet, __FUNCTION__, __LINE__);
    }

}

#if(MHDMITX_ENABLE == 1)
static bool mi_set_hdmi_3d_format(MI_DISPOUT_3dStructure_e format)
{
    // MVOP default output is TOP_BOTTOM &  3d mm case
    bool ret = false;
    MI_RESULT eRet = MI_ERR_FAILED;
    MI_DISPOUT_HANDLE hHdmiHandle = MI_HANDLE_NULL;
    MI_DISPOUT_InitParams_t stInitParam;
    INIT_ST(stInitParam);
    MI_DISPOUT_Init(&stInitParam);
    eRet = MI_DISPOUT_GetHandle(E_MI_DISPOUT_DEVICE_HDMI , E_MI_DISPOUT_ID_HDMI_INTERNAL, &hHdmiHandle);
    if (eRet != MI_OK) {
        MI_DISPOUT_OpenParams_t stOpenParams;
        INIT_ST(stOpenParams);
        stOpenParams.eDeviceType = E_MI_DISPOUT_DEVICE_HDMI;
        stOpenParams.u32DeviceId = E_MI_DISPOUT_ID_HDMI_INTERNAL;
        eRet = MI_DISPOUT_Open(&stOpenParams, &hHdmiHandle);
        if (eRet != MI_OK) {
            hHdmiHandle = MI_HANDLE_NULL;
            ALOGE("\033[31m Error 0x%x: [%s][%d] main failed to MI_DISPOUT_Open()!\033[m\n", eRet, __FUNCTION__, __LINE__);
        }
    }
    if (hHdmiHandle != MI_HANDLE_NULL) {
        MI_DISPOUT_DeviceInfo_t status;
        INIT_ST(status);
        eRet = MI_DISPOUT_HdmiGetAttr(hHdmiHandle, E_MI_DISPOUT_HDMI_ATTR_DEVICEINFO, (void*)&status);
        if (eRet == MI_OK) {
            if (status.abSupported3DType[E_MI_DISPOUT_3D_FRAME_PACKING] == FALSE) {
                //Not 3D TV, we don't enable 3D
                ALOGE("Do not support 3D framepacking \n");
            } else {
                MI_DISPOUT_3dStructure_e param = format;
                eRet = MI_DISPOUT_HdmiSetAttr(hHdmiHandle, E_MI_DISPOUT_HDMI_ATTR_3DFORMAT, (void *)&param);
                if (eRet != MI_OK) {
                    ALOGE("Error: set Hdmi attr fail.\n");
                } else {
                    ret = true;
                }
            }
        }
        MI_DISPOUT_Close(hHdmiHandle);
    }
    MI_DISPOUT_DeInit();
    return ret;
}
#endif

static void mi_demo_deinit(int enWin)
{
    ALOGD("[%d] is to deinit\n", enWin);

    if ((0 == enWin) && (_hV3dhandle != MI_HANDLE_NULL)) {
        Wrapper_Disable3D(enWin);

        MI_V3D_UnRegisterCallback(_hV3dhandle, E_MI_V3D_V3DCALLBACK_EVENT_POST_3D_STATUS_CHANGE);
        MI_V3D_UnRegisterCallback(_hV3dhandle, E_MI_V3D_V3DCALLBACK_EVENT_PRE_3D_STATUS_CHANGE);
        MI_V3D_Close(_hV3dhandle);
        _hV3dhandle = MI_HANDLE_NULL;
        MI_V3D_DeInit();
        bMVC_Enable3D = FALSE;
    }
    MI_DISPLAY_DisconnectWindowInput(_hWinHandle[enWin]);
    MI_DISPLAY_CloseWindowHandle(_hWinHandle[enWin]);
    ALOGD("hVdecHandle[enWin] = %ld\n", hVdecHandle[enWin]);
    MI_VIDEO_Close(hVdecHandle[enWin]);
    bVideoModeSet[enWin] = FALSE;
    hVdecHandle[enWin] = MI_HANDLE_NULL;

    if (hVdecHandle[0] == MI_HANDLE_NULL && hVdecHandle[1] == MI_HANDLE_NULL) {
        ALOGD("all deinit!!!\n");
        if (pPictureManager != NULL) {
            pPictureManager->disconnect();
            pPictureManager.clear();
        }
        if (mi_check_Pannel4K2K() == TRUE) {
            MI_DISPLAY_OutputTiming_e eOutTiming = E_MI_DISPLAY_OUTPUT_TIMING_AUTO;
            MI_DISPLAY_VideoTiming_t stVideoTiming;
            INIT_ST(stVideoTiming);
            stVideoTiming.eVideoTiming = eOutTiming;
            MI_DISPLAY_SetOutputTiming(_hDisplay0Handle, &stVideoTiming);
        }

        if (_hDisplay0Handle != MI_HANDLE_NULL) {
            MI_DISPLAY_UnRegisterDisplayCallback(_hDisplay0Handle,E_MI_DISPLAY_DISPLAYCALLBACK_EVENT_ALL);
            MI_DISPLAY_Close(_hDisplay0Handle);
            _hDisplay0Handle = MI_HANDLE_NULL;
        }
        MI_DISPLAY_DeInit();
        if (_hDispoutHandle != MI_HANDLE_NULL) {
            MI_DISPOUT_Close(_hDispoutHandle);
            _hDispoutHandle = MI_HANDLE_NULL;
        }
        MI_DISPOUT_DeInit();
        MI_VIDEO_DeInit();
    }
}

extern "C" void Wrapper_connect()
{
    ALOGD("Wrapper_connect IN.\n");
#ifdef BUILD_FOR_STB
    if (pPictureManager == NULL) {
        pPictureManager = PictureManager::connect();
        if (pPictureManager == NULL) {
            ALOGE("Fail to connect service mstar.PictureManager");
        } else {
            sp<PicMgrListener> listener = new PicMgrListener();
            pPictureManager->setListener(listener);
        }
    }
#endif
    e4K2KMode = E_4K2K_MODE_LOCK_OFF;
    return;
}
extern "C"  void Wrapper_open_resource(int enWin)
{
    MI_VIDEO_PlayerAttr_t stPlayerAttr;
    MI_RESULT eRet;

    _mi_display_init();

    INIT_ST(stPlayerAttr);
    stPlayerAttr.eVidStreamType = E_MI_VIDEO_STREAM_ES;
    stPlayerAttr.eDecoderPath = E_MI_VIDEO_DECODER_MAIN;
    stPlayerAttr.bEnableDosMode = TRUE;
    if (hVdecHandle[enWin] == MI_HANDLE_NULL) {
        if (enWin == 1) {
            stPlayerAttr.eDecoderPath = E_MI_VIDEO_DECODER_SUB;
        }
        eRet = MI_VIDEO_Open(&stPlayerAttr, &hVdecHandle[enWin]);
        if (eRet != MI_OK) {
            ALOGE("[%d]:Fail to to MI_VIDEO_Open.\n", enWin);
        }
        eRet = MI_DISPLAY_GetWindowHandle(enWin,_hDisplay0Handle, &_hWinHandle[enWin]);
        if (eRet != MI_OK) {
            ALOGE("[%d]:Fail to to MI_DISPLAY_GetWindowHandle.\n", enWin);
            eRet = MI_DISPLAY_OpenWindowHandle(enWin, _hDisplay0Handle, &_hWinHandle[enWin]);
        }
        if (eRet != MI_OK) {
            ALOGE("[%d]:Fail to to MI_DISPLAY_OpenWindowHandle.\n", enWin);
        }
    } else {
        ALOGE("[%d]:hVdecHandle is opened\n", enWin);
    }
}
void Wrapper_open_resource(int enWin, MS_WINDOW *win)
{

}

extern "C"  void Wrapper_enable_window(int enWin, MS_WINDOW *win)
{
    if(hVdecHandle[enWin] == NULL || _hWinHandle[enWin] == NULL)
    {
        ALOGE("Wrapper_enable_window err\n");
        return;
    }
    MI_DISPLAY_ConnectWindowInput(_hWinHandle[enWin],hVdecHandle[enWin]);
    MI_RESULT eRet = MI_ERR_FAILED;

    MI_Display_WindowCallbackParams_t hdrCallbackParams;
    memset(&hdrCallbackParams, 0, sizeof(hdrCallbackParams));
    hdrCallbackParams.u32EventFlags = E_MI_DISPLAY_WINDOWCALLBACK_EVENT_HDR;
    hdrCallbackParams.pfEventCallback = _mi_hdr_callback;
    MI_DISPLAY_RegisterWindowCallback(_hWinHandle[enWin], &hdrCallbackParams);

    if (enWin == 0) {
        MI_V3D_HANDLE hV3d = MI_HANDLE_NULL;
        eRet = mi_get_v3d_handle(enWin, &hV3d);
        if (eRet == MI_OK) {
            MI_V3D_CallbackParams_t stCallbackParams;
            memset(&stCallbackParams, 0, sizeof(stCallbackParams));
            enum {
                 E_VID_EVENT_3D_ON,
                 E_VID_EVENT_3D_OFF,
                 E_VID_EVENT_3D_NUM,
            };
            static MI_V3D_3D_Status_t _gstV3dCallbackData[E_VID_EVENT_3D_NUM];
            memset(&_gstV3dCallbackData[E_VID_EVENT_3D_OFF], 0, sizeof(_gstV3dCallbackData[E_VID_EVENT_3D_OFF]));
            stCallbackParams.pUsrParam = &_gstV3dCallbackData[E_VID_EVENT_3D_OFF];
            stCallbackParams.u32EventFlags = E_MI_V3D_V3DCALLBACK_EVENT_POST_3D_STATUS_CHANGE;
            stCallbackParams.pfEventCallback = _mi_3D_callback;
            eRet = MI_V3D_RegisterCallback(hV3d, &stCallbackParams);
            if (MI_OK != eRet) {
                ALOGE("Fail to MI_V3D_RegisterCallback() eRet: %d.\n", eRet);
            } else {
                ALOGD("Sucess to MI_V3D_RegisterCallback(). event flags : %d \n",stCallbackParams.u32EventFlags);
            }

            memset(&_gstV3dCallbackData[E_VID_EVENT_3D_ON], 0, sizeof(_gstV3dCallbackData[E_VID_EVENT_3D_ON]));
            stCallbackParams.pUsrParam = &_gstV3dCallbackData[E_VID_EVENT_3D_ON];
            stCallbackParams.u32EventFlags = E_MI_V3D_V3DCALLBACK_EVENT_PRE_3D_STATUS_CHANGE;
            stCallbackParams.pfEventCallback = _mi_3D_callback;
            eRet = MI_V3D_RegisterCallback(hV3d, &stCallbackParams);
            if (MI_OK != eRet) {
                ALOGE("Fail to MI_V3D_RegisterCallback(): eRet: %d.\n", eRet);
            } else {
                ALOGE("Sucess to MI_V3D_RegisterCallback(). event flags : %d \n",stCallbackParams.u32EventFlags);
            }
            MI_BOOL bV3dEnable = FALSE;
            if (mi_get_v3d_status(enWin, &bV3dEnable) == MI_OK) {
                if (bV3dEnable == TRUE) {
                    mi_enable_ds = FALSE;
                } else {
                    mi_enable_ds = TRUE;
                }
            }
        } else {
            mi_enable_ds = TRUE;
        }
    }

    if (enWin == 1) {
        MI_DISPLAY_VidWin_Rect_t stDispRect;
        stDispRect.u16X = win->x;
        stDispRect.u16Y = win->y;
        stDispRect.u16Width = win->width;
        stDispRect.u16Height = win->height;
        MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
        MI_DISPLAY_UpdateWindowOutputRect(_hWinHandle[enWin], &stDispRect);
        MI_DISPLAY_ApplyWindowRect(_hWinHandle[enWin]);
        MI_DISPLAY_DisableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
    }
    MI_DISPLAY_EnableWindow(_hWinHandle[enWin]);
}

static void update_current_mvop_timing(int enWin)
{
    MVOP_Timing stMVOPTiming;

    if (enWin) {
        MDrv_MVOP_SubGetOutputTiming(&stMVOPTiming);
    } else {
        MDrv_MVOP_GetOutputTiming(&stMVOPTiming);
    }
    MVOP_width[enWin] = stMVOPTiming.u16Width;
    MVOP_height[enWin] = stMVOPTiming.u16Height;

    #ifdef UFO_MVOP_GET_HV_RATIO
    MVOP_Handle stMvopHd;
    MVOP_OutputHVRatio stOutputHVRatio = {0,};

    if (enWin == 0) {
        stMvopHd.eModuleNum = E_MVOP_MODULE_MAIN;
    } else {
        stMvopHd.eModuleNum = E_MVOP_MODULE_SUB;
    }

    if ((E_MVOP_OK == MDrv_MVOP_GetCommand(&stMvopHd, (MVOP_Command)E_MVOP_CMD_GET_OUTPUT_HV_RATIO,
        &stOutputHVRatio, sizeof(MVOP_OutputHVRatio)))
        && stOutputHVRatio.fHratio && stOutputHVRatio.fVratio) {

        MVOP_width[enWin] = MVOP_width[enWin] / stOutputHVRatio.fHratio;
        MVOP_height[enWin] = MVOP_height[enWin] / stOutputHVRatio.fVratio;
    }
    ALOGD("MVOP timing is %dx%d [%f %f]", MVOP_width[enWin], MVOP_height[enWin],
        stOutputHVRatio.fHratio, stOutputHVRatio.fVratio);
    #else
    ALOGD("MVOP timing is %dx%d", MVOP_width[enWin], MVOP_height[enWin]);
    #endif
}

extern "C"  int Wrapper_get_resource(int enWin, Wrapper_control_resource_st *pstCtrlRes)
{
    int eRet = -1;
    if ((NULL != pstCtrlRes)  && (MI_HANDLE_NULL != hVdecHandle[enWin])){
        pstCtrlRes->hVdec    = hVdecHandle[enWin];
        pstCtrlRes->hDisplay =  _hDisplay0Handle;
        pstCtrlRes->hWin      = _hWinHandle[enWin];
        eRet = 0;
        traceD("hVdec: 0x%x, hDisplay: 0x%x, hWin: 0x%x.\n", pstCtrlRes->hVdec, pstCtrlRes->hDisplay, pstCtrlRes->hWin);
    } else {
        ALOGE("Fail to Wrapper_get_resource.\n");
    }

    return eRet;
}
extern "C" void Wrapper_attach_callback(E_CALLBACK_MODE eType, void(*pfn)(int))
{
    switch (eType)
    {
        case E_CALLBACK_ASPECT_RATIO:
            fpSetAspectRatio = pfn;
            break;
        case E_CALLBACK_ENABLE3D:
            fpEnable3D = pfn;
            break;
        default:
            break;
    }
}

static void reset_window(float Wratio, float Hratio, MS_WINDOW *win, MS_U16 panel_w, MS_U16 panel_h)
{
    // apply to Width
    if (win->x < 0) {
        win->x = 0;
    } else {
        win->x *= Wratio;
    }

    win->width *= Wratio;
    if (win->width > panel_w) {
        win->width = panel_w;
    }

    // apply to Height
    if (win->y < 0) {
        win->y = 0;
    } else {
        win->y *= Hratio;
    }

    win->height *= Hratio;
    if (win->height > panel_h) {
        win->height = panel_h;
    }
}

extern "C" void Wrapper_seq_change(int enWin, MS_DispFrameFormat *dff, int forceDS)
{
    ALOGD("Wrapper_seq_change IN.\n");

    // per PM / XC request, under 23 fps needs set as 30 fps for performance issue.
    dff->u32FrameRate = dff->u32FrameRate > 23000 ? dff->u32FrameRate : 30000;

    VIDEOSET_LOCK;
    // Must disable DS before set MVOP, Otherwise XC maybe dead lock. This is HW issue.
    if (Wrapper_Video_DS_State(enWin)) {
        MI_RESULT eRet = MI_ERR_FAILED;
        MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
        MI_DISPLAY_DsInfo_t stDsInfo;
        INIT_ST(stDsInfo);
        stDsInfo.stVidDispInfo = *((MI_VideoDispInfo *)&SetMode_Vinfo[enWin]);
        stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_OFF;

        eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[enWin], eAttrType, &stDsInfo);
        if (eRet == MI_OK) {
            DS_STATUS_SET(0);
        }

        forceDS = 1;
    }

    MS_FrameFormat *sFrame = &dff->sFrames[MS_VIEW_TYPE_CENTER];

    if (dff->CodecType == MS_CODEC_TYPE_MVC) {
        SetMode_Vinfo[enWin].u16HorSize = SetWin_Vinfo[enWin].u16HorSize = sFrame->u32Width - sFrame->u32CropRight - sFrame->u32CropLeft;
        SetMode_Vinfo[enWin].u16VerSize = SetWin_Vinfo[enWin].u16VerSize = sFrame->u32Height - sFrame->u32CropBottom - sFrame->u32CropTop;
        SetMode_Vinfo[enWin].u16CropRight = SetWin_Vinfo[enWin].u16CropRight = 0;
        SetMode_Vinfo[enWin].u16CropLeft = SetWin_Vinfo[enWin].u16CropLeft = 0;
        SetMode_Vinfo[enWin].u16CropBottom = SetWin_Vinfo[enWin].u16CropBottom = 0;
        SetMode_Vinfo[enWin].u16CropTop = SetWin_Vinfo[enWin].u16CropTop = 0;
        if (dff->u83DMode == MS_3DMODE_SIDEBYSIDE) {
            SetMode_Vinfo[enWin].u8Is3DLayout = 0;
        } else {
            SetMode_Vinfo[enWin].u8Is3DLayout = 1;
        }
        WIN_Initialized[enWin] = 0;
    } else {
        SetMode_Vinfo[enWin].u16HorSize = SetWin_Vinfo[enWin].u16HorSize=  sFrame->u32Width;
        SetMode_Vinfo[enWin].u16VerSize = SetWin_Vinfo[enWin].u16VerSize = sFrame->u32Height;
        SetMode_Vinfo[enWin].u16CropRight = SetWin_Vinfo[enWin].u16CropRight = sFrame->u32CropRight;
        SetMode_Vinfo[enWin].u16CropLeft = SetWin_Vinfo[enWin].u16CropLeft = sFrame->u32CropLeft;
        SetMode_Vinfo[enWin].u16CropBottom = SetWin_Vinfo[enWin].u16CropBottom = sFrame->u32CropBottom;
        SetMode_Vinfo[enWin].u16CropTop = SetWin_Vinfo[enWin].u16CropTop = sFrame->u32CropTop;
    }


    ALOGD("Wrapper_seq_change [%d, %d], u32FrameRate = %d, codectype = %d, u8Interlace = %d", enWin,
        dff->u8SeamlessDS, dff->u32FrameRate, dff->CodecType, dff->u8Interlace);

    SetMode_Vinfo[enWin].u16FrameRate = dff->u32FrameRate;
    SetMode_Vinfo[enWin].enCodecType = dff->CodecType;
    SetWin_Vinfo[enWin].u16FrameRate = dff->u32FrameRate;
    SetWin_Vinfo[enWin].enCodecType = dff->CodecType;

    SetMode_Vinfo[enWin].u8Interlace = dff->u8Interlace;
    SetWin_Vinfo[enWin].u8Interlace = dff->u8Interlace;

    if (forceDS) {
        DS_STATUS_SET(1);
    }

    if (dff->sHDRInfo.u32FrmInfoExtAvail) {
        ALOGD("HDR info: [%d %d %d %d]", dff->sHDRInfo.u32FrmInfoExtAvail,
        dff->sHDRInfo.stColorDescription.u8ColorPrimaries,
        dff->sHDRInfo.stColorDescription.u8TransferCharacteristics,
        dff->sHDRInfo.stColorDescription.u8MatrixCoefficients);
        Wrapper_SetHdrMetadata((int)enWin, &dff->sHDRInfo);
    }

    SetMode_Vinfo[enWin].u8EnableDynamicScaling = DS_Enabled[enWin];
    SetWin_Vinfo[enWin].u8EnableDynamicScaling = DS_Enabled[enWin];
#if SUPPORT_4KDS_CHIP
    if (DS_Enabled[enWin]) {
        VinfoReserve1 *u8Reserve1_mode;
        VinfoReserve1 *u8Reserve1_win;
        u8Reserve1_mode = (VinfoReserve1 *)&SetMode_Vinfo[enWin].u8Reserve[1];
        u8Reserve1_win = (VinfoReserve1 *)&SetWin_Vinfo[enWin].u8Reserve[1];

        if (!dff->u8Interlace) {
            u8Reserve1_mode->ds_4k_mode = 1;
            u8Reserve1_win->ds_4k_mode = 1;
            SetMode_Vinfo[enWin].u16HorSize = (SetMode_Vinfo->u16HorSize > 3840) ? SetMode_Vinfo->u16HorSize : 3840;
            SetMode_Vinfo[enWin].u16VerSize = (SetMode_Vinfo->u16VerSize > 2160) ? SetMode_Vinfo->u16VerSize : 2160;
        } else {
            u8Reserve1_mode->ds_4k_mode = 0;
            u8Reserve1_win->ds_4k_mode = 0;
            SetMode_Vinfo[enWin].u16HorSize = (SetMode_Vinfo->u16HorSize > 1920) ? SetMode_Vinfo->u16HorSize : 1920;
            SetMode_Vinfo[enWin].u16VerSize = (SetMode_Vinfo->u16VerSize > 1088) ? SetMode_Vinfo->u16VerSize : 1088;
        }
    }
#endif
    Wrapper_Video_setMode(&SetMode_Vinfo[enWin], enWin);

    if (DS_Enabled[enWin]) {
        update_current_mvop_timing(enWin);
    }

    MI_RESULT eRet;
    MI_DISPLAY_VidWin_Rect_t stRect;
    MS_WINDOW DispWin;
    MS_WINDOW CropWin;
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
         SetMode_Vinfo[enWin].u16HorSize = SetWin_Vinfo[enWin].u16HorSize = sFrame->u32Width;
         SetMode_Vinfo[enWin].u16VerSize = SetWin_Vinfo[enWin].u16VerSize = sFrame->u32Height;
         SetMode_Vinfo[enWin].u16CropRight = SetWin_Vinfo[enWin].u16CropRight = 0;
         SetMode_Vinfo[enWin].u16CropLeft = SetWin_Vinfo[enWin].u16CropLeft = 0;
         SetMode_Vinfo[enWin].u16CropBottom = SetWin_Vinfo[enWin].u16CropBottom = 0;
         SetMode_Vinfo[enWin].u16CropTop = SetWin_Vinfo[enWin].u16CropTop = 0;
        } else {
         SetMode_Vinfo[enWin].u16HorSize = SetWin_Vinfo[enWin].u16HorSize=  sFrame->u32Width;
         SetMode_Vinfo[enWin].u16VerSize = SetWin_Vinfo[enWin].u16VerSize = sFrame->u32Height;
         SetMode_Vinfo[enWin].u16CropRight = SetWin_Vinfo[enWin].u16CropRight = SetMode_Vinfo[enWin].u16HorSize - (x + width);
         SetMode_Vinfo[enWin].u16CropLeft = SetWin_Vinfo[enWin].u16CropLeft = x;
         SetMode_Vinfo[enWin].u16CropBottom = SetWin_Vinfo[enWin].u16CropBottom = SetMode_Vinfo[enWin].u16VerSize - (y + height);
         SetMode_Vinfo[enWin].u16CropTop = SetWin_Vinfo[enWin].u16CropTop = y;
        }
    }

    DispWin.x = VideoWin[enWin].x;
    DispWin.y = VideoWin[enWin].y;
    DispWin.width = VideoWin[enWin].width;
    DispWin.height = VideoWin[enWin].height;
    CropWin.x = SetWin_Vinfo[enWin].u16CropLeft;
    CropWin.y = SetWin_Vinfo[enWin].u16CropTop;
    CropWin.width = SetWin_Vinfo[enWin].u16HorSize - SetWin_Vinfo[enWin].u16CropLeft - SetWin_Vinfo[enWin].u16CropRight;
    CropWin.height = SetWin_Vinfo[enWin].u16VerSize - SetWin_Vinfo[enWin].u16CropTop - SetWin_Vinfo[enWin].u16CropBottom;

    vsync_bridge_CalAspectRatio(&DispWin, &CropWin, dff, TRUE, FALSE, 0, 0);

    XCWin[enWin].x = DispWin.x;
    XCWin[enWin].y = DispWin.y;
    XCWin[enWin].width = DispWin.width;
    XCWin[enWin].height = DispWin.height;
    SetWin_Vinfo[enWin].u16CropLeft = CropWin.x;
    SetWin_Vinfo[enWin].u16CropTop = CropWin.y;
    SetWin_Vinfo[enWin].u16HorSize = CropWin.width + CropWin.x;
    SetWin_Vinfo[enWin].u16VerSize = CropWin.height + CropWin.y;
    SetWin_Vinfo[enWin].u16CropRight = 0;
    SetWin_Vinfo[enWin].u16CropBottom = 0;

    if ((!dff->u8SeamlessDS) || forceDS) {
        WIN_Initialized[enWin] = 0;
        stRect.u16X = CropWin.x;
        stRect.u16Y = CropWin.y;
        stRect.u16Width = CropWin.width;
        stRect.u16Height= CropWin.height;
        if (dff->CodecType == MS_CODEC_TYPE_MVC) {
            if (SetMode_Vinfo[enWin].u8Is3DLayout == 0) {
                stRect.u16Width = CropWin.width * 2;
            } else {
                stRect.u16Height = CropWin.height * 2;
            }
        }
        eRet = MI_DISPLAY_UpdateWindowInputRect(_hWinHandle[enWin], &stRect);
        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_SetInputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }
        showCropWin(stRect);

        stRect.u16X = DispWin.x;
        stRect.u16Y =  DispWin.y;
        stRect.u16Width = DispWin.width;
        stRect.u16Height = DispWin.height;
        MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);

        eRet = MI_DISPLAY_UpdateWindowOutputRect(_hWinHandle[enWin], &stRect);
        if (eRet != MI_OK) {
                ALOGE("Fail to MI_DISP_SetOutputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }

        eRet = MI_DISPLAY_ApplyWindowRect(_hWinHandle[enWin]);
        if (eRet != MI_OK) {
                ALOGE("Fail to MI_DISP_ApplySetting.\n");
        }
        MI_DISPLAY_DisableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
        WIN_Initialized[enWin] = 1;
    }
    VIDEOSET_UNLOCK;
}

extern "C" void Wrapper_disconnect(int enWin)
{
    MI_RESULT eRet = MI_ERR_FAILED;
    VIDEOSET_LOCK;

    MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
    if (DS_Enabled[enWin] == 1) {
        MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
        MI_DISPLAY_DsInfo_t stDsInfo;
        INIT_ST(stDsInfo);
        stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_OFF;
        stDsInfo.stVidDispInfo = *((MI_VideoDispInfo *)&SetMode_Vinfo[enWin]);
        eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[enWin], eAttrType, &stDsInfo);
        if (eRet == MI_OK) {
            ALOGD("%s @ line: %d: Enter Stop ds.\n", __func__, __LINE__);
        }
        DS_STATUS_SET(0);
    } else {
        ALOGD("%s @ line: %d: Ds already Stop.\n", __func__, __LINE__);
    }
    e4K2KMode = E_4K2K_MODE_LOCK_OFF;
    WIN_Initialized[enWin] = 0;
    mi_demo_deinit(enWin);
    State[enWin] = 0;
    Mute_Status[enWin] = 0;
    VIDEOSET_UNLOCK;
}

extern "C" int Wrapper_Video_setMode(MS_VideoInfo *info, int enWin)
{
    int ret = -1;

    // per PM / XC request, under 23 fps needs set as 30 fps for performance issue.
    info->u16FrameRate = info->u16FrameRate > 23000 ? info->u16FrameRate : 30000;

    ALOGD("Wrapper_Video_setMode %d, DS: %d , 3D: %d ", enWin, info->u8EnableDynamicScaling,info->u8Is3DLayout);
    used[enWin] = 1;
    if (info->u8EnableDynamicScaling) {
        DS_STATUS_SET(1);
    }

    if (pPictureManager != NULL) {
        if (info->u16HorSize >= 3840 && info->u16VerSize >= 1080) {
            pPictureManager->disableAllDualWinMode();
        }
    }
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&info->u8Reserve[0];
    MI_VIDEO_DispInfo_t stDispInfo;
    //INIT_ST(stDispInfo);
    MI_RESULT eRet = MI_ERR_FAILED;

    MI_U16 u16HorSize = info->u16HorSize;
    MI_U16 u16VerSize = info->u16VerSize;

    if (info->enCodecType== E_MI_VIDEO_CODEC_TYPE_MVC && info->u8Is3DLayout==0) {
        u16HorSize = info->u16HorSize*2;
        u16VerSize = info->u16VerSize;
    }

    if ((e4K2KMode == E_4K2K_MODE_LOCK_OFF)  && (mi_check_Pannel4K2K() == TRUE)) {
        e4K2KMode = E_4K2K_MODE_LOCK_ON;
        MI_DISPLAY_OutputTiming_e eOutTiming = E_MI_DISPLAY_OUTPUT_TIMING_3840X2160_30P;
        MI_DISPLAY_VideoTiming_t stVideoTiming;
        INIT_ST(stVideoTiming);
        if((info->u16FrameRate == 10000) && info->enCodecType == E_MI_VIDEO_CODEC_TYPE_MJPEG)  //If decode photo , set4k2kmode anyway
        {
            stVideoTiming.eVideoTiming = eOutTiming;
            MI_DISPLAY_SetOutputTiming(_hDisplay0Handle, &stVideoTiming);
        }
        else if(u16HorSize > WIDTH_2K)
        {
            if (info->u8EnableDynamicScaling)
            {
                info->u8EnableDynamicScaling = 0;
                DS_STATUS_SET(0);
            }
            if(info->u16VerSize>=HEIGHT_2K && info->enCodecType == E_MI_VIDEO_CODEC_TYPE_MJPEG)
            { //It must be a 4K2K photo
                stVideoTiming.eVideoTiming = eOutTiming;
                MI_DISPLAY_SetOutputTiming(_hDisplay0Handle, &stVideoTiming);
                //MSrv_Control::GetMSrvVideo()->ForceFreerun(TRUE, FALSE);
            }
            else
            {
                eOutTiming = E_MI_DISPLAY_OUTPUT_TIMING_3840X2160_30P;
                stVideoTiming.eVideoTiming = eOutTiming;
                MI_DISPLAY_SetOutputTiming(_hDisplay0Handle, &stVideoTiming);
            }
        } else {
            eOutTiming = E_MI_DISPLAY_OUTPUT_TIMING_1920X1080_60P;
            stVideoTiming.eVideoTiming = eOutTiming;
            MI_DISPLAY_SetOutputTiming(_hDisplay0Handle, &stVideoTiming);
            if (info->enCodecType == E_MI_VIDEO_CODEC_TYPE_MJPEG) {
                //MSrv_Control::GetMSrvVideo()->ForceFreerun(TRUE, FALSE);
            }
        }
    } else {
        ALOGI("Skip 4K2K Pannel setting .\n");
    }

    VINFO_TO_MI(stDispInfo, info);
    if ((mi_enable_ds == TRUE) && (info->u8EnableDynamicScaling == 1)) {
        if (IS_4K_DS_MODE(info)) {
            stDispInfo.u16HorSize = (info->u16HorSize > 3840) ? info->u16HorSize : 3840;
            stDispInfo.u16VerSize = (info->u16VerSize > 2160) ? info->u16VerSize : 2160;
        }
        else {
            stDispInfo.u16HorSize = (info->u16HorSize > 1920) ? info->u16HorSize : 1920;
            stDispInfo.u16VerSize = (info->u16VerSize > 1088) ? info->u16VerSize : 1088;
        }
    }
    eRet = MI_VIDEO_SetMode(hVdecHandle[enWin], &stDispInfo);
    if (mi_enable_ds == TRUE) {
        if (eRet == MI_OK)
        {
            ret = 1;
            if (DS_Enabled[enWin] == 1) {
                MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
                MI_DISPLAY_DsInfo_t stDsInfo;
                INIT_ST(stDsInfo);
                ALOGD("%s @ line: %d: Enter Start ds.\n", __func__, __LINE__);
                stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_ON;
                stDsInfo.stVidDispInfo = *((MI_VideoDispInfo*)info);
                eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[enWin], eAttrType, &stDsInfo);
                if (eRet != MI_OK) {
                    ret = 0;
                }
            } else if (DS_Enabled[enWin] == 0) {
                MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
                MI_DISPLAY_DsInfo_t stDsInfo;
                INIT_ST(stDsInfo);
                ALOGD("%s @ line: %d: Enter Stop ds.\n", __func__, __LINE__);
                stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_OFF;
                stDsInfo.stVidDispInfo = *((MI_VideoDispInfo*)info);
                eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[enWin], eAttrType, &stDsInfo);
                if (eRet != MI_OK) {
                    ret = 0;
                }
            }
        }
    }

    if ((info->enCodecType == E_MI_VIDEO_CODEC_TYPE_MVC) && (enWin == 0)) {
        // MVC with no 3D Layout, change output as output side by side (maybe 4K2K video)
        if (info->u8Is3DLayout == 0) {
            bMVC_Enable3D = FALSE;
        } else {
            Wrapper_Enable3D(enWin);
        }
    }

    if (info->u8EnableDynamicScaling) {

        update_current_mvop_timing(enWin);

        if (IS_4K_DS_MODE(info)
            && (MVOP_width[enWin] < 3840)
            && (MVOP_height[enWin] < 2160)) {
            // If we set 4K DS to SN, but the MVOP timing is not 4K.
            // It is 2 state DS due to it connect to special ursa
            // This case:
            //   If video under FHD, it will run FHD DS, XC output timing is FHD.
            //   If video larger than FHD, it will run 4K DS. XC output timing is 4K.
            b2StageDS = 1;
        } else {
            b2StageDS = 0;
        }
    }

    MI_VIDEO_CodecParam_t stVidCodecParam;
    memset(&stVidCodecParam,0,sizeof(MI_VIDEO_CodecParam_t));
    stVidCodecParam.attr.stPrivateCodecData.u16Width = info->u16HorSize;
    stVidCodecParam.attr.stPrivateCodecData.u16Height = info->u16VerSize;
    stVidCodecParam.attr.stPrivateCodecData.u32FrameRate = info->u16FrameRate;
    if (MI_VIDEO_SetCodecParam(hVdecHandle[enWin], &stVidCodecParam) != MI_OK)
        ALOGE("ERR: MI_VIDEO_SetCodecParam fail \n");

    bVideoModeSet[enWin] = TRUE;
    ret = 1;
    return ret;
}

extern "C" int Wrapper_Video_setXCWin(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio)
{
    int ret = -1;
    bool ResetXC = false;
    // For MI setXCWin is unnecessary after open 3D return to prevent screen blink
    MI_BOOL bV3dEnable = FALSE;

    if (info == NULL) {
        return ret;
    }

    if (WIN_Initialized[enWin] == 1) {
        if (mi_get_v3d_status(enWin, &bV3dEnable) == MI_OK) {
            if (bV3dEnable == TRUE) {
                return ret;
            }
        }
    }
    MS_VideoInfo vinfo;
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&SetWin_Vinfo[enWin].u8Reserve[0];

    memcpy(&vinfo, info, sizeof(MS_VideoInfo));
    ALOGD("Wrapper_Video_setXCWin IN.\n");

    ALOGD("Wrapper_Video_setXCWin %d", enWin);

    VIDEOSET_LOCK;

    if (!used[enWin]) {
        used[enWin] = 1;

        if (info->u8EnableDynamicScaling) {
            DS_STATUS_SET(1);
        }

        memcpy(&SetMode_Vinfo[enWin], info, sizeof(MS_VideoInfo));
    }

    // Set CropWindow should use Correct value for all case.
    MS_WINDOW DispWin;

    DispWin.x = x;
    DispWin.y = y;
    DispWin.width = width;
    DispWin.height = height;
    x = DispWin.x;
    y = DispWin.y;
    width = DispWin.width;
    height = DispWin.height;

    info->u8AspectRatio = 0;    // temp set 0, can remove if SN is update

    if ((SetWin_Vinfo[enWin].u16CropLeft != vinfo.u16CropLeft)
        || (SetWin_Vinfo[enWin].u16CropRight != vinfo.u16CropRight)
        || (SetWin_Vinfo[enWin].u16CropTop != vinfo.u16CropTop)
        || (SetWin_Vinfo[enWin].u16CropBottom != vinfo.u16CropBottom)
        || (SetWin_Vinfo[enWin].u16HorSize != vinfo.u16HorSize)
        || (SetWin_Vinfo[enWin].u16VerSize != vinfo.u16VerSize)) {
        ResetXC = true;
    }

    memcpy(&SetWin_Vinfo[enWin], &vinfo, sizeof(MS_VideoInfo));
    videoflags->no_lock = 0;    //Cannot store this flag. It should be used case by case.

    if ((XCWin[enWin].x != x) || (XCWin[enWin].y != y) || (XCWin[enWin].width != width) || (XCWin[enWin].height != height)) {
        XCWin[enWin].x = x;
        XCWin[enWin].y = y;
        XCWin[enWin].width = width;
        XCWin[enWin].height = height;
        ResetXC = true;
    }
    if (WIN_Initialized[enWin] && (ResetXC == false)) {
        VIDEOSET_UNLOCK;
        return 0;
    }
    MI_RESULT eRet;
    MI_DISPLAY_VidWin_Rect_t stRect;

    MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);

    if ((TRUE == bVideoModeSet[enWin]) || (Wrapper_Video_setMode(&vinfo, enWin) != 0)) {
        stRect.u16X = vinfo.u16CropLeft;
        stRect.u16Y = vinfo.u16CropTop;
        stRect.u16Width = vinfo.u16HorSize - vinfo.u16CropLeft - vinfo.u16CropRight;
        stRect.u16Height = vinfo.u16VerSize - vinfo.u16CropTop - vinfo.u16CropBottom;
        if (info->enCodecType == E_MI_VIDEO_CODEC_TYPE_MVC) {
            if (info->u8Is3DLayout == 0) {
                stRect.u16Width = (vinfo.u16HorSize - vinfo.u16CropLeft - vinfo.u16CropRight) * 2;
            } else {
                stRect.u16Height = (vinfo.u16VerSize - vinfo.u16CropTop - vinfo.u16CropBottom) * 2;
            }
        }
        eRet = MI_DISPLAY_UpdateWindowInputRect(_hWinHandle[enWin], &stRect);
        showCropWin(stRect);

        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_SetInputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }
        stRect.u16X = x;
        stRect.u16Y =  y;
        stRect.u16Width = width;
        stRect.u16Height = height;
        eRet = MI_DISPLAY_UpdateWindowOutputRect(_hWinHandle[enWin], &stRect);
        ALOGD("DisplayWindow (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);

        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_SetInputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }
        eRet = MI_DISPLAY_ApplyWindowRect(_hWinHandle[enWin]);

        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_ApplySetting.\n");
        }

        ret = 0;
    }

    MI_DISPLAY_DisableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);

    WIN_Initialized[enWin] = 1;
    VIDEOSET_UNLOCK;

    return ret;
}

extern "C" int Wrapper_Video_disableDS(int enWin, int Mute)
{
    int ret = -1;

    VIDEOSET_LOCK;
    if (pVideoSet != NULL) {
        ALOGD("Wrapper_Video_disableDS %d ", enWin);
        ret = pVideoSet->video_disableDS(enWin, Mute);
        if (ret == 0)   DS_STATUS_SET(0);
    }
    ALOGD("Wrapper_Video_disableDS %d, DS_STATUS %d", enWin, Mute);
    if (mi_enable_ds == TRUE) {
        MI_RESULT eRet = MI_ERR_FAILED;
        MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
        MI_DISPLAY_DsInfo_t stDsInfo;
        INIT_ST(stDsInfo);
        stDsInfo.stVidDispInfo = *((MI_VideoDispInfo *)&SetMode_Vinfo[enWin]);
        if (DS_Enabled[enWin] == TRUE) {
            stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_OFF;
        } else if ((DS_Enabled[enWin] == FALSE) && (Mute == 1) && (SetMode_Vinfo[enWin].u8EnableDynamicScaling == 1)) {
            stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_RESTORE;
        } else {
            VIDEOSET_UNLOCK;
            ret = 1;
            return ret;
        }
        MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
        eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[enWin], eAttrType, &stDsInfo);
        if (eRet == MI_OK) {
            if (Mute == 0) {
                DS_STATUS_SET(0);
            } else if (Mute == 1) {
                DS_STATUS_SET(1);
            }
            ret = 0;
        }
        MS_VideoInfo vinfo;
        memcpy(&vinfo, &SetMode_Vinfo[0], sizeof(MS_VideoInfo));
        MI_VIDEO_DispInfo_t stSetMode;
        VINFO_TO_MI(stSetMode, &vinfo);
        if (stDsInfo.eDsStatus == E_MI_DISPLAY_DS_STATUS_RESTORE) {
            if (IS_4K_DS_MODE(&vinfo)) {
                stSetMode.u16HorSize = (vinfo.u16HorSize > 3840) ? vinfo.u16HorSize : 3840;
                stSetMode.u16VerSize = (vinfo.u16VerSize > 2160) ? vinfo.u16VerSize : 2160;
            }
            else {
                stSetMode.u16HorSize = (vinfo.u16HorSize > 1920) ? vinfo.u16HorSize : 1920;
                stSetMode.u16VerSize = (vinfo.u16VerSize > 1088) ? vinfo.u16VerSize : 1088;
            }
        }
        eRet = MI_VIDEO_SetMode(hVdecHandle[enWin], &stSetMode);
        if (MI_OK != eRet) {
            ALOGE("Fail to set mode.\n");
        }

        MI_DISPLAY_VidWin_Rect_t stRect;
        stRect.u16X = SetWin_Vinfo[enWin].u16CropLeft;
        stRect.u16Y = SetWin_Vinfo[enWin].u16CropTop;
        stRect.u16Width = SetWin_Vinfo[enWin].u16HorSize - SetWin_Vinfo[enWin].u16CropLeft - SetWin_Vinfo[enWin].u16CropRight;
        stRect.u16Height = SetWin_Vinfo[enWin].u16VerSize - SetWin_Vinfo[enWin].u16CropTop - SetWin_Vinfo[enWin].u16CropBottom;

        if (SetWin_Vinfo[enWin].enCodecType == E_MI_VIDEO_CODEC_TYPE_MVC) {
            if (SetWin_Vinfo[enWin].u8Is3DLayout == 0) {
                stRect.u16Width = (SetWin_Vinfo[enWin].u16HorSize - SetWin_Vinfo[enWin].u16CropLeft - SetWin_Vinfo[enWin].u16CropRight) * 2;
            } else {
                stRect.u16Height = (SetWin_Vinfo[enWin].u16VerSize - SetWin_Vinfo[enWin].u16CropTop - SetWin_Vinfo[enWin].u16CropBottom) * 2;
            }
        }
        eRet = MI_DISPLAY_UpdateWindowInputRect(_hWinHandle[enWin], &stRect);
        showCropWin(stRect);

        if (eRet != MI_OK) {
            ALOGE("line: %d, Fail to MI_DISP_SetInputRect (%d %d) %d X %d.\n", __LINE__, stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }

        eRet = MI_DISPLAY_ApplyWindowRect(_hWinHandle[enWin]);

        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_ApplySetting.\n");
        }
        MI_DISPLAY_DisableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
    }
    VIDEOSET_UNLOCK;

    return ret;
}

extern "C" int Wrapper_Video_closeDS(int enWin, int Mute, int lock)
{
    int ret = -1;

    // no lock if it is call from postevent
    if (lock) {
        VIDEOSET_LOCK;
    }
    if (pVideoSet != NULL) {
        ALOGD("Wrapper_Video_closeDS %d ", enWin);
        ret = pVideoSet->video_disableDS(enWin, Mute);
        if (ret == 0)   DS_STATUS_SET(0);
    }
    ALOGD("Wrapper_Video_closeDS %d, DS_STATUS %d", enWin, Mute);
    if (mi_enable_ds == TRUE) {
        MI_RESULT eRet = MI_ERR_FAILED;
        MI_DISPLAY_WindowAttrType_e eAttrType = E_MI_DISPLAY_WINDOW_ATTR_DSSTATUS;
        MI_DISPLAY_DsInfo_t stDsInfo;
        INIT_ST(stDsInfo);
        stDsInfo.stVidDispInfo = *((MI_VideoDispInfo *)&SetMode_Vinfo[enWin]);
        if (DS_Enabled[enWin] == TRUE) {
            stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_OFF;
        } else if ((DS_Enabled[enWin] == FALSE) && (Mute == 1) && (SetMode_Vinfo[enWin].u8EnableDynamicScaling == 1)) {
            stDsInfo.eDsStatus = E_MI_DISPLAY_DS_STATUS_RESTORE;
        } else {
            if (lock) {
                VIDEOSET_UNLOCK;
            }
            ret = 1;
            return ret;
        }
        MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
        eRet = MI_DISPLAY_SetWindowAttr(_hWinHandle[enWin], eAttrType, &stDsInfo);
        if (eRet == MI_OK) {
            if (Mute == 0) {
                DS_STATUS_SET(0);
            } else if (Mute == 1) {
                DS_STATUS_SET(1);
            }
            ret = 0;
        }
        MS_VideoInfo vinfo;
        memcpy(&vinfo, &SetMode_Vinfo[0], sizeof(MS_VideoInfo));
        MI_VIDEO_DispInfo_t stSetMode;
        VINFO_TO_MI(stSetMode, &vinfo);
        if (stDsInfo.eDsStatus == E_MI_DISPLAY_DS_STATUS_RESTORE) {
            if (IS_4K_DS_MODE(&vinfo)) {
                stSetMode.u16HorSize = (vinfo.u16HorSize > 3840) ? vinfo.u16HorSize : 3840;
                stSetMode.u16VerSize = (vinfo.u16VerSize > 2160) ? vinfo.u16VerSize : 2160;
            }
            else {
                stSetMode.u16HorSize = (vinfo.u16HorSize > 1920) ? vinfo.u16HorSize : 1920;
                stSetMode.u16VerSize = (vinfo.u16VerSize > 1088) ? vinfo.u16VerSize : 1088;
            }
        }
        eRet = MI_VIDEO_SetMode(hVdecHandle[enWin], &stSetMode);
        if (MI_OK != eRet) {
            ALOGE("Fail to set mode.\n");
        }

        MI_DISPLAY_VidWin_Rect_t stRect;
        stRect.u16X = SetWin_Vinfo[enWin].u16CropLeft;
        stRect.u16Y = SetWin_Vinfo[enWin].u16CropTop;
        stRect.u16Width = SetWin_Vinfo[enWin].u16HorSize - SetWin_Vinfo[enWin].u16CropLeft - SetWin_Vinfo[enWin].u16CropRight;
        stRect.u16Height = SetWin_Vinfo[enWin].u16VerSize - SetWin_Vinfo[enWin].u16CropTop - SetWin_Vinfo[enWin].u16CropBottom;

        if (SetWin_Vinfo[enWin].enCodecType == E_MI_VIDEO_CODEC_TYPE_MVC) {
            if (SetWin_Vinfo[enWin].u8Is3DLayout == 0) {
                stRect.u16Width = (SetWin_Vinfo[enWin].u16HorSize - SetWin_Vinfo[enWin].u16CropLeft - SetWin_Vinfo[enWin].u16CropRight) * 2;
            } else {
                stRect.u16Height = (SetWin_Vinfo[enWin].u16VerSize - SetWin_Vinfo[enWin].u16CropTop - SetWin_Vinfo[enWin].u16CropBottom) * 2;
            }
        }
        eRet = MI_DISPLAY_UpdateWindowInputRect(_hWinHandle[enWin], &stRect);
        showCropWin(stRect);

        if (eRet != MI_OK) {
            ALOGE("line: %d, Fail to MI_DISP_SetInputRect (%d %d) %d X %d.\n", __LINE__, stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
        }

        eRet = MI_DISPLAY_ApplyWindowRect(_hWinHandle[enWin]);

        if (eRet != MI_OK) {
            ALOGE("Fail to MI_DISP_ApplySetting.\n");
        }
        MI_DISPLAY_DisableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEOSETWRAPPER_MUTE);
    }
    if (lock) {
        VIDEOSET_UNLOCK;
    }

    return ret;
}

extern "C" int Wrapper_Video_DS_State(int enWin)
{
    int ret = -1;
    ret = DS_Enabled[enWin];
    return ret;
}

extern "C" void Wrapper_SurfaceVideoSizeChange(int win,int x,int y,int width,int height,int SrcWidth,int SrcHeight, int lock)
{
    MI_RESULT eRet;
    MI_DISPLAY_VidWin_Rect_t stRect;
    stRect.u16X = x;
    stRect.u16Y =  y;
    stRect.u16Width = width;
    stRect.u16Height = height;
    eRet = MI_DISPLAY_UpdateWindowOutputRect(_hWinHandle[win], &stRect);
    ALOGD("Wrapper_SurfaceVideoSizeChange UpdateOutputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);

    if (eRet != MI_OK) {
        ALOGE("Fail to UpdateOutputRect (%d %d) %d X %d.\n", stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height);
    }

    VinfoReserve0 *videoflags = (VinfoReserve0 *)&SetMode_Vinfo[0].u8Reserve[0];
    if (videoflags->application_type != MS_APP_TYPE_IMAGE) {
        VIDEOSET_LOCK;
        XCWin[win].x = x;
        XCWin[win].y = y;
        XCWin[win].width = width;
        XCWin[win].height = height;
        VIDEOSET_UNLOCK;
    }
}

static int mute_lock[2] = {0};
extern "C" void Wrapper_Video_setMute(int enWin, int enable, int lock)
{
    VIDEOSET_LOCK;
    if ((Mute_Status[enWin] == enable)) {
        VIDEOSET_UNLOCK;
        return;
    }
    if (lock) {
        mute_lock[enWin] = 1;   // it means it will un mute later, others can not interrupt
    } else if (mute_lock[enWin]){
        VIDEOSET_UNLOCK;
        return;
    }
    if (enable) {
        if (Mute_Status[enWin] == FALSE) {
            MI_DISPLAY_EnableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEO_MUTE);
            ALOGD("MI Wrapper_Video_setMute, enWin = %d, enable = %d, lock = %d\n", enWin, enable, lock);
            Mute_Status[enWin] = TRUE;
        }
    } else {
        if (Mute_Status[enWin]) {
            MI_DISPLAY_DisableWindowMute(_hWinHandle[enWin], MI_DISPLAY_MUTE_FLAG_VSYNC_BRIDGE_VIDEO_MUTE);
            ALOGD("MI Wrapper_Video_setMute, enWin = %d, enable = %d, lock = %d\n", enWin, enable, lock);
            Mute_Status[enWin] = FALSE;
        }
    }
    if (lock) {
        if (!enable) mute_lock[enWin] = 0;
    }
    VIDEOSET_UNLOCK;
}

extern "C" void Wrapper_get_3DStatus(int enWin, MS_BOOL *pbEnable)
{
    MI_BOOL bEnable = FALSE;
    if (mi_get_v3d_status(enWin, &bEnable) == MI_OK) {
        ALOGD("V3d is Opened.\n");
    } else {
        ALOGE("Fail to get v3d status.\n");
    }
    *pbEnable = bEnable;
}

extern "C" void Wrapper_SetDispWin(int enWin, int x, int y, int width, int height, MS_VideoInfo *info)
{
        VideoWin[enWin].x = x;
        VideoWin[enWin].y = y;
        VideoWin[enWin].width = width;
        VideoWin[enWin].height = height;
        if (info != NULL) {
            memcpy(&SetMode_Vinfo[enWin], info, sizeof(MS_VideoInfo));
        }
}

extern "C" int Wrapper_get_Status(int enWin)
{
    int ret = 0;
    ret = State[enWin];
    return ret;
}

extern "C" int Wrapper_Video_need_seq_change(int enWin, int width, int height)
{
    int ret = 0;
    if ((width > MVOP_width[enWin]) || (height > MVOP_height[enWin])) {
        ret = 1;
    }

    if (b2StageDS && MVOP_width[enWin] >= 3840 && MVOP_height[enWin] >= 2160) {
        if (width <= 1920 && height <= 1088) {
            // trigger seq_change to back to FHD DS
            ret = 1;
        }
    }
    return ret;
}

extern "C" int Wrapper_SetHdrMetadata(int enWin, const MS_HDRInfo *pHdrInfo)
{
    MI_RESULT eRet;
    MI_DISPLAY_Hdr_Metadata_t stHDRMetadata;
    INIT_ST(stHDRMetadata);

    //ALOGD("Wrapper_SetHdrMetadata FrmInfoExtAvail:0x%x\n", pHdrInfo->u32FrmInfoExtAvail);

    if (pHdrInfo == NULL) {
        stHDRMetadata.u16HdrMetadataType |= E_DISPLAY_HDR_METADATA_INVALID;
    } else {
        if (pHdrInfo->u32FrmInfoExtAvail & 0x01) {
            stHDRMetadata.u16HdrMetadataType |= E_DISPLAY_HDR_METATYPE_MPEG_VUI;
            stHDRMetadata.stHdrMetadataMpegVui.u8ColorPrimaries = pHdrInfo->stColorDescription.u8ColorPrimaries;
            stHDRMetadata.stHdrMetadataMpegVui.u8TransferCharacteristics = pHdrInfo->stColorDescription.u8TransferCharacteristics;
            stHDRMetadata.stHdrMetadataMpegVui.u8MatrixCoefficients = pHdrInfo->stColorDescription.u8MatrixCoefficients;
        }

        if (pHdrInfo->u32FrmInfoExtAvail & 0x02) {
            stHDRMetadata.u16HdrMetadataType |= E_DISPLAY_HDR_METADATA_MPEG_SEI_TONE_MAPPING;
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.au16DisplayPrimariesX[0] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[0][0];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.au16DisplayPrimariesY[0] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[0][1];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.au16DisplayPrimariesX[1] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[1][0];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.au16DisplayPrimariesY[1] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[1][1];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.au16DisplayPrimariesX[2] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[2][0];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.au16DisplayPrimariesY[2] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[2][1];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.u16WhitePointX = pHdrInfo->stMasterColorDisplay.u16WhitePoint[0];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.u16WhitePointY = pHdrInfo->stMasterColorDisplay.u16WhitePoint[1];
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.u32MaxDisplayMasteringLuminance = pHdrInfo->stMasterColorDisplay.u32MaxLuminance;
            stHDRMetadata.stHdrMetadataMpegSeiMasteringColorVolume.u32MinDisplayMasteringLuminance = pHdrInfo->stMasterColorDisplay.u32MinLuminance;
        }

        if (pHdrInfo->u32FrmInfoExtAvail & 0x04) {
            #ifdef HDR_METADATA_DOLBY_HDR
            stHDRMetadata.u16HdrMetadataType |= HDR_METADATA_DOLBY_HDR;
            #else
            ALOGE("video_SetHDRMetadata not support HDR_METADATA_DOLBY_HDR");
            #endif
        }
    }

    eRet = MI_DISPLAY_SetHdrMetadata(_hWinHandle[enWin], &stHDRMetadata);
    if (eRet != MI_OK)
    {
        ALOGE("MI_DISPLAY_SetHdrMetadata failed \n ");
        return -1;
    }

    return 0;
}

extern "C" int Wrapper_GetRenderDelayTime(int enWin) {
    int DelayTime = 0;

    return DelayTime;
}

extern "C" void Wrapper_UpdateModeSize(int enWin, int horSize, int verSize, int cropLeft, int cropRight, int cropTop, int cropBottom)
{
    SetMode_Vinfo[enWin].u16HorSize = horSize;
    SetMode_Vinfo[enWin].u16VerSize = verSize;
    SetMode_Vinfo[enWin].u16CropLeft = cropLeft;
    SetMode_Vinfo[enWin].u16CropRight = cropRight;
    SetMode_Vinfo[enWin].u16CropTop = cropTop;
    SetMode_Vinfo[enWin].u16CropBottom = cropBottom;
}

extern "C" void Wrapper_Enable3D(int enWin)
{
    ALOGD("enable 3D\n");
#if(MHDMITX_ENABLE == 1)
    if (mi_set_hdmi_3d_format(E_MI_DISPOUT_3D_FRAME_PACKING))
#endif
    {
        sp<ThreeDimensionManager> pThreeDimensionManager = NULL;
        pThreeDimensionManager = ThreeDimensionManager::connect();
        if (pThreeDimensionManager == NULL) {
            ALOGE("Fail to connect service mstar.3DManager");
#if(MHDMITX_ENABLE == 1)
            mi_set_hdmi_3d_format(E_MI_DISPOUT_3D_NOT_IN_USE);
#endif
        } else {
            pThreeDimensionManager->enable3d(E_MI_V3D_TOP_BOTTOM);
            pThreeDimensionManager->disconnect();
            pThreeDimensionManager.clear();
            bMVC_Enable3D = TRUE;
        }
    }
}

extern "C" void Wrapper_Disable3D(int enWin)
{
    ALOGD("disable 3D\n");
    if ((0 == enWin) && (_hV3dhandle != MI_HANDLE_NULL)) {
        if (bMVC_Enable3D == TRUE) {
#ifdef BUILD_FOR_STB
            sp<ThreeDimensionManager> pThreeDimensionManager = NULL;
            pThreeDimensionManager = ThreeDimensionManager::connect();
            if (pThreeDimensionManager == NULL) {
                ALOGE("Fail to connect service mstar.3DManager");
            } else {
                pThreeDimensionManager->enable3d(E_MI_V3D_NONE);
                pThreeDimensionManager->disconnect();
                pThreeDimensionManager.clear();
            }
#endif

#if(MHDMITX_ENABLE == 1)
            mi_set_hdmi_3d_format(E_MI_DISPOUT_3D_NOT_IN_USE);
#endif
        }
    }
}
