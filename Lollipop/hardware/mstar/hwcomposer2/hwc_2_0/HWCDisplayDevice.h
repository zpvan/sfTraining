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

#ifndef HWCDISPLAYDEVICE_H_
#define HWCDISPLAYDEVICE_H_
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11
#include "HWComposerContext.h"
#include "HWComposerDevice.h"
#include "HWComposerOSD.h"
#include "HWCCursor.h"
#include "HWComposerDip.h"
#include "HWComposerTVVideo.h"
#include "HWComposerMMVideo.h"
#include "HWCLayer.h"
#include "FencedBuffer.h"
#include <inttypes.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <sync/sync.h>
#include <cutils/log.h>
#include <gralloc_priv.h>


class HWCLayer;
class HWComposerDevice;
class HWComposerOSD;
class HWComposerDip;
class HWComposerTVVideo;
class HWComposerMMVideo;
class HWCCursor;

#define RES_4K2K_WIDTH 3840
#define RES_4K2K_HEIGHT 2160

class HWCDisplayDevice
{
private:
    class Changes {
        public:
            uint32_t getNumTypes() const {
                return static_cast<uint32_t>(mTypeChanges.size());
            }

            uint32_t getNumLayerRequests() const {
                return static_cast<uint32_t>(mLayerRequests.size());
            }

            const std::unordered_map<hwc2_layer_t, hwc2_composition_t>&
                    getTypeChanges() const {
                return mTypeChanges;
            }

            const std::unordered_map<hwc2_layer_t, hwc2_layer_request_t>&
                    getLayerRequests() const {
                return mLayerRequests;
            }

            int32_t getDisplayRequests() const {
                int32_t requests = 0;
                for (auto request : mDisplayRequests) {
                    requests |= static_cast<int32_t>(request);
                }
                return requests;
            }

            void addTypeChange(hwc2_layer_t layerId,
                    hwc2_composition_t type) {
                //if key is the same,value is not replace,so we must erase and then insert
                if (mTypeChanges.count(layerId) != 0) {
                    mTypeChanges.erase(layerId);
                }
                mTypeChanges.insert({layerId, type});
            }

            void clearTypeChanges() { mTypeChanges.clear(); }

            void addLayerRequest(hwc2_layer_t layerId,
                    hwc2_layer_request_t request) {
                mLayerRequests.insert({layerId, request});
            }

            void removeLayerRequest(hwc2_layer_t layerId) {
                if (mLayerRequests.count(layerId) != 0) {
                    mLayerRequests.erase(layerId);
                }
            }

        private:
            std::unordered_map<hwc2_layer_t, hwc2_composition_t>
                    mTypeChanges;
            std::unordered_map<hwc2_layer_t, hwc2_layer_request_t>
                    mLayerRequests;
            std::unordered_set<hwc2_display_request_t> mDisplayRequests;
    };

public:
    HWCDisplayDevice(HWComposerDevice& device, hwc2_display_type_t type);
    ~HWCDisplayDevice();
    hwc2_display_t getId() const;
    std::map<hwc2_layer_t, std::shared_ptr<HWCLayer>>& getLayers(){ return mLayers;};

    void initConfigs(uint32_t width, uint32_t height);
    hwc2_error_t acceptChanges();
    hwc2_error_t createLayer(hwc2_layer_t* outLayerId);
    hwc2_error_t destroyLayer(hwc2_layer_t layerId);
    hwc2_error_t getActiveConfig(hwc2_config_t* outConfigId);
    hwc2_error_t getDisplayAttribute(hwc2_config_t configId,
            int32_t /*hwc2_attribute_t*/ inAttribute, int32_t* outValue);
    hwc2_error_t getChangedCompositionTypes(uint32_t* outNumElements,
            hwc2_layer_t* outLayers, int32_t* outTypes);
    hwc2_error_t getColorModes(uint32_t* outNumModes, int32_t* outModes);
    hwc2_error_t getDisplayConfigs(uint32_t* outNumConfigs,
            hwc2_config_t* outConfigIds);
    hwc2_error_t getDozeSupport(int32_t* outSupport);
    hwc2_error_t getHdrCapabilities(uint32_t* outNumTypes,
            int32_t* outTypes, float* outMaxLuminance,
            float* outMaxAverageLuminance, float* outMinLuminance);
    hwc2_error_t getDisplayName(uint32_t* outSize, char* outName);
    hwc2_error_t getReleaseFences(uint32_t* outNumElements,
            hwc2_layer_t* outLayers, int32_t* outFences);
    hwc2_error_t getDisplayRequests(int32_t* outDisplayRequests,
            uint32_t* outNumElements, hwc2_layer_t* outLayers,
            int32_t* outLayerRequests);
    hwc2_error_t getDisplayType(int32_t* outType);
    hwc2_error_t present(int32_t* outRetireFence);
    hwc2_error_t setActiveConfig(hwc2_config_t configId);
    hwc2_error_t setClientTarget(buffer_handle_t target,
            int32_t acquireFence, int32_t dataspace, hwc_region_t damage);
    hwc2_error_t setColorMode(int32_t mode);
    hwc2_error_t setColorTransform(const float* /*matrix*/ matrix,
            int32_t /*android_color_transform_t*/ hint);
    hwc2_error_t setOutputBuffer(buffer_handle_t buffer,
            int32_t releaseFence);
    hwc2_error_t setPowerMode(int32_t /*PowerMode*/ mode);
    hwc2_error_t setVsyncEnabled(int32_t /*Vsync*/ enabled);
    bool isVsyncEnabled();
    hwc2_error_t validate(uint32_t* outNumTypes,
            uint32_t* outNumRequests);
    hwc2_error_t updateLayerZ(hwc2_layer_t layerId, uint32_t z);
    hwc2_error_t setCursorPosition(hwc2_layer_t layerId, int32_t x, int32_t y);
    hwc2_error_t reDrawHwCursorFb(hwc2_layer_t layerId,
            int32_t srcX, int32_t srcY, int32_t srcWidth,int32_t srcHeight,
            int32_t stride, int32_t dstWidth, int32_t dstHeight);
    std::string dump() const;
    std::unique_ptr<Changes>& getChanges() { return mChanges;};
    float getDisplayScaleRatioW() { return displayFrameFTScaleRatio_w;}
    float getDisplayScaleRatioH() { return displayFrameFTScaleRatio_h;}
    uint32_t getDefaultGopNumber();
    uint32_t getGopDest();
    bool checkGopTimingChanged();
    bool checkGopSupportAFBC(int index);
    hwc2_error_t get3DTransform(hwc_3d_param_t* params, hwc_rect_t frame);
    hwc2_error_t getCurrentTiming(int32_t* width, int32_t* height);
    hwc2_error_t isStbTarget(bool* outValue);
    hwc2_error_t getOsdHeight(int32_t* outHeight);
    hwc2_error_t getOsdWidth(int32_t* outWidth);
    hwc2_error_t getMirrorMode(uint32_t* outMode);
    hwc2_error_t getRefreshPeriod(nsecs_t* outPeriod);
    hwc2_error_t captureScreen(uint32_t* outNumElements, hwc2_layer_t* outLayers,
        hwc_capture_data_t* outDatas, uintptr_t* addr, int* width, int* height, int withOsd);
    hwc2_error_t releaseCaptureScreenMemory();
    hwc2_error_t isRGBColorSpace(bool *outValue);
    hwc2_error_t isAFBCFramebuffer(bool *outValue);
    void mmVideoSizeChangedNotify();
    void mmVideoCheckDownScaleXC();
#ifdef ENABLE_GAMECURSOR
    hwc2_error_t gameCursorShow(int32_t devNo);
    hwc2_error_t gameCursorHide(int32_t devNo);
    hwc2_error_t gameCursorSetAlpha(int32_t devNo, float alpha);
    hwc2_error_t gameCursorSetPosition(int32_t devNo, int32_t positionX, int32_t positionY);
    hwc2_error_t gameCursorRedrawFb(int32_t devNo, int srcX, int srcY, int srcWidth, int srcHeight, int stride,int dstWidth, int dstHeight);
    hwc2_error_t gameCursorDoTransaction(int32_t devNo);
    hwc2_error_t gameCursorInit(int32_t devNo);
    hwc2_error_t gameCursorDeinit(int32_t devNo);
    hwc2_error_t gameCursorGetFbVaddrByDevNo(int32_t devNo, uintptr_t* vaddr);
#endif

    int32_t mVsyncPeriod;
    int mOsdWidth;
    int mOsdHeight;
    int32_t mWidth;
    int32_t mHeight;
    int32_t mXdpi;
    int32_t mYdpi;
    int mPanelWidth;
    int mPanelHeight;
    int mPanelHStart;
    int mUrsaVersion;
    int mPanelLinkType;
    int mPanelLinkExtType;
    int mOsdc_enable;
    int mPanelType;
    int mMirrorModeH;
    int mMirrorModeV;
    bool bStbTarget;
    // 1.if stbTarget is true, set bRGBColorSpace as false;
    //    if stbTarget is false, set bRGBColorSpace as true.
    // 2.if the property mstar.RGBColorSpace is set, set the
    //    bRGBColorSpace as mstar.ColorSpace.bRGB
    bool bRGBColorSpace;
    // some case ,the timing changed,we need to
    // record the current timing
    int mCurTimingWidth;
    int mCurTimingHeight;
    // sometimes only show part of the frambuffer,then
    // need the displayRegionWidth/displayRegionHeight
    // describe the region
    int mDisplayRegionWidth;
    int mDisplayRegionHeight;
    int mGopCount;
    unsigned int mGopDest;
    bool bAFBCNeedReset;
    bool bVsyncOff;
    bool b3DModeChanged;
    uint32_t mCurrent3DMode;
    BootLogoObj mBootlogo;
    GOP_LayerConfig stLayerSetting;
    bool bAFBCFramebuffer;
    bool bFramebufferNeedRedraw;
    uint32_t mFrameCount;
    uint32_t mRequiredGopOverlycount;
    bool mGameCursorChanged;
    bool mGameCursorEnabled;
private:
    class Config {
        public:
            Config(HWCDisplayDevice& display)
              : mDisplay(display),
                mAttributes() {}

            bool isOnDisplay(const HWCDisplayDevice& display) const {
                return display.getId() == mDisplay.getId();
            }

            void setAttribute(hwc2_attribute_t attribute, int32_t value);
            int32_t getAttribute(hwc2_attribute_t attribute) const;

            void setId(hwc2_config_t id) { mId = id; }
            hwc2_config_t getId() const { return mId; }

            // dumpsys SurfaceFlinger
            std::string toString() const;

        private:
            HWCDisplayDevice& mDisplay;
            hwc2_config_t mId;
            std::unordered_map<hwc2_attribute_t, int32_t> mAttributes;
    };

    void loadHardWareModules();
    bool isCursorAndGopLayer(const std::shared_ptr<HWCLayer>& inlayer);
    std::shared_ptr<const Config> getConfig(hwc2_config_t configId) const;
    bool layerSupportOverlay(std::shared_ptr<HWCLayer>& inlayer);
    void setCompositionTypeStageOne();
    void setCompositionTypeForAFBCLimitation(bool hasFB);
    bool checkClientTargetNeedRedraw();
    void setCompositionTypeStageTwo();
    static std::atomic<hwc2_display_t> sNextId;
    const hwc2_display_t mId;
    HWComposerDevice& mDevice;
    hwc2_display_type_t mType;
    std::set<int32_t> mColorModes;
    int32_t mActiveColorMode;
    std::string mName;
    hwc2_vsync_t mVsyncEnabled;
    hwc2_power_mode_t mPowerMode;
    FencedBuffer mOutputBuffer;
    std::map<hwc2_layer_t, std::shared_ptr<HWCLayer>> mLayers;
    std::vector<std::shared_ptr<HWCLayer>> mSortLayersByZ;
    std::vector<std::shared_ptr<HWCLayer>> mVideoLayers;
    std::vector<std::shared_ptr<HWCLayer>> mGopOverlayLayers;
    std::vector<std::shared_ptr<HWCLayer>> mHwCursors;
    std::vector<std::shared_ptr<Config>> mConfigs;
    std::shared_ptr<const Config> mActiveConfig;
    std::unordered_map<hwc2_layer_t, hwc2_composition_t> mLastCompositionType;
    // Will only be non-null after the layer has been validated but
    // before it has been presented
    std::unique_ptr<Changes> mChanges;
    std::shared_ptr<HWCLayer> mClientTarget;
    float displayFrameFTScaleRatio_w;
    float displayFrameFTScaleRatio_h;
    std::mutex mMutex;
    int32_t mClientLayerCount;
    std::unique_ptr<HWComposerOSD> mOsdModule;
    std::unique_ptr<HWComposerTVVideo> mTVVideoModule;
    std::unique_ptr<HWComposerMMVideo> mMMVideoModule;
    std::unique_ptr<HWComposerDip> mDipModule;
    std::unique_ptr<HWCCursor> mCursorModule;
};
#endif /* HWCDISPLAYDEVICE_H_ */
