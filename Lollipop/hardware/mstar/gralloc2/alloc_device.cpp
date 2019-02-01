/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"

#include "alloc_device_allocator_specific.h"
#if MALI_AFBC_GRALLOC == 1
#include "gralloc_buffer_priv.h"
#endif

#include "format_chooser.h"

#define AFBC_PIXELS_PER_BLOCK                    16
#define AFBC_BODY_BUFFER_BYTE_ALIGNMENT          1024
#define AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY  16
#define AFBC_WIDEBLK_WIDTH_ALIGN                 32

// MStar: moved to gralloc_priv.h so that both mali and gralloc use the same alignment
#ifndef MSTAR_GRALLOC
// This value is platform specific and should be set according to hardware YUV planes restrictions.
// Please note that EGL winsys platform config file needs to use the same value when importing buffers.
#define YUV_MALI_PLANE_ALIGN 128

// Default YUV stride aligment in Android
#define YUV_ANDROID_PLANE_ALIGN 16
#endif

#ifdef MSTAR_GRALLOC_AFBC
/*
 * Type of allocation
 */
enum AllocType
{
    UNCOMPRESSED = 0,
    AFBC,
    /* AFBC_WIDEBLK mode requires buffer to have 32 * 16 pixels alignment */
    AFBC_WIDEBLK,
    /* AN AFBC buffer with additional padding to ensure a 64-bte alignment
     * for each row of blocks in the header */
    AFBC_PADDED
};
#endif

#ifdef MSTAR_GRALLOC_AFBC
static int gralloc_alloc_framebuffer_locked(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, int* stride, int* byte_stride, AllocType type)
#else
static int gralloc_alloc_framebuffer_locked(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, int* stride, int* byte_stride)
#endif
{
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

    // allocate the framebuffer
    if (m->framebuffer == NULL)
    {
        // initialize the framebuffer, the framebuffer is mapped once and forever.
        int err = init_frame_buffer_locked(m);
        if (err < 0)
        {
            return err;
        }
    }

    const uint32_t bufferMask = m->bufferMask;
    const uint32_t numBuffers = m->numBuffers;
    /* framebufferSize is used for allocating the handle to the framebuffer and refers
     *                 to the size of the actual framebuffer.
     * alignedFramebufferSize is used for allocating a possible internal buffer and
     *                        thus need to consider internal alignment requirements. */
    const size_t framebufferSize = m->finfo.line_length * m->info.yres;
    const size_t alignedFramebufferSize = GRALLOC_ALIGN(m->finfo.line_length, 64) * m->info.yres;

#ifdef MSTAR_GRALLOC_AFBC
    if (type == AFBC)
    {
        /* calculate buffer size for AFBC */
        int pixel_size = m->info.bits_per_pixel >> 3;
        int w_aligned = GRALLOC_ALIGN(m->info.xres, AFBC_PIXELS_PER_BLOCK);
        int h_aligned = GRALLOC_ALIGN(m->info.yres, AFBC_PIXELS_PER_BLOCK);
        int nblocks = (w_aligned / AFBC_PIXELS_PER_BLOCK) * (h_aligned / AFBC_PIXELS_PER_BLOCK);
        *(const_cast<size_t*>(&framebufferSize)) = w_aligned * h_aligned * pixel_size + GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT);
        *(const_cast<size_t*>(&alignedFramebufferSize)) = GRALLOC_ALIGN(framebufferSize, PAGE_SIZE);
    }
#endif

    *stride = m->info.xres;

    if (numBuffers == 1)
    {
        // If we have only one buffer, we never use page-flipping. Instead,
        // we return a regular buffer which will be memcpy'ed to the main
        // screen when post is called.
        int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
        AWAR( "fallback to single buffering. Virtual Y-res too small %d", m->info.yres );
        *byte_stride = GRALLOC_ALIGN(m->finfo.line_length, 64);
        return alloc_backend_alloc(dev, alignedFramebufferSize, newUsage, pHandle, 0, 0, 0);
    }

    if (bufferMask >= ((1LU<<numBuffers)-1))
    {
        // We ran out of buffers.
        return -ENOMEM;
    }

    uintptr_t framebufferVaddr = (uintptr_t)m->framebuffer->base;
    // find a free slot
    for (uint32_t i=0 ; i<numBuffers ; i++)
    {
        if ((bufferMask & (1LU<<i)) == 0)
        {
            m->bufferMask |= (1LU<<i);
            break;
        }
#ifdef MSTAR_GRALLOC
        framebufferVaddr += alignedFramebufferSize;
#else
        framebufferVaddr += framebufferSize;
#endif
    }

    // The entire framebuffer memory is already mapped, now create a buffer object for parts of this memory
    // MStar: we don't have a real fbdev. hw_base is used to create handles
#ifdef MSTAR_FAKE_FBDEV
    private_handle_t* hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, usage, alignedFramebufferSize,
            (void*)framebufferVaddr, 0, m->framebuffer->hw_base,
            (framebufferVaddr - (uintptr_t)m->framebuffer->base));
#else
    private_handle_t* hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER, usage, size,
            (void*)framebufferVaddr, 0, dup(m->framebuffer->fd),
            (framebufferVaddr - (uintptr_t)m->framebuffer->base), 0);
#endif

    /*
     * Perform allocator specific actions. If these fail we fall back to a regular buffer
     * which will be memcpy'ed to the main screen when fb_post is called.
     */
    if (alloc_backend_alloc_framebuffer(m, hnd) == -1)
    {
        delete hnd;
        int newUsage = (usage & ~GRALLOC_USAGE_HW_FB) | GRALLOC_USAGE_HW_2D;
        AERR( "Fallback to single buffering. Unable to map framebuffer memory to handle:%p", hnd );
        *byte_stride = GRALLOC_ALIGN(m->finfo.line_length, 64);
        return alloc_backend_alloc(dev, alignedFramebufferSize, newUsage, pHandle, 0, 0, 0);
    }

    *pHandle = hnd;
    *byte_stride = m->finfo.line_length;

    return 0;
}

#ifdef MSTAR_GRALLOC_AFBC
static int gralloc_alloc_framebuffer(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, int* stride, int* byte_stride, AllocType type)
#else
static int gralloc_alloc_framebuffer(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, int* stride, int* byte_stride)
#endif
{
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
    pthread_mutex_lock(&m->lock);
#ifdef MSTAR_GRALLOC_AFBC
    int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle, stride, byte_stride, type);
#else
    int err = gralloc_alloc_framebuffer_locked(dev, size, usage, pHandle, stride, byte_stride);
#endif
    pthread_mutex_unlock(&m->lock);
    return err;
}

#ifndef MSTAR_GRALLOC_AFBC
/*
 * Type of allocation
 */
enum AllocType
{
    UNCOMPRESSED = 0,
    AFBC,
    /* AFBC_WIDEBLK mode requires buffer to have 32 * 16 pixels alignment */
    AFBC_WIDEBLK,
    /* AN AFBC buffer with additional padding to ensure a 64-bte alignment
     * for each row of blocks in the header */
    AFBC_PADDED
};
#endif

/*
 * Computes the strides and size for an RGB buffer
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 * pixel_size          size of one pixel in bytes
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * type         (in)   if buffer should be allocated for afbc
 */
static void get_rgb_stride_and_size(int width, int height, int pixel_size,
                                    int* pixel_stride, int* byte_stride, size_t* size, AllocType type)
{
    int stride;

    // MStar: GOP pitch needs 2-pixel alignment
#ifdef MSTAR_GRALLOC
    width = GRALLOC_ALIGN(width, 8);
#endif

    stride = width * pixel_size;

    /* Align the lines to 64 bytes.
     * It's more efficient to write to 64-byte aligned addresses because it's the burst size on the bus */
    stride = GRALLOC_ALIGN(stride, 64);

    if (size != NULL)
    {
        *size = stride * height;
    }

    if (byte_stride != NULL)
    {
        *byte_stride = stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = stride / pixel_size;
    }

    if (type != UNCOMPRESSED)
    {
        int w_aligned;
        int h_aligned = GRALLOC_ALIGN( height, AFBC_PIXELS_PER_BLOCK );
        int nblocks;

        if (type == AFBC_PADDED)
        {
            w_aligned = GRALLOC_ALIGN( width, 64 );
        }
        else if (type == AFBC_WIDEBLK)
        {
            w_aligned = GRALLOC_ALIGN( width, AFBC_WIDEBLK_WIDTH_ALIGN );
        }
        else
        {
            w_aligned = GRALLOC_ALIGN( width, AFBC_PIXELS_PER_BLOCK );
        }

        nblocks = w_aligned / AFBC_PIXELS_PER_BLOCK * h_aligned / AFBC_PIXELS_PER_BLOCK;

        if ( size != NULL )
        {
            *size = w_aligned * h_aligned * pixel_size +
                    GRALLOC_ALIGN( nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT );
        }
    }
}

/*
 * Computes the strides and size for an AFBC 8BIT YUV 4:2:0 buffer
 *
 * width                Public known width of the buffer in pixels
 * height               Public known height of the buffer in pixels
 *
 * pixel_stride   (out) stride of the buffer in pixels
 * byte_stride    (out) stride of the buffer in bytes
 * size           (out) size of the buffer in bytes
 * type                 if buffer should be allocated for a certain afbc type
 * internalHeight (out) The internal height, which may be greater than the public known height.
 */
static bool get_afbc_yuv420_8bit_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride,
                                                 size_t* size, AllocType type, int *internalHeight)
{
    int yuv420_afbc_luma_stride, yuv420_afbc_chroma_stride;

    *internalHeight = height;

    if (type == UNCOMPRESSED)
    {
        AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV420_8BIT_AFBC!");
        return false;
    }

    if (type == AFBC_PADDED)
    {
        AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
        return false;
    }

    if (type == AFBC_WIDEBLK)
    {
        width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
    }
    else
    {
        width = GRALLOC_ALIGN(width, AFBC_PIXELS_PER_BLOCK);
    }

#if AFBC_YUV420_EXTRA_MB_ROW_NEEDED
    /* If we have a greater internal height than public we set the internalHeight. This
     * implies that cropping will be applied of internal dimensions to fit the public one. */
    *internalHeight += AFBC_PIXELS_PER_BLOCK;
#endif

    /* The actual height used in size calculation must include the possible extra row. But
     * it must also be AFBC-aligned. Only the extra row-padding should be reported back in
     * internalHeight. This as only this row needs to be considered when cropping. */
    height = GRALLOC_ALIGN( *internalHeight, AFBC_PIXELS_PER_BLOCK );

    yuv420_afbc_luma_stride = width;
    yuv420_afbc_chroma_stride = GRALLOC_ALIGN(yuv420_afbc_luma_stride / 2, 16); /* Horizontal downsampling*/

    if (size != NULL)
    {
        int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
        /* Simplification of (height * luma-stride + 2 * (height /2 * chroma_stride) */
        *size =
            ( yuv420_afbc_luma_stride + yuv420_afbc_chroma_stride ) * height +
            GRALLOC_ALIGN( nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT );
    }

    if (byte_stride != NULL)
    {
        *byte_stride = yuv420_afbc_luma_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = yuv420_afbc_luma_stride;
    }

    return true;
}

/*
 * Computes the strides and size for an YV12 buffer
 *
 * width                  Public known width of the buffer in pixels
 * height                 Public known height of the buffer in pixels
 *
 * pixel_stride     (out) stride of the buffer in pixels
 * byte_stride      (out) stride of the buffer in bytes
 * size             (out) size of the buffer in bytes
 * type             (in)  if buffer should be allocated for a certain afbc type
 * internalHeight   (out) The internal height, which may be greater than the public known height.
 * stride_alignment (in)  stride aligment value in bytes.
 */
static bool get_yv12_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size,
                                     AllocType type, int* internalHeight, int stride_alignment)
{
    int luma_stride;

    if (type != UNCOMPRESSED)
    {
        return get_afbc_yuv420_8bit_stride_and_size(width, height, pixel_stride, byte_stride, size, type, internalHeight);
    }

    /* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
     * Width will be even stride aligned anyway so just adjust height here for size calculation. */
    height = GRALLOC_ALIGN(height, 2);

    luma_stride = GRALLOC_ALIGN(width, stride_alignment);

    if (size != NULL)
    {
        int chroma_stride = GRALLOC_ALIGN(luma_stride / 2, stride_alignment);
        /* Simplification of ((height * luma_stride ) + 2 * ((height / 2) * chroma_stride)). */
        // MStar: the height of NV12 or NV21 is required to be 16-pixel aligned for Utgard
#ifdef MSTAR_GRALLOC
        *size = GRALLOC_ALIGN(height, 16)  * (luma_stride + chroma_stride);
#else
        *size = height * (luma_stride + chroma_stride);
#endif
    }

    if (byte_stride != NULL)
    {
        *byte_stride = luma_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = luma_stride;
    }

    return true;
}
/*
 * Computes the strides and size for an 8 bit YUYV 422 buffer
 *
 * width                  Public known width of the buffer in pixels
 * height                 Public known height of the buffer in pixels
 *
 * pixel_stride     (out) stride of the buffer in pixels
 * byte_stride      (out) stride of the buffer in bytes
 * size             (out) size of the buffer in bytes
 */
static bool get_yuv422_8bit_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size)
{
    int local_byte_stride, local_pixel_stride;

    /* 4:2:2 formats must have buffers with even width as the clump size is 2x1 pixels.
     * This is taken care of by the even stride alignment. */

    local_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
    local_byte_stride  = GRALLOC_ALIGN(width * 2, YUV_MALI_PLANE_ALIGN); /* 4 bytes per 2 pixels */

    if (size != NULL)
    {
        *size = local_byte_stride * height;
    }

    if (byte_stride != NULL)
    {
        *byte_stride = local_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = local_pixel_stride;
    }

    return true;
}

/*
 * Computes the strides and size for an AFBC 8BIT YUV 4:2:2 buffer
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * type                if buffer should be allocated for a certain afbc type
 */
static bool get_afbc_yuv422_8bit_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size, AllocType type)
{
    int yuv422_afbc_luma_stride;

    if (type == UNCOMPRESSED)
    {
        AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV422_8BIT_AFBC!");
        return false;
    }

    if (type == AFBC_PADDED)
    {
        AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
        return false;
    }

    if (type == AFBC_WIDEBLK)
    {
        width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
    }
    else
    {
        width = GRALLOC_ALIGN(width, AFBC_PIXELS_PER_BLOCK);
    }
    height = GRALLOC_ALIGN(height, AFBC_PIXELS_PER_BLOCK);

    yuv422_afbc_luma_stride = width;

    if (size != NULL)
    {
        int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
        /* YUV 4:2:2 luma size equals chroma size */
        *size = yuv422_afbc_luma_stride * height * 2
            + GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT);
    }

    if (byte_stride != NULL)
    {
        *byte_stride = yuv422_afbc_luma_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = yuv422_afbc_luma_stride;
    }

    return true;
}

/*
 * Calculate strides and sizes for a P010 (Y-UV 4:2:0) or P210 (Y-UV 4:2:2) buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param vss           [in]    Vertical sub-sampling factor (2 for P010, 1 for
 *                              P210. Anything else is invalid).
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv_pX10_stride_and_size(int width, int height, int vss, int* pixel_stride, int* byte_stride, size_t* size)
{
    int luma_pixel_stride, luma_byte_stride;

    if (vss < 1 || vss > 2)
    {
        AERR("Invalid vertical sub-sampling factor: %d, should be 1 or 2", vss);
        return false;
    }

    /* 4:2:2 must have even width as the clump size is 2x1 pixels. This will be taken care of by the
     * even stride alignment */
    if (vss == 2)
    {
        /* 4:2:0 must also have even height as the clump size is 2x2 */
        height = GRALLOC_ALIGN(height, 2);
    }

    luma_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
    luma_byte_stride  = GRALLOC_ALIGN(width * 2, YUV_MALI_PLANE_ALIGN);

    if (size != NULL)
    {
        int chroma_size = GRALLOC_ALIGN(width * 2, YUV_MALI_PLANE_ALIGN) * (height / vss);
        *size = luma_byte_stride * height + chroma_size;
    }

    if (byte_stride != NULL)
    {
        *byte_stride = luma_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = luma_pixel_stride;
    }

    return true;
}

/*
 *  Calculate strides and strides for Y210 (10 bit YUYV packed, 4:2:2) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv_y210_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size)
{
    int y210_byte_stride, y210_pixel_stride;

    /* 4:2:2 formats must have buffers with even width as the clump size is 2x1 pixels.
     * This is taken care of by the even stride alignment */

    y210_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
    /* 4x16 bits per 2 pixels */
    y210_byte_stride  = GRALLOC_ALIGN(width * 4, YUV_MALI_PLANE_ALIGN);

    if (size != NULL)
    {
        *size = y210_byte_stride * height;
    }

    if (byte_stride != NULL)
    {
        *byte_stride = y210_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = y210_pixel_stride;
    }

    return true;
}

/*
 *  Calculate strides and strides for Y0L2 (YUYAAYVYAA, 4:2:0) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 *
 * @note Each YUYAAYVYAA clump encodes a 2x2 area of pixels. YU&V are 10 bits. A is 1 bit. total 8 bytes
 *
 */
static bool get_yuv_y0l2_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size)
{
    int y0l2_byte_stride, y0l2_pixel_stride;

    /* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
     * Width is take care of by the even stride alignment so just adjust height here for size calculation. */
    height = GRALLOC_ALIGN(height, 2);

    y0l2_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
    y0l2_byte_stride  = GRALLOC_ALIGN(width * 4, YUV_MALI_PLANE_ALIGN); /* 2 horiz pixels per 8 byte clump */

    if (size != NULL)
    {
        *size = y0l2_byte_stride * height / 2; /* byte stride covers 2 vert pixels */
    }

    if (byte_stride != NULL)
    {
        *byte_stride = y0l2_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = y0l2_pixel_stride;
    }
    return true;
}
/*
 *  Calculate strides and strides for Y410 (AVYU packed, 4:4:4) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv_y410_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size)
{
    int y410_byte_stride, y410_pixel_stride;

    y410_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
    y410_byte_stride  = GRALLOC_ALIGN(width * 4, YUV_MALI_PLANE_ALIGN);

    if (size != NULL)
    {
        /* 4x8bits per pixel */
        *size = y410_byte_stride * height;
    }

    if (byte_stride != NULL)
    {
        *byte_stride = y410_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = y410_pixel_stride;
    }
    return true;
}

/*
 *  Calculate strides and strides for YUV420_10BIT_AFBC (Compressed, 4:2:0) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 * @param type          [in]    afbc mode that buffer should be allocated with.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv420_10bit_afbc_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size, AllocType type)
{
    int yuv420_afbc_byte_stride, yuv420_afbc_pixel_stride;

    if (width & 3)
    {
        return false;
    }

    if (type == UNCOMPRESSED)
    {
        AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV420_10BIT_AFBC!");
        return false;
    }

    if (type == AFBC_PADDED)
    {
        AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
        return false;
    }

    if (type == AFBC_WIDEBLK)
    {
        width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
    }
    else
    {
        width = GRALLOC_ALIGN(width, AFBC_PIXELS_PER_BLOCK);
    }
    height = GRALLOC_ALIGN(height/2, AFBC_PIXELS_PER_BLOCK); /* vertically downsampled */

    yuv420_afbc_pixel_stride = GRALLOC_ALIGN(width, 16);
    yuv420_afbc_byte_stride  = GRALLOC_ALIGN(width * 4, 16); /* 64-bit packed and horizontally downsampled */

    if (size != NULL)
    {
        int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
        *size = yuv420_afbc_byte_stride * height
            + GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT);
    }

    if (byte_stride != NULL)
    {
        *byte_stride = yuv420_afbc_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = yuv420_afbc_pixel_stride;
    }

    return true;
}

/*
 *  Calculate strides and strides for YUV422_10BIT_AFBC (Compressed, 4:2:2) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 * @param type          [in]    afbc mode that buffer should be allocated with.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv422_10bit_afbc_stride_and_size(int width, int height, int* pixel_stride, int* byte_stride, size_t* size, AllocType type)
{
    int yuv422_afbc_byte_stride, yuv422_afbc_pixel_stride;

    if (width & 3)
    {
        return false;
    }

    if (type == UNCOMPRESSED)
    {
        AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV422_10BIT_AFBC!");
        return false;
    }

    if (type == AFBC_PADDED)
    {
        AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
        return false;
    }

    if (type == AFBC_WIDEBLK)
    {
        width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
    }
    else
    {
        width = GRALLOC_ALIGN(width, AFBC_PIXELS_PER_BLOCK);
    }
    height = GRALLOC_ALIGN(height, AFBC_PIXELS_PER_BLOCK); /* total number of rows must be even number */

    yuv422_afbc_pixel_stride = GRALLOC_ALIGN(width, 16);
    yuv422_afbc_byte_stride  = GRALLOC_ALIGN(width * 2, 16);

    if (size != NULL)
    {
        int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
        /* YUV 4:2:2 chroma size equals to luma size */
        *size = yuv422_afbc_byte_stride * height * 2
            + GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT);
    }

    if (byte_stride != NULL)
    {
        *byte_stride = yuv422_afbc_byte_stride;
    }

    if (pixel_stride != NULL)
    {
        *pixel_stride = yuv422_afbc_pixel_stride;
    }

    return true;
}

#if PLATFORM_SDK_VERSION >= 23
/*
 *  Calculate strides and strides for Camera RAW and Blob formats
 *
 * @param w             [in]    Buffer width.
 * @param h             [in]    Buffer height.
 * @param format        [in]    Requested HAL format
 * @param out_stride    [out]   Pixel stride; number of pixels/bytes between
 *                              consecutive rows. Format description calls for
 *                              either bytes or pixels.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_camera_formats_stride_and_size(int w, int h, uint64_t format, int *out_stride, size_t *out_size)
{
    int stride, size;

    switch (format)
    {
        case HAL_PIXEL_FORMAT_RAW16:
            stride = w; /* Format assumes stride in pixels */
            stride = GRALLOC_ALIGN(stride, 16); /* Alignment mandated by Android */
            size = stride * h * 2; /* 2 bytes per pixel */
            break;

        case HAL_PIXEL_FORMAT_RAW12:
            if (w % 4 != 0)
            {
                ALOGE("ERROR: Width for HAL_PIXEL_FORMAT_RAW12 buffers has to be multiple of 4.");
                return false;
            }
            stride = (w / 2) * 3; /* Stride in bytes; 2 pixels in 3 bytes */
            size = stride * h;
            break;

        case HAL_PIXEL_FORMAT_RAW10:
            if (w % 4 != 0)
            {
                ALOGE("ERROR: Width for HAL_PIXEL_FORMAT_RAW10 buffers has to be multiple of 4.");
                return false;
            }
            stride = (w / 4) * 5; /* Stride in bytes; 4 pixels in 5 bytes */
            size = stride * h;
            break;

        case HAL_PIXEL_FORMAT_BLOB:
            if (h != 1)
            {
                ALOGE("ERROR: Height for HAL_PIXEL_FORMAT_BLOB must be 1.");
                return false;
            }
            stride = 0; /* No 'rows', it's effectively a long one dimensional array */
            size = w;
            break;

        default:
            return false;

    }

    if (out_size != NULL)
    {
        *out_size = size;
    }

    if (out_stride != NULL)
    {
        *out_stride = stride;
    }

    return true;
}

#endif /* PLATFORM_SDK_VERSION >= 23 */

static int alloc_device_alloc(alloc_device_t* dev, int w, int h, int format, int usage, buffer_handle_t* pHandle, int* pStride)
{

    if (!pHandle || !pStride)
    {
        return -EINVAL;
    }

    size_t size;       // Size to be allocated for the buffer
    int byte_stride;   // Stride of the buffer in bytes
    int pixel_stride;  // Stride of the buffer in pixels - as returned in pStride
    uint64_t internal_format;
    AllocType type = UNCOMPRESSED;
    bool alloc_for_extended_yuv = false, alloc_for_arm_afbc_yuv = false;
    int internalWidth,internalHeight;

#ifndef MSTAR_FAKE_FBDEV
#if defined(GRALLOC_FB_SWAP_RED_BLUE)
    /* match the framebuffer format */
    if (usage & GRALLOC_USAGE_HW_FB)
    {
#ifdef GRALLOC_16_BITS
        format = HAL_PIXEL_FORMAT_RGB_565;
#else
        format = HAL_PIXEL_FORMAT_BGRA_8888;
#endif
    }
#endif
#endif

    /* Some formats require an internal width and height that may be used by
     * consumers/producers.
     */
    internalWidth = w;
    internalHeight = h;

    internal_format = gralloc_select_format(format, usage, w*h);

    alloc_for_extended_yuv = (internal_format & GRALLOC_ARM_INTFMT_EXTENDED_YUV) == GRALLOC_ARM_INTFMT_EXTENDED_YUV;
    alloc_for_arm_afbc_yuv = (internal_format & GRALLOC_ARM_INTFMT_ARM_AFBC_YUV) == GRALLOC_ARM_INTFMT_ARM_AFBC_YUV;

    if (internal_format & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK))
    {
        // MStar: GRALLOC_USAGE_PRIVATE_2 is used for overlays
#ifdef MSTAR_GRALLOC_USAGE_PRIVATE
        if (0)
#else
        if (usage & GRALLOC_USAGE_PRIVATE_2)
#endif
        {
            type = AFBC_PADDED;
        }
        else if (internal_format & GRALLOC_ARM_INTFMT_AFBC_WIDEBLK)
        {
#if 1 != MALI_USE_YUV_AFBC_WIDEBLK
            if (alloc_for_arm_afbc_yuv)
            {
                ALOGE("Unsupported format YUV AFBC WIDEBLK.");
                return -EINVAL;
            }
#endif
            type = AFBC_WIDEBLK;
        }
        else
        {
            type = AFBC;
        }
    }

    /* Map format if necessary (also removes internal format extension bits) */
    uint64_t mapped_format = map_format(internal_format);

    if (!alloc_for_extended_yuv && !alloc_for_arm_afbc_yuv)
    {
        switch (mapped_format)
        {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_BGRA_8888:
#if (PLATFORM_SDK_VERSION >= 19) && (PLATFORM_SDK_VERSION <= 22)
            case HAL_PIXEL_FORMAT_sRGB_A_8888:
            case HAL_PIXEL_FORMAT_sRGB_X_8888:
#endif
                get_rgb_stride_and_size(w, h, 4, &pixel_stride, &byte_stride, &size, type );
                break;
            case HAL_PIXEL_FORMAT_RGB_888:
                get_rgb_stride_and_size(w, h, 3, &pixel_stride, &byte_stride, &size, type );
                break;
            case HAL_PIXEL_FORMAT_RGB_565:
#if PLATFORM_SDK_VERSION < 19
            case HAL_PIXEL_FORMAT_RGBA_5551:
            case HAL_PIXEL_FORMAT_RGBA_4444:
#endif
                get_rgb_stride_and_size(w, h, 2, &pixel_stride, &byte_stride, &size, type );
                break;

#ifdef MSTAR_GRALLOC_FORMAT
            case HAL_PIXEL_FORMAT_YCbCr_420_SP:
#endif
            case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            case HAL_PIXEL_FORMAT_YV12:
            case GRALLOC_ARM_HAL_FORMAT_INDEXED_NV12:
            case GRALLOC_ARM_HAL_FORMAT_INDEXED_NV21:
            {
                /* Mali subsystem prefers higher stride alignment values (128 bytes) for YUV, but software components assume
                 * default of 16. We only need to care about YV12 as it's the only, implicit, HAL YUV format in Android.
                 */
                int yv12_align = YUV_MALI_PLANE_ALIGN;
                if(usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
                {
                    yv12_align = YUV_ANDROID_PLANE_ALIGN;
                }

                if (!get_yv12_stride_and_size(w, h, &pixel_stride, &byte_stride, &size, type,
                                              &internalHeight, yv12_align))
                {
                    return -EINVAL;
                }
                break;
            }
            case HAL_PIXEL_FORMAT_YCbCr_422_I:
            {
                /* YUYV 4:2:2 */
                if (!get_yuv422_8bit_stride_and_size(w, h, &pixel_stride, &byte_stride, &size))
                {
                    return -EINVAL;
                }
                break;
            }
#if PLATFORM_SDK_VERSION >= 23
            case HAL_PIXEL_FORMAT_RAW16:
            case HAL_PIXEL_FORMAT_RAW12:
            case HAL_PIXEL_FORMAT_RAW10:
            case HAL_PIXEL_FORMAT_BLOB:
                get_camera_formats_stride_and_size(w, h, mapped_format, &pixel_stride, &size);
                byte_stride = pixel_stride; /* For Raw/Blob formats stride is defined to be either in bytes or pixels per format */
                break;
#endif /*  PLATFORM_SDK_VERSION >= 23 */

                /*
                 * Additional custom formats can be added here
                 * and must fill the variables pixel_stride, byte_stride and size.
                 */
            default:
                return -EINVAL;
        }
    }
    else
    {
        switch (mapped_format)
        {
            case GRALLOC_ARM_HAL_FORMAT_INDEXED_Y0L2:
                /* YUYAAYUVAA 4:2:0 */
                if (!get_yuv_y0l2_stride_and_size(w, h, &pixel_stride, &byte_stride, &size))
                {
                    return -EINVAL;
                }
                break;

            case GRALLOC_ARM_HAL_FORMAT_INDEXED_P010:
                /* Y-UV 4:2:0 */
                if (!get_yuv_pX10_stride_and_size(w, h, 2, &pixel_stride, &byte_stride, &size))
                {
                    return -EINVAL;
                }
                break;

            case GRALLOC_ARM_HAL_FORMAT_INDEXED_P210:
                /* Y-UV 4:2:2 */
                if (!get_yuv_pX10_stride_and_size(w, h, 1, &pixel_stride, &byte_stride, &size))
                {
                    return -EINVAL;
                }
                break;

            case GRALLOC_ARM_HAL_FORMAT_INDEXED_Y210:
                /* YUYV 4:2:2 */
                if (!get_yuv_y210_stride_and_size(w, h, &pixel_stride, &byte_stride, &size))
                {
                    return -EINVAL;
                }
                break;

            case GRALLOC_ARM_HAL_FORMAT_INDEXED_Y410:
                /* AVYU 2-10-10-10 */
                if (!get_yuv_y410_stride_and_size(w, h, &pixel_stride, &byte_stride, &size))
                {
                    return -EINVAL;
                }
                break;
                /* 8BIT AFBC YUV 4:2:0 testing usage */
            case GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_8BIT_AFBC:
                if (!get_afbc_yuv420_8bit_stride_and_size(w, h, &pixel_stride, &byte_stride, &size, type, &internalHeight))
                {
                    return -EINVAL;
                }
                break;

                /* 8BIT AFBC YUV4:2:2 testing usage */
            case GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV422_8BIT_AFBC:
                if (!get_afbc_yuv422_8bit_stride_and_size(w, h, &pixel_stride, &byte_stride, &size, type))
                {
                    return -EINVAL;
                }
                break;

            case GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV420_10BIT_AFBC:
                /* YUV 4:2:0 compressed */
                if (!get_yuv420_10bit_afbc_stride_and_size(w, h, &pixel_stride, &byte_stride, &size, type))
                {
                    return -EINVAL;
                }
                break;
            case GRALLOC_ARM_HAL_FORMAT_INDEXED_YUV422_10BIT_AFBC:
                /* YUV 4:2:2 compressed */
                if (!get_yuv422_10bit_afbc_stride_and_size(w, h, &pixel_stride, &byte_stride, &size, type))
                {
                    return -EINVAL;
                }
                break;
            default:
                AERR("Invalid internal format %llx", internal_format & GRALLOC_ARM_INTFMT_FMT_MASK);
                return -EINVAL;
        }
    }

    int err;
#if DISABLE_FRAMEBUFFER_HAL != 1
    if (usage & GRALLOC_USAGE_HW_FB)
    {
#ifdef MSTAR_GRALLOC
        if (usage & GRALLOC_USAGE_ION_MASK)
        {
            err = alloc_backend_alloc(dev, size, usage, pHandle, internal_format, w, h);
        }
        else
        {
#endif
#ifdef MSTAR_GRALLOC_AFBC
        err = gralloc_alloc_framebuffer(dev, size, usage, pHandle, &pixel_stride, &byte_stride, type);
        pixel_stride = w;
        private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
        int pixel_size = m->info.bits_per_pixel >> 3;
        byte_stride = pixel_stride * pixel_size;
#else
        err = gralloc_alloc_framebuffer(dev, size, usage, pHandle, &pixel_stride, &byte_stride);
#endif
#ifdef MSTAR_GRALLOC
        }
#endif
    }
    else
#endif
    {
        err = alloc_backend_alloc(dev, size, usage, pHandle, internal_format, w, h);
    }

    if (err < 0)
    {
        return err;
    }

    private_handle_t *hnd = (private_handle_t *)*pHandle;

#if MALI_AFBC_GRALLOC == 1
    err = gralloc_buffer_attr_allocate( hnd );
    if( err < 0 )
    {
        private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

        if ( (usage & GRALLOC_USAGE_HW_FB) )
        {
            /*
             * Having the attribute region is not critical for the framebuffer so let it pass.
             */
            err = 0;
        }
        else
        {
            alloc_backend_alloc_free( hnd, m );
            return err;
        }
    }
#endif

    hnd->req_format = format;
    hnd->byte_stride = byte_stride;
    hnd->internal_format = internal_format;

    // MStar: private usage flags 0 and 1 are used for overlays
#ifndef MSTAR_GRALLOC_USAGE_PRIVATE
    int private_usage = usage & (GRALLOC_USAGE_PRIVATE_0 |
                                 GRALLOC_USAGE_PRIVATE_1);
    switch (private_usage)
    {
        case 0:
            hnd->yuv_info = MALI_YUV_BT601_NARROW;
            break;
        case GRALLOC_USAGE_PRIVATE_1:
            hnd->yuv_info = MALI_YUV_BT601_WIDE;
            break;
        case GRALLOC_USAGE_PRIVATE_0:
            hnd->yuv_info = MALI_YUV_BT709_NARROW;
            break;
        case (GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1):
            hnd->yuv_info = MALI_YUV_BT709_WIDE;
            break;
    }
#endif

    hnd->width = w;
    hnd->height = h;
    hnd->stride = pixel_stride;
    hnd->internalWidth = internalWidth;
    hnd->internalHeight = internalHeight;

    *pStride = pixel_stride;
    return 0;
}

static int alloc_device_free(alloc_device_t* dev, buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
    {
        return -EINVAL;
    }

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);

    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
    {
        // free this buffer
        private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
#ifdef MSTAR_GRALLOC
        const size_t bufferSize = hnd->size;
#else
        const size_t bufferSize = m->finfo.line_length * m->info.yres;
#endif
        int index = ((uintptr_t)hnd->base - (uintptr_t)m->framebuffer->base) / bufferSize;
        m->bufferMask &= ~(1 << index);
#ifndef MSTAR_FAKE_FBDEV
        close(hnd->fd);
#endif
    }

#if MALI_AFBC_GRALLOC == 1
    gralloc_buffer_attr_free( (private_handle_t *) hnd );
#endif
    alloc_backend_alloc_free(hnd, m);

    delete hnd;

    return 0;
}

int alloc_device_open(hw_module_t const* module, const char* name, hw_device_t** device)
{
    alloc_device_t *dev;

    dev = new alloc_device_t;
    if (NULL == dev)
    {
        return -1;
    }

    /* initialize our state here */
    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = const_cast<hw_module_t*>(module);
    dev->common.close = alloc_backend_close;
    dev->alloc = alloc_device_alloc;
    dev->free = alloc_device_free;

    if (0 != alloc_backend_open(dev)) {
        delete dev;
        return -1;
    }

    *device = &dev->common;

    return 0;
}
