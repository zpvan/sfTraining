//<MStar Software>
//******************************************************************************
//MStar Software
//Copyright (c) 2010 - 2016 MStar Semiconductor, Inc. All rights reserved.
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

#define LOG_TAG "MM_GOPDisp"
//#define LOG_NDEBUG 0
#include <cutils/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "gop_display.h"
#include "vsync_bridge.h"
#include "dip.h"

#include <mmap.h>
#include <apiPNL.h>
#include <apiGOP.h>
#include <apiGFX.h>

#include <graphics.h>
#include <iniparser.h>

#define ROTATE_FLAG             (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_ROT_180 | HAL_TRANSFORM_ROT_270)
#define MAX_GOP_WIN_NUM         MAX_SIDEBAND_COUNT


#define GOP_FRAME_BUF_WIDTH     1920
#define GOP_FRAME_BUF_HEIGHT    1080

#define GOP_FRAME_BUF_NUM       3
#define GOP_GWIN_INDEX          5
#define GOP_INDEX               3

static pthread_mutex_t gop_lock = PTHREAD_MUTEX_INITIALIZER;
#define GOP_LOCK  pthread_mutex_lock(&gop_lock)
#define GOP_UNLOCK pthread_mutex_unlock(&gop_lock)

static MS_ColorFormat eGopBufFormat;
static Data_Fmt eDipBufFormat;
static GFX_Buffer_Format eGfxBufFormat;
static MS_U32 u32GopBufBPP;
static MS_BOOL bGopDispInit = FALSE;
static pthread_t GopThread;
static gop_win_info_t GopWin[MAX_GOP_WIN_NUM];
static int isGopRun = FALSE;
static MS_BOOL bNeedClearBuf = FALSE;
static void *diphandle;
static MS_PHY phyGopFrmBufAddr = NULL;
static MS_PHY phyGopFrmTempBufAddr = NULL;  // for scaling up use
static MS_U8 u8GWinId;
static MS_U8 u8VideoGop;
static MS_U32 u32GopWidth;
static MS_U32 u32GopHeight;
static MS_U32 u32LastGopWidth;
static MS_U32 u32LastGopHeight;
static MS_U32 u32GopFrameBufSize;
static MS_U32 u32GopFrameBufId;
//#define ALIGN_BYTES(value, base) (((value) + ((base) - 1)) & ~((base) - 1))
#define ALIGN_BYTES(value, base) ((value)&~((base) - 1))
#define DIP_WINDOW_WIDTH_ALIGN_NUM 16
#define HWCOMPOSER13_INI_FILENAME   "/system/etc/hwcomposer13.ini"

static int gop_get_avail_gwin_id(int gopNo) {
    if (gopNo < 0) {
        ALOGE("gop_get_avail_gwin_id the given gopNo is %d,check it!", gopNo);
        return -1;
    }
    int availGwinNum = 0;
    int i = 0;
    for (i; i < gopNo; i++) {
        availGwinNum += MApi_GOP_GWIN_GetGwinNum(i);
    }
    return availGwinNum;
}

static int getGopFrameBufferSize(void)
{
    return u32GopWidth * u32GopHeight * u32GopBufBPP;
}

static void gop_display_init(void)
{
    vsync_bridge_init();    // init driver

    int i;
    u32GopWidth = g_IPanel.Width();
    u32GopHeight = g_IPanel.Height();
    u32GopFrameBufSize = getGopFrameBufferSize();
    mmap_info_t* mmapInfo;
    GOP_InitInfo gopInitInfo;
    int panelWidth, panelHeight, panelHStart;
    GFX_Config gfx_config = {0,};
    dictionary *pHwcINI = NULL;

    memset(&gopInitInfo, 0, sizeof(GOP_InitInfo));

    if (!isGopRun) {
        //load hwcomposer13.ini file
        pHwcINI = iniparser_load(HWCOMPOSER13_INI_FILENAME);

        if (pHwcINI != NULL) {
            u8VideoGop = iniparser_getint(pHwcINI, "hwcomposer:HWC_gop_MMGRAPHICDISP", GOP_INDEX);
            ALOGD("u8VideoGop = %d", u8VideoGop);
            iniparser_freedict(pHwcINI);
        }
        u8GWinId = gop_get_avail_gwin_id(u8VideoGop);
    }
    ALOGD("u8GWinId = %d", u8GWinId);

    mmapInfo = mmap_get_info("E_MST_GOP_REGDMA");
    if (mmapInfo == NULL) {
        ALOGD("get E_MST_GOP_REGDMA fail");
        return;
    } else {
        gopInitInfo.u32GOPRegdmaAdr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        gopInitInfo.u32GOPRegdmaLen = mmapInfo->size;
        gopInitInfo.bEnableVsyncIntFlip = TRUE;
    }

    mmapInfo = mmap_get_info("E_MMAP_ID_MM_GRAPHIC_DISPLAY");
    if (mmapInfo == NULL) {
        ALOGD("get E_MMAP_ID_MM_GRAPHIC_DISPLAY fail");
        return;
    } else {
        // if memory not enough, change gop width and height to default GOP_FRAME_BUF_WIDTH  and GOP_FRAME_BUF_HEIGHT, then scaling up
        if (mmapInfo->size < (u32GopFrameBufSize * (GOP_FRAME_BUF_NUM + 1))) {
            if (u32GopWidth > GOP_FRAME_BUF_WIDTH) {
                u32GopWidth = GOP_FRAME_BUF_WIDTH;
            }
            if (u32GopHeight > GOP_FRAME_BUF_HEIGHT) {
                u32GopHeight = GOP_FRAME_BUF_HEIGHT;
            }
            u32GopFrameBufSize = getGopFrameBufferSize();
        }

        // default is ARGB8888, if memory not enough, change to RGB565

        if (mmapInfo->size < (u32GopFrameBufSize * (GOP_FRAME_BUF_NUM + 1))) {
            // not enough, change to RGB565
            eGopBufFormat = E_MS_FMT_RGB565;
            eDipBufFormat = E_FMT_RGB565;
            eGfxBufFormat = GFX_FMT_RGB565;
            u32GopBufBPP = 2;
            u32GopFrameBufSize = getGopFrameBufferSize();
        }

        if (mmapInfo->size < (u32GopFrameBufSize * GOP_FRAME_BUF_NUM)) {
            ALOGE("E_MMAP_ID_MM_GRAPHIC_DISPLAY size is is 0x%x not enough", mmapInfo->size);
            return;
        }

        phyGopFrmBufAddr = mmapInfo->addr;

        if (mmapInfo->size < (u32GopFrameBufSize * (GOP_FRAME_BUF_NUM + 1))) {
            // Not support scaling up, but still can run GOP display
            phyGopFrmTempBufAddr = NULL;
            ALOGE("get E_MMAP_ID_MM_GRAPHIC_DISPLAY size 0x%x is not enough, not support scaling up", mmapInfo->size);
        } else {
            phyGopFrmTempBufAddr = phyGopFrmBufAddr + (u32GopFrameBufSize * GOP_FRAME_BUF_NUM);
        }
    }

    ALOGD("gop_display_init eGopBufFormat = %d", eGopBufFormat);

    if (!bGopDispInit) {
        gfx_config.u8Miu = 0;
        gfx_config.u8Dither = FALSE;
        gfx_config.u32VCmdQSize = 0;
        gfx_config.u32VCmdQAddr = 0;
        gfx_config.bIsHK = 1;
        gfx_config.bIsCompt = FALSE;
        MApi_GFX_Init(&gfx_config);
    }

    panelWidth = g_IPanel.Width();
    panelHeight = g_IPanel.Height();
    panelHStart = g_IPanel.HStart();
    gopInitInfo.u16PanelWidth  = panelWidth;
    gopInitInfo.u16PanelHeight = panelHeight;
    gopInitInfo.u16PanelHStr   = panelHStart;

    MApi_GOP_InitByGOP(&gopInitInfo, u8VideoGop);
    MApi_GOP_MIUSel(u8VideoGop, mmapInfo->miuno);

    if (eGopBufFormat == E_MS_FMT_YUV422) {
        MApi_GOP_SetUVSwap(u8VideoGop, true);
    } else {
        MApi_GOP_SetUVSwap(u8VideoGop, false);
    }

    MApi_GOP_GWIN_Set_HSCALE_EX(u8VideoGop, TRUE, u32GopWidth, panelWidth);
    MApi_GOP_GWIN_Set_VSCALE_EX(u8VideoGop, TRUE, u32GopHeight, panelHeight);
    MApi_GOP_GWIN_Set_STRETCHWIN(u8VideoGop, E_GOP_DST_OP0, 0, 0, u32GopWidth, u32GopHeight);

    if (isGopRun) {
        MApi_GOP_GWIN_Destroy32FB(u32GopFrameBufId);
    }
    u32GopFrameBufId = MApi_GOP_GWIN_GetFree32FBID();
    MApi_GOP_GWIN_CreateFBbyStaticAddr(u32GopFrameBufId, 0, 0, u32GopWidth, u32GopHeight,  eGopBufFormat, phyGopFrmBufAddr);

    MApi_GOP_GWIN_MapFB2Win(u32GopFrameBufId, u8GWinId);
    MApi_GOP_GWIN_SetBlending(u8GWinId,
        (eGopBufFormat == E_MS_FMT_ARGB8888) ? true : false,
        (eGopBufFormat == E_MS_FMT_ARGB8888) ? 0 : 0xFF);

    MApi_GFX_BeginDraw();
    for (i = 0; i < GOP_FRAME_BUF_NUM; i++) {
        MApi_GFX_ClearFrameBufferByWord(phyGopFrmBufAddr + (i * u32GopFrameBufSize), u32GopFrameBufSize, 0);
    }
    MApi_GFX_EndDraw();

    MS_PNL_DST_DispInfo pDstInfo={1,1,1,1,1,1,1,1};
    MApi_PNL_GetDstInfo(&pDstInfo, sizeof(MS_PNL_DST_DispInfo));
    if (pDstInfo.bYUVOutput)
        MApi_GOP_GWIN_OutputColor_EX(u8VideoGop, GOPOUT_YUV);

    bGopDispInit = TRUE;
}

static void gop_display_swap_buf(MS_PHY phyBufAddr)
{
    /* swap buffer */
    MS_U16 u16QueueCnt = GOP_FRAME_BUF_NUM - 1;
    MS_U16 tagID;

    tagID = MApi_GFX_GetNextTAGID(TRUE);
    MApi_GFX_SetTAGID(tagID);
    if (MApi_GOP_Switch_GWIN_2_FB_BY_ADDR(u8GWinId, phyBufAddr, tagID, &u16QueueCnt) == FALSE) {
        ALOGD("MApi_GOP_Switch_GWIN_2_FB_BY_ADDR FAIL!!");
    }
}

static MS_BOOL gop_bitblt(GFX_BufferInfo *stSrcbuf, GFX_BufferInfo *stDstbuf, MS_BOOL bMirrorH, MS_BOOL bMirrorV, GFX_RotateAngle eAngle)
{
    MS_U32 u32GfxScale;
    GFX_DrawRect stBitbltInfo;
    GFX_Point stP0, stP1;

    MApi_GFX_BeginDraw();
    MApi_GFX_EnableDFBBlending(FALSE);
    MApi_GFX_EnableAlphaBlending(FALSE);
    MApi_GFX_SetAlphaSrcFrom(ABL_FROM_ASRC);

    if ((stSrcbuf->u32Width == stDstbuf->u32Width) && (stSrcbuf->u32Height == stDstbuf->u32Height)) {
        u32GfxScale = GFXDRAW_FLAG_DEFAULT;
    } else {
        u32GfxScale = GFXDRAW_FLAG_SCALE;
    }

    stP0.x = 0;
    stP0.y = 0;
    stP1.x = stDstbuf->u32Width;
    stP1.y = stDstbuf->u32Height;
    MApi_GFX_SetClip(&stP0, &stP1);

    if (MApi_GFX_SetSrcBufferInfo(stSrcbuf, 0) != GFX_SUCCESS) {
        ALOGE("GFX set SrcBuffer failed.");
        MApi_GFX_EndDraw();
        return FALSE;
    }

    if (MApi_GFX_SetDstBufferInfo(stDstbuf, 0) != GFX_SUCCESS) {
        ALOGE("GFX set DstBuffer failed.");
        MApi_GFX_EndDraw();
        return FALSE;
    }

    stBitbltInfo.srcblk.x = 0;
    stBitbltInfo.srcblk.y = 0;
    stBitbltInfo.srcblk.width = stSrcbuf->u32Width;
    stBitbltInfo.srcblk.height = stSrcbuf->u32Height;

    stBitbltInfo.dstblk.x = 0;
    stBitbltInfo.dstblk.y = 0;

    if (eAngle == GEROTATE_90 || eAngle == GEROTATE_270) {
        stBitbltInfo.dstblk.width = stDstbuf->u32Height;
        stBitbltInfo.dstblk.height = stDstbuf->u32Width;
    } else {
        stBitbltInfo.dstblk.width = stDstbuf->u32Width;
        stBitbltInfo.dstblk.height = stDstbuf->u32Height;
    }

    if (bMirrorH) {
        stBitbltInfo.srcblk.x += stBitbltInfo.srcblk.width - 1;
    }

    if (bMirrorV) {
        stBitbltInfo.srcblk.y += stBitbltInfo.srcblk.height - 1;
    }

    MApi_GFX_SetMirror(bMirrorH, bMirrorV);
    MApi_GFX_SetRotate(eAngle);
    if (MApi_GFX_BitBlt(&stBitbltInfo, u32GfxScale) != GFX_SUCCESS) {
        MApi_GFX_EndDraw();
        ALOGE("GFX BitBlt failed.");
        return FALSE;
    }

    MApi_GFX_SetRotate(GEROTATE_0);

    if (MApi_GFX_FlushQueue() != GFX_SUCCESS) {
        MApi_GFX_EndDraw();
        ALOGE("GFX FlushQueue failed.");
        return FALSE;
    }
    MApi_GFX_EndDraw();

    return TRUE;
}

static void Swap(int* array, int x, int y)
{
    int temp = array[x];
    array[x] = array[y];
    array[y] = temp;
}

static void *Gop_thread(void *pObj)
{
    ALOGD("enter GOP_thread");
#ifdef SUPPORT_NWAY_DIP
    int i, dip_num = 0, gop_buf_index = 0, clear_cnt = 0;
    Dip_Vdecframe_Info st_dip_frame_info[MAX_GOP_WIN_NUM] = {0, };
    gop_win_frame_t st_sideband_frame[MAX_GOP_WIN_NUM] = {0, };
    MS_WinInfo last_wininfo[MAX_GOP_WIN_NUM] = {0, };
    MS_BOOL bNeedScaling[MAX_GOP_WIN_NUM] = {0, };
    MS_U32 u32TimeCurr = 0, u32TimeLast = 0, u32CurrFrameCnt = 0, u32LastFrameCnt = 0;
    MS_PHY phyCurrGopBufAddr = phyGopFrmBufAddr;
    MS_PHY phyLastGopBufAddr = phyGopFrmBufAddr;
    int zorder[MAX_GOP_WIN_NUM] = {0, };
    int fire_order[MAX_GOP_WIN_NUM] = {0, };

    while (isGopRun) {
        dip_num = 0;
        GOP_LOCK;
        for (i = 0; i < MAX_GOP_WIN_NUM; i++) {

            bNeedScaling[i] = FALSE;
            fire_order[i] = i;

            if (GopWin[i].status == E_STATUS_UNREGISTERED || GopWin[i].callback == NULL) {
                st_dip_frame_info[i].bGetFrame = FALSE;
                memset(&st_sideband_frame[i], 0, sizeof(gop_win_frame_t));
                continue;
            }

            int rtn = 0;
            rtn = GopWin[i].callback(GopWin[i].pCtx, &st_sideband_frame[i]);

            if (rtn == E_SIDEBANDQUEUE_UNLOAD) {
                st_dip_frame_info[i].bGetFrame = FALSE;
                continue;
            }

            if ((rtn == E_SIDEBANDQUEUE_RENDERED) || st_dip_frame_info[i].bGetFrame) {
                if ((u32LastGopWidth != g_IPanel.Width()) ||
                    (u32LastGopHeight != g_IPanel.Height())) {
                    if (u32LastGopWidth > 0 && u32LastGopHeight > 0) {
                        gop_display_init();
                    }
                    u32LastGopWidth = g_IPanel.Width();
                    u32LastGopHeight = g_IPanel.Height();
                }

                ALOGV("Gop_thread callback stream %d u64Pts = %lld [%d %d %d %d]", i , st_sideband_frame[i].dff.u64Pts,
                    st_sideband_frame[i].wininfo.disp_win.x, st_sideband_frame[i].wininfo.disp_win.y,
                    st_sideband_frame[i].wininfo.disp_win.width, st_sideband_frame[i].wininfo.disp_win.height);
                if (st_sideband_frame[i].wininfo.disp_win.x >= 0
                    && st_sideband_frame[i].wininfo.disp_win.y >= 0
                    && st_sideband_frame[i].wininfo.disp_win.width > 0
                    && st_sideband_frame[i].wininfo.disp_win.height > 0
                    && ((st_sideband_frame[i].wininfo.disp_win.x + st_sideband_frame[i].wininfo.disp_win.width) <= u32GopWidth)
                    && ((st_sideband_frame[i].wininfo.disp_win.y + st_sideband_frame[i].wininfo.disp_win.height) <= u32GopHeight)) {

                    dip_num++;

                    if (st_sideband_frame[i].dff.sFrames[0].eColorFormat == MS_COLOR_FORMAT_YUYV) {
                        st_dip_frame_info[i].eDIPRFmt = E_FMT_YUV422;
                    } else if (st_sideband_frame[i].dff.sFrames[0].eColorFormat == MS_COLOR_FORMAT_HW_EVD) {
                        st_dip_frame_info[i].eDIPRFmt = E_FMT_YUV420_H265;
                    } else if (st_sideband_frame[i].dff.sFrames[0].eColorFormat == MS_COLOR_FORMAT_HW_32x32) {
                        st_dip_frame_info[i].eDIPRFmt = E_FMT_YUV420_32x32;
                    } else {
                        st_dip_frame_info[i].eDIPRFmt = E_FMT_YUV420_16x32;
                    }

                    if (st_sideband_frame[i].dff.u8Interlace) {
                        if (st_sideband_frame[i].dff.u8BottomFieldFirst) {
                            st_dip_frame_info[i].eFieldType = E_BOTTOM_FIRST;
                        } else {
                            st_dip_frame_info[i].eFieldType = E_TOP_FIRST;
                        }
                    } else {
                        st_dip_frame_info[i].eFieldType = E_PROGRESSVIE;
                    }

                    st_dip_frame_info[i].FrameFormat = st_sideband_frame[i].dff.sFrames[0];
                    st_dip_frame_info[i].eDIPWFmt = eDipBufFormat;
                    st_dip_frame_info[i].u32DispX = st_sideband_frame[i].wininfo.disp_win.x * u32GopWidth / st_sideband_frame[i].wininfo.osdWidth;
                    st_dip_frame_info[i].u32DispWidth = ALIGN_BYTES(st_sideband_frame[i].wininfo.disp_win.width * u32GopWidth / st_sideband_frame[i].wininfo.osdWidth, DIP_WINDOW_WIDTH_ALIGN_NUM);
                    st_dip_frame_info[i].u32DispY = st_sideband_frame[i].wininfo.disp_win.y * u32GopHeight / st_sideband_frame[i].wininfo.osdHeight;
                    st_dip_frame_info[i].u32DispHeight = st_sideband_frame[i].wininfo.disp_win.height * u32GopHeight / st_sideband_frame[i].wininfo.osdHeight;
                    st_dip_frame_info[i].phyLumaAddr_Bottom = st_sideband_frame[i].dff.sDispFrmInfoExt.u32LumaAddrExt[MS_DISP_FRM_INFO_EXT_TYPE_INTERLACE];
                    st_dip_frame_info[i].phyChromaAddr_Bottom = st_sideband_frame[i].dff.sDispFrmInfoExt.u32ChromaAddrExt[MS_DISP_FRM_INFO_EXT_TYPE_INTERLACE];

                    if (((st_dip_frame_info[i].u32DispWidth > st_dip_frame_info[i].FrameFormat.u32Width)
                        || (st_dip_frame_info[i].u32DispHeight > st_dip_frame_info[i].FrameFormat.u32Height)
                        || st_sideband_frame[i].wininfo.transform & ROTATE_FLAG
                        || (st_dip_frame_info[i].u32DispWidth != ALIGN_BYTES(st_dip_frame_info[i].u32DispWidth, DIP_WINDOW_WIDTH_ALIGN_NUM)))
                        && (phyGopFrmTempBufAddr != NULL)) {
                        ALOGV("stream [%d] need scaling [%d %d] [%d %d] crop:[%d %d][%d %d] transform:[%d]", i,
                            st_dip_frame_info[i].u32DispWidth, st_dip_frame_info[i].u32DispHeight,
                            st_dip_frame_info[i].FrameFormat.u32Width, st_dip_frame_info[i].FrameFormat.u32Height,
                            st_dip_frame_info[i].FrameFormat.u32CropLeft,st_dip_frame_info[i].FrameFormat.u32CropRight,
                            st_dip_frame_info[i].FrameFormat.u32CropTop,st_dip_frame_info[i].FrameFormat.u32CropBottom,
                            st_sideband_frame[i].wininfo.transform);
                        bNeedScaling[i] = TRUE;

                        MS_U32 real_width = st_dip_frame_info[i].FrameFormat.u32Width
                                            - st_dip_frame_info[i].FrameFormat.u32CropRight
                                            - st_dip_frame_info[i].FrameFormat.u32CropLeft;
                        MS_U32 real_height = st_dip_frame_info[i].FrameFormat.u32Height
                                            - st_dip_frame_info[i].FrameFormat.u32CropTop
                                            - st_dip_frame_info[i].FrameFormat.u32CropBottom;

                        st_dip_frame_info[i].u32DispX = 0;
                        st_dip_frame_info[i].u32DispY = 0;
                        st_dip_frame_info[i].u32DispWidth = ALIGN_BYTES((real_width > u32GopWidth) ? u32GopWidth : real_width, DIP_WINDOW_WIDTH_ALIGN_NUM);
                        st_dip_frame_info[i].u32DispHeight = (real_height > u32GopHeight) ? u32GopHeight : real_height;
                    }

                    if (bNeedScaling[i]) {
                        // DIP only can scaling down. If needs scaling up, needs DIP to a temp buf first then using GE scaling up.
                        st_dip_frame_info[i].phyDIPStartMemAddr = phyGopFrmTempBufAddr;
                    } else {
                        st_dip_frame_info[i].phyDIPStartMemAddr = phyCurrGopBufAddr;
                    }
                    zorder[i] = st_sideband_frame[i].wininfo.zorder;

                    st_dip_frame_info[i].u32DIPTotalWidth = u32GopWidth;
                    st_dip_frame_info[i].u32DIPTotalHeight = u32GopHeight;
                    st_dip_frame_info[i].u32DIPLine = u32GopWidth;
                    st_dip_frame_info[i].bGetFrame = TRUE;
                } else {
                    ALOGW("Gop_thread callback stream %d u64Pts = %lld abnornal win[%d %d %d %d]", i , st_sideband_frame[i].dff.u64Pts,
                        st_sideband_frame[i].wininfo.disp_win.x, st_sideband_frame[i].wininfo.disp_win.y,
                        st_sideband_frame[i].wininfo.disp_win.width, st_sideband_frame[i].wininfo.disp_win.height);
                    bNeedClearBuf = TRUE;
                }

            } else {
                ALOGV("stream %d Gop_thread callback rtn = %d", i, rtn);
            }

            if (st_dip_frame_info[i].bGetFrame) {
                if (memcmp(&last_wininfo[i], &st_sideband_frame[i].wininfo, sizeof(MS_WinInfo))) {
                    bNeedClearBuf = TRUE;
                }
                last_wininfo[i] = st_sideband_frame[i].wininfo;
            }
        }

        if (bNeedClearBuf) {
            // needs to clear next buffer
            clear_cnt = GOP_FRAME_BUF_NUM;
            bNeedClearBuf = FALSE;
        }
        GOP_UNLOCK;

        if (dip_num) {

            if (clear_cnt) {
                MApi_GFX_BeginDraw();
                MApi_GFX_ClearFrameBufferByWord(phyCurrGopBufAddr, u32GopFrameBufSize, 0);
                MS_U16 u16TagID = MApi_GFX_SetNextTAGID();
                MApi_GFX_WaitForTAGID(u16TagID);
                MApi_GFX_EndDraw();
                clear_cnt--;
            }

            // decide DIP fire order
            for (i = 0; i < MAX_GOP_WIN_NUM; i++) {
                int j;
                for(j = 1; j < MAX_GOP_WIN_NUM - i; j++) {
                    if (zorder[j] < zorder[j - 1]) {
                        Swap(&zorder, j, j - 1);
                        Swap(&fire_order, j, j - 1);
                    }
                }
            }

            for (i = 0; i < MAX_GOP_WIN_NUM; i++) {

                if (!st_dip_frame_info[fire_order[i]].bGetFrame) {
                    continue;
                }

                ALOGV("Gop_thread DIP_CaptureNWayFrame stream[%d]:[%d %d --> %d %d]", i , st_dip_frame_info[fire_order[i]].FrameFormat.u32Width, st_dip_frame_info[fire_order[i]].FrameFormat.u32Height, st_dip_frame_info[fire_order[i]].u32DispWidth, st_dip_frame_info[fire_order[i]].u32DispHeight);
                if (E_OK != DIP_CaptureNWayFrame(diphandle, &st_dip_frame_info[fire_order[i]], 1)) {
                    ALOGE("DIP_CaptureNWayFrame fail");
                }

                if (bNeedScaling[fire_order[i]]) {
                    GFX_BufferInfo srcbuf, dstbuf;
                    GFX_RotateAngle eAngle = GEROTATE_0;

                    srcbuf.u32ColorFmt = eGfxBufFormat;
                    srcbuf.u32Addr = st_dip_frame_info[fire_order[i]].phyDIPStartMemAddr;

                    srcbuf.u32Pitch = u32GopWidth * u32GopBufBPP;

                    srcbuf.u32Width = ALIGN_BYTES(st_dip_frame_info[fire_order[i]].u32DispWidth, DIP_WINDOW_WIDTH_ALIGN_NUM);
                    srcbuf.u32Height = st_dip_frame_info[fire_order[i]].u32DispHeight;

                    dstbuf.u32ColorFmt = eGfxBufFormat;

                    MS_WINDOW real_disp_win;
                    real_disp_win.x = st_sideband_frame[fire_order[i]].wininfo.disp_win.x * u32GopWidth / st_sideband_frame[i].wininfo.osdWidth;
                    real_disp_win.width = st_sideband_frame[fire_order[i]].wininfo.disp_win.width * u32GopWidth / st_sideband_frame[i].wininfo.osdWidth;
                    real_disp_win.y = st_sideband_frame[fire_order[i]].wininfo.disp_win.y * u32GopHeight / st_sideband_frame[i].wininfo.osdHeight;
                    real_disp_win.height = st_sideband_frame[fire_order[i]].wininfo.disp_win.height * u32GopHeight / st_sideband_frame[i].wininfo.osdHeight;

                    dstbuf.u32Addr = phyCurrGopBufAddr +
                                    + (real_disp_win.x * u32GopBufBPP)
                                    + (real_disp_win.y * u32GopBufBPP * u32GopWidth);

                    dstbuf.u32Pitch = u32GopWidth * u32GopBufBPP;
                    dstbuf.u32Width = real_disp_win.width;
                    dstbuf.u32Height = real_disp_win.height;

                    if (st_sideband_frame[fire_order[i]].wininfo.transform == HAL_TRANSFORM_ROT_90) {
                        eAngle = GEROTATE_90;
                    } else if (st_sideband_frame[fire_order[i]].wininfo.transform == HAL_TRANSFORM_ROT_270) {
                        eAngle = GEROTATE_270;
                    } else if (st_sideband_frame[fire_order[i]].wininfo.transform == HAL_TRANSFORM_ROT_180) {
                        eAngle = GEROTATE_180;
                    }

                    ALOGV("Gop_thread gop_bitblt stream[%d]:[%d %d --> %d %d]", i , srcbuf.u32Width, srcbuf.u32Height, dstbuf.u32Width, dstbuf.u32Height);

                    if (FALSE == gop_bitblt(&srcbuf, &dstbuf, FALSE, FALSE, eAngle)) {
                        ALOGE("gop_bitblt fail");
                    }
                }
            }

            // swap to new gop buf
            gop_display_swap_buf(phyCurrGopBufAddr);

            // update next gop buffer
            phyLastGopBufAddr = phyCurrGopBufAddr;
            gop_buf_index = ((gop_buf_index + 1) % GOP_FRAME_BUF_NUM);
            phyCurrGopBufAddr = phyGopFrmBufAddr + (gop_buf_index * u32GopFrameBufSize);

            u32TimeCurr = MsOS_GetSystemTime();
            u32CurrFrameCnt++;
            if (u32TimeCurr - u32TimeLast >= 1000) {
                ALOGD("MM GOP display %d DIP way, rate %.2f fps", dip_num, (((float)(u32CurrFrameCnt - u32LastFrameCnt) * 1000) / (u32TimeCurr - u32TimeLast)));
                u32LastFrameCnt = u32CurrFrameCnt;
                u32TimeLast = u32TimeCurr;
            }
        }
        else
        {
            usleep(16000);
        }
    }
#endif
    ALOGD("leave GOP_thread");
    return NULL;
}

EN_GOP_WIN_RET MS_Video_Disp_GOP_Win_Register(void** handle, void* pCont, gopwin_get_frame callback)
{
#ifdef SUPPORT_NWAY_DIP
    ALOGD("MS_Video_Disp_GOP_Win_Register pCont = 0x%x", pCont);
    int i;
    GOP_LOCK;
    if (!isGopRun) {
        Dip_ErrType ret = E_OK;

        u32LastGopWidth  = 0;
        u32LastGopHeight = 0;
        u8VideoGop = GOP_INDEX;
        u32GopFrameBufId = 0;
        eGopBufFormat = E_MS_FMT_ARGB8888;
        eDipBufFormat = E_FMT_ARGB8888;
        eGfxBufFormat = GFX_FMT_ARGB8888;
        u32GopBufBPP = 4;  //default ARGB8888
        gop_display_init();
        Dip_Output_Info dipOutput = {eDipBufFormat, 0, E_NWAY, NULL, NULL};
        diphandle = NULL;

        if (!phyGopFrmBufAddr) {
            ALOGE("phyGopFrmBufAddr is NULL");
            return E_GOP_WIN_FAIL;
        }
        MApi_GOP_GWIN_Enable(u8GWinId, true);

        ret = DIP_Init(&diphandle, dipOutput);

        if (ret != E_OK) {
            ALOGE("DIP_Init fail %d", ret);
            return E_GOP_WIN_FAIL;
        }

        for (i = 0; i < MAX_GOP_WIN_NUM; i++) {
            GopWin[i].callback = NULL;
            GopWin[i].pCtx = NULL;
            GopWin[i].status = E_STATUS_UNREGISTERED;
        }
        isGopRun = TRUE;
        pthread_create(&GopThread, NULL, Gop_thread, NULL);
    }

    for (i = 0; i < MAX_GOP_WIN_NUM; i++) {
        if (GopWin[i].status == E_STATUS_UNREGISTERED) {
            GopWin[i].callback = callback;
            GopWin[i].pCtx = pCont;
            GopWin[i].status = E_STATUS_REGISTERED;
            *handle = i + 1; // return 1 ~ MAX_GOP_WIN_NUM
            bNeedClearBuf = TRUE;
            GOP_UNLOCK;
            return E_GOP_WIN_OK;
        }
    }
    GOP_UNLOCK;

#else
    ALOGE("Error not SUPPORT_NWAY_DIP !!!");
#endif

    return E_GOP_WIN_FAIL;
}

EN_GOP_WIN_RET MS_Video_Disp_GOP_Win_Unregister(void* handle)
{
#ifdef SUPPORT_NWAY_DIP
    ALOGD("MS_Video_Disp_GOP_Win_Unregister handle = 0x%x", handle);
    int close_gop = MAX_GOP_WIN_NUM;
    int i = handle;

    if (i < 1 || i > MAX_GOP_WIN_NUM) {
        return E_GOP_WIN_FAIL;
    }
    i -= 1; // change to 0 ~ (MAX_GOP_WIN_NUM - 1)

    GOP_LOCK;
    if (GopWin[i].status == E_STATUS_UNREGISTERED) {
        GOP_UNLOCK;
        return E_GOP_WIN_FAIL;
    }
    GopWin[i].callback = NULL;
    GopWin[i].status = E_STATUS_UNREGISTERED;
    GopWin[i].pCtx = NULL;
    for (i = 0; i < MAX_GOP_WIN_NUM; i++) {
        if (GopWin[i].status == E_STATUS_UNREGISTERED) {
            close_gop --;
        }
    }
    bNeedClearBuf = TRUE;
    if (close_gop == 0) {
        isGopRun = FALSE;
        MApi_GOP_GWIN_Enable(u8GWinId, false);
    }
    GOP_UNLOCK;

    if (!isGopRun && (close_gop == 0)) {
        pthread_join(GopThread, NULL);
        MApi_GFX_BeginDraw();
        for (i = 0; i < GOP_FRAME_BUF_NUM; i++) {
            MApi_GFX_ClearFrameBufferByWord(phyGopFrmBufAddr + (i * u32GopFrameBufSize), u32GopFrameBufSize, 0);
        }
        MApi_GFX_EndDraw();
        DIP_DeInit(diphandle);
        MApi_GOP_GWIN_Destroy32FB(u32GopFrameBufId);
    }
    return E_GOP_WIN_OK;
#else
    ALOGE("Error not SUPPORT_NWAY_DIP !!!");
    return E_GOP_WIN_FAIL;
#endif
}
