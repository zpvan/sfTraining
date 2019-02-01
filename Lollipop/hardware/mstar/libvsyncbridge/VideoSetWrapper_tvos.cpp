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
#include <tvmanager/TvManager.h>
#include "MsFrmFormat.h"
#include "vsync_bridge.h"
#include "VideoSetWrapper.h"
#if !defined(MSTAR_MADISON) // FIXME: too old utopia did not have UFO.h
#include <UFO.h>
#endif
#include <drvXC_IOPort.h>
#include <apiXC.h>
#include <apiPNL.h>
#include <drvMVOP.h>

static sp<VideoSet> pVideoSet = NULL;
static sp<PictureManager> pPictureManager = NULL;
static sp<ThreeDimensionManager> pThreeDimensionManager = NULL;
static sp<PipManager> pPipManager = NULL;
static sp<TvManager> pTvManager = NULL;

static int used[2] = {0};
static int WIN_Initialized[2] = {0};
static int DS_Enabled[2] = {0};
static int DS_Restore[2] = {0};
static int State[2] = {0};
static MS_VideoInfo SetMode_Vinfo[2];
static MS_VideoInfo SetWin_Vinfo[2];
static MS_WINDOW VideoWin[2];
static MS_WINDOW XCWin[2] = {0};
static int panelW = 0;
static int panelH = 0;
static int MVOP_width[2] = {0};
static int MVOP_height[2] = {0};
static bool b2StageDS = 0;
#define IS_4K_DS_MODE(pVinfo) ((pVinfo)->u8Reserve[1] & 0x01)

static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;


//#define VIDEOSET_DEBUG
#ifdef VIDEOSET_DEBUG
    #define VIDEOSET_PRINT  ALOGD
#else
    #define VIDEOSET_PRINT
#endif

#define VIDEOSET_LOCK  {    \
    VIDEOSET_PRINT("Entar %s[%d]", __FUNCTION__, __LINE__);    \
    pthread_mutex_lock(&sLock); }
#define VIDEOSET_UNLOCK  {    \
    VIDEOSET_PRINT("Leave %s[%d]", __FUNCTION__, __LINE__);    \
    pthread_mutex_unlock(&sLock);}

void (*fpSetAspectRatio)(int32_t u32Value) = NULL;
void (*fpEnable3D)(int32_t u32Value) = NULL;

static void reset_window(float Wratio, float Hratio, MS_WINDOW *win, MS_U16 panel_w, MS_U16 panel_h)
{
    // apply to Width
    win->x *= Wratio;

    win->width *= Wratio;
    if (win->width > panel_w) {
        win->width = panel_w;
    }

    // apply to Height
    win->y *= Hratio;

    win->height *= Hratio;
    if (win->height > panel_h) {
        win->height = panel_h;
    }
}

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
    void notifyCB(int32_t nEvt, int32_t ext1, int32_t ext2, const Parcel *obj);
    void PostEvent_Template(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_SnServiceDeadth(int32_t nEvt, int32_t ext1, int32_t ext2);
    void PostEvent_Enable3D(int32_t ext1, int32_t ext2);
    void PostEvent_4k2kUnsupportDualView(int32_t ext1, int32_t ext2);
};

void ThreeDMgrListener::notifyCB(int32_t nEvt, int32_t ext1, int32_t ext2, const Parcel *obj)
{

}

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
                    vsync_bridge_ds_update_table(0);
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
        vsync_bridge_update_status(0);
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
                    vsync_bridge_ds_update_table(0);
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
        vsync_bridge_update_status(0);
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
                    vsync_bridge_ds_update_table(0);
                    DS_Enabled[0] = 1;
                    DS_Restore[0] = 0;
                }
            }
        }
        vsync_bridge_update_status(0);
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

extern "C" void Wrapper_connect()
{
    if (pVideoSet == NULL) {
        pVideoSet = VideoSet::connect();
        if (pVideoSet == NULL) {
            ALOGE("Fail to connect service mstar.VideoSet");
        } else {
            //ALOGD("Success to connect mstar.VideoSet");
            sp<VdSetListener> listener = new VdSetListener();
            pVideoSet->setListener(listener);
        }
    }

    if (pPictureManager == NULL) {
        pPictureManager = PictureManager::connect();
        if (pPictureManager == NULL) {
            ALOGE("Fail to connect service mstar.PictureManager");
        } else {
            sp<PicMgrListener> listener = new PicMgrListener();
            pPictureManager->setListener(listener);
        }
    }

    if (pThreeDimensionManager == NULL) {
        pThreeDimensionManager = ThreeDimensionManager::connect();
        if (pThreeDimensionManager == NULL) {
            ALOGE("Fail to connect service mstar.3DManager");
        } else {
            sp<ThreeDMgrListener> listener = new ThreeDMgrListener();
            pThreeDimensionManager->setListener(listener);
        }
    }

    if (pPipManager == NULL) {
        pPipManager = PipManager::connect();
        if (pPipManager == NULL) {
            ALOGE("Fail to connect service mstar.PipManager");
        } else {
            sp<PipMgrListener> listener = new PipMgrListener();
            pPipManager->setListener(listener);
        }
    }

    if (pTvManager == NULL) {
        pTvManager = TvManager::connect();
        if (pTvManager == NULL) {
            ALOGE("Fail to connect service mstar.pTvManager");
        } else {
            //ALOGD("Success to connect mstar.pTvManager");
        }
    }
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

extern "C" void Wrapper_seq_change(int enWin, MS_DispFrameFormat *dff, int forceDS)
{
    // per PM / XC request, under 23 fps needs set as 30 fps for performance issue.
    dff->u32FrameRate = dff->u32FrameRate > 23000 ? dff->u32FrameRate : 30000;

    // Must disable DS before set MVOP, Otherwise XC maybe dead lock. This is HW issue.
    if (Wrapper_Video_DS_State(enWin)) {
        Wrapper_Video_closeDS(enWin, 0, 1);
        forceDS = 1;
    }
    VIDEOSET_LOCK;

    if (pVideoSet != NULL) {
        MS_FrameFormat *sFrame = &dff->sFrames[MS_VIEW_TYPE_CENTER];

        if (dff->CodecType == MS_CODEC_TYPE_MVC) {
            SetMode_Vinfo[enWin].u16HorSize = SetWin_Vinfo[enWin].u16HorSize = sFrame->u32Width - sFrame->u32CropRight - sFrame->u32CropLeft;
            SetMode_Vinfo[enWin].u16VerSize = SetWin_Vinfo[enWin].u16VerSize = sFrame->u32Height - sFrame->u32CropBottom - sFrame->u32CropTop;
            SetMode_Vinfo[enWin].u16CropRight = SetWin_Vinfo[enWin].u16CropRight = 0;
            SetMode_Vinfo[enWin].u16CropLeft = SetWin_Vinfo[enWin].u16CropLeft = 0;
            SetMode_Vinfo[enWin].u16CropBottom = SetWin_Vinfo[enWin].u16CropBottom = 0;
            SetMode_Vinfo[enWin].u16CropTop = SetWin_Vinfo[enWin].u16CropTop = 0;
        } else {
            SetMode_Vinfo[enWin].u16HorSize = SetWin_Vinfo[enWin].u16HorSize = sFrame->u32Width;
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
            DS_Enabled[enWin] = 1;
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
            } else {
                u8Reserve1_mode->ds_4k_mode = 0;
                u8Reserve1_win->ds_4k_mode = 0;
            }
        }
#endif

        if (dff->sHDRInfo.u32FrmInfoExtAvail) {
            ALOGD("HDR info: [%d %d %d %d]", dff->sHDRInfo.u32FrmInfoExtAvail,
                dff->sHDRInfo.stColorDescription.u8ColorPrimaries,
                dff->sHDRInfo.stColorDescription.u8TransferCharacteristics,
                dff->sHDRInfo.stColorDescription.u8MatrixCoefficients);
            Wrapper_SetHdrMetadata((int)enWin, &dff->sHDRInfo);
        }

        pVideoSet->video_setMode(&SetMode_Vinfo[enWin], enWin);

        if (DS_Enabled[enWin]) {
            update_current_mvop_timing(enWin);
        }

        MS_WINDOW DispWin;
        MS_WINDOW CropWin;

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
        if (DS_Enabled[enWin]) {
            // use the max size to load qmap
            // aspect ratio will cal. in prepare DS table
            DispWin.x = 0;
            DispWin.y = 0;
            DispWin.width = g_IPanel.Width();
            DispWin.height = g_IPanel.Height();
        }
        SetWin_Vinfo[enWin].u16CropLeft = CropWin.x;
        SetWin_Vinfo[enWin].u16CropTop = CropWin.y;
        SetWin_Vinfo[enWin].u16HorSize = CropWin.width + CropWin.x;
        SetWin_Vinfo[enWin].u16VerSize = CropWin.height + CropWin.y;
        SetWin_Vinfo[enWin].u16CropRight = 0;
        SetWin_Vinfo[enWin].u16CropBottom = 0;

        panelW = g_IPanel.Width();
        panelH = g_IPanel.Height();

        vsync_bridge_CheckInterlaceOut(&DispWin);

        if (!dff->u8SeamlessDS) {
            pVideoSet->video_setXCWin(DispWin.x, DispWin.y, DispWin.width, DispWin.height, &SetWin_Vinfo[enWin], enWin);
        }
    }

    VIDEOSET_UNLOCK;
}

extern "C" void Wrapper_disconnect(int enWin)
{
    VIDEOSET_LOCK;

    if (pVideoSet != NULL) {
        ALOGD("Wrapper_disconnect %d", enWin);

        WIN_Initialized[enWin] = 0;

        pVideoSet->video_closeWin(enWin);

        used[enWin] = 0;
        DS_Enabled[enWin] = 0;
        DS_Restore[enWin] = 0;
        State[enWin] = 0;
        if (used[0] == 0  && used[1] == 0) {
            pVideoSet->disconnect();
            pVideoSet.clear();
        }
    }

    if (!vsync_bridge_is_hw_auto_gen_black()) {
        // unlock app mute lock to prevent other source black screen
        if (pTvManager != NULL) {
            pTvManager->setVideoMute(0, 0, 0, enWin ? INPUT_SOURCE_STORAGE2 : INPUT_SOURCE_STORAGE);
        }
    }

    if (used[0] == 0 && used[1] == 0) {
        if (pPictureManager != NULL) {
            pPictureManager->disconnect();
            pPictureManager.clear();
        }

        if (pThreeDimensionManager != NULL) {
            pThreeDimensionManager->disconnect();
            pThreeDimensionManager.clear();
        }

        if (pPipManager != NULL) {
            pPipManager->disconnect();
            pPipManager.clear();
        }

        if (pTvManager != NULL) {
            pTvManager->disconnect();
            pTvManager.clear();
        }
    }

    VIDEOSET_UNLOCK;
}

extern "C" int Wrapper_Video_setMode(MS_VideoInfo *info, int enWin)
{
    int ret = -1;
    MS_VideoInfo vinfo;
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&vinfo.u8Reserve[0];

    // per PM / XC request, under 23 fps needs set as 30 fps for performance issue.
    info->u16FrameRate = info->u16FrameRate > 23000 ? info->u16FrameRate : 30000;

    memcpy(&vinfo, info, sizeof(MS_VideoInfo));

    VIDEOSET_LOCK;
    Wrapper_connect();

    if (pVideoSet != NULL) {
        ALOGD("Wrapper_Video_setMode %d ", enWin);
        used[enWin] = 1;
        if (info->u8EnableDynamicScaling) {
            DS_Enabled[enWin] = 1;
        }

        if (pPictureManager != NULL) {
            if (info->u16HorSize >= 3840 && info->u16VerSize >= 1080) {
                pPictureManager->disableAllDualWinMode();
            }
        }

        ret = pVideoSet->video_setMode(&vinfo, enWin);

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

        panelW = g_IPanel.Width();
        panelH = g_IPanel.Height();
    }
    VIDEOSET_UNLOCK;

    return ret;
}

extern "C" int Wrapper_Video_setXCWin(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio)
{
    int ret = -1;
    bool ResetXC = false;
    MS_VideoInfo vinfo;
    MS_WINDOW DispWin;
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&SetWin_Vinfo[enWin].u8Reserve[0];

    if (info == NULL) {
        return ret;
    }

    memcpy(&vinfo, info, sizeof(MS_VideoInfo));

    if (pVideoSet != NULL) {
        ALOGD("Wrapper_Video_setXCWin %d", enWin);

        VIDEOSET_LOCK;

        if (!used[enWin]) {
            used[enWin] = 1;

            if (info->u8EnableDynamicScaling) {
                DS_Enabled[enWin] = 1;
            }

            memcpy(&SetMode_Vinfo[enWin], info, sizeof(MS_VideoInfo));
        }

        if ((XCWin[enWin].x != x) || (XCWin[enWin].y != y) || (XCWin[enWin].width != width) || (XCWin[enWin].height != height)) {
            XCWin[enWin].x = x;
            XCWin[enWin].y = y;
            XCWin[enWin].width = width;
            XCWin[enWin].height = height;
            ResetXC = true;
        }
        if (!WIN_Initialized[enWin] && vinfo.u8EnableDynamicScaling) {
            // use the max size to load qmap
            // aspect ratio will cal. in prepare DS table
            x = 0;
            y = 0;
            width = g_IPanel.Width();
            height = g_IPanel.Height();
        }
        DispWin.x = x;
        DispWin.y = y;
        DispWin.width = width;
        DispWin.height = height;

        if (vsync_bridge_CheckInterlaceOut(&DispWin)) {
            ResetXC = true;
        }
        info->u8AspectRatio = 0;    // temp set 0, can remove if SN is update

        if ((SetWin_Vinfo[enWin].u16CropLeft != vinfo.u16CropLeft)
            || (SetWin_Vinfo[enWin].u16CropTop != vinfo.u16CropTop)
            || (SetWin_Vinfo[enWin].u16HorSize != vinfo.u16HorSize)
            || (SetWin_Vinfo[enWin].u16VerSize != vinfo.u16VerSize)) {
            ResetXC = true;
        }

        memcpy(&SetWin_Vinfo[enWin], &vinfo, sizeof(MS_VideoInfo));
        videoflags->no_lock = 0;    //Cannot store this flag. It should be used case by case.

        VIDEOSET_UNLOCK;
        if (WIN_Initialized[enWin] && (ResetXC == false)) {
            return 0;
        }
        ret = pVideoSet->video_setXCWin(DispWin.x, DispWin.y, DispWin.width, DispWin.height, &vinfo, enWin);
        VIDEOSET_LOCK;
        WIN_Initialized[enWin] = 1;
        VIDEOSET_UNLOCK;
    }

    return ret;
}

extern "C" int Wrapper_Video_disableDS(int enWin, int restore)
{
    int ret = -1;

    VIDEOSET_LOCK;
    if (pVideoSet != NULL) {
        ALOGD("Wrapper_Video_disableDS %d %d", enWin, restore);
        ret = pVideoSet->video_disableDS(enWin, restore);
        if (ret == 0) {
            if (restore) {
                DS_Enabled[enWin] = 1;
            } else {
                DS_Enabled[enWin] = 0;
            }
        }
    }
    VIDEOSET_UNLOCK;

    return ret;
}

int Wrapper_Video_closeDS(int enWin, int restore, int lock)
{
    int ret = -1;

    // no lock if it is call from postevent
    if (lock) {
        VIDEOSET_LOCK;
    }
    if (pVideoSet != NULL) {
        ALOGD("Wrapper_Video_closeDS %d %d", enWin, restore);
        ret = pVideoSet->video_disableDS(enWin, restore);
        if (ret == 0) {
            if (restore) {
                DS_Enabled[enWin] = 1;
            } else {
                DS_Enabled[enWin] = 0;
            }
        }
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
    // no lock if it is call from postevent
    if (lock) {
        VIDEOSET_LOCK;
    }
    if (pVideoSet != NULL) {
        MS_WINDOW msWin;
        msWin.x = x;
        msWin.y = y;
        msWin.width= width;
        msWin.height= height;
        XCWin[win].x = x;
        XCWin[win].y = y;
        XCWin[win].width = width;
        XCWin[win].height = height;
        pVideoSet->video_SurfaceVideoSizeChange(win, msWin.x, msWin.y, msWin.width, msWin.height, SrcWidth, SrcHeight);
    }
    if (lock) {
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
    if (pTvManager != NULL) {
        if (!pTvManager->setVideoMute(enable, 0, 0, enWin ? INPUT_SOURCE_STORAGE2 : INPUT_SOURCE_STORAGE)){
            ALOGE("Wrapper_Video_setMute FAIL !!! [%d %d]", enWin, enable);
        } else {
            ALOGD("Wrapper_Video_setMute [%d %d]", enWin, enable);
        }
    }
    if (lock) {
        if (!enable) mute_lock[enWin] = 0;
    }
    VIDEOSET_UNLOCK;
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
#ifdef MS_VIDEO_SUPPORT_HDR
    int ret = -1;

    VIDEOSET_LOCK;
    Wrapper_connect();

    ST_HDR_Metadata stHDRMetadata;
    memset(&stHDRMetadata, 0, sizeof(ST_HDR_Metadata));

    if (pHdrInfo->u32FrmInfoExtAvail & 0x01) {
        stHDRMetadata.u16HdrMetadataType |= HDR_METATYPE_MPEG_VUI;
        stHDRMetadata.stHdrMetadataMpegVUI.u8ColorPrimaries = pHdrInfo->stColorDescription.u8ColorPrimaries;
        stHDRMetadata.stHdrMetadataMpegVUI.u8TransferCharacteristics = pHdrInfo->stColorDescription.u8TransferCharacteristics;
        stHDRMetadata.stHdrMetadataMpegVUI.u8MatrixCoefficients = pHdrInfo->stColorDescription.u8MatrixCoefficients;
    }

    if (pHdrInfo->u32FrmInfoExtAvail & 0x02) {
        stHDRMetadata.u16HdrMetadataType |= HDR_METADATA_MPEG_SEI_MASTERING_COLOR_VOLUME;
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16DisplayPrimariesX[0] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[0][0];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16DisplayPrimariesY[0] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[0][1];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16DisplayPrimariesX[1] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[1][0];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16DisplayPrimariesY[1] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[1][1];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16DisplayPrimariesX[2] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[2][0];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16DisplayPrimariesY[2] = pHdrInfo->stMasterColorDisplay.u16DisplayPrimaries[2][1];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16WhitePointX = pHdrInfo->stMasterColorDisplay.u16WhitePoint[0];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u16WhitePointY = pHdrInfo->stMasterColorDisplay.u16WhitePoint[1];
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u32MaxDisplayMasteringLuminance = pHdrInfo->stMasterColorDisplay.u32MaxLuminance;
        stHDRMetadata.stHdrMetadataMpegSEIMasteringColorVolume.u32MinDisplayMasteringLuminance = pHdrInfo->stMasterColorDisplay.u32MinLuminance;
    }

    if (pHdrInfo->u32FrmInfoExtAvail & 0x04) {
        #ifdef HDR_METADATA_DOLBY_HDR
        stHDRMetadata.u16HdrMetadataType |= HDR_METADATA_DOLBY_HDR;
        #else
        ALOGE("video_SetHDRMetadata not support HDR_METADATA_DOLBY_HDR");
        #endif
    }
    ret = pVideoSet->video_SetHDRMetadata(stHDRMetadata, enWin);
    VIDEOSET_UNLOCK;

    return ret;
#else
    ALOGE("Not support video_SetHDRMetadata");
    return -1;
#endif
}

extern "C" int Wrapper_GetRenderDelayTime(int enWin) {
    int DelayTime = 0;
    if (pPictureManager != NULL) {
        DelayTime = pPictureManager->getPQDelayTime(enWin);
    }

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

void Wrapper_Enable3D(int enWin)
{
}

void Wrapper_Disable3D(int enWin)
{
}
