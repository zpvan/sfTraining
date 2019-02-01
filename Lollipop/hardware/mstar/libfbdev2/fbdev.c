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

#define LOG_TAG "fbdev"
#include <utils/Log.h>

#include "stdbool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <linux/fb.h>
#include <cutils/properties.h>
#include <iniparser.h>
#include <mmap.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <drvCMAPool.h>
#include <drvMMIO.h>
#include <drvSEM.h>
#include <drvGPIO.h>
#include <drvXC_IOPort.h>
#include <apiXC.h>
#include <apiXC_Ace.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <drvSERFLASH.h>
#include <assert.h>
#include "fbdev.h"
#include "stdbool.h"
#include <drvSYS.h>
#include <tvini.h>

static int fake_fd = -100; /* -1 is used in mmap, so use -100 instead */
static void* fb_mem_mapped;
static int panelWidth = 1920, panelHeight = 1080;
static int osdWidth = 1920, osdHeight = 1080;
static MS_U32 u32FBLength = 0;

//#define ALIGN(_val_,_align_) ((_val_ + _align_ - 1) & (~(_align_ - 1)))
#define ALIGNMENT( value, base ) ((value) & ~((base) - 1))

static struct fb_var_screeninfo fb_vinfo = {
    .activate     = FB_ACTIVATE_NOW,
    .height       = -1,
    .width        = -1,
    .right_margin = 0,
    .upper_margin = 0,
    .lower_margin = 0,
    .vsync_len    = 4,
    .vmode        = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo fb_finfo = {
    .id    = "MSTAR VFB",
    .type  = FB_TYPE_PACKED_PIXELS,
    .accel = FB_ACCEL_NONE,
};

static int getBufferCount(int length, int width, int height) {
    int bufferCount = 0;
    int bufferSize = width * height *fb_vinfo.bits_per_pixel/8;
    if (length/bufferSize >= 3) {
        bufferCount= 3;
    } else {
        bufferCount = length/bufferSize;
    }
    return bufferCount ;
}

static int gfx_Init(dictionary* panelINI,dictionary* modelINI) {
    const mmap_info_t* mmapInfo;
    MS_PHY addr = 0;
    MS_U8 miu_sel = 0;
    MS_PHY maskPhyAddr = 0;
    int cmaEnable = 0;
    struct CMA_Pool_Init_Param CMA_Pool_Init_PARAM;
    struct CMA_Pool_Alloc_Param CMA_Alloc_PARAM;
    MS_BOOL ret = FALSE;
    MS_BOOL bSTB = FALSE;
    char property[PROPERTY_VALUE_MAX] = {0};
    property_get("mstar.build.for.stb", property, "false");
    if (strcmp(property, "true") == 0) {
        bSTB = TRUE;
    } else {
        bSTB = FALSE;
    }

    osdWidth = iniparser_getint(panelINI, "panel:osdWidth", 1920);
    osdHeight = iniparser_getint(panelINI, "panel:osdHeight", 1080);
    panelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
    panelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
    if (property_get("ro.sf.osd_size", property, NULL) > 0) {
        if ((0==strcmp(property,"1080p")) || (0==strcmp(property,"1080P"))) {
            osdWidth = 1920;
            osdHeight = 1080;
            ALOGI("the ro.sf.osd_size is set as 1080p");
        } else if ((0==strcmp(property,"720p")) || (0==strcmp(property,"720P"))) {
            osdWidth = 1280;
            osdHeight = 720;
            ALOGI("the ro.sf.osd_size is set as 720p");
        } else {
            ALOGI("warning!! wrong value is set to the ro.sf.osd_size ");
        }
    }

    mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
    if (mmapInfo != NULL) {
        addr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        u32FBLength = mmapInfo->size;
        if (mmapInfo->cmahid > 0) {
            cmaEnable = 1;
        }
    }

    fb_vinfo.bits_per_pixel = 32;
    fb_vinfo.grayscale = 0;
    fb_vinfo.transp.offset = 24;
    fb_vinfo.transp.length = 8;
    fb_vinfo.red.offset = 0;
    fb_vinfo.red.length = 8;
    fb_vinfo.green.offset = 8;
    fb_vinfo.green.length = 8;
    fb_vinfo.blue.offset = 16;
    fb_vinfo.blue.length = 8;
    fb_vinfo.reserved[0] = osdWidth;
    fb_vinfo.reserved[1] = osdHeight;
    int bufferCount = 3;
    if ((bSTB==FALSE)&&((osdWidth > panelWidth) || (osdHeight > panelHeight))) {
        //check GOP buffer alignment
        //panelWidth = panelWidth & ~3U;
        fb_vinfo.xres = ALIGNMENT(panelWidth, 16);
        fb_vinfo.yres = panelHeight;
        bufferCount = getBufferCount(u32FBLength, fb_vinfo.xres,fb_vinfo.yres);
        fb_vinfo.xres_virtual = ALIGNMENT(panelWidth, 16);
        fb_vinfo.yres_virtual = panelHeight * 3;
    } else {
        //check GOP buffer alignment
        bufferCount = getBufferCount(u32FBLength, osdWidth,osdHeight);
        osdWidth = ALIGNMENT(osdWidth, 16);
        fb_vinfo.xres = osdWidth;
        fb_vinfo.yres = osdHeight;
        fb_vinfo.xres_virtual = osdWidth;
        fb_vinfo.yres_virtual = osdHeight * bufferCount;

    }
    fb_vinfo.xoffset = 0;
    fb_vinfo.yoffset = 0;
    // Some dummy values for timing to make fbset happy
    fb_vinfo.pixclock = 100000000000LLU / (6 *  fb_vinfo.xres * fb_vinfo.yres);
    fb_vinfo.left_margin = 0;
    fb_vinfo.hsync_len = 0;

    fb_finfo.smem_start  = addr;
    fb_finfo.smem_len    = fb_vinfo.xres * fb_vinfo.yres * 4;
    fb_finfo.visual      = FB_VISUAL_TRUECOLOR;
    fb_finfo.line_length = fb_vinfo.xres * 4;
    maskPhyAddr =  fbdev_getMaskPhyAdr((int)(mmapInfo->miuno));
    ALOGI("gfx_Init invoked maskPhyAddr=%llx", (long long unsigned int)maskPhyAddr);
    if (cmaEnable==1) {
        ALOGI("MMAP_GOP_FB was integrated into CMA init it!!");
        //CMA_POOL_INIT
        CMA_Pool_Init_PARAM.heap_id = mmapInfo->cmahid;
        CMA_Pool_Init_PARAM.flags = CMA_FLAG_MAP_VMA;
        ret = MApi_CMA_Pool_Init(&CMA_Pool_Init_PARAM);
        if (ret ==  FALSE) {
            ALOGE("gfx_Init MApi_CMA_Pool_Init error");
        }
        //CMA_Pool_GetMem
        CMA_Alloc_PARAM.pool_handle_id = CMA_Pool_Init_PARAM.pool_handle_id;
        CMA_Alloc_PARAM.offset_in_pool = (addr & maskPhyAddr) - CMA_Pool_Init_PARAM.heap_miu_start_offset;
        CMA_Alloc_PARAM.length = mmapInfo->size;
        CMA_Alloc_PARAM.flags = CMA_FLAG_VIRT_ADDR;
        ret = MApi_CMA_Pool_GetMem(&CMA_Alloc_PARAM);
        if (ret == FALSE) {
            ALOGE("gfx_Init MApi_CMA_Pool_GetMem error");
        }
        //store
        fb_mem_mapped = (void*)CMA_Alloc_PARAM.virt_addr;
    }
    /**
     *MMAP_GOP_FB was not integrated into CMA
     *or MMAP_GOP_FB was placed into CMA but
     *CMA pool has been got in other process
     *use MsOS_MPool_Mapping to get virtual addr
     */
    if ((cmaEnable==0) || (ret==FALSE)) {
        if (addr >= mmap_get_miu1_offset()) {
            if (!MsOS_MPool_Mapping(2, (addr & maskPhyAddr), ((u32FBLength+4095)&~4095U), 1)) {
                ALOGE("MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            fb_mem_mapped = (void*)MsOS_PA2KSEG1((addr & maskPhyAddr) | mmap_get_miu1_offset());

            ALOGI("GOP FB in miu2 0x%llx, map to 0x%p\n",  (long long unsigned int)addr & maskPhyAddr, fb_mem_mapped);


        } else if (addr >= mmap_get_miu0_offset()) {
            if (!MsOS_MPool_Mapping(1, (addr & maskPhyAddr), ((u32FBLength+4095)&~4095U), 1)) {
                ALOGE("MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            fb_mem_mapped = (void*)MsOS_PA2KSEG1((addr & maskPhyAddr) | mmap_get_miu0_offset());

            ALOGI("GOP FB in miu1 0x%llx, map to 0x%p\n",  (long long unsigned int)addr & maskPhyAddr, fb_mem_mapped);
        } else {
            if (!MsOS_MPool_Mapping(0, (addr & maskPhyAddr), ((u32FBLength+4095)&~4095U), 1)) {
                ALOGE("MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
            }
            fb_mem_mapped = (void*)MsOS_PA2KSEG1(addr & maskPhyAddr);
            ALOGI("GOP FB in miu0 0x%llx, map to 0x%p\n",  (long long unsigned int)addr & maskPhyAddr, fb_mem_mapped);
        }
    }
    return 0;
}

int fbdev_init(void) {
    dictionary *sysINI = NULL, *modelINI = NULL, *panelINI = NULL;

    if (mmap_init() != 0) {
        return -1;
    }

    MDrv_SYS_GlobalInit();
    mdrv_gpio_init();
    MsOS_MPool_Init();
    MsOS_Init();
    MApi_PNL_IOMapBaseInit();

    tvini_init(&sysINI, &modelINI, &panelINI);

    gfx_Init(panelINI,modelINI);

    tvini_free(sysINI, modelINI, panelINI);
    return 0;
}

int fbdev_open(const char* pathname, int flags) {
    fbdev_init();
    if (fake_fd < 0) {
        fake_fd = open("/dev/null", O_RDWR);
    }
    return fake_fd;
}

int fbdev_close(int fd) {
    fake_fd = -100;
    return 0;
}

int fbdev_ioctl(int fd, int request, void* data) {
    struct fb_var_screeninfo* vinfo;
    struct fb_fix_screeninfo* finfo;

    switch (request) {
        case FBIOGET_VSCREENINFO:
            vinfo = (struct fb_var_screeninfo*)data;
            if (vinfo) {
                memcpy(vinfo, &fb_vinfo, sizeof(struct fb_var_screeninfo));
                return 0;
            }
            break;
        case FBIOPUT_VSCREENINFO:
            vinfo = (struct fb_var_screeninfo*)data;
            if (vinfo) {
                int retry_times;

                if (vinfo->xres_virtual > fb_vinfo.xres_virtual) {
                    vinfo->xres_virtual = fb_vinfo.xres_virtual;
                }
                if (vinfo->yres_virtual > fb_vinfo.yres_virtual) {
                    vinfo->yres_virtual = fb_vinfo.yres_virtual;
                }

                vinfo->transp = fb_vinfo.transp;
                vinfo->red    = fb_vinfo.red;
                vinfo->green  = fb_vinfo.green;
                vinfo->blue   = fb_vinfo.blue;

                fb_vinfo.yoffset = vinfo->yoffset;

                // post framebuffer moved to hwcomposer

                return 0;
            }
            break;
        case FBIOGET_FSCREENINFO:
            finfo = (struct fb_fix_screeninfo*)data;
            if (finfo) {
                memcpy(finfo, &fb_finfo, sizeof(struct fb_fix_screeninfo));
                return 0;
            }
            break;
        case FBIOPUTCMAP:
            break;
        case FBIOGETCMAP:
            break;
        default:
            break;
    }
    errno = EINVAL;
    return -1;
}

void* fbdev_mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset) {
    return fb_mem_mapped;
}

int fbdev_munmap(void* start, size_t length) {
    return 0;
}

unsigned int fbdev_getMIU1Offset() {
    return mmap_get_miu0_offset();
}

unsigned int fbdev_getMaskPhyAdr(int u8MiuSel) {
    unsigned int miu0Len = mmap_get_miu0_length();
    unsigned int miu1Len = mmap_get_miu1_length();
    unsigned int miu2Len  = mmap_get_miu2_length();

    if (u8MiuSel==2) {
        ALOGI("fbdev_getMaskPhyAdr MIU2 mask=%x", (miu2Len-1));
        return(miu2Len - 1) ;
    } else if (u8MiuSel==1){
        ALOGI("fbdev_getMaskPhyAdr MIU1 mask=%x", (miu1Len-1));
        return(miu1Len - 1);
    } else {
        ALOGI("fbdev_getMaskPhyAdr MIU0 mask=%x", (miu0Len-1));
        return(miu0Len - 1);
    }
}

// obsolete
int fbdev_read(int left, int top, int* width, int* height, void* data, int mode) {
    ALOGE("fbdev_read is obsolete");
    return -1;
}
