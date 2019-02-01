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


#include <stdlib.h>
#include "debug.h"

Debug Debug::debug_;
uint32_t Debug::DebugHandler::debug_flags_ = 0x1;
uint32_t Debug::DebugHandler::verbose_level_ = 0;

Debug::Debug() : debug_handler_() {
}

bool Debug::IsFramebufferAFBCDisabled() {
    int debugType = 0;
    if (debug_.debug_handler_->GetProperty(HWCOMPOSER20_DEBUG_PROPERTY, &debugType)
        == HWC2_ERROR_NONE) {
        if (debugType == HWC2_DEBUGTYPE_DISABLE_FRAMEBUFFER_AFBC) {
            return true;
        }
    }
    return false;
}

bool Debug::IsGopOverlayDisabled() {
    int debugType = 0;
    if (debug_.debug_handler_->GetProperty(HWCOMPOSER20_DEBUG_PROPERTY, &debugType)
        == HWC2_ERROR_NONE) {
        if (debugType == HWC2_DEBUGTYPE_DISABLE_GOPOVERLAY) {
            return true;
        }
    }
    return false;
}

bool Debug::IsForceGlesBlendEnabled() {
    int debugType = 0;
    if (debug_.debug_handler_->GetProperty(HWCOMPOSER20_DEBUG_PROPERTY, &debugType)
        == HWC2_ERROR_NONE) {
        if (debugType == HWC2_DEBUGTYPE_FORCE_GLESBLEND) {
            return true;
        }
    }
    return false;
}

bool Debug::GetProperty(const char* property_name, char* value) {
  if (debug_.debug_handler_->GetProperty(property_name, value) != HWC2_ERROR_NONE) {
    return false;
  }

  return true;
}

void Debug::UpdateDebugType() {
    int debugType = 0;
    if (debug_.debug_handler_->GetProperty(HWCOMPOSER20_DEBUG_PROPERTY, &debugType) == HWC2_ERROR_NONE) {
        if (debugType == HWC2_DEBUGTYPE_NONE) {
            debug_.debug_handler_->debug_flags_ = 0x1;
        } else {
            EnableDebugTag(debugType);
        }
    }
}

void Debug::EnableDebugTag(uint32_t tag) {
    SET_BIT(debug_.debug_handler_->debug_flags_,tag);
}

void Debug::DisableDebugTag(uint32_t tag) {
    CLEAR_BIT(debug_.debug_handler_->debug_flags_,tag);
}

void Debug::DebugHandler::Error(HWC2DebugTag tag, const char *format, ...) {
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, format, list);
}

void Debug::DebugHandler::Warning(HWC2DebugTag tag, const char *format, ...) {
    va_list list;
    va_start(list, format);
    __android_log_vprint(ANDROID_LOG_WARN, LOG_TAG, format, list);
}

void Debug::DebugHandler::Info(HWC2DebugTag tag, const char *format, ...) {
    if (IS_BIT_SET(debug_flags_, tag)) {
        va_list list;
        va_start(list, format);
        __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, format, list);
    }

}

void Debug::DebugHandler::Debug(HWC2DebugTag tag, const char *format, ...) {
    if (IS_BIT_SET(debug_flags_, tag)) {
        va_list list;
        va_start(list, format);
        __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, format, list);
    }
}

void Debug::DebugHandler::Verbose(HWC2DebugTag tag, const char *format, ...) {
    if (IS_BIT_SET(debug_flags_, tag) && verbose_level_) {
        va_list list;
        va_start(list, format);
        __android_log_vprint(ANDROID_LOG_VERBOSE, LOG_TAG, format, list);
    }
}

hwc2_error_t Debug::DebugHandler::GetProperty(const char *property_name, int *value) {
    char property[PROPERTY_VALUE_MAX];

    if (property_get(property_name, property, NULL) > 0) {
        *value = atoi(property);
        return HWC2_ERROR_NONE;
    }

    return HWC2_ERROR_UNSUPPORTED;
}
hwc2_error_t Debug::DebugHandler::GetProperty(const char *property_name, char *value) {
  return HWC2_ERROR_UNSUPPORTED;
}
