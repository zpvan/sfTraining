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

#ifdef ENABLE_HWCURSOR

#include <sys/mman.h>

#include <dlfcn.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/fb.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <GLES/gl.h>

#ifdef MALI_VSYNC_EVENT_REPORT_ENABLE
    #include "gralloc_vsync_report.h"
#endif

#include "gralloc_priv.h"
#include "gralloc_helper.h"

#include "hardware/hwcursor.h"
#include "hwcursordev.h"
#include "hwcursor_device.h"


/*****************************************************************************/
static int hwcursor_close(struct hw_device_t *device)
{
    hwcursor_device_t* dev = reinterpret_cast<hwcursor_device_t*>(device);
    if (dev) {
        //ump_close();
        delete dev;
    }
    return 0;
}

static int hwcursor_show(struct hwcursor_device_t *device, int devNo)
{
    device->hwCursorVinfo[devNo].state |= HWCURSOR_SHOW;
    return 0;
}

static int hwcursor_hide(struct hwcursor_device_t *device, int devNo)
{
    device->hwCursorVinfo[devNo].state |= HWCURSOR_HIDE;
    return 0;
}

static int hwcursor_setMatrix(struct hwcursor_device_t *device, int devNo, float dsdx, float dtdx, float dsdy, float dtdy)
{
    device->hwCursorVinfo[devNo].matrix[0] = dsdx;
    device->hwCursorVinfo[devNo].matrix[1] = dtdx;
    device->hwCursorVinfo[devNo].matrix[2] = dsdy;
    device->hwCursorVinfo[devNo].matrix[3] = dtdy;
    device->hwCursorVinfo[devNo].state |= HWCURSOR_MATRIX;
    return 0;
}

static int hwcursor_setPosition(struct hwcursor_device_t *device, int devNo, float positionX, float positionY, float hotSpotX, float hotSpotY, int iconWidth, int iconHeight)
{
    device->hwCursorVinfo[devNo].positionX = positionX;
    device->hwCursorVinfo[devNo].positionY = positionY;
    device->hwCursorVinfo[devNo].hotSpotX = hotSpotX;
    device->hwCursorVinfo[devNo].hotSpotY = hotSpotY;
    device->hwCursorVinfo[devNo].iconWidth = iconWidth;
    device->hwCursorVinfo[devNo].iconHeight = iconHeight;
    device->hwCursorVinfo[devNo].state |= HWCURSOR_POSITION;
    return 0;
}

static int hwcursor_setAlpha(struct hwcursor_device_t *device, int devNo, float alpha)
{
    device->hwCursorVinfo[devNo].alpha = alpha;
    device->hwCursorVinfo[devNo].state |= HWCURSOR_ALPHA;
    return 0;
}

static int hwcursor_changeIcon(struct hwcursor_device_t *device, int devNo)
{
    device->hwCursorVinfo[devNo].state |= HWCURSOR_ICON;
    return 0;
}

static int hwcursor_resolutionChanged(struct hwcursor_device_t *device)
{
    for (int i=0; i< HWCURSOR_MAX_COUNT; i++)
        device->hwCursorVinfo[i].state |= HWCURSOR_RESOLUTION_CHANGED;
    return 0;
}

static int hwcursor_overscanChange(struct hwcursor_device_t* device,int left, int top, int right, int bottom)
{
    if (device->hwOverscanInfo.left != left ||
        device->hwOverscanInfo.top != top ||
        device->hwOverscanInfo.right != right ||
        device->hwOverscanInfo.bottom != bottom) {
        hwcursordev_setOverscan(left,top,right,bottom);
        for (int i=0; i < HWCURSOR_MAX_COUNT; i++)
            device->hwCursorVinfo[i].state |= HWCURSOR_RESOLUTION_CHANGED;
        device->hwOverscanInfo.left = left;
        device->hwOverscanInfo.top = top;
        device->hwOverscanInfo.right = right;
        device->hwOverscanInfo.bottom = bottom;
    }
    return 0;
}

static int hwcursor_doTransaction(struct hwcursor_device_t *device, int devNo)
{
    hwcursordev_doTransaction(&device->hwCursorVinfo[devNo]);
    device->hwCursorVinfo[devNo].state = 0;
    return 0;
}

static int hwcursor_getfinfo(struct hwcursor_device_t *device, struct hwcursor_fix_info * hwCursorFinfo, int devNo)
{
    if (device->hwCursorFinfo[devNo].bInitialized != 1) {
        ALOGE("hwcursor_getfinfo error hwcursordev:%d has not been initialized!",devNo);
        return -1;
    }
    hwcursordev_getfinfo(hwCursorFinfo, devNo);
    return 0;
}

static int hwcursor_initByDevNo(struct hwcursor_device_t *device, int devNo) {
    hwcursordev_init(devNo);
    hwcursordev_getfinfo(&(device->hwCursorFinfo[devNo]), devNo);
    memset(&(device->hwCursorVinfo[devNo]), 0, sizeof(struct hwcursor_var_info));
    device->hwCursorVinfo[devNo].devIndex = devNo;
    return 0;
}

static int hwcursor_DeinitByDevNo(struct hwcursor_device_t *device, int devNo) {
    hwcursordev_deinit(devNo);
    memset(&(device->hwCursorVinfo[devNo]), 0, sizeof(struct hwcursor_var_info));
    return 0;
}

int hwcursor_device_open(hw_module_t const* module, const char* name,
                            hw_device_t** device)
{
    int status = -EINVAL;
    int fmt = 0;
    int i = 0;

    /* initialize our state here */
    hwcursor_device_t *dev = new hwcursor_device_t;
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = hwcursor_close;
    dev->hwCursorShow = hwcursor_show;
    dev->hwCursorHide = hwcursor_hide;
    dev->hwCursorSetMatrix = hwcursor_setMatrix;
    dev->hwCursorSetPosition = hwcursor_setPosition;
    dev->hwCursorSetAlpha = hwcursor_setAlpha;
    dev->hwCursorChangeIcon = hwcursor_changeIcon;
    dev->hwCursorDoTransaction = hwcursor_doTransaction;
    dev->hwCursorResolutionChanged = hwcursor_resolutionChanged;
    dev->hwCursorGetfinfo = hwcursor_getfinfo;
    dev->hwCursorOverscanChange = hwcursor_overscanChange;
    dev->hwCursorInitByDevNo = hwcursor_initByDevNo;
    dev->hwCursorDeinitByDevNo = hwcursor_DeinitByDevNo;
    //Init system icon used by InputManager
    hwcursordev_init(0);
    hwcursordev_getfinfo(&(dev->hwCursorFinfo[0]), 0);

    memset(&(dev->hwCursorVinfo[0]), 0, sizeof(struct hwcursor_var_info));

    memset(&(dev->hwOverscanInfo), 0, sizeof(struct hwcursor_overscan_info));
    char property[PROPERTY_VALUE_MAX] = {0};
    if (property_get("mstar.reproducerate",property,NULL) > 0) {
       int value = atoi(property);
       dev->hwOverscanInfo.top = (value>>24)&0xff;
       dev->hwOverscanInfo.bottom = (value>>16)&0xff;
       dev->hwOverscanInfo.left = (value>>8)&0xff;
       dev->hwOverscanInfo.right  = (value)&0xff;
    }

    ALOGI("hwcursor_device_open Config:\n");
    ALOGI("hwCursorFinfo.hwCursorAddr    : 0x%lx\n", (long unsigned int)dev->hwCursorFinfo[0].hwCursorAddr);
    ALOGI("hwCursorFinfo.hwCursorVaddr   : 0x%lx\n", (long unsigned int)dev->hwCursorFinfo[0].hwCursorVaddr);
    ALOGI("hwCursorFinfo.hwCursorWidth   : %d\n", dev->hwCursorFinfo[0].hwCursorWidth);
    ALOGI("hwCursorFinfo.hwCursorHeight : %d\n", dev->hwCursorFinfo[0].hwCursorHeight);
    ALOGI("hwCursorFinfo.hwCursorStride  : %d\n", dev->hwCursorFinfo[0].hwCursorStride);
    ALOGI("hwCursorFinfo.matrix[0]  : %f\n", dev->hwCursorFinfo[0].matrix[0]);
    ALOGI("hwCursorFinfo.matrix[1]  : %f\n", dev->hwCursorFinfo[0].matrix[1]);
    ALOGI("hwCursorFinfo.matrix[2]  : %f\n", dev->hwCursorFinfo[0].matrix[2]);
    ALOGI("hwCursorFinfo.matrix[3]  : %f\n", dev->hwCursorFinfo[0].matrix[3]);
    for (i = 0; i < HWCURSOR_DSTFB_NUM; i++)
    {
        ALOGI("hwCursorFinfo.hwCursorDstAddr[%d]    : 0x%lx\n", i, (long unsigned int)dev->hwCursorFinfo[0].hwCursorDstAddr[i]);
    }
    ALOGI("hwCursorFinfo.hwCursorDstWidth   : %d\n", dev->hwCursorFinfo[0].hwCursorDstWidth);
    ALOGI("hwCursorFinfo.hwCursorDstHeight : %d\n", dev->hwCursorFinfo[0].hwCursorDstHeight);
    ALOGI("hwCursorFinfo.hwCursorDstStride  : %d\n", dev->hwCursorFinfo[0].hwCursorDstStride);

    *device = &dev->common;

    return 0;
}
#endif
