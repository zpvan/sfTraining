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
#define LOG_TAG "HWComposerGop"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define __CLASS__ "HWComposerGop"

#include "HWComposerGop.h"
#include <cutils/log.h>
#include <gralloc_priv.h>
#include <cutils/properties.h>



HWComposerGop::HWComposerGop(HWCDisplayDevice& display)
    :mDisplay(display),
    bGopHasInit()
{
}
HWComposerGop::~HWComposerGop() {

}

int32_t HWComposerGop::getAvailGwinId(int gopNo) {
    if (gopNo < 0) {
        DLOGE("getAvailGwinNum_ByGop the given gopNo is %d,check it!",gopNo);
        return -1;
    }
    int availGwinNum = 0;
    int i = 0;
    for (i; i < gopNo; i++) {
        availGwinNum += MApi_GOP_GWIN_GetGwinNum(i);
    }
    return availGwinNum;
}
hwc2_error_t HWComposerGop::init(std::shared_ptr<HWWindowInfo>& hwwindowinfo) {
    GOP_InitInfo gopInitInfo;

    Mapi_GOP_GWIN_ResetGOP(hwwindowinfo->gop);

    EN_GOP_IGNOREINIT Initlist = E_GOP_IGNORE_MUX;
    memset(&gopInitInfo, 0, sizeof(GOP_InitInfo));
    gopInitInfo.u16PanelWidth  = mDisplay.mCurTimingWidth;
    gopInitInfo.u16PanelHeight = mDisplay.mCurTimingHeight;
    gopInitInfo.u16PanelHStr   = mDisplay.mPanelHStart;
    gopInitInfo.u32GOPRBAdr = 0;
    gopInitInfo.u32GOPRBLen = 0;
    gopInitInfo.bEnableVsyncIntFlip = TRUE;

    MApi_GOP_SetConfigEx(hwwindowinfo->gop,E_GOP_IGNOREINIT, (MS_U32 *)(&Initlist));
    MApi_GOP_InitByGOP(&gopInitInfo, hwwindowinfo->gop);

    MApi_GOP_RegisterFBFmtCB(_SetFBFmt);
    MApi_GOP_RegisterXCIsInterlaceCB(_XC_IsInterlace);
    MApi_GOP_RegisterXCGetCapHStartCB(_XC_GetCapwinHStr);
    MApi_GOP_RegisterXCReduceBWForOSDCB(_SetPQReduceBWForOSD);

    if (mDisplay.mMirrorModeH == 1) {
        MApi_GOP_GWIN_SetHMirror_EX(hwwindowinfo->gop, true);
        DLOGD("SET THE HMirror!!!!!");
    }
    if (mDisplay.mMirrorModeV == 1) {
        MApi_GOP_GWIN_SetVMirror_EX(hwwindowinfo->gop, true);
        DLOGD("SET THE VMirror!!!!");
    }

    if (mDisplay.bStbTarget && !mDisplay.bRGBColorSpace) {
        MApi_GOP_GWIN_OutputColor_EX(hwwindowinfo->gop, GOPOUT_YUV);
    }
    MApi_GOP_MIUSel(hwwindowinfo->gop, (EN_GOP_SEL_TYPE)hwwindowinfo->channel);
    static bool layerHasSet = false;
    if (!layerHasSet) {
        GOP_LayerConfig stLayerSetting = mDisplay.stLayerSetting;
        E_GOP_API_Result enResult = MApi_GOP_GWIN_SetLayer(&stLayerSetting, sizeof(GOP_LayerConfig));
        if (enResult == GOP_API_SUCCESS) {
            layerHasSet = true;
            DLOGD("MApi_GOP_GWIN_SetLayer  success %s(%d)\n",__FUNCTION__,__LINE__);
        } else {
            DLOGD("MApi_GOP_GWIN_SetLayer fail, return %u!!! %s(%d)\n", enResult,__FUNCTION__,__LINE__);
        }
    }
    MApi_GOP_GWIN_SetGOPDst(hwwindowinfo->gop, (EN_GOP_DST_TYPE)hwwindowinfo->gop_dest);
    return HWC2_ERROR_NONE;

}

hwc2_error_t HWComposerGop::display( std::vector<std::shared_ptr<HWWindowInfo>>& inHWwindowinfo,
                            std::vector<std::shared_ptr<HWWindowInfo>>& lastHWwindowinfo) {
    int bitMask = 0;
    int gop = 0;
    int goptimeout = 0;

    char property[PROPERTY_VALUE_MAX] = {0};
    if (bGopHasInit.empty()) {
        bGopHasInit.assign(mDisplay.mGopCount,FALSE);
    }

    int gopCount = mDisplay.mGopCount;
    for (int j=0; j< gopCount; j++) {
        auto hwwindowinfo = inHWwindowinfo[j];
        if (hwwindowinfo->layerIndex >= 0) {
            if (!bGopHasInit.at(j)) {
                DLOGD("init Gop[%d]",hwwindowinfo->gop);
                init(hwwindowinfo);
                bGopHasInit.at(j) = TRUE;
            }
        }
    }

    if (!bGopHasInit.at(0)) return HWC2_ERROR_NOT_VALIDATED;

    for (int j=0; j<gopCount; j++) {
        auto hwwindowinfo = inHWwindowinfo[j];
        if (!bGopHasInit.at(j)) {
            continue;
        }
        bitMask |= getBitMask(hwwindowinfo->gop);
        gop = hwwindowinfo->gop;
    }
    if (mDisplay.bVsyncOff) {
        MApi_GOP_GWIN_SetForceWrite(true);
    } else {
        MApi_GOP_GWIN_UpdateRegOnceByMask(bitMask,TRUE, FALSE);
    }

    // TODO: change mux, timing, 3D

    for (int j=0; j<gopCount; j++) {
        auto hwwindowinfo = inHWwindowinfo[j];
        auto lasthwwindowinfo = lastHWwindowinfo[j];
        if (!bGopHasInit.at(j)) {
            continue;
        }

        if (hwwindowinfo->layerIndex < 0) {
            if (lasthwwindowinfo->layerIndex >= 0) {
                DLOGD("GOP[%d]:disable gwin[%d]",hwwindowinfo->gop,hwwindowinfo->gwin);
                MApi_GOP_GWIN_Enable(hwwindowinfo->gwin, false);
#ifdef MALI_AFBC_GRALLOC
                //disable afbc
                DLOGD("GOP[%d]:disable afbc",hwwindowinfo->gop);
                MS_BOOL plist = FALSE;
                MApi_GOP_SetConfigEx(hwwindowinfo->gop,E_GOP_AFBC_ENABLE,&plist);
#endif
                if (lasthwwindowinfo->fbid != INVALID_FBID) {
                    MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo->fbid);
                    lasthwwindowinfo->fbid = INVALID_FBID;
                }
                if (lasthwwindowinfo->subfbid != INVALID_FBID) {
                    int status = MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo->subfbid);
                    if (status != GOP_API_SUCCESS) {
                        DLOGD("MApi_GOP_GWIN_Delete32FB sub failed");
                    }
                    lasthwwindowinfo->subfbid = INVALID_FBID;
                }
            }
        } else {

            // TODO: multiple gwin on single gop
            if ( lasthwwindowinfo->channel != hwwindowinfo->channel ) {
                int status=MApi_GOP_MIUSel(hwwindowinfo->gop, (EN_GOP_SEL_TYPE)hwwindowinfo->channel);
                if (status!=GOP_API_SUCCESS) {
                    DLOGD("gop miu sel failed ");
                }
            }
            // TODO: add video layer
            int pitch = hwwindowinfo->stride;
            // TODO: add resolution switch
            if (lasthwwindowinfo->fbid == INVALID_FBID ||
                lasthwwindowinfo->bufferWidth != hwwindowinfo->bufferWidth ||
                lasthwwindowinfo->bufferHeight != hwwindowinfo->bufferHeight ||
                lasthwwindowinfo->format != hwwindowinfo->format ||
                lasthwwindowinfo->stride != hwwindowinfo->stride ||
                mDisplay.b3DModeChanged ||
                lasthwwindowinfo->bEnableGopAFBC != hwwindowinfo->bEnableGopAFBC) {
                reCreateFBInfo(mDisplay.mCurrent3DMode,hwwindowinfo,lasthwwindowinfo);
                mDisplay.b3DModeChanged  = false;
            } else {
                if (lasthwwindowinfo->fbid != INVALID_FBID) {
                    hwwindowinfo->fbid = lasthwwindowinfo->fbid;
                }
                if (lasthwwindowinfo->subfbid != INVALID_FBID) {
                    hwwindowinfo->subfbid =lasthwwindowinfo->subfbid;
                }
            }

            updateGWinInfo(mDisplay.mCurrent3DMode,hwwindowinfo,lasthwwindowinfo);
#ifdef MALI_AFBC_GRALLOC
            if (hwwindowinfo->bAFBCNeedReset) {
                DLOGD("Timing has changed and stabilized,reset AFBC now!!!!!");
                resetAFBC(hwwindowinfo);
            }
#endif
            MApi_GOP_GWIN_EnableTransClr_EX(hwwindowinfo->gop, GOPTRANSCLR_FMT0, false);

            // TODO: alpha, opaque, pre-mutiply
            if (hwwindowinfo->gop_dest==E_GOP_DST_OP0
#ifdef MSTAR_MASERATI
                || hwwindowinfo->gop_dest==E_GOP_DST_OP_DUAL_RATE
#endif
                ) {
                if (hwwindowinfo->blending!=lasthwwindowinfo->blending) {
                    switch (hwwindowinfo->blending) {
                        case HWC2_BLEND_MODE_PREMULTIPLIED:
                            {
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo->gwin, TRUE, 0x3F);
                                // TODO:  pre-mutiply,using corresponding formula with opengl:SRC*1 + (1-As)*DST
                                E_APIXC_ReturnValue ret = MApi_XC_SetOSDBlendingFormula(
                                                                                       (E_XC_OSD_INDEX)getLayerFromGOP(hwwindowinfo->gop),
                                                                                       E_XC_OSD_BlENDING_MODE1, MAIN_WINDOW);
                                if (ret != E_APIXC_RET_OK) {
                                    DLOGE("MApi_XC_SetOSDBlendingFormula return fail ret=%d",ret);
                                }
                                MApi_GOP_GWIN_SetNewAlphaMode(hwwindowinfo->gwin, true);
                                MApi_GOP_GWIN_SetAlphaInverse_EX(hwwindowinfo->gop,FALSE);
                            } break;
                        case HWC2_BLEND_MODE_COVERAGE:
                            {
                                // TODO: using corresponding formula with opengl:SRC*As + (1-As)*DST
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo->gwin, TRUE, 0x3F);
                                E_APIXC_ReturnValue ret = MApi_XC_SetOSDBlendingFormula(
                                                                                       (E_XC_OSD_INDEX)getLayerFromGOP(hwwindowinfo->gop),
                                                                                       E_XC_OSD_BlENDING_MODE0, MAIN_WINDOW);
                                if (ret != E_APIXC_RET_OK) {
                                    DLOGE("MApi_XC_SetOSDBlendingFormula return fail ret=%d",ret);
                                }
                                MApi_GOP_GWIN_SetNewAlphaMode(hwwindowinfo->gwin, false);
                                MApi_GOP_GWIN_SetAlphaInverse_EX(hwwindowinfo->gop,TRUE);
                            } break;
                        case HWC2_BLEND_MODE_NONE:
                        default:
                            MApi_GOP_GWIN_SetBlending(hwwindowinfo->gwin, FALSE, hwwindowinfo->planeAlpha);
                            break;
                    }
                }
            } else if (hwwindowinfo->gop_dest==E_GOP_DST_FRC) {
            // TODO:what to do according to the hwwindowinfo.blending for FRC in the future?
               if (hwwindowinfo->blending!=lasthwwindowinfo->blending) {
                    switch (hwwindowinfo->blending) {
                        case HWC2_BLEND_MODE_PREMULTIPLIED:
                        case HWC2_BLEND_MODE_COVERAGE:
                            {
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo->gwin, TRUE, 0x3F);
                                break;
                            }
                        case HWC2_BLEND_MODE_NONE:
                        default:
                            {
                                MApi_GOP_GWIN_SetBlending(hwwindowinfo->gwin, FALSE, hwwindowinfo->planeAlpha);
                                break;
                            }
                    }
               }

            }

            if (mDisplay.mCurrent3DMode != DISPLAYMODE_NORMAL) {
                MS_U16 tagID = MApi_GFX_GetNextTAGID(TRUE);
                MApi_GFX_SetTAGID(tagID);
                // Need to ask gop owner afford other interface to set the subfb addr for 3D case and avoid gflip
                // set u16QueueCnt as 2 to avoid blocking the platform_work_thread_post_primary
                MS_U16 u16QueueCnt = 2;
                MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(hwwindowinfo->gwin, hwwindowinfo->mainFBphysicalAddr,
                                                    hwwindowinfo->subFBphysicalAddr, tagID, &u16QueueCnt);
            }


            if (lasthwwindowinfo->layerIndex < 0) {
                DLOGD("GOP[%d]:enable gwin[%d]",hwwindowinfo->gop,hwwindowinfo->gwin);
                MApi_GOP_GWIN_Enable(hwwindowinfo->gwin, true);
            }
            //add debug code.disable osd
            if (property_get("mstar.disable.osd", property, "0") > 0) {
                int value = atoi(property);
                if (value == 1) {
                    if (MApi_GOP_GWIN_IsGWINEnabled(hwwindowinfo->gwin)) {
                        MApi_GOP_GWIN_Enable(hwwindowinfo->gwin, false);
                    }
                } else {
                    if (hwwindowinfo->layerIndex > 0 && !MApi_GOP_GWIN_IsGWINEnabled(hwwindowinfo->gwin)) {
                        MApi_GOP_GWIN_Enable(hwwindowinfo->gwin, true);
                    }
                }
            }
        }
    }
    // close the mbootlogo after mstar.close.bootlogo.gop.frame
    close_boot_logo(inHWwindowinfo[0]);

    if (mDisplay.bVsyncOff) {
        MApi_GOP_GWIN_SetForceWrite(false);
    } else {
        MApi_GOP_GWIN_UpdateRegOnceByMask(bitMask,FALSE, FALSE);
        //update all gop together.so we need choose only one of gop to wait next vsync
        while (!MApi_GOP_IsRegUpdated(gop)) {
            usleep(1000);
            if (goptimeout++ > 100) {
                DLOGE("Error, MApi_GOP_IsRegUpdated timeout gop=%d, bitmask=%x", gop, bitMask);
                break;
            }
        }
    }
    /*when change timing, SN will mute gop output; after Surfaceflinger done the timing changed action,
      set property to inform SN to unmute gop output.
    */

    auto hwwindowinfo = inHWwindowinfo[0];
    auto lasthwwindowinfo = lastHWwindowinfo[0];
    if ((hwwindowinfo->dstWidth != lasthwwindowinfo->dstWidth )
        ||(hwwindowinfo->dstHeight!= lasthwwindowinfo->dstHeight )
        &&(hwwindowinfo->srcWidth <= hwwindowinfo->dstWidth)
        &&(hwwindowinfo->srcHeight <= hwwindowinfo->dstHeight)) {
        if (property_get("mstar.resolutionState", property, NULL) > 0) {
            property_set("mstar.resolutionStateOSD", property);
            DLOGD("set prop mstar.resolutionStateOSD = %s",property );
        }
    }
    return HWC2_ERROR_NONE;
}
uint32_t HWComposerGop::getDestination() {
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

hwc2_error_t HWComposerGop::getCurOPTiming(int *ret_width, int *ret_height, int *ret_hstart) {
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
hwc2_error_t HWComposerGop::getCurOCTiming(int *ret_width, int *ret_height, int *ret_hstart) {
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
    return HWC2_ERROR_NONE;
}
uint32_t HWComposerGop::getBitMask(unsigned int gop) {
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

hwc2_error_t HWComposerGop::setInterlave(int gop, int bInterlave) {
    DLOGI("gfx_GOPSetInterlave was invoked bInterlave=%d",bInterlave);
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
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWComposerGop::setStretchWindow(unsigned char gop,  unsigned short srcWidth, unsigned short srcHeight,
                          unsigned short dstX, unsigned short dstY, unsigned short dstWidth, unsigned short dstHeight) {
    MS_U8 gopNum = MApi_GOP_GWIN_GetMaxGOPNum();
    if (gop >= gopNum) {
        DLOGE("GOP_SetStretchWindow was invoked invalid gop=%d",gop);
        return HWC2_ERROR_NOT_VALIDATED;
    }
    /* GOP stretch setting */
    MApi_GOP_GWIN_Set_HSCALE_EX(gop,TRUE, srcWidth, dstWidth);
    MApi_GOP_GWIN_Set_VSCALE_EX(gop,TRUE, srcHeight, dstHeight);

    // set the stretchwindow scale mode
    MApi_GOP_GWIN_Set_VStretchMode_EX(gop, E_GOP_VSTRCH_LINEAR_GAIN2);
    MApi_GOP_GWIN_Set_HStretchMode_EX(gop, E_GOP_HSTRCH_6TAPE_LINEAR);

    MApi_GOP_GWIN_Set_STRETCHWIN(gop, E_GOP_DST_OP0, dstX, dstY, srcWidth, srcHeight);
    return HWC2_ERROR_NONE;
}

#ifdef MALI_AFBC_GRALLOC
hwc2_error_t HWComposerGop::resetAFBC(std::shared_ptr<HWWindowInfo>& hwwindowinfo) {
    MS_BOOL plist = TRUE;
    MApi_GOP_SetConfigEx(hwwindowinfo->gop,E_GOP_AFBC_RESET,&plist);
    return HWC2_ERROR_NONE;
}
#endif

hwc2_error_t HWComposerGop::reCreateFBInfo(int osdMode,
    std::shared_ptr<HWWindowInfo>& hwwindowinfo,std::shared_ptr<HWWindowInfo>& lasthwwindowinfo) {
    char property[PROPERTY_VALUE_MAX] = {0};
    int bResizeBufferEnable = 0;
    if (property_get("mstar.resize.framebuffer", property, "0") > 0) {
        bResizeBufferEnable = atoi(property);
    }
    int pitch = hwwindowinfo->stride;
    MS_U32 U32MainFbid = 0;
    MS_U32 U32SubFbid = 0;
    EN_GOP_3D_MODETYPE mode = E_GOP_3D_DISABLE;
    hwwindowinfo->mainFBphysicalAddr = hwwindowinfo->physicalAddr + hwwindowinfo->srcY * pitch +
                             hwwindowinfo->srcX * changeHalPixelFormat2bpp(hwwindowinfo->format);
    EN_GOP_FRAMEBUFFER_STRING FBString = E_GOP_FB_NULL;
#ifdef MALI_AFBC_GRALLOC
    if (hwwindowinfo->bSupportGopAFBC) {
        if (hwwindowinfo->bEnableGopAFBC) {
            //enable AFBC
            FBString = E_GOP_FB_AFBC_NONSPLT_YUVTRS_ARGB8888;
        } else {
            //disable AFBC
            FBString = E_GOP_FB_NULL;
        }
    }
    DLOGD("GOP[%d]:AFBC MODE FBString = %d",hwwindowinfo->gop,FBString);
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
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                    hwwindowinfo->bufferHeight>>1,
                                                    changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                    hwwindowinfo->mainFBphysicalAddr,
                                                    pitch,
                                                    &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight>>1,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                MApi_GOP_GWIN_GetFBInfo(U32MainFbid,&fbInfo);
                MS_PHY fb2_addr = fbInfo.addr + fbInfo.pitch * fbInfo.height;
                hwwindowinfo->subFBphysicalAddr = fb2_addr;
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight>>1,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        fb2_addr,
                                                        pitch,
                                                        &U32SubFbid);
                } else {
                    U32SubFbid = MApi_GOP_GWIN_GetFree32FBID();
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32SubFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight>>1,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
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
                    MS_U32 u32SubAddrOffset = (hwwindowinfo->srcWidth>>1)*u8BytePerPixel;
                    if ((u32SubAddrOffset%u32WordUnit) != 0) {
                        DLOGE("Attention! Wrong H resolution[%u] for 3D output, fmt=%u\n", hwwindowinfo->bufferWidth,
                              changeHalPixelFormat2MsColorFormat(hwwindowinfo->format));
                        return HWC2_ERROR_NOT_VALIDATED; //Wrong res, must obey GOP address alignment rule(1 Word)
                    }
                    if (!bResizeBufferEnable) {
                        MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth>>1,
                                                            hwwindowinfo->bufferHeight,
                                                            changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                            hwwindowinfo->mainFBphysicalAddr,
                                                            pitch,
                                                            &U32MainFbid);
                    } else {
                        MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth>>1,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                    }
                    MApi_GOP_GWIN_GetFBInfo(U32MainFbid,&fbInfo);
                    MS_PHY fb2_addr = fbInfo.addr + u32SubAddrOffset;
                    hwwindowinfo->subFBphysicalAddr = fb2_addr;
                    if (!bResizeBufferEnable) {
                        MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth>>1,
                                                            hwwindowinfo->bufferHeight,
                                                            changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                            fb2_addr,
                                                            pitch,
                                                            &U32SubFbid);
                    } else {
                        U32SubFbid = MApi_GOP_GWIN_GetFree32FBID();
                        MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32SubFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth>>1,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        fb2_addr,
                                                        FBString);
                    }
                    mode = E_GOP_3D_SWITH_BY_FRAME;
                }
            } break;
        case DISPLAYMODE_NORMAL_FR:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                U32SubFbid = U32MainFbid;
                hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
                mode = E_GOP_3D_SWITH_BY_FRAME;
            } break;
        case DISPLAYMODE_NORMAL_FP:
            {
                if ((hwwindowinfo->srcWidth == 1920 && hwwindowinfo->srcHeight == 1080) ||
                    (hwwindowinfo->srcWidth == 1280 && hwwindowinfo->srcHeight == 720)) {
                    if (!bResizeBufferEnable) {
                        MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                            hwwindowinfo->bufferHeight,
                                                            changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                            hwwindowinfo->mainFBphysicalAddr,
                                                            pitch,
                                                            &U32MainFbid);
                    } else {
                        MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                    }
                    U32SubFbid = U32MainFbid;
                    hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
                    mode = E_GOP_3D_FRAMEPACKING;
                } else {
                    if (lasthwwindowinfo->fbid != INVALID_FBID) {
                        hwwindowinfo->fbid =lasthwwindowinfo->fbid;
                    }
                    if (lasthwwindowinfo->subfbid != INVALID_FBID) {
                        hwwindowinfo->subfbid =lasthwwindowinfo->subfbid;
                    }
                    return HWC2_ERROR_NONE;
                }
            } break;
        case DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_TOP_BOTTOM;
            } break;
        case DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_LINE_ALTERNATIVE;
            } break;
        case DISPLAYMODE_LEFTRIGHT_FULL:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
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
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_DISABLE;
            } break;
        case DISPLAYMODE_FRAME_ALTERNATIVE_LLRR:
            {
                if (!bResizeBufferEnable) {
                    MApi_GOP_GWIN_Create32FBFrom3rdSurf(hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight>>1,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->physicalAddr,
                                                        pitch,
                                                        &U32MainFbid);
                } else {
                    MApi_GOP_GWIN_Create32FBbyStaticAddr2(U32MainFbid,
                                                        0,0,
                                                        hwwindowinfo->bufferWidth,
                                                        hwwindowinfo->bufferHeight>>1,
                                                        changeHalPixelFormat2MsColorFormat(hwwindowinfo->format),
                                                        hwwindowinfo->mainFBphysicalAddr,
                                                        FBString);
                }
                hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
                U32SubFbid = U32MainFbid;
                mode = E_GOP_3D_DISABLE;
            } break;
    }
    hwwindowinfo->fbid = U32MainFbid;
    hwwindowinfo->subfbid = U32SubFbid;
    int status= 1;
    status = MApi_GOP_GWIN_Map32FB2Win(hwwindowinfo->fbid, hwwindowinfo->gwin);
    if (status != GOP_API_SUCCESS) {
        DLOGD("MApi_GOP_GWIN_Map32FB2Win failed gop %d gwin %d",hwwindowinfo->gop,hwwindowinfo->gwin);
    }

    if (lasthwwindowinfo->subfbid != INVALID_FBID) {
        int status = MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo->subfbid);
        if (status != GOP_API_SUCCESS) {
            DLOGD("MApi_GOP_GWIN_Delete32FB sub failed");
        }
        if (lasthwwindowinfo->subfbid == lasthwwindowinfo->fbid) {
            lasthwwindowinfo->subfbid = INVALID_FBID;
            lasthwwindowinfo->fbid = INVALID_FBID;
        } else {
            lasthwwindowinfo->subfbid = INVALID_FBID;
        }
    }

    if (lasthwwindowinfo->fbid != INVALID_FBID) {
        int status = MApi_GOP_GWIN_Delete32FB(lasthwwindowinfo->fbid);
        if (status != GOP_API_SUCCESS) {
            DLOGD("MApi_GOP_GWIN_Delete32FB failed");
        }
        lasthwwindowinfo->fbid = INVALID_FBID;
    }
    MApi_GOP_Set3DOSDMode(hwwindowinfo->gwin, hwwindowinfo->fbid, hwwindowinfo->subfbid,mode);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWComposerGop::updateGWinInfo(int osdMode,std::shared_ptr<HWWindowInfo>& hwwindowinfo,
                                    std::shared_ptr<HWWindowInfo>& lasthwwindowinfo) {
    static int old3DMode = DISPLAYMODE_NORMAL;
    int pitch = hwwindowinfo->stride;
    MS_U16 srcWidth = hwwindowinfo->srcWidth;
    MS_U16 srcHeight = hwwindowinfo->srcHeight;
    MS_U16 dstX = hwwindowinfo->dstX;
    MS_U16 dstY = hwwindowinfo->dstY;
    MS_U16 dstWidth = hwwindowinfo->dstWidth;
    MS_U16 dstHeight = hwwindowinfo->dstHeight;
    MS_PHY fb2_addr = 0;
    GOP_GwinInfo pinfo;
    GOP_GwinFBAttr fbInfo;
    static MS_BOOL mirrorModeH, mirrorModeV;
    static int enableHwcursor = 0;
    memset(&pinfo,0,sizeof(pinfo));
    memset(&fbInfo,0,sizeof(fbInfo));
    MApi_GOP_GWIN_GetWinInfo(hwwindowinfo->gwin, &pinfo);
    pinfo.u16DispVPixelStart = 0;
    pinfo.u16DispVPixelEnd = hwwindowinfo->srcHeight;
    pinfo.u16DispHPixelStart = 0;
    // the gwin width must be 2 pixel alignment
    pinfo.u16DispHPixelEnd = hwwindowinfo->srcWidth &(~0x1);
    pinfo.u32DRAMRBlkStart = hwwindowinfo->physicalAddr + hwwindowinfo->srcY * pitch +
                             hwwindowinfo->srcX * changeHalPixelFormat2bpp(hwwindowinfo->format);
    hwwindowinfo->mainFBphysicalAddr = pinfo.u32DRAMRBlkStart;
    switch (osdMode) {
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_BOTTOM_LA:
            pinfo.u16DispHPixelEnd = hwwindowinfo->srcWidth;
            pinfo.u16DispVPixelEnd = hwwindowinfo->srcHeight>>1;
            srcHeight = hwwindowinfo->srcHeight>>1;
            dstHeight = hwwindowinfo->dstHeight>>1;
            MApi_GOP_GWIN_Get32FBInfo(hwwindowinfo->fbid,&fbInfo);
            fb2_addr = hwwindowinfo->mainFBphysicalAddr + fbInfo.pitch * fbInfo.height;
            hwwindowinfo->subFBphysicalAddr = fb2_addr;
            break;
        case DISPLAYMODE_LEFTRIGHT_FR:
            pinfo.u16DispHPixelEnd = hwwindowinfo->srcWidth>>1;
            pinfo.u16DispVPixelEnd = hwwindowinfo->srcHeight;
            srcWidth = hwwindowinfo->srcWidth>>1;
            fb2_addr = hwwindowinfo->mainFBphysicalAddr + srcWidth*changeHalPixelFormat2bpp(hwwindowinfo->format);;
            hwwindowinfo->subFBphysicalAddr = fb2_addr;
            break;
        case DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY:
            dstHeight = hwwindowinfo->dstHeight>>1;
            hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
            break;
        case DISPLAYMODE_TOPBOTTOM_LA_HIGH_QUALITY:
            dstHeight = hwwindowinfo->dstHeight>>1;
            hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
            break;
        case DISPLAYMODE_LEFTRIGHT_FULL:
            dstWidth = hwwindowinfo->dstWidth>>1;
            hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
            break;
        case DISPLAYMODE_NORMAL_FP:
            if ((hwwindowinfo->srcWidth==1920)&&(hwwindowinfo->srcHeight==1080)) {
                dstWidth = 1920;
                dstHeight = 1080;
            } else if ((hwwindowinfo->srcWidth==1280)&&(hwwindowinfo->srcHeight==720)) {
                //only support 1920*1080 framepacket, if 720p framebuffer, scale to 1080p
                //through stretchwindow
                dstWidth = 1920;
                dstHeight = 1080;
            }
            hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
            break;
        case DISPLAYMODE_FRAME_ALTERNATIVE_LLRR:
            srcHeight = hwwindowinfo->srcHeight>>1;
            hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
            break;
        case DISPLAYMODE_NORMAL:
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        case DISPLAYMODE_RIGHT_ONLY:
        case DISPLAYMODE_NORMAL_FR:
        default:
            hwwindowinfo->subFBphysicalAddr = hwwindowinfo->mainFBphysicalAddr;
            break;
    }
    if (lasthwwindowinfo->srcWidth != hwwindowinfo->srcWidth ||
        lasthwwindowinfo->srcHeight != hwwindowinfo->srcHeight ||
        lasthwwindowinfo->dstX != hwwindowinfo->dstX ||
        lasthwwindowinfo->dstY != hwwindowinfo->dstY ||
        lasthwwindowinfo->dstWidth != hwwindowinfo->dstWidth ||
        lasthwwindowinfo->dstHeight != hwwindowinfo->dstHeight ||
        old3DMode != osdMode) {
        setStretchWindow(hwwindowinfo->gop,srcWidth,srcHeight,dstX, dstY,dstWidth,dstHeight);
        //in DISPLAYMODE_FRAME_ALTERNATIVE_LLRR mode, video Vertical Mirroreyd, so does OSD.
        //and hwcursor should be disabled
        if ((osdMode == DISPLAYMODE_FRAME_ALTERNATIVE_LLRR) && (old3DMode != osdMode)) {
            char property[PROPERTY_VALUE_MAX] = {0};
            MApi_GOP_GWIN_IsMirrorOn_EX(hwwindowinfo->gop, &mirrorModeH, &mirrorModeV);
            MApi_GOP_GWIN_SetVMirror_EX(hwwindowinfo->gop, true);
            if (property_get("mstar.desk-enable-hwcursor", property, "1") > 0) {
                enableHwcursor = atoi(property);
            }
            property_set("mstar.desk-enable-hwcursor", "0");
        } else if ((old3DMode == DISPLAYMODE_FRAME_ALTERNATIVE_LLRR) && (old3DMode != osdMode)) {
            MApi_GOP_GWIN_SetVMirror_EX(hwwindowinfo->gop, mirrorModeV);
            if(enableHwcursor) {
                property_set("mstar.desk-enable-hwcursor", "1" );
            }
        }
        old3DMode = osdMode;
    }
    //MApi_GOP_GWIN_SetForceWrite(TRUE);
#ifdef MALI_AFBC_GRALLOC
    if (hwwindowinfo->bEnableGopAFBC) {
        pinfo.u16DispHPixelEnd = ALIGNMENT(pinfo.u16DispHPixelEnd,16);
        pinfo.u16DispVPixelEnd = ALIGNMENT(pinfo.u16DispVPixelEnd,16);
    }
#endif
    MApi_GOP_GWIN_SetWinInfo(hwwindowinfo->gwin, &pinfo);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWComposerGop::close_boot_logo(std::shared_ptr<HWWindowInfo>& hwwindowinfo) {
    char property[PROPERTY_VALUE_MAX] = {0};
    if (!bGopHasInit.at(0)) return HWC2_ERROR_NOT_VALIDATED;
    if ((!mDisplay.mBootlogo.bGopClosed)&&(property_get("mstar.close.bootlogo.gop.frame", property, NULL) > 0)) {
        mDisplay.mBootlogo.closeThreshold = atoi(property);
        mDisplay.mBootlogo.frameCountAfterBoot++;
        if (mDisplay.mBootlogo.frameCountAfterBoot >= mDisplay.mBootlogo.closeThreshold) {
            unsigned int gwin = getAvailGwinId(mDisplay.mBootlogo.gop);
            DLOGD("bootlogogop = %d ,gwin = %d, request frame = %d, current frame = %d",mDisplay.mBootlogo.gop,gwin,mDisplay.mBootlogo.closeThreshold,mDisplay.mBootlogo.frameCountAfterBoot);
            if (MApi_GOP_GWIN_IsGWINEnabled(gwin) && gwin != hwwindowinfo->gwin) {
                MApi_GOP_GWIN_UpdateRegOnceByMask(getBitMask(mDisplay.mBootlogo.gop),TRUE,FALSE);
                MApi_GOP_GWIN_Enable(gwin, false);
                MApi_GOP_GWIN_UpdateRegOnceByMask(getBitMask(mDisplay.mBootlogo.gop),FALSE,FALSE);
                DLOGD("close bootlogo gop sucess!!!!!!");
                mDisplay.mBootlogo.frameCountAfterBoot = 0;
                mDisplay.mBootlogo.bGopClosed = true;
            } else {
                DLOGD("bootlogo gop has closed by other process or using same gwin with An!!!!!!");
                mDisplay.mBootlogo.frameCountAfterBoot = 0;
                mDisplay.mBootlogo.bGopClosed = true;
            }
        }
     }
    return HWC2_ERROR_NONE;
}
uint32_t HWComposerGop::getLayerFromGOP(uint32_t gop) {
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

int HWComposerGop::changeHalPixelFormat2MsColorFormat(int inFormat) {
    switch (inFormat) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return E_MS_FMT_ABGR8888;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return E_MS_FMT_ABGR8888;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return E_MS_FMT_ARGB8888;
        case HAL_PIXEL_FORMAT_RGB_565:
            return E_MS_FMT_RGB565;
        default:
            DLOGE("changeHalPixelFormat2MsColorFormat failed");
            return E_MS_FMT_GENERIC;
    }
}

int HWComposerGop::changeHalPixelFormat2bpp(int inFormat) {
    switch (inFormat) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return 4;
        case HAL_PIXEL_FORMAT_RGB_565:
            return 2;
        default:
            DLOGE("changeHalPixelFormat2bpp failed");
            return 0;
    }
}
