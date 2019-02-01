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
#include "VideoSetWrapper.h"
#include <errno.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <mmap.h>
#include <MsTypesEx.h>
#include <MsCommon.h>
#include "vsync_bridge.h"

static int used[2] = {0};
static int WIN_Initialized[2] = {0};
static int DS_Enabled[2] = {0};
static int DS_Restore[2] = {0};
static MS_VideoInfo SetMode_Vinfo[2];
static MS_VideoInfo SetWin_Vinfo[2];
static MS_WINDOW VideoWin[2];

static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
#define VIDEOSET_LOCK  pthread_mutex_lock(&sLock)
#define VIDEOSET_UNLOCK pthread_mutex_unlock(&sLock)

void (*fpSetAspectRatio)(int32_t u32Value) = NULL;

extern "C" void Wrapper_connect()
{

}

extern "C" void Wrapper_attach_callback(E_CALLBACK_MODE eType, void(*pfn)(int))
{

}

extern "C" void Wrapper_seq_change(int enWin, MS_DispFrameFormat *dff, int forceDS)
{

}

extern "C" void Wrapper_disconnect(int enWin)
{

}

extern "C" int Wrapper_Video_setMode(MS_VideoInfo *info, int enWin)
{
    return 0;
}

extern "C" int Wrapper_Video_setXCWin(int x, int y, int width, int height, MS_VideoInfo *info, int enWin, float fRatio)
{
    return 0;
}

extern "C" int Wrapper_Video_disableDS(int enWin, int Mute)
{
    return 0;
}

int Wrapper_Video_closeDS(int enWin, int Mute, int lock)
{
    return 0;
}

extern "C" int Wrapper_Video_DS_State(int enWin)
{
    return 0;
}

extern "C" void Wrapper_SurfaceVideoSizeChange(int win,int x,int y,int width,int height,int SrcWidth,int SrcHeight, int lock)
{
}

extern "C" void Wrapper_Video_setMute(int enWin, int enable, int lock)
{
}

extern "C" void Wrapper_SetDispWin(int enWin, int x, int y, int width, int height)
{
}

extern "C" int Wrapper_get_Status(int enWin)
{
}

extern "C" int Wrapper_Video_need_seq_change(int enWin, int width, int height)
{
    return 0;
}

extern "C" int Wrapper_SetHdrMetadata(int enWin, const MS_HDRInfo *pHdrInfo)
{
    return -1;
}

extern "C" int Wrapper_GetRenderDelayTime(int enWin) {
    return 0;
}

extern "C" void Wrapper_UpdateModeSize(int enWin, int horSize, int verSize, int cropLeft, int cropRight, int cropTop, int cropBottom)
{
}

void Wrapper_Enable3D(int enWin)
{
}

void Wrapper_Disable3D(int enWin)
{
}
