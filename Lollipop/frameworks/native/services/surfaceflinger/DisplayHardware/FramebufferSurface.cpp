/*
 **
 ** Copyright 2012 The Android Open Source Project
 **
 ** Licensed under the Apache License Version 2.0(the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing software
 ** distributed under the License is distributed on an "AS IS" BASIS
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <cutils/log.h>

#include <utils/String8.h>

#include <ui/Rect.h>

#include <EGL/egl.h>

#include <hardware/hardware.h>
#include <gui/Surface.h>
#include <gui/GraphicBufferAlloc.h>
#include <ui/GraphicBuffer.h>

#include "FramebufferSurface.h"
#include "HWComposer.h"
#include <cutils/properties.h>
// MStar Android Patch Begin
#include "DisplayDevice.h"
// MStar Android Patch End

#ifndef NUM_FRAMEBUFFER_SURFACE_BUFFERS
#define NUM_FRAMEBUFFER_SURFACE_BUFFERS (2)
#endif

// ----------------------------------------------------------------------------
namespace android {
// ----------------------------------------------------------------------------

/*
 * This implements the (main) framebuffer management. This class is used
 * mostly by SurfaceFlinger, but also by command line GL application.
 *
 */

FramebufferSurface::FramebufferSurface(HWComposer& hwc, int disp,
        const sp<IGraphicBufferConsumer>& consumer) :
    ConsumerBase(consumer),
    mDisplayType(disp),
    mCurrentBufferSlot(-1),
    mCurrentBuffer(0),
    mHwc(hwc)
{
    mName = "FramebufferSurface";
    mConsumer->setConsumerName(mName);
    // MStar Android Patch Begin
#ifdef MALI_AFBC_GRALLOC
    mUsage = GRALLOC_USAGE_HW_FB |
                   GRALLOC_USAGE_HW_RENDER |
                   GRALLOC_USAGE_HW_COMPOSER |
                   GRALLOC_USAGE_AFBC_ON;
#else
    mUsage = GRALLOC_USAGE_HW_FB |
                   GRALLOC_USAGE_HW_RENDER |
                   GRALLOC_USAGE_HW_COMPOSER ;
#endif
    mConsumer->setConsumerUsageBits(mUsage);
    // MStar Android Patch End
    mConsumer->setDefaultBufferFormat(mHwc.getFormat(disp));
    mConsumer->setDefaultBufferSize(mHwc.getWidth(disp),  mHwc.getHeight(disp));
    // MStar Android Patch Begin
    //Next buffer is allocated ,then release current buffer,in original android flow.
    //and in owner platform, framebuffer size is fixed,so we change it to 2 buffers.
    if (isFrameBufferResizeOpen()) {
        mConsumer->setDefaultMaxBufferCount(2);
    } else {
        mConsumer->setDefaultMaxBufferCount(NUM_FRAMEBUFFER_SURFACE_BUFFERS);
    }
#ifdef MALI_AFBC_GRALLOC
        int mirrorMode = mHwc.getMirrorMode();
        //mali can't dispose h/v flip,so we just do oritation using opengl
        if ((mirrorMode & DisplayDevice::DISPLAY_MIRROR_H)
            && (mirrorMode & DisplayDevice::DISPLAY_MIRROR_V)) {
            mConsumer->setTransformHint(HAL_TRANSFORM_ROT_180);
        }
#endif
    mOsdWidth = mHwc.getOsdWidth(disp);
    mOsdHeight = mHwc.getOsdHeight(disp);
    // MStar Android Patch End
}

status_t FramebufferSurface::beginFrame(bool /*mustRecompose*/) {
    return NO_ERROR;
}

status_t FramebufferSurface::prepareFrame(CompositionType /*compositionType*/) {
    return NO_ERROR;
}

status_t FramebufferSurface::advanceFrame() {
    // Once we remove FB HAL support, we can call nextBuffer() from here
    // instead of using onFrameAvailable(). No real benefit, except it'll be
    // more like VirtualDisplaySurface.
    return NO_ERROR;
}

status_t FramebufferSurface::nextBuffer(sp<GraphicBuffer>& outBuffer, sp<Fence>& outFence) {
    Mutex::Autolock lock(mMutex);

    BufferQueue::BufferItem item;
    status_t err = acquireBufferLocked(&item, 0);
    if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
        outBuffer = mCurrentBuffer;
        return NO_ERROR;
    } else if (err != NO_ERROR) {
        ALOGE("error acquiring buffer: %s (%d)", strerror(-err), err);
        return err;
    }

    // If the BufferQueue has freed and reallocated a buffer in mCurrentSlot
    // then we may have acquired the slot we already own.  If we had released
    // our current buffer before we call acquireBuffer then that release call
    // would have returned STALE_BUFFER_SLOT, and we would have called
    // freeBufferLocked on that slot.  Because the buffer slot has already
    // been overwritten with the new buffer all we have to do is skip the
    // releaseBuffer call and we should be in the same state we'd be in if we
    // had released the old buffer first.
    if (mCurrentBufferSlot != BufferQueue::INVALID_BUFFER_SLOT &&
        item.mBuf != mCurrentBufferSlot) {
        // Release the previous buffer.
        err = releaseBufferLocked(mCurrentBufferSlot, mCurrentBuffer,
                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
        if (err < NO_ERROR) {
            ALOGE("error releasing buffer: %s (%d)", strerror(-err), err);
            return err;
        }
    }
    mCurrentBufferSlot = item.mBuf;
    mCurrentBuffer = mSlots[mCurrentBufferSlot].mGraphicBuffer;
    outFence = item.mFence;
    outBuffer = mCurrentBuffer;
    return NO_ERROR;
}

// Overrides ConsumerBase::onFrameAvailable(), does not call base class impl.
void FramebufferSurface::onFrameAvailable(const BufferItem& /* item */) {
    sp<GraphicBuffer> buf;
    sp<Fence> acquireFence;
    status_t err = nextBuffer(buf, acquireFence);
    if (err != NO_ERROR) {
        ALOGE("error latching nnext FramebufferSurface buffer: %s (%d)",
                strerror(-err), err);
        return;
    }
    // MStar Android Patch Begin
#ifdef DEBUG_DUMP_SURFACEBUFFER
    char property[PROPERTY_VALUE_MAX] = {0};
    property_get("mstar.dumpframe", property, "false");
    if (strcmp(property, "true") == 0) {
        graphic_buffer_dump_helper::enable_dump_graphic_buffer(true, "/data/dumplayer");
        graphic_buffer_dump_helper::set_prefix("FrameBuffer_");
        graphic_buffer_dump_helper::dump_graphic_buffer_if_needed(mCurrentBuffer);
        graphic_buffer_dump_helper::set_prefix( "unknow_");
        graphic_buffer_dump_helper::enable_dump_graphic_buffer(false, NULL);
    }
#endif
    // MStar Android Patch End

    err = mHwc.fbPost(mDisplayType, acquireFence, buf);
    if (err != NO_ERROR) {
        ALOGE("error posting framebuffer: %d", err);
    }
}

void FramebufferSurface::freeBufferLocked(int slotIndex) {
    ConsumerBase::freeBufferLocked(slotIndex);
    if (slotIndex == mCurrentBufferSlot) {
        mCurrentBufferSlot = BufferQueue::INVALID_BUFFER_SLOT;
    }
}

void FramebufferSurface::onFrameCommitted() {
    sp<Fence> fence = mHwc.getAndResetReleaseFence(mDisplayType);
    if (fence->isValid() &&
            mCurrentBufferSlot != BufferQueue::INVALID_BUFFER_SLOT) {
        status_t err = addReleaseFence(mCurrentBufferSlot,
                mCurrentBuffer, fence);
        ALOGE_IF(err, "setReleaseFenceFd: failed to add the fence: %s (%d)",
                strerror(-err), err);
    }
}

status_t FramebufferSurface::compositionComplete()
{
    return mHwc.fbCompositionComplete();
}

// Since DisplaySurface and ConsumerBase both have a method with this
// signature, results will vary based on the static pointer type the caller is
// using:
//   void dump(FrameBufferSurface* fbs, String8& s) {
//       // calls FramebufferSurface::dump()
//       fbs->dump(s);
//
//       // calls ConsumerBase::dump() since it is non-virtual
//       static_cast<ConsumerBase*>(fbs)->dump(s);
//
//       // calls FramebufferSurface::dump() since it is virtual
//       static_cast<DisplaySurface*>(fbs)->dump(s);
//   }
// To make sure that all of these end up doing the same thing, we just redirect
// to ConsumerBase::dump() here. It will take the internal lock, and then call
// virtual dumpLocked(), which is where the real work happens.
void FramebufferSurface::dump(String8& result) const {
    ConsumerBase::dump(result);
}

void FramebufferSurface::dumpLocked(String8& result, const char* prefix) const
{
    mHwc.fbDump(result);
    ConsumerBase::dumpLocked(result, prefix);
}

// MStar Android Patch Begin
int FramebufferSurface::getOsdHeight() const {
    return mOsdHeight;
}

int FramebufferSurface::getOsdWidth() const {
    return mOsdWidth;
}

void FramebufferSurface::recalc3DTransform(Transform** first, Transform** second, Rect& frame) {
    mHwc.recalc3DTransform(first,second,frame);
}
void FramebufferSurface::getCurTiming(int *width, int *height) {
    mHwc.getCurOPTiming(width,height);
}

void FramebufferSurface::resizeBuffers(const uint32_t w, const uint32_t h) {
        mConsumer->setDefaultBufferSize(w, h);
}

void FramebufferSurface::changeFBtoAFBC(bool afbc) {
    mUsage &= ~GRALLOC_USAGE_AFBC_MASK;
    if (afbc) {
        mUsage |= GRALLOC_USAGE_AFBC_ON;
    } else {
        mUsage |= GRALLOC_USAGE_AFBC_OFF;
    }
    mConsumer->setConsumerUsageBits(mUsage);
}

int FramebufferSurface::isFrameBufferResizeOpen() {
    return mHwc.isFrameBufferResizeOpen();
}

int FramebufferSurface::isStbTarget() {
    return mHwc.isStbTarget();
}
// MStar Android Patch End

// ----------------------------------------------------------------------------
}; // namespace android
// ----------------------------------------------------------------------------
