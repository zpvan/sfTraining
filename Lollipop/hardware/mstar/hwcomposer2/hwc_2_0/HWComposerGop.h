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

#ifndef HWCOMPOSERGOP_H_
#define HWCOMPOSERGOP_H_
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11
#include "HWCLayer.h"
#include "HWCDisplayDevice.h"
#include "HWComposerContext.h"
#include <vector>


class HWCLayer;
class HWCDisplayDevice;

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

enum {
    GOP0_BIT_MASK = 0x1,
    GOP1_BIT_MASK = 0x2,
    GOP2_BIT_MASK = 0x4,
    GOP3_BIT_MASK = 0x8,
    GOP4_BIT_MASK = 0x10
};

class HWComposerGop {
public:

    HWComposerGop(HWCDisplayDevice& display);
    ~HWComposerGop();

    int32_t getAvailGwinId(int gopNo);
    hwc2_error_t init(std::shared_ptr<HWWindowInfo>& hwwindowinfo);
    hwc2_error_t display( std::vector<std::shared_ptr<HWWindowInfo>>& inHWwindowinfo,
                          std::vector<std::shared_ptr<HWWindowInfo>>& lastHWwindowinfo);
    uint32_t getDestination();
    hwc2_error_t getCurOPTiming(int *ret_width, int *ret_height, int *ret_hstart);
    hwc2_error_t getCurOCTiming(int *ret_width, int *ret_height, int *ret_hstart);
    uint32_t getBitMask(unsigned int gop);
    hwc2_error_t setInterlave(int gop, int bInterlave);
    hwc2_error_t setStretchWindow(unsigned char gop,  unsigned short srcWidth, unsigned short srcHeight,
                              unsigned short dstX, unsigned short dstY, unsigned short dstWidth, unsigned short dstHeight);
private:
    hwc2_error_t resetAFBC(std::shared_ptr<HWWindowInfo>& hwwindowinfo);
    hwc2_error_t reCreateFBInfo(int osdMode,std::shared_ptr<HWWindowInfo>& hwwindowinfo,
                                    std::shared_ptr<HWWindowInfo>& lasthwwindowinfo);
    hwc2_error_t updateGWinInfo(int osdMode,std::shared_ptr<HWWindowInfo>& hwwindowinfo,
                                    std::shared_ptr<HWWindowInfo>& lasthwwindowinfo);
    hwc2_error_t close_boot_logo(std::shared_ptr<HWWindowInfo>& hwwindowinfo);
    uint32_t getLayerFromGOP(uint32_t gop);
    int HAL_PIXEL_FORMAT_to_MS_ColorFormat(int inFormat);
    int changeHalPixelFormat2MsColorFormat(int inFormat);
    int HAL_PIXEL_FORMAT_byteperpixel(int inFormat);
    int changeHalPixelFormat2bpp(int inFormat);
    std::vector<MS_BOOL> bGopHasInit;
    HWCDisplayDevice& mDisplay;
};

#endif /* HWCOMPOSERGOP_H_ */
