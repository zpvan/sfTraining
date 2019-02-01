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

#ifndef _DYNAMIC_SCALE_
#define _DYNAMIC_SCALE_

#include <stdint.h>
//#include  "mhal_xc_chip_config.h"

/*
Check BYTE_PER_WORD with dynamic_scale.c
For memory alignment, if complier error need to add define and modify dynamic_scale_init()
at dynamic_scale.c

#define BYTE_PER_WORD           (16)

*/
#if 0
    #if (defined(CHIP_T12)||defined(CHIP_T13)||defined(CHIP_J2)||defined(CHIP_A1)||defined(CHIP_A2)||defined(CHIP_A6)||defined(CHIP_A7)||defined(CHIP_AMETHYST) || defined(CHIP_EAGLE)||defined(CHIP_EMERALD))
        #if(BYTE_PER_WORD!=16)
            #ERROR
        #endif
    #elif(defined(CHIP_A5)||defined(CHIP_A3)  || defined(CHIP_AGATE) || defined(CHIP_EDISON) || defined(CHIP_Einstein) || defined(CHIP_EIFFEL))
        #if(BYTE_PER_WORD!=32)
            #ERROR
        #endif
    #elif(defined(CHIP_T7)||defined(CHIP_K1)||defined(CHIP_K2)||defined(CHIP_U4)||defined(CHIP_MACAW12)||defined(CHIP_EDEN)||defined(CHIP_EULER))
        #if(BYTE_PER_WORD !=8)
            #ERROR
        #endif
    #else
        #if(BYTE_PER_WORD !=8)
            #ERROR
        #endif
    #endif
#endif

#define BYTE_PER_WORD   32

#if 0
/*
M10 J2 A5 A6 supoort field packet mode
#define _FIELD_PACKING_MODE_SUPPORTED       0
*/
    #if(defined(CHIP_J2)|| defined(CHIP_A5) || defined(CHIP_A6) ||defined(CHIP_A3) || defined(CHIP_AGATE) || defined(CHIP_EDISON)||defined(CHIP_MACAW12)||defined(CHIP_EDEN) || defined(CHIP_EULER) || defined(CHIP_EAGLE)|| defined(CHIP_EMERALD) || defined(CHIP_Einstein) || defined(CHIP_EIFFEL))
        #define ENABEL_FIELD_PACKING_MODE_SUPPORTED 1
    #else
        #define ENABEL_FIELD_PACKING_MODE_SUPPORTED 0
    #endif

    #if(ENABEL_FIELD_PACKING_MODE_SUPPORTED !=_FIELD_PACKING_MODE_SUPPORTED)
        #ERROR
    #endif
#endif

#define DS_compiler 1  // not to compiler oringial dynamic scaling code.

#if DS_compiler
    #define DS_DEBUG    1
    #if DS_DEBUG
        #define ERROR_CODE(x)       do{pDSIntData->ErrorCode = x;}while(0)
    #else
        #define ERROR_CODE(x)
    #endif

    #define DS_VERSION          0x03

    #define DS_COMMAND_BYTE_LEN 16
    #define DS_BUFFER_NUM       12
    #define DS_BUFFER_NUM_EX    6

    #define DS_ENHANCE_MODE_NONE    0
    #define DS_ENHANCE_MODE_3D      1
    #define DS_ENHANCE_MODE_RESET_DISP_WIN    2

typedef enum {
    DS_Euclid,
    DS_T3,
    DS_T4,
    DS_T7,
    DS_T8,
    DS_T9,
    DS_Unarus4,
    DS_T12,
    DS_T13,
    DS_MARIA10,
    DS_J2,
    DS_K1,
    DS_K2,
    DS_A1,
    DS_A2,
    DS_A3,
    DS_A5,
    DS_A6,
    DS_A7,
    DS_AGATE,
    DS_AMETHYST,
    DS_EAGLE,
    DS_EMERALD,
    DS_EDISON,
    DS_EIFFEL,
    DS_CEDRIC,
    DS_KAISER,
    DS_NIKE,
    DS_EINSTEIN,
    DS_NAPOLI,
    DS_MADISON,
    DS_MONACO,
    DS_CLIPPERS,
    DS_MUJI,
} ds_chip_id;

/// MVOP Mirror
typedef enum {
    E_MVOP_MIRROR_VERTICAL    = 1,
    E_MVOP_MIRROR_HORIZONTALL = 2,
    E_MVOP_MIRROR_HVBOTH       = 3,
    E_MVOP_MIRROR_NONE         = 4,
} MVOP_Mirror;

typedef struct {
    union {
        struct {
            uint32_t reg_value      : 16;
            uint32_t addr           : 8;
            uint32_t bank           : 8;
        };

        uint32_t reg_setting;
    };
} ds_reg_setting_structure;

typedef struct {
    // 0x000
    uint8_t u8DSVersion;                  // Versin
    uint8_t bHKIsUpdating;                // House keeping is writing
    uint8_t bFWIsUpdating;                // firmware is reading
    uint8_t bFWGotXCInfo;                 // for firmware internal use, indicate that firmware received XC current setting
    uint8_t u8XCInfoUpdateIdx;            //
    uint8_t bFWGotNewSetting;             // for firmware internal use, indicate that firmware received XC current setting
    uint8_t bSaveBandwidthMode;
    uint8_t u8Dummy_2;

    // 0x008
    MS_WINDOW_TYPE stCapWin;
    MS_WINDOW_TYPE stCropWin;

    // 0x018
    MS_WINDOW_TYPE stDispWin;
    uint16_t u16H_SizeAfterPreScaling;
    uint16_t u16V_SizeAfterPreScaling;
    uint32_t  u32PNL_Width;                ///<Panel active width

    // 0x028
    uint32_t u32IPMBase0;                  ///<IPM base 0
    uint32_t u32IPMBase1;                  ///<IPM base 1
    uint32_t u32IPMBase2;                  ///<IPM base 2

    // 0x034
    MS_WINDOW_TYPE stNewCropWin;                ///< Zoom in/out new crop win
    MS_WINDOW_TYPE stNewDispWin;                ///< Zoom in/out new disp win

    // 0x044
    uint8_t bLinearMode;                  ///<Is current memory format LinearMode?
    uint8_t u8BitPerPixel;                ///<Bits number Per Pixel
    uint8_t bInterlace;                   ///<Interlaced or Progressive
    uint8_t u8StoreFrameNum;

    // 0x048
    uint16_t u16IPMOffset;                ///<IPM offset
    uint16_t u16Dummy0;

    // 0x4C
    uint8_t bMirrorMode;                  ///<Mirror
    uint8_t bResolutionChanging;          ///<indicate the resolution will be changed.
    uint8_t u8MVOPMirror;                 ///<MVOP Mirror
    uint8_t u8Dummy2;

    // internal use only
    // 0x050
////////////////// START:USED for SW DS only, VDEC DS firmware do not use////////////////////////////
    MS_WINDOW_TYPE stDSScaledCropWin_SWDS;

    // 0x058
    uint16_t u16SizeAfterPreScaling_SWDS;
    uint16_t u16Fetch_SWDS;
    uint16_t u16Offset_SWDS;
    uint16_t u16PixelOffset_SWDS;
    uint16_t u16LineBufOffset_SWDS;
    uint16_t u16Dummy1_SWDS;

    // 0x064
    uint32_t u32CropOffset_SWDS;

    // for debugging, 0x68
    uint8_t OPRegMaxDepth_SWDS;                // point to record max OP reg depth
    uint8_t IPRegMaxDepth_SWDS;                // point to record max IP reg depth
    uint8_t ErrorCode_SWDS;                    // point to record error code
    uint8_t u8Dummy3_SWDS;

    // 0x6C
    MS_WINDOW_TYPE stDSCropWin_SWDS;
////////////////// END:USED for SW DS only, VDEC DS firmware do not use////////////////////////////

    // 0x74
    uint16_t u16HCusScalingSrc;        ///< horizontal scaling down/up source size
    uint16_t u16VCusScalingSrc;        ///< vertical scaling down/up source size
    uint16_t u16HCusScalingDst;        ///< horizontal prescaling down/up destination size
    uint16_t u16VCusScalingDst;        ///< vertical prescaling down/up destination size

    // 0x7C
    E_XC_3D_INPUT_MODE  e3DInputType;        ///< input 3D type decided by APP layer
    E_XC_3D_OUTPUT_MODE e3DOutputType;       ///< output 3D type decided by APP layer

    // 0x84
    uint16_t u16MVOPHStart;            ///<MVOP HStart
    uint16_t u16MVOPVStart;            ///<MVOP VStart
    uint8_t u8BobMode;                 ///<set BOB mode for De-interlace
} ds_xc_data_structure;

typedef struct {
    ds_chip_id enChipID;                            // ChipID
    uint32_t bIsInitialized;                        // BSS will be cleared to 0x00
    uint32_t MemAlign;                              // depends on chips, unit = bytes
    uint32_t OffsetAlign;                           // depends on chips, unit = bytes (for Crop)
    uint32_t DS_Depth;                              // dynamic scaling depth
    uint32_t MaxWidth;                              // max width of frame buffer, assigned by MM application
    uint32_t MaxHeight;                             // max height of frame buffer, assigned by MM application
    uint8_t *pDSBufBase;                            // DS buffer
    uint32_t  DSBufSize;                            // DS buffer size
    ds_xc_data_structure *pDSXCData;                // MM passed XC info data

    // for filling DS buffer
    ds_reg_setting_structure *pCurDSBuf;            // pointer to current DS buffer based on pDSBufBase & DSBufIdx
    ds_reg_setting_structure *pOPRegSetting;        // point to current writable OP reg setting
    ds_reg_setting_structure *pIPRegSetting;        // point to current writable IP reg setting
    uint8_t OPRegMaxDepth;                          // point to record max OP reg depth
    uint8_t IPRegMaxDepth;                          // point to record max IP reg depth
    uint8_t ErrorCode;                              // point to record error code
    uint8_t u8EnhanceMode;                          // For Enhance mode settings.
} ds_internal_data_structure;

typedef struct {
    // IN
    ds_chip_id ChipID;                              // chip id
    ds_internal_data_structure *pDSIntData;         // internal buffer for DS to store global variables
    uint32_t MaxWidth;                              // max width of frame buffer
    uint32_t MaxHeight;                             // max height of frame buffer
    uint8_t *pDSBufBase;                            // DS buffer
    uint32_t  DSBufSize;                            // DS buffer size
    uint8_t *pDSXCData;                             // To store XC info passed from MM
    uint8_t u8EnhanceMode;                          // For Enhance mode settings.
    uint32_t DS_Depth;                              // DS command depth
} ds_init_structure;

typedef struct {
    ds_internal_data_structure *pDSIntData;         // internal buffer for DS to store global variables
    uint16_t  CodedWidth;                           // coded width, current decoded width
    uint16_t  CodedHeight;                          // coded height, current decoded height
    uint16_t  CropRight;
    uint16_t  DSBufIdx;                             // fill to which index of DS buffer
} ds_recal_structure;

typedef enum {
    DS_IP,
    DS_OP,
} ds_reg_ip_op_sel;

#ifdef __cplusplus
extern "C" {
#endif

    extern void dynamic_scale_init(ds_init_structure *pDS_Init_Info);
    extern void dynamic_scale(ds_recal_structure *pDS_Info);
    extern void dynamic_scale_info_update(uint8_t *pDS_Update_Info, ds_internal_data_structure *pDS_Int_Data, uint8_t update_cmd);
    extern unsigned char dynamic_scale_is_got_xc_info(ds_internal_data_structure *pDS_Int_Data);
    extern unsigned char dynamic_scale_get_xc_info_update_idx(ds_internal_data_structure *pDS_Int_Data);
    extern unsigned char dynamic_scale_is_xc_fw_get_new_setting(ds_internal_data_structure *pDS_Int_Data);
    void ds_write_2bytes(ds_internal_data_structure *pDSIntData,
                         ds_reg_ip_op_sel IPOP_Sel,
                         uint32_t bank,
                         uint32_t addr,
                         uint32_t reg_value);

    void ds_write_4bytes(ds_internal_data_structure *pDSIntData,
                         ds_reg_ip_op_sel IPOP_Sel,
                         uint32_t bank,
                         uint32_t addr,
                         uint32_t reg_value);

#ifdef __cplusplus
}
#endif

#endif // DS_compiler

#endif // _DYNAMIC_SCALE_

