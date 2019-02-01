/*
 * Copyright (C) 2006 The Android Open Source Project
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

#ifndef ANDROID_GUI_ISURFACE_COMPOSER_H
#define ANDROID_GUI_ISURFACE_COMPOSER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <utils/Timers.h>
#include <utils/Vector.h>

#include <binder/IInterface.h>

#include <ui/FrameStats.h>
#include <ui/PixelFormat.h>

#include <gui/IGraphicBufferAlloc.h>
#include <gui/ISurfaceComposerClient.h>

// MStar Android Patch Begin
#include <binder/IMemory.h>
// MStar Android Patch End

namespace android {
// ----------------------------------------------------------------------------

class ComposerState;
class DisplayState;
struct DisplayInfo;
class DisplayStatInfo;
class IDisplayEventConnection;
class IMemoryHeap;
class Rect;

/*
 * This class defines the Binder IPC interface for accessing various
 * SurfaceFlinger features.
 */
class ISurfaceComposer: public IInterface {
public:
    DECLARE_META_INTERFACE(SurfaceComposer);

    // flags for setTransactionState()
    enum {
        eSynchronous = 0x01,
        eAnimation   = 0x02,
    };

    enum {
        eDisplayIdMain = 0,
        eDisplayIdHdmi = 1
    };

    enum Rotation {
        eRotateNone = 0,
        eRotate90   = 1,
        eRotate180  = 2,
        eRotate270  = 3
    };

    /* create connection with surface flinger, requires
     * ACCESS_SURFACE_FLINGER permission
     */
    virtual sp<ISurfaceComposerClient> createConnection() = 0;

    /* create a graphic buffer allocator
     */
    virtual sp<IGraphicBufferAlloc> createGraphicBufferAlloc() = 0;

    /* return an IDisplayEventConnection */
    virtual sp<IDisplayEventConnection> createDisplayEventConnection() = 0;

    /* create a virtual display
     * requires ACCESS_SURFACE_FLINGER permission.
     */
    virtual sp<IBinder> createDisplay(const String8& displayName,
            bool secure) = 0;

    /* destroy a virtual display
     * requires ACCESS_SURFACE_FLINGER permission.
     */
    virtual void destroyDisplay(const sp<IBinder>& display) = 0;

    /* get the token for the existing default displays. possible values
     * for id are eDisplayIdMain and eDisplayIdHdmi.
     */
    virtual sp<IBinder> getBuiltInDisplay(int32_t id) = 0;

    /* open/close transactions. requires ACCESS_SURFACE_FLINGER permission */
    virtual void setTransactionState(const Vector<ComposerState>& state,
            const Vector<DisplayState>& displays, uint32_t flags) = 0;

    /* signal that we're done booting.
     * Requires ACCESS_SURFACE_FLINGER permission
     */
    virtual void bootFinished() = 0;

    /* verify that an IGraphicBufferProducer was created by SurfaceFlinger.
     */
    virtual bool authenticateSurfaceTexture(
            const sp<IGraphicBufferProducer>& surface) const = 0;

    /* set display power mode. depending on the mode, it can either trigger
     * screen on, off or low power mode and wait for it to complete.
     * requires ACCESS_SURFACE_FLINGER permission.
     */
    virtual void setPowerMode(const sp<IBinder>& display, int mode) = 0;

    /* returns information for each configuration of the given display
     * intended to be used to get information about built-in displays */
    virtual status_t getDisplayConfigs(const sp<IBinder>& display,
            Vector<DisplayInfo>* configs) = 0;

    /* returns display statistics for a given display
     * intended to be used by the media framework to properly schedule
     * video frames */
    virtual status_t getDisplayStats(const sp<IBinder>& display,
            DisplayStatInfo* stats) = 0;

    /* indicates which of the configurations returned by getDisplayInfo is
     * currently active */
    virtual int getActiveConfig(const sp<IBinder>& display) = 0;

    /* specifies which configuration (of those returned by getDisplayInfo)
     * should be used */
    virtual status_t setActiveConfig(const sp<IBinder>& display, int id) = 0;

    /* Capture the specified screen. requires READ_FRAME_BUFFER permission
     * This function will fail if there is a secure window on screen.
     */
    virtual status_t captureScreen(const sp<IBinder>& display,
            const sp<IGraphicBufferProducer>& producer,
            Rect sourceCrop, uint32_t reqWidth, uint32_t reqHeight,
            uint32_t minLayerZ, uint32_t maxLayerZ,
            bool useIdentityTransform,
            Rotation rotation = eRotateNone) = 0;

    /* Clears the frame statistics for animations.
     *
     * Requires the ACCESS_SURFACE_FLINGER permission.
     */
    virtual status_t clearAnimationFrameStats() = 0;

    /* Gets the frame statistics for animations.
     *
     * Requires the ACCESS_SURFACE_FLINGER permission.
     */
    virtual status_t getAnimationFrameStats(FrameStats* outStats) const = 0;
    // MStar Android Patch Begin
    virtual status_t setAutoStereoMode(int32_t identity, int32_t autoStereo) = 0;
    virtual status_t setBypassTransformMode(int32_t identity, int32_t bypassTransform) = 0;
    virtual status_t setPanelMode(int32_t panelMode) = 0;
    virtual status_t getPanelMode(int32_t *panelMode) = 0;
    virtual status_t setGopStretchWin(int32_t gopNo, int32_t dest_Width, int32_t dest_Height) = 0;
    virtual status_t setGameCursorShow() = 0;
    virtual status_t setGameCursorHide() = 0;
    virtual status_t setGameCursorMatrix(float dsdx, float dtdx, float dsdy, float dtdy) = 0;
    virtual status_t setGameCursorPosition(float positionX, float positionY) =0;
    virtual status_t setGameCursorAlpha(float alpha) = 0;
    virtual status_t changeGameCursorIcon() = 0;
    virtual status_t doGameCursorTransaction() = 0;
    virtual status_t setGameCursorWidth(int32_t cursorWidth) = 0;
    virtual status_t setGameCursorHeight(int32_t cursorHeight) = 0;
    virtual status_t setGameCursorStride(int32_t cursorStride) = 0;
    virtual sp<IMemory> getGameCursorIconBuf() = 0;
    virtual status_t initGameCursor() = 0;
    virtual status_t setSurfaceResolutionMode(int32_t width, int32_t height, int32_t hstart,int32_t interleave,int32_t orientation, int32_t value) = 0;
    // MStar Android Patch End
};

// ----------------------------------------------------------------------------

class BnSurfaceComposer: public BnInterface<ISurfaceComposer> {
public:
    enum {
        // Note: BOOT_FINISHED must remain this value, it is called from
        // Java by ActivityManagerService.
        BOOT_FINISHED = IBinder::FIRST_CALL_TRANSACTION,
        CREATE_CONNECTION,
        CREATE_GRAPHIC_BUFFER_ALLOC,
        CREATE_DISPLAY_EVENT_CONNECTION,
        CREATE_DISPLAY,
        DESTROY_DISPLAY,
        GET_BUILT_IN_DISPLAY,
        SET_TRANSACTION_STATE,
        AUTHENTICATE_SURFACE,
        GET_DISPLAY_CONFIGS,
        GET_ACTIVE_CONFIG,
        SET_ACTIVE_CONFIG,
        CONNECT_DISPLAY,
        CAPTURE_SCREEN,
        CLEAR_ANIMATION_FRAME_STATS,
        GET_ANIMATION_FRAME_STATS,
        SET_POWER_MODE,
        GET_DISPLAY_STATS,
        // MStar Android Patch Begin
        MSTAR_PERSERVED = 100,
        SET_AUTO_STEREO_MODE,
        SET_BYPASS_TRANSFORM_MODE,
        SET_PANEL_MODE,
        SET_SURFACE_RESOLUTION_MODE,
        GET_PANEL_MODE,
        SET_GOP_STRETCH_WIN,
        SET_GAMECURSOR_SHOW,
        SET_GAMECURSOR_HIDE,
        SET_GAMECURSOR_MATRIX,
        SET_GAMECURSOR_POSITION,
        SET_GAMECURSOR_ALPHA,
        CHANGE_GAMECURSOR_ICON,
        DO_GAMECURSOR_TRANSACTION,
        GET_GAMECURSOR_ICONBUF,
        SET_GAMECURSOR_WIDTH,
        SET_GAMECURSOR_HEIGHT,
        SET_GAMECURSOR_STRIDE,
        INIT_GAMECURSOR_MODULE,
        // MStar Android Patch End
    };

    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_GUI_ISURFACE_COMPOSER_H
