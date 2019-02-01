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

#ifndef HWCOMPOSERDEVICE_H_
#define HWCOMPOSERDEVICE_H_
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

#include "hwcomposer2_mstar.h"
#include "HWComposerContext.h"
#include "HWCLayer.h"
#include "HWCDisplayDevice.h"
#include <unordered_set>
#include <vector>
#include <utils/Timers.h>
#include <inttypes.h>
#include <map>
#include <unordered_map>
#include <sstream>

class HWCLayer;
class HWCDisplayDevice;

template <typename PFN, typename T>
static hwc2_function_pointer_t asFuncPointer(T function)
{
    static_assert(std::is_same<PFN, T>::value, "Incompatible function pointer");
    return reinterpret_cast<hwc2_function_pointer_t>(function);
}

class HWComposerDevice : public hwc2_device_t
{
public:
    HWComposerDevice();
    ~HWComposerDevice();
private:
    static inline HWComposerDevice* getHWCDevice(hwc2_device_t* device) {
        return static_cast<HWComposerDevice*>(device);
    }

    template <typename T, typename MF, MF memFunc, typename ...Args>
    static T hwcDeviceHook(hwc2_device_t* device,
            Args... args) {
        return (getHWCDevice(device)->*memFunc)(std::forward<Args>(args)...);
    }

    template <typename ...Args>
    static int32_t callDisplayFunction(hwc2_device_t* device,
            hwc2_display_t displayId, hwc2_error_t (HWCDisplayDevice::*member)(Args...),
            Args... args) {
        auto display = getHWCDevice(device)->getDisplay(displayId);
        if (!display) {
            return static_cast<int32_t>(HWC2_ERROR_BAD_DISPLAY);
        }
        auto error = ((*display).*member)(std::forward<Args>(args)...);
        return static_cast<int32_t>(error);
    }

    template <typename MF, MF memFunc, typename ...Args>
    static int32_t displayHook(hwc2_device_t* device, hwc2_display_t displayId,
            Args... args) {
        return HWComposerDevice::callDisplayFunction(device, displayId, memFunc,
                std::forward<Args>(args)...);
    }

    template <typename ...Args>
    static int32_t callLayerFunction(hwc2_device_t* device,
            hwc2_display_t displayId, hwc2_layer_t layerId,
            hwc2_error_t (HWCLayer::*member)(Args...), Args... args) {
        auto result = getHWCDevice(device)->getLayer(displayId, layerId);
        auto error = std::get<hwc2_error_t>(result);
        if (error == HWC2_ERROR_NONE) {
            auto layer = std::get<HWCLayer*>(result);
            error = ((*layer).*member)(std::forward<Args>(args)...);
        }
        return static_cast<int32_t>(error);
    }

    template <typename MF, MF memFunc, typename ...Args>
    static int32_t layerHook(hwc2_device_t* device, hwc2_display_t displayId,
            hwc2_layer_t layerId, Args... args) {
        return HWComposerDevice::callLayerFunction(device, displayId, layerId,
            memFunc, std::forward<Args>(args)...);
    }

    void getCapabilitiesImpl(uint32_t* outCount,
            int32_t* /*hwc2_capability_t*/ outCapabilities);
    hwc2_function_pointer_t getFunctionImpl(int32_t descriptor);
    hwc2_error_t createVirtualDisplay(uint32_t width, uint32_t height,
            int32_t* /*android_pixel_format_t*/ format, hwc2_display_t* outDisplay);
    hwc2_error_t destroyVirtualDisplay(hwc2_display_t display);
    void dump(uint32_t* outSize, char* outBuffer);
    uint32_t getMaxVirtualDisplayCount();
    hwc2_error_t registerCallback(int32_t /*hwc2_callback_descriptor_t*/ intDesc,
            hwc2_callback_data_t callbackData, hwc2_function_pointer_t pointer);

    HWCDisplayDevice* getDisplay(hwc2_display_t id);
    HWCDisplayDevice* getDisplay(hwc2_display_type_t type);
    std::tuple<HWCLayer*, hwc2_error_t> getLayer(hwc2_display_t displayId, hwc2_layer_t layerId);
    void initCapabilities();
    void initPrimaryDevice();
    void eventWorkThread();
    std::map<hwc2_display_t, std::shared_ptr<HWCDisplayDevice>> mDisplays;
    std::unordered_set<hwc2_capability_t> mCapabilities;
    CallbackInfo<HWC2_PFN_HOTPLUG> mHotplug;
    CallbackInfo<HWC2_PFN_REFRESH> mRefresh;
    CallbackInfo<HWC2_PFN_VSYNC> mVsync;
    std::vector<std::pair<hwc2_display_t, int32_t>>
            mPendingHotplugs;
    std::vector<std::shared_ptr<HWCDisplayDevice>> mPendingRefreshes;
    std::vector<std::pair<hwc2_display_t, nsecs_t>> mPendingVsyncs;
    std::string mDumpString;
};
#endif /* HWCOMPOSERDEVICE_H_ */
