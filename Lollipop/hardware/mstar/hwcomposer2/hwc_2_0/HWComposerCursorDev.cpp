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
#undef LOG_TAG
#define LOG_TAG "HWComposerCursorDev"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define __CLASS__ "HWComposerCursorDev"

#include "HWComposerCursorDev.h"

HWComposerCursorDev::HWComposerCursorDev(HWCDisplayDevice& display)
    :mDisplay(display),
    mHwcursordstfb(GAMECURSOR_COUNT)
{

}

HWComposerCursorDev::~HWComposerCursorDev() {

}

int HWComposerCursorDev::init(std::shared_ptr<HWCursorInfo>& hwcursorinfo) {
    GOP_InitInfo gopInitInfo;
    char property[PROPERTY_VALUE_MAX] = {0};

    //Mapi_GOP_GWIN_ResetGOP(hwcursorinfo->gop);

    EN_GOP_IGNOREINIT Initlist = E_GOP_IGNORE_MUX;
    memset(&gopInitInfo, 0, sizeof(GOP_InitInfo));
    gopInitInfo.u16PanelWidth  = mDisplay.mCurTimingWidth;
    gopInitInfo.u16PanelHeight = mDisplay.mCurTimingHeight;
    gopInitInfo.u16PanelHStr   = mDisplay.mPanelHStart;
    gopInitInfo.u32GOPRBAdr = 0;
    gopInitInfo.u32GOPRBLen = 0;
    gopInitInfo.bEnableVsyncIntFlip = TRUE;

    MApi_GOP_SetConfigEx(hwcursorinfo->gop,E_GOP_IGNOREINIT, (MS_U32 *)(&Initlist));
    MApi_GOP_InitByGOP(&gopInitInfo, hwcursorinfo->gop);

    MApi_GOP_RegisterFBFmtCB(_SetFBFmt);
    MApi_GOP_RegisterXCIsInterlaceCB(_XC_IsInterlace);
    MApi_GOP_RegisterXCGetCapHStartCB(_XC_GetCapwinHStr);
    MApi_GOP_RegisterXCReduceBWForOSDCB(_SetPQReduceBWForOSD);

    if (mDisplay.mMirrorModeH == 1) {
        MApi_GOP_GWIN_SetHMirror_EX(hwcursorinfo->gop, true);
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "SET THE HMirror!!!!!");
    }
    if (mDisplay.mMirrorModeV == 1) {
        MApi_GOP_GWIN_SetVMirror_EX(hwcursorinfo->gop, true);
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "SET THE VMirror!!!!");
    }

    if (mDisplay.bStbTarget && !mDisplay.bRGBColorSpace) {
        MApi_GOP_GWIN_OutputColor_EX(hwcursorinfo->gop, GOPOUT_YUV);
    }
    MApi_GOP_MIUSel(hwcursorinfo->gop, (EN_GOP_SEL_TYPE)hwcursorinfo->channel);
    MApi_GOP_GWIN_SetGOPDst(hwcursorinfo->gop, (EN_GOP_DST_TYPE)hwcursorinfo->gop_dest);

    if (property_get("mstar.gwin.alignment", property, "2") > 0) {
        hwcursorinfo->gwinPosAlignment = atoi(property);
    }
    DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwcursorinfo.gwinPosAlignment %d",hwcursorinfo->gwinPosAlignment);
    MApi_GOP_GWIN_EnableTransClr_EX(hwcursorinfo->gop, GOPTRANSCLR_FMT0, false);
    MApi_GOP_GWIN_SetBlending(hwcursorinfo->gwin, TRUE, 0x3F);

    return 0;
}

int HWComposerCursorDev::deinit(int32_t devNo UTILS_UNUSED) {
    return 0;
}

int32_t HWComposerCursorDev::getAvailGwinId(int gopNo) {
    if (gopNo < 0) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "getAvailGwinNum_ByGop the given gopNo is %d,check it!",gopNo);
        return -1;
    }
    int availGwinNum = 0;
    int i = 0;
    for (i; i < gopNo; i++) {
        availGwinNum += MApi_GOP_GWIN_GetGwinNum(i);
    }
    return availGwinNum;
}

uint32_t HWComposerCursorDev::getDestination() {
    unsigned int GOPdest = -1;
    int panelType;
    switch (mDisplay.mUrsaVersion) {
        case -1:
        case 0:
            if ((mDisplay.mPanelWidth==RES_4K2K_WIDTH)&&(mDisplay.mPanelHeight==RES_4K2K_HEIGHT)) {
                panelType  = E_PANEL_4K2K_CSOT;
            } else {
                panelType  = E_PANEL_FULLHD;
            }
            break;
        case 6:
            if ((mDisplay.mPanelWidth==RES_4K2K_WIDTH)&&(mDisplay.mPanelHeight==RES_4K2K_HEIGHT)) {
                panelType  = E_PANEL_4K2K_URSA6;
            } else {
                panelType  = E_PANEL_FULLHD_URSA6;
            }
            break;
        case 7:
            if ((mDisplay.mPanelWidth==RES_4K2K_WIDTH)&&(mDisplay.mPanelHeight==RES_4K2K_HEIGHT)) {
                panelType  = E_PANEL_4K2K_URSA7;
            } else {
                panelType = E_PANEL_FULLHD_URSA7;
            }
            break;
        case 8:
            if ((mDisplay.mPanelWidth==RES_4K2K_WIDTH)&&(mDisplay.mPanelHeight==RES_4K2K_HEIGHT)) {
                panelType = E_PANEL_4K2K_URSA8;
            } else {
                panelType = E_PANEL_FULLHD_URSA8;
            }
            break;
        case 9:
            if ((mDisplay.mPanelWidth==RES_4K2K_WIDTH)&&(mDisplay.mPanelHeight==RES_4K2K_HEIGHT)) {
                panelType = E_PANEL_4K2K_URSA9;
            } else {
                panelType = E_PANEL_FULLHD_URSA9;
            }
            break;
        case 11:
            if ((mDisplay.mPanelWidth==RES_4K2K_WIDTH)&&(mDisplay.mPanelHeight==RES_4K2K_HEIGHT)) {
                panelType = E_PANEL_4K2K_URSA11;
            } else {
                panelType = E_PANEL_FULLHD_URSA11;
            }
            break;
        default :
            panelType = E_PANEL_DEFAULT;
            break;
    }

#ifdef MSTAR_MASERATI
    if (mDisplay.mPanelLinkType == LINK_EXT && mDisplay.mPanelLinkExtType == LINK_VBY1_10BIT_16LANE) {
        panelType = E_PANEL_4K2K_120HZ;
    }
#endif

    switch (panelType) {
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
            if ((mDisplay.mDisplayRegionWidth<RES_4K2K_WIDTH)&&(mDisplay.mDisplayRegionHeight<RES_4K2K_HEIGHT)) {
                GOPdest = E_GOP_DST_FRC;
            } else if ((mDisplay.mDisplayRegionWidth==RES_4K2K_WIDTH)&&(mDisplay.mDisplayRegionHeight==RES_4K2K_HEIGHT)) {
                GOPdest = E_GOP_DST_OP0;
            }
            break;
        case E_PANEL_FULLHD_URSA9:
        case E_PANEL_4K2K_URSA9:
        case E_PANEL_FULLHD_URSA11:
        case E_PANEL_4K2K_URSA11:
            if(mDisplay.mOsdc_enable)
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

unsigned int HWComposerCursorDev::getMaskPhyAddr(int u8MiuSel) {
    unsigned int miu0Len = mmap_get_miu0_length();
    unsigned int miu1Len = mmap_get_miu1_length();
    unsigned int miu2Len = mmap_get_miu2_length();

    if (u8MiuSel==2) {
        DLOGI_IF(HWC2_DEBUGTYPE_HWCCURSOR, "HWComposerCursorDev_getMaskPhyAddr MIU2 mask=%x", (miu2Len-1));
        return(miu2Len - 1) ;
    } else if (u8MiuSel==1) {
        DLOGI_IF(HWC2_DEBUGTYPE_HWCCURSOR, "HWComposerCursorDev_getMaskPhyAddr MIU1 mask=%x", (miu1Len-1));
        return(miu1Len - 1);
    } else {
        DLOGI_IF(HWC2_DEBUGTYPE_HWCCURSOR, "HWComposerCursorDev_getMaskPhyAddr MIU0 mask=%x", (miu0Len-1));
        return(miu0Len - 1) ;
    }
}

int HWComposerCursorDev::hwCursorBlitToFB(std::shared_ptr<HWCursorInfo>& transaction, int osdMode, int32_t devNo) {
    GFX_BufferInfo srcbuf, dstbuf;
    GFX_Point v0, v1;
    GFX_DrawRect bitbltInfo;
    GFX_RgbColor color={0x0, 0x0, 0x0, 0x0};

    if ((transaction->dstWidth > HWCURSOR_MAX_WIDTH)
        ||(transaction->dstHeight > HWCURSOR_MAX_HEIGHT)
        || (transaction->srcWidth > HWCURSOR_MAX_WIDTH)
        ||(transaction->srcHeight> HWCURSOR_MAX_HEIGHT)) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursorBlitToFB size setting is error!!");
        return -1;
    }
    MApi_GFX_BeginDraw();
    MApi_GFX_EnableDFBBlending(TRUE);
    MApi_GFX_EnableAlphaBlending(FALSE);
    MApi_GFX_SetDFBBldFlags(GFX_DFB_BLD_FLAG_ALPHACHANNEL|GFX_DFB_BLD_FLAG_COLORALPHA);
    MApi_GFX_SetDFBBldOP(GFX_DFB_BLD_OP_SRCALPHA, GFX_DFB_BLD_OP_ZERO);

    v0.x = 0;
    v0.y = 0;
    v1.x = transaction->dstWidth;
    v1.y = transaction->dstHeight;
    MApi_GFX_SetClip(&v0, &v1);

    srcbuf.u32ColorFmt = GFX_FMT_ARGB8888;
    srcbuf.u32Addr = transaction->srcAddr + transaction->srcX * BYTE_PER_PIXEL + transaction->srcY * transaction->srcStride;
    srcbuf.u32Width =  transaction->srcWidth;
    srcbuf.u32Height = transaction->srcHeight;
    srcbuf.u32Pitch = transaction->srcStride;
    if (MApi_GFX_SetSrcBufferInfo(&srcbuf, 0) != GFX_SUCCESS) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursorBlitToFB GFX set SrcBuffer failed\n");
        goto HW_COPYICON_ERROR;
    }

    dstbuf.u32ColorFmt = GFX_FMT_ARGB8888;
    dstbuf.u32Addr = transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]];
    dstbuf.u32Width = transaction->dstWidth;
    dstbuf.u32Height = transaction->dstHeight;
    dstbuf.u32Pitch = HWCURSOR_MAX_WIDTH * BYTE_PER_PIXEL;
    if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursorBlitToFB GFX set DetBuffer failed\n");
        goto HW_COPYICON_ERROR;
    }
    color.a = transaction->planeAlpha;
    color.r = 0xff;
    color.g = 0xff;
    color.b = 0xff;
    MApi_GFX_SetDFBBldConstColor(color);

    bitbltInfo.srcblk.x = 0;
    bitbltInfo.srcblk.y = 0;
    bitbltInfo.srcblk.width = transaction->srcWidth;
    bitbltInfo.srcblk.height = transaction->srcHeight;
    bitbltInfo.dstblk.x = 0;
    bitbltInfo.dstblk.y = 0;
    bitbltInfo.dstblk.width = transaction->dstWidth;
    bitbltInfo.dstblk.height = transaction->dstHeight;

    if (osdMode == DISPLAYMODE_TOPBOTTOM_LA ||
        osdMode == DISPLAYMODE_NORMAL_LA ||
        osdMode == DISPLAYMODE_TOP_LA ||
        osdMode == DISPLAYMODE_BOTTOM_LA||
        osdMode == DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY ||
        osdMode == DISPLAYMODE_NORMAL_LA_HIGH_QUALITY ||
        osdMode == DISPLAYMODE_TOP_LA_HIGH_QUALITY ||
        osdMode == DISPLAYMODE_BOTTOM_LA_HIGH_QUALITY ||
        osdMode == DISPLAYMODE_TOPBOTTOM ||
        osdMode == DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY ||
        osdMode == DISPLAYMODE_TOP_ONLY ||
        osdMode == DISPLAYMODE_BOTTOM_ONLY) {
        bitbltInfo.dstblk.height = bitbltInfo.dstblk.height >> 1;
    }  else if (osdMode == DISPLAYMODE_LEFTRIGHT ||
               osdMode == DISPLAYMODE_LEFT_ONLY ||
               osdMode == DISPLAYMODE_RIGHT_ONLY) {
        bitbltInfo.dstblk.width= bitbltInfo.dstblk.width>> 1;
    }

    if (MApi_GFX_BitBlt(&bitbltInfo, GFXDRAW_FLAG_SCALE) != GFX_SUCCESS) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "hwCursorBlitToFB GFX BitBlt failed\n");
        goto HW_COPYICON_ERROR;
    }

    MApi_GFX_EnableAlphaBlending(TRUE);
    MApi_GFX_EndDraw();
    MApi_GFX_SetTAGID(MApi_GFX_GetNextTAGID(TRUE));

    return 0;
    HW_COPYICON_ERROR:
    MApi_GFX_EndDraw();
    return -1;
}

int HWComposerCursorDev::hwCusorSetPosition(std::shared_ptr<HWCursorInfo>& transaction, int osdMode) {
    int gwinposX = 0;
    int gwinposY = 0;
    int stretchWinX = 0;
    int stretechWinY = 0;
    int posX = transaction->positionX;
    int posY = transaction->positionY;
    int osdWidth = transaction->gop_srcWidth;
    int osdHeight = transaction->gop_srcHeight;

    switch (osdMode) {
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_LEFT_ONLY:
        case DISPLAYMODE_LEFTRIGHT_FULL:
        {
            posX = posX >> 1;
            osdWidth = osdWidth>>1;
            break;
        }
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY:
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY:
        case DISPLAYMODE_NORMAL_LA_HIGH_QUALITY:
        case DISPLAYMODE_TOP_LA_HIGH_QUALITY:
        case DISPLAYMODE_BOTTOM_LA_HIGH_QUALITY:
        case DISPLAYMODE_BOTTOM_LA:
        {
            posY = posY >> 1;
            osdHeight = osdHeight>>1;
            break;
        }
        case DISPLAYMODE_RIGHT_ONLY:
        {
            posX = (posX >> 1) + (osdWidth >> 1);
            break;
        }
        case DISPLAYMODE_BOTTOM_ONLY:
        {
            posY = (posY >> 1) + (osdHeight>> 1);
            break;
        }
        default:
            break;
    }
    gwinposX = posX / transaction->gwinPosAlignment * transaction->gwinPosAlignment;
    stretchWinX = posX % transaction->gwinPosAlignment;
    gwinposY = posY;
    stretechWinY = 0;
    //set stretchwindow
    MApi_GOP_GWIN_Set_STRETCHWIN(transaction->gop, (EN_GOP_DST_TYPE)transaction->gop_dest,
                                 stretchWinX, stretechWinY, osdWidth, osdHeight);

    MApi_GOP_GWIN_SetWinPosition(transaction->gwin, gwinposX, gwinposY);

    /* one utopia use this to replace MApi_GOP_GWIN_SetWinPosition, but has some issue now, so mask it temporarily
    GOP_GwinInfo info;
    MApi_GOP_GWIN_GetWinInfo( transaction->gwin, &info);
    info.u16DispHPixelStart = gwinposX;
    info.u16DispVPixelStart = gwinposY;
    MApi_GOP_GWIN_SetWinInfo (transaction->gwin, &info);
    */
    return 0;
}

void HWComposerCursorDev::hwCursorConfigGop(std::shared_ptr<HWCursorInfo>& transaction, int osdMode, int32_t devNo) {
    int bufferWidth = transaction->dstWidth;
    int bufferHeight = transaction->dstHeight;
    int pitch = HWCURSOR_MAX_WIDTH * BYTE_PER_PIXEL;
    int osdWidth = transaction->gop_srcWidth;
    int osdHeight = transaction->gop_srcHeight;
    int panelWidth = transaction->gop_dstWidth;
    int panelHeight = transaction->gop_dstHeight;
    int panelHStart = 112;
    int gwinId = transaction->gwin;
    int gopNo = transaction->gop;
    unsigned int fbid = MAX_GWIN_FB_SUPPORT;
    unsigned int U32MainFbid = MAX_GWIN_FB_SUPPORT;
    unsigned int U32SubFbid = MAX_GWIN_FB_SUPPORT;
    EN_GOP_3D_MODETYPE mode = E_GOP_3D_DISABLE;
    int gfmt = E_MS_FMT_ABGR8888;

    unsigned int u32OrgFbId = MApi_GOP_GWIN_Get32FBfromGWIN(gwinId);
    if (u32OrgFbId != MAX_GWIN_FB_SUPPORT) {
        MApi_GOP_GWIN_Delete32FB(u32OrgFbId);
    }

    switch (osdMode) {
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_BOTTOM_LA:
        case DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY:
        case DISPLAYMODE_NORMAL_LA_HIGH_QUALITY:
        case DISPLAYMODE_TOP_LA_HIGH_QUALITY:
        case DISPLAYMODE_BOTTOM_LA_HIGH_QUALITY:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight>>1, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = U32SubFbid = fbid;
            osdHeight = osdHeight>>1;
            panelHeight = panelHeight>>1;
            mode = E_GOP_3D_LINE_ALTERNATIVE;
        }
        break;
        case DISPLAYMODE_LEFTRIGHT_FR:
        case DISPLAYMODE_NORMAL_FR:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = U32SubFbid = fbid;
            mode = E_GOP_3D_SWITH_BY_FRAME;
        }
        break;
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_LEFTRIGHT_FULL:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth>>1, bufferHeight, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = U32SubFbid = fbid;
            osdWidth = osdWidth>>1;
            panelWidth = panelWidth>>1;
            mode = E_GOP_3D_SIDE_BY_SYDE;
        }
        break;
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight>>1, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = U32SubFbid = fbid;
            mode = E_GOP_3D_TOP_BOTTOM;
            osdHeight = osdHeight>>1;
            panelHeight = panelHeight>>1;
        }
        break;
        case DISPLAYMODE_LEFT_ONLY:
        case DISPLAYMODE_RIGHT_ONLY:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth>>1, bufferHeight, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = fbid;
            U32SubFbid = MAX_GWIN_FB_SUPPORT;
            mode = E_GOP_3D_DISABLE;
        }
        break;
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight>>1, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = fbid;
            U32SubFbid = MAX_GWIN_FB_SUPPORT;
            mode = E_GOP_3D_DISABLE;
        }
        break;
        case DISPLAYMODE_NORMAL:
        default:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight, gfmt,
                                                transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = fbid;
            U32SubFbid = MAX_GWIN_FB_SUPPORT;
            mode = E_GOP_3D_DISABLE;
        }
        break;
    }
    transaction->fbid = fbid;

    ST_GOP_TIMING_INFO stGopTiming;
    memset(&stGopTiming, 0, sizeof(ST_GOP_TIMING_INFO));
    MApi_GOP_GetConfigEx(transaction->gop, E_GOP_TIMING_INFO, (MS_U32*)&stGopTiming);
    stGopTiming.u16DEHSize = transaction->gop_dstWidth;
    stGopTiming.u16DEVSize = transaction->gop_dstHeight;
    MApi_GOP_SetConfigEx(transaction->gop, E_GOP_TIMING_INFO, (MS_U32*)&stGopTiming);

    //update gop setting and set stretch win and gwin position
    MApi_GOP_GWIN_Map32FB2Win(fbid, gwinId);

    getCurOPTiming(NULL,NULL,&panelHStart);
    MApi_GOP_SetGOPHStart(gopNo, panelHStart);

    MApi_GOP_Set3DOSDMode(gwinId, U32MainFbid, U32SubFbid, mode);
    setStretchWindow(gopNo, osdWidth, osdHeight, 0, 0, panelWidth, panelHeight);
    hwCusorSetPosition(transaction, osdMode);
}

hwc2_error_t HWComposerCursorDev::getCurOPTiming(int *ret_width, int *ret_height, int *ret_hstart) {
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
    return HWC2_ERROR_NONE;
}

uint32_t HWComposerCursorDev::getBitMask(unsigned int gop) {
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

hwc2_error_t HWComposerCursorDev::setStretchWindow(unsigned char gop,  unsigned short srcWidth, unsigned short srcHeight,
                          unsigned short dstX, unsigned short dstY, unsigned short dstWidth, unsigned short dstHeight) {
    MS_U8 gopNum = MApi_GOP_GWIN_GetMaxGOPNum();
    if (gop >= gopNum) {
        DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "GOP_SetStretchWindow was invoked invalid gop=%d",gop);
        return HWC2_ERROR_NOT_VALIDATED;
    }
    /* GOP stretch setting */
    MApi_GOP_GWIN_Set_HSCALE_EX(gop,TRUE, srcWidth, dstWidth);
    MApi_GOP_GWIN_Set_VSCALE_EX(gop,TRUE, srcHeight, dstHeight);
    MApi_GOP_GWIN_Set_STRETCHWIN(gop, E_GOP_DST_OP0, dstX, dstY, srcWidth, srcHeight);
    return HWC2_ERROR_NONE;
}

int HWComposerCursorDev::present(std::shared_ptr<HWCursorInfo>& transaction, int32_t devNo) {
    unsigned short u16QueueCnt = 1;
    unsigned short tagID;
    int goptimeout = 0;
    static int gPrevOSDmode = 0;
    char property[PROPERTY_VALUE_MAX] = {0};
    int osdMode = 0;
    int gwinposX = 0;
    int gwinposY = 0;
    int stretchWinX = 0;
    int stretechWinY = 0;
    unsigned char needFlip = 0;
    MS_U8 currentGOP;
    int gopNo = transaction->gop;
    if (property_get("mstar.desk-display-mode", property, NULL) > 0) {
        osdMode = atoi(property);
    }
    //MApi_GOP_GWIN_UpdateRegOnceEx2(gopNo, TRUE, FALSE);
    MApi_GOP_GWIN_UpdateRegOnceByMask(getBitMask(gopNo),TRUE, FALSE);
    if (transaction->operation & HWCURSOR_HIDE){
        if (MApi_GOP_GWIN_IsGWINEnabled(transaction->gwin)){
            MApi_GOP_GWIN_Enable(transaction->gwin, FALSE);
        }
        goto done;
    }
    if ((transaction->operation & HWCURSOR_ICON) ||
        (transaction->operation & HWCURSOR_ALPHA) ||
        osdMode != gPrevOSDmode) {
        if (transaction->acquireFenceFd != -1) {
            int err = sync_wait(transaction->acquireFenceFd, 1000);
            if (err < 0) {
                DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "acquirefence sync_wait failed in hwcursor work thread");
            }
            close(transaction->acquireFenceFd);
            transaction->acquireFenceFd = -1;
        }
        //Blit it and create FB
        mHwcursordstfb[devNo] = (mHwcursordstfb[devNo] + 1) % HWCURSOR_DSTFB_NUM;
        hwCursorBlitToFB(transaction, osdMode, devNo);
        hwCursorConfigGop(transaction, osdMode, devNo);
        needFlip = 1;
        gPrevOSDmode = osdMode;
    }
    if (transaction->operation & HWCURSOR_POSITION
        && !needFlip) {
        hwCusorSetPosition(transaction, osdMode);
    }
    if (needFlip) {
        tagID = MApi_GFX_GetNextTAGID(FALSE);
        u16QueueCnt = 1;
        if (gPrevOSDmode != DISPLAYMODE_NORMAL) {
            if (!MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(transaction->gwin, transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]],
                                                    transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]], tagID, &u16QueueCnt)) {
                DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "flip errro! in 3D sterre model!");
            }
        } else {
            if (!MApi_GOP_Switch_GWIN_2_FB_BY_ADDR(transaction->gwin, transaction->mHwCursorDstAddr[mHwcursordstfb[devNo]],
                                                   tagID, &u16QueueCnt)) {
                DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "flip errro in normal model!");
            }
        }
        if (u16QueueCnt <= 0) {
            DLOGE_IF(HWC2_DEBUGTYPE_HWCCURSOR, "Serious warning, unknow error!!\n");
        }
    }
    if (MApi_GOP_GWIN_IsGWINEnabled(transaction->gwin)==0) {
        MApi_GOP_GWIN_Enable(transaction->gwin, TRUE);
    }

done:
    //MApi_GOP_GWIN_UpdateRegOnceEx2(gopNo, FALSE, FALSE);
    MApi_GOP_GWIN_UpdateRegOnceByMask(getBitMask(gopNo),FALSE, FALSE);
    if (!needFlip) {
        while (!MApi_GOP_IsRegUpdated(transaction->gop) && (goptimeout++ <= GOP_TIMEOUT_CNT))
            usleep(1000);
    }
    showfps();
    return 0;

}

void HWComposerCursorDev::showfps() {
    static int frames = 0;
    static struct timeval begin;
    static struct timeval end;
    if (frames==0) {
        gettimeofday(&begin,NULL);
        end = begin;
    }
    frames++;
    if (frames%100==0) {
        gettimeofday(&end,NULL);
        float fps = 100.0*1000/((end.tv_sec-begin.tv_sec)*1000+(end.tv_usec-begin.tv_usec)/1000);
        begin = end;
        DLOGD_IF(HWC2_DEBUGTYPE_HWCCURSOR, "show hwcursor fps is %f",fps);
    }
}
