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

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>


#include <utils/Errors.h>
#include <gralloc_priv.h>
#include <sync/sync.h>
//#include <sync/sw_sync.h>
#include <sw_sync.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include <assert.h>
#include <stdlib.h>

#include <utils/Timers.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <cutils/properties.h>

/* mstar header */
#include <mmap.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <apiXC.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <drvCMAPool.h>

#include "hwcomposer_context.h"
#include "hwcomposer_util.h"
#include "hwcomposer_gop.h"
#include "hwcursordev.h"
#include "fbdev.h"

static int hwcursordstfb[GAMECURSOR_COUNT] = {0, 0};
static MS_PHY hwCursorDstAddr[GAMECURSOR_COUNT][HWCURSOR_DSTFB_NUM] = {{0,0}, {0,0}};

static pthread_t work_thread[GAMECURSOR_COUNT] = {-1, -1};
static pthread_mutex_t global_mutex[GAMECURSOR_COUNT] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
static pthread_cond_t hwcursor_cond[GAMECURSOR_COUNT] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};

//used to identify whether workthread exit
static unsigned char gWorkthreadExit[GAMECURSOR_COUNT] = {1, 1};
static pthread_mutex_t gWorkthredExit_mutex[GAMECURSOR_COUNT] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

#define BYTE_PER_PIXEL 4
#define GOP_TIMEOUT_CNT 100

static void hwCursor_showfps()
{
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("mstar.hwcursor.showfps", value, "false");
    if (strcmp(value, "true") == 0) {
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
            ALOGD("show hwcursor fps is %f",fps);
        }
    }
}

unsigned int hwcursordev_getMaskPhyAdr(int u8MiuSel) {
    unsigned int miu0Len = mmap_get_miu0_length();
    unsigned int miu1Len = mmap_get_miu1_length();
    unsigned int miu2Len = mmap_get_miu2_length();

    if (u8MiuSel==2) {
        ALOGI("hwcursordev_getMaskPhyAdr MIU2 mask=%x", (miu2Len-1));
        return(miu2Len - 1) ;
    } else if (u8MiuSel==1) {
        ALOGI("hwcursordev_getMaskPhyAdr MIU1 mask=%x", (miu1Len-1));
        return(miu1Len - 1);
    } else {
        ALOGI("hwcursordev_getMaskPhyAdr MIU0 mask=%x", (miu0Len-1));
        return(miu0Len - 1) ;
    }
}

extern  int hwCursorDoTransaction(hwc_context_t *ctx, struct HwCursorInfo_t *transaction);

static void* hwCursor_workthread(void *data) {
    HwCursorInfo_t *cursorInfo = (HwCursorInfo_t*)data;
    int devNo = cursorInfo->gameCursorIndex;
    hwc_context_t *ctx = (hwc_context_t*)cursorInfo->pCtx;
    HwCursorInfo_t transaction;
    unsigned char bGameCursorExit = 0;
    //set thread name as hwCursor_workthread
    char thread_name[64] = {0};
    snprintf(thread_name, 64, "hwCursor%d_thread", devNo);
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);
    const int64_t reltime = 16000000;
    while (1) {
        pthread_mutex_lock(&gWorkthredExit_mutex[devNo]);
        bGameCursorExit = gWorkthreadExit[devNo];
        pthread_mutex_unlock(&gWorkthredExit_mutex[devNo]);
        if (bGameCursorExit) {
            ALOGI("hwCursor_workthread quit devNo=%d",devNo);
            break;
        }
        struct timeval t;
        struct timespec ts;
        pthread_mutex_lock(&global_mutex[devNo]);
        gettimeofday(&t, NULL);
        ts.tv_sec = t.tv_sec;
        ts.tv_nsec= t.tv_usec*1000;
        ts.tv_sec += reltime/1000000000;
        ts.tv_nsec+= reltime%1000000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec  += 1;
        }
        pthread_cond_timedwait(&hwcursor_cond[devNo], &global_mutex[devNo], &ts);
        memcpy(&transaction, &ctx->hwCusorInfo[devNo], sizeof(HwCursorInfo_t));
        ctx->hwCusorInfo[devNo].operation = 0;
        pthread_mutex_unlock(&global_mutex[devNo]);
        if (transaction.operation != 0) {
            hwCursorDoTransaction(ctx, &transaction);
        }
    }
    return NULL;
}

int hwcursordev_init(hwc_context_t* ctx, int32_t devNo) {
    if (ctx->hwCusorInfo[devNo].bInitialized) {
        ALOGD("hwcursordev_init has done, devNo=%d", devNo);
        if (gWorkthreadExit[devNo]) {
            gWorkthreadExit[devNo] = 0;
            ALOGD("hwCursor_workthread created again");
            pthread_create(&work_thread[devNo], NULL, hwCursor_workthread, &(ctx->hwCusorInfo[devNo]));
        }
        return 0;
    }
    const mmap_info_t* mmapInfo = NULL;
    MS_PHY gopAddr = 0;
    MS_PHY gopLen = 0;
    MS_PHY hwCursorAddr = 0;
    MS_PHY hwCursorLen = HWCURSOR_MAX_WIDTH*HWCURSOR_MAX_HEIGHT*BYTE_PER_PIXEL*3;// 3:three framebuffer
    void * hwCursorVaddr = NULL;
    char value[PROPERTY_VALUE_MAX] = {0};
    int miu_sel =  0;
    int cmaEnable = 0;
    MS_BOOL ret = FALSE;
    struct CMA_Pool_Init_Param CMA_Pool_Init_PARAM;
    struct CMA_Pool_Alloc_Param CMA_Alloc_PARAM;

    mmapInfo = mmap_get_info("E_MMAP_ID_HW_CURSOR");
    if (mmapInfo == NULL) {
        mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
        if (mmapInfo == NULL)
            return -1;
        gopAddr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        gopLen = mmapInfo->size;
        hwCursorAddr = gopAddr + gopLen - GAMECURSOR_COUNT * ((hwCursorLen+4095)&~4095U);
    } else {
        hwCursorAddr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
    }
    if (mmapInfo->cmahid > 0) {
        cmaEnable = 1;
    }

    ctx->CursorPhyAddr[devNo] = hwCursorAddr + devNo * hwCursorLen;
    for (int i=0; i<HWCURSOR_DSTFB_NUM; i++) {
        hwCursorDstAddr[devNo][i] = ctx->CursorPhyAddr[devNo] + hwCursorLen/3 * (i + 1);
    }
    // get gop & gwin
    MS_U8 gopNum = MApi_GOP_GWIN_GetMaxGOPNum();
    ctx->hwCusorInfo[devNo].gop = gopNum - 1 - devNo;
    ctx->hwCusorInfo[devNo].gwin = GOP_getAvailGwinId(ctx->hwCusorInfo[devNo].gop);

    if (hwCursorAddr >= mmap_get_miu1_offset()) {
        miu_sel = 2;
    } else if (hwCursorAddr >= mmap_get_miu0_offset()) {
        miu_sel = 1;
    } else {
        miu_sel = 0;
    }

    ctx->hwCusorInfo[devNo].gop_dest = GOP_getDestination(ctx);
    ctx->hwCusorInfo[devNo].channel = miu_sel;
    ctx->hwCusorInfo[devNo].releaseFenceFd = -1;
    ctx->hwCusorInfo[devNo].releaseSyncTimelineFd= sw_sync_timeline_create();
    if (ctx->hwCusorInfo[devNo].releaseSyncTimelineFd < 0) {
        ALOGE("sw_sync_timeline_create failed");
    }

    GOP_init(ctx, ctx->hwCusorInfo[devNo]);

    if (cmaEnable==1) {
        ALOGI("MMAP_GOP_FB was integrated into CMA init it!!");
        void* fbVaddr = fbdev_mmap(0, gopLen, PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
        hwCursorVaddr =  (void*)((MS_PHY)fbVaddr + gopLen - (GAMECURSOR_COUNT-devNo) * ((hwCursorLen+4095)&~4095U));
    } else {
        unsigned int maskPhyAddr =  hwcursordev_getMaskPhyAdr(miu_sel);
        if (miu_sel==2) {
            if (!MsOS_MPool_Mapping(2, (hwCursorAddr & maskPhyAddr), ((hwCursorLen+4095)&~4095U), 1)) {
                ALOGE("hwcursordev_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            hwCursorVaddr = (void*)MsOS_PA2KSEG1((hwCursorAddr & maskPhyAddr) | mmap_get_miu1_offset());
        }else if (miu_sel==1){
            if (!MsOS_MPool_Mapping(1, (hwCursorAddr & maskPhyAddr), ((hwCursorLen+4095)&~4095U), 1)) {
                ALOGE("hwcursordev_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            hwCursorVaddr = (void*)MsOS_PA2KSEG1((hwCursorAddr & maskPhyAddr) | mmap_get_miu0_offset());
        }else {
            if (!MsOS_MPool_Mapping(0, (hwCursorAddr & maskPhyAddr), ((hwCursorLen+4095)&~4095U), 1)) {
                ALOGE("hwcursordev_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            hwCursorVaddr = (void*)MsOS_PA2KSEG1(hwCursorAddr & maskPhyAddr);
        }
    }
    memset(hwCursorVaddr, 0x0, hwCursorLen);
    ctx->CursorVaddr[devNo] = (uintptr_t)hwCursorVaddr;
    ctx->hwCusorInfo[devNo].gameCursorIndex = devNo;
    ctx->hwCusorInfo[devNo].pCtx = (uintptr_t)ctx;

    pthread_create(&work_thread[devNo], NULL, hwCursor_workthread, &(ctx->hwCusorInfo[devNo]));
    gWorkthreadExit[devNo] = 0;
    ctx->hwCusorInfo[devNo].bInitialized = true;
    return 0;
}

int hwcursordev_deinit(hwc_context_t* ctx, int32_t devNo) {
    void* status;
    if (ctx->hwCusorInfo[devNo].bInitialized) {
        if (gWorkthreadExit[devNo] != 1) {
            pthread_mutex_lock(&gWorkthredExit_mutex[devNo]);
            gWorkthreadExit[devNo] = 1;
            pthread_mutex_unlock(&gWorkthredExit_mutex[devNo]);
            pthread_join(work_thread[devNo], &status);
            work_thread[devNo] = -1;
        }
        MApi_GOP_GWIN_Enable(ctx->hwCusorInfo[devNo].gwin, FALSE);
        return 1;
    }
    return 0;
}

static int hwCursorBlitToFB(struct HwCursorInfo_t *transaction, int osdMode) {
    int devNo = transaction->gameCursorIndex;
    GFX_BufferInfo srcbuf, dstbuf;
    GFX_Point v0, v1;
    GFX_DrawRect bitbltInfo;
    GFX_RgbColor color={0x0, 0x0, 0x0, 0x0};

    if ((transaction->dstWidth > HWCURSOR_MAX_WIDTH)
        ||(transaction->dstHeight > HWCURSOR_MAX_HEIGHT)
        || (transaction->srcWidth > HWCURSOR_MAX_WIDTH)
        ||(transaction->srcHeight> HWCURSOR_MAX_HEIGHT)) {
        ALOGD("hwCursorBlitToFB size setting is error!!");
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
        ALOGE("hwCursorBlitToFB GFX set SrcBuffer failed\n");
        goto HW_COPYICON_ERROR;
    }

    dstbuf.u32ColorFmt = GFX_FMT_ARGB8888;
    dstbuf.u32Addr = hwCursorDstAddr[devNo][hwcursordstfb[devNo]];
    dstbuf.u32Width = transaction->dstWidth;
    dstbuf.u32Height = transaction->dstHeight;
    dstbuf.u32Pitch = HWCURSOR_MAX_WIDTH * BYTE_PER_PIXEL;
    if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
        ALOGE("hwCursorBlitToFB GFX set DetBuffer failed\n");
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
        ALOGE("hwCursorBlitToFB GFX BitBlt failed\n");
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

static int hwCusorSetPosition(struct HwCursorInfo_t *transaction, int osdMode) {
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


static void hwCursorConfigGop(struct HwCursorInfo_t *transaction, int osdMode) {
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
    int devNo = transaction->gameCursorIndex;
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
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
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
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = U32SubFbid = fbid;
            mode = E_GOP_3D_SWITH_BY_FRAME;
        }
        break;
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_LEFTRIGHT_FULL:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth>>1, bufferHeight, gfmt,
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
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
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
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
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = fbid;
            U32SubFbid = MAX_GWIN_FB_SUPPORT;
            mode = E_GOP_3D_DISABLE;
        }
        break;
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight>>1, gfmt,
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
            U32MainFbid = fbid;
            U32SubFbid = MAX_GWIN_FB_SUPPORT;
            mode = E_GOP_3D_DISABLE;
        }
        break;
        case DISPLAYMODE_NORMAL:
        default:
        {
            MApi_GOP_GWIN_Create32FBFrom3rdSurf(bufferWidth, bufferHeight, gfmt,
                                                hwCursorDstAddr[devNo][hwcursordstfb[devNo]], pitch, &fbid);
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

    GOP_getCurOPTiming(NULL,NULL,&panelHStart);
    MApi_GOP_SetGOPHStart(gopNo, panelHStart);

    MApi_GOP_Set3DOSDMode(gwinId, U32MainFbid, U32SubFbid, mode);
    GOP_setStretchWindow(gopNo, osdWidth, osdHeight, 0, 0, panelWidth, panelHeight);
    hwCusorSetPosition(transaction, osdMode);
}

int hwCursorDoTransaction(hwc_context_t *ctx, struct HwCursorInfo_t *transaction) {
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
    int devNo = transaction->gameCursorIndex;
    if (property_get("mstar.desk-display-mode", property, NULL) > 0) {
        osdMode = atoi(property);
    }
    //MApi_GOP_GWIN_UpdateRegOnceEx2(gopNo, TRUE, FALSE);
    MApi_GOP_GWIN_UpdateRegOnceByMask(GOP_getBitMask(gopNo),TRUE, FALSE);
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
                ALOGE("acquirefence sync_wait failed in hwcursor work thread");
            }
            close(transaction->acquireFenceFd);
            transaction->acquireFenceFd = -1;
        }
        //Blit it and create FB
        hwcursordstfb[devNo] = (hwcursordstfb[devNo] + 1) % HWCURSOR_DSTFB_NUM;
        hwCursorBlitToFB(transaction, osdMode);
        hwCursorConfigGop(transaction, osdMode);
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
            if (!MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(transaction->gwin, hwCursorDstAddr[devNo][hwcursordstfb[devNo]],
                                                    hwCursorDstAddr[devNo][hwcursordstfb[devNo]], tagID, &u16QueueCnt)) {
                ALOGE("flip errro! in 3D sterre model!");
            }
        } else {
            if (!MApi_GOP_Switch_GWIN_2_FB_BY_ADDR(transaction->gwin, hwCursorDstAddr[devNo][hwcursordstfb[devNo]],
                                                   tagID, &u16QueueCnt)) {
                ALOGE("flip errro in normal model!");
            }
        }
        if (u16QueueCnt <= 0) {
            ALOGE("Serious warning, unknow error!!\n");
        }
    }
    if (MApi_GOP_GWIN_IsGWINEnabled(transaction->gwin)==0) {
        MApi_GOP_GWIN_Enable(transaction->gwin, TRUE);
    }

done:
    //MApi_GOP_GWIN_UpdateRegOnceEx2(gopNo, FALSE, FALSE);
    MApi_GOP_GWIN_UpdateRegOnceByMask(GOP_getBitMask(gopNo),FALSE, FALSE);
    if (!needFlip) {
        while (!MApi_GOP_IsRegUpdated(transaction->gop) && (goptimeout++ <= GOP_TIMEOUT_CNT))
            usleep(1000);
    }
    memcpy(&ctx->lastHwCusorInfo[devNo], transaction, sizeof(HwCursorInfo_t));
    hwCursor_showfps();
    return 0;
}

int hwcursordev_commitTransaction(int32_t devNo) {
    pthread_mutex_lock(&global_mutex[devNo]);
    pthread_cond_signal(&hwcursor_cond[devNo]);
    pthread_mutex_unlock(&global_mutex[devNo]);
    return 0;
}
