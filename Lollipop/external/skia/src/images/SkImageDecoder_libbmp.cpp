
/*
 * Copyright 2007 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// MStar Android Patch Begin
#define LOG_TAG "SkiaBmp"
#include <utils/Log.h>
// MStar Android Patch End

#include "bmpdecoderhelper.h"
#include "SkColorPriv.h"
#include "SkImageDecoder.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkStreamHelpers.h"
#include "SkTDArray.h"

// MStar Android Patch Begin
#ifdef SW_BMP_OPTIMIZATION
#define READ_BMP_HEADER_LENGTH_MAX   2048
#define READ_BMP_DATA_MAX_LENGTH_EVERY_TIME   (256 * 1024)
#endif

#define BMP_MAX_WIDTH    (1920 * 16)
#define BMP_MAX_HEIGHT    (1080 * 16)
// MStar Android Patch End

class SkBMPImageDecoder : public SkImageDecoder {
public:
    SkBMPImageDecoder() {}

    virtual Format getFormat() const SK_OVERRIDE {
        return kBMP_Format;
    }

protected:
    virtual Result onDecode(SkStream* stream, SkBitmap* bm, Mode mode) SK_OVERRIDE;

private:
    typedef SkImageDecoder INHERITED;
};

///////////////////////////////////////////////////////////////////////////////
DEFINE_DECODER_CREATOR(BMPImageDecoder);
///////////////////////////////////////////////////////////////////////////////

static bool is_bmp(SkStreamRewindable* stream) {
    static const char kBmpMagic[] = { 'B', 'M' };


    char buffer[sizeof(kBmpMagic)];

    return stream->read(buffer, sizeof(kBmpMagic)) == sizeof(kBmpMagic) &&
        !memcmp(buffer, kBmpMagic, sizeof(kBmpMagic));
}

static SkImageDecoder* sk_libbmp_dfactory(SkStreamRewindable* stream) {
    if (is_bmp(stream)) {
        return SkNEW(SkBMPImageDecoder);
    }
    return NULL;
}

static SkImageDecoder_DecodeReg gReg(sk_libbmp_dfactory);

static SkImageDecoder::Format get_format_bmp(SkStreamRewindable* stream) {
    if (is_bmp(stream)) {
        return SkImageDecoder::kBMP_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

static SkImageDecoder_FormatReg gFormatReg(get_format_bmp);

///////////////////////////////////////////////////////////////////////////////

class SkBmpDecoderCallback : public image_codec::BmpDecoderCallback {
public:
    // we don't copy the bitmap, just remember the pointer
    SkBmpDecoderCallback(bool justBounds) : fJustBounds(justBounds) {}

    // override from BmpDecoderCallback
    virtual uint8* SetSize(int width, int height) {
        fWidth = width;
        fHeight = height;
        if (fJustBounds) {
            return NULL;
        }

        fRGB.setCount(width * height * 3);  // 3 == r, g, b
        return fRGB.begin();
    }

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    const uint8_t* rgb() const { return fRGB.begin(); }

private:
    SkTDArray<uint8_t> fRGB;
    int fWidth;
    int fHeight;
    bool fJustBounds;
};

SkImageDecoder::Result SkBMPImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    // MStar Android Patch Begin
#ifdef SW_BMP_OPTIMIZATION
    SkASSERT(stream != NULL);

    SkAutoMalloc storage;
    size_t readSize = 0;
    size_t haveRead = 0;
    const bool justBounds = SkImageDecoder::kDecodeBounds_Mode == mode;

    if (justBounds)
        readSize = READ_BMP_HEADER_LENGTH_MAX;
    else
        readSize = READ_BMP_DATA_MAX_LENGTH_EVERY_TIME;

    storage.reset(readSize);
    haveRead = stream->read(storage.get(), readSize);
    if (haveRead < 1) {
        ALOGE("[BMP] error: first read error!");
        return kFailure;
    }

    SkBmpDecoderCallback callback(justBounds);
    image_codec::BmpDecoderHelper helper;
    const int max_pixels = 16383*16383; // max width*height

    if (justBounds) {
        if (!helper.DecodeImage((const char*)storage.get(), haveRead, max_pixels, &callback)) {
            ALOGE("[BMP] error: decode resolution fail 1!");
            return kFailure;
        }
    } else {
        // 1. get width and height
        SkBmpDecoderCallback callback_ForGetInfo(true);
        if (!helper.DecodeImage((const char*)storage.get(), haveRead, max_pixels, &callback_ForGetInfo)) {
            ALOGE("[BMP] error: decode resolution fail 2!");
            return kFailure;
        }

        int width = callback_ForGetInfo.width();
        int height = callback_ForGetInfo.height();

        if ((width * height) > (BMP_MAX_WIDTH * BMP_MAX_HEIGHT)) {
            return kFailure;
        }

        SkColorType colorType = this->getPrefColorType(k32Bit_SrcDepth, false);

        // only accept prefConfig if it makes sense for us
        if (kARGB_4444_SkColorType != colorType && kRGB_565_SkColorType != colorType) {
            colorType = kN32_SkColorType;
        }

        SkScaledBitmapSampler sampler(width, height, getSampleSize());
        bm->setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(),
                                  colorType, kOpaque_SkAlphaType));
        // @MSTARFIXME
        //bm->setIsOpaque(true);

        if (!this->allocPixelRef(bm, NULL)) {
            return kFailure;
        }

        SkAutoLockPixels alp(*bm);

        if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
            return kFailure;
        }

        // 2. call DecodeImageEx to decode and do scale down
        if (!helper.DecodeImageEx(stream, (const char*)storage.get(), haveRead, max_pixels, &callback, &sampler, this)) {
            ALOGE("[BMP] error: decode data fail!");
            return kFailure;
        }

        // we don't need this anymore, so free it now (before we try to allocate
        // the bitmap's pixels) rather than waiting for its destructor
        storage.free();
        return kSuccess;
    }
#else
    // First read the entire stream, so that all of the data can be passed to
    // the BmpDecoderHelper.

    // Allocated space used to hold the data.
    SkAutoMalloc storage;
    // Byte length of all of the data.
    const size_t length = CopyStreamToStorage(&storage, stream);
    if (0 == length) {
        return kFailure;
    }

    const bool justBounds = SkImageDecoder::kDecodeBounds_Mode == mode;
    SkBmpDecoderCallback callback(justBounds);

    // Now decode the BMP into callback's rgb() array [r,g,b, r,g,b, ...]
    {
        image_codec::BmpDecoderHelper helper;
        const int max_pixels = 16383*16383; // max width*height
        if (!helper.DecodeImage((const char*)storage.get(), length,
                                max_pixels, &callback)) {
            return kFailure;
        }
    }
#endif
    // MStar Android Patch End

    // we don't need this anymore, so free it now (before we try to allocate
    // the bitmap's pixels) rather than waiting for its destructor
    storage.free();

    int width = callback.width();
    int height = callback.height();
    SkColorType colorType = this->getPrefColorType(k32Bit_SrcDepth, false);

    // only accept prefConfig if it makes sense for us
    if (kARGB_4444_SkColorType != colorType && kRGB_565_SkColorType != colorType) {
        colorType = kN32_SkColorType;
    }

    SkScaledBitmapSampler sampler(width, height, getSampleSize());

    bm->setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(),
                                  colorType, kOpaque_SkAlphaType));

    if (justBounds) {
        return kSuccess;
    }

    if (!this->allocPixelRef(bm, NULL)) {
        return kFailure;
    }

    SkAutoLockPixels alp(*bm);

    if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
        return kFailure;
    }

    const int srcRowBytes = width * 3;
    const int dstHeight = sampler.scaledHeight();
    const uint8_t* srcRow = callback.rgb();

    srcRow += sampler.srcY0() * srcRowBytes;
    for (int y = 0; y < dstHeight; y++) {
        sampler.next(srcRow);
        srcRow += sampler.srcDY() * srcRowBytes;
    }
    return kSuccess;
}
