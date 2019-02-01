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
#define LOG_TAG "HWCDisplayDevice"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "HWCDisplayDevice.h"

#define __CLASS__ "HWCDisplayDevice"

static bool SortLayersByZ(const std::shared_ptr<HWCLayer>& lhs,
        const std::shared_ptr<HWCLayer>& rhs) {
    return lhs->getLayerZ() < rhs->getLayerZ();
}

std::atomic<hwc2_display_t> HWCDisplayDevice::sNextId(HWC_DISPLAY_PRIMARY);
HWCDisplayDevice::HWCDisplayDevice(HWComposerDevice& device, hwc2_display_type_t type)
    :mDisplayRegionWidth(1920),
    mDisplayRegionHeight(1080),
    bAFBCNeedReset(false),
    bVsyncOff(false),
    b3DModeChanged(false),
    mCurrent3DMode(DISPLAYMODE_NORMAL),
    bAFBCFramebuffer(false),
    bFramebufferNeedRedraw(false),
    mGameCursorChanged(false),
    mGameCursorEnabled(false),
    mId(sNextId++),
    mDevice(device),
    mType(type),
    mColorModes(),
    mActiveColorMode(0),
    mName(getDisplayTypeName(type)),
    mVsyncEnabled(HWC2_VSYNC_INVALID),
    mPowerMode(HWC2_POWER_MODE_OFF),
    mOutputBuffer(),
    mLayers(),
    mSortLayersByZ(),
    mVideoLayers(),
    mGopOverlayLayers(),
    mHwCursors(),
    mConfigs(),
    mActiveConfig(nullptr),
    mLastCompositionType(),
    mChanges(new Changes),
    mClientTarget(nullptr),
    mOsdModule(nullptr),
    mTVVideoModule(nullptr),
    mMMVideoModule(nullptr),
    mDipModule(nullptr),
    mCursorModule(nullptr)
{
    DLOGD("Hw display device mId = %" PRIu64 ",mType = %d", mId, mType);
    if (mType == HWC2_DISPLAY_TYPE_PHYSICAL && mClientTarget == nullptr) {
        mClientTarget = std::make_shared<HWCLayer>(*this);
        mClientTarget->setLayerName("Client Target");
        mClientTarget->setLayerCompositionType(HWC2_COMPOSITION_CLIENT_TARGET);
        DLOGD_IF(HWC2_DEBUGTYPE_HWCLAYER,"Device[%" PRIu64 "] create Client Target  %" PRIu64,mId,mClientTarget->getId());
    }
    loadHardWareModules();
    initConfigs(mWidth,mHeight);

}
HWCDisplayDevice::~HWCDisplayDevice(){
}
void HWCDisplayDevice::loadHardWareModules() {
    if (mType != HWC2_DISPLAY_TYPE_PHYSICAL) {
        return;
    }
    if (mOsdModule == nullptr) {
        mOsdModule = std::make_unique<HWComposerOSD>(*this);
    }
    if (mDipModule == nullptr) {
        mDipModule = std::make_unique<HWComposerDip>(*this);
    }
    if (mTVVideoModule == nullptr) {
        mTVVideoModule = std::make_unique<HWComposerTVVideo>(*this);
    }
    if (mMMVideoModule == nullptr) {
        mMMVideoModule = std::make_unique<HWComposerMMVideo>(*this);
    }
    if (mCursorModule == nullptr) {
        mCursorModule = std::make_unique<HWCCursor>(*this);
    }
}

hwc2_display_t HWCDisplayDevice::getId() const{
    return mId;
}

hwc2_error_t HWCDisplayDevice::acceptChanges() {
    if (!mChanges) {
        DLOGW("Device[%" PRIu64 "] acceptChanges failed, not validated", mId);
        return HWC2_ERROR_NOT_VALIDATED;
    }

    //TODO:resize hw window ,video window etc.
    mLastCompositionType.clear();
    for (auto& layer:mSortLayersByZ) {
        mLastCompositionType.insert({layer->getId(), layer->getLayerCompositionType()});
    }
    mChanges->clearTypeChanges();

    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::createLayer(hwc2_layer_t* outLayerId) {
    auto layer = std::make_shared<HWCLayer>(*this);
    mSortLayersByZ.push_back(layer);
    mLayers.emplace(std::make_pair(layer->getId(), layer));
    *outLayerId = layer->getId();
    DLOGD_IF(HWC2_DEBUGTYPE_HWCLAYER,"Device[%" PRIu64 "] created layer %" PRIu64, mId, *outLayerId);
    std::sort(mSortLayersByZ.begin(),mSortLayersByZ.end(),SortLayersByZ);
    //std::unique(mSortLayersByZ.begin(),mSortLayersByZ.end());
    //reset layer active degree
    mFrameCount = 0;
    for (auto& layer : mSortLayersByZ) {
        layer->resetDegree();
    }
    //layer count is changed,redraw framebuffer
    bFramebufferNeedRedraw = true;
    // here reset the mempool Available
    //mMemAvailable = mem_type_mask;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::destroyLayer(hwc2_layer_t layerId) {
    const auto layerIt = mLayers.find(layerId);
    if (layerIt == mLayers.end()) {
        DLOGE("Device[%" PRIu64 "] destroyLayer(%" PRIu64 ") failed:mLayers has no such layer",
                mId, layerId);
        return HWC2_ERROR_BAD_LAYER;
    }
    const auto layer = layerIt->second;
    std::string name = layer->getLayerName();
    mLayers.erase(layerIt);
    const auto sortLayerIt = std::find(mSortLayersByZ.begin(),mSortLayersByZ.end(),layer);
    if (sortLayerIt == mSortLayersByZ.end()) {
        DLOGE("Device[%" PRIu64 "] destroyLayer(%" PRIu64 ") failed:mSortLayersByZ has no such layer",
                mId, layerId);
        return HWC2_ERROR_BAD_LAYER;
    }
    mSortLayersByZ.erase(sortLayerIt);
    DLOGD_IF(HWC2_DEBUGTYPE_HWCLAYER, "Device[%" PRIu64 "] destroyed layer[%s] %" PRIu64, mId, name.c_str(), layerId);
    //sort  and unique mSortLayersByZ
    std::sort(mSortLayersByZ.begin(),mSortLayersByZ.end(),SortLayersByZ);
    //std::unique(mSortLayersByZ.begin(),mSortLayersByZ.end());
    //reset layer active degree
    mFrameCount = 0;
    for (auto& layer : mSortLayersByZ) {
        layer->resetDegree();
    }
    //layer count is changed,redraw framebuffer
    bFramebufferNeedRedraw = true;
    // here reset the mempool Available
    //mMemAvailable = mem_type_mask;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getChangedCompositionTypes(uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outTypes) {
    if (!mChanges) {
        DLOGE("Device [%" PRIu64 "] getChangedCompositionTypes failed: not validated",
                mId);
        return HWC2_ERROR_NOT_VALIDATED;
    }

    if ((outLayers == nullptr) || (outTypes == nullptr)) {
        *outNumElements = mChanges->getTypeChanges().size();
        return HWC2_ERROR_NONE;
    }

    uint32_t numWritten = 0;
    for (const auto& element : mChanges->getTypeChanges()) {
        if (numWritten == *outNumElements) {
            break;
        }
        auto layerId = element.first;
        auto intType = static_cast<int32_t>(element.second);
        DLOGD_IF(HWC2_DEBUGTYPE_HWCLAYER,"Adding %" PRIu64 " %s", layerId,
                getCompositionName(element.second));
        outLayers[numWritten] = layerId;
        outTypes[numWritten] = intType;
        ++numWritten;
    }
    *outNumElements = numWritten;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::getColorModes(uint32_t* outNumModes, int32_t* outModes) {
    if (!outModes) {
        *outNumModes = mColorModes.size();
        return HWC2_ERROR_NONE;
    }
    uint32_t numModes = std::min(*outNumModes,
            static_cast<uint32_t>(mColorModes.size()));
    std::copy_n(mColorModes.cbegin(), numModes, outModes);
    *outNumModes = numModes;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getDozeSupport(int32_t* outSupport) {
    return HWC2_ERROR_UNSUPPORTED;
}
hwc2_error_t HWCDisplayDevice::getHdrCapabilities(uint32_t* outNumTypes,
        int32_t* outTypes, float* outMaxLuminance,
        float* outMaxAverageLuminance, float* outMinLuminance) {
    return HWC2_ERROR_UNSUPPORTED;
}
hwc2_error_t HWCDisplayDevice::getDisplayName(uint32_t* outSize, char* outName) {
    if (!outName) {
        *outSize = mName.size();
        return HWC2_ERROR_NONE;
    }
    auto numCopied = mName.copy(outName, *outSize);
    *outSize = numCopied;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::getReleaseFences(uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outFences) {
    uint32_t numWritten = 0;
    bool outputsNonNull = (outLayers != nullptr) && (outFences != nullptr);
    for (const auto& layer : mSortLayersByZ) {
        if (outputsNonNull && (numWritten == *outNumElements)) {
            break;
        }

        auto releaseFenceFd = layer->getLayerReleaseFence();
        if (releaseFenceFd != -1) {
            if (outputsNonNull) {
                outLayers[numWritten] = layer->getId();
                auto fd = dup(releaseFenceFd);
                outFences[numWritten] = fd;
                if (fd == -1) {
                    DLOGE_IF(HWC2_DEBUGTYPE_FENCE,"getReleaseFences layer: %" PRIu64 "'s fence[%d],original fence[%d],error %s",
                        outLayers[numWritten],fd,releaseFenceFd,strerror(errno));
                } else {
                    DLOGD_IF(HWC2_DEBUGTYPE_FENCE,"getReleaseFences layer: %" PRIu64 "'s fence[%d],original fence[%d]",
                        outLayers[numWritten],fd,releaseFenceFd);
                }

                close(releaseFenceFd);
                layer->setLayerReleaseFence(-1);
            }
            ++numWritten;
        }
    }
    *outNumElements = numWritten;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::getDisplayRequests(int32_t* outDisplayRequests,
        uint32_t* outNumElements, hwc2_layer_t* outLayers,
        int32_t* outLayerRequests) {

    if (!mChanges) {
        return HWC2_ERROR_NOT_VALIDATED;
    }

    if (outLayers == nullptr || outLayerRequests == nullptr) {
        *outNumElements = mChanges->getNumLayerRequests();
        return HWC2_ERROR_NONE;
    }

    *outDisplayRequests = mChanges->getDisplayRequests();
    uint32_t numWritten = 0;
    for (const auto& request : mChanges->getLayerRequests()) {
        if (numWritten == *outNumElements) {
            break;
        }
        outLayers[numWritten] = request.first;
        outLayerRequests[numWritten] = static_cast<int32_t>(request.second);
        ++numWritten;
    }
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::getDisplayType(int32_t* outType) {
    *outType = static_cast<int32_t>(mType);
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::setClientTarget(buffer_handle_t target,
        int32_t acquireFence, int32_t dataspace, hwc_region_t damage) {
    /**/
    if (target == nullptr) {
        DLOGE("Device[%" PRIu64 "] setClientTarget(%p, %d) failed",
            mId, target, acquireFence);
        return HWC2_ERROR_NOT_VALIDATED;
    }

    if (mClientTarget == nullptr) {
        mClientTarget = std::make_shared<HWCLayer>(*this);
        mClientTarget->setLayerName("Client Target");
        mClientTarget->setLayerCompositionType(HWC2_COMPOSITION_CLIENT_TARGET);
        DLOGD_IF(HWC2_DEBUGTYPE_HWCLAYER,"Device[%" PRIu64 "] create Client Target  %" PRIu64,mId,mClientTarget->getId());
    }
    mClientTarget->setLayerBuffer(target,acquireFence);
    mClientTarget->setLayerDataspace(dataspace);
    private_handle_t *handle = (private_handle_t *)target;
    hwc_frect_t crop= {0.0f,0.0f,(float)handle->width,(float)handle->height};
    mClientTarget->setLayerSourceCrop(crop);
    hwc_region_t rawVisible;
    rawVisible.numRects = 1;
    hwc_rect_t rect = {0,0,handle->width,handle->height};
    rawVisible.rects = &rect;
    mClientTarget->setLayerVisibleRegion(rawVisible);
    hwc_rect_t frame = {0,0,handle->width,handle->height};
    //current timing
    displayFrameFTScaleRatio_w = (float)mCurTimingWidth/handle->width;
    displayFrameFTScaleRatio_h = (float)mCurTimingHeight/handle->height;
    mDisplayRegionWidth = handle->width;
    mDisplayRegionHeight = handle->height;
    mClientTarget->setLayerDisplayFrame(frame);
    mClientTarget->setLayerSurfaceDamage(damage);
    mClientTarget->setLayerBlendMode(HWC2_BLEND_MODE_PREMULTIPLIED);
    mClientTarget->setLayerPlaneAlpha(1.0f);
    mClientTarget->setIndex(mSortLayersByZ.size());
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::setColorMode(int32_t mode) {
    DLOGV("Device[%" PRIu64 "] setColorMode(%d)", mId, mode);

    if (mode == mActiveColorMode) {
        return HWC2_ERROR_NONE;
    }
    if (mColorModes.count(mode) == 0) {
        DLOGE("Device[%" PRIu64 "] Mode %d not found in mColorModes", mId, mode);
        return HWC2_ERROR_UNSUPPORTED;
    }
    //we don't how to implement

    mActiveColorMode = mode;
    return HWC2_ERROR_UNSUPPORTED;
}
hwc2_error_t HWCDisplayDevice::setColorTransform(const float* /*matrix*/ matrix,
        int32_t /*android_color_transform_t*/ hint) {
    DLOGD("Device[%" PRIu64 "] setColorTransform(%d)", mId,
        static_cast<int32_t>(hint));
    //we don't how to implement
    return HWC2_ERROR_UNSUPPORTED;
}
hwc2_error_t HWCDisplayDevice::setOutputBuffer(buffer_handle_t buffer,
        int32_t releaseFence) {
    DLOGV("Device[%" PRIu64 "] setOutputBuffer(%p, %d)", mId, buffer, releaseFence);
    if (mType != HWC2_DISPLAY_TYPE_VIRTUAL) {
        return HWC2_ERROR_UNSUPPORTED;
    }
    mOutputBuffer.setBuffer(buffer);
    mOutputBuffer.setFence(releaseFence);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::setPowerMode(int32_t /*PowerMode*/ mode) {
    auto powerMode = static_cast<hwc2_power_mode_t>(mode);
    DLOGD("Device[%" PRIu64 "] setPowerMode(%s)", mId, getPowerModeName(powerMode));
    return HWC2_ERROR_UNSUPPORTED;
}

static bool isValid(hwc2_vsync_t enable) {
    switch (enable) {
        case HWC2_VSYNC_ENABLE: // Fall-through
        case HWC2_VSYNC_DISABLE: return true;
        default: return false;
    }
}

hwc2_error_t HWCDisplayDevice::setVsyncEnabled(int32_t /*Vsync*/ enabled) {
    std::unique_lock <std::mutex> lock(mMutex);
    auto enable = static_cast<hwc2_vsync_t>(enabled);
    if (!isValid(enable)) {
        return HWC2_ERROR_BAD_PARAMETER;
    }
    if (enable == mVsyncEnabled) {
        return HWC2_ERROR_NONE;
    }
    mVsyncEnabled = enable;
    return HWC2_ERROR_NONE;
}

bool HWCDisplayDevice::isVsyncEnabled() {
    std::unique_lock <std::mutex> lock(mMutex);
    return mVsyncEnabled == HWC2_VSYNC_ENABLE;
}

bool HWCDisplayDevice::layerSupportOverlay(std::shared_ptr<HWCLayer>& inlayer) {
    if (inlayer->isVideoOverlay()) {
        return true;
    }
    if (inlayer->canChangetoOverlay()) {
        // when coming here ,the layer satisfied the basic conditions of GOPOVERLAY except the memory type
        // If the layer counts has been stable ,try to allocte contiguous memory for the proper layers.
        // if the layer count has not changed in the past 20 frames, the layers count is decided as stable
        // HardWare limited: On Each MIU only one layer can allocate contiguous memory.
        if ((mRequiredGopOverlycount < MAX_UIOVERLAY_COUNT)&&(mFrameCount>20)) {
            // only check the lowest zorder layer and the highest zorder layer excluding framebufferTarget layer
            if ((inlayer->getIndex() == 0)
                ||(inlayer->getIndex() == (int)(mSortLayersByZ.size()-1))) {
                inlayer->requestChangeContiguousMemory();
            }
        }
        return inlayer->isOverlay();
    }
    return false;
}

void HWCDisplayDevice::setCompositionTypeForAFBCLimitation(bool hasFB) {
    //adjust layer Composition type if gop unsurpport afbc mode.
    //only count UI overlay, no video overlay
    int uiOverlayCount = mGopOverlayLayers.size();

    if (uiOverlayCount != 0){
        bool bGopoverlayToFB = false;
        if (hasFB) {
            //have framebuffer target and ui overlay together.
            //if framebuffer target or ui overlay is 4k.rollback all ui overlays to framebuffer
            for (auto& layer:mSortLayersByZ) {
                if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                        !layer->isVideoOverlay() && layer->isAFBCFormat()){
                    unsigned int width = 1920,height = 1080;
                    width = layer->getLayerSourceCrop().right - layer->getLayerSourceCrop().left;
                    height = layer->getLayerSourceCrop().bottom - layer->getLayerSourceCrop().top;

                    if (width == RES_4K2K_WIDTH && height == RES_4K2K_HEIGHT) {
                        bGopoverlayToFB = true;
                        break;
                    }
                }
            }
        } else {
            //have ui overlay only.
            //if has 4k ui overlay,rollback all ui overlays to framebuffer
            if (uiOverlayCount > 1) {
                for (auto& layer:mSortLayersByZ) {
                    if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                        !layer->isVideoOverlay() && layer->isAFBCFormat()){
                        unsigned int width = 1920,height = 1080;
                        width = layer->getLayerSourceCrop().right - layer->getLayerSourceCrop().left;
                        height = layer->getLayerSourceCrop().bottom - layer->getLayerSourceCrop().top;

                        if (width == RES_4K2K_WIDTH && height == RES_4K2K_HEIGHT) {
                            bGopoverlayToFB = true;
                            break;
                        }
                    }
                }
            }
        }
        if (bGopoverlayToFB) {
            for (auto& layer:mSortLayersByZ) {
                if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                        !layer->isVideoOverlay() && layer->isAFBCFormat()) {
                    layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                    const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                    if (hwLayerIt != mGopOverlayLayers.end()) {
                        mGopOverlayLayers.erase(hwLayerIt);
                    }
                }
            }
        }
    }

    int currentHWWindowIndex = 0;
    int framebuffertargetHWWindowIndex = -1;
    for (auto& layer:mSortLayersByZ) {
        if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
            !layer->isVideoOverlay()) {
            //if layer's buffer format is afbc and current gop unsurpport afbc,rollback layer to FB
            if (layer->isAFBCFormat()
                && !checkGopSupportAFBC(currentHWWindowIndex)) {
                layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                if (hwLayerIt != mGopOverlayLayers.end()) {
                    mGopOverlayLayers.erase(hwLayerIt);
                }
                if (framebuffertargetHWWindowIndex == -1) {
                    framebuffertargetHWWindowIndex = currentHWWindowIndex++;
                }
            } else {
                currentHWWindowIndex++;
            }
        } else if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_CLIENT) {
            if (framebuffertargetHWWindowIndex == -1) {
                framebuffertargetHWWindowIndex = currentHWWindowIndex++;
            }
        }
    }

    //adjust gop layer's type & FB target's buffer format if gop unsurpport afbc mode.
    int index = framebuffertargetHWWindowIndex != -1? framebuffertargetHWWindowIndex:0;
    //if framebuffer target is enable and it's gop unsurpport afbc ,turn off framebuffer's afbc.
    bAFBCFramebuffer = mOsdModule->isGopSupportAFBC(index);
    //UI overlay is not exist,we use one gop only.judge 3D display mode.
    //Hardware limitation,In AFBC mode,GOP not support all 3D mode.so we rollback
    //to linear buffer and using gop 3D.
    if (mCurrent3DMode == DISPLAYMODE_NORMAL_FP ||
        mCurrent3DMode == DISPLAYMODE_TOPBOTTOM_HIGH_QUALITY ||
        mCurrent3DMode == DISPLAYMODE_FRAME_ALTERNATIVE_LLRR) {
        bAFBCFramebuffer = false;
    }
    if (Debug::IsFramebufferAFBCDisabled()) {
        // disable framebuffer afbc
        bAFBCFramebuffer = false;
    }
}

void HWCDisplayDevice::setCompositionTypeStageOne() {
    int first_fb = 0, last_fb = 0;
    bool hasfb = false;
    int LayerSize = mSortLayersByZ.size();
    for (auto rbegin = mSortLayersByZ.rbegin(); rbegin != mSortLayersByZ.rend();++rbegin) {
        auto layer = *rbegin;
        if (layer->getLayerCompositionType() == HWC2_COMPOSITION_SIDEBAND) {
            mChanges->addLayerRequest(layer->getId(),HWC2_LAYER_REQUEST_CLEAR_CLIENT_TARGET);
            sideband_handle_t *sidebandStream = (sideband_handle_t *)layer->getSideband();
            if (sidebandStream && sidebandStream->sideband_type != TV_INPUT_SIDEBAND_TYPE_MM_GOP) {
                //make mVideoLayers's order by z-order
                auto it = mVideoLayers.begin();
                mVideoLayers.insert(it,layer);
            }
            continue;
        }
#ifdef ENABLE_HWC14_CURSOR
        if(layer->isHwcursor()) {
            auto it = mHwCursors.begin();
            mHwCursors.insert(it,layer);
            continue;
        }
#endif
        if (layerSupportOverlay(layer)) {
            layer->setLayerCompositionType(HWC2_COMPOSITION_DEVICE);
            if (layer->isVideoOverlay()) {
                //make mVideoLayers's order by z-order
                auto it = mVideoLayers.begin();
                mVideoLayers.insert(it,layer);
            } else {
                auto it = mGopOverlayLayers.begin();
                mGopOverlayLayers.insert(it,layer);
            }
            continue;
        }
        layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
        if (!hasfb) {
            hasfb = true;
            last_fb = layer->getIndex();
        }
        first_fb = layer->getIndex();
    }
    if (hasfb) {
        for (int i = first_fb+1; i < last_fb; i++) {
            auto layer = mSortLayersByZ.at(i);
            if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                !layer->isVideoOverlay()) {
                layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                if (hwLayerIt != mGopOverlayLayers.end()) {
                    mGopOverlayLayers.erase(hwLayerIt);
                }
            }
        }
    }
    int uiOverlayCount = mGopOverlayLayers.size();
    if (hasfb) {
        int needfbcount = uiOverlayCount - MAX_UIOVERLAY_COUNT;
        if (needfbcount > 0) {
            for (int i = first_fb-1; i >= 0 && needfbcount > 0; i--) {
                auto layer = mSortLayersByZ.at(i);
                if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                    !layer->isVideoOverlay()) {
                    layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                    const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                    if (hwLayerIt != mGopOverlayLayers.end()) {
                        mGopOverlayLayers.erase(hwLayerIt);
                    }
                    needfbcount--;
                }
            }
            if (needfbcount > 0) {
                for (int i = last_fb+1; i < LayerSize && needfbcount > 0; i++) {
                    auto layer = mSortLayersByZ.at(i);
                    if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                        !layer->isVideoOverlay()) {
                        layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                        const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                        if (hwLayerIt != mGopOverlayLayers.end()) {
                            mGopOverlayLayers.erase(hwLayerIt);
                        }
                        needfbcount--;
                    }
                }
            }
            if (needfbcount > 0) {
                DLOGE("needfbcount too much");
            }
            // Maybe still has the case the framebufferTarget is not the lowest gop overlay
            // this will make the the gop alpha blend result different from gles blend..
        }
    } else {
        if (uiOverlayCount - mGopCount >= 0) {
            int needfbcount = uiOverlayCount - MAX_UIOVERLAY_COUNT;
            for (int i = 0; i < LayerSize && needfbcount > 0; i++) {
                auto layer = mSortLayersByZ.at(i);
                if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                        !layer->isVideoOverlay()) {
                    layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                    const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                    if (hwLayerIt != mGopOverlayLayers.end()) {
                        mGopOverlayLayers.erase(hwLayerIt);
                    }
                    needfbcount--;
                    hasfb = true;
                }
            }
            if (needfbcount > 0) {
                DLOGE("needfbcount too much");
            }
        }
    }
    // The GOP and video zorder will not be changed,GOP always on the video window.
    // so if there is ui layer under the video layer.
    // the video player need to  be set as HWC2_LAYER_REQUEST_CLEAR_CLIENT_TARGET
    // and the ui layer need to be set as HWC2_COMPOSITION_CLIENT compositiontype.
    for (auto& videoLayer:mVideoLayers) {
        int videoIndex = videoLayer->getIndex();
        for(int j= videoIndex - 1; j>=0; j--) {
            auto layer =  mSortLayersByZ.at(j);
            if ((layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE
                && !layer->isVideoOverlay())
                || (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_CLIENT)) {
                hasfb = true;
                layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
                const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),layer);
                if (hwLayerIt != mGopOverlayLayers.end()) {
                    mGopOverlayLayers.erase(hwLayerIt);
                }
                mChanges->addLayerRequest(videoLayer->getId(),HWC2_LAYER_REQUEST_CLEAR_CLIENT_TARGET);
            }
        }
    }
#ifdef MALI_AFBC_GRALLOC
    setCompositionTypeForAFBCLimitation(hasfb);
#endif

    mGopOverlayLayers.clear();
    int currentHWWindowIndex = 0;
    int framebuffertargetHWWindowIndex = -1;
    for (auto& layer:mSortLayersByZ) {
        if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
            !layer->isVideoOverlay()) {
            currentHWWindowIndex++;
            mGopOverlayLayers.push_back(layer);
        } else if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_CLIENT) {
            if (framebuffertargetHWWindowIndex == -1) {
                framebuffertargetHWWindowIndex = currentHWWindowIndex++;
                mGopOverlayLayers.push_back(mClientTarget);
            }
        }
    }
}

bool HWCDisplayDevice::checkClientTargetNeedRedraw() {
    bool bNeedGlesBlend = false;
    int clientLayerCount = 0;
    for (auto& layer:mSortLayersByZ) {
        //if mstar.desk-enable-gamecursor set, the system cursor should't show
        if(layer->getLayerCompositionType() == HWC2_COMPOSITION_CURSOR
            && mGameCursorEnabled) {
            layer->setLayerCompositionType(HWC2_COMPOSITION_DEVICE);
            const auto hwCursorIt = std::find(mHwCursors.begin(),mHwCursors.end(),layer);
            if (hwCursorIt != mHwCursors.end()) {
                mHwCursors.erase(hwCursorIt);
            }
            continue;
        }
        if (std::find(mVideoLayers.begin(),mVideoLayers.end(),layer) != mVideoLayers.end()) {
            auto requests = mChanges->getLayerRequests();
            if (requests.count(layer->getId()) == 0) {
                continue;
            }
        }
        //check composition type
        const auto layerIt = mLastCompositionType.find(layer->getId());
        if (layerIt != mLastCompositionType.end()) {
            if(layerIt->second != layer->getLayerPendingCompositionType()) {
                bNeedGlesBlend = true;
            }
        }
        if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE &&
                !layer->isVideoOverlay()) {
            continue;
        }
        if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_CLIENT) {
            clientLayerCount++;
        }
        if (layer->isDirty()) {
            bNeedGlesBlend = true;
        }
    }

    if (mClientLayerCount != clientLayerCount) {
        bNeedGlesBlend = true;
        mClientLayerCount = clientLayerCount;
    }
    if (b3DModeChanged) {
        DLOGD("3D Mode changed need GlesBlend");
        bNeedGlesBlend = true;
    }
    if (bFramebufferNeedRedraw) {
        bNeedGlesBlend = true;
        bFramebufferNeedRedraw = false;
    }
    if (Debug::IsForceGlesBlendEnabled()) {
        // force Rerender to the FramebufferTarget,
        bNeedGlesBlend = true;
    }
    return bNeedGlesBlend;
}
void HWCDisplayDevice::setCompositionTypeStageTwo() {
    for (auto& layer:mSortLayersByZ) {
        if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_DEVICE
            || layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_SIDEBAND
        ) {
            mChanges->removeLayerRequest(layer->getId());
        }
        if (layer->getLayerPendingCompositionType() == HWC2_COMPOSITION_CLIENT) {
            layer->setLayerCompositionType(HWC2_COMPOSITION_DEVICE);//skip redraw
            layer->setLayerSkipRender(true);
        }
    }
}

hwc2_error_t HWCDisplayDevice::validate(uint32_t* outNumTypes,
        uint32_t* outNumRequests) {
    //accept SF's composition change
    for (unsigned int i = 0;i < mSortLayersByZ.size();i++) {
       auto layer = mSortLayersByZ.at(i);
       layer->acceptPendingCompositionType(false);
       layer->setIndex(i);
       layer->setLayerSkipRender(false);
    }
    mChanges.reset(new Changes);
    mGopOverlayLayers.clear();
    mVideoLayers.clear();
    mHwCursors.clear();
    mFrameCount++;
    mRequiredGopOverlycount = 0;
    if (mType == HWC2_DISPLAY_TYPE_PHYSICAL) {
        setCompositionTypeStageOne();
        bool bneedGlesBlend = checkClientTargetNeedRedraw();
        if (!bneedGlesBlend) {
            setCompositionTypeStageTwo();
        }
    } else {
        for (auto& layer:mSortLayersByZ) {
            layer->setLayerCompositionType(HWC2_COMPOSITION_CLIENT);
        }
    }
    //accept HWC compostion type changes
    for (auto& layer : mSortLayersByZ) {
       layer->acceptPendingCompositionType(true);
    }
    //fallback memory type for inactivity layer.
    for (auto& layer : mSortLayersByZ) {
        layer->requestChangeDiscreteMemory();
    }
    *outNumTypes = mChanges->getNumTypes();
    *outNumRequests = mChanges->getNumLayerRequests();
    return *outNumTypes > 0 ? HWC2_ERROR_HAS_CHANGES : HWC2_ERROR_NONE;
}

bool HWCDisplayDevice::isCursorAndGopLayer(const std::shared_ptr<HWCLayer>& inlayer) {
    const auto hwCursorLayerIt = std::find(mHwCursors.begin(),mHwCursors.end(),inlayer);
    if (hwCursorLayerIt != mHwCursors.end()) {
        return true;
    }
    const auto hwLayerIt = std::find(mGopOverlayLayers.begin(),mGopOverlayLayers.end(),inlayer);
    if (hwLayerIt != mGopOverlayLayers.end()) {
        return true;
    }
    return false;
}

hwc2_error_t HWCDisplayDevice::present(int32_t* outRetireFence) {
    //close acquire fence except cursor and gop's layers
    for (const auto& element : mLayers) {
        const auto& layer = element.second;
        if (!isCursorAndGopLayer(layer)) {
            int32_t fence = layer->getLayerBuffer().getFence();
            if (fence >= 0) {
                close(fence);
                DLOGD_IF(HWC2_DEBUGTYPE_FENCE,"close layer: %" PRIu64 "'s fence[%d],mLayers size:%zu",
                    layer->getId(),fence,mLayers.size());
                layer->getLayerBuffer().setFence(-1);
            }
        }
    }
    *outRetireFence = -1;
    if (mType != HWC2_DISPLAY_TYPE_PHYSICAL) {
        return HWC2_ERROR_NONE;
    }
    mOsdModule->present(mGopOverlayLayers,outRetireFence);
#ifdef ENABLE_HWC14_CURSOR
    mCursorModule->present(mHwCursors,nullptr);
#endif
    mMMVideoModule->setOsdRegion(mClientTarget->getLayerDisplayFrame());
    mMMVideoModule->present(mVideoLayers,nullptr);
    mTVVideoModule->present(mVideoLayers,nullptr);
    for (auto& layer:mSortLayersByZ) {
        layer->resetDirty();
    }
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::updateLayerZ(hwc2_layer_t layerId, uint32_t z) {

    const auto layerIt = mLayers.find(layerId);
    if (layerIt == mLayers.end()) {
        DLOGW("Device[%" PRIu64 "] updateLayerZ(%" PRIu64 ") failed:mLayers has no such layer",
                mId, layerId);
        return HWC2_ERROR_BAD_LAYER;
    }

    const auto layer = layerIt->second;
    const auto sortLayerIt = std::find(mSortLayersByZ.begin(),mSortLayersByZ.end(),layer);
    if (sortLayerIt == mSortLayersByZ.end()) {
        DLOGW("Device[%" PRIu64 "] updateLayerZ(%" PRIu64 ") failed:mSortLayersByZ has no such layer",
                mId, layerId);
        return HWC2_ERROR_BAD_LAYER;
    }
    if (layer->getLayerZ() == z) {
        // Don't change anything if the Z hasn't changed
        return HWC2_ERROR_NONE;
    }


    layer->setLayerZ(z);
    //sort  and unique mSortLayersByZ
    std::sort(mSortLayersByZ.begin(),mSortLayersByZ.end(),SortLayersByZ);
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::setCursorPosition(hwc2_layer_t layerId, int32_t x, int32_t y) {
    const auto layerIt = mLayers.find(layerId);
    if (layerIt == mLayers.end()) {
        DLOGW("Device[%" PRIu64 "] updateLayerZ(%" PRIu64 ") failed:mLayers has no such layer",
                mId, layerId);
        return HWC2_ERROR_BAD_LAYER;
    }

    const auto layer = layerIt->second;
    if (layer->getLayerCompositionType() != HWC2_COMPOSITION_CURSOR) {
        return HWC2_ERROR_BAD_LAYER;
    }
    return mCursorModule->setCursorPositionAsync(x,y);
}

hwc2_error_t HWCDisplayDevice::reDrawHwCursorFb(hwc2_layer_t layerId,
        int32_t srcX, int32_t srcY, int32_t srcWidth,
        int32_t srcHeight, int32_t stride, int32_t dstWidth, int32_t dstHeight) {
    const auto layerIt = mLayers.find(layerId);
    if (layerIt == mLayers.end()) {
        DLOGW("Device[%" PRIu64 "] updateLayerZ(%" PRIu64 ") failed:mLayers has no such layer",
                mId, layerId);
        return HWC2_ERROR_BAD_LAYER;
    }

    const auto layer = layerIt->second;
    if (layer->getLayerCompositionType() != HWC2_COMPOSITION_CURSOR) {
        return HWC2_ERROR_BAD_LAYER;
    }
    return mCursorModule->redrawFb(srcX, srcY, srcWidth, srcHeight, stride, dstWidth, dstHeight);
}

uint32_t HWCDisplayDevice::getDefaultGopNumber() {
    return mOsdModule->getDefaultGopNumber();
}

uint32_t HWCDisplayDevice::getGopDest() {
    return mOsdModule.get()->getGopDest();
}

bool HWCDisplayDevice::checkGopTimingChanged() {
    return mOsdModule.get()->checkGopTimingChanged();
}

bool HWCDisplayDevice::checkGopSupportAFBC(int index) {
    return mOsdModule.get()->isGopSupportAFBC(index);
}

hwc2_error_t HWCDisplayDevice::get3DTransform(hwc_3d_param_t* params, hwc_rect_t frame) {
    uint32_t current3DMode = DISPLAYMODE_NORMAL;
    char property[PROPERTY_VALUE_MAX] = {0};
    if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
        current3DMode = atoi(property);
        if (current3DMode >= DISPLAYMODE_MAX) {
            current3DMode = DISPLAYMODE_NORMAL;
        }
    } else {
        current3DMode = DISPLAYMODE_NORMAL;
    }
    if (current3DMode != mCurrent3DMode) {
        b3DModeChanged= true;
        mCurrent3DMode = current3DMode;
    }
    switch (current3DMode) {
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_LEFTRIGHT_FR:
            //left
            params[0].isVisiable = 1;
            params[0].scaleX = 0.5f;
            params[0].scaleY = 1.0f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //right
            params[1].isVisiable = 1;
            params[1].scaleX = 0.5f;
            params[1].scaleY = 1.0f;
            params[1].TanslateX = (float)(frame.right - frame.left)/2;
            params[1].TanslateY = 0.0f;
            break;
        case DISPLAYMODE_LEFT_ONLY:
            //left
            params[0].isVisiable = 1;
            params[0].scaleX = 0.5f;
            params[0].scaleY = 1.0f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //right
            params[1].isVisiable = 0;
            break;
        case DISPLAYMODE_RIGHT_ONLY:
            //left
            params[0].isVisiable = 0;
            //right
            params[1].isVisiable = 1;
            params[1].scaleX = 0.5f;
            params[1].scaleY = 1.0f;
            params[1].TanslateX = (float)(frame.right - frame.left)/2;
            params[1].TanslateY = 0.0f;
            break;
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOPBOTTOM_LA:
            //top
            params[0].isVisiable = 1;
            params[0].scaleX = 1.0f;
            params[0].scaleY = 0.5f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //bottom
            params[1].isVisiable = 1;
            params[1].scaleX = 1.0f;
            params[1].scaleY = 0.5f;
            params[1].TanslateX = 0.0f;
            params[1].TanslateY = (float)(frame.bottom - frame.top)/2;
            break;
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_FRAME_ALTERNATIVE_LLRR:
            //top
            params[0].isVisiable = 1;
            params[0].scaleX = 1.0f;
            params[0].scaleY = 0.5f;
            params[0].TanslateX = 0.0f;
            params[0].TanslateY = 0.0f;
            //bottom
            params[1].isVisiable = 0;
            break;
        case DISPLAYMODE_BOTTOM_LA:
        case DISPLAYMODE_BOTTOM_ONLY:
            //top
            params[0].isVisiable = 0;
            //bottom
            params[1].isVisiable = 1;
            params[1].scaleX = 1.0f;
            params[1].scaleY = 0.5f;
            params[1].TanslateX = 0.0f;
            params[1].TanslateY = (float)(frame.bottom - frame.top)/2;
            break;
        default:
            //top
            params[0].isVisiable = 0;
            //bottom
            params[1].isVisiable = 0;
    }
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getCurrentTiming(int32_t* width, int32_t* height) {
    if (width != nullptr) {
        *width = mCurTimingWidth;
    }
    if (height != nullptr) {
        *height = mCurTimingHeight;
    }
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::isStbTarget(bool* outValue) {
    *outValue = bStbTarget;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getOsdHeight(int32_t* outHeight) {
    *outHeight = mOsdHeight;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getOsdWidth(int32_t* outWidth) {
    *outWidth = mOsdWidth;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getMirrorMode(uint32_t* outMode) {
    int mirror = HWC2_DISPLAY_MIRROR_DEFAULT;
    if (mMirrorModeH == 1) {
        mirror |= HWC2_DISPLAY_MIRROR_H;
    }
    if (mMirrorModeV == 1) {
        mirror |= HWC2_DISPLAY_MIRROR_V;
    }
    *outMode = mirror;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getRefreshPeriod(nsecs_t* outPeriod) {
    char values[PROPERTY_VALUE_MAX] = {0};
    if (property_get("mstar.override.refresh.rate", values, NULL) > 0) {
        int overridefps = atoi(values);
        *outPeriod = static_cast<nsecs_t>(1e9 / overridefps);
        return HWC2_ERROR_NONE;
    }
    int fps = MApi_XC_GetOutputVFreqX100() / 100;
    if ((fps > 60) || (fps < 24)) {
        DLOGW("the fps get from MApi_XC_GetOutputVFreqX100 is abnormal %d,force the fps to 60",fps);
        fps = 60;
    }
    *outPeriod = static_cast<nsecs_t>(1e9 / fps);
    return HWC2_ERROR_NONE;
}


hwc2_error_t HWCDisplayDevice::captureScreen(uint32_t* outNumElements, hwc2_layer_t* outLayers,
    hwc_capture_data_t* outDatas, uintptr_t* addr, int* width, int* height, int withOsd) {
    if ((outLayers == nullptr) || (outDatas == nullptr)) {
        *outNumElements = mVideoLayers.size();
        return HWC2_ERROR_NONE;
    }
    uint32_t numWritten = 0;
    for (const auto& layer : mVideoLayers) {
        if (numWritten == *outNumElements) {
            break;
        }
        auto layerId = layer->getId();
        outLayers[numWritten] = layerId;
        outDatas[numWritten].region = layer->getLayerDisplayFrame();
        ++numWritten;
    }
    auto err = mDipModule->startCaptureScreenWithClip(*outNumElements,outDatas,width,height,withOsd);
    if (err != HWC2_ERROR_NONE) {
        *addr = 0;
        return HWC2_ERROR_NOT_VALIDATED;
    } else {
        *addr = mDipModule->getCaptureScreenAddr();
    }

    *outNumElements = numWritten;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCDisplayDevice::releaseCaptureScreenMemory() {
    return mDipModule->releaseCaptureScreenMemory();
}

hwc2_error_t HWCDisplayDevice::isRGBColorSpace(bool *outValue) {
    *outValue = bRGBColorSpace;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::isAFBCFramebuffer(bool *outValue) {
    *outValue = bAFBCFramebuffer;
    return HWC2_ERROR_NONE;
}

void HWCDisplayDevice::mmVideoSizeChangedNotify() {
    mMMVideoModule->mmVideoSizeChangedNotify();
}

void HWCDisplayDevice::mmVideoCheckDownScaleXC() {
    mMMVideoModule->mmVideoCheckDownScaleXC();
}

void HWCDisplayDevice::initConfigs(uint32_t width, uint32_t height) {
    switch (mType) {
        case HWC2_DISPLAY_TYPE_PHYSICAL: {
            auto newConfig = std::make_shared<Config>(*this);
            newConfig->setAttribute(HWC2_ATTRIBUTE_WIDTH,mWidth);
            newConfig->setAttribute(HWC2_ATTRIBUTE_HEIGHT,mHeight);
            newConfig->setAttribute(HWC2_ATTRIBUTE_VSYNC_PERIOD,mVsyncPeriod);
            newConfig->setAttribute(HWC2_ATTRIBUTE_DPI_X,mXdpi);
            newConfig->setAttribute(HWC2_ATTRIBUTE_DPI_Y,mYdpi);
            newConfig->setId(static_cast<hwc2_config_t>(mConfigs.size()));
            DLOGD("initConfigs new config %u: %s", newConfig->getId(),
                    newConfig->toString().c_str());
            mConfigs.emplace_back(std::move(newConfig));
            setActiveConfig(0);
            break;
        }
        case HWC2_DISPLAY_TYPE_VIRTUAL: {
            auto newConfig = std::make_shared<Config>(*this);
            newConfig->setAttribute(HWC2_ATTRIBUTE_WIDTH,width);
            newConfig->setAttribute(HWC2_ATTRIBUTE_HEIGHT,height);
            newConfig->setId(static_cast<hwc2_config_t>(mConfigs.size()));
            DLOGD("initConfigs new config %u: %s", newConfig->getId(),
                    newConfig->toString().c_str());
            mConfigs.emplace_back(std::move(newConfig));
            setActiveConfig(0);
            break;
        }
        case HWC2_DISPLAY_TYPE_INVALID: {
            break;
        }
    }
}
std::shared_ptr<const HWCDisplayDevice::Config>
        HWCDisplayDevice::getConfig(hwc2_config_t configId) const
{
    if (configId > mConfigs.size() || !mConfigs[configId]->isOnDisplay(*this)) {
        return nullptr;
    }
    return mConfigs[configId];
}

hwc2_error_t HWCDisplayDevice::setActiveConfig(hwc2_config_t configId) {
    auto config = getConfig(configId);
    if (!config) {
        return HWC2_ERROR_BAD_CONFIG;
    }
    if (config == mActiveConfig) {
        return HWC2_ERROR_NONE;
    }
    mActiveConfig = config;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getActiveConfig(hwc2_config_t* outConfigId) {
    if (!mActiveConfig) {
        DLOGE("Device[%" PRIu64 "] getActiveConfig --> %s", mId,
                getErrorName(HWC2_ERROR_BAD_CONFIG));
        return HWC2_ERROR_BAD_CONFIG;
    }
    auto configId = mActiveConfig->getId();
    DLOGV("Device[%" PRIu64 "] getActiveConfig --> %u : %s", mId, configId,mActiveConfig->toString().c_str());
    *outConfigId = configId;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getDisplayConfigs(uint32_t* outNumConfigs,
        hwc2_config_t* outConfigIds) {
    if (!outConfigIds) {
        *outNumConfigs = mConfigs.size();
        return HWC2_ERROR_NONE;
    }
    uint32_t numWritten = 0;
    for (const auto& config : mConfigs) {
        if (numWritten == *outNumConfigs) {
            break;
        }
        outConfigIds[numWritten] = config->getId();
        ++numWritten;
    }
    *outNumConfigs = numWritten;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCDisplayDevice::getDisplayAttribute(hwc2_config_t configId,
        int32_t /*hwc2_attribute_t*/ inAttribute, int32_t* outValue) {
    auto config = getConfig(configId);
    if (!config) {
        return HWC2_ERROR_BAD_CONFIG;
    }
    auto attribute = static_cast<hwc2_attribute_t>(inAttribute);
    switch (attribute) {
        case HWC2_ATTRIBUTE_VSYNC_PERIOD:
            *outValue = config->getAttribute(HWC2_ATTRIBUTE_VSYNC_PERIOD);
            break;
        case HWC2_ATTRIBUTE_WIDTH:
            *outValue = config->getAttribute(HWC2_ATTRIBUTE_WIDTH);
            break;
        case HWC2_ATTRIBUTE_HEIGHT:
            *outValue = config->getAttribute(HWC2_ATTRIBUTE_HEIGHT);
            break;
        case HWC2_ATTRIBUTE_DPI_X:
            *outValue = config->getAttribute(HWC2_ATTRIBUTE_DPI_X);
            break;
        case HWC2_ATTRIBUTE_DPI_Y:
            *outValue = config->getAttribute(HWC2_ATTRIBUTE_DPI_Y);
            break;
        default:
            DLOGE("getDisplayAttribute: Unknown display attribute: %d (%s)",
                    static_cast<int32_t>(attribute),
                    getAttributeName(attribute));
            return HWC2_ERROR_BAD_CONFIG;
    }
    return HWC2_ERROR_NONE;
}

void HWCDisplayDevice::Config::setAttribute(hwc2_attribute_t attribute,
        int32_t value)
{
    mAttributes[attribute] = value;
}

int32_t HWCDisplayDevice::Config::getAttribute(hwc2_attribute_t attribute) const
{
    if (mAttributes.count(attribute) == 0) {
        return -1;
    }
    return mAttributes.at(attribute);
}

std::string HWCDisplayDevice::Config::toString() const
{
    std::string output;

    const size_t BUFFER_SIZE = 100;
    char buffer[BUFFER_SIZE] = {};
    auto writtenBytes = snprintf(buffer, BUFFER_SIZE,
            "%u x %u", mAttributes.at(HWC2_ATTRIBUTE_WIDTH),
            mAttributes.at(HWC2_ATTRIBUTE_HEIGHT));
    output.append(buffer, writtenBytes);

    if (mAttributes.count(HWC2_ATTRIBUTE_VSYNC_PERIOD) != 0) {
        std::memset(buffer, 0, BUFFER_SIZE);
        writtenBytes = snprintf(buffer, BUFFER_SIZE, " @ %.1f Hz",
                1e9 / mAttributes.at(HWC2_ATTRIBUTE_VSYNC_PERIOD));
        output.append(buffer, writtenBytes);
    }

    if (mAttributes.count(HWC2_ATTRIBUTE_DPI_X) != 0 &&
            mAttributes.at(HWC2_ATTRIBUTE_DPI_X) != -1) {
        std::memset(buffer, 0, BUFFER_SIZE);
        writtenBytes = snprintf(buffer, BUFFER_SIZE,
                ", DPI: %.1f x %.1f",
                mAttributes.at(HWC2_ATTRIBUTE_DPI_X) / 1000.0f,
                mAttributes.at(HWC2_ATTRIBUTE_DPI_Y) / 1000.0f);
        output.append(buffer, writtenBytes);
    }

    return output;
}

std::string HWCDisplayDevice::dump() const {
    std::stringstream output;
    output << "Hwcomposer Display " << mId << ": Type " << mName;
    output << "Power mode: " << getPowerModeName(mPowerMode) << "  ";
    output << "Vsync: " << getVsyncName(mVsyncEnabled) << '\n';

    output << "    Color modes [active]:";
    for (const auto& mode : mColorModes) {
        if (mode == mActiveColorMode) {
            output << " [" << mode << ']';
        } else {
            output << " " << mode;
        }
    }
    output << '\n';

    output << "    " << mConfigs.size() << " Config" <<
            (mConfigs.size() == 1 ? "" : "s") << " (* active)\n";
    for (const auto& config : mConfigs) {
        output << (config == mActiveConfig ? "    * " : "      ");
        output << config->toString() << '\n';
    }

    output << "    " << mLayers.size() << " HWC Layer" <<
            (mLayers.size() == 1 ? "" : "s") << '\n';
    output << "----------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    for (auto& layer : mSortLayersByZ) {
        output << layer->dump();
    }
    if (mClientTarget != nullptr) {
        output << mClientTarget->dump();
    } else {
        output << "    No Client Target";
    }
    if (mOutputBuffer.getBuffer() != nullptr) {
        output << "    Output buffer: " << mOutputBuffer.getBuffer() << '\n';
    }
    output << "curTimingWidth = " << mCurTimingWidth << ", curTimingHeight = " << mCurTimingHeight << '\n';
    output << "panelWidth = " << mPanelWidth << ",panelHeight = " << mPanelHeight <<", panelHStart = " << mPanelHStart << '\n';
    output << "display region = " << mDisplayRegionWidth << " x " << mDisplayRegionHeight << '\n';
    if (mOsdModule != nullptr) {
        output << mOsdModule->dump();
    }
    if (mCursorModule != nullptr) {
        output << mCursorModule->dump();
    }
    return output.str();
}
#ifdef ENABLE_GAMECURSOR
hwc2_error_t HWCDisplayDevice::gameCursorShow(int32_t devNo) {
    return mCursorModule->gameCursorShow(devNo);
}

hwc2_error_t HWCDisplayDevice::gameCursorHide(int32_t devNo) {
    return mCursorModule->gameCursorHide(devNo);
}

hwc2_error_t HWCDisplayDevice::gameCursorSetAlpha(int32_t devNo, float alpha) {
    return mCursorModule->gameCursorSetAlpha(devNo, alpha);
}

hwc2_error_t HWCDisplayDevice::gameCursorSetPosition(int32_t devNo, int32_t positionX, int32_t positionY) {
    return mCursorModule->gameCursorSetPosition(devNo, positionX, positionY);
}

hwc2_error_t HWCDisplayDevice::gameCursorRedrawFb(int32_t devNo, int srcX, int srcY, int srcWidth, int srcHeight,
                int stride,int dstWidth, int dstHeight) {
    return mCursorModule->gameCursorRedrawFb(devNo,srcX, srcY, srcWidth, srcHeight, stride, dstWidth, dstHeight);
}

hwc2_error_t HWCDisplayDevice::gameCursorDoTransaction(int32_t devNo) {
    return mCursorModule->gameCursorDoTransaction(devNo);
}

hwc2_error_t HWCDisplayDevice::gameCursorInit(int32_t devNo) {
    return mCursorModule->gameCursorInit(devNo);
}

hwc2_error_t HWCDisplayDevice::gameCursorDeinit(int32_t devNo) {
    return mCursorModule->gameCursorDeinit(devNo);
}

hwc2_error_t HWCDisplayDevice::gameCursorGetFbVaddrByDevNo(int32_t devNo, uintptr_t* vaddr) {
    return mCursorModule->gameCursorGetFbVaddrByDevNo(devNo, vaddr);
}
#endif
