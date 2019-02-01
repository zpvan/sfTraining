/*
 * Copyright 2013 The Android Open Source Project
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

#include <stdint.h>
#include <string.h>

#include <utils/TypeHelpers.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "Description.h"

namespace android {

Description::Description() :
    mUniformsDirty(true) {
    mPlaneAlpha = 1.0f;
    mPremultipliedAlpha = false;
    mOpaque = true;
    mTextureEnabled = false;
    mColorMatrixEnabled = false;

    memset(mColor, 0, sizeof(mColor));

    // MStar Android Patch Begin
    mR2YTransform = R2Y::Transform::NONE;
    mSDR2HDREnabled = false;
    //for HDR 100-->4000 case.
    //calculate method:OSD_SDR2HDR_details v1 0112 2016.pptx
    mNits = 600.0f;
    mTmoSlope = 3.0f;
    mTmoRolloff = 0.5f;
    mTmoC1 = 0.0000082496f;
    mTmoC2 = 1.9594f;
    mTmoC3 = -0.5534f;
    // MStar Android Patch End
}

Description::~Description() {
}

void Description::setPlaneAlpha(GLclampf planeAlpha) {
    if (planeAlpha != mPlaneAlpha) {
        mUniformsDirty = true;
        mPlaneAlpha = planeAlpha;
    }
}

void Description::setPremultipliedAlpha(bool premultipliedAlpha) {
    if (premultipliedAlpha != mPremultipliedAlpha) {
        mPremultipliedAlpha = premultipliedAlpha;
    }
}

void Description::setOpaque(bool opaque) {
    if (opaque != mOpaque) {
        mOpaque = opaque;
    }
}

void Description::setTexture(const Texture& texture) {
    mTexture = texture;
    mTextureEnabled = true;
    mUniformsDirty = true;
}

void Description::disableTexture() {
    mTextureEnabled = false;
}

void Description::setColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    mColor[0] = red;
    mColor[1] = green;
    mColor[2] = blue;
    mColor[3] = alpha;
    mUniformsDirty = true;
}

void Description::setProjectionMatrix(const mat4& mtx) {
    mProjectionMatrix = mtx;
    mUniformsDirty = true;
}

void Description::setColorMatrix(const mat4& mtx) {
    const mat4 identity;
    mColorMatrix = mtx;
    mColorMatrixEnabled = (mtx != identity);
}

// MStar Android Patch Begin
void Description::setR2YTransform(R2Y::Transform transform) {
    mR2YTransform = transform;
}
// SDR -> HDR
void Description::enableSDR2HDR(bool enable) {
    mSDR2HDREnabled = enable;
}
// RGB -> VYU
const mat4& R2Y::getMatrix(Transform transform) {
    switch (transform)
    {
        case BT_601_NARROW:
        {
            const static mat4 mat(
                     0.439215686f, 0.182585882f, -0.100643732f, 0.000000000f,
                    -0.398942163f, 0.614230588f, -0.338571954f, 0.000000000f,
                    -0.040273524f, 0.062007059f,  0.439215686f, 0.000000000f,
                     0.501960784f, 0.062745098f,  0.501960784f, 1.000000000f
            );
            return mat;
        }
        case BT_601_WIDE:
        {
            const static mat4 mat(
                     0.500000000f, 0.299000000f, -0.168735892f, 0.000000000f,
                    -0.418687589f, 0.587000000f, -0.331264108f, 0.000000000f,
                    -0.081312411f, 0.114000000f,  0.500000000f, 0.000000000f,
                     0.501960784f, 0.000000000f,  0.501960784f, 1.000000000f
            );
            return mat;
        }
        case BT_709_NARROW:
        {
            const static mat4 mat(
                     0.439215686f, 0.182585882f, -0.100643732f, 0.000000000f,
                    -0.398942163f, 0.614230588f, -0.338571954f, 0.000000000f,
                    -0.040273524f, 0.062007059f,  0.439215686f, 0.000000000f,
                     0.501960784f, 0.062745098f,  0.501960784f, 1.000000000f
            );
            return mat;
        }
        case BT_709_WIDE:
        {
            const static mat4 mat(
                     0.500000000f, 0.212600000f, -0.114572106f, 0.000000000f,
                    -0.454152908f, 0.715200000f, -0.385427894f, 0.000000000f,
                    -0.045847092f, 0.072200000f,  0.500000000f, 0.000000000f,
                     0.501960784f, 0.000000000f,  0.501960784f, 1.000000000f
            );
            return mat;
        }
        default:
        {
            const static mat4 identity;
            return identity;
        }
    }
}
// MStar Android Patch End


} /* namespace android */
