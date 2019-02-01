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

#include "MsTypesEx.h"
#include "drvXC_IOPort.h"
#include "apiXC.h"
#include "dynamic_scale.h"

#if DS_compiler
#include <stdint.h>
#include <stdio.h>

/////////////////////////////////////////////////////////
// local defines, structures
/////////////////////////////////////////////////////////

#define DS_DEBUG_PRT 0

#if DS_DEBUG_PRT
    #define DS_MSG(_fmt, _args...) printf(_fmt, ##_args)
#else
//#define DS_MSG(_fmt, _args...)
    #include <cutils/log.h>

    #define DS_MSG ALOGD
#endif

#define H_PreScalingDownRatio(Input, Output)     ( (((Output)) * 1048576ul)/ (Input) + 1 )
#define H_PreScalingDownRatioAdv(Input, Output)  ( (((Input)-1) * 1048576ul)/ ((Output)-1) ) //Advance scaling
#define V_PreScalingDownRatio(Input, Output)     ( (((Output)) * 1048576ul)/ (Input) + 1 )

#define H_PostScalingRatio(Input, Output)        ( ((Input)) * 1048576ul / (Output) + 1 )
#define V_PostScalingRatio(Input, Output)        ( ((Input)-1) * 1048576ul / ((Output)-1) + 1 )

typedef enum {
    MM_DS_XC_CMD_UPDATE_ZOOM_INFO = 1,
    MM_DS_XC_CMD_UPDATE_XC_INFO = 2,
} MM_DS_XC_CMD;

/////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////
static char bFirstEntry = 0; // this will be inited in hvd__dynamic_scale_init()

/////////////////////////////////////////////////////////
// local utilities
/////////////////////////////////////////////////////////
static void ds_memset(void *dstvoid, char val, int length)
{
    char *dst = dstvoid;

    while (length--)
        *dst++ = val;
}

static void *ds_memcpy (void *__restrict dstvoid,
                        __const void *__restrict srcvoid, unsigned int length)
{
    char *dst = dstvoid;
    const char *src = (const char *) srcvoid;

    while (length--)
        *dst++ = *src++;
    return dst;
}

void dynamic_scale_init(ds_init_structure *pDS_Init_Info)
{
    ds_internal_data_structure *pDSIntData;

    pDSIntData = pDS_Init_Info->pDSIntData;

    // clear DS buffer
    ds_memset(pDS_Init_Info->pDSBufBase, 0xFF, pDS_Init_Info->DSBufSize);

    // decide memory alignement based on chip ID
    pDSIntData->enChipID =  pDS_Init_Info->ChipID;
    switch (pDS_Init_Info->ChipID) {
    case DS_T3:
    case DS_T8:
    case DS_T9:
    case DS_T12:
    case DS_T13:
    case DS_J2:
    case DS_A1:
    case DS_A2:
    case DS_A6:
    case DS_A7:
    case DS_AMETHYST:
    case DS_EAGLE:
    case DS_EMERALD:
    case DS_NIKE:
    case DS_MADISON:
        pDSIntData->MemAlign = 16;
        break;

    case DS_A3:
    case DS_A5:
    case DS_AGATE:
    case DS_EDISON:
    case DS_CEDRIC:
    case DS_KAISER:
    case DS_EINSTEIN:
    case DS_NAPOLI:
    case DS_MONACO:
    case DS_CLIPPERS:
    case DS_MUJI:
        pDSIntData->MemAlign = 32;
        break;

    case DS_Euclid:
    case DS_T4:
    case DS_T7:
    case DS_MARIA10:
    case DS_K1:
    case DS_K2:
    default:
        pDSIntData->MemAlign = 8;
        break;
    }

    pDSIntData->OffsetAlign = pDSIntData->MemAlign * 2;     // for calculate crop
    pDSIntData->DS_Depth = pDS_Init_Info->DS_Depth;
    pDSIntData->u8EnhanceMode = pDS_Init_Info->u8EnhanceMode;

    pDSIntData->MaxWidth = pDS_Init_Info->MaxWidth;
    pDSIntData->MaxHeight = pDS_Init_Info->MaxHeight;
    pDSIntData->pDSBufBase = pDS_Init_Info->pDSBufBase;
    pDSIntData->DSBufSize = pDS_Init_Info->DSBufSize;

    pDSIntData->pDSXCData = (ds_xc_data_structure *) pDS_Init_Info->pDSXCData;
    pDSIntData->pDSXCData->u8DSVersion = DS_VERSION;
    pDSIntData->pDSXCData->bFWGotXCInfo = 0;
    pDSIntData->pDSXCData->bFWGotNewSetting = 0;
    pDSIntData->pDSXCData->u8XCInfoUpdateIdx = 0;
    pDSIntData->pDSXCData->bFWIsUpdating = 0;
    pDSIntData->pDSXCData->bHKIsUpdating = 0;

    bFirstEntry = 1;
    pDSIntData->bIsInitialized = 1;
}

// addr is 16bit address, reg_value contains 16bits data
void ds_write_2bytes(ds_internal_data_structure *pDSIntData, ds_reg_ip_op_sel IPOP_Sel, unsigned int bank, unsigned int addr, unsigned int reg_value)
{
    unsigned int pntJump;

    pntJump = pDSIntData->MemAlign / sizeof(ds_reg_setting_structure);

    if (IPOP_Sel == DS_IP) {
        pDSIntData->pIPRegSetting->bank = bank;
        pDSIntData->pIPRegSetting->addr = addr;
        pDSIntData->pIPRegSetting->reg_value = reg_value;
        pDSIntData->pIPRegSetting += pntJump;

        pDSIntData->IPRegMaxDepth++;
    } else {

        pDSIntData->pOPRegSetting->bank = bank;
        pDSIntData->pOPRegSetting->addr = addr;
        pDSIntData->pOPRegSetting->reg_value = reg_value;
        pDSIntData->pOPRegSetting += pntJump;

        pDSIntData->OPRegMaxDepth++;
    }
}

// addr is 16bit address, reg_value contains 16bits data
void ds_write_4bytes(ds_internal_data_structure *pDSIntData, ds_reg_ip_op_sel IPOP_Sel, unsigned int bank, unsigned int addr, unsigned int reg_value)
{
    ds_write_2bytes(pDSIntData, IPOP_Sel, bank, addr, reg_value & 0xFFFF);
    ds_write_2bytes(pDSIntData, IPOP_Sel, bank, addr+1, (reg_value & 0xFFFF0000) >> 16);
}

static void ds_write_buf_flush(ds_internal_data_structure *pDSIntData)
{
    while (pDSIntData->IPRegMaxDepth < pDSIntData->DS_Depth) {
        ds_write_2bytes(pDSIntData, DS_IP, 0xFF, 0x01, 0x0000);
    }

    while (pDSIntData->OPRegMaxDepth < pDSIntData->DS_Depth) {
        ds_write_2bytes(pDSIntData, DS_OP, 0xFF, 0x01, 0x0000);
    }
}

void dynamic_scale(ds_recal_structure *pDS_Info)
{
    ds_internal_data_structure *pDSIntData = NULL;
    ds_xc_data_structure *pDSXCData = NULL;
    MS_WINDOW_TYPE stDSCropWin;
    MS_WINDOW_TYPE stDSScaledCropWin;
    MS_U16 u16HSizeAfterPreScaling;
    MS_U16 u16VSizeAfterPreScaling;
    MS_U16 u16LineBufOffset;
    MS_U16 u16OPMFetch=0,u16IPMFetch;
    MS_U16 u16H_mirrorOffset = 0;
    MS_U32 u32CropOffset;
    MS_U32 u32PixelOffset;
    MS_U8 u8Framenumber;
    MS_BOOL bInterlace;
    MS_BOOL bResolutionChanging;
    MS_BOOL bBobMode;
    static MS_U16 _FirstCodedWidth = 1920;
    static MS_U16 _FirstCodedHeight = 1080;

    if ((pDSIntData == NULL) || (pDSXCData == NULL) || (pDS_Info == NULL)) {
        return;
    }

    pDSIntData = pDS_Info->pDSIntData;
    pDSXCData = pDSIntData->pDSXCData;
    u8Framenumber = pDSXCData->u8StoreFrameNum;
    bInterlace = pDSXCData->bInterlace;
    bResolutionChanging = pDSXCData->bResolutionChanging;
    bBobMode = pDSXCData->u8BobMode;

    DS_MSG("pDSBufBase(%p), DSBufIdx(%d)\n", pDSIntData->pDSBufBase, pDS_Info->DSBufIdx);

    if ((pDSXCData->stDispWin.width == 0) || (pDSXCData->stDispWin.height == 0)) {
        return;
    }
    // decide DS buf start point
    pDSIntData->pCurDSBuf = (ds_reg_setting_structure *)
                            (pDSIntData->pDSBufBase + (pDS_Info->DSBufIdx * (pDSIntData->MemAlign * pDSIntData->DS_Depth)));

    // decide OP/IP reg setting start point
    pDSIntData->pOPRegSetting = pDSIntData->pCurDSBuf;
    pDSIntData->pIPRegSetting = pDSIntData->pCurDSBuf + 1;
    pDSIntData->OPRegMaxDepth = 0;
    pDSIntData->IPRegMaxDepth = 0;
    DS_MSG("u8StoreFrameNum(%d)\n", pDSXCData->u8StoreFrameNum);
    DS_MSG("bFWGotXCInfo(%d) ,u8XCInfoUpdateIdx(%d), bFWGotNewSetting(%d)\n", pDSXCData->bFWGotXCInfo, pDSXCData->u8XCInfoUpdateIdx, pDSXCData->bFWGotNewSetting);


    pDSXCData->bFWIsUpdating = 1;
    // no need to clear DS buffer here because all registers are the same except value
    if (pDSXCData->bFWGotXCInfo) {

        if ((pDSIntData->u8EnhanceMode != DS_ENHANCE_MODE_RESET_DISP_WIN) && (bFirstEntry == 1)) {
            _FirstCodedWidth  = pDS_Info->CodedWidth;
            _FirstCodedHeight = pDS_Info->CodedHeight;
            bFirstEntry = 0;
        }

        if (pDSXCData->bFWGotNewSetting) {
            ds_memcpy(&pDSXCData->stCropWin, &pDSXCData->stNewCropWin, sizeof(MS_WINDOW_TYPE));
            ds_memcpy(&pDSXCData->stDispWin, &pDSXCData->stNewDispWin, sizeof(MS_WINDOW_TYPE));
            pDSXCData->bFWGotNewSetting = 0;
        }

        DS_MSG("Code[%d,%d]\n", pDS_Info->CodedWidth, pDS_Info->CodedHeight);
        DS_MSG("Max[%d,%d]\n",  pDSIntData->MaxWidth, pDSIntData->MaxHeight);
        DS_MSG("Cap[%d,%d]\n",  pDSXCData->stCapWin.x, pDSXCData->stCapWin.y);
        DS_MSG("Cap[%d,%d]\n",  pDSXCData->stCapWin.width, pDSXCData->stCapWin.height);
        DS_MSG("Crop[%d,%d]\n", pDSXCData->stCropWin.x , pDSXCData->stCropWin.y);
        DS_MSG("Crop[%d,%d]\n", pDSXCData->stCropWin.width, pDSXCData->stCropWin.height);
        DS_MSG("Disp[%d,%d]\n", pDSXCData->stDispWin.x, pDSXCData->stDispWin.y);
        DS_MSG("Disp[%d,%d]\n", pDSXCData->stDispWin.width, pDSXCData->stDispWin.height);

        //ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x02, 0x47BB);
        if ( pDSIntData->enChipID == DS_T8 ||  pDSIntData->enChipID == DS_T9 ||
             pDSIntData->enChipID == DS_T12 || pDSIntData->enChipID == DS_T13 ||
             pDSIntData->enChipID == DS_MARIA10 ) {
            ds_write_2bytes(pDSIntData, DS_IP, 0x1F, 0x14, 0x0000);
            ds_write_2bytes(pDSIntData, DS_OP, 0x1F, 0x14, 0x0000);
        }

        ///////////////////////////////////////////////////////
        // set BOB/3DDI mode for interlace resolution changes
        ///////////////////////////////////////////////////////
        if (bInterlace) {
            if (bResolutionChanging || bBobMode) {
                ds_write_2bytes(pDSIntData, DS_IP, 0x22, 0x78, 0x8F8F);
            } else {
                ds_write_2bytes(pDSIntData, DS_IP, 0x22, 0x78, 0x0000);
            }
        }

        ///////////////////////////////////////////////////////
        // Handle Capture Window HStart/VStart for MVOP Mirror mode
        ///////////////////////////////////////////////////////
        if (pDSXCData->bMirrorMode == 0) {  // SEC use XC mirror while some China project use MVOP mirror
            if ( (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_HORIZONTALL) || (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_HVBOTH) ) {
                ds_write_2bytes(pDSIntData, DS_IP, 0x01, 0x05, pDSXCData->u16MVOPHStart + pDS_Info->CropRight);
            } else if ( (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_VERTICAL) || (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_NONE) ) {
                //ds_write_2bytes(pDSIntData, DS_IP, 0x01, 0x05, pDSXCData->u16MVOPHStart + pDS_Info->CropLeft);  // need refinement if there is CropLeft
            }

            if ( (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_VERTICAL) || (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_HVBOTH) ) {
                ////  Scaler HW Limitation:
                ////    Scaler timing is generated according to Capture Window VStart. If it is changed, it would lead to temporal glitch.
                ////    Thus in DS, we should not change Capture Window VStart.
                ////    So VDEC FW does not make MVOP transport crop_bottom part to Scaler.
                //ds_write_2bytes(pDSIntData, DS_IP, 0x01, 0x04, pDSXCData->u16MVOPVStart);    // Omit always writing the same value
            } else if ( (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_HORIZONTALL) || (pDSXCData->u8MVOPMirror==(MS_U8)E_MVOP_MIRROR_NONE) ) {
                //ds_write_2bytes(pDSIntData, DS_IP, 0x01, 0x04, pDSXCData->u16MVOPVStart);   // Omit always writing the same value
                // need refinement if there is CropTop
            }
        }

        ///////////////////////////////////////////////////////
        // set IP capture & pre-scaling, DNR fetch/offset
        ///////////////////////////////////////////////////////
        ds_write_2bytes(pDSIntData, DS_IP, 0x01, 0x07, pDS_Info->CodedWidth);
        ds_write_2bytes(pDSIntData, DS_IP, 0x01, 0x06, pDS_Info->CodedHeight);
        if ( (pDSIntData->enChipID != DS_MARIA10) && (pDSIntData->enChipID != DS_J2) &&
             (pDSIntData->enChipID != DS_A3)  && (pDSIntData->enChipID != DS_A5) &&
             (pDSIntData->enChipID != DS_AGATE) && (pDSIntData->enChipID != DS_EDISON) &&
             (pDSIntData->enChipID != DS_EAGLE) && (pDSIntData->enChipID != DS_EMERALD) &&
             (pDSIntData->enChipID != DS_CEDRIC) && (pDSIntData->enChipID != DS_KAISER) &&
             (pDSIntData->enChipID != DS_NIKE) && (pDSIntData->enChipID != DS_EINSTEIN) &&
             (pDSIntData->enChipID != DS_NAPOLI) && (pDSIntData->enChipID != DS_MADISON) &&
             (pDSIntData->enChipID != DS_MONACO) && (pDSIntData->enChipID != DS_CLIPPERS) &&
             (pDSIntData->enChipID != DS_MUJI)
           ) {
            ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x18, 0x1000 | pDS_Info->CodedHeight);
        } else if(pDSIntData->enChipID == DS_CLIPPERS){
            //Per HW, because of MVOP send 1080 line to XC, XC can not set line limit smaller than 1080,
            //Use pDS_Info->CodedHeight will cause left-top have garbage point.
            //(XC line limit must equal to MVOP send line)
            //HW says in FB mode (buffer is 1920x1080) always set line limit 1080 is OK
            ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x18, 0x8000 | pDSIntData->MaxHeight);
        } else {
            ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x18, 0x8000 | pDS_Info->CodedHeight);
        }

        if(pDSIntData->enChipID == DS_KAISER)
        {
            // If scaling down to less than 80% of original height, force V pre-scaling
            if(((double)pDSXCData->stDispWin.height / pDS_Info->CodedHeight) < 0.8)
            {
                if(bInterlace)
                {
                    u16VSizeAfterPreScaling = (pDSXCData->stDispWin.height+1) & ~0x01;
                }
                else
                {
                    u16VSizeAfterPreScaling = pDSXCData->stDispWin.height;
                }

                ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x80000000 | V_PreScalingDownRatio(pDS_Info->CodedHeight, u16VSizeAfterPreScaling));

                // Need to force H pre-scaling if already V pre-scaling
                // Force H pre-scaling down to less than 1280px
                if(pDSXCData->stDispWin.width > 1280)
                {
                    u16HSizeAfterPreScaling = 1280;
                    if(pDS_Info->CodedWidth != u16HSizeAfterPreScaling)
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x80000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
                    else
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x00000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
                }
                else
                {
                    if(pDS_Info->CodedWidth > pDSXCData->stDispWin.width)
                    {
                        u16HSizeAfterPreScaling = pDSXCData->stDispWin.width;
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x80000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
                    }
                    else
                    {
                        u16HSizeAfterPreScaling = pDS_Info->CodedWidth;
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x00000000);
                    }
                }


            }
            else
            {
                u16VSizeAfterPreScaling = pDS_Info->CodedHeight;
                ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x00000000);

                // If disp win width is less than 1280px, force H pre-scaling
                if((pDSXCData->stDispWin.width <= 1280) && (pDS_Info->CodedWidth > pDSXCData->stDispWin.width))
                {
                    u16HSizeAfterPreScaling = pDSXCData->stDispWin.width;
                    ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x80000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
                }
                else
                {
                    u16HSizeAfterPreScaling = pDS_Info->CodedWidth;
                    ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x00000000);
                }
            }
        } else {
            ////////////////////////////////////
            // Horizontal prescaling calculation
            ////////////////////////////////////
            u16HSizeAfterPreScaling = (pDS_Info->CodedWidth +1) & ~0x01;
            DS_MSG("pDSXCData->u32PNL_Width[%d]\n", pDSXCData->u32PNL_Width);
            if (pDSXCData->bSaveBandwidthMode == 1) {
                u16HSizeAfterPreScaling = (pDS_Info->CodedWidth/2 +1) & ~0x01;
                // pre-scaling down
                ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x80000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
            } else if (pDSXCData->u32PNL_Width == 3840) {
                if ((pDS_Info->CodedWidth*9) > (pDSXCData->stDispWin.width*10)) {
                    u16HSizeAfterPreScaling = (pDS_Info->CodedWidth/2 +1) & ~0x01;
                    ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x80000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
                } else {
                    ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x00000000);
                }
            } else {
                if ((pDS_Info->CodedWidth*6) > (pDSXCData->stDispWin.width*10)) {
                    u16HSizeAfterPreScaling = (pDS_Info->CodedWidth/2 +1) & ~0x01;
                    // pre-scaling down
                    ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x80000000 | H_PreScalingDownRatio(pDS_Info->CodedWidth, u16HSizeAfterPreScaling));
                } else {
                    // pre-scaling down
                    ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x04, 0x00000000);
                }
            }

            ////////////////////////////////////
            // Vertical prescaling calculation
            ////////////////////////////////////

            if ((pDSIntData->enChipID == DS_EDISON) || (pDSIntData->enChipID == DS_EINSTEIN) ||
                (pDSIntData->enChipID == DS_NAPOLI) || (pDSIntData->enChipID == DS_MONACO) ||
                (pDSIntData->enChipID == DS_MUJI)) {
                    if (pDSXCData->bSaveBandwidthMode == 1) {
                        u16VSizeAfterPreScaling = (pDS_Info->CodedHeight/2 +1) & ~0x01;
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x80000000 | V_PreScalingDownRatio(pDS_Info->CodedHeight, u16VSizeAfterPreScaling));
                    } else {
                        if ((pDSXCData->stDispWin.height * 10) < (pDSXCData->stCropWin.height* 6)) {
                            if (bInterlace) {
                                // do not write like
                                // ex: u16VSizeAfterPreScaling = (pDS_Info->CodedHeight -1 + 1) & ~0x01;
                                // it may cause the prescaling down becoming NO prescaling down
                                // for example :
                                //              pDS_Info->CodedHeight is 1080
                                //              pDS_Info->CodedHeight -1 is 1079
                                //              pDS_Info->CodedHeight -1 +1 is 1080
                                // since we always set capture window to be 1920x1080
                                // then if V doesn't do prescaling down. that means the clock is not changed
                                // if later a new resolution came and it require prescaling down, the
                                // clock changed there will be garbage shown.
                                u16VSizeAfterPreScaling = ((pDS_Info->CodedHeight / 2) -1) & ~0x01 ;
                            } else {
                                if (pDSXCData->stCropWin.height > pDSXCData->stDispWin.height) {
                                    u16VSizeAfterPreScaling = ( (MS_U32)pDS_Info->CodedHeight * pDSXCData->stDispWin.height + ( pDSXCData->stCropWin.height / 2 ) ) / pDSXCData->stCropWin.height;
                                } else {
                                    u16VSizeAfterPreScaling = pDS_Info->CodedHeight;
                                }
                            }
                            ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x80000000 | V_PreScalingDownRatio(pDS_Info->CodedHeight, u16VSizeAfterPreScaling));
                        }else {
                            u16VSizeAfterPreScaling = pDS_Info->CodedHeight;
                            ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x00000000);
                        }
                    }
                } else {

                    u16VSizeAfterPreScaling = pDS_Info->CodedHeight;

                    if ( (u16VSizeAfterPreScaling == pDS_Info->CodedHeight) || (u16VSizeAfterPreScaling > pDS_Info->CodedHeight ) ) {
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x00000000);//disable V prescaling
                    } else {
                        ds_write_4bytes(pDSIntData, DS_IP, 0x02, 0x08, 0x80000000 | V_PreScalingDownRatio(pDS_Info->CodedHeight, u16VSizeAfterPreScaling));
                    }
                }

        }

        DS_MSG("u16HSizeAfterPreScaling (%d)\n", u16HSizeAfterPreScaling);
        DS_MSG("u16VSizeAfterPreScaling %d\n", u16VSizeAfterPreScaling);


        /*
        for bandwidth issue, adjust the IPMFetch according the Hsize after pre-scaling
        But change the IPM fetch will cause the vidoe transition, suppose when change display window(zoom)
        will use black video and keep IPM fetch accroding the panel size.
        for mirror mode need to do horizontal mirror offset.
        */
        // 用max width而不用 u16HSizeAfterPreScaling或者是pDSXCData->stDispWin.width的原因是：
        // u16HSizeAfterPreScaling是實際上的capture window而非虛擬1920x1080
        // stDispWin.width以前一直是1920x1080，但是現在三星有可能傳不同的值，所以我們的fetch
        // 不能再用stDispWin.width來做處理，不然會造成offset不是0x780
        u16IPMFetch = (u16HSizeAfterPreScaling + pDSIntData->MemAlign*2 - 1) & ~(pDSIntData->MemAlign*2-1);
        //when do zoom change, change IPM for BW issue.
        //keep the IPM fetch when do scene change according display window.
        ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x0F, u16IPMFetch);

        if (pDSIntData->enChipID == DS_KAISER) {
            //Offset setting - to avoid utopia calculate offset formula is different with DS formula,
            //force write offset value here
            /*
             * Adjust IPM/OPM offset as same as IPM Fetch
             * Offset must be equal to or above fetch
             */
            ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x0E, u16IPMFetch);
            ds_write_2bytes(pDSIntData, DS_OP, 0x12, 0x16, u16IPMFetch);

            // Refine IPMOffset for cropping
            pDSXCData->u16IPMOffset = u16IPMFetch;
        }

        if (pDSXCData->bMirrorMode) {
            if ( (pDSIntData->enChipID != DS_MARIA10) && (pDSIntData->enChipID != DS_J2) &&
                 (pDSIntData->enChipID != DS_A3)  && (pDSIntData->enChipID != DS_A5) &&
                 (pDSIntData->enChipID != DS_AGATE) && (pDSIntData->enChipID != DS_EDISON) &&
                 (pDSIntData->enChipID != DS_EAGLE) && (pDSIntData->enChipID != DS_EMERALD) &&
                 (pDSIntData->enChipID != DS_CEDRIC) && (pDSIntData->enChipID != DS_KAISER) &&
                 (pDSIntData->enChipID != DS_NIKE) && (pDSIntData->enChipID != DS_EINSTEIN) &&
                 (pDSIntData->enChipID != DS_NAPOLI) && (pDSIntData->enChipID != DS_MADISON) &&
                 (pDSIntData->enChipID != DS_MONACO) && (pDSIntData->enChipID != DS_CLIPPERS) &&
                 (pDSIntData->enChipID != DS_MUJI)
               ) {
                u16H_mirrorOffset = (pDSIntData->MaxWidth * (pDSXCData->u8BitPerPixel/8)*u8Framenumber/pDSIntData->MemAlign)-3;
                ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x1C, u16H_mirrorOffset|0x1000);
            } else {
                u16H_mirrorOffset = (pDSIntData->MaxWidth * (pDSXCData->u8BitPerPixel/8)*u8Framenumber/pDSIntData->MemAlign);
                ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x1C, u16H_mirrorOffset|0x8000);
            }
        } else {
            ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x1C, 0x0000);
        }
        DS_MSG("u16H_mirrorOffset(%d),u16IPMFetch(%d)\n",u16H_mirrorOffset,u16IPMFetch);

        ///////////////////////////////////////////////////////
        //
        // set OP base, fetch/offset & scaling ratio
        //
        ///////////////////////////////////////////////////////
        // set v length
        // set v length later.
        // calculate DS crop win based on new coded width/height and original crop win
        if (pDSIntData->u8EnhanceMode != DS_ENHANCE_MODE_RESET_DISP_WIN) {
            stDSCropWin.x = (unsigned long) pDSXCData->stCropWin.x * pDS_Info->CodedWidth/ _FirstCodedWidth;
            stDSCropWin.width = (unsigned long) pDSXCData->stCropWin.width * pDS_Info->CodedWidth/ _FirstCodedWidth;

            stDSCropWin.y = (unsigned long) pDSXCData->stCropWin.y * pDS_Info->CodedHeight / _FirstCodedHeight;
            stDSCropWin.height = (unsigned long) pDSXCData->stCropWin.height * pDS_Info->CodedHeight/ _FirstCodedHeight;
        } else {
            stDSCropWin.x = (unsigned long) pDSXCData->stCropWin.x;// * pDS_Info->CodedWidth/ _FirstCodedWidth;
            stDSCropWin.width = (unsigned long) pDSXCData->stCropWin.width;// * pDS_Info->CodedWidth/ _FirstCodedWidth;

            stDSCropWin.y = (unsigned long) pDSXCData->stCropWin.y;// * pDS_Info->CodedHeight / _FirstCodedHeight;
            stDSCropWin.height = (unsigned long) pDSXCData->stCropWin.height;// * pDS_Info->CodedHeight/ _FirstCodedHeight;
        }

        DS_MSG("stDSCropWin x,y (%d,%d)\n",stDSCropWin.x, stDSCropWin.y);
        DS_MSG("stDSCropWin width,height (%d,%d)\n",stDSCropWin.width, stDSCropWin.height);


        // re-calculate scaled crop window based if pre-scaling down
        //if(pDS_Info->CodedWidth > pDSIntData->MaxWidth)
        if (stDSCropWin.width > pDS_Info->CodedWidth) {
            // 新計算出來放大過後的crop size比resolution還要大？這是不被允許的
            // 所以新計算過後的crop size必須剛好跟resolution一樣大
            // pre-scaling down, need to re-calculate crop width
            //print2(0xEE18, PDD,   pDS_Info->CodedWidth,  pDSIntData->MaxWidth);

            //stDSScaledCropWin.x = ((unsigned long) stDSCropWin.x ) & ~0x1;
            //stDSScaledCropWin.width = (stDSCropWin.width + 1) & ~0x1 ;

            //意思就是沒有crop window
            stDSScaledCropWin.x = 0;
            stDSScaledCropWin.width = u16HSizeAfterPreScaling;
        } else {
            stDSScaledCropWin.x = stDSCropWin.x * u16HSizeAfterPreScaling / ((pDS_Info->CodedWidth +1) & ~0x01);
            stDSScaledCropWin.width = stDSCropWin.width * u16HSizeAfterPreScaling / ((pDS_Info->CodedWidth +1) & ~0x01);
        }

        // re-calculate crop window based on input V capture change, V has no pre-scaling in DS case
        if (stDSCropWin.height > pDS_Info->CodedHeight) {
            // pre-scaling down, need to re-calculate crop width
            //print2(0xEE19, PDD,   pDS_Info->CodedHeight,  pDSIntData->MaxHeight);

            //stDSScaledCropWin.y = stDSCropWin.y;//((unsigned long) stDSCropWin.y * pDSIntData->MaxHeight/ pDS_Info->CodedHeight) & ~0x1;
            //stDSScaledCropWin.height = stDSCropWin.height;//pDS_Info->CodedHeight;//((unsigned long) stDSCropWin.height * pDSIntData->MaxHeight / pDS_Info->CodedHeight) & ~0x1;

            // crop window比resolution還要大，這是不被允許的
            // 只好把crop window縮小到跟resolution一樣大
            stDSScaledCropWin.y = 0;
            stDSScaledCropWin.height = (u16VSizeAfterPreScaling + 1) & ~0x1;
        } else {
            stDSScaledCropWin.y = stDSCropWin.y * u16VSizeAfterPreScaling / ((pDS_Info->CodedHeight +1) & ~0x01);
            stDSScaledCropWin.height = stDSCropWin.height * u16VSizeAfterPreScaling / ((pDS_Info->CodedHeight +1) & ~0x01);
        }

        //check height value
        //set V length
        //when do crop v length should set according the crop size
        //due to at the lower boundary filter will do 4 line scaling set v length to avoid capture garbge line.
        if (stDSScaledCropWin.y+stDSScaledCropWin.height>u16VSizeAfterPreScaling) {
            if (u16VSizeAfterPreScaling >= stDSScaledCropWin.height) {
                stDSScaledCropWin.y = (u16VSizeAfterPreScaling - stDSScaledCropWin.height) & ~0x1;
            } else {
                stDSScaledCropWin.y = 0;
                stDSScaledCropWin.height = u16VSizeAfterPreScaling;
            }
            ds_write_2bytes(pDSIntData, DS_OP, 0x20, 0x15, stDSScaledCropWin.height);
        } else {
            ds_write_2bytes(pDSIntData, DS_OP, 0x20, 0x15, u16VSizeAfterPreScaling);
        }

        DS_MSG("stDSScaledCropWin x,y (%d,%d)\n",  stDSScaledCropWin.x, stDSScaledCropWin.y);
        DS_MSG("stDSScaledCropWin width,height (%d,%d)\n",  stDSScaledCropWin.width, stDSScaledCropWin.height);

        if (bInterlace) {
            u32PixelOffset = ((unsigned long)stDSScaledCropWin.y/2 * (unsigned long)pDSXCData->u16IPMOffset) + (unsigned long)stDSScaledCropWin.x;
        } else {
            u32PixelOffset = ((unsigned long)stDSScaledCropWin.y * (unsigned long)pDSXCData->u16IPMOffset) + (unsigned long)stDSScaledCropWin.x;
        }

        if (pDSXCData->bMirrorMode) {
            u32PixelOffset = u32PixelOffset - stDSScaledCropWin.x;
        }

        DS_MSG("u32PixelOffset (%d)\n",  u32PixelOffset);

        u16LineBufOffset = (unsigned short)(u32PixelOffset % pDSIntData->OffsetAlign);
        u32PixelOffset -= (unsigned long)u16LineBufOffset;
        u32PixelOffset *= (unsigned long)pDSXCData->bLinearMode ? 1:u8Framenumber;       // if not linear mode, need to multiply by 2

        DS_MSG("u32PixelOffset(%d), u8BitPerPixel(%d)\n",  u32PixelOffset,pDSXCData->u8BitPerPixel);

        if (u32PixelOffset != 0) {
            u32CropOffset =  u32PixelOffset * (pDSXCData->u8BitPerPixel/8);
        } else {
            u32CropOffset = 0;
        }


        DS_MSG("PixelOff %d (align %d, linear %d)\n", u32PixelOffset, pDSIntData->OffsetAlign, pDSXCData->bLinearMode);
        DS_MSG("IPMOffset=%d, OPMFetch=%d\n\n", pDSXCData->u16IPMOffset, u16OPMFetch);
        DS_MSG("PixelOffset=%d, LBOffset=%d\n", u32PixelOffset, u16LineBufOffset);

        if (pDSXCData->bMirrorMode) {
            if (pDSXCData->bLinearMode) {
                u32CropOffset = (pDSIntData->MaxWidth - u16OPMFetch -
                                 ((stDSScaledCropWin.x +pDSIntData->MemAlign*2 -1)& ~(pDSIntData->MemAlign*2 -1)))*(pDSXCData->u8BitPerPixel/8) + u32CropOffset;
                u32CropOffset = u32CropOffset + (pDSIntData->MaxHeight-u16VSizeAfterPreScaling) *pDSIntData->MaxWidth*(pDSXCData->u8BitPerPixel/8);
            } else {
                u32CropOffset += (pDSIntData->MaxWidth -
                                  (( stDSScaledCropWin.width +stDSScaledCropWin.x+pDSIntData->MemAlign*2 -1)& ~(pDSIntData->MemAlign*2 -1)))*(pDSXCData->u8BitPerPixel/8)*u8Framenumber;
            }

            if (bInterlace) {
                u32CropOffset = u32CropOffset + ((pDSIntData->MaxHeight-u16VSizeAfterPreScaling)/2*u8Framenumber *pDSIntData->MaxWidth*(pDSXCData->u8BitPerPixel/8));// + 575
            } else {

                u32CropOffset = u32CropOffset + ((pDSIntData->MaxHeight-u16VSizeAfterPreScaling)*u8Framenumber *pDSIntData->MaxWidth*(pDSXCData->u8BitPerPixel/8));// + 575
            }

            DS_MSG("Mirror mode u32CropOffset %x--\n",u32CropOffset);
        }

        // For box chips, OPM fetch should align 2px (in FB mode)
        if (pDSIntData->enChipID != DS_CLIPPERS) {
            u16OPMFetch = (stDSScaledCropWin.width + u16LineBufOffset + pDSIntData->MemAlign * 2 - 1) & ~(pDSIntData->MemAlign * 2 - 1);
        } else {
            u16OPMFetch = (stDSScaledCropWin.width + u16LineBufOffset + 2 * 2 - 1) & ~(2 * 2 - 1);
        }

        if (pDSXCData->bMirrorMode) {
            MS_U32 tempAlign = (stDSScaledCropWin.width+stDSScaledCropWin.x)%(pDSIntData->MemAlign*2);
            u16LineBufOffset = (pDSIntData->MemAlign*2 - tempAlign)%(pDSIntData->MemAlign*2);
            u16OPMFetch = (stDSScaledCropWin.width + u16LineBufOffset+ 1) & ~0x1;
            // write OP base
            ds_write_4bytes(pDSIntData, DS_OP, 0x12, 0x10, (pDSXCData->u32IPMBase0 + u32CropOffset)/pDSIntData->MemAlign);
            ds_write_4bytes(pDSIntData, DS_OP, 0x12, 0x12, (pDSXCData->u32IPMBase1 + u32CropOffset)/pDSIntData->MemAlign);

            if ( (pDSIntData->enChipID != DS_MARIA10) && (pDSIntData->enChipID != DS_J2) &&
                 (pDSIntData->enChipID != DS_A3)  && (pDSIntData->enChipID != DS_A5) &&
                 (pDSIntData->enChipID != DS_AGATE) && (pDSIntData->enChipID != DS_EDISON) &&
                 (pDSIntData->enChipID != DS_EAGLE) && (pDSIntData->enChipID != DS_EMERALD) &&
                 (pDSIntData->enChipID != DS_CEDRIC) && (pDSIntData->enChipID != DS_NIKE) &&
                 (pDSIntData->enChipID != DS_EINSTEIN) && (pDSIntData->enChipID != DS_NAPOLI) &&
                 (pDSIntData->enChipID != DS_MADISON) && (pDSIntData->enChipID != DS_MONACO) &&
                 (pDSIntData->enChipID != DS_CLIPPERS) && (pDSIntData->enChipID != DS_MUJI)
               ) {
                ds_write_4bytes(pDSIntData, DS_OP, 0x12, 0x14, (pDSXCData->u32IPMBase2 + u32CropOffset)/pDSIntData->MemAlign);
            }
        } else {
            // write OP base
            ds_write_4bytes(pDSIntData, DS_OP, 0x12, 0x10, (pDSXCData->u32IPMBase0 + u32CropOffset)/pDSIntData->MemAlign);
            ds_write_4bytes(pDSIntData, DS_OP, 0x12, 0x12, (pDSXCData->u32IPMBase1 + u32CropOffset)/pDSIntData->MemAlign);

            if ( (pDSIntData->enChipID != DS_MARIA10) && (pDSIntData->enChipID != DS_J2) &&
                 (pDSIntData->enChipID != DS_A3)  && (pDSIntData->enChipID != DS_A5) &&
                 (pDSIntData->enChipID != DS_AGATE) && (pDSIntData->enChipID != DS_EDISON) &&
                 (pDSIntData->enChipID != DS_EAGLE) && (pDSIntData->enChipID != DS_EMERALD) &&
                 (pDSIntData->enChipID != DS_CEDRIC) && (pDSIntData->enChipID != DS_NIKE) &&
                 (pDSIntData->enChipID != DS_EINSTEIN) && (pDSIntData->enChipID != DS_NAPOLI) &&
                 (pDSIntData->enChipID != DS_MADISON) && (pDSIntData->enChipID != DS_MONACO) &&
                 (pDSIntData->enChipID != DS_CLIPPERS) && (pDSIntData->enChipID != DS_MUJI)
               ) {
                ds_write_4bytes(pDSIntData, DS_OP, 0x12, 0x14, (pDSXCData->u32IPMBase2 + u32CropOffset)/pDSIntData->MemAlign);
            }
        }

        //write OP fetch & LBoffset
        ds_write_2bytes(pDSIntData, DS_OP, 0x12, 0x17, u16OPMFetch);
        ds_write_2bytes(pDSIntData, DS_OP, 0x20, 0x1D, (u16LineBufOffset&0xFF) | ((u16LineBufOffset&0xFF)<<8));

        // post H/V scaling
        DS_MSG("stDSScaledCropWin.width %d, pDSXCData->stDispWin.width %d\n", stDSScaledCropWin.width, pDSXCData->stDispWin.width);
        ds_write_4bytes(pDSIntData, DS_OP, 0x23, 0x07, 0x01000000 | H_PostScalingRatio(stDSScaledCropWin.width, pDSXCData->stDispWin.width));
        if (pDSXCData->stDispWin.height == 1)
            ds_write_4bytes(pDSIntData, DS_OP, 0x23, 0x09, 0x01000000 | V_PostScalingRatio(stDSScaledCropWin.height, (pDSXCData->stDispWin.height + 1) & ~0x1));
        else
            ds_write_4bytes(pDSIntData, DS_OP, 0x23, 0x09, 0x01000000 | V_PostScalingRatio(stDSScaledCropWin.height, pDSXCData->stDispWin.height));

        if (pDSIntData->u8EnhanceMode == DS_ENHANCE_MODE_RESET_DISP_WIN) {
            ds_write_2bytes(pDSIntData, DS_OP, 0x10, 0x0A, pDSXCData->stDispWin.y);
            ds_write_2bytes(pDSIntData, DS_OP, 0x10, 0x0B, pDSXCData->stDispWin.y + pDSXCData->stDispWin.height -1);
            ds_write_2bytes(pDSIntData, DS_OP, 0x10, 0x08, pDSXCData->stDispWin.x);
            ds_write_2bytes(pDSIntData, DS_OP, 0x10, 0x09, pDSXCData->stDispWin.x + pDSXCData->stDispWin.width -1);
        }

        // Disable 4K2K bypass mode for Clippers
        if (pDSIntData->enChipID == DS_CLIPPERS) {
            ds_write_2bytes(pDSIntData, DS_OP, 0x20, 0x01, 0x0001);

            if (bInterlace) {
                ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x07, 0x4018);
            } else {
                ds_write_2bytes(pDSIntData, DS_IP, 0x12, 0x07, 0x2018);
            }
        }

        if (pDSXCData->u8Dummy2) {
            pDSXCData->u8Dummy2 = 0;
            ds_write_2bytes(pDSIntData, DS_IP, 0x1F, 0x13, 0);
        }

        // flush memory
        ds_write_buf_flush(pDSIntData);
    } else {
        ERROR_CODE(1);
    }
    pDSXCData->bFWIsUpdating = 0;

}

void dynamic_scale_info_update(unsigned char *pDS_Update_Info, ds_internal_data_structure *pDS_Int_Data, unsigned char update_cmd)
{
    ds_xc_data_structure *pDSXCData = pDS_Int_Data->pDSXCData;

    if (pDS_Int_Data->bIsInitialized == 0) {
        printf("ISR: DS not inited\n");

        while (1);

    }

    // copy info to DS buffer
    if (pDS_Update_Info != 0) {
        switch (update_cmd) {
        case MM_DS_XC_CMD_UPDATE_ZOOM_INFO:

            pDSXCData->u8XCInfoUpdateIdx += 1;

            // update new crop/disp win based on first CapWin
            // it's safe to copy NewCrop info here because when interrupt here, there is no change that HK doing DS buf calculation
            // (because when HK doing DS calculation, it will stop interrupt)
            //ds_memcpy(&pDSXCData->stCropWin, &pDSXCData->stNewCropWin, sizeof(MS_WINDOW_TYPE));
            //ds_memcpy(&pDSXCData->stDispWin, &pDSXCData->stNewDispWin, sizeof(MS_WINDOW_TYPE));
            pDSXCData->bFWGotNewSetting = 1;

            DS_MSG("bFWGotNewSetting(%d),u8XCInfoUpdateIdx(%d)\n", pDSXCData->bFWGotNewSetting, pDSXCData->u8XCInfoUpdateIdx);

            break;

        case MM_DS_XC_CMD_UPDATE_XC_INFO:

            DS_MSG("bFWGotXCInfo(%d),u8XCInfoUpdateIdx(%d)\n",  pDSXCData->bFWGotXCInfo, pDSXCData->u8XCInfoUpdateIdx);

            pDSXCData->u8XCInfoUpdateIdx += 1;
            pDSXCData->bFWGotXCInfo = 1;
            break;

        default:
            break;
        }
    } else {
    }
}

unsigned char dynamic_scale_is_got_xc_info(ds_internal_data_structure *pDS_Int_Data)
{
    return pDS_Int_Data->pDSXCData->bFWGotXCInfo;
}

unsigned char dynamic_scale_get_xc_info_update_idx(ds_internal_data_structure *pDS_Int_Data)
{
    return pDS_Int_Data->pDSXCData->u8XCInfoUpdateIdx;
}

unsigned char dynamic_scale_is_xc_fw_get_new_setting(ds_internal_data_structure *pDS_Int_Data)
{
    return pDS_Int_Data->pDSXCData->bFWGotNewSetting;
}

#endif // DS_compiler
