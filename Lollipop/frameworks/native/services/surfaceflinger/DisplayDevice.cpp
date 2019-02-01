/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <cutils/properties.h>

#include <utils/RefBase.h>
#include <utils/Log.h>

#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>

#include <gui/Surface.h>

#include <hardware/gralloc.h>

#include "DisplayHardware/DisplaySurface.h"
#include "DisplayHardware/HWComposer.h"
#include "RenderEngine/RenderEngine.h"

#include "clz.h"
#include "DisplayDevice.h"
#include "SurfaceFlinger.h"
#include "Layer.h"

// ----------------------------------------------------------------------------
using namespace android;
// ----------------------------------------------------------------------------

/*
 * Initialize the display to the specified values.
 *
 */

DisplayDevice::DisplayDevice(
        const sp<SurfaceFlinger>& flinger,
        DisplayType type,
        int32_t hwcId,
        int format,
        bool isSecure,
        const wp<IBinder>& displayToken,
        const sp<DisplaySurface>& displaySurface,
        const sp<IGraphicBufferProducer>& producer,
        EGLConfig config)
    : lastCompositionHadVisibleLayers(false),
      mFlinger(flinger),
      mType(type), mHwcDisplayId(hwcId),
      mDisplayToken(displayToken),
      mDisplaySurface(displaySurface),
      mDisplay(EGL_NO_DISPLAY),
      mSurface(EGL_NO_SURFACE),
      mDisplayWidth(), mDisplayHeight(), mFormat(),
      mFlags(),
      mPageFlipCount(),
      mIsSecure(isSecure),
      mSecureLayerVisible(false),
      mLayerStack(NO_LAYER_STACK),
      mOrientation(),
      mPowerMode(HWC_POWER_MODE_OFF),
      mActiveConfig(0),
      // MStar Android Patch Begin
      mOverscanAnd3DEnabled(false),
      mLeftOverscan(0),
      mTopOverscan(0),
      mRightOverscan(0),
      mBottomOverscan(0),
      mLeftGlobalTransform(0),
      mRightGlobalTransform(0),
      skipSwapBuffer(false)
      // MStar Android Patch End
{
    mNativeWindow = new Surface(producer, false);
    ANativeWindow* const window = mNativeWindow.get();

    /*
     * Create our display's surface
     */

    EGLSurface surface;
    EGLint w, h;
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (config == EGL_NO_CONFIG) {
        config = RenderEngine::chooseEglConfig(display, format);
    }
    surface = eglCreateWindowSurface(display, config, window, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH,  &mDisplayWidth);
    eglQuerySurface(display, surface, EGL_HEIGHT, &mDisplayHeight);

    // Make sure that composition can never be stalled by a virtual display
    // consumer that isn't processing buffers fast enough. We have to do this
    // in two places:
    // * Here, in case the display is composed entirely by HWC.
    // * In makeCurrent(), using eglSwapInterval. Some EGL drivers set the
    //   window's swap interval in eglMakeCurrent, so they'll override the
    //   interval we set here.
    if (mType >= DisplayDevice::DISPLAY_VIRTUAL)
        window->setSwapInterval(window, 0);

    mConfig = config;
    mDisplay = display;
    mSurface = surface;
    mFormat  = format;
    mPageFlipCount = 0;
    mViewport.makeInvalid();
    mFrame.makeInvalid();
    // MStar Android Patch Begin
    if (mDisplaySurface == NULL) {
        ALOGE("DisplayDevice::DisplayDevice mDisplaySurface == NULL can not get mOSDWidth/mOSDHeight");
        mOsdWidth = mDisplayWidth;
        mOsdHeight = mDisplayHeight;
    } else {
        mOsdWidth = mDisplaySurface->getOsdWidth();
        mOsdHeight = mDisplaySurface->getOsdHeight();
    }
    osdWidthForAndroid = mDisplayWidth;
    osdHeightForAndroid = mDisplayHeight;
    // MStar Android Patch End

    // virtual displays are always considered enabled
    mPowerMode = (mType >= DisplayDevice::DISPLAY_VIRTUAL) ?
                  HWC_POWER_MODE_NORMAL : HWC_POWER_MODE_OFF;

    // Name the display.  The name will be replaced shortly if the display
    // was created with createDisplay().
    switch (mType) {
        case DISPLAY_PRIMARY:
            mDisplayName = "Built-in Screen";
            break;
        case DISPLAY_EXTERNAL:
            mDisplayName = "HDMI Screen";
            break;
        default:
            mDisplayName = "Virtual Screen";    // e.g. Overlay #n
            break;
    }

    // initialize the display orientation transform.
    setProjection(DisplayState::eOrientationDefault, mViewport, mFrame);
}

// MStar Android Patch Begin
void DisplayDevice::changeGlobalTransform(Rect& newFrame) {
    Rect frame(newFrame);
    setProjectionPlatform(mOrientation, mViewport, frame);
}
// MStar Android Patch End

DisplayDevice::~DisplayDevice() {
    if (mSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplay, mSurface);
        mSurface = EGL_NO_SURFACE;
    }
    // MStar Android Patch Begin
    if (mLeftGlobalTransform) {
        delete mLeftGlobalTransform;
        mLeftGlobalTransform = NULL;
    }
    if (mRightGlobalTransform) {
        delete mRightGlobalTransform;
        mRightGlobalTransform = NULL;
    }
    // MStar Android Patch End
}

void DisplayDevice::disconnect(HWComposer& hwc) {
    if (mHwcDisplayId >= 0) {
        hwc.disconnectDisplay(mHwcDisplayId);
        if (mHwcDisplayId >= DISPLAY_VIRTUAL)
            hwc.freeDisplayId(mHwcDisplayId);
        mHwcDisplayId = -1;
    }
}

bool DisplayDevice::isValid() const {
    return mFlinger != NULL;
}

int DisplayDevice::getWidth() const {
    return mDisplayWidth;
}

int DisplayDevice::getHeight() const {
    return mDisplayHeight;
}

// MStar Android Patch Begin
int DisplayDevice::getOsdWidth() const {
    return mOsdWidth;
}

int DisplayDevice::getOsdHeight() const {
    return mOsdHeight;
}
// MStar Android Patch End

PixelFormat DisplayDevice::getFormat() const {
    return mFormat;
}

EGLSurface DisplayDevice::getEGLSurface() const {
    return mSurface;
}

void DisplayDevice::setDisplayName(const String8& displayName) {
    if (!displayName.isEmpty()) {
        // never override the name with an empty name
        mDisplayName = displayName;
    }
}

uint32_t DisplayDevice::getPageFlipCount() const {
    return mPageFlipCount;
}

status_t DisplayDevice::compositionComplete() const {
    return mDisplaySurface->compositionComplete();
}

void DisplayDevice::flip(const Region& dirty) const
{
    mFlinger->getRenderEngine().checkErrors();

    EGLDisplay dpy = mDisplay;
    EGLSurface surface = mSurface;

#ifdef EGL_ANDROID_swap_rectangle
    if (mFlags & SWAP_RECTANGLE) {
        const Region newDirty(dirty.intersect(bounds()));
        const Rect b(newDirty.getBounds());
        eglSetSwapRectangleANDROID(dpy, surface,
                b.left, b.top, b.width(), b.height());
    }
#else
    (void) dirty; // Eliminate unused parameter warning
#endif

    mPageFlipCount++;
}

status_t DisplayDevice::beginFrame(bool mustRecompose) const {
    return mDisplaySurface->beginFrame(mustRecompose);
}

status_t DisplayDevice::prepareFrame(const HWComposer& hwc) const {
    DisplaySurface::CompositionType compositionType;
    bool haveGles = hwc.hasGlesComposition(mHwcDisplayId);
    bool haveHwc = hwc.hasHwcComposition(mHwcDisplayId);
    if (haveGles && haveHwc) {
        compositionType = DisplaySurface::COMPOSITION_MIXED;
    } else if (haveGles) {
        compositionType = DisplaySurface::COMPOSITION_GLES;
    } else if (haveHwc) {
        compositionType = DisplaySurface::COMPOSITION_HWC;
    } else {
        // Nothing to do -- when turning the screen off we get a frame like
        // this. Call it a HWC frame since we won't be doing any GLES work but
        // will do a prepare/set cycle.
        compositionType = DisplaySurface::COMPOSITION_HWC;
    }
    return mDisplaySurface->prepareFrame(compositionType);
}

void DisplayDevice::swapBuffers(HWComposer& hwc) const {
    // We need to call eglSwapBuffers() if:
    //  (1) we don't have a hardware composer, or
    //  (2) we did GLES composition this frame, and either
    //    (a) we have framebuffer target support (not present on legacy
    //        devices, where HWComposer::commit() handles things); or
    //    (b) this is a virtual display
    if (hwc.initCheck() != NO_ERROR ||
            (hwc.hasGlesComposition(mHwcDisplayId) &&
             (hwc.supportsFramebufferTarget() || mType >= DISPLAY_VIRTUAL))) {
        // MStar Android Patch Begin
#if defined(ENABLE_EGL_SWAP_BUFFERS_WITH_DAMAGE) && defined(EGL_EXT_swap_buffers_with_damage)
        EGLBoolean success = EGL_FALSE;
        if (!mDisplaySurface->isFrameBufferResizeOpen()
            &&mType == DISPLAY_PRIMARY
            &&mFlinger->mEglSwapBuffersWithDamageEXT != NULL
            &&(mFlinger->get4k2kAnd2k1kSwitch() && mFlinger->isChangeTo2k1kMode())) {
            float hScale = 0.5f;
            float vScale = 0.5f;
            int realHeight = mDisplayHeight * hScale;
            int realWidth = mDisplayWidth * vScale;
            const EGLint rects[] = { 0, 0, realWidth, realHeight };
            success = eglSwapBuffersWithDamageEXT(mDisplay, mSurface, rects, 1);
        } else {
            success = eglSwapBuffers(mDisplay, mSurface);
        }
#else
        EGLBoolean success = eglSwapBuffers(mDisplay, mSurface);
#endif
        // MStar Android Patch End
        if (!success) {
            EGLint error = eglGetError();
            if (error == EGL_CONTEXT_LOST ||
                    mType == DisplayDevice::DISPLAY_PRIMARY) {
                LOG_ALWAYS_FATAL("eglSwapBuffers(%p, %p) failed with 0x%08x",
                        mDisplay, mSurface, error);
            } else {
                ALOGE("eglSwapBuffers(%p, %p) failed with 0x%08x",
                        mDisplay, mSurface, error);
            }
        }
    }

    status_t result = mDisplaySurface->advanceFrame();
    if (result != NO_ERROR) {
        ALOGE("[%s] failed pushing new frame to HWC: %d",
                mDisplayName.string(), result);
    }
}

void DisplayDevice::onSwapBuffersCompleted(HWComposer& hwc) const {
    if (hwc.initCheck() == NO_ERROR) {
        mDisplaySurface->onFrameCommitted();
    }
}

uint32_t DisplayDevice::getFlags() const
{
    return mFlags;
}

EGLBoolean DisplayDevice::makeCurrent(EGLDisplay dpy, EGLContext ctx) const {
    EGLBoolean result = EGL_TRUE;
    EGLSurface sur = eglGetCurrentSurface(EGL_DRAW);
    if (sur != mSurface) {
        result = eglMakeCurrent(dpy, mSurface, mSurface, ctx);
        if (result == EGL_TRUE) {
            if (mType >= DisplayDevice::DISPLAY_VIRTUAL)
                eglSwapInterval(dpy, 0);
        }
    }
    setViewportAndProjection();
    return result;
}

void DisplayDevice::setViewportAndProjection() const {
    size_t w = mDisplayWidth;
    size_t h = mDisplayHeight;
    Rect sourceCrop(0, 0, w, h);
    mFlinger->getRenderEngine().setViewportAndProjection(w, h, sourceCrop, h,
        false, Transform::ROT_0);
}

// ----------------------------------------------------------------------------

void DisplayDevice::setVisibleLayersSortedByZ(const Vector< sp<Layer> >& layers) {
    mVisibleLayersSortedByZ = layers;
    mSecureLayerVisible = false;
    size_t count = layers.size();
    for (size_t i=0 ; i<count ; i++) {
        const sp<Layer>& layer(layers[i]);
        if (layer->isSecure()) {
            mSecureLayerVisible = true;
        }
    }
}

const Vector< sp<Layer> >& DisplayDevice::getVisibleLayersSortedByZ() const {
    return mVisibleLayersSortedByZ;
}

bool DisplayDevice::getSecureLayerVisible() const {
    return mSecureLayerVisible;
}

Region DisplayDevice::getDirtyRegion(bool repaintEverything) const {
    Region dirty;
    if (repaintEverything) {
        dirty.set(getBounds());
    } else {
        const Transform& planeTransform(mGlobalTransform);
        dirty = planeTransform.transform(this->dirtyRegion);
        dirty.andSelf(getBounds());
    }
    return dirty;
}

// ----------------------------------------------------------------------------
void DisplayDevice::setPowerMode(int mode) {
    mPowerMode = mode;
}

int DisplayDevice::getPowerMode()  const {
    return mPowerMode;
}

bool DisplayDevice::isDisplayOn() const {
    return (mPowerMode != HWC_POWER_MODE_OFF);
}

// ----------------------------------------------------------------------------
void DisplayDevice::setActiveConfig(int mode) {
    mActiveConfig = mode;
}

int DisplayDevice::getActiveConfig()  const {
    return mActiveConfig;
}

// ----------------------------------------------------------------------------

void DisplayDevice::setLayerStack(uint32_t stack) {
    mLayerStack = stack;
    dirtyRegion.set(bounds());
}

// ----------------------------------------------------------------------------

uint32_t DisplayDevice::getOrientationTransform() const {
    uint32_t transform = 0;
    switch (mOrientation) {
        case DisplayState::eOrientationDefault:
            transform = Transform::ROT_0;
            break;
        case DisplayState::eOrientation90:
            transform = Transform::ROT_90;
            break;
        case DisplayState::eOrientation180:
            transform = Transform::ROT_180;
            break;
        case DisplayState::eOrientation270:
            transform = Transform::ROT_270;
            break;
    }
    return transform;
}

status_t DisplayDevice::orientationToTransfrom(
        int orientation, int w, int h, Transform* tr)
{
    uint32_t flags = 0;
    switch (orientation) {
    case DisplayState::eOrientationDefault:
        flags = Transform::ROT_0;
        break;
    case DisplayState::eOrientation90:
        flags = Transform::ROT_90;
        break;
    case DisplayState::eOrientation180:
        flags = Transform::ROT_180;
        break;
    case DisplayState::eOrientation270:
        flags = Transform::ROT_270;
        break;
    default:
        return BAD_VALUE;
    }
    tr->set(flags, w, h);
    return NO_ERROR;
}

void DisplayDevice::setDisplaySize(const int newWidth, const int newHeight) {
    dirtyRegion.set(getBounds());

    if (mSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mDisplay, mSurface);
        mSurface = EGL_NO_SURFACE;
    }

    mDisplaySurface->resizeBuffers(newWidth, newHeight);

    ANativeWindow* const window = mNativeWindow.get();
    mSurface = eglCreateWindowSurface(mDisplay, mConfig, window, NULL);
    eglQuerySurface(mDisplay, mSurface, EGL_WIDTH,  &mDisplayWidth);
    eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &mDisplayHeight);

    LOG_FATAL_IF(mDisplayWidth != newWidth,
                "Unable to set new width to %d", newWidth);
    LOG_FATAL_IF(mDisplayHeight != newHeight,
                "Unable to set new height to %d", newHeight);
}

void DisplayDevice::setProjection(int orientation,
        const Rect& newViewport, const Rect& newFrame) {
    Rect viewport(newViewport);
    Rect frame(newFrame);

    const int w = mDisplayWidth;
    const int h = mDisplayHeight;

    Transform R;
    DisplayDevice::orientationToTransfrom(orientation, w, h, &R);

    if (!frame.isValid()) {
        // the destination frame can be invalid if it has never been set,
        // in that case we assume the whole display frame.
        frame = Rect(w, h);
    }

    if (viewport.isEmpty()) {
        // viewport can be invalid if it has never been set, in that case
        // we assume the whole display size.
        // it's also invalid to have an empty viewport, so we handle that
        // case in the same way.

        // MStar Android Patch Begin
        if (mFlinger->get4k2kAnd2k1kSwitch()) {
            viewport = Rect(w/2, h/2);
        } else {
            viewport = Rect(mOsdWidth, mOsdHeight);
        }
        // MStar Android Patch End

        if (R.getOrientation() & Transform::ROT_90) {
            // viewport is always specified in the logical orientation
            // of the display (ie: post-rotation).
            swap(viewport.right, viewport.bottom);
        }
    }

    dirtyRegion.set(getBounds());

    Transform TL, TP, S;
    float src_width  = viewport.width();
    float src_height = viewport.height();
    float dst_width  = frame.width();
    float dst_height = frame.height();
    if (src_width != dst_width || src_height != dst_height) {
        float sx = dst_width  / src_width;
        float sy = dst_height / src_height;
        S.set(sx, 0, 0, sy);
    }

    float src_x = viewport.left;
    float src_y = viewport.top;
    float dst_x = frame.left;
    float dst_y = frame.top;
    TL.set(-src_x, -src_y);
    TP.set(dst_x, dst_y);

    // The viewport and frame are both in the logical orientation.
    // Apply the logical translation, scale to physical size, apply the
    // physical translation and finally rotate to the physical orientation.
    mGlobalTransform = R * TP * S * TL;

    const uint8_t type = mGlobalTransform.getType();
    mNeedsFiltering = (!mGlobalTransform.preserveRects() ||
            (type >= Transform::SCALE));

    mScissor = mGlobalTransform.transform(viewport);
    if (mScissor.isEmpty()) {
        mScissor = getBounds();
    }

    mOrientation = orientation;
    mViewport = viewport;
    mFrame = frame;
}

// MStar Android Patch Begin
bool DisplayDevice::resizeDeviceBuffer(const int newWidth, const int newHeight) {
    if (mDisplayWidth != newWidth || mDisplayHeight != newHeight) {
        ALOGD(">>> primary display: %dx%d -> %dx%d",mDisplayWidth,mDisplayHeight,newWidth,newHeight);
        mDisplaySurface->resizeBuffers(newWidth, newHeight);
        mDisplayWidth = newWidth;
        mDisplayHeight = newHeight;
        return true;
    }
    return false;
}

void DisplayDevice::changeFBtoAFBC(bool afbc) {
    mDisplaySurface->changeFBtoAFBC(afbc);
}

void DisplayDevice::setProjectionPlatform(int orientation,
        const Rect& newViewport, const Rect& newFrame) {
    Rect viewport(newViewport);
    Rect frame(newFrame);

    const int w = mDisplayWidth;
    const int h = mDisplayHeight;

    if (!frame.isValid()) {
        // the destination frame can be invalid if it has never been set,
        // in that case we assume the whole display frame.
        frame = Rect(w, h);
    }

    Transform TOverscan;
    if (mLeftOverscan<0 || mTopOverscan<0 || mRightOverscan<0 || mBottomOverscan<0) {
        ALOGE("setProjection overscan<0!");
    } else if (mLeftOverscan + mRightOverscan >= mDisplayWidth - 10 || mTopOverscan + mBottomOverscan >= mDisplayHeight - 10) {
        ALOGE("setProjection overscan excceed screensize-10!");
    } else {
        // Overscan
        int left,right,top,bottom;
        int curTimingWidth = mDisplayWidth,curTimingHeight = mDisplayHeight;
        mDisplaySurface->getCurTiming(&curTimingWidth,&curTimingHeight);
        if (curTimingWidth > frame.getWidth()) {
            left = mLeftOverscan * ((float)frame.getWidth() / curTimingWidth);
            right = mRightOverscan * ((float)frame.getWidth() / curTimingWidth);
        } else {
            left = mLeftOverscan;
            right = mRightOverscan;
        }
        if (curTimingHeight > frame.getHeight()) {
            top = mTopOverscan * ((float)frame.getHeight() / curTimingHeight);
            bottom = mBottomOverscan * ((float)frame.getHeight() / curTimingHeight);
        } else {
            top = mTopOverscan;
            bottom = mBottomOverscan;
        }
        TOverscan.set( (float)(frame.getWidth() - left - right)/frame.getWidth(), .0, .0,
                (float)(frame.getHeight() - top - bottom)/frame.getHeight());
        TOverscan.set(left,top);
    }

    Transform R;
    DisplayDevice::orientationToTransfrom(orientation, frame.getWidth(), frame.getHeight(), &R);
    if((orientation == DisplayState::eOrientation270) ||
       (orientation == DisplayState::eOrientation90)) {
        swap(frame.right, frame.bottom);
    }

    dirtyRegion.set(getBounds());

    Transform TL, TP, S;
    float src_width  = viewport.width();
    float src_height = viewport.height();
    float dst_width  = frame.width();
    float dst_height = frame.height();
    if (src_width != dst_width || src_height != dst_height) {
        float sx = dst_width  / src_width;
        float sy = dst_height / src_height;
        S.set(sx, 0, 0, sy);
    }

    float src_x = viewport.left;
    float src_y = viewport.top;
    float dst_x = frame.left;
    float dst_y = frame.top;
    TL.set(-src_x, -src_y);
    TP.set(dst_x, dst_y);
    // The viewport and frame are both in the logical orientation.
    // Apply the logical translation, scale to physical size, apply the
    // physical translation and finally rotate to the physical orientation.
    mGlobalTransformPlatform = R * TP * S * TL;
    const uint8_t type = mGlobalTransformPlatform.getType();
    mNeedsFiltering = (!mGlobalTransformPlatform.preserveRects() ||
            (type >= Transform::SCALE));

    mScissor = mGlobalTransformPlatform.transform(viewport);
    if (mScissor.isEmpty()) {
        mScissor = getBounds();
    }

    mGlobalTransformPlatform = TOverscan * mGlobalTransformPlatform;

    mFramePlatfom = frame;
    mScissorPlatform = mGlobalTransformPlatform.transform(viewport);
    if (mScissorPlatform.isEmpty()) {
        mScissorPlatform = getBounds();
    }
    recalc3DTransformAndScissor();
    setOverscanAnd3DEnabled(true);
}

void DisplayDevice::recalc3DTransformAndScissor() {
    Transform *TFirst=NULL,*TSecond=NULL;
    mDisplaySurface->recalc3DTransform(&TFirst,&TSecond,mFramePlatfom);
    if (NULL == mLeftGlobalTransform) {
        mLeftGlobalTransform = new Transform();
    }
    if (NULL == mRightGlobalTransform) {
        mRightGlobalTransform = new Transform();
    }
    if (TFirst != NULL) {
        *mLeftGlobalTransform = (*TFirst) * mGlobalTransformPlatform;
        mLeftScissor = mLeftGlobalTransform->transform(mViewport);
        delete TFirst;
    } else {
        delete mLeftGlobalTransform;
        mLeftGlobalTransform = NULL;
    }
    if (TSecond != NULL) {
        *mRightGlobalTransform = (*TSecond) * mGlobalTransformPlatform;
        mRightScissor = mRightGlobalTransform->transform(mViewport);
        delete TSecond;
    } else {
        delete mRightGlobalTransform;
        mRightGlobalTransform = NULL;
    }
}

bool DisplayDevice::setOverscan(int leftOverscan, int topOverscan, int rightOverscan, int bottomOverscan) {
    if (leftOverscan<0 || topOverscan<0 || rightOverscan<0 || bottomOverscan<0) {
        ALOGE("setOverscan<0!");
        return false;
    } else if (leftOverscan + rightOverscan >= mDisplayWidth - 10 || topOverscan + bottomOverscan >= mDisplayHeight - 10) {
        ALOGE("setOverscan excceed screensize-10!");
        return false;
    } else if (mLeftOverscan == leftOverscan &&
            mTopOverscan == topOverscan &&
            mRightOverscan == rightOverscan &&
            mBottomOverscan == bottomOverscan) {
        return false;
    }

    mLeftOverscan   = leftOverscan;
    mTopOverscan    = topOverscan;
    mRightOverscan  = rightOverscan;
    mBottomOverscan = bottomOverscan;

    // Overscan
    Transform TOverscan;
    int left,right,top,bottom;
    int curTimingWidth = mDisplayWidth,curTimingHeight = mDisplayHeight;
    mDisplaySurface->getCurTiming(&curTimingWidth,&curTimingHeight);
    if (curTimingWidth > mFramePlatfom.getWidth()) {
        left = mLeftOverscan * ((float)mFramePlatfom.getWidth() / curTimingWidth);
        right = mRightOverscan * ((float)mFramePlatfom.getWidth() / curTimingWidth);
    } else {
        left = mLeftOverscan;
        right = mRightOverscan;
    }
    if (curTimingHeight > mFramePlatfom.getHeight()) {
        top = mTopOverscan * ((float)mFramePlatfom.getHeight() / curTimingHeight);
        bottom = mBottomOverscan * ((float)mFramePlatfom.getHeight() / curTimingHeight);
    } else {
        top = mTopOverscan;
        bottom = mBottomOverscan;
    }
    TOverscan.set( (float)(mFramePlatfom.getWidth() - left - right)/mFramePlatfom.getWidth(), .0, .0,
            (float)(mFramePlatfom.getHeight() - top - bottom)/mFramePlatfom.getHeight());
    TOverscan.set(left,top);
    mGlobalTransformPlatform = TOverscan * mGlobalTransformPlatform;

    mScissorPlatform = mGlobalTransformPlatform.transform(mViewport);
    if (mScissorPlatform.isEmpty()) {
        mScissorPlatform = getBounds();
    }

    recalc3DTransformAndScissor();
    return true;
}

void DisplayDevice::getOverscan(int *outLeftOverscan, int *outTopOverscan, int *outRightOverscan, int *outBottomOverscan) const {
    *outLeftOverscan    = mLeftOverscan;
    *outTopOverscan     = mTopOverscan;
    *outRightOverscan   = mRightOverscan;
    *outBottomOverscan  = mBottomOverscan;
}

void DisplayDevice::setOverscanAnd3DEnabled(bool enable) {
    mOverscanAnd3DEnabled = enable;
}

bool DisplayDevice::isOverscanAnd3DEnabled() const {
    return mOverscanAnd3DEnabled;
}

const Transform& DisplayDevice::getTransform() const {
    return mOverscanAnd3DEnabled ? mGlobalTransformPlatform : mGlobalTransform;
}

const Rect DisplayDevice::getViewport() const {
    return mViewport;
}

const Rect DisplayDevice::getFrame() const {
    return mOverscanAnd3DEnabled ? mFramePlatfom : mFrame;
}

const Rect& DisplayDevice::getScissor() const {
    return mOverscanAnd3DEnabled ? mScissorPlatform : mScissor;
}

bool DisplayDevice::needsFiltering() const {
    return mOverscanAnd3DEnabled ? true : mNeedsFiltering;
}

void DisplayDevice::getStbResolution(int* Width, int* Height) {
    char property[PROPERTY_VALUE_MAX] = {0};
    *Width = mDisplayWidth;
    *Height = mDisplayHeight;
    if (property_get("mstar.resolutionState", property, NULL) > 0) {
        if (strcmp(property, "RESOLUTION_1080P")==0) {
            *Width = 1920;
            *Height = 1080;
        } else if (strcmp(property, "RESOLUTION_1080I")==0) {
            *Width = 1920;
            *Height = 1080;
        } else if (strcmp(property, "RESOLUTION_720P")==0) {
            *Width = 1280;
            *Height = 720;
        } else if (strcmp(property, "RESOLUTION_576P")==0) {
            *Width = 720;
            *Height = 576;
        } else if (strcmp(property, "RESOLUTION_576I")==0) {
            *Width = 720;
            *Height = 576;
        } else if (strcmp(property, "RESOLUTION_480P")==0) {
            *Width = 720;
            *Height = 480;
        } else if (strcmp(property, "RESOLUTION_480I")==0) {
            *Width = 720;
            *Height = 480;
        } else if ((strcmp(property, "RESOLUTION_4K2K_30")==0)||
                  (strcmp(property, "RESOLUTION_4K2K_25")==0)||
                  (strcmp(property, "RESOLUTION_4K2KP")==0)) {
            *Width = 3840;
            *Height = 2160;
        }
    } else {
        ALOGI("mstar.resolutionState is invalid:%s",property);
    }
}
// MStar Android Patch End

void DisplayDevice::dump(String8& result) const {
    const Transform& tr(mGlobalTransform);
    result.appendFormat(
        "+ DisplayDevice: %s\n"
        "   type=%x, hwcId=%d, layerStack=%u, (%4dx%4d), ANativeWindow=%p, orient=%2d (type=%08x), "
        "flips=%u, isSecure=%d, secureVis=%d, powerMode=%d, activeConfig=%d, numLayers=%zu\n"
        "   v:[%d,%d,%d,%d], f:[%d,%d,%d,%d], s:[%d,%d,%d,%d],"
        "transform:[[%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f]]\n",
        mDisplayName.string(), mType, mHwcDisplayId,
        mLayerStack, mDisplayWidth, mDisplayHeight, mNativeWindow.get(),
        mOrientation, tr.getType(), getPageFlipCount(),
        mIsSecure, mSecureLayerVisible, mPowerMode, mActiveConfig,
        mVisibleLayersSortedByZ.size(),
        mViewport.left, mViewport.top, mViewport.right, mViewport.bottom,
        mFrame.left, mFrame.top, mFrame.right, mFrame.bottom,
        mScissor.left, mScissor.top, mScissor.right, mScissor.bottom,
        tr[0][0], tr[1][0], tr[2][0],
        tr[0][1], tr[1][1], tr[2][1],
        tr[0][2], tr[1][2], tr[2][2]);
    // MStar Android Patch Begin
    result.appendFormat(
        "mFramePlatfom:[%d,%d,%d,%d]\n",
        mFramePlatfom.left, mFramePlatfom.top, mFramePlatfom.right, mFramePlatfom.bottom);

    const Transform& trWithOverScan(mGlobalTransformPlatform);
        result.appendFormat(
        "mGlobalTransformWithOverscan:[[%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f]]\n",
        trWithOverScan[0][0], trWithOverScan[1][0], trWithOverScan[2][0],
        trWithOverScan[0][1], trWithOverScan[1][1], trWithOverScan[2][1],
        trWithOverScan[0][2], trWithOverScan[1][2], trWithOverScan[2][2]
        );
        if (mLeftGlobalTransform != NULL) {
            const Transform& trLeft(*mLeftGlobalTransform);
            result.appendFormat(
                "mLeftGlobalTransform:[[%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f]]\n",
                trLeft[0][0], trLeft[1][0], trLeft[2][0],
                trLeft[0][1], trLeft[1][1], trLeft[2][1],
                trLeft[0][2], trLeft[1][2], trLeft[2][2]
                );
        }
        if (mRightGlobalTransform != NULL) {
            const Transform& trRight(*mRightGlobalTransform);
            result.appendFormat(
                "mRightGlobalTransform:[[%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f]]\n",
                trRight[0][0], trRight[1][0], trRight[2][0],
                trRight[0][1], trRight[1][1], trRight[2][1],
                trRight[0][2], trRight[1][2], trRight[2][2]
                );
        }
    // MStar Android Patch End

    String8 surfaceDump;
    // MStar Android Patch Begin
    char buf[256];
    snprintf(buf, 256, "DisplayDevice_%s_", mDisplayName.string());
    buf[255] = 0;
    graphic_buffer_dump_helper::set_prefix( buf);
    mDisplaySurface->dump(surfaceDump);
    graphic_buffer_dump_helper::set_prefix( "unknow_");
    // MStar Android Patch End
    result.append(surfaceDump);
}
