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
#ifndef DISP_API_H
#define DISP_API_H

#include <MsTypesEx.h>
#include "MsFrmFormat.h"

/// the Window ID
typedef enum
{
    /// Main MVOP, Main XC window
    MS_MAIN_WINDOW = 0,
    /// Sub MVOP, Sub XC window
    MS_SUB_WINDOW  = 1,
    /// DIP0
    MS_DIP_0  = 2,
    /// DIP1
    MS_DIP_1  = 3,
    /// DIP2
    MS_DIP_2  = 4,
    /// DIP3
    MS_DIP_3  = 5,
    /// DIP3
    MS_DIP_4  = 6,
    /// DIP3
    MS_DIP_5  = 7,
    /// DIP3
    MS_DIP_6  = 8,
    /// Maximum window number
    MS_MAX_WINDOW
} MS_Win_ID;

/// the waiting type
typedef enum
{
    /// wait frame show finish
    MS_WAIT_FRAME_DONE = 0,
    /// wait video capture done
    MS_WAIT_CAPTURE_DONE  = 1,
    /// wait display path init done
    MS_WAIT_INIT_DONE = 2,
    /// maximum waiting type
    MS_WAIT_MAX
} MS_Wait_Mode;

/// the display path command type
typedef enum
{
    /// saving BW mode
    MS_DISP_CMD_SAVING_BW_MODE = 0,
    /// update status of FRC mode, FBL mode etc...
    MS_DISP_CMD_UPDATE_DISP_STATUS,
    /// mute video at rolling mode or close rolling mode
    MS_DISP_CMD_CONTROL_ROLLING_MODE,
    /// setup the frame done callback
    MS_DISP_CMD_SET_FRAME_DONE_CALLBACK,
    /// get render delay time, unit: ms
    MS_DISP_CMD_GET_RENDER_DELAY_TIME,
    MS_DISP_CMD_MAX
} MS_Cmd_Type;

/// the frame done callback info
typedef struct
{
    /// handle
    void *pHandle;
    /// callback function
    void *callback;
} MS_CallbackInfo;

/// display path state
typedef enum
{
    MS_STATE_CLOSE,
    MS_STATE_RES_WAITING,
    MS_STATE_OPENING,
    MS_STATE_OPENED,
} MS_Disp_State;

#ifdef __cplusplus
extern "C" {
#endif

/**
*   Init disp path module
*
*   @param eWinID               \b IN: The window id
*   @return                     None
*/
void MS_Video_Disp_Init(MS_Win_ID eWinID);

/**
*   DeInit disp path module
*
*   @param eWinID               \b IN: The window id
*   @return                     None
*/
void MS_Video_Disp_DeInit(MS_Win_ID eWinID);


/**
*   Display path open
*   give the window and video information to setup MVOP/XC
*   @param eWinID               \b IN: The window id
*   @param stWinInfo            \b IN: The window information
*   @param dff                  \b IN: The source video information
*   @return                     \b 0: OK
*/
int MS_Video_Disp_Open(MS_Win_ID eWinID, MS_WinInfo stWinInfo, const MS_DispFrameFormat *dff);

/**
*   Reset the display information
*   It will reset XC according the input information.
*   @param eWinID               \b IN: The window id
*   @param stWinInfo            \b IN: The window information
*   @param dff                  \b IN: The source video information
*   @return                     \b 0: OK
*/
int MS_Video_Disp_Resize(MS_Win_ID eWinID, MS_WinInfo stWinInfo, const MS_DispFrameFormat *dff);

/**
*   Display path close
*   close MVOP/XC
*   @param eWinID               \b IN: The window id
*   @return                     \b 0: OK
*/
int MS_Video_Disp_Close(MS_Win_ID eWinID);

/**
*   Flip one frame
*   It will set the display frame info into disp queue, when MVOP vsync
*   interrupt happen, will set it into MVOP reg.
*   @param eWinID               \b IN: The window id
*   @param dff                  \b IN: The source video information
*   @return                     None
*/
void MS_Video_Disp_Flip(MS_Win_ID eWinID, MS_DispFrameFormat *dff);

/**
*   Wait display path for some purpose.
*   @param eWinID               \b IN: The window id
*   @param eMode                \b IN: The waiting type
*   @param dff                  \b IN: The source video information
*   @return                     \b 0: OK
*/
int MS_Video_Disp_Wait(MS_Win_ID eWinID, MS_Wait_Mode eMode, MS_DispFrameFormat *dff);


/**
*   Capture video frame data according to MS_FreezeMode
*   Captue video frame, call with NULL pdata will get video size in width/height/pWIn
*   User allocate buffer by width/height/pWIn then call second time to get video in pdata
*   interrupt happen, will set it into MVOP reg.
*   @param eWinID               \b IN: The window id
*   @param eMode                \b IN: The freeze mode
*   @param *width               \b IN: the capture video width
*   @param *height              \b IN: the capture video height
*   @param *pWin                \b IN: the video crop info
*   @param *pdata               \b IN: output address
*   @return                     \b 0: OK
*/
int MS_Video_Capture(MS_Win_ID eWinID, MS_FreezeMode eMode, int *width, int *height, MS_WINDOW *pWin, void *pdata);

/**
*   Capture video frame data according to MS_FreezeMode
*   Captue video frame, call with NULL pdata will get video size in width/height/pWIn
*   User allocate buffer by width/height/pWIn then call second time to get video in pdata
*   interrupt happen, will set it into MVOP reg.
*   @param eWinID               \b IN: The window id
*   @param eMode                \b IN: The freeze mode
*   @param *width               \b IN: the capture video width
*   @param *height              \b IN: the capture video height
*   @param *pWin                \b IN: the video crop info
*   @param *pdata               \b IN: output address
*   @param stride               \b IN: the align width by mali
*   @return                     \b 0: OK
*/
int MS_Video_Frame_Capture(MS_Win_ID eWinID, MS_FreezeMode eMode, int *width, int *height, MS_WINDOW *pWin, void *pdata, int stride);


/**
*   Display path get state
*   get display state
*   @param eWinID               \b IN: The window id
*   @return                     \b the state of display path
*/
MS_Disp_State MS_Video_Disp_Get_State(MS_Win_ID eWinID);

/**
*   Display path set command
*   set display command
*   @param eWinID               \b IN: The window id
*   @param eCmd                 \b IN: the command type
*   @param pPara                \b IN: the command parameter
*   @return                     \b 0: OK
*/
int MS_Video_Disp_Set_Cmd(MS_Win_ID eWinID, MS_Cmd_Type eCmd, void* pPara);

/**
*   Display path get command
*   get display infomation command
*   @param eWinID               \b IN: The window id
*   @param eCmd                 \b IN: the command type
*   @param pPara                \b OUT: the information
*   @param ParaSize            \b IN: the data size of return value
*   @return                     \b 0: OK
*/
int MS_Video_Disp_Get_Cmd(MS_Win_ID eWinID, MS_Cmd_Type eCmd, void* pPara, MS_U32 ParaSize);

#ifdef __cplusplus
}
#endif

#endif // DISP_API_H
