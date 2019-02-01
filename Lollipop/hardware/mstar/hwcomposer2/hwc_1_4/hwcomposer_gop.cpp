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

/* android header */
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <utils/Errors.h>
#include <assert.h>
#include <stdlib.h>
#include <cutils/properties.h>


/* mstar header */
#include <MsCommon.h>
#include <MsOS.h>

#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <apiXC.h>

#include "hwcomposer_gop.h"
#include "hwcomposer_util.h"

enum {
    GOP0_BIT_MASK = 0x1,
    GOP1_BIT_MASK = 0x2,
    GOP2_BIT_MASK = 0x4,
    GOP3_BIT_MASK = 0x8,
    GOP4_BIT_MASK = 0x10
};

typedef enum {
    E_PANEL_FULLHD=1,
    E_PANEL_4K2K_CSOT,
    E_PANEL_FULLHD_URSA6,
    E_PANEL_4K2K_URSA6,
    E_PANEL_FULLHD_URSA7,
    E_PANEL_4K2K_URSA7,
    E_PANEL_FULLHD_URSA8,
    E_PANEL_4K2K_URSA8,
    E_PANEL_FULLHD_URSA9,
    E_PANEL_4K2K_URSA9,
    E_PANEL_FULLHD_URSA11,
    E_PANEL_4K2K_URSA11,
    E_PANEL_4K2K_120HZ,
    E_PANEL_DEFAULT,
}EN_PANEL_TYPE;

#define ALIGNMENT( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

static MS_U32 _SetFBFmt(MS_U16 pitch, MS_PHY addr, MS_U16 fmt) {
    // Set FB info to GE (For GOP callback fucntion use)
    return 1;
}

static MS_BOOL _XC_IsInterlace(void) {
    return 0;
}

static void _SetPQReduceBWForOSD(MS_U8 PqWin, MS_BOOL enable) {
}

static MS_U16 _XC_GetCapwinHStr(void) {
    return 0x80;
}

int GOP_setStretchWindow(unsigned char gop,  unsigned short srcWidth, unsigned short srcHeight,
                          unsigned short dstX, unsigned short dstY, unsigned short dstWidth, unsigned short dstHeight) {
    MS_U8 gopNum = MApi_GOP_GWIN_GetMaxGOPNum();
    if (gop >= gopNum) {
        ALOGE("GOP_SetStretchWindow was invoked invalid gop=%d",gop);
        return -1;
    }
    /* GOP stretch setting */
    MApi_GOP_GWIN_Set_HSCALE_EX(gop,TRUE, srcWidth, dstWidth);
    MApi_GOP_GWIN_Set_VSCALE_EX(gop,TRUE, srcHeight, dstHeight);

    // set the stretchwindow scale mode
    MApi_GOP_GWIN_Set_VStretchMode_EX(gop, E_GOP_VSTRCH_LINEAR_GAIN2);
    MApi_GOP_GWIN_Set_HStretchMode_EX(gop, E_GOP_HSTRCH_6TAPE_LINEAR);

    MApi_GOP_GWIN_Set_STRETCHWIN(gop, E_GOP_DST_OP0, dstX, dstY, srcWidth, srcHeight);
    return 0;
}

int GOP_getAvailGwinId(int gopNo) {
    if (gopNo < 0) {
        ALOGE("getAvailGwinNum_ByGop the given gopNo is %d,check it!",gopNo);
        return -1;
    }
    int availGwinNum = 0;
    int i = 0;
    for (i; i < gopNo; i++) {
        availGwinNum += MApi_GOP_GWIN_GetGwinNum(i);
    }
    return availGwinNum;
}

#ifdef MALI_AFBC_GRALLOC
void GOP_AFBCReset(HWWindowInfo_t &hwwindowinfo) {
    MS_BOOL plist = TRUE;
    MApi_GOP_SetConfigEx(hwwindowinfo.gop,E_GOP_AFBC_RESET,&plist);
}
#endif

static void GOP_reCreateFBInfo(int osdMode,HWWindowInfo_t &hwwindowinfo,HWWindowInfo_t &lasthwwindowinfo) {
    char property[PROPERTY_VALUE_MAX] = {0};
    int bResizeBufferEnable = 0;
    if (property_get("mstar.resize.framebuffer", property, "0") > 0) {
        bResizeBufferEnable = atoi(property);
    }
    int pitch = hwwindowinfo.stride;
    MS_U32 U32MainFbid = 0;
    MS_U32 U32SubFbid = 0;
    EN_GOP_3D_MODETYPE mode = E_GOP_3D_DISABLE;
    hwwindowinfo.mainFBphysicalAddr = hwwindowinfo.physicalAddr + hwwindowinfo.srcY * pitch +
                             hwwindowinfo.srcX * HAL_PIXEL_FORMAT_byteperpixel(hwwindowinfo.format);
    EN_GOP_FRAMEBUFFER_STRING FBString = E_GOP_FB_NULL;
#ifdef MALI_AFBC_GRALLOC
    if (hwwindowinfo.bSupportGopAFBC) {
        if (hwwindowinfo.bEnableGopAFBC) {
            //enable AFBC
            FBString = E_GOP_FB_AFBC_NONSPLT_YUVTRS_ARGB8888;
        } else {
            //disable AFBC
            FBString = E_GOP_FB_NULL;
        }
    }
    ALOGD("GOP[%d]:AFBC MODE FBString = %d",hwwindowinfo.gop,FBString);
#endif
    if (bResizeBufferEnable) {
        U32MainFbid = MApi_GOP_GWIN_GetFree32FBID();
    }
    switch (osdMode) {
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_BOTTOM_LA:
            {
                GOP_GwinFBAttr fbInfo;
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                    hwwindowinfo.bufferHeight>>1,
                                                    HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                    hwwindowinfo.mainFBphysicalAddr,
                                                    pitch,
                                                    &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight>>1,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                MApi_GOP_GWIN_GetFBInfo(U32MainFbid,&fbInfo);
                MS_PHY fb2_addr = fbInfo.addr + fbInfo.pitch * fbInfo.height;
                hwwindowinfo.subFBphysicalAddr = fb2_addr;
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight>>1,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        fb2_addr,
                                                        pitch,
                                                        &U32SubFbid);
                } else {
                    U32SubFbid = MApi_GOP_GWIN_GetFree32FBID();
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32SubFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight>>1,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        fb2_addr,
                                                        FBString);
                }
                mode = E_GOP_3D_LINE_ALTERNATIVE;
            } break;
        case DISPLAYMODE_LEFTRIGHT_FR:
            {
                MS_U32 u32WordUnit=0;
                GOP_GwinFBAttr fbInfo;
                E_GOP_API_Result result=GOP_API_SUCCESS;
                //For SBS->FR, re-create FB as H-Half frame size
                result = MApi_GOP_GetChipCaps(E_GOP_CAP_WORD_UNIT,&u32WordUnit,sizeof(MS_U32));
                if ((GOP_API_SUCCESS==result)&&(u32WordUnit != 0)) {
                    MS_U32 u32FbId = MAX_GWIN_FB_SUPPORT;
                    MS_U8 u8BytePerPixel = 4;
                    //address to half line end
                    MS_U32 u32SubAddrOffset = (hwwindowinfo.srcWidth>>1)*u8BytePerPixel;
                    if ((u32SubAddrOffset%u32WordUnit) != 0) {
                        ALOGE("Attention! Wrong H resolution[%u] for 3D output, fmt=%u\n", hwwindowinfo.bufferWidth,
                              HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format));
                        return; //Wrong res, must obey GOP address alignment rule(1 Word)
                    }
                    if (!bResizeBufferEnable) {
                        MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth>>1,
                                                            hwwindowinfo.bufferHeight,
                                                            HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                            hwwindowinfo.mainFBphysicalAddr,
                                                            pitch,
                                                            &U32MainFbid);
                    } else {
                        MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth>>1,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                    }
                    MApi_GOP_GWIN_GetFBInfo(U32MainFbid,&fbInfo);
                    MS_PHY fb2_addr = fbInfo.addr + u32SubAddrOffset;
                    hwwindowinfo.subFBphysicalAddr = fb2_addr;
                    if (!bResizeBufferEnable) {
                        MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth>>1,
                                                            hwwindowinfo.bufferHeight,
                                                            HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                            fb2_addr,
                                                            pitch,
                                                            &U32SubFbid);
                    } else {
                        U32SubFbid = MApi_GOP_GWIN_GetFree32FBID();
                        MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32SubFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth>>1,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        fb2_addr,
                                                        FBString);
                    }
                    mode = E_GOP_3D_SWITH_BY_FRAME;
                }
            } break;
        case DISPLAYMODE_NORMAL_FR:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                U32SubFbid = U32MainFbid;
                hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                mode = E_GOP_3D_SWITH_BY_FRAME;
            } break;
        case DISPLAYMODE_NORMAL_FP:
            {
                if ((hwwindowinfo.srcWidth == 1920 && hwwindowinfo.srcHeight == 1080) ||
                    (hwwindowinfo.srcWidth == 1280 && hwwindowinfo.srcHeight == 720)) {
                    if (!bResizeBufferEnable) {
                        MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                            hwwindowinfo.bufferHeight,
                                                            HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                            hwwindowinfo.mainFBphysicalAddr,
                                                            pitch,
                                                            &U32MainFbid);
                    } else {
                        MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                    }
                    U32SubFbid = U32MainFbid;
                    hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                    mode = E_GOP_3D_FRAMEPACKING;
                } else {
                    if (lasthwwindowinfo.fbid != INVALID_FBID) {
                        hwwindowinfo.fbid =lasthwwindowinfo.fbid;
                    }
                    if (lasthwwindowinfo.subfbid != INVALID_FBID) {
                        hwwindowinfo.subfbid =lasthwwindowinfo.subfbid;
                    }
                    return;
                }
            } break;
        case DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_TOP_BOTTOM;
            } break;
        case DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_LINE_ALTERNATIVE;
            } break;
        case DISPLAYMODE_LEFTRIGHT_FULL:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_SIDE_BY_SYDE;
            } break;
        case DISPLAYMODE_NORMAL:
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        case DISPLAYMODE_RIGHT_ONLY:
        default:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_DISABLE;
            } break;
        case DISPLAYMODE_FRAME_ALTERNATIVE_LLRR:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight>>1,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo.bufferWidth,
                                                        hwwindowinfo.bufferHeight>>1,
                                                        HAL_PIXEL_FORMAT_to_MS_ColorFormat(hwwindowinfo.format),
                                                        hwwindowinfo.mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_DISABLE;
            } break;
    }
    hwwindowinfo.fbid = U32MainFbid;
    hwwindowinfo.subfbid = U32SubFbid;
    int status= 1;
    status = MApi_GOP_GWIN_Map32FB2Win(hwwindowinfo.fbid, hwwindowinfo.gwin);
    if (status != GOP_API_SUCCESS) {
        ALOGD("MApi_GOP_GWIN_Map32FB2Win failed gop %d gwin %d",hwwindowinfo.gop,hwwindowinfo.gwin);
    }

    if (lasthwwindowinfo.subfbid != INVALID_FBID) {
        int status = MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo.subfbid);
        if (status != GOP_API_SUCCESS) {
            ALOGD("MApi_GOP_GWIN_Delete32FB sub failed");
        }
        if (lasthwwindowinfo.subfbid == lasthwwindowinfo.fbid) {
            lasthwwindowinfo.subfbid = INVALID_FBID;
            lasthwwindowinfo.fbid = INVALID_FBID;
        } else {
            lasthwwindowinfo.subfbid = INVALID_FBID;
        }
    }

    if (lasthwwindowinfo.fbid != INVALID_FBID) {
        int status = MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo.fbid);
        if (status != GOP_API_SUCCESS) {
            ALOGD("MApi_GOP_GWIN_Delete32FB failed");
        }
        lasthwwindowinfo.fbid = INVALID_FBID;
    }
    MApi_GOP_Set3DOSDMode(hwwindowinfo.gwin, hwwindowinfo.fbid, hwwindowinfo.subfbid,mode);
}

static void GOP_updateGWinInfo(int osdMode,HWWindowInfo_t &hwwindowinfo,HWWindowInfo_t &lasthwwindowinfo) {
    static int old3DMode = DISPLAYMODE_NORMAL;
    int pitch = hwwindowinfo.stride;
    MS_U16 srcWidth = hwwindowinfo.srcWidth;
    MS_U16 srcHeight = hwwindowinfo.srcHeight;
    MS_U16 dstX = hwwindowinfo.dstX;
    MS_U16 dstY = hwwindowinfo.dstY;
    MS_U16 dstWidth = hwwindowinfo.dstWidth;
    MS_U16 dstHeight = hwwindowinfo.dstHeight;
    MS_PHY fb2_addr = 0;
    GOP_GwinInfo pinfo;
    GOP_GwinFBAttr fbInfo;
    static MS_BOOL mirrorModeH, mirrorModeV;
    static int enableHwcursor = 0;
    memset(&pinfo,0,sizeof(pinfo));
    memset(&fbInfo,0,sizeof(fbInfo));
    MApi_GOP_GWIN_GetWinInfo(hwwindowinfo.gwin, &pinfo);
    pinfo.u16DispVPixelStart = 0;
    pinfo.u16DispVPixelEnd = hwwindowinfo.srcHeight;
    pinfo.u16DispHPixelStart = 0;
    // the gwin width must be 2 pixel alignment
    pinfo.u16DispHPixelEnd = hwwindowinfo.srcWidth &(~0x1);
    pinfo.u32DRAMRBlkStart = hwwindowinfo.physicalAddr + hwwindowinfo.srcY * pitch +
                             hwwindowinfo.srcX * HAL_PIXEL_FORMAT_byteperpixel(hwwindowinfo.format);
    hwwindowinfo.mainFBphysicalAddr = pinfo.u32DRAMRBlkStart;
    switch (osdMode) {
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_BOTTOM_LA:
            pinfo.u16DispHPixelEnd = hwwindowinfo.srcWidth;
            pinfo.u16DispVPixelEnd = hwwindowinfo.srcHeight>>1;
            srcHeight = hwwindowinfo.srcHeight>>1;
            dstHeight = hwwindowinfo.dstHeight>>1;
            MApi_GOP_GWIN_Get32FBInfo(hwwindowinfo.fbid,&fbInfo);
            fb2_addr = hwwindowinfo.mainFBphysicalAddr + fbInfo.pitch * fbInfo.height;
            hwwindowinfo.subFBphysicalAddr = fb2_addr;
            break;
        case DISPLAYMODE_LEFTRIGHT_FR:
            pinfo.u16DispHPixelEnd = hwwindowinfo.srcWidth>>1;
            pinfo.u16DispVPixelEnd = hwwindowinfo.srcHeight;
            srcWidth = hwwindowinfo.srcWidth>>1;
            fb2_addr = hwwindowinfo.mainFBphysicalAddr + srcWidth*HAL_PIXEL_FORMAT_byteperpixel(hwwindowinfo.format);;
            hwwindowinfo.subFBphysicalAddr = fb2_addr;
            break;
        case DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY:
            dstHeight = hwwindowinfo.dstHeight>>1;
            hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
            break;
        case DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY:
            dstHeight = hwwindowinfo.dstHeight>>1;
            hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
            break;
        case DISPLAYMODE_LEFTRIGHT_FULL:
            dstWidth = hwwindowinfo.dstWidth>>1;
            hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
            break;
        case DISPLAYMODE_NORMAL_FP:
            if ((hwwindowinfo.srcWidth==1920)&&(hwwindowinfo.srcHeight==1080)) {
                dstWidth = 1920;
                dstHeight = 1080;
            } else if ((hwwindowinfo.srcWidth==1280)&&(hwwindowinfo.srcHeight==720)) {
                //only support 1920*1080 framepacket, if 720p framebuffer, scale to 1080p
                //through stretchwindow
                dstWidth = 1920;
                dstHeight = 1080;
            }
            hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
            break;
        case DISPLAYMODE_FRAME_ALTERNATIVE_LLRR:
            srcHeight = hwwindowinfo.srcHeight>>1;
            hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
            break;
        case DISPLAYMODE_NORMAL:
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        case DISPLAYMODE_RIGHT_ONLY:
        case DISPLAYMODE_NORMAL_FR:
        default:
            hwwindowinfo.subFBphysicalAddr = hwwindowinfo.mainFBphysicalAddr;
            break;
    }
    if (lasthwwindowinfo.srcWidth != hwwindowinfo.srcWidth ||
        lasthwwindowinfo.srcHeight != hwwindowinfo.srcHeight ||
        lasthwwindowinfo.dstX != hwwindowinfo.dstX ||
        lasthwwindowinfo.dstY != hwwindowinfo.dstY ||
        lasthwwindowinfo.dstWidth != hwwindowinfo.dstWidth ||
        lasthwwindowinfo.dstHeight != hwwindowinfo.dstHeight ||
        old3DMode != osdMode) {
        GOP_setStretchWindow(hwwindowinfo.gop,srcWidth,srcHeight,dstX, dstY,dstWidth,dstHeight);
        //in DISPLAYMODE_FRAME_ALTERNATIVE_LLRR mode, video Vertical Mirroreyd, so does OSD.
        //and hwcursor should be disabled
        if ((osdMode == DISPLAYMODE_FRAME_ALTERNATIVE_LLRR) && (old3DMode != osdMode)) {
            char property[PROPERTY_VALUE_MAX] = {0};
            MApi_GOP_GWIN_IsMirrorOn_EX(hwwindowinfo.gop, &mirrorModeH, &mirrorModeV);
            MApi_GOP_GWIN_SetVMirror_EX(hwwindowinfo.gop, true);
            if (property_get("mstar.desk-enable-hwcursor", property, "1") > 0) {
                enableHwcursor = atoi(property);
            }
            property_set("mstar.desk-enable-hwcursor", "0");
        } else if ((old3DMode == DISPLAYMODE_FRAME_ALTERNATIVE_LLRR) && (old3DMode != osdMode)) {
            MApi_GOP_GWIN_SetVMirror_EX(hwwindowinfo.gop, mirrorModeV);
            if(enableHwcursor) {
                property_set("mstar.desk-enable-hwcursor", "1" );
            }
        }
        old3DMode = osdMode;
    }
    //MApi_GOP_GWIN_SetForceWrite(TRUE);
#ifdef MALI_AFBC_GRALLOC
    if (hwwindowinfo.bEnableGopAFBC) {
        pinfo.u16DispHPixelEnd = ALIGNMENT(pinfo.u16DispHPixelEnd,16);
        pinfo.u16DispVPixelEnd = ALIGNMENT(pinfo.u16DispVPixelEnd,16);
    }
#endif
    MApi_GOP_GWIN_SetWinInfo(hwwindowinfo.gwin, &pinfo);
    //MApi_GOP_GWIN_SetForceWrite(FALSE);
}

unsigned int GOP_getBitMask(unsigned int gop){
    unsigned int bitMask = 0;
    switch (gop) {
        case 0:
            bitMask = GOP0_BIT_MASK;
            break;
        case 1:
            bitMask = GOP1_BIT_MASK;
            break;
        case 2:
            bitMask = GOP2_BIT_MASK;
            break;
        case 3:
            bitMask = GOP3_BIT_MASK;
            break;
        case 4:
            bitMask = GOP4_BIT_MASK;
            break;
    }
    return bitMask;
}

//osd gop init
int GOP_init(hwc_context_t *ctx,HWWindowInfo_t &hwwindowinfo) {
    GOP_InitInfo gopInitInfo;

    Mapi_GOP_GWIN_ResetGOP(hwwindowinfo.gop);

    EN_GOP_IGNOREINIT Initlist = E_GOP_IGNORE_MUX;
    memset(&gopInitInfo, 0, sizeof(GOP_InitInfo));
    gopInitInfo.u16PanelWidth  = ctx->curTimingWidth;
    gopInitInfo.u16PanelHeight = ctx->curTimingHeight;
    gopInitInfo.u16PanelHStr   = ctx->panelHStart;
    gopInitInfo.u32GOPRBAdr = 0;
    gopInitInfo.u32GOPRBLen = 0;
    gopInitInfo.bEnableVsyncIntFlip = TRUE;

    MApi_GOP_SetConfigEx(hwwindowinfo.gop,E_GOP_IGNOREINIT, (MS_U32 *)(&Initlist));
    MApi_GOP_InitByGOP(&gopInitInfo, hwwindowinfo.gop);

    MApi_GOP_RegisterFBFmtCB(_SetFBFmt);
    MApi_GOP_RegisterXCIsInterlaceCB(_XC_IsInterlace);
    MApi_GOP_RegisterXCGetCapHStartCB(_XC_GetCapwinHStr);
    MApi_GOP_RegisterXCReduceBWForOSDCB(_SetPQReduceBWForOSD);

    if (ctx->mirrorModeH == 1) {
        MApi_GOP_GWIN_SetHMirror_EX(hwwindowinfo.gop, true);
        ALOGD("SET THE HMirror!!!!!");
    }
    if (ctx->mirrorModeV == 1) {
        MApi_GOP_GWIN_SetVMirror_EX(hwwindowinfo.gop, true);
        ALOGD("SET THE VMirror!!!!");
    }

    if (ctx->stbTarget && !ctx->bRGBColorSpace) {
        MApi_GOP_GWIN_OutputColor_EX(hwwindowinfo.gop, GOPOUT_YUV);
    }
    MApi_GOP_MIUSel(hwwindowinfo.gop, (EN_GOP_SEL_TYPE)hwwindowinfo.channel);
    static bool layerHasSet = false;
    if (!layerHasSet) {
        GOP_LayerConfig stLayerSetting = ctx->stLayerSetting;
        E_GOP_API_Result enResult = MApi_GOP_GWIN_SetLayer(&stLayerSetting, sizeof(GOP_LayerConfig));
        if (enResult == GOP_API_SUCCESS) {
            layerHasSet = true;
            ALOGD("MApi_GOP_GWIN_SetLayer  success %s(%d)\n",__FUNCTION__,__LINE__);
        } else {
            ALOGD("MApi_GOP_GWIN_SetLayer fail, return %u!!! %s(%d)\n", enResult,__FUNCTION__,__LINE__);
        }
    }
    MApi_GOP_GWIN_SetGOPDst(hwwindowinfo.gop, (EN_GOP_DST_TYPE)hwwindowinfo.gop_dest);
    return 0;
}

//cursor gop init
int GOP_init(hwc_context_t *ctx, HwCursorInfo_t &hwcursorinfo) {
    GOP_InitInfo gopInitInfo;
    char property[PROPERTY_VALUE_MAX] = {0};

    Mapi_GOP_GWIN_ResetGOP(hwcursorinfo.gop);

    EN_GOP_IGNOREINIT Initlist = E_GOP_IGNORE_MUX;
    memset(&gopInitInfo, 0, sizeof(GOP_InitInfo));
    gopInitInfo.u16PanelWidth  = ctx->curTimingWidth;
    gopInitInfo.u16PanelHeight = ctx->curTimingHeight;
    gopInitInfo.u16PanelHStr   = ctx->panelHStart;
    gopInitInfo.u32GOPRBAdr = 0;
    gopInitInfo.u32GOPRBLen = 0;
    gopInitInfo.bEnableVsyncIntFlip = TRUE;

    MApi_GOP_SetConfigEx(hwcursorinfo.gop,E_GOP_IGNOREINIT, (MS_U32 *)(&Initlist));
    MApi_GOP_InitByGOP(&gopInitInfo, hwcursorinfo.gop);

    MApi_GOP_RegisterFBFmtCB(_SetFBFmt);
    MApi_GOP_RegisterXCIsInterlaceCB(_XC_IsInterlace);
    MApi_GOP_RegisterXCGetCapHStartCB(_XC_GetCapwinHStr);
    MApi_GOP_RegisterXCReduceBWForOSDCB(_SetPQReduceBWForOSD);

    if (ctx->mirrorModeH == 1) {
        MApi_GOP_GWIN_SetHMirror_EX(hwcursorinfo.gop, true);
        ALOGD("SET THE HMirror!!!!!");
    }
    if (ctx->mirrorModeV == 1) {
        MApi_GOP_GWIN_SetVMirror_EX(hwcursorinfo.gop, true);
        ALOGD("SET THE VMirror!!!!");
    }

    if (ctx->stbTarget && !ctx->bRGBColorSpace) {
        MApi_GOP_GWIN_OutputColor_EX(hwcursorinfo.gop, GOPOUT_YUV);
    }
    MApi_GOP_MIUSel(hwcursorinfo.gop, (EN_GOP_SEL_TYPE)hwcursorinfo.channel);
    MApi_GOP_GWIN_SetGOPDst(hwcursorinfo.gop, (EN_GOP_DST_TYPE)hwcursorinfo.gop_dest);

    if (property_get("mstar.gwin.alignment", property, "2") > 0) {
        hwcursorinfo.gwinPosAlignment = atoi(property);
    }
    ALOGD("hwcursorinfo.gwinPosAlignment %d",hwcursorinfo.gwinPosAlignment);
    MApi_GOP_GWIN_EnableTransClr_EX(hwcursorinfo.gop, GOPTRANSCLR_FMT0, false);
    MApi_GOP_GWIN_SetBlending(hwcursorinfo.gwin, TRUE, 0x3F);

    return 0;
}

void Gop_close_boot_logo(hwc_context_t *ctx) {
    char property[PROPERTY_VALUE_MAX] = {0};
    if (!ctx->bGopHasInit[0]) return;
    if ((!ctx->bootlogo.bGopClosed)&&(property_get("mstar.close.bootlogo.gop.frame", property, NULL) > 0)) {
        ctx->bootlogo.closeThreshold = atoi(property);
        ctx->bootlogo.frameCountAfterBoot++;
        if (ctx->bootlogo.frameCountAfterBoot >= ctx->bootlogo.closeThreshold) {
            unsigned int gwin = GOP_getAvailGwinId(ctx->bootlogo.gop);
            ALOGD("bootlogogop = %d ,gwin = %d, request frame = %d, current frame = %d",ctx->bootlogo.gop,gwin,ctx->bootlogo.closeThreshold,ctx->bootlogo.frameCountAfterBoot);
            if (MApi_GOP_GWIN_IsGWINEnabled(gwin) && gwin != ctx->HWWindowInfo[0].gwin) {
             MApi_GOP_GWIN_UpdateRegOnceByMask(GOP_getBitMask(ctx->bootlogo.gop),TRUE,FALSE);
             MApi_GOP_GWIN_Enable(gwin, false);
             MApi_GOP_GWIN_UpdateRegOnceByMask(GOP_getBitMask(ctx->bootlogo.gop),FALSE,FALSE);
             ALOGD("close bootlogo gop sucess!!!!!!");
             ctx->bootlogo.frameCountAfterBoot = 0;
             ctx->bootlogo.bGopClosed = true;
            } else {
             ALOGD("bootlogo gop has closed by other process or using same gwin with An!!!!!!");
             ctx->bootlogo.frameCountAfterBoot = 0;
             ctx->bootlogo.bGopClosed = true;
            }
        }
     }
}

unsigned int GOP_GetLayerFromGOP(unsigned int gop) {

    GOP_LayerConfig stGOPLayerConfig;
    unsigned int u32Layer = LAYER_ID_INVALID;
    unsigned int u32Index;
    memset(&stGOPLayerConfig, 0, sizeof(GOP_LayerConfig));

    MApi_GOP_GWIN_GetLayer(&stGOPLayerConfig, sizeof(GOP_LayerConfig));
    for (u32Index = 0; u32Index < stGOPLayerConfig.u32LayerCounts; u32Index++) {
        if (stGOPLayerConfig.stGopLayer[u32Index].u32GopIndex == gop) {
            u32Layer = stGOPLayerConfig.stGopLayer[u32Index].u32LayerIndex;
            break;
        }
    }
    return u32Layer;
}
void GOP_display(hwc_context_t *ctx, HWWindowInfo_t (&HWWindowInfo)[MAX_HWWINDOW_COUNT])
{
    int bitMask = 0;
    int gop = 0;
    int goptimeout = 0;

    char property[PROPERTY_VALUE_MAX] = {0};
    for (int j=0; j<ctx->gopCount; j++) {
        HWWindowInfo_t &hwwindowinfo = HWWindowInfo[j];
        if (hwwindowinfo.layerIndex >= 0) {
            if (!ctx->bGopHasInit[j]) {
                GOP_init(ctx,hwwindowinfo);
                ctx->bGopHasInit[j] = true;
            }
        }
    }

    if (!ctx->bGopHasInit[0]) return;

    for (int j=0; j<ctx->gopCount; j++) {
        HWWindowInfo_t &hwwindowinfo = HWWindowInfo[j];
        if (!ctx->bGopHasInit[j]) {
            continue;
        }
        bitMask |= GOP_getBitMask(hwwindowinfo.gop);
        gop = hwwindowinfo.gop;
    }
    if (ctx->vsync_off) {
        MApi_GOP_GWIN_SetForceWrite(true);
    } else {
        MApi_GOP_GWIN_UpdateRegOnceByMask(bitMask,TRUE, FALSE);
    }

    // TODO: change mux, timing, 3D
    for (int j=0; j<ctx->gopCount; j++) {
        HWWindowInfo_t &hwwindowinfo = HWWindowInfo[j];
        HWWindowInfo_t &lasthwwindowinfo = ctx->lastHWWindowInfo[j];
        if (!ctx->bGopHasInit[j]) {
            continue;
        }

        if (hwwindowinfo.layerIndex < 0) {
            if (lasthwwindowinfo.layerIndex >= 0) {
                MApi_GOP_GWIN_Enable(hwwindowinfo.gwin, false);
#ifdef MALI_AFBC_GRALLOC
                //disable afbc
                ALOGD("GOP[%d]:disable afbc",hwwindowinfo.gop);
                MS_BOOL plist = FALSE;
                MApi_GOP_SetConfigEx(hwwindowinfo.gop,E_GOP_AFBC_ENABLE,&plist);
#endif
                if (lasthwwindowinfo.fbid != INVALID_FBID) {
                    MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo.fbid);
                    lasthwwindowinfo.fbid = INVALID_FBID;
                }
                if (lasthwwindowinfo.subfbid != INVALID_FBID) {
                    int status = MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo.subfbid);
                    if (status != GOP_API_SUCCESS) {
                        ALOGD("MApi_GOP_GWIN_Delete32FB sub failed");
                    }
                    lasthwwindowinfo.subfbid = INVALID_FBID;
                }
            }
        } else {

            // TODO: multiple gwin on single gop
            if ( lasthwwindowinfo.channel != hwwindowinfo.channel ) {
                int status=MApi_GOP_MIUSel(hwwindowinfo.gop, (EN_GOP_SEL_TYPE)hwwindowinfo.channel);
                if (status!=GOP_API_SUCCESS) {
                    ALOGD("gop miu sel failed ");
                }
            }
            // TODO: add video layer
            int pitch = hwwindowinfo.stride;
            // TODO: add resolution switch
            if (lasthwwindowinfo.fbid == INVALID_FBID ||
                lasthwwindowinfo.bufferWidth != hwwindowinfo.bufferWidth ||
                lasthwwindowinfo.bufferHeight != hwwindowinfo.bufferHeight ||
                lasthwwindowinfo.format != hwwindowinfo.format ||
                lasthwwindowinfo.stride != hwwindowinfo.stride ||
                ctx->b3DModeChanged ||
                lasthwwindowinfo.bEnableGopAFBC != hwwindowinfo.bEnableGopAFBC) {
                GOP_reCreateFBInfo(ctx->current3DMode,hwwindowinfo,lasthwwindowinfo);
                ctx->b3DModeChanged = false;
            } else {
                if (lasthwwindowinfo.fbid != INVALID_FBID) {
                    hwwindowinfo.fbid =lasthwwindowinfo.fbid;
                }
                if (lasthwwindowinfo.subfbid != INVALID_FBID) {
                    hwwindowinfo.subfbid =lasthwwindowinfo.subfbid;
                }
            }

            GOP_updateGWinInfo(ctx->current3DMode,hwwindowinfo,lasthwwindowinfo);
#ifdef MALI_AFBC_GRALLOC
            if (hwwindowinfo.bAFBCNeedReset) {
                ALOGD("Timing has changed and stabilized,reset AFBC now!!!!!");
                GOP_AFBCReset(hwwindowinfo);
            }
#endif
            MApi_GOP_GWIN_EnableTransClr_EX(hwwindowinfo.gop, GOPTRANSCLR_FMT0, false);

            // TODO: alpha, opaque, pre-mutiply
            if (hwwindowinfo.gop_dest==E_GOP_DST_OP0
#ifdef MSTAR_MASERATI
                || hwwindowinfo.gop_dest==E_GOP_DST_OP_DUAL_RATE
#endif
                ) {
                if (hwwindowinfo.blending!=lasthwwindowinfo.blending) {
                    switch (hwwindowinfo.blending) {
                        case HWC_BLENDING_PREMULT:
                            {
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo.gwin, TRUE, 0x3F);
                                // TODO:  pre-mutiply,using corresponding formula with opengl:SRC*1 + (1-As)*DST
                                E_APIXC_ReturnValue ret = MApi_XC_SetOSDBlendingFormula(
                                                                                       (E_XC_OSD_INDEX)GOP_GetLayerFromGOP(hwwindowinfo.gop),
                                                                                       E_XC_OSD_BlENDING_MODE1, MAIN_WINDOW);
                                if (ret != E_APIXC_RET_OK) {
                                    ALOGE("MApi_XC_SetOSDBlendingFormula return fail ret=%d",ret);
                                }
                                MApi_GOP_GWIN_SetNewAlphaMode(hwwindowinfo.gwin, true);
                                MApi_GOP_GWIN_SetAlphaInverse_EX(hwwindowinfo.gop,FALSE);
                            } break;
                        case HWC_BLENDING_COVERAGE:
                            {
                                // TODO: using corresponding formula with opengl:SRC*As + (1-As)*DST
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo.gwin, TRUE, 0x3F);
                                E_APIXC_ReturnValue ret = MApi_XC_SetOSDBlendingFormula(
                                                                                       (E_XC_OSD_INDEX)GOP_GetLayerFromGOP(hwwindowinfo.gop),
                                                                                       E_XC_OSD_BlENDING_MODE0, MAIN_WINDOW);
                                if (ret != E_APIXC_RET_OK) {
                                    ALOGE("MApi_XC_SetOSDBlendingFormula return fail ret=%d",ret);
                                }
                                MApi_GOP_GWIN_SetNewAlphaMode(hwwindowinfo.gwin, false);
                                MApi_GOP_GWIN_SetAlphaInverse_EX(hwwindowinfo.gop,TRUE);
                            } break;
                        case HWC_BLENDING_NONE:
                        default:
                            MApi_GOP_GWIN_SetBlending(hwwindowinfo.gwin, FALSE, hwwindowinfo.planeAlpha);
                            break;
                    }
                }
            } else if (hwwindowinfo.gop_dest==E_GOP_DST_FRC) {
            // TODO:what to do according to the hwwindowinfo.blending for FRC in the future?
               if (hwwindowinfo.blending!=lasthwwindowinfo.blending) {
                    switch (hwwindowinfo.blending) {
                        case HWC_BLENDING_PREMULT:
                        case HWC_BLENDING_COVERAGE:
                            {
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo.gwin, TRUE, 0x3F);
                                break;
                            }
                        case HWC_BLENDING_NONE:
                        default:
                            {
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo.gwin, FALSE, hwwindowinfo.planeAlpha);
                                break;
                            }
                    }
               }

            }

            if (ctx->current3DMode != DISPLAYMODE_NORMAL) {
                MS_U16 tagID = MApi_GFX_GetNextTAGID(TRUE);
                MApi_GFX_SetTAGID(tagID);
                // Need to ask gop owner afford other interface to set the subfb addr for 3D case and avoid gflip
                // set u16QueueCnt as 2 to avoid blocking the platform_work_thread_post_primary
                MS_U16 u16QueueCnt = 2;
                MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(hwwindowinfo.gwin, hwwindowinfo.mainFBphysicalAddr,
                                                    hwwindowinfo.subFBphysicalAddr, tagID, &u16QueueCnt);
            }


            if (lasthwwindowinfo.layerIndex < 0) {
                MApi_GOP_GWIN_Enable(hwwindowinfo.gwin, true);
            }
            //add debug code.disable osd
            if (property_get("mstar.disable.osd", property, "0") > 0) {
                int value = atoi(property);
                if (value == 1) {
                    if (MApi_GOP_GWIN_IsGWINEnabled(hwwindowinfo.gwin)) {
                        MApi_GOP_GWIN_Enable(hwwindowinfo.gwin, false);
                    }
                } else {
                    if (hwwindowinfo.layerIndex > 0 && !MApi_GOP_GWIN_IsGWINEnabled(hwwindowinfo.gwin)) {
                        MApi_GOP_GWIN_Enable(hwwindowinfo.gwin, true);
                    }
                }
            }
        }
    }
    // close the mbootlogo after mstar.close.bootlogo.gop.frame
    Gop_close_boot_logo(ctx);

    if (ctx->vsync_off) {
        MApi_GOP_GWIN_SetForceWrite(false);
    } else {
        MApi_GOP_GWIN_UpdateRegOnceByMask(bitMask,FALSE, FALSE);
        //update all gop together.so we need choose only one of gop to wait next vsync
        while (!MApi_GOP_IsRegUpdated(gop)) {
            usleep(1000);
            if (goptimeout++ > 100) {
                ALOGE("Error, MApi_GOP_IsRegUpdated timeout gop=%d, bitmask=%x", gop, bitMask);
                break;
            }
        }
    }
    /*when change timing, SN will mute gop output; after Surfaceflinger done the timing changed action,
      set property to inform SN to unmute gop output.
    */
    HWWindowInfo_t &hwwindowinfo = HWWindowInfo[0];
    HWWindowInfo_t &lasthwwindowinfo = ctx->lastHWWindowInfo[0];
    if ((hwwindowinfo.dstWidth != lasthwwindowinfo.dstWidth )
        ||(hwwindowinfo.dstHeight!= lasthwwindowinfo.dstHeight )
        &&(hwwindowinfo.srcWidth <= hwwindowinfo.dstWidth)
        &&(hwwindowinfo.srcHeight <= hwwindowinfo.dstHeight)) {
        if (property_get("mstar.resolutionState", property, NULL) > 0) {
            property_set("mstar.resolutionStateOSD", property);
            ALOGD("set prop mstar.resolutionStateOSD = %s",property );
        }
    }
}

unsigned int GOP_getDestination(hwc_context_t* ctx) {
    unsigned int GOPdest = -1;
    switch (ctx->ursaVersion) {
        case -1:
        case 0:
            if ((ctx->panelWidth==RES_4K2K_WIDTH)&&(ctx->panelHeight==RES_4K2K_HEIGHT)) {
                ctx->panelType  = E_PANEL_4K2K_CSOT;
            } else {
                ctx->panelType  = E_PANEL_FULLHD;
            }
            break;
        case 6:
            if ((ctx->panelWidth==RES_4K2K_WIDTH)&&(ctx->panelHeight==RES_4K2K_HEIGHT)) {
                ctx->panelType  = E_PANEL_4K2K_URSA6;
            } else {
                ctx->panelType  = E_PANEL_FULLHD_URSA6;
            }
            break;
        case 7:
            if ((ctx->panelWidth==RES_4K2K_WIDTH)&&(ctx->panelHeight==RES_4K2K_HEIGHT)) {
                ctx->panelType  = E_PANEL_4K2K_URSA7;
            } else {
                ctx->panelType = E_PANEL_FULLHD_URSA7;
            }
            break;
        case 8:
            if ((ctx->panelWidth==RES_4K2K_WIDTH)&&(ctx->panelHeight==RES_4K2K_HEIGHT)) {
                ctx->panelType = E_PANEL_4K2K_URSA8;
            } else {
                ctx->panelType = E_PANEL_FULLHD_URSA8;
            }
            break;
        case 9:
            if ((ctx->panelWidth==RES_4K2K_WIDTH)&&(ctx->panelHeight==RES_4K2K_HEIGHT)) {
                ctx->panelType = E_PANEL_4K2K_URSA9;
            } else {
                ctx->panelType = E_PANEL_FULLHD_URSA9;
            }
            break;
        case 11:
            if ((ctx->panelWidth==RES_4K2K_WIDTH)&&(ctx->panelHeight==RES_4K2K_HEIGHT)) {
                ctx->panelType = E_PANEL_4K2K_URSA11;
            } else {
                ctx->panelType = E_PANEL_FULLHD_URSA11;
            }
            break;
        default :
            ctx->panelType = E_PANEL_DEFAULT;
            break;
    }

#ifdef MSTAR_MASERATI
    if (ctx->panelLinkType == LINK_EXT && ctx->panelLinkExtType == LINK_VBY1_10BIT_16LANE) {
        ctx->panelType = E_PANEL_4K2K_120HZ;
    }
#endif

    switch (ctx->panelType) {
        case E_PANEL_FULLHD:
        case E_PANEL_FULLHD_URSA6:
        case E_PANEL_4K2K_URSA6:
        case E_PANEL_FULLHD_URSA8:
        case E_PANEL_4K2K_URSA8:
        case E_PANEL_4K2K_CSOT:
            GOPdest = E_GOP_DST_OP0;
            break;
        case E_PANEL_FULLHD_URSA7:
        case E_PANEL_4K2K_URSA7:
            if ((ctx->displayRegionWidth<RES_4K2K_WIDTH)&&(ctx->displayRegionHeight<RES_4K2K_HEIGHT)) {
                GOPdest = E_GOP_DST_FRC;
            } else if ((ctx->displayRegionWidth==RES_4K2K_WIDTH)&&(ctx->displayRegionHeight==RES_4K2K_HEIGHT)) {
                GOPdest = E_GOP_DST_OP0;
            }
            break;
        case E_PANEL_FULLHD_URSA9:
        case E_PANEL_4K2K_URSA9:
        case E_PANEL_FULLHD_URSA11:
        case E_PANEL_4K2K_URSA11:
            if(ctx->osdc_enable)
            {
                GOPdest = E_GOP_DST_FRC;
            }else{
                GOPdest = E_GOP_DST_OP0;
            }
            break;
#ifdef MSTAR_MASERATI
        case E_PANEL_4K2K_120HZ:
            GOPdest = E_GOP_DST_OP_DUAL_RATE;
            break;
#endif
        default:
            GOPdest = E_GOP_DST_OP0;
            break;
    }

    return GOPdest;
}

int GOP_getCurOPTiming(int *ret_width, int *ret_height, int *ret_hstart) {
    MS_PNL_DST_DispInfo dstDispInfo;
    memset(&dstDispInfo,0,sizeof(MS_PNL_DST_DispInfo));
    MApi_PNL_GetDstInfo(&dstDispInfo, sizeof(MS_PNL_DST_DispInfo));
    if(ret_width != NULL) {
        *ret_width  = dstDispInfo.DEHEND - dstDispInfo.DEHST + 1;
    }
    if(ret_height != NULL) {
        *ret_height = dstDispInfo.DEVEND - dstDispInfo.DEVST + 1;
    }
    if(ret_hstart != NULL) {
        *ret_hstart = dstDispInfo.DEHST;
    }
    return 0;
}

int GOP_getCurOCTiming(int *ret_width, int *ret_height, int *ret_hstart) {
    MS_OSDC_DST_DispInfo dstDispInfo;
    memset(&dstDispInfo,0,sizeof(MS_OSDC_DST_DispInfo));
    dstDispInfo.ODSC_DISPInfo_Version =ODSC_DISPINFO_VERSIN;
    MApi_XC_OSDC_GetDstInfo(&dstDispInfo,sizeof(MS_OSDC_DST_DispInfo));
    if(ret_width != NULL) {
        *ret_width  = dstDispInfo.DEHEND - dstDispInfo.DEHST + 1;
    }
    if(ret_height != NULL) {
        *ret_height = dstDispInfo.DEVEND - dstDispInfo.DEVST + 1;
    }

    if(ret_hstart != NULL) {
        *ret_hstart = dstDispInfo.DEHST;
    }
    return 0;
}

int GOP_setInterlave(int gop, int bInterlave) {
    ALOGI("gfx_GOPSetInterlave was invoked bInterlave=%d",bInterlave);
    /*GOP Interlave setting*/
    if (bInterlave==1) {
        MApi_GOP_GWIN_SetForceWrite(TRUE);
        MApi_GOP_GWIN_EnableProgressive_EX(gop,FALSE);
        MApi_GOP_GWIN_SetFieldInver_EX(gop,TRUE);
        MApi_GOP_GWIN_SetForceWrite(FALSE);
    } else {
        MApi_GOP_GWIN_SetForceWrite(TRUE);
        MApi_GOP_GWIN_EnableProgressive_EX(gop,TRUE);
        MApi_GOP_GWIN_SetFieldInver_EX(gop,FALSE);
        MApi_GOP_GWIN_SetForceWrite(FALSE);
    }
    return 0;
}
