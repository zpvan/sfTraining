//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2017 MStar Semiconductor, Inc. All rights reserved.
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
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
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
//    MStar Software in conjunction with your or your customer`s product
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

#include "disp_api.h"
#include "vsync_bridge.h"
#include "VideoSetWrapper.h"

#include <UFO.h>
#include <MsOS.h>
#include <drvMMIO.h>
#include <drvSEM.h>
#include <drvMIU.h>
#include <drvSYS.h>
#include <drvMVOP.h>
#include <drvXC_IOPort.h>
#include <apiPNL.h>
#include <apiXC.h>

#ifdef BUILD_MSTARTV_MI
#include "mi_display.h"
static void timingToWidthHeight(MI_DISPLAY_OutputTiming_e eOutputTiming, int *width, int *height);
#endif

#ifdef UFO_XC_DS_PQ
#include <drvPQ.h>
#endif
#include "sideband.h"
#include "gop_display.h"

#define TIMER_MAX_MS  10000   // 10 sec
#define ROLLING_SLEEP_TIME  70000   // 70ms
#define WAIT_MALI_TIME 30000   // 30ms
#ifdef BUILD_MSTARTV_MI
#define TIME_SWITCH_REF 480     // switch channel reference duration time: 480 ms, MI XC enhance the performance.
#else
#define TIME_SWITCH_REF 900     // switch channel reference duration time: 800 ms, start: begin mute on, end: end of mute off
#endif

/// Rolling mode cmd type
typedef enum
{
    MS_ROLLING_NONE = 0,
    MS_ROLLING_MUTE = 1,
    MS_ROLLING_MUTE_CLOSE = 2,
} MS_Disp_RollingMode_Cmd;

///Error msg
typedef enum {
/// OK
    E_OK    = 0,
/// Error
    E_ERR = -1,
}MS_Disp_ErrType;

#ifndef ANDROID
#include <stdio.h>
#include <stdlib.h>

#define PROPERTY_VALUE_MAX  256
#define DISP_PRINTF(x...) printf(x)
#define DISP_PRINTF_DBG(x...) (void(0))
#define DISP_PRINTF_ERR(x...) printf(x)


static int property_get(const char *prop, char *str, const char *def)
{
    const char *ret = NULL;
    char *val = getenv(prop);

    if (val == NULL) {
        if (def == NULL) {
            return 0;
        } else {
            ret = def;
        }
    } else {
        ret = val;
    }

    strncpy(str, ret, PROPERTY_VALUE_MAX);
    str[PROPERTY_VALUE_MAX - 1] = 0; /* null character manually added */
    return strlen(str);
}

#else

#include <cutils/properties.h>
#include <cutils/log.h>
#define DISP_PRINTF(x...) ALOGD(x)
#define DISP_PRINTF_DBG(x...) void(0)
#define DISP_PRINTF_ERR(x...) ALOGE(x)

#include <threedimensionmanager/ThreeDimensionManager.h>
#include <pipmanager/PipManager.h>
#include <picturemanager/PictureManager.h>

#define RESOURCE_MANAGER
#endif

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} MS_Region;

static pthread_mutex_t sLock[MS_MAX_WINDOW] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
static unsigned int DisPlayMappingID[MS_MAX_WINDOW] = {0,};
#define MIU_INTERVAL    vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_MIU_INTERVAL)
#define DISP_LOCK(x)  pthread_mutex_lock(&sLock[x])
#define DISP_UNLOCK(x) pthread_mutex_unlock(&sLock[x])

#include <pthread.h>

class DisplayMessageQueue
{
public:

    enum MsgIdType
    {
        MSG_ID_EXIT,            // handle thread exit
        MSG_ID_STATE_CHANGE,    // state change
        MSG_ID_RESIZE,          // window resize
        MSG_ID_SETZOOM,         // window set zoom
        MSG_ID_SETROLLING,      // set rolling mode
    };

    struct MsgDataType
    {
        MS_WinInfo stWin_Info;
        MS_DispFrameFormat stDff;
        int Zoom_Mode;
        int State;
    };

    struct MsgType
    {
        MsgIdType id;
        MsgDataType data;
    };

    DisplayMessageQueue():
      m_nHead(0),
      m_nSize(0)
    {
        memset(m_aMsgQ, 0, sizeof(MsgType) * MAX_MSG_QUEUE_SIZE);
        (void) pthread_mutex_init(&m_mutex, NULL);
        (void) pthread_cond_init(&m_cond, NULL);
    }

    ~DisplayMessageQueue()
    {
        (void) pthread_mutex_destroy(&m_mutex);
        (void) pthread_cond_destroy(&m_cond);
    }

    int PushMsg(MsgIdType eMsgId,
        const MsgDataType *pMsgData)
    {
        DISP_PRINTF_DBG("PushMsg");
        pthread_mutex_lock(&m_mutex);
        // find the tail of the queue
        int idx = (m_nHead + m_nSize) % MAX_MSG_QUEUE_SIZE;
        if (m_nSize >= MAX_MSG_QUEUE_SIZE)
        {
            DISP_PRINTF_DBG("msg q is full...");
            pthread_mutex_unlock(&m_mutex);
            return -1;
        }

        // put data at tail of queue
        if (pMsgData != NULL)
        {
            memcpy(&m_aMsgQ[idx].data, pMsgData, sizeof(MsgDataType));
        }

        m_aMsgQ[idx].id = eMsgId;
        // update queue size
        m_nSize++;
        DISP_PRINTF_DBG("PushMsg size: %d", m_nSize);
        // unlock queue
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);

        DISP_PRINTF_DBG("PushMsg done");
        return 0;
    }

    int PopMsg(MsgType *pMsg)
    {
        // listen for a message...
        DISP_PRINTF_DBG("PopMsg");

        pthread_mutex_lock(&m_mutex);
        while (m_nSize == 0)
        {
            pthread_cond_wait(&m_cond, &m_mutex);
        }
        // get msg at head of queue
        memcpy(pMsg, &m_aMsgQ[m_nHead], sizeof(MsgType));
        // pop message off the queue
        m_nHead = (m_nHead + 1) % MAX_MSG_QUEUE_SIZE;
        m_nSize--;
        DISP_PRINTF_DBG("PopMsg size: %d", m_nSize);
        pthread_mutex_unlock(&m_mutex);
        DISP_PRINTF_DBG("PopMsg done");
        return 0;
    }

private:
    static const int MAX_MSG_QUEUE_SIZE = 100;
    MsgType m_aMsgQ[MAX_MSG_QUEUE_SIZE];
    int m_nHead;
    int m_nSize;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

class DispPath
{
private:
    pthread_t m_HandleThread;
    pthread_t m_TimerThread;
    static void *handle_thread(void *pClassObj);
    static void *Timer_thread(void *pObj);

public:
    int mWinID;
    int mState;
    int m3DMode;
    pthread_cond_t m_condTimer;
    pthread_mutex_t m_mutexTimer;
    pthread_cond_t m_cond;
    MS_U32 mTime;
    MS_U32 mStreamUniqueId;
    bool mRolling;
    bool mFreezeMode;
    bool mRollingMuteOn;
    bool mTimerRun;
    bool mIsSupportDS;
    bool mUseDipGop;
    void *mGopHandle;
    MS_DispFrameFormat mDipGopDff;
    MS_U8 mRollingModeConfig;
    bool mGotResource;
    MS_WinInfo mWinInfo;
    MS_WinInfo mSettingWinInfo;
    MS_DispFrameFormat mDff;
    DisplayMessageQueue mMsgQ;
    void RollingModeMuteOnOff(MS_DispFrameFormat *dff, int enableMute, int rollingFlagOn);
    int getCurrentFrame(gop_win_frame *frame);
    DispPath(int id);
    ~DispPath(void);
};

DispPath::DispPath(int id) :
    mWinID(id)
{
    mState = MS_STATE_CLOSE;
    mIsSupportDS = false;
    mRolling = false;
    mFreezeMode = false;
    mRollingMuteOn = false;
    mTimerRun = true;
    mUseDipGop = false;
    mGopHandle = NULL;
    mRollingModeConfig = MS_ROLLING_NONE;
    mGotResource = false;
    DisPlayMappingID[id] = 0;
    (void) pthread_mutex_init(&m_mutexTimer, NULL);
    (void) pthread_cond_init(&m_condTimer, NULL);
    (void) pthread_cond_init(&m_cond, NULL);
    pthread_create(&m_HandleThread, NULL, handle_thread, this);
    pthread_create(&m_TimerThread, NULL, Timer_thread, this);
}

DispPath::~DispPath(void)
{
    pthread_mutex_lock(&m_mutexTimer);
    mTimerRun = false;
    mRolling = false;
    mFreezeMode = false;
    pthread_cond_signal(&m_condTimer);
    pthread_mutex_unlock(&m_mutexTimer);
    pthread_join(m_HandleThread, NULL);
    pthread_join(m_TimerThread, NULL);
    (void) pthread_mutex_destroy(&m_mutexTimer);
    (void) pthread_cond_destroy(&m_condTimer);
    (void) pthread_cond_destroy(&m_cond);
    DISP_PRINTF("~DispPath");
}

static DispPath *g_pDispPath[MS_MAX_WINDOW] = {NULL,};

#ifdef RESOURCE_MANAGER
#include <RMClient.h>
#include <RMClientListener.h>
#include <RMResponse.h>
#include <RMRequest.h>
#include <RMTypes.h>
using namespace android;

struct VideoRMClientListener : public RMClientListener {
    VideoRMClientListener(int id);
    virtual ~VideoRMClientListener();
    virtual void handleRequestDone(const sp<RMResponse> &response);
    virtual bool handleResourcePreempted();
    virtual void handleAllResourcesReturned();
private:
    int mOverlayID;
    Mutex mLock;
};

static sp<VideoRMClientListener> pVideoRMClientListener[MS_MAX_WINDOW];
static sp<RMClient> pVideoRMClient[MS_MAX_WINDOW];
static sp<RMRequest> pRequest[MS_MAX_WINDOW];

VideoRMClientListener::VideoRMClientListener(int id)
    :mOverlayID(id)
{
}

VideoRMClientListener::~VideoRMClientListener()
{
}

void VideoRMClientListener::handleRequestDone(const sp<RMResponse> &response)
{
    Mutex::Autolock autoLock(mLock);

    DISP_LOCK(mOverlayID);
    if (g_pDispPath[mOverlayID] != NULL) {
        DISP_PRINTF("handleRequestDone: mOverlayID = %d, mState = 0x%d",
            mOverlayID, g_pDispPath[mOverlayID]->mState);

        g_pDispPath[mOverlayID]->mGotResource = true;

        DisplayMessageQueue::MsgIdType msgId;
        DisplayMessageQueue::MsgDataType msgData;

        memset(&msgData, 0, sizeof(DisplayMessageQueue::MsgDataType));
        msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
        msgData.State = MS_STATE_OPENING;
        g_pDispPath[mOverlayID]->mMsgQ.PushMsg(msgId, &msgData);
    } else {
        DISP_PRINTF_ERR("handleRequestDone: g_pDispPath[%d] is NULL", mOverlayID);
    }
    DISP_UNLOCK(mOverlayID);
}

bool VideoRMClientListener::handleResourcePreempted()
{
    Mutex::Autolock autoLock(mLock);
    DISP_PRINTF("handleResourcePreempted mOverlayID=%d", mOverlayID);

    DISP_LOCK(mOverlayID);
    if (g_pDispPath[mOverlayID] != NULL) {
        DisplayMessageQueue::MsgIdType msgId;
        DisplayMessageQueue::MsgDataType msgData;

        memset(&msgData, 0, sizeof(DisplayMessageQueue::MsgDataType));
        msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
        msgData.State = MS_STATE_CLOSE;
        pthread_mutex_lock(&g_pDispPath[mOverlayID]->m_mutexTimer);
        if (g_pDispPath[mOverlayID]->mRolling) {
            g_pDispPath[mOverlayID]->mRolling = false;
            g_pDispPath[mOverlayID]->mRollingMuteOn = true;
        }
        pthread_mutex_unlock(&g_pDispPath[mOverlayID]->m_mutexTimer);
        vsync_bridge_SetBlackScreenFlag(DisPlayMappingID[mOverlayID], FALSE);
        g_pDispPath[mOverlayID]->mMsgQ.PushMsg(msgId, &msgData);
        DISP_PRINTF("g_pDispPath[%d].mState= %d", mOverlayID, g_pDispPath[mOverlayID]->mState);
    }
    DISP_UNLOCK(mOverlayID);
    return true;
}

void VideoRMClientListener::handleAllResourcesReturned()
{
    Mutex::Autolock autoLock(mLock);
    DISP_PRINTF("handleAllResourcesReturned mOverlayID=%d", mOverlayID);
}
#endif

static unsigned char CheckOverlayID (MS_Win_ID eWinID , unsigned char StreamType) {
    unsigned char eID = (unsigned char)eWinID;

    if (StreamType != MS_STREAM_TYPE_N) {
        eID = StreamType;
        #if defined(MSTAR_CURRY)
        if (eID != MS_MAIN_WINDOW) {
            DISP_PRINTF("Warring !! Curry do not have XC[%d] set to -> N-WAY", eID);
            eID = MS_STREAM_TYPE_N;
        }
        #endif
    }
    return eID;
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
static void recal_window_ratio(int osdWidth, int osdHeight, MS_WINDOW *win)
{
    float Wratio, Hratio;
    int panelWidth = g_IPanel.Width();
    int panelHeight = g_IPanel.Height();

    if (vsync_bridge_GetInterlaceOut()) {
        panelHeight = panelHeight << 1;
    }
#ifdef BUILD_MSTARTV_MI
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
    timingToWidthHeight(stCurrentVideoTiming.eVideoTiming, &panelWidth, &panelHeight);
    MI_DISPLAY_Close(hDispHandle);
    MI_DISPLAY_DeInit();
    DISP_PRINTF("timing %d , width %d,height %d \n", stCurrentVideoTiming.eVideoTiming, panelWidth, panelHeight);
#endif

    Wratio = (float)panelWidth / osdWidth;
    Hratio = (float)panelHeight / osdHeight;
    DISP_PRINTF("panel [%d %d] osd [%d %d] ratio [%f %f]", panelWidth,
        panelHeight, osdWidth, osdHeight, Wratio, Hratio);

    // apply to Width
    if (Wratio > 1.0f) {
        if (win->x < 0) {
            win->x = 0;
        } else {
            win->x *= Wratio;
        }

        win->width *= Wratio;
        if (win->width > panelWidth) {
            win->width = panelWidth;
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
        if (win->height > panelHeight) {
            win->height = panelHeight;
        }
    }
    DISP_PRINTF("apply ratio, x=%d, y=%d, width=%d, height=%d",
          win->x, win->y, win->width, win->height);
}

static int recal_clip_region(const MS_Region *pos, MS_Region *disp, int screen_w, int screen_h, int img_w, int img_h, int rotation, MS_Region *clip)
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
        if ((x + w) < 0) {
            rc = -1;
        }
        if ((x + w) > screen_w) {
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

        if ((y + h) < 0) {
            rc = -1;
        }
        if ((y + h) > screen_h) {
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

        if (x >= screen_w) {
            rc = -1;
        } else
            if (x < 0) {
            clip_x = (-x) * obj_w / w;
            temp_x = 0;
        }

        if (y >= screen_h) {
            rc = -1;
        } else
            if (y < 0) {
            clip_y = (-y) * obj_h / h;
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

static void convert_crop_ratio(const MS_WINDOW *pCropWin, const MS_DispFrameFormat *dff, MS_WINDOW_RATIO *pCropWinRatio)
{
    MS_FrameFormat stFrame;
    stFrame = dff->sFrames[MS_VIEW_TYPE_CENTER];
    MS_U32 u32CalWidth = (stFrame.u32Width - stFrame.u32CropRight - stFrame.u32CropLeft);
    MS_U32 u32CalHeight = (stFrame.u32Height - stFrame.u32CropBottom - stFrame.u32CropTop);

    pCropWinRatio->x = (float)pCropWin->x / (float)u32CalWidth;
    pCropWinRatio->y = (float)pCropWin->y / (float)u32CalHeight;
    pCropWinRatio->width = (float)pCropWin->width / (float)u32CalWidth;
    pCropWinRatio->height = (float)pCropWin->height / (float)u32CalHeight;
}

static void convert_video_info(MS_Win_ID eWinID, MS_WinInfo stWinInfo,  MS_DispFrameFormat *dff, MS_VideoInfo *pVinfo)
{
    VinfoReserve0 *u8Reserve0;
    VinfoReserve1 *u8Reserve1;
    MS_FrameFormat stFrame;

    memset(pVinfo, 0, sizeof(MS_VideoInfo));
    u8Reserve0 = (VinfoReserve0 *)&pVinfo->u8Reserve[0];
    u8Reserve1 = (VinfoReserve1 *)&pVinfo->u8Reserve[1];
    stFrame = dff->sFrames[MS_VIEW_TYPE_CENTER];
    pVinfo->u32YAddress = (stFrame.sHWFormat.u32LumaAddr & ~MIU_INTERVAL);
    pVinfo->u32UVAddress = (stFrame.sHWFormat.u32ChromaAddr & ~MIU_INTERVAL);
    pVinfo->enCodecType = dff->CodecType;
    pVinfo->u8DataOnMIU = (stFrame.sHWFormat.u32LumaAddr & MIU_INTERVAL) ? 1 : 0;
    pVinfo->u16FrameRate = dff->u32FrameRate ? dff->u32FrameRate : 30000;
    pVinfo->u8Interlace = dff->u8Interlace ? 1 : 0;
    u8Reserve0->transform = stWinInfo.transform & 0x07;
    pVinfo->en3DLayout = dff->u83DLayout;

    if (dff->CodecType == MS_CODEC_TYPE_MVC) {
        pVinfo->u16HorSize = stFrame.u32Width - stFrame.u32CropRight - stFrame.u32CropLeft;
        pVinfo->u16VerSize = stFrame.u32Height - stFrame.u32CropBottom - stFrame.u32CropTop;
        pVinfo->u16CropRight = 0;
        pVinfo->u16CropLeft = 0;
        pVinfo->u16CropBottom = 0;
        pVinfo->u16CropTop = 0;

        if (dff->u83DMode == MS_3DMODE_SIDEBYSIDE) {
            pVinfo->u8Is3DLayout = 0;
        } else {
            pVinfo->u8Is3DLayout = 1;
        }

    } else {
        pVinfo->u16HorSize = stFrame.u32Width;
        pVinfo->u16VerSize = stFrame.u32Height;
        pVinfo->u16CropRight = stFrame.u32CropRight;
        pVinfo->u16CropLeft = stFrame.u32CropLeft;
        pVinfo->u16CropBottom = stFrame.u32CropBottom;
        pVinfo->u16CropTop = stFrame.u32CropTop;
    }

    if (eWinID == MS_MAIN_WINDOW) {
        pVinfo->u32XCAddr = vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_XC_ADDR);
        pVinfo->u32DSAddr = vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUF_ADDR);

    } else {
        pVinfo->u32XCAddr = vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_SUB_DS_XC_ADDR);
        pVinfo->u32DSAddr = vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_SUB_BUF_ADDR);
    }
    pVinfo->u8DS_MIU_Select = (pVinfo->u32DSAddr & vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_MIU_INTERVAL)) ? 1 : 0;
    pVinfo->u8DS_Index_Depth = (MS_U8)vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUFFER_DEPTH);

    if (dff->u8FBLMode) {
        u8Reserve0->fbl_mode = 1;
        if (dff->u8Interlace) {
            pVinfo->u16FrameRate = 30000;
        } else {
            pVinfo->u16FrameRate = 60000;
        }
    }

    if (dff->u8MCUMode) {
        u8Reserve0->mcu_mode = 1;
    }

    ALOGD("u8ApplicationType = %d\r\n",dff->u8ApplicationType);
    u8Reserve0->application_type = dff->u8ApplicationType;

#if SUPPORT_4KDS_CHIP
    if (!dff->u8Interlace) {
        u8Reserve1->ds_4k_mode = 1;
    }
#endif

    if (dff->u8ApplicationType & MS_APP_TYPE_IMAGE) {
        u8Reserve1->csc_mode = (stFrame.eColorFormat == MS_COLOR_FORMAT_YUYV_CSC_255) ? 1 : 0;
    } else {
        u8Reserve1->csc_mode = dff->u8ColorInXVYCC;
    }

    ALOGD("convert_video_info dff->u8ApplicationType:%d", dff->u8ApplicationType);

    if (dff->u8ApplicationType & MS_APP_TYPE_GAME)
    {
        pVinfo->u8Reserve[1] |= MS_APP_TYPE_GAME;
    }
}

static void set_xc_win(MS_Win_ID eWinID, MS_WinInfo stWinInfo, MS_DispFrameFormat *dff, MS_VideoInfo vInfo)
{
    MS_FrameFormat stFrame = dff->sFrames[MS_VIEW_TYPE_CENTER];
    MS_Region disp_old;
    MS_Region disp_new = {0,0,0,0};
    MS_Region crop = {0,0,0,0};
    MS_U32 u32CalWidth = (stFrame.u32Width - stFrame.u32CropRight - stFrame.u32CropLeft);
    MS_U32 u32CalHeight = (stFrame.u32Height - stFrame.u32CropBottom - stFrame.u32CropTop);
    int panelWidth = g_IPanel.Width();
    int panelHeight = g_IPanel.Height();
    if (vsync_bridge_GetInterlaceOut()) {
        panelHeight = panelHeight << 1;
    }
#ifdef BUILD_MSTARTV_MI
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
    timingToWidthHeight(stCurrentVideoTiming.eVideoTiming, &panelWidth, &panelHeight);
    MI_DISPLAY_Close(hDispHandle);
    MI_DISPLAY_DeInit();
    DISP_PRINTF("timing %d , width %d,height %d \n",stCurrentVideoTiming.eVideoTiming, panelWidth, panelHeight);
#endif

    disp_old.left = (short)stWinInfo.disp_win.x;
    disp_old.top = (short)stWinInfo.disp_win.y;
    disp_old.right = (short)stWinInfo.disp_win.x + (short)stWinInfo.disp_win.width;
    disp_old.bottom = (short)stWinInfo.disp_win.y + (short)stWinInfo.disp_win.height;

    if (recal_clip_region(&disp_old,
                                  &disp_new, panelWidth,
                                  panelHeight,
                                  u32CalWidth,
                                  u32CalHeight,
                                  0, &crop) == 0) {
        DISP_PRINTF("disp_old[%d %d %d %d], disp_new[%d %d %d %d], crop[%d %d %d %d]",
              disp_old.left,  disp_old.top, disp_old.right, disp_old.bottom,
              disp_new.left,  disp_new.top, disp_new.right, disp_new.bottom,
              crop.left,  crop.top, crop.right, crop.bottom);

        stWinInfo.disp_win.x = disp_new.left;
        stWinInfo.disp_win.y = disp_new.top;
        stWinInfo.disp_win.width = disp_new.right - disp_new.left;
        stWinInfo.disp_win.height = disp_new.bottom - disp_new.top;
#if defined(BUILD_MSTARTV_MI) && defined(BUILD_FOR_STB)
        // Fix me : do nothing here, this is a patch for mi clippers 3D get timing is not correct
#else
        vInfo.u16CropLeft = vInfo.u16CropLeft + crop.left;
        vInfo.u16CropRight = vInfo.u16CropRight + (u32CalWidth - crop.right);
        vInfo.u16CropTop = vInfo.u16CropTop + crop.top;
        vInfo.u16CropBottom = vInfo.u16CropBottom + (u32CalHeight - crop.bottom);
#endif
    } else {
        DISP_PRINTF_ERR("overlay_recal_clip_region fail");
    }

    vInfo.u32AspectWidth = dff->u32AspectWidth;
    vInfo.u32AspectHeight = dff->u32AspectHeight;

    float fRatio = 1.0;
    if (dff->u32AspectWidth && dff->u32AspectHeight
        && (dff->CodecType != MS_CODEC_TYPE_MVC)) {
        fRatio = (float)dff->u32AspectWidth/(float)dff->u32AspectHeight;
    } else if (u32CalWidth && u32CalHeight) {
        fRatio = (float)u32CalWidth/(float)u32CalHeight;
    }

    MS_WINDOW dispWin;
    dispWin.x = stWinInfo.disp_win.x;
    dispWin.y = stWinInfo.disp_win.y;
    dispWin.width = stWinInfo.disp_win.width;
    dispWin.height = stWinInfo.disp_win.height;
    if (vsync_bridge_adjust_win(dff, &dispWin, fRatio)) {
        stWinInfo.disp_win.x = dispWin.x;
        stWinInfo.disp_win.y = dispWin.y;
        stWinInfo.disp_win.width = dispWin.width;
        stWinInfo.disp_win.height = dispWin.height;
    }

    Wrapper_Video_setXCWin(stWinInfo.disp_win.x, stWinInfo.disp_win.y, stWinInfo.disp_win.width,
        stWinInfo.disp_win.height, &vInfo, dff->OverlayID, fRatio);
}

static void init_disp_path(MS_Win_ID eWinID, MS_WinInfo stWinInfo, MS_DispFrameFormat *dff)
{
    MS_VideoInfo vInfo;
    MS_BOOL bInterlaceOut = FALSE;
    int Current3DMode = 0;
    int CurrentPIPMode = 0;
#ifdef BUILD_MSTARTV_MI
    MS_WINDOW dissubwin;
#endif
    char property[PROPERTY_VALUE_MAX];
    MS_WINDOW_RATIO CropWinRatio;

    VinfoReserve0 *videoflags = (VinfoReserve0 *)&vInfo.u8Reserve[0];
    convert_crop_ratio(&stWinInfo.crop_win, dff, &CropWinRatio);
    convert_video_info((MS_Win_ID)dff->OverlayID, stWinInfo, dff, &vInfo);
    vsync_bridge_set_dispinfo(dff->OverlayID, CropWinRatio, &stWinInfo);
    DISP_PRINTF("enXC=%d, enCodecType=%d, miu=%d, frameRate=%d, interlace=%d, Hsize=%d, Vsize=%d, {%d, %d, %d, %d}", dff->OverlayID,
          (int)vInfo.enCodecType, vInfo.u8DataOnMIU, vInfo.u16FrameRate, vInfo.u8Interlace, vInfo.u16HorSize,
          vInfo.u16VerSize, vInfo.u16CropLeft, vInfo.u16CropRight, vInfo.u16CropTop, vInfo.u16CropBottom);
    DISP_PRINTF("crop[%d %d %d %d] disp[%d %d %d %d] [%d, %d] apptype = %d", stWinInfo.crop_win.x,
          stWinInfo.crop_win.y, stWinInfo.crop_win.width, stWinInfo.crop_win.height, stWinInfo.disp_win.x,
          stWinInfo.disp_win.y, stWinInfo.disp_win.width, stWinInfo.disp_win.height, dff->u32AspectWidth,
          dff->u32AspectHeight, dff->u8ApplicationType);

    if (property_get("mstar.resolutionState", property, NULL) > 0) {
        if ((strcmp(property,"RESOLUTION_1080I") == 0) || (strcmp(property,"RESOLUTION_576I") == 0)
            || (strcmp(property,"RESOLUTION_480I") == 0)) {
            if (!vInfo.u8Is3DLayout) {
                bInterlaceOut = TRUE;
            }
        }
    }

#if SUPPORT_INTERLACEOUT_CHIP
    vsync_bridge_SetInterlaceOut(bInterlaceOut);
#endif

    if (!stWinInfo.default_zoom_mode) {
        sp<PictureManager> pMyPictureManager = NULL;

        pMyPictureManager = PictureManager::connect();
        if (pMyPictureManager == NULL) {
            ALOGE("Fail to connect service mstar.PictureManager");
        } else {
            //ALOGD("Success to connect mstar.PipManager");
            vsync_bridge_set_aspectratio(dff->OverlayID, (Vsync_Bridge_ARC_Type)pMyPictureManager->getAspectRatio());
            pMyPictureManager->disconnect();
            pMyPictureManager.clear();
        }
    }

#ifdef BUILD_MSTARTV_SN
    sp<ThreeDimensionManager> pThreeDimensionManager = NULL;
    sp<PipManager> pPipManager = NULL;

    if (dff->OverlayID == MS_MAIN_WINDOW) {
        Current3DMode = 0;
        if (pThreeDimensionManager == NULL) {
            pThreeDimensionManager = ThreeDimensionManager::connect();
            if (pThreeDimensionManager == NULL) {
                ALOGE("Fail to connect service mstar.3DManager");
            } else {
                //ALOGD("Success to connect mstar.3DManager");
                Current3DMode = pThreeDimensionManager->getCurrent3dFormat();
                pThreeDimensionManager->disconnect();
                pThreeDimensionManager.clear();
            }
        }
#elif BUILD_MSTARTV_MI
    //FIXME: should use mi PIP
    MS_BOOL bEnable3D = FALSE;
    Wrapper_get_3DStatus(dff->OverlayID,  &bEnable3D);
    if (bEnable3D == TRUE) {
        Current3DMode = 1;
    } else {
        Current3DMode = 0;
    }
#endif

    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        g_pDispPath[eWinID]->m3DMode = Current3DMode;
    }
    DISP_UNLOCK(eWinID);

#ifdef BUILD_MSTARTV_SN
        if (pPipManager == NULL) {
            pPipManager = PipManager::connect();
            if (pPipManager == NULL) {
                ALOGE("Fail to connect service mstar.PipManager");
            } else {
                //ALOGD("Success to connect mstar.PipManager");
                CurrentPIPMode = pPipManager->getPipMode();
                pPipManager->disconnect();
                pPipManager.clear();
            }
        }
    }
#endif

    ALOGD("Current 3D format = %d, PIPMode = %d", Current3DMode, CurrentPIPMode);

#ifdef ENABLE_DYNAMIC_SCALING
    bool bDisableDS = 0;
    char value[PROPERTY_VALUE_MAX];

    if ((property_get("ms.disable.ds", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) ||
        (property_get("mstar.disable.ds", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true")))) {
        bDisableDS = 1;
    }

    DISP_LOCK(eWinID);
    if (
#ifndef ENABLE_SUB_DYNAMIC_SCALING
        dff->OverlayID == MS_MAIN_WINDOW
#endif
#ifndef ENABLE_INTERLACE_DS
        && !vInfo.u8Interlace
#endif
#if defined(MSTAR_NAPOLI)
        && !((g_IPanel.Width() == 3840)
            && (g_pDispPath[1] != NULL)
            && (g_pDispPath[1]->mState == MS_STATE_OPENED))
#endif
#if defined(MSTAR_CLIPPERS)
        // This is a SW patch for clippers interlace mm play shaking when open DS by HW issue.
        && !(vInfo.u8Interlace)
#endif
        && !bDisableDS
        && !((vInfo.u16HorSize > DS_MAX_WIDTH) || (vInfo.u16VerSize > DS_MAX_HEIGHT))
        && !(Current3DMode)
        && !(CurrentPIPMode)
        && !(dff->u8MCUMode)
        && !(dff->u8ApplicationType & MS_APP_TYPE_GAME)
        && (dff->CodecType != MS_CODEC_TYPE_MVC)) {

        if (vsync_bridge_ds_init(dff) == 0) {
            vInfo.u8EnableDynamicScaling = 1;
            if (dff->u8SeamlessDS) {
                ALOGD("SeamlessDS Mode!");
            }
        }
    }
#endif
    DISP_UNLOCK(eWinID);

    Wrapper_SetDispWin(dff->OverlayID, stWinInfo.disp_win.x, stWinInfo.disp_win.y, stWinInfo.disp_win.width, stWinInfo.disp_win.height, &vInfo);
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        g_pDispPath[eWinID]->mIsSupportDS = vInfo.u8EnableDynamicScaling;
    }
    DISP_UNLOCK(eWinID);

    if ((int)dff->OverlayID == MS_SUB_WINDOW) {
        // Wrapper_Video_setMode for sub window needs window size for player initialize
        // We call Wrapper_SurfaceVideoSizeChange to info videosetservice first
        MS_WINDOW CropWin;
        MS_WinInfo subwin;

        memcpy(&subwin, &stWinInfo, sizeof(MS_WINDOW));
        CropWin.x = vInfo.u16CropLeft;
        CropWin.y = vInfo.u16CropTop;
        CropWin.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
        CropWin.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;
#ifdef BUILD_MSTARTV_MI
        dissubwin.x = subwin.disp_win.x;
        dissubwin.y = subwin.disp_win.y;
        dissubwin.width= subwin.disp_win.width;
        dissubwin.height= subwin.disp_win.height;
        Wrapper_enable_window(dff->OverlayID, &dissubwin);
#endif
        vsync_bridge_CalAspectRatio(&subwin.disp_win, &CropWin, dff, TRUE, FALSE, 0, 0);
        Wrapper_SurfaceVideoSizeChange((int)dff->OverlayID, subwin.disp_win.x, subwin.disp_win.y,
            subwin.disp_win.width, subwin.disp_win.height, vInfo.u16HorSize, vInfo.u16VerSize, 1);
    }

    if ((videoflags->application_type == MS_APP_TYPE_IMAGE) && (!Current3DMode)) {
        vInfo.u16CropLeft = 0;
        vInfo.u16CropRight = 0;
        vInfo.u16CropTop = 0;
        vInfo.u16CropBottom = 0;
    }

    if (dff->sHDRInfo.u32FrmInfoExtAvail) {
        DISP_PRINTF("HDR info: [%d %d %d %d]", dff->sHDRInfo.u32FrmInfoExtAvail,
            dff->sHDRInfo.stColorDescription.u8ColorPrimaries,
            dff->sHDRInfo.stColorDescription.u8TransferCharacteristics,
            dff->sHDRInfo.stColorDescription.u8MatrixCoefficients);
        Wrapper_SetHdrMetadata((int)dff->OverlayID, &dff->sHDRInfo);
    }

#ifdef UFO_XC_DS_PQ
    if (videoflags->application_type & MS_APP_TYPE_ONLINE) {
        MDrv_PQ_SetNetworkMM_OnOff(TRUE);
    }
#endif

    if (Wrapper_Video_setMode(&vInfo, (int)dff->OverlayID)) {

        // get panel ratio after setMode
        MS_WINDOW CropWin;
        MS_BOOL bCalAspectRatio = TRUE;
        MS_FrameFormat stFrame;
        stFrame = dff->sFrames[MS_VIEW_TYPE_CENTER];

        Wrapper_Video_setMute((int)dff->OverlayID ? 1 : 0, 1, 0);
        if ((videoflags->application_type == MS_APP_TYPE_IMAGE) && Current3DMode) {
            stWinInfo.disp_win.x = vInfo.u16CropLeft;
            stWinInfo.disp_win.y = vInfo.u16CropTop;
            stWinInfo.disp_win.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
            stWinInfo.disp_win.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;
            bCalAspectRatio = FALSE;
        }

        if (stWinInfo.cropWinByExt) {
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
                vInfo.u16CropRight = vInfo.u16HorSize - (stWinInfo.crop_win.x + stWinInfo.crop_win.width);
                vInfo.u16CropLeft = stWinInfo.crop_win.x;
                vInfo.u16CropBottom = vInfo.u16VerSize - (stWinInfo.crop_win.y + stWinInfo.crop_win.height);
                vInfo.u16CropTop = stWinInfo.crop_win.y;
            }
        }

        CropWin.x = vInfo.u16CropLeft;
        CropWin.y = vInfo.u16CropTop;
        CropWin.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
        CropWin.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;

        vsync_bridge_CalAspectRatio(&stWinInfo.disp_win, &CropWin, dff, bCalAspectRatio, FALSE, 0, 0);
        vInfo.u16CropLeft = CropWin.x;
        vInfo.u16CropTop = CropWin.y;
        vInfo.u16HorSize = CropWin.width + CropWin.x;
        vInfo.u16VerSize = CropWin.height + CropWin.y;
        vInfo.u16CropRight = 0;
        vInfo.u16CropBottom = 0;

#if SUPPORT_MVOP_BW_SAVING_CHIP
        if (!(dff->u8ApplicationType & MS_APP_TYPE_IMAGE)
            && (vInfo.u16HorSize > 3800 || vInfo.u16VerSize > 2000)) {

            MVOP_Handle tMVOPHandle;
            MS_BOOL     bEnable = TRUE;

            tMVOPHandle.eModuleNum = E_MVOP_MODULE_MAIN;
            MDrv_MVOP_SetCommand(&tMVOPHandle, E_MVOP_CMD_SET_420_BW_SAVING_MODE, (void *)&bEnable);
        }
#endif
        set_xc_win((MS_Win_ID)dff->OverlayID, stWinInfo, dff, vInfo);

        Wrapper_SurfaceVideoSizeChange(dff->OverlayID, stWinInfo.disp_win.x, stWinInfo.disp_win.y,
            stWinInfo.disp_win.width, stWinInfo.disp_win.height, vInfo.u16HorSize, vInfo.u16VerSize, 1);
    } else {
        ALOGE("Video_setMode failed!!");
    }
}

static void set_aspect_ratio(int Mode)
{
    DISP_PRINTF("set_aspect_ratio %d", Mode);
    for (int i = 0; i < MS_MAX_WINDOW; i++) {
        DISP_LOCK(i);
        if (g_pDispPath[i] != NULL) {
            DisplayMessageQueue::MsgIdType msgId;
            DisplayMessageQueue::MsgDataType msgData;
            memset(&msgData, 0, sizeof(DisplayMessageQueue::MsgDataType));

            msgId = DisplayMessageQueue::MSG_ID_SETZOOM;
            msgData.Zoom_Mode = Mode;
            g_pDispPath[i]->mMsgQ.PushMsg(msgId, &msgData);
        }
        DISP_UNLOCK(i);
    }
}

static void Enable3D(int Enable3DMode)
{
    DISP_PRINTF("Enable3D: %d", Enable3DMode);

    int eWinID = 0;
    MS_WinInfo stWinInfo;
    MS_DispFrameFormat stDff;
    MS_VideoInfo vinfo;
    VinfoReserve0 *videoflags = (VinfoReserve0 *)&vinfo.u8Reserve[0];
    int Current3DMode = g_pDispPath[eWinID]->m3DMode;

    for (int i = 0; i < MS_MAX_WINDOW; i++) {
        DISP_LOCK(i);
        if (g_pDispPath[i] != NULL) {
            g_pDispPath[i]->m3DMode = Enable3DMode;
        }
        DISP_UNLOCK(i);
    }

    DISP_LOCK(eWinID);
    stDff = g_pDispPath[eWinID]->mDff;
    stWinInfo = g_pDispPath[eWinID]->mWinInfo;
    DISP_UNLOCK(eWinID);
    if (g_pDispPath[eWinID]->mUseDipGop) return;
    convert_video_info((MS_Win_ID)stDff.OverlayID, stWinInfo, &stDff, &vinfo);
    if (videoflags->application_type == MS_APP_TYPE_IMAGE) {
        MS_WINDOW CropWin;
        MS_BOOL bCalAspectRatio = TRUE;
        MS_BOOL bResetXC = TRUE;

        if (Enable3DMode != 0) {
            stWinInfo.disp_win.x = vinfo.u16CropLeft;
            stWinInfo.disp_win.y = vinfo.u16CropTop;
            stWinInfo.disp_win.width = vinfo.u16HorSize - vinfo.u16CropLeft - vinfo.u16CropRight;
            stWinInfo.disp_win.height = vinfo.u16VerSize - vinfo.u16CropTop - vinfo.u16CropBottom;
            bCalAspectRatio = FALSE;
        } else if (Current3DMode){
            vinfo.u16CropLeft = 0;
            vinfo.u16CropRight = 0;
            vinfo.u16CropTop = 0;
            vinfo.u16CropBottom = 0;
        } else {
            bResetXC = FALSE;
        }

        if (bResetXC) {
            videoflags->no_lock = 1;
            Wrapper_Video_setMode(&vinfo, (MS_Win_ID)stDff.OverlayID);

            CropWin.x = vinfo.u16CropLeft;
            CropWin.y = vinfo.u16CropTop;
            CropWin.width = vinfo.u16HorSize - vinfo.u16CropLeft - vinfo.u16CropRight;
            CropWin.height = vinfo.u16VerSize - vinfo.u16CropTop - vinfo.u16CropBottom;

            vsync_bridge_CalAspectRatio(&stWinInfo.disp_win, &CropWin, &stDff, bCalAspectRatio, FALSE, 0, 0);
            vinfo.u16CropLeft = CropWin.x;
            vinfo.u16CropTop = CropWin.y;
            vinfo.u16HorSize = CropWin.width + CropWin.x;
            vinfo.u16VerSize = CropWin.height + CropWin.y;
            vinfo.u16CropRight = 0;
            vinfo.u16CropBottom = 0;
            set_xc_win((MS_Win_ID)stDff.OverlayID, stWinInfo, &stDff, vinfo);
        }
    } else if (Enable3DMode == 0) {
        MS_WINDOW CropWin;
        CropWin.x = vinfo.u16CropLeft;
        CropWin.y = vinfo.u16CropTop;
        CropWin.width = vinfo.u16HorSize - vinfo.u16CropLeft - vinfo.u16CropRight;
        CropWin.height = vinfo.u16VerSize - vinfo.u16CropTop - vinfo.u16CropBottom;
        vsync_bridge_CalAspectRatio(&stWinInfo.disp_win, &CropWin, &stDff, TRUE, FALSE, 0, 0);
        // when restore DS, update the display window info to SN according latest panel timing
        Wrapper_SurfaceVideoSizeChange(stDff.OverlayID, stWinInfo.disp_win.x, stWinInfo.disp_win.y,
        stWinInfo.disp_win.width, stWinInfo.disp_win.height, vinfo.u16HorSize, vinfo.u16VerSize, 0);
    }
}

static bool GetOverlayRollingMode(MS_Win_ID eWinID)
{
    char value[PROPERTY_VALUE_MAX];
    bool EnableRolling = false;
    if (g_pDispPath[eWinID] == NULL)
        return EnableRolling;

    if ((property_get("ms.overlayrolling", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) ||
        (property_get("mstar.overlayrolling", value, NULL)
        && (!strcmp(value, "1") || !strcasecmp(value, "true"))) ||
        g_pDispPath[eWinID]->mDff.u8SeamlessDS) {
        EnableRolling = true;
    }

    // 3D/PIP/POP/MVC/IMAGE_PLAYER, dont support Rolling mode
    if (g_pDispPath[eWinID]->m3DMode || (g_pDispPath[eWinID]->mDff.CodecType == MS_CODEC_TYPE_MVC)
        || (g_pDispPath[eWinID]->mDff.u8ApplicationType & MS_APP_TYPE_IMAGE) || (Wrapper_get_Status(DisPlayMappingID[eWinID])  != 0 )
        || (DisPlayMappingID[eWinID] != MS_MAIN_WINDOW) || g_pDispPath[eWinID]->mUseDipGop
        || (g_pDispPath[eWinID]->mDff.u8ApplicationType & MS_APP_TYPE_GAME)) {
        EnableRolling = false;
    }
    return EnableRolling;
}

static int GopWinGetFrame(void *context, gop_win_frame *frame)
{
    DispPath *render = (DispPath *)context;
    return render->getCurrentFrame(frame);
}

int DispPath::getCurrentFrame(gop_win_frame *frame)
{
    DISP_LOCK(mWinID);
    // get new frame or new display info
    frame->wininfo = mWinInfo;
    frame->dff = mDff;
    // set mDipGopDff for checking if DIP is using it or not
    mDipGopDff = mDff;
    pthread_cond_signal(&m_cond);
    DISP_UNLOCK(mWinID);
    return E_SIDEBANDQUEUE_RENDERED;
}

void DispPath::RollingModeMuteOnOff(MS_DispFrameFormat *dff, int enableMute, int rollingFlagOn) {
    if (enableMute == FALSE) {
        if (dff && dff->u8Interlace) {
            vsync_bridge_set_fdmask(dff, FALSE, TRUE);
        }
    }
    Wrapper_Video_setMute(DisPlayMappingID[mWinID], enableMute, 0);
    if (enableMute == FALSE) {
        if (dff && dff->u8Interlace) {
            vsync_bridge_set_fdmask(dff, TRUE, TRUE);
        }
    }
    mRollingMuteOn = rollingFlagOn;
    if(!enableMute) vsync_bridge_SetBlankVideo(DisPlayMappingID[mWinID], 0);
}

void *DispPath::Timer_thread(void *pObj)
{
    DispPath *pDispPath = reinterpret_cast<DispPath *>(pObj);

    if (pDispPath == NULL)
    {
        DISP_PRINTF_ERR("Timer_thread is null. exiting.");
        return 0;
    }
    DISP_PRINTF("enter Timer_thread");

    pthread_mutex_lock(&pDispPath->m_mutexTimer);
    while (pDispPath->mTimerRun) {
        while (!pDispPath->mRolling && pDispPath->mTimerRun) {
            DISP_PRINTF_DBG("Timer_thread waiting........ ");
            pthread_cond_wait(&pDispPath->m_condTimer, &pDispPath->m_mutexTimer);
        }
#if defined(MSTAR_CLIPPERS) || defined(MSTAR_CURRY)
        while (pDispPath->mRolling)
#else
        while (((MsOS_GetSystemTime() - pDispPath->mTime) < TIMER_MAX_MS) && pDispPath->mRolling)
#endif
        {
            pthread_mutex_unlock(&pDispPath->m_mutexTimer);
            usleep(ROLLING_SLEEP_TIME);
            #if defined(MSTAR_CLIPPERS) || defined(MSTAR_CURRY)
            /** Info to mute video : below condition msut to be match
            * 1. Player exit already
            * 2. It should be frame freezed when player exit last time
            * 3. Mute after Mali ready (magic number 1 vsync time : 30ms), otherwise user will see the blackscreen first.
            */
            pthread_mutex_lock(&pDispPath->m_mutexTimer);
            char property[PROPERTY_VALUE_MAX] = {0};
            property_get("debug.exit.rolling", property, "0");
            int value = atoi(property);
            if (pDispPath->mRolling && (pDispPath->mRollingModeConfig || value))
            {   // Config 2: mute video and close rolling mode when player exit.
                if (pDispPath->mRollingModeConfig == MS_ROLLING_MUTE_CLOSE || value == 2) {
                    usleep(WAIT_MALI_TIME);
                    Wrapper_Video_setMute(DisPlayMappingID[pDispPath->mWinID], TRUE, 0);
                    if (pDispPath->mFreezeMode) {
                        MApi_XC_FreezeImg(FALSE, (SCALER_WIN)DisPlayMappingID[pDispPath->mWinID]);
                    }
                    pDispPath->mRollingModeConfig = MS_ROLLING_NONE;
                    ALOGD("exit player,close rolling mode,mute video win(%d), Close XC[%d] !!!", pDispPath->mWinID, DisPlayMappingID[pDispPath->mWinID]);
                    pthread_mutex_unlock(&pDispPath->m_mutexTimer);
                    property_set("debug.exit.rolling", "0");
                    break;
                } else if (pDispPath->mRollingModeConfig == MS_ROLLING_MUTE || value == 1) {
                    // Config 1: mute video when player exit.
                    usleep(WAIT_MALI_TIME);
                    Wrapper_Video_setMute(DisPlayMappingID[pDispPath->mWinID], TRUE, 0);
                    if (pDispPath->mFreezeMode) {
                        MApi_XC_FreezeImg(FALSE, (SCALER_WIN)DisPlayMappingID[pDispPath->mWinID]);
                    }
                    pDispPath->mRollingMuteOn = true;
                    pDispPath->mRollingModeConfig = MS_ROLLING_NONE;
                    ALOGD("exit player,keep rolling mode,mute video win(%d) XC[%d]!!!", pDispPath->mWinID, DisPlayMappingID[pDispPath->mWinID]);
                    property_set("debug.exit.rolling", "0");
                }
            }
            pthread_mutex_unlock(&pDispPath->m_mutexTimer);
            #endif
            pthread_mutex_lock(&pDispPath->m_mutexTimer);
        }

        if (pDispPath->mRolling) {
            DisplayMessageQueue::MsgIdType msgId;
            DisplayMessageQueue::MsgDataType msgData;

            msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
            msgData.State = MS_STATE_CLOSE;
            pDispPath->mRolling = false;
            pDispPath->mRollingMuteOn = true;
            pDispPath->mFreezeMode = false;
            vsync_bridge_SetBlackScreenFlag(DisPlayMappingID[pDispPath->mWinID], FALSE);
            DISP_PRINTF("Win %d rollingmode off, close XC[%d]", pDispPath->mWinID, DisPlayMappingID[pDispPath->mWinID]);
            if (pDispPath->mMsgQ.PushMsg(msgId, &msgData) != 0) {
                DISP_PRINTF_ERR("rollingmode close Win %d Fail !!", pDispPath->mWinID);
            }
        }
    }
    pthread_mutex_unlock(&pDispPath->m_mutexTimer);
    DISP_PRINTF("leave Timer_thread");
    return NULL;
}

void *DispPath::handle_thread(void *pObj)
{
    bool bRunning = true;
    DispPath *pDispPath = reinterpret_cast<DispPath *>(pObj);

    DISP_PRINTF("enter handle_thread");

    if (pDispPath == NULL)
    {
        DISP_PRINTF_ERR("thread param is null. exiting.");
        return 0;
    }

    while (bRunning)
    {
        DisplayMessageQueue::MsgType msg;
        DISP_PRINTF_DBG("component thread is checking msg q...");
        if (pDispPath->mMsgQ.PopMsg(&msg) != 0)
        {
            DISP_PRINTF_ERR("failed to pop msg");
        }

        DISP_PRINTF("[%d] handle_thread thread got msg 0x%x, mState = %d", pDispPath->mWinID, msg.id, pDispPath->mState);
        switch (msg.id)
        {
            case DisplayMessageQueue::MSG_ID_STATE_CHANGE:

                DISP_PRINTF("Overlay %d state change from %d to %d",
                    pDispPath->mWinID, pDispPath->mState, msg.data.State);

                switch (pDispPath->mState)
                {
                    case MS_STATE_CLOSE:
                        if (msg.data.State == MS_STATE_RES_WAITING) {
                            DISP_PRINTF("Init Overlay %d", pDispPath->mWinID);

                            DISP_LOCK(pDispPath->mWinID);
                            memcpy(&pDispPath->mWinInfo, &msg.data.stWin_Info, sizeof(MS_WinInfo));
                            memcpy(&pDispPath->mDff, &msg.data.stDff, sizeof(MS_DispFrameFormat));
                            pDispPath->mGotResource = false;
                            DISP_UNLOCK(pDispPath->mWinID);

                            if (pDispPath->mUseDipGop) {
                                DisplayMessageQueue::MsgIdType msgId;
                                DisplayMessageQueue::MsgDataType msgData;
                                msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
                                msgData.State = MS_STATE_OPENING;
                                pDispPath->mMsgQ.PushMsg(msgId, &msgData);

                            } else {
                                #ifdef RESOURCE_MANAGER
                                pVideoRMClientListener[pDispPath->mWinID] = new VideoRMClientListener(pDispPath->mWinID);
                                pVideoRMClient[pDispPath->mWinID] = new RMClient(pVideoRMClientListener[pDispPath->mWinID]);
                                pRequest[pDispPath->mWinID] = new RMRequest();

                                if (pDispPath->mDff.OverlayID) {
                                    pRequest[pDispPath->mWinID]->addComponent(kComponentNativeVideoPlaneSub);
                                } else {
                                    pRequest[pDispPath->mWinID]->addComponent(kComponentNativeVideoPlaneMain);
                                }

                                pVideoRMClient[pDispPath->mWinID]->requestResourcesAsync(pRequest[pDispPath->mWinID]);
                                #else
                                DisplayMessageQueue::MsgIdType msgId;
                                DisplayMessageQueue::MsgDataType msgData;
                                msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
                                msgData.State = MS_STATE_OPENING;
                                pDispPath->mMsgQ.PushMsg(msgId, &msgData);
                                #endif

                                Wrapper_connect();

                                if (pDispPath->mWinInfo.default_zoom_mode) {
                                    vsync_bridge_set_aspectratio(pDispPath->mDff.OverlayID, E_AR_DEFAULT);
                                } else {
                                    Wrapper_attach_callback(E_CALLBACK_ASPECT_RATIO, set_aspect_ratio);
                                    Wrapper_attach_callback(E_CALLBACK_ENABLE3D, Enable3D);
                                }
                            }
                            pDispPath->mState = MS_STATE_RES_WAITING;
                            break;
                        } else {
                            DISP_PRINTF_ERR("wrong state [%d %d]", pDispPath->mState, msg.data.State);
                        }
                        break;
                    case MS_STATE_RES_WAITING:
                        if (msg.data.State == MS_STATE_OPENING) {
#if BUILD_MSTARTV_MI
                            if (!pDispPath->mUseDipGop) {
                                //FIXME: should use mi PIP
                                Wrapper_open_resource(DisPlayMappingID[pDispPath->mWinID]);
                                Wrapper_enable_window(DisPlayMappingID[pDispPath->mWinID], NULL);
                            }
#endif
                            MS_WinInfo stWin_Info;
                            MS_DispFrameFormat stDff;

                            DISP_LOCK(pDispPath->mWinID);
                            memcpy(&stDff, &pDispPath->mDff, sizeof(MS_DispFrameFormat));
                            DISP_UNLOCK(pDispPath->mWinID);

                            memcpy(&stWin_Info, &pDispPath->mWinInfo, sizeof(MS_WinInfo));
                            memcpy(&pDispPath->mSettingWinInfo, &stWin_Info, sizeof(MS_WinInfo));
                            if (pDispPath->mUseDipGop) {
                                MS_Video_Disp_GOP_Win_Register(&pDispPath->mGopHandle, pObj, GopWinGetFrame);
                            } else {
                                init_disp_path((MS_Win_ID)pDispPath->mWinID, stWin_Info, &stDff);
                            }
                            pDispPath->mState = MS_STATE_OPENING;
                            // send state change command to STATE_OPENED
                            DisplayMessageQueue::MsgIdType msgId;
                            DisplayMessageQueue::MsgDataType msgData;
                            msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
                            msgData.State = MS_STATE_OPENED;
                            pDispPath->mMsgQ.PushMsg(msgId, &msgData);
                        } else if (msg.data.State == MS_STATE_CLOSE) {
                            if (pDispPath->mUseDipGop) {
                                // do nothing
                            } else {
                                // direct change from waiting to close state
                                vsync_bridge_signal_abort_waiting(DisPlayMappingID[pDispPath->mWinID]);
                                pthread_cond_signal(&pDispPath->m_cond);
                                Wrapper_disconnect(DisPlayMappingID[pDispPath->mWinID]);
#ifdef RESOURCE_MANAGER
                                int Count = 2000;

                                // direct change from MS_STATE_RES_WAITING to MS_STATE_CLOSE
                                // needs to wait ResMgr handleRequestDone then returnAllResources
                                while (--Count && !pDispPath->mGotResource) {
                                    usleep(1000);
                                }

                                if (pVideoRMClient[pDispPath->mWinID] != NULL) {
                                    pVideoRMClient[pDispPath->mWinID]->returnAllResources();
                                }
#endif
                            }
                            pDispPath->mState = MS_STATE_CLOSE;
                        } else {
                            DISP_PRINTF_ERR("wrong state [%d %d]", pDispPath->mState, msg.data.State);
                        }
                        break;

                    case MS_STATE_OPENING:
                        if (msg.data.State == MS_STATE_OPENED) {

                            MS_DispFrameFormat stDff;

                            DISP_LOCK(pDispPath->mWinID);
                            pDispPath->mState = MS_STATE_OPENED;
                            pthread_cond_signal(&pDispPath->m_cond);
                            memcpy(&stDff, &pDispPath->mDff, sizeof(MS_DispFrameFormat));
                            pDispPath->mState = MS_STATE_OPENED;
                            pDispPath->mStreamUniqueId = stDff.u32StreamUniqueId;
                            DISP_UNLOCK(pDispPath->mWinID);

                            if (pDispPath->mUseDipGop) {
                                // no need to set XC
                                break;
                            }

                            if (memcmp(&pDispPath->mSettingWinInfo, &pDispPath->mWinInfo, sizeof(MS_WinInfo))) {
                                DISP_PRINTF_ERR("mWinInfo changed when initialize disp path");
                                MS_WinInfo stWin_Info;
                                MS_VideoInfo vInfo;
                                MS_WINDOW CropWin;
                                VinfoReserve0 *videoflags = (VinfoReserve0 *)&vInfo.u8Reserve[0];
                                MS_BOOL bCalAspectRatio = TRUE;
                                MS_WINDOW_RATIO CropWinRatio;

                                memcpy(&stWin_Info, &pDispPath->mWinInfo, sizeof(MS_WinInfo));
                                convert_crop_ratio(&stWin_Info.crop_win, &stDff, &CropWinRatio);

                                // update for vsync_bridge for DS
                                vsync_bridge_set_dispinfo(stDff.OverlayID, CropWinRatio, &stWin_Info);

                                convert_video_info((MS_Win_ID)stDff.OverlayID,
                                    stWin_Info, &stDff, &vInfo);
                                vInfo.u8EnableDynamicScaling = pDispPath->mIsSupportDS;

                                if (videoflags->application_type == MS_APP_TYPE_IMAGE) {
                                    if (pDispPath->m3DMode) {
                                        stWin_Info.disp_win.x = vInfo.u16CropLeft;
                                        stWin_Info.disp_win.y = vInfo.u16CropTop;
                                        stWin_Info.disp_win.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
                                        stWin_Info.disp_win.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;
                                        bCalAspectRatio = FALSE;
                                    } else {
                                        vInfo.u16CropLeft = 0;
                                        vInfo.u16CropRight = 0;
                                        vInfo.u16CropTop = 0;
                                        vInfo.u16CropBottom = 0;
                                    }
                                }
                                CropWin.x = vInfo.u16CropLeft;
                                CropWin.y = vInfo.u16CropTop;
                                CropWin.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
                                CropWin.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;

                                vsync_bridge_CalAspectRatio(&stWin_Info.disp_win, &CropWin, &stDff, bCalAspectRatio, FALSE, 0, 0);
                                vInfo.u16CropLeft = CropWin.x;
                                vInfo.u16CropTop = CropWin.y;
                                vInfo.u16HorSize = CropWin.width + CropWin.x;
                                vInfo.u16VerSize = CropWin.height + CropWin.y;
                                vInfo.u16CropRight = 0;
                                vInfo.u16CropBottom = 0;

                                if (Wrapper_Video_DS_State(stDff.OverlayID)) {
                                    // already update DS table in vsync_bridge_ds_set_win
                                } else {
                                    set_xc_win((MS_Win_ID)stDff.OverlayID, stWin_Info, &stDff, vInfo);
                                }
                            }
                            int panelWidth = g_IPanel.Width();
                            int panelHeight = g_IPanel.Height();
#ifdef BUILD_MSTARTV_MI
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
                            timingToWidthHeight(stCurrentVideoTiming.eVideoTiming, &panelWidth, &panelHeight);
                            MI_DISPLAY_Close(hDispHandle);
                            MI_DISPLAY_DeInit();
                            DISP_PRINTF("timing %d , width %d,height %d \n",stCurrentVideoTiming.eVideoTiming, panelWidth, panelHeight);
#endif
                            vsync_bridge_open(&stDff, panelWidth, panelHeight);

                            Wrapper_Video_setMute(stDff.OverlayID ? 1 : 0, 0, 0);
                        } else if (msg.data.State == MS_STATE_CLOSE) {
                            if (pDispPath->mUseDipGop) {
                                DISP_PRINTF("Remove DIP/GOP Overlay %d [%d] in openining state %p", pDispPath->mWinID, pDispPath->mGopHandle);
                                if (pDispPath->mGopHandle) {
                                    MS_Video_Disp_GOP_Win_Unregister(pDispPath->mGopHandle);
                                    DISP_LOCK(pDispPath->mWinID);
                                    memset(&pDispPath->mWinInfo, 0, sizeof(MS_WinInfo));
                                    memset(&pDispPath->mDff, 0, sizeof(MS_DispFrameFormat));
                                    DISP_UNLOCK(pDispPath->mWinID);
                                    pDispPath->mGopHandle = NULL;
                                }
                            } else {
                                // direct change from opening to close state
                                vsync_bridge_signal_abort_waiting(DisPlayMappingID[pDispPath->mWinID]);
                                pthread_cond_signal(&pDispPath->m_cond);
                                #ifdef ENABLE_DYNAMIC_SCALING
                                if (Wrapper_Video_DS_State(DisPlayMappingID[pDispPath->mWinID])) {
                                    vsync_bridge_ds_deinit(DisPlayMappingID[pDispPath->mWinID]);
                                }
                                #endif
                                Wrapper_disconnect(DisPlayMappingID[pDispPath->mWinID]);
                                #ifdef RESOURCE_MANAGER
                                if (pVideoRMClient[pDispPath->mWinID] != NULL) {
                                    pVideoRMClient[pDispPath->mWinID]->returnAllResources();
                                }
                                #endif
                            }
                            pDispPath->mState = MS_STATE_CLOSE;
                        } else {
                            DISP_PRINTF_ERR("wrong state [%d %d]", pDispPath->mState, msg.data.State);
                        }
                        break;
                    case MS_STATE_OPENED:
                        if (msg.data.State == MS_STATE_CLOSE) {

                            if (pDispPath->mUseDipGop) {
                                DISP_PRINTF("Remove DIP/GOP Overlay %d [%d] closing state %p", pDispPath->mWinID, pDispPath->mGopHandle);
                                if (pDispPath->mGopHandle) {
                                    MS_Video_Disp_GOP_Win_Unregister(pDispPath->mGopHandle);
                                    DISP_LOCK(pDispPath->mWinID);
                                    memset(&pDispPath->mWinInfo, 0, sizeof(MS_WinInfo));
                                    memset(&pDispPath->mDff, 0, sizeof(MS_DispFrameFormat));
                                    pDispPath->mGopHandle = NULL;
                                    DISP_UNLOCK(pDispPath->mWinID);
                                }
                            } else {
                                MS_DispFrameFormat stDff;
                                memcpy(&stDff, &pDispPath->mDff, sizeof(MS_DispFrameFormat));
                                vsync_bridge_set_frame_done_callback(DisPlayMappingID[pDispPath->mWinID], NULL, NULL);
                                DISP_PRINTF("Remove Overlay %d", pDispPath->mWinID);
                                if (vsync_bridge_is_hw_auto_gen_black() && pDispPath->mRollingMuteOn) {
                                    pDispPath->RollingModeMuteOnOff(&stDff, FALSE, FALSE);
                                    DISP_PRINTF("Video Mute OFF");
                                }
                                if (!vsync_bridge_is_hw_auto_gen_black()) {
                                    // To prevent display garbage, 3rd apk maybe not to
                                    // close player when remove overlay.
                                    Wrapper_Video_setMute(DisPlayMappingID[pDispPath->mWinID] ? 1 : 0, 1, 0);
                                }

                                #ifdef ENABLE_DYNAMIC_SCALING
                                if (Wrapper_Video_DS_State(DisPlayMappingID[pDispPath->mWinID])) {
                                    vsync_bridge_ds_deinit(DisPlayMappingID[pDispPath->mWinID]);
                                }
                                #endif
                                vsync_bridge_close(DisPlayMappingID[pDispPath->mWinID], &pDispPath->mDff);
                                Wrapper_disconnect(DisPlayMappingID[pDispPath->mWinID]);

                                #ifdef RESOURCE_MANAGER
                                pVideoRMClient[pDispPath->mWinID]->returnAllResources();
                                #endif
                            }
                            pDispPath->mState = MS_STATE_CLOSE;
                            DisPlayMappingID[pDispPath->mWinID] = 0;
                        } else {
                            DISP_PRINTF_ERR("wrong state [%d %d]", pDispPath->mState, msg.data.State);
                        }
                        break;
                }
                break;

            case DisplayMessageQueue::MSG_ID_SETZOOM:
                DISP_PRINTF("SetZoom %d", msg.data.Zoom_Mode);
                vsync_bridge_set_aspectratio(DisPlayMappingID[pDispPath->mWinID], (Vsync_Bridge_ARC_Type)msg.data.Zoom_Mode);
                // fall through
            case DisplayMessageQueue::MSG_ID_SETROLLING:
                //fall through
            case DisplayMessageQueue::MSG_ID_RESIZE:
                MS_VideoInfo vInfo;
                MS_WinInfo stWin_Info;
                MS_DispFrameFormat stDff;

                DISP_LOCK(pDispPath->mWinID);
                if (msg.id != DisplayMessageQueue::MSG_ID_SETZOOM) {
                    if (!msg.data.stWin_Info.load_last_setting) {
                        memcpy(&pDispPath->mDff, &msg.data.stDff, sizeof(MS_DispFrameFormat));
                        memcpy(&pDispPath->mWinInfo, &msg.data.stWin_Info, sizeof(MS_WinInfo));
                    }
                    DISP_PRINTF("Overlay %d XC[%d] Resize, mState = 0x%d, %d, crop[%d %d %d %d] disp[%d %d %d %d] osd[%d %d]",
                        pDispPath->mWinID, DisPlayMappingID[pDispPath->mWinID], pDispPath->mState, msg.data.stWin_Info.load_last_setting,
                        pDispPath->mWinInfo.crop_win.x , pDispPath->mWinInfo.crop_win.y,
                        pDispPath->mWinInfo.crop_win.width, pDispPath->mWinInfo.crop_win.height,
                        pDispPath->mWinInfo.disp_win.x , pDispPath->mWinInfo.disp_win.y,
                        pDispPath->mWinInfo.disp_win.width, pDispPath->mWinInfo.disp_win.height,
                        pDispPath->mWinInfo.osdWidth, pDispPath->mWinInfo.osdHeight);
                }
                memcpy(&stWin_Info, &pDispPath->mWinInfo, sizeof(MS_WinInfo));
                memcpy(&stDff, &pDispPath->mDff, sizeof(MS_DispFrameFormat));
                DISP_UNLOCK(pDispPath->mWinID);

                if (pDispPath->mUseDipGop) {
                    break;
                }

                convert_video_info((MS_Win_ID)stDff.OverlayID,
                    stWin_Info, &stDff, &vInfo);
                vInfo.u8EnableDynamicScaling = pDispPath->mIsSupportDS;

                if (pDispPath->mState == MS_STATE_OPENED) {

                    MS_WINDOW CropWin;
                    MS_BOOL bCalAspectRatio = TRUE;
                    VinfoReserve0 *videoflags = (VinfoReserve0 *)&vInfo.u8Reserve[0];
                    MS_WINDOW_RATIO CropWinRatio;

                    convert_crop_ratio(&stWin_Info.crop_win, &stDff, &CropWinRatio);
                    // update for vsync_bridge for DS
                    vsync_bridge_set_dispinfo(stDff.OverlayID, CropWinRatio, &stWin_Info);

#ifdef BUILD_MSTARTV_SN
                    if (msg.data.stWin_Info.load_last_setting) {    // Panel timing change
                        sp<ThreeDimensionManager> pThreeDimensionManager = ThreeDimensionManager::connect();
                        if (pThreeDimensionManager == NULL) {
                            DISP_PRINTF("Fail to connect service mstar.3DManager");
                        } else {
                            pDispPath->m3DMode = pThreeDimensionManager->getCurrent3dFormat();
                            pThreeDimensionManager->disconnect();
                            pThreeDimensionManager.clear();
                        }
                    }
#endif
                    if (videoflags->application_type == MS_APP_TYPE_IMAGE) {
                        if (pDispPath->m3DMode) {
                            stWin_Info.disp_win.x = vInfo.u16CropLeft;
                            stWin_Info.disp_win.y = vInfo.u16CropTop;
                            stWin_Info.disp_win.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
                            stWin_Info.disp_win.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;
                            bCalAspectRatio = FALSE;
                        } else {
                            vInfo.u16CropLeft = 0;
                            vInfo.u16CropRight = 0;
                            vInfo.u16CropTop = 0;
                            vInfo.u16CropBottom = 0;
                        }
                    }

                    MS_FrameFormat stFrame;
                    stFrame = stDff.sFrames[MS_VIEW_TYPE_CENTER];

                    if (stWin_Info.cropWinByExt) {
                         if (stDff.CodecType == MS_CODEC_TYPE_MVC) {
                            vInfo.u16HorSize = stFrame.u32Width;
                            vInfo.u16VerSize = stFrame.u32Height;
                            vInfo.u16CropRight = 0;
                            vInfo.u16CropLeft = 0;
                            vInfo.u16CropBottom = 0;
                            vInfo.u16CropTop = 0;
                        } else {
                            vInfo.u16HorSize = stFrame.u32Width;
                            vInfo.u16VerSize = stFrame.u32Height;
                            vInfo.u16CropRight = vInfo.u16HorSize - (stWin_Info.crop_win.x + stWin_Info.crop_win.width);
                            vInfo.u16CropLeft = stWin_Info.crop_win.x;
                            vInfo.u16CropBottom = vInfo.u16VerSize - (stWin_Info.crop_win.y + stWin_Info.crop_win.height);
                            vInfo.u16CropTop = stWin_Info.crop_win.y;
                        }
                    }

                    CropWin.x = vInfo.u16CropLeft;
                    CropWin.y = vInfo.u16CropTop;
                    CropWin.width = vInfo.u16HorSize - vInfo.u16CropLeft - vInfo.u16CropRight;
                    CropWin.height = vInfo.u16VerSize - vInfo.u16CropTop - vInfo.u16CropBottom;

                    vsync_bridge_CalAspectRatio(&stWin_Info.disp_win, &CropWin, &stDff, bCalAspectRatio, FALSE, 0, 0);
                    vInfo.u16CropLeft = CropWin.x;
                    vInfo.u16CropTop = CropWin.y;
                    vInfo.u16HorSize = CropWin.width + CropWin.x;
                    vInfo.u16VerSize = CropWin.height + CropWin.y;
                    vInfo.u16CropRight = 0;
                    vInfo.u16CropBottom = 0;
                    if (Wrapper_Video_DS_State(stDff.OverlayID)) {
                        // already update DS table in vsync_bridge_ds_set_win
                    } else {
                        set_xc_win((MS_Win_ID)stDff.OverlayID, stWin_Info, &stDff, vInfo);
                    }

                    if (pDispPath->mRollingMuteOn && (msg.id == DisplayMessageQueue::MSG_ID_SETROLLING)) {
                        pDispPath->RollingModeMuteOnOff(&stDff, FALSE, FALSE);
                        DISP_PRINTF("[Time] display_switch: %d, ref: %d, resize", (MsOS_GetSystemTime() - pDispPath->mTime), TIME_SWITCH_REF);
                    }
                }

                break;
            case DisplayMessageQueue::MSG_ID_EXIT:
                DISP_PRINTF("got MSG_ID_EXIT");
                bRunning = false;
                break;

            default:
                break;
        }
    }

    DISP_PRINTF("leave handle_thread");
    return NULL;
}

void MS_Video_Disp_Init(MS_Win_ID eWinID)
{
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] == NULL) {

        static bool bInitialized = false;

        if (!bInitialized) {
            bInitialized = true;
            // Init MVOP
            MDrv_SYS_GlobalInit();
            MDrv_MVOP_Init();
            MDrv_MVOP_SubInit();
            MDrv_MVOP_SetMMIOMapBase();
            vsync_bridge_init();

            // FIXME: clear DS buffer to 0xFF
            // Currently, XC utopia will only clear 6 set table(size is
            // 0x200 * 6) when enable DS. But if VDEC close before XC,
            // the DS hardware will fetch a HW default index 0x0F to
            // load the DRAM content to write XC REG. IF we don't
            // initialize it, the random DRAM data will broken XC.
            // but how about the 0x0F position is large than this size ????
            if (vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUF_ADDR)
                && vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUF_SIZE)) {
                void *pVAddr = (void*)MsOS_PA2KSEG1(vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUF_ADDR));
                if (pVAddr) {
                    memset(pVAddr, 0xFF, vsync_bridge_get_cmd(VSYNC_BRIDGE_GET_DS_BUF_SIZE));
                }
            }

#ifdef UFO_XC_DS_PQ
            MS_PQ_Init_Info stPQInitInfo = {0,};
            MDrv_PQ_Init(&stPQInitInfo);
#endif
        }
        g_pDispPath[eWinID] = new DispPath(eWinID);
    }
    DISP_UNLOCK(eWinID);
}

void MS_Video_Disp_DeInit(MS_Win_ID eWinID)
{
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        DisplayMessageQueue::MsgIdType msgId;
        DisplayMessageQueue::MsgDataType msgData;

        pthread_mutex_lock(&g_pDispPath[eWinID]->m_mutexTimer);
        if (g_pDispPath[eWinID]->mRolling) {
            g_pDispPath[eWinID]->mRolling = false;
            g_pDispPath[eWinID]->mRollingMuteOn = true;
        }
        pthread_mutex_unlock(&g_pDispPath[eWinID]->m_mutexTimer);
        vsync_bridge_SetBlackScreenFlag(DisPlayMappingID[eWinID], FALSE);
        msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
        msgData.State = MS_STATE_CLOSE;
        g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);

        msgId = DisplayMessageQueue::MSG_ID_EXIT;
        g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);

        delete g_pDispPath[eWinID];
        DISP_PRINTF("MS_Video_Disp_DeInit g_pDispPath[%d] = %p", (int)eWinID, g_pDispPath[eWinID]);
        g_pDispPath[eWinID] = NULL;
    }
    DISP_UNLOCK(eWinID);
}

int MS_Video_Disp_Open(MS_Win_ID eWinID, MS_WinInfo stWinInfo, const MS_DispFrameFormat *dff)
{
    int rtn = E_ERR;

    if (dff == NULL) {
        return rtn;
    }

    DisplayMessageQueue::MsgIdType msgId;
    DisplayMessageQueue::MsgDataType msgData;

    memcpy(&msgData.stDff, dff, sizeof(MS_DispFrameFormat));
    memcpy(&msgData.stWin_Info, &stWinInfo, sizeof(MS_WinInfo));

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
       stWinInfo.crop_win.x = msgData.stWin_Info.crop_win.x = x;
       stWinInfo.crop_win.y = msgData.stWin_Info.crop_win.y = y;
       stWinInfo.crop_win.width = msgData.stWin_Info.crop_win.width = width;
       stWinInfo.crop_win.height = msgData.stWin_Info.crop_win.height = height;
       msgData.stWin_Info.cropWinByExt = true;
    }

    msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
    msgData.State = MS_STATE_RES_WAITING;
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        pthread_mutex_lock(&g_pDispPath[eWinID]->m_mutexTimer);
        if (g_pDispPath[eWinID]->mRolling) {
            msgData.stDff.OverlayID = DisPlayMappingID[eWinID];
            if (msgData.stDff.u8ApplicationType & MS_APP_TYPE_IMAGE) {
                g_pDispPath[eWinID]->mRollingMuteOn = true;
                msgData.State = MS_STATE_CLOSE;
                rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
                msgData.State = MS_STATE_RES_WAITING;
                rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
            } else if ((g_pDispPath[eWinID]->mWinInfo.disp_win.x != stWinInfo.disp_win.x)
                || (g_pDispPath[eWinID]->mWinInfo.disp_win.y != stWinInfo.disp_win.y)
                || (g_pDispPath[eWinID]->mWinInfo.disp_win.width != stWinInfo.disp_win.width)
                || (g_pDispPath[eWinID]->mWinInfo.disp_win.height != stWinInfo.disp_win.height)
                || (msgData.stWin_Info.crop_win.x != g_pDispPath[eWinID]->mWinInfo.crop_win.x)
                || (msgData.stWin_Info.crop_win.y != g_pDispPath[eWinID]->mWinInfo.crop_win.y)
                || (msgData.stWin_Info.crop_win.width != g_pDispPath[eWinID]->mWinInfo.crop_win.width)
                || (msgData.stWin_Info.crop_win.height != g_pDispPath[eWinID]->mWinInfo.crop_win.height)) {
                g_pDispPath[eWinID]->mRollingMuteOn = true;
                msgId = DisplayMessageQueue::MSG_ID_SETROLLING;
                msgData.State = MS_STATE_OPENED;
                rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
            } else {
                memcpy(&g_pDispPath[eWinID]->mDff, dff, sizeof(MS_DispFrameFormat));
                memcpy(&g_pDispPath[eWinID]->mWinInfo, &stWinInfo, sizeof(MS_WinInfo));
                g_pDispPath[eWinID]->mDff.OverlayID = DisPlayMappingID[eWinID];
                if (g_pDispPath[eWinID]->mRollingMuteOn) {
                    g_pDispPath[eWinID]->RollingModeMuteOnOff(&msgData.stDff, FALSE, FALSE);
                    DISP_PRINTF("[Time] display_switch: %d, ref: %d", (MsOS_GetSystemTime() - g_pDispPath[eWinID]->mTime), TIME_SWITCH_REF);
                }
                rtn = E_OK;
            }
            g_pDispPath[eWinID]->mIsSupportDS = Wrapper_Video_DS_State(DisPlayMappingID[eWinID]);
        } else {
            DisPlayMappingID[eWinID] = CheckOverlayID(eWinID, msgData.stDff.u8StreamType);
            if (stWinInfo.transform || (dff->u8StreamType == MS_STREAM_TYPE_N) ||
                ((dff->u8StreamType != MS_STREAM_TYPE_N) && (DisPlayMappingID[eWinID] == MS_STREAM_TYPE_N))) {
                // Use DIP gop render
                g_pDispPath[eWinID]->mUseDipGop = true;
            } else {
                // Use Physical XC hw render
                msgData.stDff.OverlayID = DisPlayMappingID[eWinID];
            }
            rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
        }
        property_set("debug.exit.rolling", "0");
        g_pDispPath[eWinID]->mRolling = false;
        pthread_mutex_unlock(&g_pDispPath[eWinID]->m_mutexTimer);
    } else {
        DISP_PRINTF_ERR("%s fail", __FUNCTION__);
    }
    DISP_UNLOCK(eWinID);

    return rtn;
}

int MS_Video_Disp_Resize(MS_Win_ID eWinID, MS_WinInfo stWinInfo, const MS_DispFrameFormat *dff)
{
    int rtn = E_ERR;

    if (dff == NULL) {
        return rtn;
    }

    DisplayMessageQueue::MsgIdType msgId;
    DisplayMessageQueue::MsgDataType msgData;

    msgId = DisplayMessageQueue::MSG_ID_RESIZE;
    memcpy(&msgData.stDff, dff, sizeof(MS_DispFrameFormat));
    memcpy(&msgData.stWin_Info, &stWinInfo, sizeof(MS_WinInfo));
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        msgData.stDff.OverlayID = DisPlayMappingID[eWinID];
       if (!g_pDispPath[eWinID]->mRolling) {
            rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
        }
    } else {
        DISP_PRINTF_ERR("%s fail", __FUNCTION__);
    }
    DISP_UNLOCK(eWinID);

    return rtn;
}

int MS_Video_Disp_Close(MS_Win_ID eWinID)
{
    int rtn = E_ERR;
    DisplayMessageQueue::MsgIdType msgId;
    DisplayMessageQueue::MsgDataType msgData;
    msgId = DisplayMessageQueue::MSG_ID_STATE_CHANGE;
    msgData.State = MS_STATE_CLOSE;
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        pthread_mutex_lock(&g_pDispPath[eWinID]->m_mutexTimer);
        if (g_pDispPath[eWinID]->mUseDipGop) {
            rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
        } else {
            if (vsync_bridge_GetLastFrameFreezeMode(DisPlayMappingID[eWinID]) == MS_FREEZE_MODE_VIDEO) {
                g_pDispPath[eWinID]->mFreezeMode = true;
            } else {
                g_pDispPath[eWinID]->mFreezeMode = false;
            }

#ifdef BUILD_FOR_STB
            if (g_pDispPath[eWinID]->mDff.sHDRInfo.u32FrmInfoExtAvail) {
                Wrapper_SetHdrMetadata((int)DisPlayMappingID[eWinID], NULL);
            }
#endif
            if ((GetOverlayRollingMode((MS_Win_ID)eWinID)) && (g_pDispPath[eWinID]->mState == MS_STATE_OPENED)) {
                g_pDispPath[eWinID]->mTime = MsOS_GetSystemTime();
                if (!g_pDispPath[eWinID]->mFreezeMode) {
    #if defined(MSTAR_CLIPPERS) || defined(MSTAR_CURRY)
                    g_pDispPath[eWinID]->mRollingModeConfig = MS_ROLLING_MUTE;
    #else
                    DISP_PRINTF("[Time] display_muteon: Win:%d", eWinID);
                    g_pDispPath[eWinID]->RollingModeMuteOnOff(&g_pDispPath[eWinID]->mDff, TRUE, TRUE);
    #endif
                }
                vsync_bridge_SetBlackScreenFlag(DisPlayMappingID[eWinID], TRUE);
                g_pDispPath[eWinID]->mRolling = true;
                pthread_cond_signal(&g_pDispPath[eWinID]->m_condTimer);
                rtn = E_OK;
            } else {
                if (g_pDispPath[eWinID]->mRolling) {
                    g_pDispPath[eWinID]->mRolling = false;
                    g_pDispPath[eWinID]->mRollingMuteOn = true;
                }
                vsync_bridge_SetBlackScreenFlag(DisPlayMappingID[eWinID], FALSE);
                rtn = g_pDispPath[eWinID]->mMsgQ.PushMsg(msgId, &msgData);
            }
            vsync_bridge_signal_abort_waiting(DisPlayMappingID[eWinID]);
        }
        pthread_cond_signal(&g_pDispPath[eWinID]->m_cond);
        pthread_mutex_unlock(&g_pDispPath[eWinID]->m_mutexTimer);
    } else {
        DISP_PRINTF("%s fail", __FUNCTION__);
    }
    DISP_UNLOCK(eWinID);

    return rtn;
}

void MS_Video_Disp_Flip(MS_Win_ID eWinID, MS_DispFrameFormat *dff)
{
    if (dff == NULL) {
        return;
    }
    MS_U32 OrgOverlayID = 0xFF;
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {

        if (g_pDispPath[eWinID]->mUseDipGop || (dff->u8StreamType == MS_STREAM_TYPE_N)) {
            // clear u8CurIndex to let prevent BufferQueue drop this frame
            // when enable RENDER_IN_QUEUE_BUFFER
            dff->u8CurIndex = 0;
        }

        // copy here due to we maybe use last dff in dms_flip_locked
        memcpy(&g_pDispPath[eWinID]->mDff, dff, sizeof(MS_DispFrameFormat));

        if ((g_pDispPath[eWinID]->mUseDipGop) || (dff->u8StreamType == MS_STREAM_TYPE_N)) {
            DISP_UNLOCK(eWinID);
            return;
        }
        if (g_pDispPath[eWinID]->mState != MS_STATE_OPENED) {
            DisPlayMappingID[eWinID] = CheckOverlayID(eWinID, dff->u8StreamType);
        }
        OrgOverlayID = dff->OverlayID;
        dff->OverlayID = g_pDispPath[eWinID]->mDff.OverlayID = DisPlayMappingID[eWinID];
    }
    DISP_UNLOCK(eWinID);
    if (dff->u8StreamType == MS_STREAM_TYPE_N) {
        DISP_LOCK(eWinID);
        if (OrgOverlayID != 0xFF) dff->OverlayID = OrgOverlayID;
        DISP_UNLOCK(eWinID);
        return;
    }
    vsync_bridge_render_frame(dff, VSYNC_BRIDGE_UPDATE);
    DISP_LOCK(eWinID);
    if (OrgOverlayID != 0xFF) dff->OverlayID = OrgOverlayID;
    DISP_UNLOCK(eWinID);

}
static timespec set_wait_timeout(int timeout)
{
    struct timespec ts;
    long time = 0;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += (time_t)timeout / 1000;
    time = ts.tv_nsec + ((long)(timeout % 1000) * 1000 * 1000);
    if(time >= 1000000000) {
        ts.tv_sec += (time_t)(time / 1000000000);
        ts.tv_nsec = (time % 1000000000);
    } else {
        ts.tv_nsec = time;
    }
    return ts;
}

int MS_Video_Disp_Wait(MS_Win_ID eWinID, MS_Wait_Mode eMode, MS_DispFrameFormat *dff)
{
    int rtn = E_OK;

    if ((eWinID >= MS_MAX_WINDOW) || (dff == NULL)) {
        DISP_PRINTF_ERR("MS_Video_Disp_Wait eWinID = 0x%x", eWinID);
        return E_ERR;
    }

    bool bUseDipGop = false;
    MS_Disp_State eState = MS_STATE_CLOSE;
    MS_DispFrameFormat working_dff = {0, };
    MS_DispFrameFormat stDff = {0, };
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        eState = (MS_Disp_State)g_pDispPath[eWinID]->mState;
        bUseDipGop = g_pDispPath[eWinID]->mUseDipGop;
        working_dff = g_pDispPath[eWinID]->mDipGopDff;
        memcpy(&stDff, dff, sizeof(MS_DispFrameFormat));
        stDff.OverlayID = DisPlayMappingID[eWinID];
    }
    DISP_UNLOCK(eWinID);

    switch(eMode)
    {
        case MS_WAIT_FRAME_DONE:
            if (bUseDipGop) {

                if (eState != MS_STATE_OPENED) {
                    break;
                }
                if (!memcmp(&working_dff, dff, sizeof(MS_DispFrameFormat))) {
                    struct timespec ts;
                    unsigned int wait_cnt = 5;
                    while(wait_cnt--) {
                        ts = set_wait_timeout(16);  // 16ms timeout
                        DISP_LOCK(eWinID);
                        if (g_pDispPath[eWinID] != NULL) {
                            pthread_cond_timedwait(&g_pDispPath[eWinID]->m_cond, &sLock[(int)eWinID], &ts);
                            if (memcmp(&g_pDispPath[eWinID]->mDipGopDff, dff, sizeof(MS_DispFrameFormat))) {
                                // dff is not in working state, return freame done.
                                DISP_UNLOCK(eWinID);
                                break;
                            }
                        }
                        DISP_UNLOCK(eWinID);
                    }
                    if (wait_cnt == 0) {
                        ALOGD("DIP %d wait frame done timeout", eWinID);
                    }
                }
            } else {
                vsync_bridge_wait_frame_done(&stDff);
            }
            break;
        case MS_WAIT_INIT_DONE:
            if (!dff->u8IsNoWaiting) {
                #define TIME_DISPLAY_INIT_REF   800 // display initial reference duration time: 800 ms
                DISP_LOCK(eWinID);
                if (MS_STATE_OPENED != (MS_Disp_State)g_pDispPath[eWinID]->mState) {
                    struct timespec ts = set_wait_timeout(2000);    // 2s timeout
                    MS_U32 time = MsOS_GetSystemTime();
                    ALOGD("[%d] Display Path INIT BEGIN, mRolling = %d, mState = %d", eWinID,
                        g_pDispPath[eWinID]->mRolling, g_pDispPath[eWinID]->mState);
                    if (!g_pDispPath[eWinID]->mRolling) {
                        pthread_cond_timedwait(&g_pDispPath[eWinID]->m_cond, &sLock[(int)eWinID], &ts);
                    }
                    DISP_UNLOCK(eWinID);
                    ALOGD("[%d] Display Path INIT END: %d", eWinID, (MS_STATE_OPENED == MS_Video_Disp_Get_State(eWinID)));
                    ALOGD("[Time] display_init: %d, ref: %d", (MsOS_GetSystemTime() - time), TIME_DISPLAY_INIT_REF);
                } else {
                    DISP_UNLOCK(eWinID);
                    rtn = E_ERR;
                }
            } else {
                rtn = E_ERR;
            }
            break;
        default:
            rtn = E_ERR;
            DISP_PRINTF("MS_Video_Disp_Wait eMode = 0x%x", eMode);
            break;
    }

    return rtn;
}

int MS_Video_Capture(MS_Win_ID eWinID, MS_FreezeMode eMode, int *width, int *height, MS_WINDOW *pWin, void *pdata)
{
    int rtn = E_ERR;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    if ((width == NULL) || (height == NULL) || (pWin == NULL)) {    // pData dont check because native window will set NULL when call the function first time.
        return rtn;
    }

    bool bUseDipGop = false;
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        bUseDipGop = g_pDispPath[eWinID]->mUseDipGop;
    }
    DISP_UNLOCK(eWinID);

    if (bUseDipGop) {
        if (eMode == MS_FREEZE_MODE_BLANK_VIDEO) {
            if (pdata == NULL) {
                // 1x1 AGRB8888
                left = 0;
                top = 0;
                right = 1;
                bottom = 1;
                *width = 1;
                *height = 1;
            } else {
                memset(pdata, 0, 4);  // black 1x1 AGRB8888
            }

            rtn = 0;
        }
    } else {
        rtn = vsync_bridge_capture_video((unsigned char)eMode, (unsigned int)DisPlayMappingID[eWinID],
            0, 0, width, height, &left,
            &top, &right, &bottom, pdata, *width);
    }

    pWin->x = left;
    pWin->y = top;
    pWin->width = right - left;
    pWin->height = bottom - top;

    return rtn;
}

int MS_Video_Frame_Capture(MS_Win_ID eWinID, MS_FreezeMode eMode, int *width, int *height, MS_WINDOW *pWin, void *pdata, int stride)
{
    int rtn = E_ERR;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    if ((width == NULL) || (height == NULL) || (pWin == NULL)) {    // pData dont check because native window will set NULL when call the function first time.
        return rtn;
    }

    bool bUseDipGop = false;
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        bUseDipGop = g_pDispPath[eWinID]->mUseDipGop;
    }
    DISP_UNLOCK(eWinID);

    if (bUseDipGop) {
        if (eMode == MS_FREEZE_MODE_BLANK_VIDEO) {
            if (pdata == NULL) {
                // 1x1 AGRB8888
                left = 0;
                top = 0;
                right = 1;
                bottom = 1;
                *width = 1;
                *height = 1;
            } else {
                memset(pdata, 0, 4);  // black 1x1 AGRB8888
            }

            rtn = 0;
        }
    } else {
        rtn = vsync_bridge_capture_video((unsigned char)eMode, (unsigned int)DisPlayMappingID[eWinID],
            0, 0, width, height, &left,
            &top, &right, &bottom, pdata, stride);
    }

    pWin->x = left;
    pWin->y = top;
    pWin->width = right - left;
    pWin->height = bottom - top;

    return rtn;
}

MS_Disp_State MS_Video_Disp_Get_State(MS_Win_ID eWinID)
{
    MS_Disp_State eRtn = MS_STATE_CLOSE;
    DISP_LOCK(eWinID);
    if (g_pDispPath[eWinID] != NULL) {
        eRtn = (MS_Disp_State)g_pDispPath[eWinID]->mState;
    }
    DISP_UNLOCK(eWinID);

    return eRtn;
}

int MS_Video_Disp_Set_Cmd(MS_Win_ID eWinID, MS_Cmd_Type eCmd, void* pPara)
{
    int rtn = E_OK;

    if (pPara == NULL) {
        DISP_PRINTF_ERR("MS_Video_Disp_Set_Cmd invalid para");
        return E_ERR;
    }

    switch(eCmd)
    {

        case MS_DISP_CMD_SET_FRAME_DONE_CALLBACK:
            {
                MS_CallbackInfo *pCallbackinfo = (MS_CallbackInfo *)pPara;
                vsync_bridge_set_frame_done_callback((unsigned char)DisPlayMappingID[eWinID], pCallbackinfo->pHandle , (FrameDoneCallBack)pCallbackinfo->callback);
            }
            break;

        case MS_DISP_CMD_SAVING_BW_MODE:
            MS_BOOL bEnable;
            bEnable = *(MS_BOOL*)pPara;
            vsync_bridge_SetSavingBandwidthMode((unsigned char)bEnable, (unsigned char)DisPlayMappingID[eWinID]);
            break;

        case MS_DISP_CMD_UPDATE_DISP_STATUS:
            vsync_bridge_update_status((unsigned char)DisPlayMappingID[eWinID]);
            break;

        case MS_DISP_CMD_CONTROL_ROLLING_MODE:
            if (g_pDispPath[eWinID] != NULL) {
                pthread_mutex_lock(&g_pDispPath[eWinID]->m_mutexTimer);
                g_pDispPath[eWinID]->mRollingModeConfig = *(MS_U8*)pPara;
                pthread_mutex_unlock(&g_pDispPath[eWinID]->m_mutexTimer);
            }
            break;

        default:
            rtn = E_ERR;
            DISP_PRINTF_ERR("MS_Video_Disp_Set_Cmd eCmd = %d", eCmd);
            break;
    }

    return rtn;
}

int MS_Video_Disp_Get_Cmd(MS_Win_ID eWinID, MS_Cmd_Type eCmd, void* pPara, MS_U32 ParaSize)
{
    int rtn = E_OK;

    if (pPara == NULL) {
        DISP_PRINTF_ERR("MS_Video_Disp_Get_Cmd invalid para");
        return E_ERR;
    }

    switch(eCmd)
    {

        case MS_DISP_CMD_GET_RENDER_DELAY_TIME:
            if (sizeof(MS_U32) == ParaSize) {
                bool bUseDipGop = false;
                DISP_LOCK(eWinID);
                if (g_pDispPath[eWinID] != NULL) {
                    bUseDipGop = g_pDispPath[eWinID]->mUseDipGop;
                }
                DISP_UNLOCK(eWinID);
                *(MS_U32 *)pPara = bUseDipGop ? 16 : Wrapper_GetRenderDelayTime(DisPlayMappingID[eWinID]);
            } else {
                rtn = E_ERR;
                DISP_PRINTF_ERR("MS_Video_Disp_Get_Cmd invalid para size %d", ParaSize);
            }
            break;

        default:
            rtn = E_ERR;
            DISP_PRINTF_ERR("MS_Video_Disp_Get_Cmd eCmd = %d", eCmd);
            break;
    }

    return rtn;
}
