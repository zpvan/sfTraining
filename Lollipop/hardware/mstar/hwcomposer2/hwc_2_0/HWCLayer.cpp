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
#define LOG_TAG "HWCLayer"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <utils/String8.h>

#include "HWCLayer.h"
#include <cutils/log.h>
#define __CLASS__ "HWCLayer"

static bool operator==(const hwc_color_t& lhs, const hwc_color_t& rhs) {
    return lhs.r == rhs.r &&
            lhs.g == rhs.g &&
            lhs.b == rhs.b &&
            lhs.a == rhs.a;
}

static bool operator==(const hwc_rect_t& lhs, const hwc_rect_t& rhs) {
    return lhs.left == rhs.left &&
            lhs.top == rhs.top &&
            lhs.right == rhs.right &&
            lhs.bottom == rhs.bottom;
}

static bool operator==(const hwc_frect_t& lhs, const hwc_frect_t& rhs) {
    return lhs.left == rhs.left &&
            lhs.top == rhs.top &&
            lhs.right == rhs.right &&
            lhs.bottom == rhs.bottom;
}

/*template <typename T>
static inline bool operator!=(const T& lhs, const T& rhs)
{
    return !(lhs == rhs);
}*/

static std::string rectString(hwc_rect_t rect)
{
    std::stringstream output;
    output << "[" << rect.left << ", " << rect.top << ", ";
    output << rect.right << ", " << rect.bottom << "]";
    return output.str();
}

static std::string approximateFloatString(float f)
{
    if (static_cast<int32_t>(f) == f) {
        return std::to_string(static_cast<int32_t>(f));
    }
    int32_t truncated = static_cast<int32_t>(f * 10);
    bool approximate = (static_cast<float>(truncated) != f * 10);
    const size_t BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE] = {};
    auto bytesWritten = snprintf(buffer, BUFFER_SIZE,
            "%s%.1f", approximate ? "~" : "", f);
    return std::string(buffer, bytesWritten);
}

static std::string frectString(hwc_frect_t frect)
{
    std::stringstream output;
    output << "[" << approximateFloatString(frect.left) << ", ";
    output << approximateFloatString(frect.top) << ", ";
    output << approximateFloatString(frect.right) << ", ";
    output << approximateFloatString(frect.bottom) << "]";
    return output.str();
}

static std::string colorString(hwc_color_t color)
{
    std::stringstream output;
    output << "RGBA [";
    output << static_cast<int32_t>(color.r) << ", ";
    output << static_cast<int32_t>(color.g) << ", ";
    output << static_cast<int32_t>(color.b) << ", ";
    output << static_cast<int32_t>(color.a) << "]";
    return output.str();
}

static std::string alphaString(float f)
{
    const size_t BUFFER_SIZE = 8;
    char buffer[BUFFER_SIZE] = {};
    auto bytesWritten = snprintf(buffer, BUFFER_SIZE, "%.3f", f);
    return std::string(buffer, bytesWritten);
}

static std::string regionStrings(const std::vector<hwc_rect_t>& visibleRegion,
        const std::vector<hwc_rect_t>& surfaceDamage)
{
    std::string regions;
    regions += "        Visible Region";
    regions.resize(40, ' ');
    regions += "Surface Damage\n";

    size_t numPrinted = 0;
    size_t maxSize = std::max(visibleRegion.size(), surfaceDamage.size());
    while (numPrinted < maxSize) {
        std::string line("        ");
        if (visibleRegion.empty() && numPrinted == 0) {
            line += "None";
        } else if (numPrinted < visibleRegion.size()) {
            line += rectString(visibleRegion[numPrinted]);
        }
        line.resize(40, ' ');
        if (surfaceDamage.empty() && numPrinted == 0) {
            line += "None";
        } else if (numPrinted < surfaceDamage.size()) {
            line += rectString(surfaceDamage[numPrinted]);
        }
        line += '\n';
        regions += line;
        ++numPrinted;
    }
    return regions;
}

static std::string formatString(int32_t format)
{
    std::stringstream output;
    switch (format) {
        case HAL_PIXEL_FORMAT_RGBA_8888 :
            output << "RGBA_8888";
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            output << "RGBx_8888";
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            output << "RGB_888";
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            output << "RGB_565";
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            output << "BGRA_8888";
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            output << "ImplDef";
            break;
        default:
            android::String8 result;
            result.appendFormat("? %08x", format);
            output << result;
            break;
    }
    return output.str();
}

std::atomic<hwc2_display_t> HWCLayer::sNextId(0);
HWCLayer::HWCLayer(HWCDisplayDevice& display)
    :mId(sNextId++),
    mDisplay(display),
    mZ(0),
    mIndex(-1),
    mActivityDegree(0),
    mDirtyCount(0),
    bFramebufferTarget(false),
    bSkipRenderToFrameBuffer(false),
    mBuffer(),
    mSurfaceDamage(),
    mBlendMode(*this, HWC2_BLEND_MODE_NONE),
    mColor(*this, {0, 0, 0, 0}),
    mCompositionType(display,*this, HWC2_COMPOSITION_INVALID),
    mDisplayFrame(*this, {0, 0, -1, -1}),
    mPlaneAlpha(*this, 0.0f),
    mSidebandStream(*this, nullptr),
    mSourceCrop(*this, {0.0f, 0.0f, -1.0f, -1.0f}),
    mTransform(*this, static_cast<hwc_transform_t>(0)),
    mVisibleRegion(*this, std::vector<hwc_rect_t>()),
    mReleaseFence(-1),
    mName("unknow"),
    mVendorUsageBits(0),
    mMemAvailable(mem_type_mask)
{
}
HWCLayer::~HWCLayer(){
}
void HWCLayer::incDegree() {
    if (mActivityDegree < MAX_ACTIVE_DEGREE) {
        mActivityDegree++;
    }
}
void HWCLayer::decDegree() {
    if (mActivityDegree > MIN_ACTIVE_DEGREE) {
        --mActivityDegree;
    }
}

// HWC2 Layer functions
hwc2_error_t HWCLayer::setLayerBuffer(buffer_handle_t buffer, int32_t acquireFence) {
    DLOGD_IF(HWC2_DEBUGTYPE_FENCE,"Setting buffer %p acquireFence to %d for layer %" PRIu64",[%s]", buffer,
        acquireFence, mId,mName.c_str());
    if (mBuffer.setBuffer(buffer)) {
        incDirty();
        incDegree();
    } else {
        decDegree();
    }
    mBuffer.setFence(acquireFence);
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCLayer::setCursorPosition(int32_t x, int32_t y) {
    if (mCompositionType.getValue() != HWC2_COMPOSITION_CURSOR) {
        return HWC2_ERROR_BAD_LAYER;
    }
    //
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCLayer::setLayerSurfaceDamage(hwc_region_t damage) {
    mSurfaceDamage.resize(damage.numRects);
    std::copy_n(damage.rects, damage.numRects, mSurfaceDamage.begin());
    return HWC2_ERROR_NONE;
}

// HWC2 Layer state functions
hwc2_error_t HWCLayer::setLayerBlendMode(int32_t /*BlendMode*/ inmode) {
    auto mode = static_cast<hwc2_blend_mode_t>(inmode);
    mBlendMode.setValue(mode);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerColor(hwc_color_t color) {
    mColor.setValue(color);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerCompositionType(int32_t /*Composition*/ intype) {
    if (intype == HWC2_COMPOSITION_CLIENT_TARGET) {
        bFramebufferTarget = true;
        return HWC2_ERROR_NONE;
    }
    auto type = static_cast<hwc2_composition_t>(intype);
    mCompositionType.setPending(type);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerDataspace(int32_t /*android_dataspace_t*/ indataspace) {
    auto dataspace = static_cast<android_dataspace_t>(indataspace);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerDisplayFrame(hwc_rect_t frame) {
    if (mCompositionType.getValue() == HWC2_COMPOSITION_CURSOR) {
        mDisplayFrame.setValue(frame,false);
    } else {
        mDisplayFrame.setValue(frame);
    }
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerPlaneAlpha(float alpha) {
    mPlaneAlpha.setValue(alpha);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerSidebandStream(const native_handle_t* stream) {
    mSidebandStream.setValue(stream);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerSourceCrop(hwc_frect_t crop) {
    mSourceCrop.setValue(crop);
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerTransform(int32_t /*hwc_transform_t*/ intransform) {
    auto transform = static_cast<hwc_transform_t>(intransform);
    mTransform.setValue(transform);
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCLayer::setLayerVisibleRegion(hwc_region_t rawVisible) {

    std::vector<hwc_rect_t> visible(rawVisible.rects,
            rawVisible.rects + rawVisible.numRects);
    if (mCompositionType.getValue() == HWC2_COMPOSITION_CURSOR) {
        mVisibleRegion.setValue(std::move(visible),false);
    } else {
        mVisibleRegion.setValue(std::move(visible));
    }
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerZ(uint32_t z) {
    mZ = z;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerReleaseFence(int32_t fenceFd) {
    /*if (mReleaseFence >= 0) {
        close(mReleaseFence);
    }*/
    mReleaseFence = fenceFd;
    return HWC2_ERROR_NONE;
}
hwc2_error_t HWCLayer::setLayerName(std::string name) {
    mName = name;
    return HWC2_ERROR_NONE;
}

hwc2_error_t HWCLayer::getLayerVendorUsageBits(uint32_t* usage) {
    *usage = mVendorUsageBits;
    return HWC2_ERROR_NONE;
}

void HWCLayer::setLayerSkipRender(bool skip) {
    bSkipRenderToFrameBuffer = skip;
}

bool HWCLayer::isClientTarget() {
    return bFramebufferTarget;
}

bool HWCLayer::isHwcursor() {
    char property[PROPERTY_VALUE_MAX] = {0};
    if (mCompositionType.getValue() !=  HWC2_COMPOSITION_CURSOR) {
        return false;
    }

    private_handle_t *handle = (private_handle_t *)mBuffer.getBuffer();
    if (!handle) {
        DLOGW("layer %" PRIu64": handle is NULL", mId);
        return false;
    }

    // format
    if ( handle->format != HAL_PIXEL_FORMAT_RGBA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_BGRA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_RGBX_8888 &&
         handle->format != HAL_PIXEL_FORMAT_RGB_565 ) {
        DLOGV("\tlayer %" PRIu64": format not supported", mId);
        return false;
    }
    // transform
    if (mTransform.getValue() != 0) {
        DLOGV("layer %" PRIu64": has rotation", mId);
        return false;
    }

    unsigned int dst_width = mDisplayFrame.getValue().right - mDisplayFrame.getValue().left;
    unsigned int dst_height = mDisplayFrame.getValue().bottom - mDisplayFrame.getValue().top;
    if ((dst_width > 128) || (dst_height > 128)) {
        DLOGV("layer dispframe w=%d, h=%d", dst_width, dst_height);
        return false;
    }

    if (property_get("mstar.desk-enable-gamecursor", property, "0") > 0) {
        if (1 == atoi(property)) {
            return false;
        }
    }

    if (property_get("mstar.desk-enable-hwcursor", property, "0") > 0) {
        if (0 == atoi(property)) {
            return false;
        }
    }

    //check STR suspend
    if (property_get("mstar.str.suspending", property, "0") > 0) {
        if (atoi(property) != 0) {
            return false;
        }
    }

    if (property_get("mstar.videoadvert.finished", property, "1") > 0) {
         if (0 == atoi(property)) {
            return false;
        }
    }
    // check 3D mode
    if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
        if (DISPLAYMODE_NORMAL_FP == atoi(property)) {
            return false;
        }
    }
    return true;
}

bool HWCLayer::canChangetoOverlay() {
    private_handle_t *handle = (private_handle_t *)mBuffer.getBuffer();
    if (!handle) {
        DLOGV("layer %" PRIu64": handle is NULL,name [%s]", mId,mName.c_str());
        return false;
    }
    if (((handle->memTypeInfo&mem_type_allocated_mask)>>16)!=(handle->memTypeInfo&mem_type_request_mask)) {
        // the request memtype is not available
        mMemAvailable &= ~(handle->memTypeInfo&mem_type_request_mask);
    }

    char property[PROPERTY_VALUE_MAX] = {0};
    if (property_get(MSTAR_DESK_DISPLAY_MODE, property, "0") > 0) {
        if (DISPLAYMODE_NORMAL != atoi(property)) {
            return false;
        }
    }
    // scale
    unsigned int src_width = (unsigned int)mSourceCrop.getValue().right - (unsigned int)mSourceCrop.getValue().left;
    unsigned int src_height = (unsigned int)mSourceCrop.getValue().bottom - (unsigned int)mSourceCrop.getValue().top;
    unsigned int dst_width = mDisplayFrame.getValue().right - mDisplayFrame.getValue().left;
    unsigned int dst_height = mDisplayFrame.getValue().bottom - mDisplayFrame.getValue().top;
    // if need to do down scale ,not support
    // if sourceCropf.left is not 2 pixel alignment, not support
    if (src_width > dst_width || src_height > dst_height || (unsigned int)mSourceCrop.getValue().left&0x1) {
        DLOGV("\tlayer %" PRIu64": upscale or 2 pxiel alignment not supported", mId);
        return false;
    }

    // format
    if ( handle->format != HAL_PIXEL_FORMAT_RGBA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_BGRA_8888 &&
         handle->format != HAL_PIXEL_FORMAT_RGBX_8888
#ifndef MALI_AFBC_GRALLOC
        &&handle->format != HAL_PIXEL_FORMAT_RGB_565
#endif
        ) {
        DLOGV("\tlayer %" PRIu64": format not supported", mId);
        return false;
    }
    // transform
    if (mTransform.getValue() != 0) {
        DLOGV("layer %" PRIu64": has rotation", mId);
        return false;
    }
    return true;
}

void HWCLayer::requestChangeContiguousMemory() {
    if (mActivityDegree == MAX_ACTIVE_DEGREE) {
        mDisplay.mRequiredGopOverlycount ++;
        setLayerVendorUsageBits(VENDOR_CMA_CONTIGUOUS_MEMORY);
    }
}
void HWCLayer::requestChangeDiscreteMemory() {
    if (mActivityDegree == MIN_ACTIVE_DEGREE) {
        setLayerVendorUsageBits(VENDOR_CMA_DISCRETE_MEMORY);
    }
}

void HWCLayer::setLayerVendorUsageBits(vendor_usage_bits_t vendor) {
    private_handle_t *handle = (private_handle_t *)mBuffer.getBuffer();
    if (handle) {
        char  property[PROPERTY_VALUE_MAX] = {0};
        int miu_location = -1;
        switch (vendor) {
            case VENDOR_CMA_CONTIGUOUS_MEMORY :
                {
                    if (property_get("mstar.maliion.conmem.location", property, NULL) > 0) {
                        miu_location = atoi(property);
                        if ((miu_location==2)&&(mMemAvailable&mem_type_cma_contiguous_miu2)) {
                            mVendorUsageBits |= (GRALLOC_USAGE_ION_CMA_MIU2|GRALLOC_USAGE_ION_CMA_CONTIGUOUS);
                        } else if ((miu_location==1)&&(mMemAvailable&mem_type_cma_contiguous_miu1)) {
                            mVendorUsageBits |= (GRALLOC_USAGE_ION_CMA_MIU1|GRALLOC_USAGE_ION_CMA_CONTIGUOUS);
                        } else if ((miu_location==0)&&(mMemAvailable&mem_type_cma_contiguous_miu0)) {
                            mVendorUsageBits |= (GRALLOC_USAGE_ION_CMA_MIU0|GRALLOC_USAGE_ION_CMA_CONTIGUOUS);
                        } else {
                            DLOGD("the mstar.maliion.conmem.location is in miu%d, but no available memory",miu_location);
                        }
#ifdef MALI_AFBC_GRALLOC
                        if (!mDisplay.checkGopSupportAFBC(0)) {
                            mVendorUsageBits |= GRALLOC_USAGE_AFBC_OFF;
                        }
#endif
                    }
                }
            case VENDOR_CMA_DISCRETE_MEMORY:
                {
                    if (property_get("mstar.maliion.dismem.location", property, NULL) > 0) {
                        miu_location = atoi(property);
                        if ((miu_location==2)&&(mMemAvailable&mem_type_cma_discrete_miu2)) {
                            mVendorUsageBits |= GRALLOC_USAGE_ION_CMA_MIU2|GRALLOC_USAGE_ION_CMA_DISCRETE;
                        } else if ((miu_location==1)&&(mMemAvailable&mem_type_cma_discrete_miu1)) {
                            mVendorUsageBits |= GRALLOC_USAGE_ION_CMA_MIU1|GRALLOC_USAGE_ION_CMA_DISCRETE;
                        } else if ((miu_location==0)&&(mMemAvailable&mem_type_cma_discrete_miu0)) {
                            mVendorUsageBits |= GRALLOC_USAGE_ION_CMA_MIU0|GRALLOC_USAGE_ION_CMA_DISCRETE;
                        } else {
                            DLOGD("the mstar.maliion.dismem.location is in %d, but no available memory",miu_location);
                        }
                    }

                }
            default :
                {
                    mVendorUsageBits |= GRALLOC_USAGE_ION_SYSTEM_HEAP;
                    break;
                }
       }
    }
}

bool HWCLayer::isAFBCFormat(){
    private_handle_t *handle = (private_handle_t *)mBuffer.getBuffer();
    return (handle != nullptr)&&
        (handle->internal_format & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK));
}

bool HWCLayer::isVideoOverlay() {
    if (mCompositionType.getValue() == HWC2_COMPOSITION_SIDEBAND) {
        return true;
    }
    private_handle_t *handle = (private_handle_t *)mBuffer.getBuffer();
    if (handle&&(handle->mstaroverlay & private_handle_t::PRIV_FLAGS_MM_OVERLAY ||
        handle->mstaroverlay & private_handle_t::PRIV_FLAGS_TV_OVERLAY ||
        handle->mstaroverlay & private_handle_t::PRIV_FLAGS_NW_OVERLAY )) {
        return true;
    }
    return false;
}

bool HWCLayer::isOverlay() {
    private_handle_t *handle = (private_handle_t *)mBuffer.getBuffer();
    if (handle == nullptr) {
        return false;
    }
    // gralloc usage, currently only support contiguous memory
    unsigned int mem_type_cma_contiguous_mask = mem_type_cma_contiguous_miu0|mem_type_cma_contiguous_miu1|mem_type_cma_contiguous_miu2;
    unsigned int allocated_mem_type = (handle->memTypeInfo&mem_type_allocated_mask)>>16;
    if (!(allocated_mem_type&mem_type_cma_contiguous_mask)) {
        DLOGV("\tlayer %" PRIu64": memory not supported", mId);
        return false;
    }
    if (Debug::IsGopOverlayDisabled()) {
        // force to render to the FramebufferTarget
        return false;
    }
    return true;
}

std::string HWCLayer::dump() const {
    std::stringstream output;
    const char* fill = "    ";
    unsigned int memtype = 0;
    output <<  "HWC Layer :" << mId << " (" << mName << ")\n";
    output << regionStrings(mVisibleRegion.getValue(), mSurfaceDamage);
    if (bFramebufferTarget) {
        output << fill << "Compostion Type : "<< "Client Target";
    } else if (bSkipRenderToFrameBuffer &&
            (mCompositionType.getValue() == HWC2_COMPOSITION_DEVICE)) {
        output << fill << "Compostion Type : " << "Skip Client";
    } else {
        output << fill << "Compostion Type : " << getCompositionName(mCompositionType.getValue());
    }
    output << fill << "Z = "<< mZ;

    if (mCompositionType.getValue() == HWC2_COMPOSITION_SOLID_COLOR) {
        output << "  Color = " << colorString(mColor.getValue()) << '\n';
    } else if (mCompositionType.getValue() == HWC2_COMPOSITION_SIDEBAND) {
        output << " Handle: " << mSidebandStream.getValue() << '\n';
    } else {
        output << " Buffer handle/Fence = " << mBuffer.getBuffer() << "/" <<
            mBuffer.getFence() << fill;
        buffer_handle_t hnd = mBuffer.getBuffer();
        if (hnd == nullptr) {
            output << " Foramt = " << " unknow" << fill;
        } else {
            private_handle_t *handle = (private_handle_t *)hnd;
            output << " Foramt = " << formatString(handle->format) << fill;
            memtype = handle->memTypeInfo;
        }

        output << " Transform = " << getTransformName(mTransform.getValue()) << fill;
        output << " Blend = " << getBlendModeName(mBlendMode.getValue()) << "\n";
        output << fill << " Source crop (l,t,r,b) = " << frectString(mSourceCrop.getValue()) << " ";
        output << " Display frame (l,t,r,b) = " << rectString(mDisplayFrame.getValue()) << '\n';
    }
    if (!bFramebufferTarget) {
        output << fill << "layer buffer's memory type is " << memType2String((memtype&mem_type_allocated_mask)>>16) <<'\n';
    }
    output << "----------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    return output.str();
}

void HWCLayer::acceptPendingCompositionType(bool resetChanges) {
    mCompositionType.acceptPendingValue(resetChanges);
}

void HWCLayer::CompostionState::acceptPendingValue(bool resetChanges) {
    if (isDirty()) {
        mValue = mPendingValue;
        if (resetChanges) {
            mDisplay.getChanges()->addTypeChange(mParent.getId(),mValue);
        }
    }
}
