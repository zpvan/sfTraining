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

#undef LOG_TAG
#define LOG_TAG "HWComposerDevice"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "HWComposerDevice.h"
#include <cutils/log.h>
#include <inttypes.h>
#define __CLASS__ "HWComposerDevice"

HWComposerDevice::HWComposerDevice()
    :mHotplug(),
    mPendingHotplugs(),
    mRefresh(),
    mPendingRefreshes(),
    mVsync(),
    mPendingVsyncs(),
    mDumpString()
{
    getCapabilities = hwcDeviceHook<void, decltype(&HWComposerDevice::getCapabilitiesImpl),
                    &HWComposerDevice::getCapabilitiesImpl, uint32_t*,
                    int32_t*>;
    getFunction = hwcDeviceHook<hwc2_function_pointer_t, decltype(&HWComposerDevice::getFunctionImpl),
                    &HWComposerDevice::getFunctionImpl, int32_t>;
    initCapabilities();
    initPrimaryDevice();

    DLOGD("create event work thread");
    auto t = std::make_shared<std::thread>(&HWComposerDevice::eventWorkThread,this);
    t->detach();
}
HWComposerDevice::~HWComposerDevice(){

}
void HWComposerDevice::getCapabilitiesImpl(uint32_t* outCount,
            int32_t* /*hwc2_capability_t*/ outCapabilities) {
    if (outCapabilities == nullptr) {
        *outCount = mCapabilities.size();
        return;
    }

    auto capabilityIter = mCapabilities.cbegin();
    for (size_t i = 0; i < *outCount; ++i) {
        if (capabilityIter == mCapabilities.cend()) {
            return;
        }
        outCapabilities[i] = static_cast<int32_t>(*capabilityIter);
        ++capabilityIter;
    }
}

hwc2_error_t HWComposerDevice::createVirtualDisplay(uint32_t width, uint32_t height,
            int32_t* /*android_pixel_format_t*/ format, hwc2_display_t* outDisplay) {

    if (MAX_VIRTUAL_DISPLAY_DIMENSION != 0 &&
            (width > MAX_VIRTUAL_DISPLAY_DIMENSION ||
            height > MAX_VIRTUAL_DISPLAY_DIMENSION)) {
        DLOGE("createVirtualDisplay: Can't create a virtual display with"
                " a dimension > %u (tried %u x %u)",
                MAX_VIRTUAL_DISPLAY_DIMENSION, width, height);
        return HWC2_ERROR_NO_RESOURCES;
    }

    auto display = std::make_shared<HWCDisplayDevice>(*this,
            static_cast<hwc2_display_type_t>(HWC2_DISPLAY_TYPE_VIRTUAL));
    display->initConfigs(width, height);
    hwc2_display_t id = display->getId();
    mDisplays.emplace(id, std::move(display));
    *outDisplay = id;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWComposerDevice::destroyVirtualDisplay(hwc2_display_t display) {
    const auto displayIt = mDisplays.find(display);
    if (displayIt == mDisplays.end()) {
        DLOGE("destroyVirtualDisplay(%" PRIu64 ") failed:mLayers has no such display device",
                display);
        return HWC2_ERROR_BAD_DISPLAY;
    }
    mDisplays.erase(displayIt);
    return HWC2_ERROR_NONE;
}

void HWComposerDevice::dump(uint32_t* outSize, char* outBuffer) {
    if (outBuffer != nullptr) {
        auto copiedBytes = mDumpString.copy(outBuffer, *outSize);
        *outSize = static_cast<uint32_t>(copiedBytes);
        return;
    }
    std::stringstream output;
    output << "======Mstar HAL Dump infomation Begin======\n";
    if (mCapabilities.empty()) {
        output << "Capabilities: None\n";
    } else {
        output << "Capabilities:\n";
        for (auto capability : mCapabilities) {
            output << "  " << getCapabilityName(capability) << '\n';
        }
    }
    output << '\n';

    output << "Hwcomposer Display Devices:\n";
    for (const auto& element : mDisplays) {
        const auto& display = element.second;
        output << display->dump();
    }
    output << '\n';

    output << "======Mstar HAL Dump infomation end======\n";
    mDumpString = output.str();
    *outSize = static_cast<uint32_t>(mDumpString.size());
}

uint32_t HWComposerDevice::getMaxVirtualDisplayCount() {
    return 0;
}

hwc2_error_t HWComposerDevice::registerCallback(int32_t /*hwc2_callback_descriptor_t*/ intDesc,
        hwc2_callback_data_t callbackData, hwc2_function_pointer_t pointer) {
    auto descriptor = static_cast<hwc2_callback_descriptor_t>(intDesc);
    DLOGD("callback descriptor[%s],pointer = %p"
        ,getCallbackDescriptorName(descriptor),pointer);
    switch (descriptor) {
        case HWC2_CALLBACK_HOTPLUG:
            mHotplug.pointer= reinterpret_cast<HWC2_PFN_HOTPLUG>(pointer);
            mHotplug.data = callbackData;
            for (auto& pending : mPendingHotplugs) {
                auto& displayId = pending.first;
                auto connected = pending.second;
                DLOGD("Sending pending hotplug(%" PRIu64 ", %s)", displayId,
                        getConnectionName(static_cast<hwc2_connection_t>(connected)));
                mHotplug.pointer(callbackData, displayId, connected);
            }
            return HWC2_ERROR_NONE;
        case HWC2_CALLBACK_REFRESH:
            mRefresh.pointer = reinterpret_cast<HWC2_PFN_REFRESH>(pointer);
            mRefresh.data = callbackData;
            return HWC2_ERROR_NONE;
        case HWC2_CALLBACK_VSYNC:
            mVsync.pointer = reinterpret_cast<HWC2_PFN_VSYNC>(pointer);
            mVsync.data = callbackData;
            return HWC2_ERROR_NONE;
        default:
            DLOGE("registerCallback: Unknown callback descriptor: %d (%s)",
                    static_cast<int32_t>(descriptor),
                    getCallbackDescriptorName(descriptor));
            return HWC2_ERROR_BAD_DISPLAY;
    }
    return HWC2_ERROR_BAD_DISPLAY;
}

HWCDisplayDevice* HWComposerDevice::getDisplay(hwc2_display_t id) {
    auto display = mDisplays.find(id);
    if (display == mDisplays.end()) {
        return nullptr;
    }

    return display->second.get();
}

HWCDisplayDevice* HWComposerDevice::getDisplay(hwc2_display_type_t intype) {
    for (auto display:mDisplays) {
        int32_t outType;
        display.second->getDisplayType(&outType);
        hwc2_display_type_t type = static_cast<hwc2_display_type_t>(outType);
        if (type == intype) {
            return display.second.get();
        }
    }
    return nullptr;
}

std::tuple<HWCLayer*, hwc2_error_t> HWComposerDevice::getLayer(hwc2_display_t displayId, hwc2_layer_t layerId) {
    auto display = getDisplay(displayId);
    if (!display) {
        return std::make_tuple(static_cast<HWCLayer*>(nullptr), HWC2_ERROR_BAD_DISPLAY);
    }

    auto layerEntry = display->getLayers().find(layerId);
    if (layerEntry == display->getLayers().end()) {
        return std::make_tuple(static_cast<HWCLayer*>(nullptr), HWC2_ERROR_BAD_LAYER);
    }

    auto layer = layerEntry->second;
    if (layer->getDisplay().getId() != displayId) {
        return std::make_tuple(static_cast<HWCLayer*>(nullptr), HWC2_ERROR_BAD_LAYER);
    }
    return std::make_tuple(layer.get(), HWC2_ERROR_NONE);
}

void HWComposerDevice::initCapabilities() {
    mCapabilities.insert(HWC2_CAPABILITY_SIDEBAND_STREAM);
}
void HWComposerDevice::initPrimaryDevice() {
    auto display =
        std::make_shared<HWCDisplayDevice>(*this, static_cast<hwc2_display_type_t>(HWC2_DISPLAY_TYPE_PHYSICAL));
    hwc2_display_t id = display->getId();
    mDisplays.emplace(id, std::move(display));

    DLOGD("callHotplug called, but no valid callback registered, storing display");
    mPendingHotplugs.emplace_back(id, HWC2_CONNECTION_CONNECTED);
}

void HWComposerDevice::eventWorkThread() {
    // TODO: add hw vsync, virtual display hotplug

    const char* gflip_path = "/dev/gflip";

    char thread_name[64] = "hwcVsyncThread";
    prctl(PR_SET_NAME, (unsigned long) &thread_name, 0, 0, 0);
    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    nsecs_t cur_timestamp=0;
    int fd_vsync = -1;
    bool fakevsync = false;
    char property[PROPERTY_VALUE_MAX] = {0};
    static int vsync_gop = 0;
    static nsecs_t next_fake_vsync = 0;
    int old3DMode = DISPLAYMODE_NORMAL;
    int sdr2hdrEnable = 0;
    int r2yEnable = 0;
    auto display = getDisplay(HWC2_DISPLAY_TYPE_PHYSICAL);
    // TODO: hw vsync have some problem,use sw vsync recently
    property_get("mstar.lvds-off", property, "false");
    if (strcmp(property, "true") == 0) {
        display->bVsyncOff = true;
        DLOGD("the mstar.lvds-off is true, so will force to use the fake vsync");
    }

    if (display->bVsyncOff) {
        fakevsync = true;
    } else {
        if (property_get("mstar.fake.vsync.enable", property, "false") > 0) {
            if (strcmp(property, "true") == 0) {
                fakevsync = true;
            }
        }
    }

    fd_vsync = open(gflip_path, O_RDWR);
    if (fd_vsync < 0) {
        DLOGE("FATAL:not able to open file:%s, %s",
              gflip_path, strerror(errno));
        fakevsync = true;
    }


    // usleep(1000*1000);
    vsync_gop = display->getDefaultGopNumber();
    while (true) {
        bool enabled;
        enabled = display->isVsyncEnabled();
        if (enabled) {
            if (!fakevsync) {
#define UNLIKELY(exp) (__builtin_expect((exp) != 0, false))
                if (UNLIKELY(ioctl(fd_vsync, MDRV_GFLIP_IOC_GETVSYNC, &vsync_gop) != 0)) {
                    fakevsync = true;
                    DLOGE("ioctl failed, use fake vsync");
                } else {
                    cur_timestamp = systemTime(CLOCK_MONOTONIC);
                }
            }
            if (fakevsync) {
                cur_timestamp = systemTime(CLOCK_MONOTONIC);
                nsecs_t next_vsync = next_fake_vsync;
                nsecs_t sleep = next_vsync - cur_timestamp;
                if (sleep < 0) {
                    // we missed, find where the next vsync should be
                    sleep = (display->mVsyncPeriod - ((cur_timestamp - next_vsync) % display->mVsyncPeriod));
                    next_vsync = cur_timestamp + sleep;
                }
                next_fake_vsync = next_vsync + display->mVsyncPeriod;

                struct timespec spec;
                spec.tv_sec  = next_vsync / 1000000000;
                spec.tv_nsec = next_vsync % 1000000000;

                int err;
                do {
                    err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
                } while (err<0 && errno == EINTR);

                cur_timestamp = next_vsync;
            }
            // send timestamp to HAL
            //DLOGD("%s: timestamp %llu sent to HWC for %s", __FUNCTION__, cur_timestamp, "gflip");
            if (mVsync.pointer) {
                mVsync.pointer(mVsync.data, display->getId(),cur_timestamp);
            }
        } else {
            usleep(16666);
        }
        //check debug type
        Debug::UpdateDebugType();
        //check 3D
        if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
            int new3DMode = atoi(property);
            if (new3DMode != old3DMode) {
#ifndef ENABLE_HWC14_CURSOR
                //hwcursor don't support DISPLAYMODE_NORMAL_FP,so using sw cursor
                static bool rollbackHwcursor = false;
                if ((new3DMode == DISPLAYMODE_NORMAL_FP)
                #ifdef MSTAR_MESSI
                    ||(new3DMode != DISPLAYMODE_NORMAL)
                #endif
                 ) {
                    int enableHwcursor = 0;
                    if (property_get("mstar.desk-enable-hwcursor", property, "1") > 0) {
                        enableHwcursor = atoi(property);
                    }
                    if (enableHwcursor) {
                        rollbackHwcursor = true;
                        property_set("mstar.desk-enable-hwcursor", "0");
                    }
                } else if ((old3DMode == DISPLAYMODE_NORMAL_FP)
                #ifdef MSTAR_MESSI
                        ||(old3DMode != DISPLAYMODE_NORMAL)
                #endif
                ) {
                    if (rollbackHwcursor) {
                        rollbackHwcursor = false;
                        property_set("mstar.desk-enable-hwcursor", "1");
                    }
                }
#endif
                old3DMode = new3DMode;
                DLOGD("the new3DMode is %d",new3DMode);

                for (int i = 0; i < MAX_OVERLAY_LAYER_NUM; i++) {
                    MS_Video_Disp_Set_Cmd((MS_Win_ID)i, MS_DISP_CMD_UPDATE_DISP_STATUS, NULL);
                }

                if (mRefresh.pointer) {
                    mRefresh.pointer(mRefresh.data, display->getId());
                }
            }
        }

#ifdef ENABLE_GAMECURSOR
        // check enable game cursor
        if (property_get("mstar.desk-enable-gamecursor", property, "0") > 0) {
            int value = atoi(property);
            if (display->mGameCursorEnabled != value) {
                display->mGameCursorEnabled = value;
                display->mGameCursorChanged = true;
                if (mRefresh.pointer) {
                    mRefresh.pointer(mRefresh.data, display->getId());
                    // close gamecursor thread and gwin(devNo1) if disabled
                    if(value == 0) {
                        display->gameCursorDeinit(1);
                    }
                }
            }
        }
#endif

        // check SDR --> HDR and RGB --> YUV
        {
            if (property_get("mstar.enable.sdr2hdr", property, "0") > 0) {
                int enable = atoi(property);
                 if (enable != sdr2hdrEnable) {
                    sdr2hdrEnable = enable;
                    display->bFramebufferNeedRedraw = true;
                    if (mRefresh.pointer) {
                        mRefresh.pointer(mRefresh.data, display->getId());
                    }
                 }
            }
            if (property_get("mstar.enable.r2y", property, "0") > 0) {
                 int enable = atoi(property);
                 if (enable != r2yEnable) {
                    r2yEnable = enable;
                    display->bFramebufferNeedRedraw = true;
                    if (mRefresh.pointer) {
                        mRefresh.pointer(mRefresh.data, display->getId());
                    }
                 }
            }
        }
        // check gop dest or timing changed
        if(display->checkGopTimingChanged()) {
            if (mRefresh.pointer) {
                mRefresh.pointer(mRefresh.data, display->getId());
            }
            unsigned int gop_dest = display->getGopDest();
            if(gop_dest == E_GOP_DST_OP0) {
                display->mmVideoSizeChangedNotify();
            }
        }
        // check whether down scale XC
        display->mmVideoCheckDownScaleXC();
    }
}

hwc2_function_pointer_t HWComposerDevice::getFunctionImpl(int32_t descriptor) {
    DLOGD("doGetFunction: getFunctionImpl function descriptor: %d (%s)",
                    descriptor,
                    getFunctionDescriptorMstarName(static_cast<hwc2_function_descriptor_mstar_t>(descriptor)));
    switch (descriptor) {
        // Device functions
        case HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY:
            return asFuncPointer<HWC2_PFN_CREATE_VIRTUAL_DISPLAY>(
                    hwcDeviceHook<int32_t, decltype(&HWComposerDevice::createVirtualDisplay),
                    &HWComposerDevice::createVirtualDisplay, uint32_t,
                    uint32_t, int32_t*, hwc2_display_t*>);
        case HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY:
            return asFuncPointer<HWC2_PFN_DESTROY_VIRTUAL_DISPLAY>(
                    hwcDeviceHook<int32_t, decltype(&HWComposerDevice::destroyVirtualDisplay),
                    &HWComposerDevice::destroyVirtualDisplay, hwc2_display_t>);
        case HWC2_FUNCTION_DUMP:
            return asFuncPointer<HWC2_PFN_DUMP>(
                    hwcDeviceHook<void, decltype(&HWComposerDevice::dump),
                    &HWComposerDevice::dump, uint32_t*, char*>);
        case HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT:
            return asFuncPointer<HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT>(
                    hwcDeviceHook<uint32_t, decltype(&HWComposerDevice::getMaxVirtualDisplayCount),
                    &HWComposerDevice::getMaxVirtualDisplayCount>);
        case HWC2_FUNCTION_REGISTER_CALLBACK:
            return asFuncPointer<HWC2_PFN_REGISTER_CALLBACK>(
                    hwcDeviceHook<int32_t, decltype(&HWComposerDevice::registerCallback),
                    &HWComposerDevice::registerCallback, int32_t,
                    hwc2_callback_data_t, hwc2_function_pointer_t>);

        // Display functions
        case HWC2_FUNCTION_ACCEPT_DISPLAY_CHANGES:
            return asFuncPointer<HWC2_PFN_ACCEPT_DISPLAY_CHANGES>(
                    displayHook<decltype(&HWCDisplayDevice::acceptChanges),
                    &HWCDisplayDevice::acceptChanges>);
        case HWC2_FUNCTION_CREATE_LAYER:
            return asFuncPointer<HWC2_PFN_CREATE_LAYER>(
                    displayHook<decltype(&HWCDisplayDevice::createLayer),
                    &HWCDisplayDevice::createLayer, hwc2_layer_t*>);
        case HWC2_FUNCTION_DESTROY_LAYER:
            return asFuncPointer<HWC2_PFN_DESTROY_LAYER>(
                    displayHook<decltype(&HWCDisplayDevice::destroyLayer),
                    &HWCDisplayDevice::destroyLayer, hwc2_layer_t>);
        case HWC2_FUNCTION_GET_ACTIVE_CONFIG:
            return asFuncPointer<HWC2_PFN_GET_ACTIVE_CONFIG>(
                    displayHook<decltype(&HWCDisplayDevice::getActiveConfig),
                    &HWCDisplayDevice::getActiveConfig, hwc2_config_t*>);
        case HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES:
            return asFuncPointer<HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES>(
                    displayHook<decltype(&HWCDisplayDevice::getChangedCompositionTypes),
                    &HWCDisplayDevice::getChangedCompositionTypes, uint32_t*,
                    hwc2_layer_t*, int32_t*>);
        case HWC2_FUNCTION_GET_COLOR_MODES:
            return asFuncPointer<HWC2_PFN_GET_COLOR_MODES>(
                    displayHook<decltype(&HWCDisplayDevice::getColorModes),
                    &HWCDisplayDevice::getColorModes, uint32_t*, int32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_ATTRIBUTE>(
                    displayHook<decltype(&HWCDisplayDevice::getDisplayAttribute),
                    &HWCDisplayDevice::getDisplayAttribute, hwc2_config_t, int32_t, int32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_CONFIGS:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_CONFIGS>(
                    displayHook<decltype(&HWCDisplayDevice::getDisplayConfigs),
                    &HWCDisplayDevice::getDisplayConfigs, uint32_t*, hwc2_config_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_NAME:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_NAME>(
                    displayHook<decltype(&HWCDisplayDevice::getDisplayName),
                    &HWCDisplayDevice::getDisplayName, uint32_t*, char*>);
        case HWC2_FUNCTION_GET_DISPLAY_REQUESTS:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_REQUESTS>(
                    displayHook<decltype(&HWCDisplayDevice::getDisplayRequests),
                    &HWCDisplayDevice::getDisplayRequests, int32_t*, uint32_t*, hwc2_layer_t*,
                    int32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_TYPE:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_TYPE>(
                    displayHook<decltype(&HWCDisplayDevice::getDisplayType),
                    &HWCDisplayDevice::getDisplayType, int32_t*>);
        case HWC2_FUNCTION_GET_DOZE_SUPPORT:
            return asFuncPointer<HWC2_PFN_GET_DOZE_SUPPORT>(
                    displayHook<decltype(&HWCDisplayDevice::getDozeSupport),
                    &HWCDisplayDevice::getDozeSupport, int32_t*>);
        case HWC2_FUNCTION_GET_HDR_CAPABILITIES:
            return asFuncPointer<HWC2_PFN_GET_HDR_CAPABILITIES>(
                    displayHook<decltype(&HWCDisplayDevice::getHdrCapabilities),
                    &HWCDisplayDevice::getHdrCapabilities, uint32_t*, int32_t*, float*,
                    float*, float*>);
        case HWC2_FUNCTION_GET_RELEASE_FENCES:
            return asFuncPointer<HWC2_PFN_GET_RELEASE_FENCES>(
                    displayHook<decltype(&HWCDisplayDevice::getReleaseFences),
                    &HWCDisplayDevice::getReleaseFences, uint32_t*, hwc2_layer_t*,
                    int32_t*>);
        case HWC2_FUNCTION_PRESENT_DISPLAY:
            return asFuncPointer<HWC2_PFN_PRESENT_DISPLAY>(
                    displayHook<decltype(&HWCDisplayDevice::present),
                    &HWCDisplayDevice::present, int32_t*>);
        case HWC2_FUNCTION_SET_ACTIVE_CONFIG:
            return asFuncPointer<HWC2_PFN_SET_ACTIVE_CONFIG>(
                    displayHook<decltype(&HWCDisplayDevice::setActiveConfig),
                    &HWCDisplayDevice::setActiveConfig, hwc2_config_t>);
        case HWC2_FUNCTION_SET_CLIENT_TARGET:
            return asFuncPointer<HWC2_PFN_SET_CLIENT_TARGET>(
                    displayHook<decltype(&HWCDisplayDevice::setClientTarget),
                    &HWCDisplayDevice::setClientTarget, buffer_handle_t, int32_t,
                    int32_t, hwc_region_t>);
        case HWC2_FUNCTION_SET_COLOR_MODE:
            return asFuncPointer<HWC2_PFN_SET_COLOR_MODE>(
                    displayHook<decltype(&HWCDisplayDevice::setColorMode),
                    &HWCDisplayDevice::setColorMode, int32_t>);
        case HWC2_FUNCTION_SET_COLOR_TRANSFORM:
            return asFuncPointer<HWC2_PFN_SET_COLOR_TRANSFORM>(
                    displayHook<decltype(&HWCDisplayDevice::setColorTransform),
                    &HWCDisplayDevice::setColorTransform, const float*, int32_t>);
        case HWC2_FUNCTION_SET_OUTPUT_BUFFER:
            return asFuncPointer<HWC2_PFN_SET_OUTPUT_BUFFER>(
                    displayHook<decltype(&HWCDisplayDevice::setOutputBuffer),
                    &HWCDisplayDevice::setOutputBuffer, buffer_handle_t, int32_t>);
        case HWC2_FUNCTION_SET_POWER_MODE:
            return asFuncPointer<HWC2_PFN_SET_POWER_MODE>(
                    displayHook<decltype(&HWCDisplayDevice::setPowerMode),
                    &HWCDisplayDevice::setPowerMode, int32_t>);
        case HWC2_FUNCTION_SET_VSYNC_ENABLED:
            return asFuncPointer<HWC2_PFN_SET_VSYNC_ENABLED>(
                    displayHook<decltype(&HWCDisplayDevice::setVsyncEnabled),
                    &HWCDisplayDevice::setVsyncEnabled, int32_t>);
        case HWC2_FUNCTION_VALIDATE_DISPLAY:
            return asFuncPointer<HWC2_PFN_VALIDATE_DISPLAY>(
                    displayHook<decltype(&HWCDisplayDevice::validate),
                    &HWCDisplayDevice::validate, uint32_t*, uint32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_3D_TRANSFORM:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_3D_TRANSFORM>(
                    displayHook<decltype(&HWCDisplayDevice::get3DTransform),
                    &HWCDisplayDevice::get3DTransform, hwc_3d_param_t*, hwc_rect_t>);
        case HWC2_FUNCTION_GET_DISPLAY_TIMING:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_TIMING>(
                    displayHook<decltype(&HWCDisplayDevice::getCurrentTiming),
                    &HWCDisplayDevice::getCurrentTiming, int32_t*, int32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_IS_STB:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_IS_STB>(
                    displayHook<decltype(&HWCDisplayDevice::isStbTarget),
                    &HWCDisplayDevice::isStbTarget, bool*>);
        case HWC2_FUNCTION_GET_DISPLAY_OSD_WIDTH:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_OSD_WIDTH>(
                    displayHook<decltype(&HWCDisplayDevice::getOsdWidth),
                    &HWCDisplayDevice::getOsdWidth, int32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_OSD_HEIGHT:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_OSD_HEIGHT>(
                    displayHook<decltype(&HWCDisplayDevice::getOsdHeight),
                    &HWCDisplayDevice::getOsdHeight, int32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_MIRROR_MODE:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_MIRROR_MODE>(
                    displayHook<decltype(&HWCDisplayDevice::getMirrorMode),
                    &HWCDisplayDevice::getMirrorMode, uint32_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_REFRESH_PERIOD:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_REFRESH_PERIOD>(
                    displayHook<decltype(&HWCDisplayDevice::getRefreshPeriod),
                    &HWCDisplayDevice::getRefreshPeriod, nsecs_t*>);
        case HWC2_FUNCTION_GET_DISPLAY_CAPTURE_SCREEN:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_CAPTURE_SCREEN>(
                    displayHook<decltype(&HWCDisplayDevice::captureScreen),
                    &HWCDisplayDevice::captureScreen, uint32_t*, hwc2_layer_t*,
                    hwc_capture_data_t*, uintptr_t*, int*, int*, int>);
        case HWC2_FUNCTION_GET_DISPLAY_CAPTURE_RELEASE:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_CAPTURE_RELEASE>(
                    displayHook<decltype(&HWCDisplayDevice::releaseCaptureScreenMemory),
                    &HWCDisplayDevice::releaseCaptureScreenMemory>);
        case HWC2_FUNCTION_GET_DISPLAY_IS_RGBCOLORSPACE:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_IS_RGBCOLORSPACE>(
                    displayHook<decltype(&HWCDisplayDevice::isRGBColorSpace),
                    &HWCDisplayDevice::isRGBColorSpace, bool*>);
        case HWC2_FUNCTION_GET_DISPLAY_IS_AFBC_FB:
            return asFuncPointer<HWC2_PFN_GET_DISPLAY_IS_AFBC_FB>(
                    displayHook<decltype(&HWCDisplayDevice::isAFBCFramebuffer),
                    &HWCDisplayDevice::isAFBCFramebuffer, bool*>);

        // Layer functions
        case HWC2_FUNCTION_SET_LAYER_BUFFER:
            return asFuncPointer<HWC2_PFN_SET_LAYER_BUFFER>(
                    layerHook<decltype(&HWCLayer::setLayerBuffer),
                    &HWCLayer::setLayerBuffer, buffer_handle_t, int32_t>);
        case HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE:
            return asFuncPointer<HWC2_PFN_SET_LAYER_SURFACE_DAMAGE>(
                    layerHook<decltype(&HWCLayer::setLayerSurfaceDamage),
                    &HWCLayer::setLayerSurfaceDamage, hwc_region_t>);

        // Layer state functions
        case HWC2_FUNCTION_SET_LAYER_BLEND_MODE:
            return asFuncPointer<HWC2_PFN_SET_LAYER_BLEND_MODE>(
                    layerHook<decltype(&HWCLayer::setLayerBlendMode),
                    &HWCLayer::setLayerBlendMode, int32_t>);
        case HWC2_FUNCTION_SET_LAYER_COLOR:
            return asFuncPointer<HWC2_PFN_SET_LAYER_COLOR>(
                    layerHook<decltype(&HWCLayer::setLayerColor),
                    &HWCLayer::setLayerColor,hwc_color_t>);
        case HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE:
            return asFuncPointer<HWC2_PFN_SET_LAYER_COMPOSITION_TYPE>(
                    layerHook<decltype(&HWCLayer::setLayerCompositionType),
                    &HWCLayer::setLayerCompositionType, int32_t>);
        case HWC2_FUNCTION_SET_LAYER_DATASPACE:
            return asFuncPointer<HWC2_PFN_SET_LAYER_DATASPACE>(
                    layerHook<decltype(&HWCLayer::setLayerDataspace),
                    &HWCLayer::setLayerDataspace, int32_t>);
        case HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME:
            return asFuncPointer<HWC2_PFN_SET_LAYER_DISPLAY_FRAME>(
                    layerHook<decltype(&HWCLayer::setLayerDisplayFrame),
                    &HWCLayer::setLayerDisplayFrame, hwc_rect_t>);
        case HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA:
            return asFuncPointer<HWC2_PFN_SET_LAYER_PLANE_ALPHA>(
                    layerHook<decltype(&HWCLayer::setLayerPlaneAlpha),
                    &HWCLayer::setLayerPlaneAlpha, float>);
        case HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM:
            return asFuncPointer<HWC2_PFN_SET_LAYER_SIDEBAND_STREAM>(
                    layerHook<decltype(&HWCLayer::setLayerSidebandStream),
                    &HWCLayer::setLayerSidebandStream, const native_handle_t*>);
        case HWC2_FUNCTION_SET_LAYER_SOURCE_CROP:
            return asFuncPointer<HWC2_PFN_SET_LAYER_SOURCE_CROP>(
                    layerHook<decltype(&HWCLayer::setLayerSourceCrop),
                    &HWCLayer::setLayerSourceCrop, hwc_frect_t>);
        case HWC2_FUNCTION_SET_LAYER_TRANSFORM:
            return asFuncPointer<HWC2_PFN_SET_LAYER_TRANSFORM>(
                    layerHook<decltype(&HWCLayer::setLayerTransform),
                    &HWCLayer::setLayerTransform, int32_t>);
        case HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION:
            return asFuncPointer<HWC2_PFN_SET_LAYER_VISIBLE_REGION>(
                    layerHook<decltype(&HWCLayer::setLayerVisibleRegion),
                    &HWCLayer::setLayerVisibleRegion, hwc_region_t>);
        case HWC2_FUNCTION_SET_LAYER_NAME:
            return asFuncPointer<HWC2_PFN_SET_LAYER_NAME>(
                    layerHook<decltype(&HWCLayer::setLayerName),
                    &HWCLayer::setLayerName, std::string>);
        case HWC2_FUNCTION_GET_LAYER_VENDOR_USAGES:
            return asFuncPointer<HWC2_PFN_GET_LAYER_VENDOR_USAGES>(
                    layerHook<decltype(&HWCLayer::getLayerVendorUsageBits),
                    &HWCLayer::getLayerVendorUsageBits, uint32_t*>);
        // call HWCDisplayDevice::updateLayerZ
        case HWC2_FUNCTION_SET_LAYER_Z_ORDER:
            return asFuncPointer<HWC2_PFN_SET_LAYER_Z_ORDER>(
                    displayHook<decltype(&HWCDisplayDevice::updateLayerZ),
                    &HWCDisplayDevice::updateLayerZ, hwc2_layer_t, uint32_t>);
        // hwcursor's interface.call display device's function
        case HWC2_FUNCTION_SET_CURSOR_POSITION:
            return asFuncPointer<HWC2_PFN_SET_CURSOR_POSITION>(
                    displayHook<decltype(&HWCDisplayDevice::setCursorPosition),
                    &HWCDisplayDevice::setCursorPosition, hwc2_layer_t, int32_t, int32_t>);
        case HWC2_FUNCTION_REDRAW_CURSOR:
            return asFuncPointer<HWC2_PFN_REDRAW_CURSOR>(
                    displayHook<decltype(&HWCDisplayDevice::reDrawHwCursorFb),
                    &HWCDisplayDevice::reDrawHwCursorFb, hwc2_layer_t, int32_t, int32_t,
                    int32_t, int32_t,int32_t, int32_t,int32_t>);
        //game cursor
#ifdef ENABLE_GAMECURSOR
        case HWC2_FUNCTION_SET_GAME_CURSOR_SHOW_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_SET_GAME_CURSOR_SHOW_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorShow),
                    &HWCDisplayDevice::gameCursorShow, int32_t>);
        case HWC2_FUNCTION_SET_GAME_CURSOR_HIDE_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_SET_GAME_CURSOR_HIDE_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorHide),
                    &HWCDisplayDevice::gameCursorHide, int32_t>);
        case HWC2_FUNCTION_SET_GAME_CURSOR_ALPHA_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_SET_GAME_CURSOR_ALPHA_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorSetAlpha),
                    &HWCDisplayDevice::gameCursorSetAlpha, int32_t, float>);
        case HWC2_FUNCTION_REDRAW_GAME_CURSOR_FB_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_REDRAW_GAME_CURSOR_FB_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorRedrawFb),
                    &HWCDisplayDevice::gameCursorRedrawFb, int32_t, int32_t, int32_t,
                    int32_t, int32_t,int32_t, int32_t,int32_t>);
        case HWC2_FUNCTION_SET_GAME_CURSOR_POSITION_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_SET_GAME_CURSOR_POSITION_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorSetPosition),
                    &HWCDisplayDevice::gameCursorSetPosition, int32_t, int32_t,
                    int32_t>);
        case HWC2_FUNCTION_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorDoTransaction),
                    &HWCDisplayDevice::gameCursorDoTransaction, int32_t>);
        case HWC2_FUNCTION_GET_GAME_CURSOR_FBADDR_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_GET_GAME_CURSOR_FBADDR_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorGetFbVaddrByDevNo),
                    &HWCDisplayDevice::gameCursorGetFbVaddrByDevNo, int32_t, uintptr_t*>);
        case HWC2_FUNCTION_INIT_GAME_CURSOR_BY_DEVNO:
            return asFuncPointer<HWC2_PFN_INIT_GAME_CURSOR_BY_DEVNO>(
                    displayHook<decltype(&HWCDisplayDevice::gameCursorInit),
                    &HWCDisplayDevice::gameCursorInit, int32_t>);
#endif
        default:
            DLOGE("doGetFunction: Unknown function descriptor: %d (%s)",
                    descriptor,
                    getFunctionDescriptorMstarName(static_cast<hwc2_function_descriptor_mstar_t>(descriptor)));
            return nullptr;
    }
}
