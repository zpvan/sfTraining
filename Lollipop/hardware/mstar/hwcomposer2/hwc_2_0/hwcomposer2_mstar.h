/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_HWCOMPOSER2_MSTAR_H
#define ANDROID_HARDWARE_HWCOMPOSER2_MSTAR_H

#include <hardware/hwcomposer2.h>
#include <string>
#include <utils/Timers.h>

#define GAMECURSOR_COUNT 2

__BEGIN_DECLS

#ifdef USE_HWC2
typedef struct hwc_win {
    int x;
    int y;
    int width;
    int height;
    hwc_rect_t osdRegion;
} hwc_win_t;

typedef struct hwc_3d_param {
    float scaleX;
    float scaleY;
    float TanslateX;
    float TanslateY;
    int isVisiable;
} hwc_3d_param_t;

typedef struct hwc_capture_data {
    uintptr_t address;
    hwc_rect_t region;
} hwc_capture_data_t;
#endif

enum {
    HWC2_DISPLAY_MIRROR_DEFAULT = 1,
    HWC2_DISPLAY_MIRROR_H    = 2,
    HWC2_DISPLAY_MIRROR_V    = 4,
};

/* Function descriptors for use with getFunction */
typedef enum {
    HWC2_FUNCTION_SET_LAYER_NAME = HWC2_FUNCTION_VALIDATE_DISPLAY + 1,
    HWC2_FUNCTION_REDRAW_CURSOR,
    HWC2_FUNCTION_GET_LAYER_VENDOR_USAGES,
    HWC2_FUNCTION_GET_DISPLAY_3D_TRANSFORM,
    HWC2_FUNCTION_GET_DISPLAY_TIMING,
    HWC2_FUNCTION_GET_DISPLAY_IS_STB,
    HWC2_FUNCTION_GET_DISPLAY_OSD_WIDTH,
    HWC2_FUNCTION_GET_DISPLAY_OSD_HEIGHT,
    HWC2_FUNCTION_GET_DISPLAY_MIRROR_MODE,
    HWC2_FUNCTION_GET_DISPLAY_REFRESH_PERIOD,
    HWC2_FUNCTION_GET_DISPLAY_CAPTURE_SCREEN,
    HWC2_FUNCTION_GET_DISPLAY_CAPTURE_RELEASE,
    HWC2_FUNCTION_GET_DISPLAY_IS_RGBCOLORSPACE,
    HWC2_FUNCTION_GET_DISPLAY_IS_AFBC_FB,
    HWC2_FUNCTION_SET_GAME_CURSOR_SHOW_BY_DEVNO,
    HWC2_FUNCTION_SET_GAME_CURSOR_HIDE_BY_DEVNO,
    HWC2_FUNCTION_SET_GAME_CURSOR_ALPHA_BY_DEVNO,
    HWC2_FUNCTION_REDRAW_GAME_CURSOR_FB_BY_DEVNO,
    HWC2_FUNCTION_SET_GAME_CURSOR_POSITION_BY_DEVNO,
    HWC2_FUNCTION_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO,
    HWC2_FUNCTION_GET_GAME_CURSOR_FBADDR_BY_DEVNO,
    HWC2_FUNCTION_INIT_GAME_CURSOR_BY_DEVNO,
} hwc2_function_descriptor_mstar_t;

/* Possible composition types for a given layer */
typedef enum {
    /* this layer will be skiped when drawing with openGL, this layer type will
      * be set during validate() when HWC2_COMPOSITION_CLIENT type layer needn't be redraw
      */
    HWC2_COMPOSITION_CLIENT_SKIP = HWC2_COMPOSITION_DEVICE,
    HWC2_COMPOSITION_CLIENT_TARGET,
} hwc2_composition_mstar_t;

static inline const char* getCompositionMstarName(hwc2_composition_mstar_t composition) {
    switch (composition) {
        case HWC2_COMPOSITION_CLIENT_SKIP: return "ClientSkip";
        case HWC2_COMPOSITION_CLIENT_TARGET: return "ClientTarget";
        default:
            return getCompositionName(static_cast<hwc2_composition_t>(composition));
    }
}

static inline const char* getFunctionDescriptorMstarName(
        hwc2_function_descriptor_mstar_t desc) {
    switch (desc) {
        case HWC2_FUNCTION_SET_LAYER_NAME: return "SetLayerName";
        case HWC2_FUNCTION_REDRAW_CURSOR: return "redrawHwCursorFb";
        case HWC2_FUNCTION_GET_LAYER_VENDOR_USAGES: return "getLayerVendorUsages";
        case HWC2_FUNCTION_GET_DISPLAY_3D_TRANSFORM: return "get3DTransform";
        case HWC2_FUNCTION_GET_DISPLAY_TIMING: return "getCurrentTiming";
        case HWC2_FUNCTION_GET_DISPLAY_IS_STB: return "isStbTarget";
        case HWC2_FUNCTION_GET_DISPLAY_OSD_WIDTH: return "getOsdWidth";
        case HWC2_FUNCTION_GET_DISPLAY_OSD_HEIGHT: return "getOsdHeight";
        case HWC2_FUNCTION_GET_DISPLAY_MIRROR_MODE: return "getMirrorMode";
        case HWC2_FUNCTION_GET_DISPLAY_REFRESH_PERIOD: return "getRefreshPeriod";
        case HWC2_FUNCTION_GET_DISPLAY_CAPTURE_SCREEN: return "captureScreen";
        case HWC2_FUNCTION_GET_DISPLAY_CAPTURE_RELEASE: return "releaseCaptureScreenMemory";
        case HWC2_FUNCTION_GET_DISPLAY_IS_RGBCOLORSPACE: return "isRGBColorSpace";
        case HWC2_FUNCTION_GET_DISPLAY_IS_AFBC_FB: return "isAFBCFrameBuffer";
        case HWC2_FUNCTION_SET_GAME_CURSOR_SHOW_BY_DEVNO: return "setGameCursorShowByDevNo";
        case HWC2_FUNCTION_SET_GAME_CURSOR_HIDE_BY_DEVNO: return "setGameCursorHideByDevNo";
        case HWC2_FUNCTION_SET_GAME_CURSOR_ALPHA_BY_DEVNO: return "setGameCursorAlphaByDevNo";
        case HWC2_FUNCTION_REDRAW_GAME_CURSOR_FB_BY_DEVNO: return "redrawGameCursorFbByDevNo";
        case HWC2_FUNCTION_SET_GAME_CURSOR_POSITION_BY_DEVNO: return "setGameCursorPositionByDevNo";
        case HWC2_FUNCTION_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO: return "doGameCursorTransactionByDevNo";
        case HWC2_FUNCTION_GET_GAME_CURSOR_FBADDR_BY_DEVNO: return "getGameCursorFbVaddrByDevNo";
        case HWC2_FUNCTION_INIT_GAME_CURSOR_BY_DEVNO: return "initGameCursorByDevNo";
        default:
            return getFunctionDescriptorName(static_cast<hwc2_function_descriptor_t>(desc));
    }
}

/*
 * Display Functions
 *
 * get3DTransform(..., params,frame)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_3D_TRANSFORM
 * Must be provided by all HWC2 devices
 *
 * Gets the display 3D transform
 *
 * Parameters:
 *   params - the return value
 *   frame - display frame region
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_3D_TRANSFORM)(
        hwc2_device_t* device, hwc2_display_t display, hwc_3d_param_t* params, hwc_rect_t frame);

/*
 * Display Functions
 *
 * getCurTiming(..., width,height)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_TIMING
 * Must be provided by all HWC2 devices
 *
 * Gets the display current timing
 *
 * Parameters:
 *   width - return width
 *   height - return height
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_TIMING)(
        hwc2_device_t* device, hwc2_display_t display, int32_t *width, int32_t *height);

/*
 * Display Functions
 *
 * isStbTarget(...outValue)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_IS_STB
 * Must be provided by all HWC2 devices
 *
 * Gets the display is STB
 *
 * Parameters:
 *   NONE
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_IS_STB)(
        hwc2_device_t* device, hwc2_display_t display,bool* outValue);
/*
 * Display Functions
 *
 * getOsdWidth(...outWidth)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_OSD_WIDTH
 * Must be provided by all HWC2 devices
 *
 * Gets the display is osd width
 *
 * Parameters:
 *   outWidth - return value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_OSD_WIDTH)(
        hwc2_device_t* device, hwc2_display_t display, int32_t* outWidth);

/*
 * Display Functions
 *
 * getOsdHeight(...outHeight)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_OSD_HEIGHT
 * Must be provided by all HWC2 devices
 *
 * Gets the display is osd height
 *
 * Parameters:
 *   outHeight - return vaule
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_OSD_HEIGHT)(
        hwc2_device_t* device, hwc2_display_t display,int32_t* outHeight);

/*
 * Display Functions
 *
 * getMirrorMode(...mirrorMode)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_MIRROR_MODE
 * Must be provided by all HWC2 devices
 *
 * Gets the display mirror mode
 *
 * Parameters:
 *   mirrorMode - return value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_MIRROR_MODE)(
        hwc2_device_t* device, hwc2_display_t display,uint32_t* mirrorMode);

/*
 * Display Functions
 *
 * getRefreshPeriod(...outPeriod)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_REFRESH_PERIOD
 * Must be provided by all HWC2 devices
 *
 * Gets the display mirror mode
 *
 * Parameters:
 *   outPeriod - return value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_REFRESH_PERIOD)(
        hwc2_device_t* device, hwc2_display_t display,nsecs_t* outPeriod);

/*
 * Display Functions
 *
 * captureScreen(...outNumElements,outLayers,outDatas,addr,width,height,withOsd)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_CAPTURE_SCREEN
 * Must be provided by all HWC2 devices
 *
 * capture screen,get video data from dip module.
 *
 * Parameters:
 *   outNumElements
 *   outLayers
 *   outDatas
 *   addr
 *   width
 *   height
 *   withOsd
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_LAYER - an invalid layer handle was passed in
 *   HWC2_ERROR_BAD_PARAMETER - the buffer handle passed in was invalid
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_CAPTURE_SCREEN)(
        hwc2_device_t* device, hwc2_display_t display,
        uint32_t* outNumElements, hwc2_layer_t* outLayers,
        hwc_capture_data_t* outDatas,
        uintptr_t* addr, int* width, int* height, int withOsd);
/*
 * Display Functions
 *
 * releaseCaptureScreenMemory(...)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_CAPTURE_RELEASE
 * Must be provided by all HWC2 devices
 *
 * release Capture Screen's Memory
 *
 * Parameters:
 *   NONE
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_NOT_VALIDATED - mmap error or mutex unlock error
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_CAPTURE_RELEASE)(
        hwc2_device_t* device, hwc2_display_t display);

/*
 * Display Functions
 *
 * isRGBColorSpace(...)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_IS_RGBCOLORSPACE
 * Must be provided by all HWC2 devices
 *
 * Gets the display is RGB ColorSpace
 *
 * Parameters:
 *   outValue - return value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_IS_RGBCOLORSPACE)(
        hwc2_device_t* device, hwc2_display_t display,bool* outValue);

/*
 * Display Functions
 *
 * isAFBCFramebuffer(...)
 * Descriptor: HWC2_FUNCTION_GET_DISPLAY_IS_AFBC_FB
 * Must be provided by all HWC2 devices
 *
 * Gets the display is AFBC Framebuffer
 *
 * Parameters:
 *   outValue - return value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_DISPLAY_IS_AFBC_FB)(
        hwc2_device_t* device, hwc2_display_t display,bool* outValue);

/*
 * Layer Functions
 *
 * setLayerName(..., name)
 * Descriptor: HWC2_FUNCTION_SET_LAYER_NAME
 * Must be provided by all HWC2 devices
 *
 * Sets the layer name  for this layer. It uesd for debug infomation.
 *
 * Parameters:
 *   name - the name to set
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_LAYER - an invalid layer handle was passed in
 *   HWC2_ERROR_BAD_PARAMETER - the buffer handle passed in was invalid
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_SET_LAYER_NAME)(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        std::string name);


/*
 * Display Functions
 *
 * redrawHwCursorFb(..., srcX,srcY,srcWidth,srcHeight,stride,dstWidth,dstHeight)
 * Descriptor: HWC2_FUNCTION_REDRAW_CURSOR
 * Must be provided by all HWC2 devices
 *
 * redraw HwCursor
 *
 * Parameters:
 *   srcX -
 *   srcY -
 *   srcWidth -
 *   srcHeight -
 *   stride -
 *   dstWidth -
 *   dstHeight -
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_REDRAW_CURSOR)(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t srcX, int32_t srcY, int32_t srcWidth, int32_t srcHeight, int32_t stride,
        int32_t dstWidth, int32_t dstHeight);

/*
 * Layer Functions
 *
 * getLayerVendorUsage(..., usage)
 * Descriptor: HWC2_FUNCTION_GET_LAYER_VENDOR_USAGES
 * Must be provided by all HWC2 devices
 *
 *  get Layer Vendor Usage
 *
 * Parameters:
 *   usage - return value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_LAYER_VENDOR_USAGES)(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        uint32_t* usage);

/*
 * Game Cursor Functions
 *
 * setGameCursorShowByDevNo(..., devNo)
 * Descriptor: HWC2_FUNCTION_SET_GAME_CURSOR_SHOW_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * Show game cursor.
 *
 * Parameters:
 *   devNo - the device number
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_SET_GAME_CURSOR_SHOW_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo);

/*
 * Game Cursor Functions
 *
 * setGameCursorHideByDevNo(..., devNo)
 * Descriptor: HWC2_FUNCTION_SET_GAME_CURSOR_HIDE_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * Hide game cursor.
 *
 * Parameters:
 *   devNo - the device number
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_SET_GAME_CURSOR_HIDE_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo);


/*
 * Game Cursor Functions
 *
 * setGameCursorAlphaByDevNo(..., devNo,alpha)
 * Descriptor: HWC2_FUNCTION_SET_GAME_CURSOR_ALPHA_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * Sets the game cursor's alpha.
 *
 * Parameters:
 *   devNo - the device number
 *   alpha - alpha value
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_SET_GAME_CURSOR_ALPHA_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo, float alpha);

/*
 * Game Cursor Functions
 *
 * redrawGameCursorFbByDevNo(..., devNo,srcX,srcY,srcWidth,srcHeight,stride,dstWidth,dstHeight)
 * Descriptor: HWC2_FUNCTION_REDRAW_GAME_CURSOR_FB_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * redraw Game Cursor.
 *
 * Parameters:
 *   devNo - the device number
 *   srcX,srcY,srcWidth,srcHeight,stride,dstWidth,dstHeight - GE Parameters
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_REDRAW_GAME_CURSOR_FB_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo,
        int32_t srcX, int32_t srcY, int32_t srcWidth, int32_t srcHeight, int32_t stride,
        int32_t dstWidth, int32_t dstHeight);

/*
 * Game Cursor Functions
 *
 * setGameCursorPositionByDevNo(..., devNo,x,y)
 * Descriptor: HWC2_FUNCTION_SET_GAME_CURSOR_POSITION_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * Sets the game cursor's position.
 *
 * Parameters:
 *   devNo - the device number
 *   x,y - position
 *
 * Returns HWC2_ERROR_NONE
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_SET_GAME_CURSOR_POSITION_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo, int32_t x, int32_t y);

/*
 * Game Cursor Functions
 *
 * doGameCursorTransactionByDevNo(..., devNo)
 * Descriptor: HWC2_FUNCTION_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * do GameCursor Transaction.
 *
 * Parameters:
 *   devNo - the device number
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_LAYER - an invalid layer handle was passed in
 *   HWC2_ERROR_BAD_PARAMETER - the buffer handle passed in was invalid
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo);

/*
 * Game Cursor Functions
 *
 * getGameCursorFbVaddrByDevNo(..., devNo,vaddr)
 * Descriptor: HWC2_FUNCTION_GET_GAME_CURSOR_FBADDR_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * get GameCursor's buffer Vaddr.
 *
 * Parameters:
 *   devNo - the device number
 *   vaddr - return value
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_CONFIG - hwcursor module not init!!!
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_GET_GAME_CURSOR_FBADDR_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo, uintptr_t* vaddr);

/*
 * Game Cursor Functions
 *
 * initGameCursorByDevNo(..., devNo)
 * Descriptor: HWC2_FUNCTION_INIT_GAME_CURSOR_BY_DEVNO
 * Must be provided by all HWC2 devices
 *
 * initialize devNo's cursor
 *
 * Parameters:
 *   devNo - the device number
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_CONFIG - mmapInfo is incorrect.
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_PFN_INIT_GAME_CURSOR_BY_DEVNO)(
        hwc2_device_t* device, hwc2_display_t display, int32_t devNo);

__END_DECLS

namespace HWC2 {

enum class FunctionDescriptorMstar : int32_t {
    SetLayerName = HWC2_FUNCTION_SET_LAYER_NAME,
    RedrawHwCursorFb = HWC2_FUNCTION_REDRAW_CURSOR,
    GetLayerVendorUsages = HWC2_FUNCTION_GET_LAYER_VENDOR_USAGES,
    Get3DTransform = HWC2_FUNCTION_GET_DISPLAY_3D_TRANSFORM,
    GetCurrentTiming = HWC2_FUNCTION_GET_DISPLAY_TIMING,
    IsStbTarget = HWC2_FUNCTION_GET_DISPLAY_IS_STB,
    GetOsdWidth = HWC2_FUNCTION_GET_DISPLAY_OSD_WIDTH,
    GetOsdHeight = HWC2_FUNCTION_GET_DISPLAY_OSD_HEIGHT,
    GetMirrorMode = HWC2_FUNCTION_GET_DISPLAY_MIRROR_MODE,
    GetRefreshPeriod = HWC2_FUNCTION_GET_DISPLAY_REFRESH_PERIOD,
    CaptureScreen = HWC2_FUNCTION_GET_DISPLAY_CAPTURE_SCREEN,
    ReleaseCaptureScreenMemory = HWC2_FUNCTION_GET_DISPLAY_CAPTURE_RELEASE,
    IsRGBColorSpace = HWC2_FUNCTION_GET_DISPLAY_IS_RGBCOLORSPACE,
    IsAFBCFramebuffer = HWC2_FUNCTION_GET_DISPLAY_IS_AFBC_FB,
    SetGameCursorShowByDevNo = HWC2_FUNCTION_SET_GAME_CURSOR_SHOW_BY_DEVNO,
    SetGameCursorHideByDevNo = HWC2_FUNCTION_SET_GAME_CURSOR_HIDE_BY_DEVNO,
    SetGameCursorAlphaByDevNo = HWC2_FUNCTION_SET_GAME_CURSOR_ALPHA_BY_DEVNO,
    RedrawGameCursorFbByDevNo = HWC2_FUNCTION_REDRAW_GAME_CURSOR_FB_BY_DEVNO,
    SetGameCursorPositionByDevNo = HWC2_FUNCTION_SET_GAME_CURSOR_POSITION_BY_DEVNO,
    DoGameCursorTransactionByDevNo = HWC2_FUNCTION_DO_GAME_CURSOR_TRANSACTION_BY_DEVNO,
    GetGameCursorFbVaddrByDevNo = HWC2_FUNCTION_GET_GAME_CURSOR_FBADDR_BY_DEVNO,
    InitGameCursorByDevNo = HWC2_FUNCTION_INIT_GAME_CURSOR_BY_DEVNO,
};
TO_STRING(hwc2_function_descriptor_mstar_t, FunctionDescriptorMstar,
        getFunctionDescriptorMstarName)

enum class CompositionMstar : int32_t {
    ClientSkip = HWC2_COMPOSITION_CLIENT_SKIP,
    ClientTarget = HWC2_COMPOSITION_CLIENT_TARGET,
};
TO_STRING(hwc2_composition_mstar_t, CompositionMstar,
        getCompositionMstarName)

}

#endif
