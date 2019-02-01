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


#ifndef HWCLAYER_H_
#define HWCLAYER_H_
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

#include "HWComposerDevice.h"
#include "HWCDisplayDevice.h"
#include "FencedBuffer.h"
#include <unordered_set>

class HWCDisplayDevice;

class HWCLayer
{
public:
    HWCLayer(HWCDisplayDevice& display);
    ~HWCLayer();
    bool operator==(const HWCLayer& other) { return mId == other.mId; }
    bool operator!=(const HWCLayer& other) { return !(*this == other); }
    void incDegree();
    void decDegree();
    void resetDegree() { mActivityDegree = 0; }
    void incDirty() { mDirtyCount++; }
    void resetDirty() { mDirtyCount = 0; }
    bool isDirty() { return mDirtyCount > 0; }
    // HWC2 Layer functions
    hwc2_error_t setLayerBuffer(buffer_handle_t buffer, int32_t acquireFence);
    hwc2_error_t setCursorPosition(int32_t x, int32_t y);
    hwc2_error_t setLayerSurfaceDamage(hwc_region_t damage);

    // HWC2 Layer state functions
    hwc2_error_t setLayerBlendMode(int32_t /*BlendMode*/ mode);
    int32_t getLayerBlendMode() { return mBlendMode.getValue();}
    hwc2_error_t setLayerColor(hwc_color_t color);
    hwc2_error_t setLayerCompositionType(int32_t /*Composition*/ type);
    hwc2_composition_t getLayerCompositionType() {return mCompositionType.getValue();};
    hwc2_composition_t getLayerPendingCompositionType() {return mCompositionType.getPendingValue();};
    hwc2_error_t setLayerDataspace(int32_t /*android_dataspace_t*/ dataspace);
    hwc2_error_t setLayerDisplayFrame(hwc_rect_t frame);
    hwc_rect_t getLayerDisplayFrame(){ return mDisplayFrame.getValue();}
    hwc2_error_t setLayerPlaneAlpha(float alpha);
    int32_t getLayerPlaneAlpha() { return static_cast<int32_t>(mPlaneAlpha.getValue());}
    hwc2_error_t setLayerSidebandStream(const native_handle_t* stream);
    hwc2_error_t setLayerSourceCrop(hwc_frect_t crop);
    hwc_frect_t getLayerSourceCrop(){ return mSourceCrop.getValue();}
    hwc2_error_t setLayerTransform(int32_t /*Transform*/ transform);
    hwc2_error_t setLayerVisibleRegion(hwc_region_t visible);
    hwc2_error_t setLayerZ(uint32_t z);
    hwc2_error_t setLayerReleaseFence(int32_t fenceFd);
    int32_t getLayerReleaseFence() { return mReleaseFence;}
    hwc2_error_t setLayerName(std::string name);
    std::string& getLayerName() { return mName;}
    hwc2_layer_t getId() const { return mId; }
    HWCDisplayDevice& getDisplay() const { return mDisplay; }
    uint32_t getLayerZ() const { return mZ; }
    std::string dump() const;
    FencedBuffer& getLayerBuffer() { return mBuffer;}
    bool canChangetoOverlay();
    void requestChangeContiguousMemory();
    void requestChangeDiscreteMemory();
    bool isOverlay();
    void setLayerVendorUsageBits(vendor_usage_bits_t vendor);
    void acceptPendingCompositionType(bool resetChanges);
    bool isVideoOverlay();
    bool isAFBCFormat();
    void setIndex(int index) {mIndex = index;}
    int getIndex() { return mIndex;}
    const native_handle_t* getSideband() { return mSidebandStream.getValue();}
    hwc2_error_t getLayerVendorUsageBits(uint32_t* usage);
    bool isHwcursor();
    bool isClientTarget();
    void setLayerSkipRender(bool skip);
protected:
    template <typename T>
    class ChangedState {
        public:
            ChangedState(HWCLayer& parent, T initialValue)
              : mParent(parent),
                mValue(initialValue) {}

            bool setValue(T value,bool bNeedDirty = true) {
                if (value == mValue) {
                    return false;
                }
                if (bNeedDirty) {
                    mParent.incDirty();
                }
                mValue = value;
                return true;
            }

            T getValue() const { return mValue; }

        private:
            HWCLayer& mParent;
            T mValue;
    };

    class CompostionState {
        public:
            CompostionState(HWCDisplayDevice& display,HWCLayer& parent, hwc2_composition_t initialValue)
              : mDisplay(display),
                mParent(parent),
                mPendingValue(initialValue),
                mValue(initialValue) {}

            void setPending(hwc2_composition_t value) {
                if (value == mPendingValue) {
                    return;
                }
                mPendingValue = value;
            }

            hwc2_composition_t getValue() const { return mValue; }
            hwc2_composition_t getPendingValue() const { return mPendingValue; }

            bool isDirty() const { return mPendingValue != mValue; }

            void acceptPendingValue(bool resetChanges);

        private:
            HWCDisplayDevice& mDisplay;
            HWCLayer& mParent;
            hwc2_composition_t mPendingValue;
            hwc2_composition_t mValue;
    };
    static std::atomic<hwc2_display_t> sNextId;
    const hwc2_layer_t mId;
    HWCDisplayDevice& mDisplay;
    uint32_t mZ;
    int mIndex; //index in mSortLayersByZ
    int32_t mActivityDegree;
    std::atomic<int32_t> mDirtyCount;
    bool bFramebufferTarget;
    bool bSkipRenderToFrameBuffer;
    FencedBuffer mBuffer;
    std::vector<hwc_rect_t> mSurfaceDamage;

    ChangedState<hwc2_blend_mode_t> mBlendMode;
    ChangedState<hwc_color_t> mColor;
    CompostionState mCompositionType;
    ChangedState<hwc_rect_t> mDisplayFrame;
    ChangedState<float> mPlaneAlpha;
    ChangedState<const native_handle_t*> mSidebandStream;
    ChangedState<hwc_frect_t> mSourceCrop;
    ChangedState<hwc_transform_t> mTransform;
    ChangedState<std::vector<hwc_rect_t>> mVisibleRegion;

    int32_t mReleaseFence;
    std::string mName;
    uint32_t mVendorUsageBits;
    unsigned int mMemAvailable;
};

#endif /* HWCLAYER_H_ */
