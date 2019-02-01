//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2015 MStar Semiconductor, Inc. All rights reserved.
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

#ifndef _VSYNC_BRIDGE_
#define _VSYNC_BRIDGE_

#include "MsFrmFormat.h"

typedef enum {
    VSYNC_BRIDGE_INIT,
    VSYNC_BRIDGE_UPDATE,
} Vsync_Bridge_Mode;

typedef enum {
    VSYNC_BRIDGE_NONE,
    VSYNC_BRIDGE_ON,
    VSYNC_BRIDGE_OFF,
} Vsync_Bridge_SWITCH_TYPE;

typedef enum {
    VSYNC_BRIDGE_GET_DISPQ_SHM_ADDR,
    VSYNC_BRIDGE_GET_DS_BUF_ADDR,
    VSYNC_BRIDGE_GET_DS_BUF_SIZE,
    VSYNC_BRIDGE_GET_DS_XC_ADDR,
    VSYNC_BRIDGE_GET_MIU_INTERVAL,
    VSYNC_BRIDGE_GET_FRAME_BUF_ADDR,
    VSYNC_BRIDGE_GET_SUB_FRAME_BUF_ADDR,
    VSYNC_BRIDGE_GET_DS_BUFFER_DEPTH,
    VSYNC_BRIDGE_GET_DS_SUB_BUF_ADDR,
    VSYNC_BRIDGE_GET_SUB_DISPQ_SHM_ADDR,
    VSYNC_BRIDGE_GET_SUB_DS_XC_ADDR,
} Vsync_Bridge_Get_Cmd;

// Define aspect ratio type
typedef enum {
    /// Default
    E_AR_DEFAULT = 0,
    /// 16x9
    E_AR_16x9,
    /// 4x3
    E_AR_4x3,
    /// Auto
    E_AR_AUTO,
    /// Panorama
    E_AR_Panorama,
    /// Just Scan
    E_AR_JustScan,
    /// Zoom 1
    E_AR_Zoom1,
    /// Zoom 2
    E_AR_Zoom2,
    E_AR_14x9,
    /// point to point
    E_AR_DotByDot,
    /// Subtitle
    E_AR_Subtitle,
    /// movie
    E_AR_Movie,
    /// Personal
    E_AR_Personal,

    /// Maximum value of this enum
    E_AR_MAX,
} Vsync_Bridge_ARC_Type;

typedef enum {
    E_FRC_NORMAL = 0,
    E_FRC_32PULLDOWN,               //3:2 pulldown mode (ex. 24p a 60i or 60p)
    E_FRC_PAL2NTSC ,                //PALaNTSC conversion (50i a 60i)
    E_FRC_NTSC2PAL,                 //NTSCaPAL conversion (60i a 50i)
    E_FRC_DISP_2X,                  //output rate is twice of input rate (ex. 30p a 60p)
    E_FRC_24_50,                    //output rate 24P -> 50P 48I -> 50I
    E_FRC_50P_60P,                  //output rate 50P -> 60P
    E_FRC_60P_50P,                  //output rate 60P -> 50P
    E_FRC_HALF_I,                   //output rate 120i -> 60i, 100i -> 50i
    E_FRC_120I_50I,                 //output rate 120i -> 60i
    E_FRC_100I_60I,                 //output rate 100i -> 60i
    E_FRC_DISP_4X,                  //output rate is four times of input rate (ex. 15P a 60P)
    E_FRC_15_50,                    //output rate 15P -> 50P
    E_FRC_30_50,                    //output rate 30i -> 50i
    E_FRC_30_24,                    //output rate 30p -> 24p, 60i -> 24i
    E_FRC_60_24,                    //output rate 60p -> 24p, 120i -> 24i
    E_FRC_60_25,                    //output rate 60p -> 25p , 120i -> 50i
    E_FRC_HALF_P,                   //output rate 60p -> 30p, 50p -> 25p
    E_FRC_25_30,                    //output rate 25p -> 30p , 50i -> 30i
    E_FRC_50_30,                    //output rate 25p -> 30p , 100i -> 30i
    E_FRC_24_30,                    //output rate 24p -> 30p , 48i -> 30i
    E_FRC_50_24,
} Vsync_Bridge_FRC_Mode;

typedef enum {
    E_FRC_DROP_FRAME = 0,
    E_FRC_DROP_FIELD = 1,
} Vsync_Bridge_FRC_Drop_Mode;

typedef struct {
    MS_U16 u16CropX;
    MS_U16 u16CropY;
    MS_U16 u16CropWidth;
    MS_U16 u16CropHeight;
    MS_U16 u16DispX;
    MS_U16 u16DispY;
    MS_U16 u16DispWidth;
    MS_U16 u16DispHeight;
    MS_U8 u8Win;
    MS_U8 u8SetFromExt;
}stVBDispCrop;

#define SUPPORT_4KDS_CHIP    !(defined(MSTAR_NAPOLI) || defined(MSTAR_MADISON) || defined(MSTAR_NIKE) || defined(MSTAR_KAISER) || defined(MSTAR_CLIPPERS))
#define SUPPORT_10BIT_CHIP  !(defined(MSTAR_MADISON) || defined(MSTAR_NIKE) || defined(MSTAR_KAISER))
#define SUPPORT_MVOP_BW_SAVING_CHIP !(defined(MSTAR_MADISON) || defined(MSTAR_NIKE) || defined(MSTAR_KAISER))
#define SUPPORT_FRAME_CONVERT_CHIP  !(defined(MSTAR_NAPOLI) || defined(MSTAR_MADISON) || defined(MSTAR_NIKE) || defined(MSTAR_KAISER) || defined(MSTAR_MONACO))
#define SUPPORT_INTERLACEOUT_CHIP   (defined(BUILD_FOR_STB) && defined(MSTAR_NAPOLI))
#define SUPPORT_HW_AUTO_GEN_BLACK_CHIP   !(defined(MSTAR_CLIPPERS) || defined(MSTAR_MESSI) || defined(MSTAR_KANO))

#define ENABLE_DYNAMIC_SCALING
//#define ENABLE_SUB_DYNAMIC_SCALING
#ifdef ENABLE_DYNAMIC_SCALING
#define ENABLE_INTERLACE_DS
#endif

#if SUPPORT_4KDS_CHIP
#define DS_MAX_WIDTH    4096
#define DS_MAX_HEIGHT   2176
#else
#define DS_MAX_WIDTH    1920
#define DS_MAX_HEIGHT   1088
#endif

#define DS_BUFFER_DEPTH     16
#define DS_BUFFER_DEPTH_EX  32

typedef int (*FrameDoneCallBack)(void*, int);

#ifdef __cplusplus
extern "C" {
#endif

    void vsync_bridge_init(void);
    void vsync_bridge_open(MS_DispFrameFormat *dff, int panelW, int panelH);
    void vsync_bridge_close(unsigned char Overlay_ID, MS_DispFrameFormat *dff);
    void vsync_bridge_render_frame(MS_DispFrameFormat *dff, Vsync_Bridge_Mode eMode);
    MS_BOOL vsync_bridge_wait_init_done(MS_DispFrameFormat *dff);
    void vsync_bridge_wait_frame_done(MS_DispFrameFormat *dff);
    int vsync_bridge_is_open(MS_DispFrameFormat *dff);
    int vsync_bridge_ds_init(MS_DispFrameFormat *dff);
    void vsync_bridge_ds_deinit(unsigned char Overlay_ID);
    void vsync_bridge_set_dispinfo(unsigned char Overlay_ID, MS_WINDOW_RATIO srcCropRatio, const MS_WinInfo *pVinfo);
    void vsync_bridge_ds_enable(unsigned char u8Enable, unsigned char Overlay_ID);
    MS_U64 vsync_bridge_get_cmd(Vsync_Bridge_Get_Cmd eCmd);
    MS_U32 vsync_bridge_set_aspectratio(unsigned char Overlay_ID, Vsync_Bridge_ARC_Type enARCType);
    void vsync_bridge_ds_set_ext_DipCropWin(stVBDispCrop* ptr);
    void vsync_bridge_CalAspectRatio(MS_WINDOW *pDispWin, MS_WINDOW *pCropWin, MS_DispFrameFormat *dff, MS_BOOL bCalAspect, MS_BOOL bUseDefaultArc, int panel_width, int panel_height);
    int vsync_bridge_capture_video(unsigned char u8Mode, unsigned int OverlayID, int left, int top, int *width, int *height, int *crop_left, int *crop_top, int *crop_right, int *crop_bottom, void * data, int stride);
    void vsync_bridge_wait_disp_show(MS_DispFrameFormat *dff);
    int vsync_bridge_adjust_crop(MS_DispFrameFormat *dff, MS_WINDOW *crop, int width, int height);
    int vsync_bridge_adjust_win(MS_DispFrameFormat *dff, MS_WINDOW *pDispWin, float fRatio);
    void vsync_bridge_set_mvop(MS_DispFrameFormat *dff);
    MS_BOOL vsync_bridge_GetInterlaceOut(void);
    void vsync_bridge_SetInterlaceOut(MS_BOOL bIsInterlaceOut);
    MS_BOOL vsync_bridge_CheckInterlaceOut(MS_WINDOW *pDispWin);
    void vsync_bridge_SetSavingBandwidthMode(unsigned char bSaveMode, unsigned char Overlay_ID);
    MS_U32 vsync_bridge_ds_update_table(unsigned char Overlay_ID);
    void vsync_bridge_signal_abort_waiting(unsigned char Overlay_ID);
    void vsync_bridge_SetBlackScreenFlag(unsigned char Overlay_ID, MS_BOOL bFlag);
    void vsync_bridge_update_status(unsigned char Overlay_ID);
    MS_FreezeMode vsync_bridge_GetLastFrameFreezeMode(unsigned char Overlay_ID);
    void vsync_bridge_set_frame_done_callback(unsigned char Overlay_ID, void *pHandle, FrameDoneCallBack callback);
    void vsync_bridge_set_fdmask(MS_DispFrameFormat *dff, MS_BOOL bEnable, MS_BOOL bLockMutex);
    void vsync_bridge_update_output_timing(unsigned char Overlay_ID, Vsync_Bridge_Mode eMode);
    void vsync_bridge_CalAspectRatio_WithTimingChange(unsigned char Overlay_ID, MS_WINDOW *pDispWin, MS_WINDOW *pCropWin, MS_BOOL bCalAspect, MS_BOOL bUseDefaultArc, int panel_width, int panel_height);
    MS_BOOL vsync_bridge_is_hw_auto_gen_black(void);
    void vsync_bridge_SetBlankVideo(unsigned char Overlay_ID,unsigned char Count);
#ifdef __cplusplus
}
#endif

#endif // _VSYNC_BRIDGE_

