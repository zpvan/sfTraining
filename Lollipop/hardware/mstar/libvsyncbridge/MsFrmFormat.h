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

#ifndef _MS_FRM_FORMAT_H_
#define _MS_FRM_FORMAT_H_

#include <stdint.h>

typedef enum {
    MS_CODEC_TYPE_NONE,
    MS_CODEC_TYPE_MPEG2,
    MS_CODEC_TYPE_H263,
    MS_CODEC_TYPE_MPEG4,
    MS_CODEC_TYPE_DIVX311,
    MS_CODEC_TYPE_DIVX412,
    MS_CODEC_TYPE_FLV,
    MS_CODEC_TYPE_VC1_ADV,
    MS_CODEC_TYPE_VC1_MAIN,
    MS_CODEC_TYPE_RV8,
    MS_CODEC_TYPE_RV9,
    MS_CODEC_TYPE_H264,
    MS_CODEC_TYPE_AVS,
    MS_CODEC_TYPE_MJPEG,
    MS_CODEC_TYPE_MVC,
    MS_CODEC_TYPE_VP8,
    MS_CODEC_TYPE_HEVC,
    MS_CODEC_TYPE_VP9,
    MS_CODEC_TYPE_HEVC_DV,
    MS_CODEC_TYPE_NUM
} MS_CodecType;

typedef enum {
    MS_FRAME_TYPE_I,
    MS_FRAME_TYPE_P,
    MS_FRAME_TYPE_B,
    MS_FRAME_TYPE_OTHER,
} MS_FrameType;

typedef enum {
    MS_FIELD_TYPE_NONE,
    MS_FIELD_TYPE_TOP,
    MS_FIELD_TYPE_BOTTOM,
    MS_FIELD_TYPE_BOTH,
} MS_FieldType;

typedef enum {
    MS_COLOR_FORMAT_HW_HVD,                 //YUV420 HVD tiled format (16x32)
    MS_COLOR_FORMAT_HW_MVD,                 //YUV420 MVD tiled format (8x16)
    MS_COLOR_FORMAT_SW_YUV420_PLANAR,       //YUV420 Planar
    MS_COLOR_FORMAT_SW_RGB565,              //RGB565
    MS_COLOR_FORMAT_SW_ARGB8888,            //ARGB8888
    MS_COLOR_FORMAT_YUYV,                   //YUV422 YUYV
    MS_COLOR_FORMAT_SW_RGB888,              //RGB888
    MS_COLOR_FORMAT_10BIT_TILE,             //YUV420 tiled 10 bits mode
    MS_COLOR_FORMAT_SW_YUV420_SEMIPLANAR,   //YUV420 SemiPlanar
    MS_COLOR_FORMAT_YUYV_CSC_BIT601,        //YUV422 YUYV from RGB2YUV bit601 mode
    MS_COLOR_FORMAT_YUYV_CSC_255,           //YUV422 YUYV from RGB2YUV 0~255 mode
    MS_COLOR_FORMAT_HW_EVD,                 //YUV420 EVD tiled format (32x16)
    MS_COLOR_FORMAT_HW_32x32,               //YUV420 32x32 tiled format (32x32)
} MS_VColorFormat;

typedef enum {
    MS_VIEW_TYPE_CENTER,
    MS_VIEW_TYPE_LEFT,
    MS_VIEW_TYPE_RIGHT,
    MS_VIEW_TYPE_TOP,
    MS_VIEW_TYPE_BOTTOM,
} MS_ViewType;

typedef enum
{
    MS_3DMODE_DEFAULT,
    MS_3DMODE_SIDEBYSIDE,
} MS_3DMode;

typedef enum
{
    MS_FRC_NORMAL = 0,
    MS_FRC_32PULLDOWN,               //3:2 pulldown mode (ex. 24p a 60i or 60p)
    MS_FRC_PAL2NTSC ,                //PALaNTSC conversion (50i a 60i)
    MS_FRC_NTSC2PAL,                 //NTSCaPAL conversion (60i a 50i)
    MS_FRC_DISP_2X,                  //output rate is twice of input rate (ex. 30p a 60p)
    MS_FRC_24_50,                    //output rate 24P->50P 48I->50I
    MS_FRC_50P_60P,                  //output rate 50P ->60P
    MS_FRC_60P_50P,                  //output rate 60P ->50P
    MS_FRC_HALF_I,                   //output rate 120i -> 60i, 100i -> 50i
    MS_FRC_120I_50I,                 //output rate 120i -> 60i
    MS_FRC_100I_60I,                 //output rate 100i -> 60i
} MS_FRCMode;

typedef enum
{
    MS_FREEZE_MODE_NONE = 0,            // no freeze this frame
    MS_FREEZE_MODE_VIDEO,               // freeze this frame and capture video from scaler OP for MALI
    MS_FREEZE_MODE_BLANK_VIDEO,         // freeze this frame and capture a blank video for MALI
    MS_FREEZE_MODE_FREEZE_ONLY,         // freeze this frame and no capture
    MS_FREEZE_MODE_FREEZE_MUTE,         // freeze this frame and mute video
    MS_FREEZE_MODE_MAX,
} MS_FreezeMode;

typedef enum
{
    MS_FIELD_CTRL_DEFAULT = 0,          // no one field mode
    MS_FIELD_CTRL_TOP,                  // one field, always top
    MS_FIELD_CTRL_BOTTOM,               // one field, always bottom
    MS_FIELD_CTRL_MAX,
} MS_FieldCtrl;

typedef enum
{
    MS_DISP_FRM_INFO_EXT_TYPE_10BIT,                // in MVC case it is L view 2 bit
    MS_DISP_FRM_INFO_EXT_TYPE_INTERLACE = 1,        // interlace bottom 8bit will share the same enum value
    MS_DISP_FRM_INFO_EXT_TYPE_DOLBY_EL = 1,         // with dolby enhance layer 8bit
    MS_DISP_FRM_INFO_EXT_TYPE_10BIT_INTERLACE = 2,  // interlace bottom 2bit will share the same enum
    MS_DISP_FRM_INFO_EXT_TYPE_10BIT_DOLBY_EL = 2,   // value with dolby enhance layer 2bit
    MS_DISP_FRM_INFO_EXT_TYPE_10BIT_MVC,            // R view 2 bit
    MS_DISP_FRM_INFO_EXT_TYPE_INTERLACE_MVC,
    MS_DISP_FRM_INFO_EXT_TYPE_10BIT_INTERLACE_MVC = 5, // MVC interlace R-View 2bit will share the
    MS_DISP_FRM_INFO_EXT_TYPE_DOLBY_META = 5,          // same enum with dolby meta data
    MS_DISP_FRM_INFO_EXT_TYPE_MFCBITLEN,
    MS_DISP_FRM_INFO_EXT_TYPE_MFCBITLEN_MVC,
    MS_DISP_FRM_INFO_EXT_TYPE_MAX,
} DISP_FRM_INFO_EXT_TYPE;

typedef enum {
    MS_BS_NONE,
    MS_BS_IN_QUEUE,
} MS_BufState;

typedef enum {
    MS_STREAM_TYPE_MAIN,
    MS_STREAM_TYPE_SUB,
    MS_STREAM_TYPE_N,
} MS_StreamType;

// for u8ApplicationType
#define MS_APP_TYPE_LOCAL    0          // Default Local File
#define MS_APP_TYPE_ONLINE   (1 << 0)   // Online Video
#define MS_APP_TYPE_IMAGE    (1 << 1)   // Image Playback
#define MS_APP_TYPE_GAME     (1 << 2)   // Game Mode
#define MS_APP_TYPE_LOW_BW   (1 << 3)   // Low BW mode

#define KEEP_ASPECT_RATE                1  // 1: keep aspect rate expend to surface/  0: use surface window
#define ENABLE_VSYNC_BRIDGE
#define RENDER_IN_QUEUE_BUFFER
#define BLOCK_UNTIL_DISP_INIT_DONE
#define VSYNC_BRIGE_SHM_ADDR            0xFFA00
#define MAX_VSYNC_BRIDGE_DISPQ_NUM      8
#define VSYNC_BRIDGE_EXT_VERSION        1

typedef struct {
    int x;                              // start x of the window
    int y;                              // start y of the window
    int width;                          // width of the window
    int height;                         // height of the window
} MS_WINDOW;

typedef struct {
    float x;                            // start x ratio of the window
    float y;                            // start y ratio of the window
    float width;                        // width ratio of the window
    float height;                       // height ratio of the window
} MS_WINDOW_RATIO;

typedef struct _MS_ColorHWFormat
{
    uint64_t u32LumaAddr;
    uint64_t u32ChromaAddr;
    uint32_t u32LumaPitch;
    uint32_t u32ChromaPitch;
    uint32_t u32Reserved[4];
} MS_ColorHWFormat;

typedef struct _MS_ColorSWFormat {
    uint64_t u32YAddr;
    uint64_t u32UAddr;
    uint64_t u32VAddr;
    uint32_t u32YPitch;
    uint32_t u32UPitch;
    uint32_t u32VPitch;
    uint32_t u32Reserved[1];
} MS_ColorSWFormat;

typedef struct _MS_FrameFormat {
    MS_VColorFormat eColorFormat;
    MS_FrameType eFrameType;
    MS_FieldType eFieldType;
    MS_ViewType eViewType;
    uint32_t u32Width;
    uint32_t u32Height;
    uint32_t u32CropLeft;
    uint32_t u32CropRight;
    uint32_t u32CropTop;
    uint32_t u32CropBottom;
    union {
        MS_ColorHWFormat sHWFormat;
        MS_ColorSWFormat sSWFormat;
    };
    uint32_t u32Idx;
    uint32_t u32PriData;
    uint8_t u8RangeMapY;
    uint8_t u8RangeMapUV;
    uint8_t u8Reserved[6];
} MS_FrameFormat;

typedef struct _MS_MasterColorDisplay {
    //mastering color display: color volumne of a display
    uint32_t u32MaxLuminance;
    uint32_t u32MinLuminance;
    uint16_t u16DisplayPrimaries[3][2];
    uint16_t u16WhitePoint[2];
} MS_MasterColorDisplay;

typedef struct _MS_ColorDescription {
    //color_description: indicates the chromaticity/opto-electronic coordinates of the source primaries
    uint8_t u8ColorPrimaries;
    uint8_t u8TransferCharacteristics;
    // matrix coefficients in deriving YUV signal from RGB
    uint8_t u8MatrixCoefficients;
} MS_ColorDescription;

typedef struct _MS_DolbyHDRInfo {
    // bit[0:1] 0: Disable 1:Single layer 2: Dual layer, bit[2] 0:Base Layer 1:Enhance Layer
    uint8_t  u8DVMode;
    uint64_t phyHDRMetadataAddr;
    uint32_t u32HDRMetadataSize;
    uint64_t phyHDRRegAddr;
    uint32_t u32HDRRegSize;
    uint64_t phyHDRLutAddr;
    uint32_t u32HDRLutSize;
    uint8_t  u8DMEnable;
    uint8_t  u8CompEnable;
    uint8_t  u8CurrentIndex;
} MS_DolbyHDRInfo;

typedef struct _MS_HDRInfo {
    // bit[0]: MS_ColorDescription present or valid, bit[1]: MS_MasterColorDisplay present or valid
    uint32_t u32FrmInfoExtAvail;
    // mastering color display: color volumne of a display
    MS_ColorDescription   stColorDescription;
    MS_MasterColorDisplay stMasterColorDisplay;
    MS_DolbyHDRInfo       stDolbyHDRInfo;
} MS_HDRInfo;

typedef struct _MS_BufSlot
{
    MS_BufState eBufState;  // buffer state
    uint32_t u32UniqueId;   // unique index
    uint8_t u8CurIndex;     // index of this buffer in vsync_bridge display queue
} MS_BufSlot;

typedef struct _MS_BufSlotInfo
{
    uint8_t u8InQueueCnt;
    MS_BufSlot sBufSlot[MAX_VSYNC_BRIDGE_DISPQ_NUM];
} MS_BufSlotInfo;

#pragma pack(push, 4)
typedef struct
{
    uint32_t u32LumaAddrExt[MS_DISP_FRM_INFO_EXT_TYPE_MAX];
    uint32_t u32ChromaAddrExt[MS_DISP_FRM_INFO_EXT_TYPE_MAX];
    uint32_t u32MFCodecInfo;
    uint16_t u16Width;      // the width of second frame
    uint16_t u16Height;     // the height of second frame
    uint16_t u16Pitch[2];   // the pitch of second frame
} DISP_FRM_INFO_EXT;
#pragma pack(pop)

typedef struct _MS_DispFrameFormat {
    uint32_t OverlayID;
    uint32_t u32StreamUniqueId;
    MS_FrameFormat sFrames[2];
    uint32_t FrameNum;
    uint32_t u32Handle;
    uint64_t u64Pts;
    uint64_t u32SHMAddr;
    uint32_t CodecType;
    uint32_t u32FrameRate;
    uint32_t u32ScalingMode;
    uint32_t u32AspectWidth;
    uint32_t u32AspectHeight;
    uint32_t u32VdecStreamVersion;
    uint32_t u32VdecStreamId;
    uint64_t phyVsyncBridgeExtAddr;
    uint32_t u32UniqueId;
    uint32_t u32Reserved[2];
    uint8_t u8AspectRate;
    uint8_t u8Interlace;
    uint8_t u8FrcMode;
    uint8_t u83DMode;
    uint8_t u8BottomFieldFirst;
    uint8_t u8FreezeThisFrame;
    uint8_t u8CurIndex;
    uint8_t u8ToggleTime;
    uint8_t u8IsNoWaiting;
    uint8_t u8MCUMode;
    uint8_t u8SeamlessDS;
    uint8_t u8FBLMode;
    uint8_t u8FieldCtrl;          // control one field mode, always top or bot when FF or FR
    uint8_t u8ApplicationType;
    uint8_t u83DLayout;           // 3D layout from SEI, the possible value is OMX_3D_LAYOUT enum in OMX_Video.h
    uint8_t u8ColorInXVYCC;
    uint8_t u8LowLatencyMode;     // for CTS or other application, drop new frame when render too fast
    uint8_t u8VdecComplexity;
    uint8_t u8StreamType;
    uint8_t u8Reserved[2];
    uint8_t u8HTLBTableId;
    uint8_t u8HTLBEntriesSize;
    uint8_t u8AFD;               //active frame code
    MS_HDRInfo sHDRInfo;
    DISP_FRM_INFO_EXT sDispFrmInfoExt;
    uint16_t u16MIUBandwidth;
    uint16_t u16Bitrate;
    uint32_t u32TileMode;
    uint64_t phyHTLBEntriesAddr;
} MS_DispFrameFormat;

#pragma pack(push, 4)
typedef struct
{
    uint32_t u32LumaAddr0;              ///< The start offset of luma data. Unit: byte.
    uint32_t u32ChromaAddr0;            ///< The start offset of chroma data. Unit: byte.
    uint32_t u32LumaAddr1;              ///< The start offset of luma data. Unit: byte.
    uint32_t u32ChromaAddr1;            ///< The start offset of chroma data. Unit: byte.
    uint32_t u32Idx;
    uint32_t u32PriData;
    uint32_t u32CodecType;
    uint16_t u16Pitch;
    uint16_t u16Width;
    uint16_t u16Height;
    uint16_t u16CropLeft;
    uint16_t u16CropRight;
    uint16_t u16CropBottom;
    uint16_t u16CropTop;
    uint8_t u1BottomFieldFirst:1;
    uint8_t u1DSIndex1Valid:1;
    uint8_t u2Reserved:6;
    uint8_t u8FieldType;              ///< HVD_Field_Type, none, top , bottom, both field
    uint8_t u8Interlace;
    uint8_t u8ColorFormat;            // 0 -> 420(16x32), 1 -> 422, 2 -> 420 10 bit, 3 -> 420(32x32)
    uint8_t u8FrameNum;               // if 2, u32LumaAddr1 and u32ChromaAddr1 should be use
    uint8_t u8RangeMapY;              // for VC1 or 10 BIT frame, 2 bit Y depth
    uint8_t u8RangeMapUV;             // for VC1 or 10 BIT frame, 2 bit UV depth
    uint8_t u8TB_toggle;              // 0 -> TOP then BOTTOM
    uint8_t u8Tog_Time;
    uint8_t u2Luma0Miu:2;
    uint8_t u2Luma1Miu:2;
    uint8_t u2Chroma0Miu:2;
    uint8_t u2Chroma1Miu:2;
    uint8_t u8FieldCtrl;              // 0-> Normal, 1->always top, 2->always bot
    union {
        uint8_t u8DSIndex;
        struct
        {
            uint8_t u4DSIndex0:4;
            uint8_t u4DSIndex1:4;     // it is DS index for sFrames[1] (HEVC Dolby EL frame)
        };
    };
    union {
        uint16_t u16Pitch1;           // for 10 BIT, the 2 bit frame buffer pitch
        uint16_t u16DispCnt;          // when this display queue is show finish, record the display conut for debug if frame repeat
    };
} DISP_FRM_INFO;

typedef struct
{
    // for vsync bridge dispQ bridge
    uint8_t u8DispQueNum;
    uint8_t u8McuDispSwitch;
    uint8_t u8McuDispQWPtr;
    uint8_t u8McuDispQRPtr;
    DISP_FRM_INFO McuDispQueue[MAX_VSYNC_BRIDGE_DISPQ_NUM];
    uint8_t u8DisableFDMask;
    uint8_t u8FdMaskField;            // last FD mask field, FW internal use.
    uint8_t u8ToggledTime;            // toggled time, FW internal use.
    uint8_t u8ToggleMethod;           // toggle method, when 1, it will repeat last field
    uint8_t u8InternalCtrl[2];
    uint8_t u5FRCMode:5;
    uint8_t u1FBLMode:1;
    uint8_t u2MirrorMode:2;
    uint8_t u8Reserve;
} MCU_DISPQ_INFO;

typedef struct
{
    uint8_t  u8Pattern[4];
    uint32_t u32Version;
    uint32_t u32Debug;
    uint16_t u16VsyncCnt;
    uint16_t u16Debug;
    DISP_FRM_INFO_EXT McuDispQueue[MAX_VSYNC_BRIDGE_DISPQ_NUM];
} MCU_DISPQ_INFO_EXT;

#pragma pack(pop)

/// the disp information
typedef struct
{
    /// display widnow
    MS_WINDOW  disp_win;
    /// crop window
    MS_WINDOW  crop_win;
    /// rotate information
    int     transform;
    /// osd width
    int     osdWidth;
    /// osd height
    int     osdHeight;
    /// discard SN zoom mode
    int     default_zoom_mode;
    /// using last setting, only trigger set win when timing change
    int     load_last_setting;
    /// layer zorder
    int     zorder;
    /// crop video by external(not from surfaceflinger)
    int     cropWinByExt;
    /// reserved
    int     reserved[2];
} MS_WinInfo;

#endif
