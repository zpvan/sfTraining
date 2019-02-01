/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/misc.h>
#include <utils/NativeHandle.h>
#include <utils/String8.h>
#include <utils/Thread.h>
#include <utils/Trace.h>
#include <utils/Vector.h>

#include <ui/GraphicBuffer.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include <android/configuration.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include "HWComposer.h"

#include "../Layer.h"           // needed only for debugging
#include "../SurfaceFlinger.h"

namespace android {

#define MIN_HWC_HEADER_VERSION HWC_HEADER_VERSION

static uint32_t hwcApiVersion(const hwc_composer_device_1_t* hwc) {
    uint32_t hwcVersion = hwc->common.version;
    return hwcVersion & HARDWARE_API_VERSION_2_MAJ_MIN_MASK;
}

static uint32_t hwcHeaderVersion(const hwc_composer_device_1_t* hwc) {
    uint32_t hwcVersion = hwc->common.version;
    return hwcVersion & HARDWARE_API_VERSION_2_HEADER_MASK;
}

static bool hwcHasApiVersion(const hwc_composer_device_1_t* hwc,
        uint32_t version) {
    return hwcApiVersion(hwc) >= (version & HARDWARE_API_VERSION_2_MAJ_MIN_MASK);
}

// ---------------------------------------------------------------------------

struct HWComposer::cb_context {
    struct callbacks : public hwc_procs_t {
        // these are here to facilitate the transition when adding
        // new callbacks (an implementation can check for NULL before
        // calling a new callback).
        void (*zero[4])(void);
    };
    callbacks procs;
    HWComposer* hwc;
};

// ---------------------------------------------------------------------------

HWComposer::HWComposer(
        const sp<SurfaceFlinger>& flinger,
        EventHandler& handler)
    : mFlinger(flinger),
      mFbDev(0), mHwc(0), mNumDisplays(1),
      mCBContext(new cb_context),
      mEventHandler(handler),
      mDebugForceFakeVSync(false)
{
    for (size_t i =0 ; i<MAX_HWC_DISPLAYS ; i++) {
        mLists[i] = 0;
    }

    for (size_t i=0 ; i<HWC_NUM_PHYSICAL_DISPLAY_TYPES ; i++) {
        mLastHwVSync[i] = 0;
        mVSyncCounts[i] = 0;
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.sf.no_hw_vsync", value, "0");
    mDebugForceFakeVSync = atoi(value);

    bool needVSyncThread = true;

    // Note: some devices may insist that the FB HAL be opened before HWC.
    int fberr = loadFbHalModule();
    loadHwcModule();

    if (mFbDev && mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
        // close FB HAL if we don't needed it.
        // FIXME: this is temporary until we're not forced to open FB HAL
        // before HWC.
        framebuffer_close(mFbDev);
        mFbDev = NULL;
    }

    // If we have no HWC, or a pre-1.1 HWC, an FB dev is mandatory.
    if ((!mHwc || !hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1))
            && !mFbDev) {
        ALOGE("ERROR: failed to open framebuffer (%s), aborting",
                strerror(-fberr));
        abort();
    }

    // these display IDs are always reserved
    for (size_t i=0 ; i<NUM_BUILTIN_DISPLAYS ; i++) {
        mAllocatedDisplayIDs.markBit(i);
    }

    if (mHwc) {
        ALOGI("Using %s version %u.%u", HWC_HARDWARE_COMPOSER,
              (hwcApiVersion(mHwc) >> 24) & 0xff,
              (hwcApiVersion(mHwc) >> 16) & 0xff);
        if (mHwc->registerProcs) {
            mCBContext->hwc = this;
            mCBContext->procs.invalidate = &hook_invalidate;
            mCBContext->procs.vsync = &hook_vsync;
            if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1))
                mCBContext->procs.hotplug = &hook_hotplug;
            else
                mCBContext->procs.hotplug = NULL;
            memset(mCBContext->procs.zero, 0, sizeof(mCBContext->procs.zero));
            mHwc->registerProcs(mHwc, &mCBContext->procs);
        }

        // don't need a vsync thread if we have a hardware composer
        needVSyncThread = false;
        // always turn vsync off when we start
        eventControl(HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, 0);

        // the number of displays we actually have depends on the
        // hw composer version
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_3)) {
            // 1.3 adds support for virtual displays
            mNumDisplays = MAX_HWC_DISPLAYS;
        } else if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
            // 1.1 adds support for multiple displays
            mNumDisplays = NUM_BUILTIN_DISPLAYS;
        } else {
            mNumDisplays = 1;
        }
    }

    if (mFbDev) {
        ALOG_ASSERT(!(mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)),
                "should only have fbdev if no hwc or hwc is 1.0");

        DisplayData& disp(mDisplayData[HWC_DISPLAY_PRIMARY]);
        disp.connected = true;
        disp.format = mFbDev->format;
        DisplayConfig config = DisplayConfig();
        config.width = mFbDev->width;
        config.height = mFbDev->height;
        config.xdpi = mFbDev->xdpi;
        config.ydpi = mFbDev->ydpi;
        // MStar Android Patch Begin
        config.osdWidth= mFbDev->osdWidth;
        config.osdHeight = mFbDev->osdHeight;
        // MStar Android Patch End
        config.refresh = nsecs_t(1e9 / mFbDev->fps);
        disp.configs.push_back(config);
        disp.currentConfig = 0;
    } else if (mHwc) {
        // here we're guaranteed to have at least HWC 1.1
        for (size_t i =0 ; i<NUM_BUILTIN_DISPLAYS ; i++) {
            queryDisplayProperties(i);
        }
    }

    if (needVSyncThread) {
        // we don't have VSYNC support, we need to fake it
        mVSyncThread = new VSyncThread(*this);
    }
    // MStar Android Patch Begin
    property_get("mstar.resize.framebuffer", value, "0");
    bFrameBufferResizeOpen = atoi(value);
    // MStar Android Patch End
}

HWComposer::~HWComposer() {
    if (mHwc) {
        eventControl(HWC_DISPLAY_PRIMARY, HWC_EVENT_VSYNC, 0);
    }
    if (mVSyncThread != NULL) {
        mVSyncThread->requestExitAndWait();
    }
    if (mHwc) {
        hwc_close_1(mHwc);
    }
    if (mFbDev) {
        framebuffer_close(mFbDev);
    }
    delete mCBContext;
}

// Load and prepare the hardware composer module.  Sets mHwc.
void HWComposer::loadHwcModule()
{
    hw_module_t const* module;

    if (hw_get_module(HWC_HARDWARE_MODULE_ID, &module) != 0) {
        ALOGE("%s module not found", HWC_HARDWARE_MODULE_ID);
        return;
    }

    int err = hwc_open_1(module, &mHwc);
    if (err) {
        ALOGE("%s device failed to initialize (%s)",
              HWC_HARDWARE_COMPOSER, strerror(-err));
        return;
    }

    if (!hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_0) ||
            hwcHeaderVersion(mHwc) < MIN_HWC_HEADER_VERSION ||
            hwcHeaderVersion(mHwc) > HWC_HEADER_VERSION) {
        ALOGE("%s device version %#x unsupported, will not be used",
              HWC_HARDWARE_COMPOSER, mHwc->common.version);
        hwc_close_1(mHwc);
        mHwc = NULL;
        return;
    }
}

// Load and prepare the FB HAL, which uses the gralloc module.  Sets mFbDev.
int HWComposer::loadFbHalModule()
{
    hw_module_t const* module;

    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err != 0) {
        ALOGE("%s module not found", GRALLOC_HARDWARE_MODULE_ID);
        return err;
    }

    return framebuffer_open(module, &mFbDev);
}

status_t HWComposer::initCheck() const {
    return mHwc ? NO_ERROR : NO_INIT;
}

void HWComposer::hook_invalidate(const struct hwc_procs* procs) {
    cb_context* ctx = reinterpret_cast<cb_context*>(
            const_cast<hwc_procs_t*>(procs));
    ctx->hwc->invalidate();
}

void HWComposer::hook_vsync(const struct hwc_procs* procs, int disp,
        int64_t timestamp) {
    cb_context* ctx = reinterpret_cast<cb_context*>(
            const_cast<hwc_procs_t*>(procs));
    ctx->hwc->vsync(disp, timestamp);
}

void HWComposer::hook_hotplug(const struct hwc_procs* procs, int disp,
        int connected) {
    cb_context* ctx = reinterpret_cast<cb_context*>(
            const_cast<hwc_procs_t*>(procs));
    ctx->hwc->hotplug(disp, connected);
}

void HWComposer::invalidate() {
    mFlinger->repaintEverything();
}

void HWComposer::vsync(int disp, int64_t timestamp) {
    if (uint32_t(disp) < HWC_NUM_PHYSICAL_DISPLAY_TYPES) {
        {
            Mutex::Autolock _l(mLock);

            // There have been reports of HWCs that signal several vsync events
            // with the same timestamp when turning the display off and on. This
            // is a bug in the HWC implementation, but filter the extra events
            // out here so they don't cause havoc downstream.
            if (timestamp == mLastHwVSync[disp]) {
                ALOGW("Ignoring duplicate VSYNC event from HWC (t=%" PRId64 ")",
                        timestamp);
                return;
            }

            mLastHwVSync[disp] = timestamp;
        }

        char tag[16];
        snprintf(tag, sizeof(tag), "HW_VSYNC_%1u", disp);
        ATRACE_INT(tag, ++mVSyncCounts[disp] & 1);

        mEventHandler.onVSyncReceived(disp, timestamp);
    }
}

void HWComposer::hotplug(int disp, int connected) {
    if (disp == HWC_DISPLAY_PRIMARY || disp >= VIRTUAL_DISPLAY_ID_BASE) {
        ALOGE("hotplug event received for invalid display: disp=%d connected=%d",
                disp, connected);
        return;
    }
    queryDisplayProperties(disp);
    mEventHandler.onHotplugReceived(disp, bool(connected));
}

static float getDefaultDensity(uint32_t width, uint32_t height) {
    // Default density is based on TVs: 1080p displays get XHIGH density,
    // lower-resolution displays get TV density. Maybe eventually we'll need
    // to update it for 4K displays, though hopefully those just report
    // accurate DPI information to begin with. This is also used for virtual
    // displays and even primary displays with older hwcomposers, so be
    // careful about orientation.

    uint32_t h = width < height ? width : height;
    if (h >= 1080) return ACONFIGURATION_DENSITY_XHIGH;
    else           return ACONFIGURATION_DENSITY_TV;
}

static const uint32_t DISPLAY_ATTRIBUTES[] = {
    HWC_DISPLAY_VSYNC_PERIOD,
    HWC_DISPLAY_WIDTH,
    HWC_DISPLAY_HEIGHT,
    HWC_DISPLAY_DPI_X,
    HWC_DISPLAY_DPI_Y,
    // MStar Android Patch Begin
    HWC_DISPLAY_OSDWIDTH,
    HWC_DISPLAY_OSDHEIGHT,
    // MStar Android Patch End
    HWC_DISPLAY_NO_ATTRIBUTE,
};
#define NUM_DISPLAY_ATTRIBUTES (sizeof(DISPLAY_ATTRIBUTES) / sizeof(DISPLAY_ATTRIBUTES)[0])

status_t HWComposer::queryDisplayProperties(int disp) {

    LOG_ALWAYS_FATAL_IF(!mHwc || !hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1));

    // use zero as default value for unspecified attributes
    int32_t values[NUM_DISPLAY_ATTRIBUTES - 1];
    memset(values, 0, sizeof(values));

    const size_t MAX_NUM_CONFIGS = 128;
    uint32_t configs[MAX_NUM_CONFIGS] = {0};
    size_t numConfigs = MAX_NUM_CONFIGS;
    status_t err = mHwc->getDisplayConfigs(mHwc, disp, configs, &numConfigs);
    if (err != NO_ERROR) {
        // this can happen if an unpluggable display is not connected
        mDisplayData[disp].connected = false;
        return err;
    }

    mDisplayData[disp].currentConfig = 0;
    for (size_t c = 0; c < numConfigs; ++c) {
        err = mHwc->getDisplayAttributes(mHwc, disp, configs[c],
                DISPLAY_ATTRIBUTES, values);
        if (err != NO_ERROR) {
            // we can't get this display's info. turn it off.
            mDisplayData[disp].connected = false;
            return err;
        }

        DisplayConfig config = DisplayConfig();
        for (size_t i = 0; i < NUM_DISPLAY_ATTRIBUTES - 1; i++) {
            switch (DISPLAY_ATTRIBUTES[i]) {
                case HWC_DISPLAY_VSYNC_PERIOD:
                    config.refresh = nsecs_t(values[i]);
                    break;
                case HWC_DISPLAY_WIDTH:
                    config.width = values[i];
                    break;
                case HWC_DISPLAY_HEIGHT:
                    config.height = values[i];
                    break;
                case HWC_DISPLAY_DPI_X:
                    config.xdpi = values[i] / 1000.0f;
                    break;
                case HWC_DISPLAY_DPI_Y:
                    config.ydpi = values[i] / 1000.0f;
                    break;
                // MStar Android Patch Begin
                case HWC_DISPLAY_OSDWIDTH:
                    config.osdWidth = values[i];
                    break;
                case HWC_DISPLAY_OSDHEIGHT:
                    config.osdHeight = values[i];
                    break;
                // MStar Android Patch End
                default:
                    ALOG_ASSERT(false, "unknown display attribute[%zu] %#x",
                            i, DISPLAY_ATTRIBUTES[i]);
                    break;
            }
        }

        if (config.xdpi == 0.0f || config.ydpi == 0.0f) {
            float dpi = getDefaultDensity(config.width, config.height);
            config.xdpi = dpi;
            config.ydpi = dpi;
        }

        mDisplayData[disp].configs.push_back(config);
    }

    // FIXME: what should we set the format to?
    mDisplayData[disp].format = HAL_PIXEL_FORMAT_RGBA_8888;
    mDisplayData[disp].connected = true;
    return NO_ERROR;
}

status_t HWComposer::setVirtualDisplayProperties(int32_t id,
        uint32_t w, uint32_t h, uint32_t format) {
    if (id < VIRTUAL_DISPLAY_ID_BASE || id >= int32_t(mNumDisplays) ||
            !mAllocatedDisplayIDs.hasBit(id)) {
        return BAD_INDEX;
    }
    size_t configId = mDisplayData[id].currentConfig;
    mDisplayData[id].format = format;
    DisplayConfig& config = mDisplayData[id].configs.editItemAt(configId);
    config.width = w;
    config.height = h;
    config.xdpi = config.ydpi = getDefaultDensity(w, h);
    return NO_ERROR;
}

int32_t HWComposer::allocateDisplayId() {
    if (mAllocatedDisplayIDs.count() >= mNumDisplays) {
        return NO_MEMORY;
    }
    int32_t id = mAllocatedDisplayIDs.firstUnmarkedBit();
    mAllocatedDisplayIDs.markBit(id);
    mDisplayData[id].connected = true;
    mDisplayData[id].configs.resize(1);
    mDisplayData[id].currentConfig = 0;
    return id;
}

status_t HWComposer::freeDisplayId(int32_t id) {
    if (id < NUM_BUILTIN_DISPLAYS) {
        // cannot free the reserved IDs
        return BAD_VALUE;
    }
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id)) {
        return BAD_INDEX;
    }
    mAllocatedDisplayIDs.clearBit(id);
    mDisplayData[id].connected = false;
    return NO_ERROR;
}

nsecs_t HWComposer::getRefreshTimestamp(int disp) const {
    // this returns the last refresh timestamp.
    // if the last one is not available, we estimate it based on
    // the refresh period and whatever closest timestamp we have.
    Mutex::Autolock _l(mLock);
    nsecs_t now = systemTime(CLOCK_MONOTONIC);
    size_t configId = mDisplayData[disp].currentConfig;
    return now - ((now - mLastHwVSync[disp]) %
            mDisplayData[disp].configs[configId].refresh);
}

sp<Fence> HWComposer::getDisplayFence(int disp) const {
    return mDisplayData[disp].lastDisplayFence;
}

uint32_t HWComposer::getFormat(int disp) const {
    if (uint32_t(disp)>31 || !mAllocatedDisplayIDs.hasBit(disp)) {
        return HAL_PIXEL_FORMAT_RGBA_8888;
    } else {
        return mDisplayData[disp].format;
    }
}

bool HWComposer::isConnected(int disp) const {
    return mDisplayData[disp].connected;
}

uint32_t HWComposer::getWidth(int disp) const {
    size_t currentConfig = mDisplayData[disp].currentConfig;
    return mDisplayData[disp].configs[currentConfig].width;
}

uint32_t HWComposer::getHeight(int disp) const {
    size_t currentConfig = mDisplayData[disp].currentConfig;
    return mDisplayData[disp].configs[currentConfig].height;
}

// MStar Android Patch Begin
int HWComposer::getOsdWidth(int disp) const {
    size_t currentConfig = mDisplayData[disp].currentConfig;
    return mDisplayData[disp].configs[currentConfig].osdWidth;
}

int HWComposer::getOsdHeight(int disp) const {
    size_t currentConfig = mDisplayData[disp].currentConfig;
    return mDisplayData[disp].configs[currentConfig].osdHeight;
}
// MStar Android Patch End

float HWComposer::getDpiX(int disp) const {
    size_t currentConfig = mDisplayData[disp].currentConfig;
    return mDisplayData[disp].configs[currentConfig].xdpi;
}

float HWComposer::getDpiY(int disp) const {
    size_t currentConfig = mDisplayData[disp].currentConfig;
    return mDisplayData[disp].configs[currentConfig].ydpi;
}

// MStar Android Patch Begin
nsecs_t HWComposer::getRefreshPeriod(int disp) {
// MStar Android Patch End
    size_t currentConfig = mDisplayData[disp].currentConfig;
    // MStar Android Patch Begin
    if (disp == HWC_DISPLAY_PRIMARY) {
        nsecs_t refresh = mHwc->getRefreshPeriod(mHwc);
        if (refresh > 0) {
            DisplayConfig& config = mDisplayData[disp].configs.editItemAt(currentConfig);
            config.refresh = refresh;
        }
    }
    // MStar Android Patch End
    return mDisplayData[disp].configs[currentConfig].refresh;
}

const Vector<HWComposer::DisplayConfig>& HWComposer::getConfigs(int disp) const {
    return mDisplayData[disp].configs;
}

size_t HWComposer::getCurrentConfig(int disp) const {
    return mDisplayData[disp].currentConfig;
}

void HWComposer::eventControl(int disp, int event, int enabled) {
    if (uint32_t(disp)>31 || !mAllocatedDisplayIDs.hasBit(disp)) {
        ALOGD("eventControl ignoring event %d on unallocated disp %d (en=%d)",
              event, disp, enabled);
        return;
    }
    if (event != EVENT_VSYNC) {
        ALOGW("eventControl got unexpected event %d (disp=%d en=%d)",
              event, disp, enabled);
        return;
    }
    status_t err = NO_ERROR;
    if (mHwc && !mDebugForceFakeVSync) {
        // NOTE: we use our own internal lock here because we have to call
        // into the HWC with the lock held, and we want to make sure
        // that even if HWC blocks (which it shouldn't), it won't
        // affect other threads.
        Mutex::Autolock _l(mEventControlLock);
        const int32_t eventBit = 1UL << event;
        const int32_t newValue = enabled ? eventBit : 0;
        const int32_t oldValue = mDisplayData[disp].events & eventBit;
        if (newValue != oldValue) {
            ATRACE_CALL();
            err = mHwc->eventControl(mHwc, disp, event, enabled);
            if (!err) {
                int32_t& events(mDisplayData[disp].events);
                events = (events & ~eventBit) | newValue;

                char tag[16];
                snprintf(tag, sizeof(tag), "HW_VSYNC_ON_%1u", disp);
                ATRACE_INT(tag, enabled);
            }
        }
        // error here should not happen -- not sure what we should
        // do if it does.
        ALOGE_IF(err, "eventControl(%d, %d) failed %s",
                event, enabled, strerror(-err));
    }

    if (err == NO_ERROR && mVSyncThread != NULL) {
        mVSyncThread->setEnabled(enabled);
    }
}

status_t HWComposer::createWorkList(int32_t id, size_t numLayers) {
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id)) {
        return BAD_INDEX;
    }

    if (mHwc) {
        DisplayData& disp(mDisplayData[id]);
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
            // we need space for the HWC_FRAMEBUFFER_TARGET
            numLayers++;
        }
        if (disp.capacity < numLayers || disp.list == NULL) {
            size_t size = sizeof(hwc_display_contents_1_t)
                    + numLayers * sizeof(hwc_layer_1_t);
            free(disp.list);
            disp.list = (hwc_display_contents_1_t*)malloc(size);
            disp.capacity = numLayers;
        }
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
            disp.framebufferTarget = &disp.list->hwLayers[numLayers - 1];
            memset(disp.framebufferTarget, 0, sizeof(hwc_layer_1_t));
            const DisplayConfig& currentConfig =
                    disp.configs[disp.currentConfig];
            const hwc_rect_t r = { 0, 0,
                    (int) currentConfig.width, (int) currentConfig.height };
            disp.framebufferTarget->compositionType = HWC_FRAMEBUFFER_TARGET;
            disp.framebufferTarget->hints = 0;
            disp.framebufferTarget->flags = 0;
            disp.framebufferTarget->handle = disp.fbTargetHandle;
            disp.framebufferTarget->transform = 0;
            disp.framebufferTarget->blending = HWC_BLENDING_PREMULT;
            if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_3)) {
                disp.framebufferTarget->sourceCropf.left = 0;
                disp.framebufferTarget->sourceCropf.top = 0;
                disp.framebufferTarget->sourceCropf.right =
                        currentConfig.width;
                disp.framebufferTarget->sourceCropf.bottom =
                        currentConfig.height;
            } else {
                disp.framebufferTarget->sourceCrop = r;
            }
            // MStar Android Patch Begin
            // for 4k2k switch case ,we will delay set the displayFrame
            if (!(mFlinger->get4k2kAnd2k1kSwitch()) && id == DisplayDevice::DISPLAY_PRIMARY) {
                disp.framebufferTarget->displayFrame = r;
            }
            // MStar Android Patch End
            disp.framebufferTarget->visibleRegionScreen.numRects = 1;
            disp.framebufferTarget->visibleRegionScreen.rects =
                &disp.framebufferTarget->displayFrame;
            disp.framebufferTarget->acquireFenceFd = -1;
            disp.framebufferTarget->releaseFenceFd = -1;
            disp.framebufferTarget->planeAlpha = 0xFF;
        }
        disp.list->retireFenceFd = -1;
        disp.list->flags = HWC_GEOMETRY_CHANGED;
        disp.list->numHwLayers = numLayers;
    }
    return NO_ERROR;
}

status_t HWComposer::setFramebufferTarget(int32_t id,
        const sp<Fence>& acquireFence, const sp<GraphicBuffer>& buf) {
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id)) {
        return BAD_INDEX;
    }
    DisplayData& disp(mDisplayData[id]);
    if (!disp.framebufferTarget) {
        // this should never happen, but apparently eglCreateWindowSurface()
        // triggers a Surface::queueBuffer()  on some
        // devices (!?) -- log and ignore.
        ALOGE("HWComposer: framebufferTarget is null");
        return NO_ERROR;
    }

    int acquireFenceFd = -1;
    if (acquireFence->isValid()) {
        acquireFenceFd = acquireFence->dup();
    }

    // ALOGD("fbPost: handle=%p, fence=%d", buf->handle, acquireFenceFd);
    disp.fbTargetHandle = buf->handle;
    disp.framebufferTarget->handle = disp.fbTargetHandle;
    disp.framebufferTarget->acquireFenceFd = acquireFenceFd;
    return NO_ERROR;
}

status_t HWComposer::prepare() {
    for (size_t i=0 ; i<mNumDisplays ; i++) {
        DisplayData& disp(mDisplayData[i]);
        if (disp.framebufferTarget) {
            // make sure to reset the type to HWC_FRAMEBUFFER_TARGET
            // DO NOT reset the handle field to NULL, because it's possible
            // that we have nothing to redraw (eg: eglSwapBuffers() not called)
            // in which case, we should continue to use the same buffer.
            LOG_FATAL_IF(disp.list == NULL);
            disp.framebufferTarget->compositionType = HWC_FRAMEBUFFER_TARGET;
        }
        if (!disp.connected && disp.list != NULL) {
            ALOGW("WARNING: disp %zu: connected, non-null list, layers=%zu",
                  i, disp.list->numHwLayers);
        }
        mLists[i] = disp.list;
        // MStar Android Patch Begin
        if (i < DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES && mLists[i]) {
            const Vector< sp<Layer> >& visibleLayersSortedByZ =
                    mFlinger->getLayerSortedByZForHwcDisplay(i);
            sp<const DisplayDevice> displayDevice(mFlinger->getBuiltInDisplayDevice(i));
            for (size_t j=0; j<disp.list->numHwLayers-1; j++) {
                const sp<Layer>& layer(visibleLayersSortedByZ[j]);
                mLists[i]->hwLayers[j].name = (char*) malloc(strlen(layer->getName().string()) + 1);
                strcpy(mLists[i]->hwLayers[j].name,layer->getName().string());

                const Rect& region = layer->getActiveBuffer() != NULL?
                    layer->getActiveBuffer()->getBounds():Rect(-1,-1);
                const hwc_rect_t r = { 0, 0, region.getWidth(), region.getHeight() };
                mLists[i]->hwLayers[j].activeBufferRegion = r;

                const hwc_rect_t osdr = { 0, 0, displayDevice->getFrame().getWidth(), displayDevice->getFrame().getHeight() };
                mLists[i]->hwLayers[j].osdRegion = osdr;
            }
        }
        // MStar Android Patch End
        if (mLists[i]) {
            if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_3)) {
                mLists[i]->outbuf = disp.outbufHandle;
                mLists[i]->outbufAcquireFenceFd = -1;
            } else if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
                // garbage data to catch improper use
                mLists[i]->dpy = (hwc_display_t)0xDEADBEEF;
                mLists[i]->sur = (hwc_surface_t)0xDEADBEEF;
            } else {
                mLists[i]->dpy = EGL_NO_DISPLAY;
                mLists[i]->sur = EGL_NO_SURFACE;
            }
        }
    }

    int err = mHwc->prepare(mHwc, mNumDisplays, mLists);
    ALOGE_IF(err, "HWComposer: prepare failed (%s)", strerror(-err));

    if (err == NO_ERROR) {
        // here we're just making sure that "skip" layers are set
        // to HWC_FRAMEBUFFER and we're also counting how many layers
        // we have of each type.
        //
        // If there are no window layers, we treat the display has having FB
        // composition, because SurfaceFlinger will use GLES to draw the
        // wormhole region.
        for (size_t i=0 ; i<mNumDisplays ; i++) {
            DisplayData& disp(mDisplayData[i]);
            disp.hasFbComp = false;
            disp.hasOvComp = false;
            if (disp.list) {
                for (size_t i=0 ; i<disp.list->numHwLayers ; i++) {
                    hwc_layer_1_t& l = disp.list->hwLayers[i];

                    //ALOGD("prepare: %d, type=%d, handle=%p",
                    //        i, l.compositionType, l.handle);

                    if (l.flags & HWC_SKIP_LAYER) {
                        l.compositionType = HWC_FRAMEBUFFER;
                    }
                    if (l.compositionType == HWC_FRAMEBUFFER) {
                        disp.hasFbComp = true;
                    }
                    // MStar Android Patch Begin
                    if (l.compositionType == HWC_OVERLAY || l.compositionType == HWC_FRAMEBUFFER_SKIP) {
                    // MStar Android Patch End
                        disp.hasOvComp = true;
                    }
                    if (l.compositionType == HWC_CURSOR_OVERLAY) {
                        disp.hasOvComp = true;
                    }
                }
                // MStar Android Patch Begin
                if (i < DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES && mLists[i]) {
                    //name has alloced some buffer,so we must free it after used.
                    for (size_t j=0 ; j<disp.list->numHwLayers-1; j++) {
                        free(mLists[i]->hwLayers[j].name);
                        mLists[i]->hwLayers[j].name = NULL;
                    }
                }
                // MStar Android Patch End
                if (disp.list->numHwLayers == (disp.framebufferTarget ? 1 : 0)) {
                    disp.hasFbComp = true;
                }
            } else {
                disp.hasFbComp = true;
            }
        }
    }
    return (status_t)err;
}

bool HWComposer::hasHwcComposition(int32_t id) const {
    if (!mHwc || uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id))
        return false;
    return mDisplayData[id].hasOvComp;
}

bool HWComposer::hasGlesComposition(int32_t id) const {
    if (!mHwc || uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id))
        return true;
    return mDisplayData[id].hasFbComp;
}

sp<Fence> HWComposer::getAndResetReleaseFence(int32_t id) {
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id))
        return Fence::NO_FENCE;

    int fd = INVALID_OPERATION;
    if (mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
        const DisplayData& disp(mDisplayData[id]);
        if (disp.framebufferTarget) {
            fd = disp.framebufferTarget->releaseFenceFd;
            disp.framebufferTarget->acquireFenceFd = -1;
            disp.framebufferTarget->releaseFenceFd = -1;
        }
    }
    return fd >= 0 ? new Fence(fd) : Fence::NO_FENCE;
}

status_t HWComposer::commit() {
    int err = NO_ERROR;
    if (mHwc) {
        if (!hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
            // On version 1.0, the OpenGL ES target surface is communicated
            // by the (dpy, sur) fields and we are guaranteed to have only
            // a single display.
            mLists[0]->dpy = eglGetCurrentDisplay();
            mLists[0]->sur = eglGetCurrentSurface(EGL_DRAW);
        }

        for (size_t i=VIRTUAL_DISPLAY_ID_BASE; i<mNumDisplays; i++) {
            DisplayData& disp(mDisplayData[i]);
            if (disp.outbufHandle) {
                mLists[i]->outbuf = disp.outbufHandle;
                mLists[i]->outbufAcquireFenceFd =
                        disp.outbufAcquireFence->dup();
            }
        }
        // MStar Android Patch Begin
        sp<const DisplayDevice> displayDevice(mFlinger->getDefaultDisplayDevice());
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1) || !displayDevice->skipSwapBuffer) {
            err = mHwc->set(mHwc, mNumDisplays, mLists);
        }
        // MStar Android Patch End
        for (size_t i=0 ; i<mNumDisplays ; i++) {
            DisplayData& disp(mDisplayData[i]);
            disp.lastDisplayFence = disp.lastRetireFence;
            disp.lastRetireFence = Fence::NO_FENCE;
            if (disp.list) {
                if (disp.list->retireFenceFd != -1) {
                    disp.lastRetireFence = new Fence(disp.list->retireFenceFd);
                    disp.list->retireFenceFd = -1;
                }
                disp.list->flags &= ~HWC_GEOMETRY_CHANGED;
            }
        }
    }
    return (status_t)err;
}

status_t HWComposer::setPowerMode(int disp, int mode) {
    LOG_FATAL_IF(disp >= VIRTUAL_DISPLAY_ID_BASE);
    if (mHwc) {
        if (mode == HWC_POWER_MODE_OFF) {
            eventControl(disp, HWC_EVENT_VSYNC, 0);
        }
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_4)) {
            return (status_t)mHwc->setPowerMode(mHwc, disp, mode);
        } else {
            return (status_t)mHwc->blank(mHwc, disp,
                    mode == HWC_POWER_MODE_OFF ? 1 : 0);
        }
    }
    return NO_ERROR;
}

status_t HWComposer::setActiveConfig(int disp, int mode) {
    LOG_FATAL_IF(disp >= VIRTUAL_DISPLAY_ID_BASE);
    DisplayData& dd(mDisplayData[disp]);
    dd.currentConfig = mode;
    if (mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_4)) {
        return (status_t)mHwc->setActiveConfig(mHwc, disp, mode);
    } else {
        LOG_FATAL_IF(mode != 0);
    }
    return NO_ERROR;
}

void HWComposer::disconnectDisplay(int disp) {
    LOG_ALWAYS_FATAL_IF(disp < 0 || disp == HWC_DISPLAY_PRIMARY);
    DisplayData& dd(mDisplayData[disp]);
    free(dd.list);
    dd.list = NULL;
    dd.framebufferTarget = NULL;    // points into dd.list
    dd.fbTargetHandle = NULL;
    dd.outbufHandle = NULL;
    dd.lastRetireFence = Fence::NO_FENCE;
    dd.lastDisplayFence = Fence::NO_FENCE;
    dd.outbufAcquireFence = Fence::NO_FENCE;
    // clear all the previous configs and repopulate when a new
    // device is added
    dd.configs.clear();
}

int HWComposer::getVisualID() const {
    if (mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
        // FIXME: temporary hack until HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED
        // is supported by the implementation. we can only be in this case
        // if we have HWC 1.1
        return HAL_PIXEL_FORMAT_RGBA_8888;
        //return HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    } else {
        return mFbDev->format;
    }
}

bool HWComposer::supportsFramebufferTarget() const {
    return (mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1));
}

int HWComposer::fbPost(int32_t id,
        const sp<Fence>& acquireFence, const sp<GraphicBuffer>& buffer) {
    if (mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
        return setFramebufferTarget(id, acquireFence, buffer);
    } else {
        acquireFence->waitForever("HWComposer::fbPost");
        return mFbDev->post(mFbDev, buffer->handle);
    }
}

// MStar Android Patch Begin
int HWComposer::getCurOPTiming(int *width, int *height) {
    if (mHwc->getCurOpTiming) {
        return mHwc->getCurOpTiming(mHwc,width,height);
    }
    return -1;
}

int HWComposer::isStbTarget() {
    if (mHwc->isStbTarget) {
        return mHwc->isStbTarget(mHwc);
    }
    return 0;
}

int HWComposer::isOsdNeedResize() {
    if (mHwc->isOsdNeedResize) {
        return mHwc->isOsdNeedResize(mHwc);
    }
    return 0;
}

int HWComposer::isFrameBufferResizeOpen(){
    return bFrameBufferResizeOpen;
}
// MStar Android Patch End

int HWComposer::fbCompositionComplete() {
    if (mHwc && hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1))
        return NO_ERROR;

    if (mFbDev->compositionComplete) {
        return mFbDev->compositionComplete(mFbDev);
    } else {
        return INVALID_OPERATION;
    }
}

void HWComposer::fbDump(String8& result) {
    if (mFbDev && mFbDev->common.version >= 1 && mFbDev->dump) {
        const size_t SIZE = 4096;
        char buffer[SIZE];
        mFbDev->dump(mFbDev, buffer, SIZE);
        result.append(buffer);
    }
}

status_t HWComposer::setOutputBuffer(int32_t id, const sp<Fence>& acquireFence,
        const sp<GraphicBuffer>& buf) {
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id))
        return BAD_INDEX;
    if (id < VIRTUAL_DISPLAY_ID_BASE)
        return INVALID_OPERATION;

    DisplayData& disp(mDisplayData[id]);
    disp.outbufHandle = buf->handle;
    disp.outbufAcquireFence = acquireFence;
    return NO_ERROR;
}

sp<Fence> HWComposer::getLastRetireFence(int32_t id) const {
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id))
        return Fence::NO_FENCE;
    return mDisplayData[id].lastRetireFence;
}

status_t HWComposer::setCursorPositionAsync(int32_t id, const Rect& pos)
{
    if (mHwc->setCursorPositionAsync) {
        return (status_t)mHwc->setCursorPositionAsync(mHwc, id, pos.left, pos.top);
    }
    else {
        return NO_ERROR;
    }
}

/*
 * Helper template to implement a concrete HWCLayer
 * This holds the pointer to the concrete hwc layer type
 * and implements the "iterable" side of HWCLayer.
 */
template<typename CONCRETE, typename HWCTYPE>
class Iterable : public HWComposer::HWCLayer {
protected:
    HWCTYPE* const mLayerList;
    HWCTYPE* mCurrentLayer;
    Iterable(HWCTYPE* layer) : mLayerList(layer), mCurrentLayer(layer) { }
    inline HWCTYPE const * getLayer() const { return mCurrentLayer; }
    inline HWCTYPE* getLayer() { return mCurrentLayer; }
    virtual ~Iterable() { }
private:
    // returns a copy of ourselves
    virtual HWComposer::HWCLayer* dup() {
        return new CONCRETE( static_cast<const CONCRETE&>(*this) );
    }
    virtual status_t setLayer(size_t index) {
        mCurrentLayer = &mLayerList[index];
        return NO_ERROR;
    }
};

/*
 * Concrete implementation of HWCLayer for HWC_DEVICE_API_VERSION_1_0.
 * This implements the HWCLayer side of HWCIterableLayer.
 */
class HWCLayerVersion1 : public Iterable<HWCLayerVersion1, hwc_layer_1_t> {
    struct hwc_composer_device_1* mHwc;
public:
    HWCLayerVersion1(struct hwc_composer_device_1* hwc, hwc_layer_1_t* layer)
        : Iterable<HWCLayerVersion1, hwc_layer_1_t>(layer), mHwc(hwc) { }

    virtual int32_t getCompositionType() const {
        return getLayer()->compositionType;
    }
    virtual uint32_t getHints() const {
        return getLayer()->hints;
    }
    virtual sp<Fence> getAndResetReleaseFence() {
        int fd = getLayer()->releaseFenceFd;
        getLayer()->releaseFenceFd = -1;
        return fd >= 0 ? new Fence(fd) : Fence::NO_FENCE;
    }
    virtual void setAcquireFenceFd(int fenceFd) {
        getLayer()->acquireFenceFd = fenceFd;
    }
    virtual void setPerFrameDefaultState() {
        //getLayer()->compositionType = HWC_FRAMEBUFFER;
    }
    virtual void setPlaneAlpha(uint8_t alpha) {
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_2)) {
            getLayer()->planeAlpha = alpha;
        } else {
            if (alpha < 0xFF) {
                getLayer()->flags |= HWC_SKIP_LAYER;
            }
        }
    }
    virtual void setDefaultState() {
        hwc_layer_1_t* const l = getLayer();
        l->compositionType = HWC_FRAMEBUFFER;
        l->hints = 0;
        l->flags = HWC_SKIP_LAYER;
        l->handle = 0;
        l->transform = 0;
        l->blending = HWC_BLENDING_NONE;
        l->visibleRegionScreen.numRects = 0;
        l->visibleRegionScreen.rects = NULL;
        l->acquireFenceFd = -1;
        l->releaseFenceFd = -1;
        l->planeAlpha = 0xFF;
        // MStar Android Patch Begin
        l->VendorUsageBits = 0x0;
        // MStar Android Patch End
    }
    virtual void setSkip(bool skip) {
        if (skip) {
            getLayer()->flags |= HWC_SKIP_LAYER;
        } else {
            getLayer()->flags &= ~HWC_SKIP_LAYER;
        }
    }
    virtual void setIsCursorLayerHint(bool isCursor) {
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_4)) {
            if (isCursor) {
                getLayer()->flags |= HWC_IS_CURSOR_LAYER;
            }
            else {
                getLayer()->flags &= ~HWC_IS_CURSOR_LAYER;
            }
        }
    }
    virtual void setBlending(uint32_t blending) {
        getLayer()->blending = blending;
    }
    virtual void setTransform(uint32_t transform) {
        getLayer()->transform = transform;
    }
    virtual void setFrame(const Rect& frame) {
        getLayer()->displayFrame = reinterpret_cast<hwc_rect_t const&>(frame);
    }
    virtual void setCrop(const FloatRect& crop) {
        if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_3)) {
            getLayer()->sourceCropf = reinterpret_cast<hwc_frect_t const&>(crop);
        } else {
            /*
             * Since h/w composer didn't support a flot crop rect before version 1.3,
             * using integer coordinates instead produces a different output from the GL code in
             * Layer::drawWithOpenGL(). The difference can be large if the buffer crop to
             * window size ratio is large and a window crop is defined
             * (i.e.: if we scale the buffer a lot and we also crop it with a window crop).
             */
            hwc_rect_t& r = getLayer()->sourceCrop;
            r.left  = int(ceilf(crop.left));
            r.top   = int(ceilf(crop.top));
            r.right = int(floorf(crop.right));
            r.bottom= int(floorf(crop.bottom));
        }
    }
    virtual void setVisibleRegionScreen(const Region& reg) {
        // Region::getSharedBuffer creates a reference to the underlying
        // SharedBuffer of this Region, this reference is freed
        // in onDisplayed()
        hwc_region_t& visibleRegion = getLayer()->visibleRegionScreen;
        SharedBuffer const* sb = reg.getSharedBuffer(&visibleRegion.numRects);
        visibleRegion.rects = reinterpret_cast<hwc_rect_t const *>(sb->data());
    }
    virtual void setSidebandStream(const sp<NativeHandle>& stream) {
        ALOG_ASSERT(stream->handle() != NULL);
        getLayer()->compositionType = HWC_SIDEBAND;
        getLayer()->sidebandStream = stream->handle();
    }
    virtual void setBuffer(const sp<GraphicBuffer>& buffer) {
        if (buffer == 0 || buffer->handle == 0) {
            getLayer()->compositionType = HWC_FRAMEBUFFER;
            getLayer()->flags |= HWC_SKIP_LAYER;
            getLayer()->handle = 0;
        } else {
            if (getLayer()->compositionType == HWC_SIDEBAND) {
                // If this was a sideband layer but the stream was removed, reset
                // it to FRAMEBUFFER. The HWC can change it to OVERLAY in prepare.
                getLayer()->compositionType = HWC_FRAMEBUFFER;
            }
            getLayer()->handle = buffer->handle;
        }
    }
    virtual void onDisplayed() {
        hwc_region_t& visibleRegion = getLayer()->visibleRegionScreen;
        SharedBuffer const* sb = SharedBuffer::bufferFromData(visibleRegion.rects);
        if (sb) {
            sb->release();
            // not technically needed but safer
            visibleRegion.numRects = 0;
            visibleRegion.rects = NULL;
        }

        getLayer()->acquireFenceFd = -1;
    }
    // MStar Android Patch Begin
    virtual uint32_t getVendorUsageBits() {
        return getLayer()->VendorUsageBits;
    }
    // MStar Android Patch End
};

/*
 * returns an iterator initialized at a given index in the layer list
 */
HWComposer::LayerListIterator HWComposer::getLayerIterator(int32_t id, size_t index) {
    if (uint32_t(id)>31 || !mAllocatedDisplayIDs.hasBit(id)) {
        return LayerListIterator();
    }
    const DisplayData& disp(mDisplayData[id]);
    if (!mHwc || !disp.list || index > disp.list->numHwLayers) {
        return LayerListIterator();
    }
    return LayerListIterator(new HWCLayerVersion1(mHwc, disp.list->hwLayers), index);
}

/*
 * returns an iterator on the beginning of the layer list
 */
HWComposer::LayerListIterator HWComposer::begin(int32_t id) {
    return getLayerIterator(id, 0);
}

/*
 * returns an iterator on the end of the layer list
 */
HWComposer::LayerListIterator HWComposer::end(int32_t id) {
    size_t numLayers = 0;
    if (uint32_t(id) <= 31 && mAllocatedDisplayIDs.hasBit(id)) {
        const DisplayData& disp(mDisplayData[id]);
        if (mHwc && disp.list) {
            numLayers = disp.list->numHwLayers;
            if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_1)) {
                // with HWC 1.1, the last layer is always the HWC_FRAMEBUFFER_TARGET,
                // which we ignore when iterating through the layer list.
                ALOGE_IF(!numLayers, "mDisplayData[%d].list->numHwLayers is 0", id);
                if (numLayers) {
                    numLayers--;
                }
            }
        }
    }
    return getLayerIterator(id, numLayers);
}

// Converts a PixelFormat to a human-readable string.  Max 11 chars.
// (Could use a table of prefab String8 objects.)
static String8 getFormatStr(PixelFormat format) {
    switch (format) {
    case PIXEL_FORMAT_RGBA_8888:    return String8("RGBA_8888");
    case PIXEL_FORMAT_RGBX_8888:    return String8("RGBx_8888");
    case PIXEL_FORMAT_RGB_888:      return String8("RGB_888");
    case PIXEL_FORMAT_RGB_565:      return String8("RGB_565");
    case PIXEL_FORMAT_BGRA_8888:    return String8("BGRA_8888");
    case PIXEL_FORMAT_sRGB_A_8888:  return String8("sRGB_A_8888");
    case PIXEL_FORMAT_sRGB_X_8888:  return String8("sRGB_x_8888");
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
                                    return String8("ImplDef");
    default:
        String8 result;
        result.appendFormat("? %08x", format);
        return result;
    }
}

void HWComposer::dump(String8& result) const {
    if (mHwc) {
        result.appendFormat("Hardware Composer state (version %08x):\n", hwcApiVersion(mHwc));
        result.appendFormat("  mDebugForceFakeVSync=%d\n", mDebugForceFakeVSync);
        for (size_t i=0 ; i<mNumDisplays ; i++) {
            const DisplayData& disp(mDisplayData[i]);
            if (!disp.connected)
                continue;

            const Vector< sp<Layer> >& visibleLayersSortedByZ =
                    mFlinger->getLayerSortedByZForHwcDisplay(i);


            result.appendFormat("  Display[%zd] configurations (* current):\n", i);
            for (size_t c = 0; c < disp.configs.size(); ++c) {
                const DisplayConfig& config(disp.configs[c]);
                result.appendFormat("    %s%zd: %ux%u, xdpi=%f, ydpi=%f, refresh=%" PRId64 "\n",
                        c == disp.currentConfig ? "* " : "", c, config.width, config.height,
                        config.xdpi, config.ydpi, config.refresh);
            }

            if (disp.list) {
                result.appendFormat(
                        "  numHwLayers=%zu, flags=%08x\n",
                        disp.list->numHwLayers, disp.list->flags);

                result.append(
                        "    type   |  handle  | hint | flag | tr | blnd |   format    |     source crop (l,t,r,b)      |          frame         | name \n"
                        "-----------+----------+------+------+----+------+-------------+--------------------------------+------------------------+------\n");
                //      " _________ | ________ | ____ | ____ | __ | ____ | ___________ |_____._,_____._,_____._,_____._ |_____,_____,_____,_____ | ___...
                for (size_t i=0 ; i<disp.list->numHwLayers ; i++) {
                    const hwc_layer_1_t&l = disp.list->hwLayers[i];
                    int32_t format = -1;
                    String8 name("unknown");
                    // MStar Android Patch Begin
                    int w = 0, h = 0, stride = 0;
                    // MStar Android Patch End

                    if (i < visibleLayersSortedByZ.size()) {
                        const sp<Layer>& layer(visibleLayersSortedByZ[i]);
                        const sp<GraphicBuffer>& buffer(
                                layer->getActiveBuffer());
                        if (buffer != NULL) {
                            format = buffer->getPixelFormat();
                            // MStar Android Patch Begin
                            w = buffer->getWidth();
                            h = buffer->getHeight();
                            // MStar Android Patch End
                        }
                        name = layer->getName();
                    }

                    int type = l.compositionType;
                    if (type == HWC_FRAMEBUFFER_TARGET) {
                        name = "HWC_FRAMEBUFFER_TARGET";
                        format = disp.format;
                    }

                    static char const* compositionTypeName[] = {
                            "GLES",
                            "HWC",
                            "BKGND",
                            "FB TARGET",
                            "SIDEBAND",
                            "HWC_CURSOR",
                            // MStar Android Patch Begin
                            "FB SKIP",
                            // MStar Android Patch End
                            "UNKNOWN"};
                    if (type >= NELEM(compositionTypeName))
                        type = NELEM(compositionTypeName) - 1;

                    // MStar Android Patch Begin
                    if (i < visibleLayersSortedByZ.size()) {
                        const sp<Layer>& layer(visibleLayersSortedByZ[i]);
                        if (layer != NULL) {
                            const sp<GraphicBuffer>& buffer(layer->getActiveBuffer());
                            if (buffer != NULL) {
                                char prefix[256];
                                snprintf(prefix, 256, "HWC_Z%d_Typ_%s_Disp_L%dT%dR%dB%d_w%dXh%dS%d",
                                       static_cast<int>(i), compositionTypeName[type], l.displayFrame.left, l.displayFrame.top, l.displayFrame.right, l.displayFrame.bottom,
                                       w,h, stride);
                                graphic_buffer_dump_helper::set_prefix(prefix);
                                graphic_buffer_dump_helper::dump_graphic_buffer_if_needed(buffer);
                                graphic_buffer_dump_helper::set_prefix( "unknow_");
                            }
                        }
                    }
                    // MStar Android Patch End

                    String8 formatStr = getFormatStr(format);
                    if (hwcHasApiVersion(mHwc, HWC_DEVICE_API_VERSION_1_3)) {
                        result.appendFormat(
                                " %9s | %08" PRIxPTR " | %04x | %04x | %02x | %04x | %-11s |%7.1f,%7.1f,%7.1f,%7.1f |%5d,%5d,%5d,%5d | %s\n",
                                        compositionTypeName[type],
                                        intptr_t(l.handle), l.hints, l.flags, l.transform, l.blending, formatStr.string(),
                                        l.sourceCropf.left, l.sourceCropf.top, l.sourceCropf.right, l.sourceCropf.bottom,
                                        l.displayFrame.left, l.displayFrame.top, l.displayFrame.right, l.displayFrame.bottom,
                                        name.string());
                    } else {
                        result.appendFormat(
                                " %9s | %08" PRIxPTR " | %04x | %04x | %02x | %04x | %-11s |%7d,%7d,%7d,%7d |%5d,%5d,%5d,%5d | %s\n",
                                        compositionTypeName[type],
                                        intptr_t(l.handle), l.hints, l.flags, l.transform, l.blending, formatStr.string(),
                                        l.sourceCrop.left, l.sourceCrop.top, l.sourceCrop.right, l.sourceCrop.bottom,
                                        l.displayFrame.left, l.displayFrame.top, l.displayFrame.right, l.displayFrame.bottom,
                                        name.string());
                    }
                }
            }
        }
    }

    if (mHwc && mHwc->dump) {
        const size_t SIZE = 4096;
        char buffer[SIZE];
        mHwc->dump(mHwc, buffer, SIZE);
        result.append(buffer);
    }
}

// ---------------------------------------------------------------------------

HWComposer::VSyncThread::VSyncThread(HWComposer& hwc)
    : mHwc(hwc), mEnabled(false),
      mNextFakeVSync(0),
      mRefreshPeriod(hwc.getRefreshPeriod(HWC_DISPLAY_PRIMARY))
{
}

void HWComposer::VSyncThread::setEnabled(bool enabled) {
    Mutex::Autolock _l(mLock);
    if (mEnabled != enabled) {
        mEnabled = enabled;
        mCondition.signal();
    }
}

void HWComposer::VSyncThread::onFirstRef() {
    run("VSyncThread", PRIORITY_URGENT_DISPLAY + PRIORITY_MORE_FAVORABLE);
}

bool HWComposer::VSyncThread::threadLoop() {
    { // scope for lock
        Mutex::Autolock _l(mLock);
        while (!mEnabled) {
            mCondition.wait(mLock);
        }
    }

    const nsecs_t period = mRefreshPeriod;
    const nsecs_t now = systemTime(CLOCK_MONOTONIC);
    nsecs_t next_vsync = mNextFakeVSync;
    nsecs_t sleep = next_vsync - now;
    if (sleep < 0) {
        // we missed, find where the next vsync should be
        sleep = (period - ((now - next_vsync) % period));
        next_vsync = now + sleep;
    }
    mNextFakeVSync = next_vsync + period;

    struct timespec spec;
    spec.tv_sec  = next_vsync / 1000000000;
    spec.tv_nsec = next_vsync % 1000000000;

    int err;
    do {
        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
    } while (err<0 && errno == EINTR);

    if (err == 0) {
        mHwc.mEventHandler.onVSyncReceived(0, next_vsync);
    }

    return true;
}

HWComposer::DisplayData::DisplayData()
:   configs(),
    currentConfig(0),
    format(HAL_PIXEL_FORMAT_RGBA_8888),
    connected(false),
    hasFbComp(false), hasOvComp(false),
    capacity(0), list(NULL),
    framebufferTarget(NULL), fbTargetHandle(0),
    lastRetireFence(Fence::NO_FENCE), lastDisplayFence(Fence::NO_FENCE),
    outbufHandle(NULL), outbufAcquireFence(Fence::NO_FENCE),
    events(0)
{}

HWComposer::DisplayData::~DisplayData() {
    free(list);
}

// MStar Android Patch Begin
int HWComposer::initGameCursor() {
    mGameCursorData.addr = 0;
    mGameCursorData.width = -1;
    mGameCursorData.height = -1;
    mGameCursorData.stride = -1;
    mGameCursorData.sourceCrop.makeInvalid();
    mGameCursorData.alpha = 0.0f;
    mGameCursorData.transform.set(0, 0);

    if (mHwc && mHwc->initGameCursor) {
        mHwc->initGameCursor(mHwc);
        mHwc->getGameCursorFbVaddr(mHwc, &(mGameCursorData.addr));
    }
    return 0;
}

int HWComposer::setGameCursorShow() {
    if (mHwc->setGameCursorShow) {
        mHwc->setGameCursorShow(mHwc);
    }
    return 0;
}

int HWComposer::setGameCursorHide() {
    if (mHwc->setGameCursorHide) {
        mHwc->setGameCursorHide(mHwc);
    }
    return 0;
}

int HWComposer::setGameCursorMatrix(float dsdx, float dtdx, float dsdy, float dtdy) {
    float matrix[4];
    matrix[0] = dsdx;
    matrix[1] = dtdx;
    matrix[2] = dsdy;
    matrix[3] = dtdy;
    mGameCursorData.transform.set(matrix[0], 0, 0, matrix[3]);
    mGameCursorData.transform.set(matrix[1], matrix[2]);
    return 0;
}

int HWComposer::setGameCursorWidth(int width) {
    mGameCursorData.width = width;
    return 0;
}

int HWComposer::setGameCursorHeight(int height) {
    mGameCursorData.height = height;
    return 0;
}

int HWComposer::setGameCursorStride(int stride) {
    mGameCursorData.stride = stride;
    return 0;
}

int HWComposer::setGameCursorPosition(float positionX, float positionY) {
    Rect displayFrame, sourceCrop;
    static Rect lastSourceCrop;
    static Rect lastDisplayFrame;
    const sp<const DisplayDevice> hw(mFlinger->getDefaultDisplayDevice());

    mGameCursorData.transform.set(positionX, positionY);

    sourceCrop = Rect(mGameCursorData.width, mGameCursorData.height);
    sourceCrop = mGameCursorData.transform.transform(sourceCrop);
    sourceCrop.intersect(hw->getViewport(), &sourceCrop);
    displayFrame = hw->getTransform().transform(sourceCrop);
    mHwc->setGameCursorPosition(mHwc, displayFrame.left, displayFrame.top);

    //in screen edge, should repaint fb  because sourcecrop is changed
    if((sourceCrop.getBounds() != lastSourceCrop.getBounds())||(displayFrame.getBounds() != lastDisplayFrame.getBounds())) {
        redrawGameCursorFb();
    }
    lastSourceCrop = sourceCrop;
    lastDisplayFrame = displayFrame;

    return 0;
}

int HWComposer::setGameCursorAlpha(float alpha) {
    mGameCursorData.alpha= alpha;
    if (mHwc->setGameCursorAlpha) {
        mHwc->setGameCursorAlpha(mHwc, mGameCursorData.alpha);
    }
    return 0;
}

int HWComposer::redrawGameCursorFb() {
    Rect displayFrame, sourceCrop;
    const sp<const DisplayDevice> hw(mFlinger->getDefaultDisplayDevice());

    sourceCrop = Rect(mGameCursorData.width, mGameCursorData.height);
    sourceCrop = mGameCursorData.transform.transform(sourceCrop);
    sourceCrop.intersect(hw->getViewport(), &sourceCrop);
    sourceCrop = mGameCursorData.transform.inverse().transform(sourceCrop);

    displayFrame = hw->getTransform().transform(sourceCrop);

    //GE Blit factor
    int srcWidth = sourceCrop.getWidth();
    int srcHeight = sourceCrop.getHeight();
    int srcX = sourceCrop.left;
    int srcY = sourceCrop.top;
    int dstWidth  = displayFrame.getWidth();
    int dstHeight = displayFrame.getHeight();
    if (mHwc->redrawGameCursorFb) {
        mHwc->redrawGameCursorFb(mHwc, srcX, srcY, srcWidth, srcHeight, mGameCursorData.stride, dstWidth, dstHeight);
    }
    return 0;
}

int HWComposer::doGameCursorTransaction() {
    if (mHwc->doGameCursorTransaction) {
        mHwc->doGameCursorTransaction(mHwc);
    }
    return 0;
}

intptr_t HWComposer::getGameCursorFbVaddr() {
    return mGameCursorData.addr;
}


bool HWComposer::timingChanged() {
    int opTimingWidth = 1920;
    int opTimingHeight = 1080;
    getCurOPTiming(&opTimingWidth, &opTimingHeight);
    if (mCurrentTimingWidth==opTimingWidth &&
        mCurrentTimingHeight==opTimingHeight) {
        return false;
    }
    mCurrentTimingWidth = opTimingWidth;
    mCurrentTimingHeight = opTimingHeight;
    return true;
}

int HWComposer::redrawHwCursorFb(int srcX, int srcY, int srcWidth, int srcHeight, int stride, int dstWidth, int dstHeight) {
    if (mHwc->redrawHwCursorFb) {
        mHwc->redrawHwCursorFb(mHwc, srcX, srcY, srcWidth, srcHeight, stride, dstWidth, dstHeight);
    }
    return 0;
}

status_t HWComposer::updateFramebufferTargetFrameSize(int32_t id) {
    DisplayData& disp(mDisplayData[id]);
    if (!disp.framebufferTarget) {
        // means hwcomposer 1.0 version
        return NO_ERROR;
    }

    if (mFlinger->get4k2kAnd2k1kSwitch()) {
        if (mFlinger->isChangeTo2k1kMode()) {
            const hwc_rect_t r = { 0, 0, (int) getWidth(id)/2, (int) getHeight(id)/2};
            disp.framebufferTarget->displayFrame = r;
        } else {
            const hwc_rect_t r = { 0, 0, (int) getWidth(id), (int)getHeight(id)};
            disp.framebufferTarget->displayFrame = r;
        }
    }

    return NO_ERROR;
}

status_t HWComposer::updateFramebufferTargetFrameSize(int32_t id, Rect &rect) {
    DisplayData& disp(mDisplayData[id]);
    if (!disp.framebufferTarget) {
        // means hwcomposer 1.0 version
        return NO_ERROR;
    }

    const hwc_rect_t r = { 0, 0, (int) rect.getWidth(), (int) rect.getHeight()};
    disp.framebufferTarget->displayFrame = r;

    return NO_ERROR;
}

// Add code to resize FramebufferTarget SourceCrop
status_t HWComposer::updateFramebufferTargetSourceCropSize(int32_t id, Rect &rect) {
    DisplayData& disp(mDisplayData[id]);
    if (!disp.framebufferTarget) {
        // means hwcomposer 1.0 version
        return NO_ERROR;
    }
    {
        disp.framebufferTarget->sourceCropf.left = rect.left;
        disp.framebufferTarget->sourceCropf.top = rect.top;
        disp.framebufferTarget->sourceCropf.right = rect.right;
        disp.framebufferTarget->sourceCropf.bottom = rect.bottom;
    }
    return NO_ERROR;
}

void HWComposer::recalc3DTransform(Transform** first, Transform** second, Rect& frame) {
    hwc_3d_param_t param[2];
    if (mHwc->recalc3DTransform) {
        const hwc_rect_t r = { frame.left, frame.top, frame.right, frame.bottom};
        mHwc->recalc3DTransform(mHwc,param,r);
    }
    if (param[0].isVisiable == 1) {
        *first = new Transform();
        (*first)->set( param[0].scaleX, 0, 0, param[0].scaleY);
        (*first)->set(param[0].TanslateX, param[0].TanslateY);
    } else {
        (*first) = NULL;
    }
    if (param[1].isVisiable == 1) {
        (*second) = new Transform();
        (*second)->set( param[1].scaleX, 0, 0, param[1].scaleY);
        (*second)->set(param[1].TanslateX, param[1].TanslateY);
    } else {
        (*second) = NULL;
    }
}

void HWComposer::captureScreen(uintptr_t* addr, int* width, int* height, int withOsd) {
     if (mHwc->captureScreen) {
        mHwc->captureScreen(mHwc, addr, width, height, withOsd);
     }
}

void HWComposer::captureScreenWithClip(DefaultKeyedVector< uintptr_t, CaptureData>& captureData, uintptr_t* addr, int* width, int* height, int withOsd) {
     if (mHwc->captureScreenWithClip) {
        size_t count = captureData.size();
        hwc_capture_data_t  mLists[2];
        for (size_t i = 0;i < count && i < 2;i++) {
            CaptureData data = captureData.valueAt(i);
            mLists[i].address = data.address;
            const hwc_rect_t r = { data.pos.left, data.pos.top, data.pos.right, data.pos.bottom};
            mLists[i].region = r;
        }
        ALOGD("[capture screen]:captureScreenWithClip SurfaceView count = %zu",count);
        mHwc->captureScreenWithClip(mHwc, count, mLists, addr, width, height, withOsd);
        for (size_t i = 0;i < count && i < 2;i++) {
            CaptureData data = captureData.valueAt(i);
            data.address = mLists[i].address;
            data.pos = Rect(mLists[i].region.left,mLists[i].region.top,mLists[i].region.right,mLists[i].region.bottom);
            ALOGD("[capture screen]:HWComposer::captureScreenWithClip end CaptureData[%zu].region(%d,%d x %d,%d)",
                i,data.pos.left, data.pos.top, data.pos.right, data.pos.bottom);
            captureData.replaceValueAt(i,data);
        }
     } else if (mHwc->captureScreen) {
        mHwc->captureScreen(mHwc, addr, width, height, withOsd);
     }
}

void HWComposer::releaseCaptureScreenMemory() {
    if (mHwc->releaseCaptureScreenMemory) {
        mHwc->releaseCaptureScreenMemory(mHwc);
    }
}

int HWComposer::getMirrorMode() {
    int mirror = DisplayDevice::DISPLAY_MIRROR_DEFAULT;
    int HMirror = 0,VMirror = 0;
    if (mHwc->getMirrorMode) {
        mHwc->getMirrorMode(mHwc,&HMirror,&VMirror);
    }
    if (HMirror == 1) {
        mirror |= DisplayDevice::DISPLAY_MIRROR_H;
    }
    if (VMirror == 1) {
        mirror |= DisplayDevice::DISPLAY_MIRROR_V;
    }
    return mirror;
}

int HWComposer::isAFBCFramebuffer() {
    if (mHwc->isAFBCFramebuffer) {
        return mHwc->isAFBCFramebuffer(mHwc);
    }
    return 0;
}
// MStar Android Patch End
// ---------------------------------------------------------------------------
}; // namespace android
