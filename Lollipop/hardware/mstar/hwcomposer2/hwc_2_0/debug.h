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

#ifndef __DEBUG_H__
#define __DEBUG_H__
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

#include <cutils/log.h>
#include <cutils/properties.h>


#define DLOG(tag, method, format, ...) Debug::Get()->method(tag, __CLASS__ "::%s: " format, \
                                                            __FUNCTION__, ##__VA_ARGS__)

#define DLOGE_IF(tag, format, ...) DLOG(tag, Error, format, ##__VA_ARGS__)
#define DLOGW_IF(tag, format, ...) DLOG(tag, Warning, format, ##__VA_ARGS__)
#define DLOGI_IF(tag, format, ...) DLOG(tag, Info, format, ##__VA_ARGS__)
#define DLOGD_IF(tag, format, ...) DLOG(tag, Debug, format, ##__VA_ARGS__)
#define DLOGV_IF(tag, format, ...) DLOG(tag, Verbose, format, ##__VA_ARGS__)

#define DLOGE(format, ...) DLOGE_IF(HWC2_DEBUGTYPE_NONE, format, ##__VA_ARGS__)
#define DLOGW(format, ...) DLOGW_IF(HWC2_DEBUGTYPE_NONE, format, ##__VA_ARGS__)
#define DLOGI(format, ...) DLOGI_IF(HWC2_DEBUGTYPE_NONE, format, ##__VA_ARGS__)
#define DLOGD(format, ...) DLOGD_IF(HWC2_DEBUGTYPE_NONE, format, ##__VA_ARGS__)
#define DLOGV(format, ...) DLOGV_IF(HWC2_DEBUGTYPE_NONE, format, ##__VA_ARGS__)

#define SET_BIT(value, bit) (value |= (1 << (bit)))
#define CLEAR_BIT(value, bit) (value &= (~(1 << (bit))))
#define IS_BIT_SET(value, bit) (value & (1 << (bit)))

/*
  mstar.debug.hwcomposer20 = 0 : turn off hwcomposer log
  mstar.debug.hwcomposer20 = 1 : show all hwcwindow info
  mstar.debug.hwcomposer20 = 2 : show the hwcomposer20 fps
  mstar.debug.hwcomposer20 = 3 : show all video layers attributes
  mstar.debug.hwcomposer20 = 4 : show all the osd layers attributes
  mstar.debug.hwcomposer20 = 5 : show all the cursor layers attributes
  mstar.debug.hwcomposer20 = 6 : turn on dip log
  mstar.debug.hwcomposer20 = 7 : turn on fence log
  mstar.debug.hwcomposer20 = 8 :  turn on hwc layer log
  mstar.debug.hwcomposer20 = 9 :  turn on hwc cursor log
  mstar.debug.hwcomposer20 = 10 : force to update glesblend layer
  mstar.debug.hwcomposer20 = 11 : turn off framebuffer's afbc mode
  mstar.debug.hwcomposer20 = 12 : disable gopoverlay, all layers will fallback to ClientTarget
*/
#define HWCOMPOSER20_DEBUG_PROPERTY                  "mstar.debug.hwcomposer20"

enum HWC2DebugTag {
    HWC2_DEBUGTYPE_NONE,                                 // 0
    HWC2_DEBUGTYPE_SHOW_HWWINDOWINFO,                    // 1
    HWC2_DEBUGTYPE_SHOW_FPS,                             // 2
    HWC2_DEBUGTYPE_SHOW_VIDEO_LAYERS,                    // 3
    HWC2_DEBUGTYPE_SHOW_OSD_LAYERS,                      // 4
    HWC2_DEBUGTYPE_SHOW_CURSOR_LAYERS,                   // 5
    HWC2_DEBUGTYPE_DIP_CAPTURE,                          // 6
    HWC2_DEBUGTYPE_FENCE,                                // 7
    HWC2_DEBUGTYPE_HWCLAYER,                             // 8
    HWC2_DEBUGTYPE_HWCCURSOR,                            // 9
    HWC2_DEBUGTYPE_FORCE_GLESBLEND,                      // 10
    HWC2_DEBUGTYPE_DISABLE_FRAMEBUFFER_AFBC,             // 11
    HWC2_DEBUGTYPE_DISABLE_GOPOVERLAY,                   // 12
};

class Debug {
private:
    Debug();

    class DebugHandler{
    public:
        static uint32_t debug_flags_;
        static uint32_t verbose_level_;
        void Error(HWC2DebugTag /*tag*/, const char * /*format*/, ...);
        void Warning(HWC2DebugTag /*tag*/, const char * /*format*/, ...);
        void Info(HWC2DebugTag /*tag*/, const char * /*format*/, ...);
        void Debug(HWC2DebugTag /*tag*/, const char * /*format*/, ...);
        void Verbose(HWC2DebugTag /*tag*/, const char * /*format*/, ...);
        hwc2_error_t GetProperty(const char * /*property_name*/, int * /*value*/);
        hwc2_error_t GetProperty(const char * /*property_name*/, char * /*value*/);
    };

    DebugHandler *debug_handler_;
    static Debug debug_;
public:
    static inline DebugHandler* Get() { return debug_.debug_handler_; }
    static bool IsFramebufferAFBCDisabled();
    static bool IsGopOverlayDisabled();
    static bool IsForceGlesBlendEnabled();
    static bool GetProperty(const char *property_name, char *value);
    static void UpdateDebugType();
    static void EnableDebugTag(uint32_t tag);
    static void DisableDebugTag(uint32_t tag);

};


#endif  // __DEBUG_H__
