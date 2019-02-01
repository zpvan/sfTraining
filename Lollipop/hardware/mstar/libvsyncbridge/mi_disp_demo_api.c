//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
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
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (!MStar Confidential Information!L) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   mi_demo.c
/// @author MStar Semiconductor Inc.
/// @brief  Uranus Sample Code Command Shell
///////////////////////////////////////////////////////////////////////////////////////////////////

/*
   @file mi_demo.c
   @brief The demo code for mi_xxx funcion

*/
//-------------------------------------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------------------------------------
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "mi_common.h"
#include "mi_os.h"
#include "mi_disp.h"
#include "mi_video.h"
#include "mi_vmon.h"
//#include "mi_mm.h"

//-------------------------------------------------------------------------------------------------
//  Debug Defines
//-------------------------------------------------------------------------------------------------
#define CHECK_RETURN(x)   \
        if(x != MI_OK)    \
        {   \
            return FALSE;   \
        }

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define VIDEO_SIZE_DEC_RATE 0.1  ///< video size decrease rate
#define VIDEO_SIZE_INC_STEP 5 ///< video size increase step

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
MI_HANDLE _hMainVidWin, _hSdVidWin;

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------



//------------------------------------------------------------------------------
/// @brief DISP help menu
/// @return None
/// @sa
/// @note
//------------------------------------------------------------------------------
MI_BOOL MI_DEMO_DISP_Help(void)
{
    printf("*************************DISP function List*************************\n");

    printf("*************************End of DISP function List*************************\n");
    return MI_OK;
}

MI_BOOL MI_DEMO_DISP_Init(MI_HANDLE *phVidWin, MI_HANDLE hInputSrc)
{
    MI_DISP_InitParams_t stInitParams;
    MI_DISP_VidWin_InitParams_t stMainVidWinInit, stSdVidWinInit;

    stInitParams.u32OutputPath = E_MI_DISP_OUTPUT_PATH_HDMI | E_MI_DISP_OUTPUT_PATH_YPBPR | E_MI_DISP_OUTPUT_PATH_CVBS;
    stInitParams.eOutputTiming = E_MI_DISP_OUTPUT_TIMING_1920X1080_50I;
    stInitParams.eTVSys = E_MI_DISP_TV_SYS_PAL;
    stInitParams.eHDMITxMode = E_MI_DISP_HDMITx_MODE_HDMI_HDCP;
    stInitParams.eHDMITxCD = E_MI_DISP_HDMITX_CD_8BITS;

    stInitParams.stDacParams[E_MI_DISP_DAC_HD].eDacMode = E_MI_DISP_DAC_MODE_YPBPR;
    stInitParams.stDacParams[E_MI_DISP_DAC_HD].eDacEnCtrl = E_MI_DISP_DAC_ENABLE_VYU;
    stInitParams.stDacParams[E_MI_DISP_DAC_HD].eDacLevel = E_MI_DISP_DAC_LEVEL_LOW;
    stInitParams.stDacParams[E_MI_DISP_DAC_HD].eDacSwap = E_MI_DISP_DAC_SWAP_V_Y_U;
    stInitParams.stDacParams[E_MI_DISP_DAC_HD].eDacCurrent = E_MI_DISP_DAC_CURRENT_FULL;

    stInitParams.stDacParams[E_MI_DISP_DAC_SD].eDacMode = E_MI_DISP_DAC_MODE_CVBS_SVIDEO;
    stInitParams.stDacParams[E_MI_DISP_DAC_SD].eDacEnCtrl = E_MI_DISP_DAC_ENABLE_VYU;
    stInitParams.stDacParams[E_MI_DISP_DAC_SD].eDacLevel = E_MI_DISP_DAC_LEVEL_HIGH;
    stInitParams.stDacParams[E_MI_DISP_DAC_SD].eDacSwap = E_MI_DISP_DAC_SWAP_V_Y_U;
    stInitParams.stDacParams[E_MI_DISP_DAC_SD].eDacCurrent = E_MI_DISP_DAC_CURRENT_FULL;

    stMainVidWinInit.eVidWinType = E_MI_DISP_VIDWIN_MAIN;
    stSdVidWinInit.eVidWinType = E_MI_DISP_VIDWIN_SD;
    CHECK_RETURN(MI_DISP_Init(&stInitParams));
    CHECK_RETURN(MI_DISP_Open(&stMainVidWinInit, &_hMainVidWin));
    CHECK_RETURN(MI_DISP_AddInput(_hMainVidWin, hInputSrc));
    *phVidWin = _hMainVidWin;
    CHECK_RETURN(MI_DISP_Open(&stSdVidWinInit, &_hSdVidWin));
    CHECK_RETURN(MI_DISP_SetSource(_hSdVidWin, E_MI_DISP_VIDWIN_SRC_VIDWIN_MAIN));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_Exit(void)
{
    CHECK_RETURN(MI_DISP_DeInit());
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetAR(MI_U32 *eAR, MI_U32 *eTVAR)
{
    CHECK_RETURN(MI_DISP_SetAspectRatio(_hMainVidWin, *eAR, *eTVAR));
    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetMute(void)
{
    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetUnMute(void)
{
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetOutputTiming(MI_U32 *pu32OutputTiming)
{
    MI_DISP_TVSystem_e eTVSys = E_MI_DISP_TV_SYS_PAL;
    MI_BOOL bFrameRdy;
    MI_HANDLE hZapDec;
    MI_DISP_VidWin_Rect_t stDispRect;

    CHECK_RETURN(MI_DISP_SetOutputTiming(*pu32OutputTiming));

    switch (*pu32OutputTiming)
    {
        case E_MI_DISP_OUTPUT_TIMING_720X576_50I:
        case E_MI_DISP_OUTPUT_TIMING_720X576_50P:
            stDispRect.u16X=0;
            stDispRect.u16Y=0;
            stDispRect.u16Width=720;
            stDispRect.u16Height=576;
            eTVSys = E_MI_DISP_TV_SYS_PAL;
            break;
        case E_MI_DISP_OUTPUT_TIMING_1280X720_50P:
            stDispRect.u16X=0;
            stDispRect.u16Y=0;
            stDispRect.u16Width=1280;
            stDispRect.u16Height=720;
            eTVSys = E_MI_DISP_TV_SYS_PAL;
            break;
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_50I:
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_24P:
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_25P:
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_50P:
        default :
            stDispRect.u16X=0;
            stDispRect.u16Y=0;
            stDispRect.u16Width=1920;
            stDispRect.u16Height=1080;
            eTVSys = E_MI_DISP_TV_SYS_PAL;
            break;

        case E_MI_DISP_OUTPUT_TIMING_720X480_60I:
        case E_MI_DISP_OUTPUT_TIMING_720X480_60P:
            stDispRect.u16X=0;
            stDispRect.u16Y=0;
            stDispRect.u16Width=720;
            stDispRect.u16Height=480;
            eTVSys = E_MI_DISP_TV_SYS_NTSC;
            break;
        case E_MI_DISP_OUTPUT_TIMING_1280X720_60P:
            stDispRect.u16X=0;
            stDispRect.u16Y=0;
            stDispRect.u16Width=1280;
            stDispRect.u16Height=720;
            eTVSys = E_MI_DISP_TV_SYS_NTSC;
            break;
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_60I:
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_30P:
        case E_MI_DISP_OUTPUT_TIMING_1920X1080_60P:
            stDispRect.u16X=0;
            stDispRect.u16Y=0;
            stDispRect.u16Width=1920;
            stDispRect.u16Height=1080;
            eTVSys = E_MI_DISP_TV_SYS_NTSC;
            break;
    }

    CHECK_RETURN(MI_DISP_SetTvSystem(eTVSys));

    CHECK_RETURN(MI_VMON_GetHandle(E_MI_VMON_PATH_MAIN, &hZapDec));
#if 0 //Fixme: Richard Deng: napoli android mi not support mi_mm
    MI_MM_MovieInfo_t MovieInfo;
    MI_HANDLE MMHandle;
    MI_MM_Path_e MMPath=E_MI_MM_PATH_MAIN;
    MI_MM_GetHandle(MMPath,&MMHandle);
#endif
    if(MI_VMON_GetAttr(hZapDec, E_MI_VMON_PARAMTYPE_GET_FRAMERDY, (void *)&bFrameRdy) == MI_OK)
    {
        CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    }
#if 0 //Fixme: Richard Deng: napoli android mi not support mi_mm
    else if(MI_MM_GetMovieInfo(MMHandle,&MovieInfo)==MI_OK) // only can get mm video info if playing
    {
        MI_MM_SetOption(MMHandle,E_MI_MPLAYER_OPTION_DISPLAY_WINDOW,(MI_U32) &stDispRect);
        //MI_MM_SetOption(MMHandle,E_MI_MPLAYER_OPTION_DISPLAY_WINDOW,(MI_U32) (*pu32OutputTiming));
    }
#endif
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoTestOutputTiming(void)
{
    MI_U8 i = 0;
    MI_DISP_TVSystem_e eTVSys = E_MI_DISP_TV_SYS_PAL;

    for(i = 0; i < E_MI_DISP_OUTPUTTIMING_NUM; i++)
    {
        if(( E_MI_DISP_OUTPUT_TIMING_1920X1080_24P == i) ||
            ( E_MI_DISP_OUTPUT_TIMING_1920X1080_25P == i) ||
            (E_MI_DISP_OUTPUT_TIMING_1920X1080_30P == i))
        {
            continue;
        }

        CHECK_RETURN(MI_DISP_SetOutputTiming(i));

        switch (i)
        {
            case E_MI_DISP_OUTPUT_TIMING_720X576_50I:
            case E_MI_DISP_OUTPUT_TIMING_720X576_50P:
            case E_MI_DISP_OUTPUT_TIMING_1280X720_50P:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_50I:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_24P:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_25P:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_50P:
            default :
                eTVSys = E_MI_DISP_TV_SYS_PAL;
                break;

            case E_MI_DISP_OUTPUT_TIMING_720X480_60I:
            case E_MI_DISP_OUTPUT_TIMING_720X480_60P:
            case E_MI_DISP_OUTPUT_TIMING_1280X720_60P:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_60I:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_30P:
            case E_MI_DISP_OUTPUT_TIMING_1920X1080_60P:
                eTVSys = E_MI_DISP_TV_SYS_NTSC;
                break;
        }

        CHECK_RETURN(MI_DISP_SetTvSystem(eTVSys));
        CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
        CHECK_RETURN(MI_OS_DelayTask(15000));
    }

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetVideoAlpha(MI_U32 *pu32VideoAlpha)
{
    CHECK_RETURN(MI_DISP_SetAlpha(_hMainVidWin, *pu32VideoAlpha));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetBgColor(MI_U32 *pu32BgColor)
{
    MI_DISP_VidWin_BgColor_t stBgColor;

    if(*pu32BgColor == 0)
    {
        stBgColor.u8Red = 0;
        stBgColor.u8Green = 0;
        stBgColor.u8Blue = 0;
    }
    else if(*pu32BgColor == 1)
    {
        stBgColor.u8Red = 255;
        stBgColor.u8Green = 0;
        stBgColor.u8Blue = 0;
    }
    else if(*pu32BgColor == 2)
    {
        stBgColor.u8Red = 0;
        stBgColor.u8Green = 255;
        stBgColor.u8Blue = 0;
    }
    else if(*pu32BgColor == 3)
    {
        stBgColor.u8Red = 0;
        stBgColor.u8Green = 0;
        stBgColor.u8Blue = 255;
    }
    else if(*pu32BgColor == 4)
    {
        stBgColor.u8Red = 255;
        stBgColor.u8Green = 255;
        stBgColor.u8Blue = 0;
    }
    else if(*pu32BgColor == 5)
    {
        stBgColor.u8Red = 255;
        stBgColor.u8Green = 0;
        stBgColor.u8Blue = 255;
    }
    else if(*pu32BgColor == 6)
    {
        stBgColor.u8Red = 0;
        stBgColor.u8Green = 255;
        stBgColor.u8Blue = 255;
    }
    else if(*pu32BgColor == 7)
    {
        stBgColor.u8Red = 255;
        stBgColor.u8Green = 255;
        stBgColor.u8Blue = 255;
    }
    CHECK_RETURN(MI_DISP_SetBgColor(_hMainVidWin, &stBgColor));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_GetBgColor(void)
{
    MI_DISP_VidWin_BgColor_t stBgColor;

    CHECK_RETURN(MI_DISP_GetBgColor(_hMainVidWin, &stBgColor));
    printf("BgColor(R: %d, G: %d, B: %d)\n", stBgColor.u8Red, stBgColor.u8Green, stBgColor.u8Blue);

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoScalingDown(void)
{
    MI_FLOAT fVideoSizeDecRate = 0.0;
    MI_DISP_VidWin_Rect_t stDispRect;
    MI_DISP_VidWin_Rect_t stOutRect;

    CHECK_RETURN(MI_DISP_GetDispRect(_hMainVidWin, &stDispRect));

    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stDispRect));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_OS_DelayTask(3000));

    // Same aspect video size decreasing
    for(fVideoSizeDecRate = VIDEO_SIZE_DEC_RATE; fVideoSizeDecRate < 1; fVideoSizeDecRate = fVideoSizeDecRate + VIDEO_SIZE_DEC_RATE)
    {
        printf("DEC RATE = %lf\n", fVideoSizeDecRate);
        CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

        stOutRect.u16X = (stDispRect.u16Width * fVideoSizeDecRate) / 2;
        stOutRect.u16Y = (stDispRect.u16Height * fVideoSizeDecRate) / 2;
        stOutRect.u16Width = stDispRect.u16Width - (stOutRect.u16X << 1);
        stOutRect.u16Height = stDispRect.u16Height -(stOutRect.u16Y << 1);

        CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stOutRect));
        CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
        CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
        CHECK_RETURN(MI_OS_DelayTask(3000));
    }

    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stDispRect));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_OS_DelayTask(3000));

    // Only H-size decreasing
    for(fVideoSizeDecRate = VIDEO_SIZE_DEC_RATE; fVideoSizeDecRate < 1; fVideoSizeDecRate = fVideoSizeDecRate + VIDEO_SIZE_DEC_RATE)
    {
        printf("DEC RATE = %lf\n", fVideoSizeDecRate);
        CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

        stOutRect.u16X = (stDispRect.u16Width * fVideoSizeDecRate)/2;
        stOutRect.u16Y = 0;
        stOutRect.u16Width = stDispRect.u16Width - (stOutRect.u16X << 1);
        stOutRect.u16Height = stDispRect.u16Height;

        CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stOutRect));
        CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
        CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
        CHECK_RETURN(MI_OS_DelayTask(3000));
    }

    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stDispRect));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_OS_DelayTask(3000));

    // Only V-size decreasing
    for(fVideoSizeDecRate = VIDEO_SIZE_DEC_RATE; fVideoSizeDecRate < 1; fVideoSizeDecRate = fVideoSizeDecRate + VIDEO_SIZE_DEC_RATE)
    {
        printf("DEC RATE = %lf\n", fVideoSizeDecRate);
        CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

        stOutRect.u16X = 0;
        stOutRect.u16Y = (stDispRect.u16Height * fVideoSizeDecRate) / 2;
        stOutRect.u16Width = stDispRect.u16Width;
        stOutRect.u16Height = stDispRect.u16Height - (stOutRect.u16Y << 1);

        CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stOutRect));
        CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
        CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
        CHECK_RETURN(MI_OS_DelayTask(3000));
    }

    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stDispRect));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_OS_DelayTask(3000));

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoScalingUp(void)
{
    MI_FLOAT fVideoSizeIncRatio = 1.0;
    MI_FLOAT fVideoSizeIncRate = 0.0;
    MI_BOOL isWidthLimit = FALSE, isHeightLimit = FALSE;
    MI_DISP_VidWin_Rect_t stDispRect;
    MI_DISP_VidWin_Rect_t stOutRect;
    MI_HANDLE hMainVidPath;
    MI_VIDEO_DecInfo_t stVidStatus;

    CHECK_RETURN(MI_DISP_GetDispRect(_hMainVidWin, &stDispRect));
    CHECK_RETURN(MI_VIDEO_GetHandle(E_MI_VIDEO_DECODER_MAIN, &hMainVidPath));
    CHECK_RETURN(MI_VIDEO_GetAttr(hMainVidPath, E_MI_VIDEO_PARAMTYPE_GET_DECINFO, (void *)&stVidStatus));

    stOutRect.u16Width = stVidStatus.u16HorSize;
    stOutRect.u16Height = stVidStatus.u16VerSize;
    stOutRect.u16X = (stDispRect.u16Width - stOutRect.u16Width) / 2;
    stOutRect.u16Y = (stDispRect.u16Height - stOutRect.u16Height) / 2;

    // Source video size is larger than or equal to the panel size, no need to do scaling up test
    if((stOutRect.u16Width >= stDispRect.u16Width) && (stOutRect.u16Height >= stDispRect.u16Height))
    {
        return TRUE;
    }

    // Set initial video size to original video size
    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stOutRect));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_OS_DelayTask(3000));

    // Make video size grow VIDEO_SIZE_INC_STEP times to reach one side maximum of panel
    if((stOutRect.u16Width / stOutRect.u16Height) >= (stDispRect.u16Width / stDispRect.u16Height))
    {
        fVideoSizeIncRate = ((MI_FLOAT)(stDispRect.u16Width - stOutRect.u16Width) / VIDEO_SIZE_INC_STEP) / stVidStatus.u16HorSize;
    }
    else
    {
        fVideoSizeIncRate = ((MI_FLOAT)(stDispRect.u16Height - stOutRect.u16Height) / VIDEO_SIZE_INC_STEP) / stVidStatus.u16VerSize;
    }

    while(!isWidthLimit || !isHeightLimit)
    {
        printf("RATIO=%lf, RATE=%lf\n", fVideoSizeIncRatio, fVideoSizeIncRate);
        CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

        fVideoSizeIncRatio += fVideoSizeIncRate;

        if(!isWidthLimit)
        {
            stOutRect.u16Width = stVidStatus.u16HorSize * fVideoSizeIncRatio;
            stOutRect.u16X = (stDispRect.u16Width - stOutRect.u16Width) / 2;
        }
        if(!isHeightLimit)
        {
            stOutRect.u16Height = stVidStatus.u16VerSize * fVideoSizeIncRatio;
            stOutRect.u16Y = (stDispRect.u16Height - stOutRect.u16Height) / 2;
        }

        // Once one side reach maximum, make another side keeps growing and reach maximum in VIDEO_SIZE_INC_STEP times
        if((stOutRect.u16Width >= stDispRect.u16Width) && (!isWidthLimit))
        {
            isWidthLimit = TRUE;
            stOutRect.u16Width = stDispRect.u16Width;
            stOutRect.u16X = 0;
            fVideoSizeIncRate = ((MI_FLOAT)(stDispRect.u16Height - stOutRect.u16Height) / VIDEO_SIZE_INC_STEP) / stVidStatus.u16HorSize;
        }
        if((stOutRect.u16Height >= stDispRect.u16Height) && (!isHeightLimit))
        {
            isHeightLimit = TRUE;
            stOutRect.u16Height = stDispRect.u16Height;
            stOutRect.u16Y = 0;
            fVideoSizeIncRate = ((MI_FLOAT)(stDispRect.u16Width - stOutRect.u16Width) / VIDEO_SIZE_INC_STEP) / stVidStatus.u16VerSize;
        }

        CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stOutRect));
        CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
        CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
        CHECK_RETURN(MI_OS_DelayTask(3000));
    }

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoTestBrightness(void)
{
    MI_U32 u32Brightness;
    MI_U8 i = 0x00;

    //light -> default
    for(i = 0x7F; i > 0x00;  i >>= 1)
    {
        u32Brightness = 0x80|i;
        printf("Set Brightness: %ld\n", u32Brightness);
        CHECK_RETURN(MI_DISP_SetBrightness(_hMainVidWin, u32Brightness));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetBrightness(_hMainVidWin, &u32Brightness));
        printf("Get Brightness: %ld\n", u32Brightness);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    CHECK_RETURN(MI_DISP_SetBrightness(_hMainVidWin, 0x80));

    //default ->dark
    for (i = 0x7F; i > 0; i <<= 1)
    {
        u32Brightness = (0x7F&i);
        printf("Set Brigthness: %ld\n", u32Brightness);
        CHECK_RETURN(MI_DISP_SetBrightness(_hMainVidWin, u32Brightness));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetBrightness(_hMainVidWin, &u32Brightness));
        printf("Get Brightness: %ld\n", u32Brightness);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    CHECK_RETURN(MI_DISP_SetBrightness(_hMainVidWin, 0x80));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoTestContrast(void)
{
    MI_U32 u32Contrast;
    MI_U8 i;

    for(i = 0; i < 255; i += 15)
    {
        u32Contrast = i;
        printf("Set Contrast: %ld\n", u32Contrast);
        CHECK_RETURN(MI_DISP_SetContrast(_hMainVidWin, u32Contrast));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetContrast(_hMainVidWin, &u32Contrast));
        printf("Get Contrast: %ld\n", u32Contrast);
        CHECK_RETURN(MI_OS_DelayTask(1000));

    }
    for(; i > 0; i -= 15)
    {
        u32Contrast = i;
        printf("Set Contrast: %ld\n", u32Contrast);
        CHECK_RETURN(MI_DISP_SetContrast(_hMainVidWin, u32Contrast));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetContrast(_hMainVidWin, &u32Contrast));
        printf("Get Contrast: %ld\n", u32Contrast);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    CHECK_RETURN(MI_DISP_SetContrast(_hMainVidWin, 0x80));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoTestHue(void)
{
    MI_U32 u32Hue;
    MI_U8 i;

    for(i = 0; i < 255; i += 15)
    {
        u32Hue = i;
        printf("Set Hue: %ld\n", u32Hue);
        CHECK_RETURN(MI_DISP_SetHue(_hMainVidWin, u32Hue));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetHue(_hMainVidWin, &u32Hue));
        printf("Get Hue: %ld\n", u32Hue);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    for(; i > 0; i -= 15)
    {
        u32Hue = i;
        printf("Set Hue: %ld\n", u32Hue);
        CHECK_RETURN(MI_DISP_SetHue(_hMainVidWin, u32Hue));
        MI_OS_DelayTask(1000);
        CHECK_RETURN(MI_DISP_GetHue(_hMainVidWin, &u32Hue));
        printf("Get Hue: %ld\n", u32Hue);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    CHECK_RETURN(MI_DISP_SetHue(_hMainVidWin, 0x80));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_AutoTestSaturation(void)
{
    MI_U32 u32Saturation;
    MI_U8 i;

    for(i = 0; i < 255; i += 15)
    {
        u32Saturation = i;
        printf("Set Saturation: %ld\n", u32Saturation);
        CHECK_RETURN(MI_DISP_SetSaturation(_hMainVidWin, u32Saturation));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetSaturation(_hMainVidWin, &u32Saturation));
        printf("Get Saturation: %ld\n", u32Saturation);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    for(; i > 0; i -= 15)
    {
        u32Saturation = i;
        printf("Set Saturation: %ld\n", u32Saturation);
        CHECK_RETURN(MI_DISP_SetSaturation(_hMainVidWin, u32Saturation));
        CHECK_RETURN(MI_OS_DelayTask(1000));
        CHECK_RETURN(MI_DISP_GetSaturation(_hMainVidWin, &u32Saturation));
        printf("Get Saturation: %ld\n", u32Saturation);
        CHECK_RETURN(MI_OS_DelayTask(1000));
    }
    CHECK_RETURN(MI_DISP_SetSaturation(_hMainVidWin, 0x80));
    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetInputRect(MI_U32 *pu32X, MI_U32 *pu32Y, MI_U32 *pu32Width, MI_U32 *pu32Height)
{
    MI_DISP_VidWin_Rect_t stInRect;

    stInRect.u16X = (MI_U16)*pu32X;
    stInRect.u16Y = (MI_U16)*pu32Y;
    stInRect.u16Width = (MI_U16)*pu32Width;
    stInRect.u16Height = (MI_U16)*pu32Height;

    CHECK_RETURN(MI_DISP_SetInputRect(_hMainVidWin, &stInRect));
    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetOutputRect(MI_U32 *pu32X,MI_U32 *pu32Y,MI_U32 *pu32Width,MI_U32 *pu32Height)
{
    MI_DISP_VidWin_Rect_t stOutRect;

    stOutRect.u16X = (MI_U16)*pu32X;
    stOutRect.u16Y = (MI_U16)*pu32Y;
    stOutRect.u16Width = (MI_U16)*pu32Width;
    stOutRect.u16Height = (MI_U16)*pu32Height;

    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stOutRect));
    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_ResetOutputRect(void)
{
    MI_DISP_VidWin_Rect_t stDispRect;

    CHECK_RETURN(MI_DISP_GetDispRect(_hMainVidWin, &stDispRect));

    CHECK_RETURN(MI_DISP_SetOutputRect(_hMainVidWin, &stDispRect));
    CHECK_RETURN(MI_DISP_SetAspectRatio(_hMainVidWin, E_MI_DISP_VIDWIN_AR_FULL, E_MI_DISP_TV_AR_16_9));

    CHECK_RETURN(MI_DISP_SetMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));
    CHECK_RETURN(MI_DISP_ApplySetting(_hMainVidWin, TRUE));
    CHECK_RETURN(MI_DISP_SetUnMute(_hMainVidWin, E_MI_DISP_VIDEO_MUTE_MODE_IMMEDIATE, E_MI_DISP_MUTE_FLAG_SETWINDOW));

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetZOrder(MI_U32 *pu32ZOrder)
{
    MI_DISP_VidWin_ZOrder_e eZOrder;

    switch(*pu32ZOrder)
    {
        case 0:
            eZOrder = E_MI_DISP_VIDWIN_ZORDER_MOVE_FRONTMOST;
            break;
       case 1:
            eZOrder = E_MI_DISP_VIDWIN_ZORDER_MOVE_BACKMOST;
            break;
       case 2:
            eZOrder = E_MI_DISP_VIDWIN_ZORDER_MOVE_FORWARD;
            break;
       case 3:
            eZOrder = E_MI_DISP_VIDWIN_ZORDER_MOVE_BACKWARD;
            break;
       default:
            printf("Wrong zorder type %ld\n", *pu32ZOrder);
            return FALSE;
    }

    CHECK_RETURN(MI_DISP_SetZOrder(_hMainVidWin, eZOrder));

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_GetZOrder(void)
{
    MI_U32 u32Layer = 0;

    CHECK_RETURN(MI_DISP_GetZOrder(_hMainVidWin, &u32Layer));
    printf("ZOrder layer: %ld\n", u32Layer);

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetOutputPath(MI_U32 *pu32OutputPath, MI_U32 *pu32Enable)
{
    switch(*pu32OutputPath)
    {
        case 0:
            CHECK_RETURN(MI_DISP_SetOutputPath(E_MI_DISP_OUTPUT_PATH_CVBS, *pu32Enable));
            break;
        case 1:
            CHECK_RETURN(MI_DISP_SetOutputPath(E_MI_DISP_OUTPUT_PATH_SVIDEO, *pu32Enable));
            break;
        case 2:
            CHECK_RETURN(MI_DISP_SetOutputPath(E_MI_DISP_OUTPUT_PATH_SCART_RGB, *pu32Enable));
            break;
        case 3:
            CHECK_RETURN(MI_DISP_SetOutputPath(E_MI_DISP_OUTPUT_PATH_YPBPR, *pu32Enable));
            break;
        case 4:
            CHECK_RETURN(MI_DISP_SetOutputPath(E_MI_DISP_OUTPUT_PATH_HDMI, *pu32Enable));
            break;
        case 5:
            CHECK_RETURN(MI_DISP_SetOutputPath(E_MI_DISP_OUTPUT_PATH_CVBS | E_MI_DISP_OUTPUT_PATH_YPBPR | E_MI_DISP_OUTPUT_PATH_HDMI, *pu32Enable));
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetAllOutputPath(MI_U32 *pu32Enable)
{
    CHECK_RETURN(MI_DISP_SetAllOutputPath(*pu32Enable));

    return TRUE;
}

MI_BOOL MI_DEMO_DISP_SetOutputHdcp(MI_U32 *pu32Enable)
{
    CHECK_RETURN(MI_DISP_SetOutputHdcp(*pu32Enable));

    return TRUE;
}
