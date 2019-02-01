//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2015 - 2016 MStar Semiconductor, Inc. All rights reserved.
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


#include "VideoSetWrapper.h"
#include <utils/Log.h>
#include "mi_display.h"
#include <platform.h>

#define LOG_TAG "VideoSetWrapper"

extern "C" void Wrapper_connect_sn();
extern "C" void Wrapper_connect_mi();
extern "C" void Wrapper_connect_ddi();
extern "C" void Wrapper_disconnect_sn(int enWin);
extern "C" void Wrapper_disconnect_mi(int enWin);
extern "C" void Wrapper_disconnect_ddi(int enWin);
extern "C" void Wrapper_open_resource_mi(int enWin);
extern "C" void Wrapper_enable_window_mi(int enWin, MS_WINDOW *win);
extern "C" void Wrapper_get_3DStatus_mi(int enWin, MS_BOOL *pbEnable);
extern "C" int Wrapper_get_resource_mi(int enWin, Wrapper_control_resource_st *pstCtrlRes);
extern "C" int Wrapper_Video_setMode_sn(MS_VideoInfo *info, int enWin);
extern "C" int Wrapper_Video_setMode_mi(MS_VideoInfo *info, int enWin);
extern "C" int Wrapper_Video_setMode_ddi(MS_VideoInfo *info, int enWin);
extern "C" int Wrapper_Video_setXCWin_sn(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio);
extern "C" int Wrapper_Video_setXCWin_mi(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio);
extern "C" int Wrapper_Video_setXCWin_ddi(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio);
extern "C" int Wrapper_Video_disableDS_sn(int enWin, int Mute);
extern "C" int Wrapper_Video_disableDS_mi(int enWin, int Mute);
extern "C" int Wrapper_Video_disableDS_ddi(int enWin, int Mute);
extern "C" void Wrapper_attach_callback_sn(E_CALLBACK_MODE eType, void(*pfn)(int));
extern "C" void Wrapper_attach_callback_mi(E_CALLBACK_MODE eType, void(*pfn)(int));
extern "C" void Wrapper_attach_callback_ddi(E_CALLBACK_MODE eType, void(*pfn)(int));
extern "C" int Wrapper_Video_DS_State_sn(int enWin);
extern "C" int Wrapper_Video_DS_State_mi(int enWin);
extern "C" int Wrapper_Video_DS_State_ddi(int enWin);
extern "C" void Wrapper_SurfaceVideoSizeChange_sn(int win, int x, int y, int width, int height, int SrcWidth, int SrcHeight, int lock);
extern "C" void Wrapper_SurfaceVideoSizeChange_mi(int win, int x, int y, int width, int height, int SrcWidth, int SrcHeight, int lock);
extern "C" void Wrapper_SurfaceVideoSizeChange_ddi(int win, int x, int y, int width, int height, int SrcWidth, int SrcHeight, int lock);
extern "C" void Wrapper_seq_change_sn(int enWin, MS_DispFrameFormat *dff, int forceDS);
extern "C" void Wrapper_seq_change_mi(int enWin, MS_DispFrameFormat *dff, int forceDS);
extern "C" void Wrapper_seq_change_ddi(int enWin, MS_DispFrameFormat *dff, int forceDS);
extern "C" void Wrapper_Video_setMute_sn(int enWin, int enable, int lock);
extern "C" void Wrapper_Video_setMute_mi(int enWin, int enable, int lock);
extern "C" void Wrapper_Video_setMute_ddi(int enWin, int enable, int lock);
extern "C" void Wrapper_SetDispWin_sn(int enWin, int x, int y, int width, int height, MS_VideoInfo *info);
extern "C" void Wrapper_SetDispWin_mi(int enWin, int x, int y, int width, int height, MS_VideoInfo *info);
extern "C" void Wrapper_SetDispWin_ddi(int enWin, int x, int y, int width, int height, MS_VideoInfo *info);
extern "C" int Wrapper_get_Status_sn(int enWin);
extern "C" int Wrapper_get_Status_mi(int enWin);
extern "C" int Wrapper_get_Status_ddi(int enWin);
extern "C" int Wrapper_Video_need_seq_change_sn(int enWin, int width, int height);
extern "C" int Wrapper_Video_need_seq_change_mi(int enWin, int width, int height);
extern "C" int Wrapper_Video_need_seq_change_ddi(int enWin, int width, int height);
extern "C" int Wrapper_SetHdrMetadata_sn(int enWin, const MS_HDRInfo *pHdrInfo);
extern "C" int Wrapper_SetHdrMetadata_mi(int enWin, const MS_HDRInfo *pHdrInfo);
extern "C" int Wrapper_SetHdrMetadata_ddi(int enWin, const MS_HDRInfo *pHdrInfo);
extern "C" int Wrapper_GetRenderDelayTime_sn(int enWin);
extern "C" int Wrapper_GetRenderDelayTime_mi(int enWin);
extern "C" int Wrapper_GetRenderDelayTime_ddi(int enWin);

typedef enum
{
    E_WRAPPER_PATH_SN = 0,
    E_WRAPPER_PATH_MI,
    E_WRAPPER_PATH_DDI,
    E_WRAPPER_PATH_NUM,
} EN_WRAPPER_PATH;

typedef struct
{
    void    (*pfconnect)(void);
    void    (*pfdisconnect)(int enWin);
    void    (*pfopen_resource)(int enWin);
    void    (*pfenable_window)(int enWin, MS_WINDOW *win);
    void    (*pfget_3DStatus)(int enWin, MS_BOOL *pbEnable);
    int     (*pfget_resource)(int enWin, Wrapper_control_resource_st *pstCtrlRes);
    int     (*pfVideo_setMode)(MS_VideoInfo *info, int enWin);
    int     (*pfVideo_setXCWin)(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio);
    int     (*pfVideo_disableDS)(int enWin, int Mute);
    void    (*pfattach_callback)(E_CALLBACK_MODE eType, void(*pfn)(int));
    int     (*pfVideo_DS_State)(int enWin);
    void    (*pfSurfaceVideoSizeChange)(int win, int x, int y, int width, int height, int SrcWidth, int SrcHeight, int lock);
    void    (*pfseq_change)(int enWin, MS_DispFrameFormat *dff, int forceDS);
    void    (*pfVideo_setMute)(int enWin, int enable, int lock);
    void    (*pfSetDispWin)(int enWin, int x, int y, int width, int height, MS_VideoInfo *info);
    int     (*pfget_Status)(int enWin);
    int     (*pfVideo_need_seq_change)(int enWin, int width, int height);
    int    (*pfSetHdrMetadata)(int enWin, const MS_HDRInfo *pHdrInfo);
    int    (*pfGetRenderDelayTime)(int enWin);
} ST_WRAPPER_PATH, *PST_WRAPPER_PATH;

static const ST_WRAPPER_PATH g_fnWrapperPath[E_WRAPPER_PATH_NUM] =
{
    {
        Wrapper_connect_sn, Wrapper_disconnect_sn, NULL, NULL, NULL, NULL, Wrapper_Video_setMode_sn, Wrapper_Video_setXCWin_sn,
        Wrapper_Video_disableDS_sn, Wrapper_attach_callback_sn, Wrapper_Video_DS_State_sn, Wrapper_SurfaceVideoSizeChange_sn,
        Wrapper_seq_change_sn, Wrapper_Video_setMute_sn, Wrapper_SetDispWin_sn, Wrapper_get_Status_sn, Wrapper_Video_need_seq_change_sn,
        Wrapper_SetHdrMetadata_sn, Wrapper_GetRenderDelayTime_sn
    },

    {
        Wrapper_connect_mi, Wrapper_disconnect_mi, Wrapper_open_resource_mi, Wrapper_enable_window_mi, Wrapper_get_3DStatus_mi,
        Wrapper_get_resource_mi, Wrapper_Video_setMode_mi, Wrapper_Video_setXCWin_mi, Wrapper_Video_disableDS_mi, Wrapper_attach_callback_mi,
        Wrapper_Video_DS_State_mi, Wrapper_SurfaceVideoSizeChange_mi, Wrapper_seq_change_mi, Wrapper_Video_setMute_mi, Wrapper_SetDispWin_mi,
        Wrapper_get_Status_mi, Wrapper_Video_need_seq_change_mi, Wrapper_SetHdrMetadata_mi, Wrapper_GetRenderDelayTime_mi
    },

    {
        Wrapper_connect_ddi, Wrapper_disconnect_ddi, NULL, NULL, NULL, NULL, Wrapper_Video_setMode_ddi, Wrapper_Video_setXCWin_ddi,
        Wrapper_Video_disableDS_ddi, Wrapper_attach_callback_ddi, Wrapper_Video_DS_State_ddi, Wrapper_SurfaceVideoSizeChange_ddi,
        Wrapper_seq_change_ddi, Wrapper_Video_setMute_ddi, Wrapper_SetDispWin_ddi, Wrapper_get_Status_ddi, Wrapper_Video_need_seq_change_ddi,
        Wrapper_SetHdrMetadata_ddi, Wrapper_GetRenderDelayTime_ddi
    },
};

static BUILD_PLATFORM platform = BUILD_PLATFORM_UNKNOWN;

extern "C" void Wrapper_connect()
{
    platform = get_build_platform();
    if (BUILD_PLATFORM_UNKNOWN == platform) {
        ALOGE("Error: fail to get build platform!");
        return;
    }

    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfconnect();
    }
}

extern "C" void Wrapper_disconnect(int enWin)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfdisconnect(enWin);
    }
}

extern "C" void Wrapper_open_resource(int enWin)
{
    if ((BUILD_PLATFORM_MI == platform) && platform <= E_WRAPPER_PATH_NUM) {
        g_fnWrapperPath[platform - 1].pfopen_resource(enWin);
    }
}

extern "C" void Wrapper_enable_window(int enWin, MS_WINDOW *win)
{
    if ((BUILD_PLATFORM_MI == platform) && platform <= E_WRAPPER_PATH_NUM) {
        g_fnWrapperPath[platform - 1].pfenable_window(enWin, win);
    }
}

extern "C" void Wrapper_get_3DStatus(int enWin, MS_BOOL *pbEnable)
{
    if ((BUILD_PLATFORM_MI == platform) && platform <= E_WRAPPER_PATH_NUM) {
        g_fnWrapperPath[platform - 1].pfget_3DStatus(enWin, pbEnable);
    }
}

extern "C" int Wrapper_get_resource(int enWin, Wrapper_control_resource_st *pstCtrlRes)
{
    if ((BUILD_PLATFORM_MI == platform) && platform <= E_WRAPPER_PATH_NUM) {
        return g_fnWrapperPath[platform - 1].pfget_resource(enWin, pstCtrlRes);
    }

    return 0;
}

extern "C" int Wrapper_Video_setMode(MS_VideoInfo *info, int enWin)
{
    // per PM / XC request, under 23 fps needs set as 30 fps for performance issue.
    info->u16FrameRate = info->u16FrameRate > 23000 ? info->u16FrameRate : 30000;
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfVideo_setMode(info, enWin);
    }
    return 0;
}

extern "C" int Wrapper_Video_setXCWin(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfVideo_setXCWin(x, y, width, height, info, enWin, fRatio);
    }
    return 0;
}

extern "C" int Wrapper_Video_disableDS(int enWin, int Mute)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfVideo_disableDS(enWin, Mute);
    }
    return 0;
}

extern "C" void Wrapper_attach_callback(E_CALLBACK_MODE eType, void(*pfn)(int))
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfattach_callback(eType, pfn);
    }
}

extern "C" int Wrapper_Video_DS_State(int enWin)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfVideo_DS_State(enWin);
    }
    return 0;
}

extern "C" void Wrapper_SurfaceVideoSizeChange(int win, int x, int y, int width, int height, int SrcWidth, int SrcHeight, int lock)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfSurfaceVideoSizeChange(win, x, y, width, height, SrcWidth, SrcHeight, lock);
    }
}

extern "C" void Wrapper_seq_change(int enWin, MS_DispFrameFormat *dff, int forceDS)
{
    // per PM / XC request, under 23 fps needs set as 30 fps for performance issue.
    dff->u32FrameRate = dff->u32FrameRate > 23000 ? dff->u32FrameRate : 30000;
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfseq_change(enWin, dff, forceDS);
    }
}

extern "C" void Wrapper_Video_setMute(int enWin, int enable, int lock)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfVideo_setMute(enWin, enable, lock);
    }
}

extern "C" void Wrapper_SetDispWin(int enWin, int x, int y, int width, int height, MS_VideoInfo *info)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        g_fnWrapperPath[platform - 1].pfSetDispWin(enWin, x, y, width, height, info);
    }
}

extern "C" int Wrapper_get_Status(int enWin)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfget_Status(enWin);
    }
    return 0;
}

extern "C" int Wrapper_Video_need_seq_change(int enWin, int width, int height)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfVideo_need_seq_change(enWin, width, height);
    }
    return 0;
}

extern "C" int Wrapper_SetHdrMetadata(int enWin, const MS_HDRInfo *pHdrInfo)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfSetHdrMetadata(enWin, pHdrInfo);
    }
    return 0;
}

extern "C" int Wrapper_GetRenderDelayTime(int enWin)
{
    if ((BUILD_PLATFORM_UNKNOWN != platform) && (platform <= E_WRAPPER_PATH_NUM)) {
        return g_fnWrapperPath[platform - 1].pfGetRenderDelayTime(enWin);
    }
    return 0;
}
