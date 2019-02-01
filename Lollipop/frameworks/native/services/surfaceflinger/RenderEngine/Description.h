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

#include <GLES2/gl2.h>
#include "Texture.h"

#ifndef SF_RENDER_ENGINE_DESCRIPTION_H_
#define SF_RENDER_ENGINE_DESCRIPTION_H_

namespace android {

// MStar Android Patch Begin
struct R2Y {
    enum Transform {
        NONE,
        BT_601_NARROW,
        BT_601_WIDE,
        BT_709_NARROW,
        BT_709_WIDE,
    };

    static const mat4& getMatrix(Transform transform);
};
// MStar Android Patch End

class Program;

/*
 * This holds the state of the rendering engine. This class is used
 * to generate a corresponding GLSL program and set the appropriate
 * uniform.
 *
 * Program and ProgramCache are friends and access the state directly
 */
class Description {
    friend class Program;
    friend class ProgramCache;

    // value of the plane-alpha, between 0 and 1
    GLclampf mPlaneAlpha;
    // whether textures are premultiplied
    bool mPremultipliedAlpha;
    // whether this layer is marked as opaque
    bool mOpaque;

    // Texture this layer uses
    Texture mTexture;
    bool mTextureEnabled;

    // color used when texturing is disabled
    GLclampf mColor[4];
    // projection matrix
    mat4 mProjectionMatrix;

    bool mColorMatrixEnabled;
    mat4 mColorMatrix;

    // MStar Android Patch Begin
    R2Y::Transform mR2YTransform;
    bool mSDR2HDREnabled;
    float mNits;
    float mTmoSlope;
    float mTmoRolloff;
    float mTmoC1;
    float mTmoC2;
    float mTmoC3;
    // MStar Android Patch End

public:
    Description();
    ~Description();

    void setPlaneAlpha(GLclampf planeAlpha);
    void setPremultipliedAlpha(bool premultipliedAlpha);
    void setOpaque(bool opaque);
    void setTexture(const Texture& texture);
    void disableTexture();
    void setColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void setProjectionMatrix(const mat4& mtx);
    void setColorMatrix(const mat4& mtx);
    // MStar Android Patch Begin
    void setR2YTransform(R2Y::Transform transform);
    void enableSDR2HDR(bool enable);
    // MStar Android Patch End

private:
    bool mUniformsDirty;
};

} /* namespace android */

#endif /* SF_RENDER_ENGINE_DESCRIPTION_H_ */