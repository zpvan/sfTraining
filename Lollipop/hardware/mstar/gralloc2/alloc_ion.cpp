/*
 * Copyright (C) 2013 ARM Limited. All rights reserved.
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

#include "format_chooser.h"

#ifdef MSTAR_GRALLOC
#include "ion_export.h"
#else
#include <linux/ion.h>
#include <ion/ion.h>
#endif

#ifdef TARGET_BOARD_PLATFORM_MUJI
#include <stdlib.h>
#include <cutils/properties.h>
#endif

#ifdef MSTAR_GRALLOC_CMA
#include <MsOS.h>

#define ION_CMA_MALI_MIU0_HEAP_MASK     (1 << ION_MALI_MIU0_HEAP_ID)
#define ION_CMA_MALI_MIU1_HEAP_MASK     (1 << ION_MALI_MIU1_HEAP_ID)
#define ION_CMA_MALI_MIU2_HEAP_MASK     (1 << ION_MALI_MIU2_HEAP_ID)

#define GRALLOC_USAGE_ION_CMA_MIU_MASK  (GRALLOC_USAGE_ION_CMA_MIU0 | GRALLOC_USAGE_ION_CMA_MIU1 | GRALLOC_USAGE_ION_CMA_MIU2)
#define IS_CMA_HEAP_MASK(mask)          ((mask) & (ION_CMA_MALI_MIU0_HEAP_MASK | ION_CMA_MALI_MIU1_HEAP_MASK | ION_CMA_MALI_MIU2_HEAP_MASK))

#define ALIGNSIZE 4096

static mem_type_info usage_to_mem_type(int usage)
{
    mem_type_info mem_type = mem_type_system_heap;

    switch (usage)
    {
    case GRALLOC_USAGE_ION_CMA_MIU0 | GRALLOC_USAGE_ION_CMA_CONTIGUOUS:
        mem_type = mem_type_cma_contiguous_miu0;
        break;
    case GRALLOC_USAGE_ION_CMA_MIU0 | GRALLOC_USAGE_ION_CMA_DISCRETE:
        mem_type = mem_type_cma_discrete_miu0;
        break;
    case GRALLOC_USAGE_ION_CMA_MIU1 | GRALLOC_USAGE_ION_CMA_CONTIGUOUS:
        mem_type = mem_type_cma_contiguous_miu1;
        break;
    case GRALLOC_USAGE_ION_CMA_MIU1 | GRALLOC_USAGE_ION_CMA_DISCRETE:
        mem_type = mem_type_cma_discrete_miu1;
        break;
    case GRALLOC_USAGE_ION_CMA_MIU2 | GRALLOC_USAGE_ION_CMA_CONTIGUOUS:
        mem_type = mem_type_cma_contiguous_miu2;
        break;
    case GRALLOC_USAGE_ION_CMA_MIU2 | GRALLOC_USAGE_ION_CMA_DISCRETE:
        mem_type = mem_type_cma_discrete_miu2;
        break;
    case GRALLOC_USAGE_ION_SYSTEM_HEAP:
    default:
        mem_type = mem_type_system_heap;
        break;
    }

    return mem_type;
}

static mem_type_info get_request_mem_type(int usage)
{
    mem_type_info mem_type = mem_type_system_heap;

    if (usage & GRALLOC_USAGE_ION_CMA_MIU_MASK)
    {
        int miu_usage = usage & GRALLOC_USAGE_ION_CMA_MIU_MASK;

        if (usage & GRALLOC_USAGE_ION_CMA_CONTIGUOUS)
        {
            miu_usage |= GRALLOC_USAGE_ION_CMA_CONTIGUOUS;
            mem_type = usage_to_mem_type(miu_usage);
        }
        else if (usage & GRALLOC_USAGE_ION_CMA_DISCRETE)
        {
            miu_usage |= GRALLOC_USAGE_ION_CMA_DISCRETE;
            mem_type = usage_to_mem_type(miu_usage);
        }
        else
        {
            ALOGD("error: no gralloc usage %x", usage);
        }
    }
    else
    {
        mem_type = usage_to_mem_type(GRALLOC_USAGE_ION_SYSTEM_HEAP);
    }

    return mem_type;
}
#endif

static void init_afbc(uint8_t *buf, uint64_t format, int w, int h)
{
    uint32_t n_headers = (w * h) / 64;
    uint32_t body_offset = n_headers * 16;
    uint32_t headers[][4] = { {body_offset, 0x1, 0x0, 0x0}, /* Layouts 0, 3, 4 */
                              {(body_offset + (1 << 28)), 0x200040, 0x4000, 0x80} /* Layouts 1, 5 */
                            };
    int i, layout;

    /* map format if necessary (also removes internal extension bits) */
    uint64_t mapped_format = map_format(format);

    switch (mapped_format)
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_RGB_888:
        case HAL_PIXEL_FORMAT_RGB_565:
        case HAL_PIXEL_FORMAT_BGRA_8888:
#if (PLATFORM_SDK_VERSION >= 19) && (PLATFORM_SDK_VERSION <= 22)
        case HAL_PIXEL_FORMAT_sRGB_A_8888:
        case HAL_PIXEL_FORMAT_sRGB_X_8888:
#endif
            layout = 0;
            break;

        case HAL_PIXEL_FORMAT_YV12:
        case GRALLOC_ARM_HAL_FORMAT_INDEXED_NV12:
        case GRALLOC_ARM_HAL_FORMAT_INDEXED_NV21:
            layout = 1;
            break;
        default:
            layout = 0;
    }

    ALOGV("Writing AFBC header layout %d for format %llx", layout, format);

    for (i = 0; i < n_headers; i++)
    {
        memcpy(buf, headers[layout], sizeof(headers[layout]));
        buf += sizeof(headers[layout]);
    }

}

static ion_user_handle_t alloc_from_ion_heap(int ion_fd, size_t size, unsigned int heap_mask,
        unsigned int flags, int *min_pgsz)
{
    ion_user_handle_t ion_hnd = ION_INVALID_HANDLE;
    int ret;

    if ((ion_fd < 0) || (size <= 0) || (heap_mask == 0) || (min_pgsz == NULL))
        return ION_INVALID_HANDLE;

#ifdef MSTAR_GRALLOC_CMA
    if (IS_CMA_HEAP_MASK(heap_mask))
    {
        ret = ion_cust_alloc(ion_fd, 0, size, ALIGNSIZE, heap_mask, flags, &ion_hnd);

        if (ret != 0)
        {
            ion_hnd = ION_INVALID_HANDLE;
        }
    }
    else
    {
#endif
    ret = ion_alloc(ion_fd, size, 0, heap_mask, flags, &ion_hnd);
    if (ret != 0)
    {
        /* If everything else failed try system heap */
        flags = 0; /* Fallback option flags are not longer valid */
        ion_alloc(ion_fd, size, 0, ION_HEAP_SYSTEM_MASK, flags, &ion_hnd);
    }
#ifdef MSTAR_GRALLOC_CMA
    }
#endif

    if (ion_hnd > ION_INVALID_HANDLE)
    {
        switch (heap_mask)
        {
        case ION_HEAP_SYSTEM_MASK:
            *min_pgsz = SZ_4K;
            break;
        case ION_HEAP_SYSTEM_CONTIG_MASK:
        case ION_HEAP_CARVEOUT_MASK:
#ifdef ION_HEAP_TYPE_DMA_MASK
        case ION_HEAP_TYPE_DMA_MASK:
#endif
            *min_pgsz = size;
            break;
#ifdef ION_HEAP_CHUNK_MASK
        /* NOTE: if have this heap make sure your ION chunk size is 2M*/
        case ION_HEAP_CHUNK_MASK:
            *min_pgsz = SZ_2M;
            break;
#endif
#ifdef ION_HEAP_COMPOUND_PAGE_MASK
        case ION_HEAP_COMPOUND_PAGE_MASK:
            *min_pgsz = SZ_2M;
            break;
#endif
        /* If have customized heap please set the suitable pg type according to
         * the customized ION implementation
         */
#ifdef ION_HEAP_CUSTOM_MASK
        case ION_HEAP_CUSTOM_MASK:
            *min_pgsz = SZ_4K;
            break;
#endif
        default:
            *min_pgsz = SZ_4K;
            break;
        }
    }

    return ion_hnd;
}

unsigned int pick_ion_heap(int usage)
{
    unsigned int heap_mask;

    if(usage & GRALLOC_USAGE_PROTECTED)
    {
#if defined(ION_HEAP_SECURE_MASK)
        heap_mask = ION_HEAP_SECURE_MASK;
#else
        AERR("Protected ION memory is not supported on this platform.");
        return -1;
#endif
    }
#if defined(ION_HEAP_TYPE_COMPOUND_PAGE_MASK) && GRALLOC_USE_ION_COMPOUND_PAGE_HEAP
    else if(!(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) && (usage & (GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER)))
    {
        heap_mask = ION_HEAP_TYPE_COMPOUND_PAGE_MASK;
    }
#elif defined(ION_HEAP_TYPE_DMA_MASK) && GRALLOC_USE_ION_DMA_HEAP
    else if(!(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) && (usage & (GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER)))
    {
        heap_mask = ION_HEAP_TYPE_DMA_MASK;
    }
#endif
    else
    {
#ifdef MSTAR_GRALLOC_CMA
        heap_mask = 0;

        if (usage & GRALLOC_USAGE_ION_CMA_MIU0)
        {
            heap_mask |= ION_CMA_MALI_MIU0_HEAP_MASK;
        }

        if (usage & GRALLOC_USAGE_ION_CMA_MIU1)
        {
            heap_mask |= ION_CMA_MALI_MIU1_HEAP_MASK;
        }

        if (usage & GRALLOC_USAGE_ION_CMA_MIU2)
        {
            heap_mask |= ION_CMA_MALI_MIU2_HEAP_MASK;
        }

        if (0 == heap_mask)
        {
            heap_mask = ION_HEAP_SYSTEM_MASK;
        }
        else
        {
            ALOGD("allocate buffer from ION HEAP 0x%x", heap_mask);
        }
#else
        heap_mask = ION_HEAP_SYSTEM_MASK;
#endif
    }

    return heap_mask;
}

void set_ion_flags(unsigned int heap_mask, int usage, unsigned int *priv_heap_flag, int *ion_flags)
{
    if (priv_heap_flag)
    {
#if defined(ION_HEAP_TYPE_DMA_MASK) && GRALLOC_USE_ION_DMA_HEAP
        if (heap_mask == ION_HEAP_TYPE_DMA_MASK)
        {
            *priv_heap_flag = private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP;
        }
#endif
    }

    if (ion_flags)
    {
#if defined(ION_HEAP_TYPE_DMA_MASK) && GRALLOC_USE_ION_DMA_HEAP
        if(heap_mask != ION_HEAP_TYPE_DMA_MASK)
        {
#endif
#ifdef MSTAR_GRALLOC_CMA
            if (IS_CMA_HEAP_MASK(heap_mask))
            {
                *ion_flags |= ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

                // MStar: GRALLOC_USAGE_PRIVATE_2 represents MM buffers.
                //        let MM uses non-cached buffers, so when locking buffers it needn't fluch cache every time
#ifdef MSTAR_GRALLOC_USAGE_PRIVATE
                if (usage & GRALLOC_USAGE_PRIVATE_2)
                {
                    *ion_flags &= ~(ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC);
                }
#endif
            }
            else
            {
#endif
            if ( (usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN )
            {
                *ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

                // MStar: GRALLOC_USAGE_PRIVATE_2 represents MM buffers.
                //        let MM uses non-cached buffers, so when locking buffers it needn't fluch cache every time
#ifdef MSTAR_GRALLOC_USAGE_PRIVATE
                if (usage & GRALLOC_USAGE_PRIVATE_2)
                {
                    *ion_flags &= ~(ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC);
                }
#endif
            }
#ifdef MSTAR_GRALLOC_CMA
            }
#endif
#if defined(ION_HEAP_TYPE_DMA_MASK) && GRALLOC_USE_ION_DMA_HEAP
        }
#endif
    }
}

int alloc_backend_alloc(alloc_device_t* dev, size_t size, int usage, buffer_handle_t* pHandle, uint64_t fmt, int w, int h)
{
    private_module_t* m = reinterpret_cast<private_module_t*>(dev->common.module);
    ion_user_handle_t ion_hnd;
    unsigned char *cpu_ptr = NULL;
    int shared_fd;
    int ret;
    unsigned int heap_mask, priv_heap_flag = 0;
    int ion_flags = 0;
    static int support_protected = 1; /* initially, assume we support protected memory */
    int lock_state = 0;
    int min_pgsz = 0;

#ifdef MSTAR_GRALLOC_CMA
    bool usage_contiguous;
    MS_PHY phy_addr = 0x0;
    unsigned int allocated_mem_type = 0;
    int original_usage = usage;
#endif

#ifdef TARGET_BOARD_PLATFORM_MUJI
    char property[PROPERTY_VALUE_MAX] = {0};
    int miu_location = -1;

    if ((property_get("mstar.maliion.default.location", property, NULL) > 0) && (!(usage & GRALLOC_USAGE_ION_MASK)))
    {
        if (strcmp(property, "contiguous") == 0)
        {
            if (property_get("mstar.maliion.conmem.location", property, NULL) > 0)
            {
                miu_location = atoi(property);

                if (miu_location == 2)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_MIU2 | GRALLOC_USAGE_ION_CMA_CONTIGUOUS;
                }
                else if (miu_location == 1)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_MIU1 | GRALLOC_USAGE_ION_CMA_CONTIGUOUS;
                }
                else if (miu_location == 0)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_MIU0 | GRALLOC_USAGE_ION_CMA_CONTIGUOUS;
                }
            }
        }
        else
        {
            if (property_get("mstar.maliion.dismem.location", property, NULL) > 0)
            {
                miu_location = atoi(property);

                if (miu_location == 2)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_MIU2 | GRALLOC_USAGE_ION_CMA_DISCRETE;
                }
                else if (miu_location == 1)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_MIU1 | GRALLOC_USAGE_ION_CMA_DISCRETE;
                }
                else if (miu_location == 0)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_MIU0 | GRALLOC_USAGE_ION_CMA_DISCRETE;
                }
            }
        }
    }
#endif

#ifdef MSTAR_GRALLOC_CMA
retry:
    usage_contiguous = (usage & GRALLOC_USAGE_ION_CMA_CONTIGUOUS) ? true : false;
    phy_addr = 0x0;
#endif

    heap_mask = pick_ion_heap(usage);
    set_ion_flags(heap_mask, usage, &priv_heap_flag, &ion_flags);
#ifdef MSTAR_GRALLOC_CMA
    if (IS_CMA_HEAP_MASK(heap_mask))
    {
        ion_flags |= usage_contiguous ? ION_FLAG_CONTIGUOUS : ION_FLAG_DISCRETE;
    }
#endif

    ion_hnd = alloc_from_ion_heap(m->ion_client, size, heap_mask, ion_flags, &min_pgsz);
    if (ion_hnd <= ION_INVALID_HANDLE)
    {
#ifdef MSTAR_GRALLOC_CMA
        if (IS_CMA_HEAP_MASK(heap_mask))
        {
            if (usage_contiguous)
            {
                usage &= ~GRALLOC_USAGE_ION_CMA_CONTIGUOUS;

                if (original_usage & GRALLOC_USAGE_ION_CMA_DISCRETE)
                {
                    usage |= GRALLOC_USAGE_ION_CMA_DISCRETE;
                }
            }
            else
            {
                usage &= ~GRALLOC_USAGE_ION_MASK;
            }

            goto retry;
        }
#endif

        AERR("Failed to ion_alloc from ion_client:%d", m->ion_client);
        return -1;
    }

#ifdef MSTAR_GRALLOC_CMA
    if (IS_CMA_HEAP_MASK(heap_mask))
    {
        if (usage_contiguous)
        {
            MS_PHY bus_addr = ion_get_user_data(m->ion_client, ion_hnd);

            if (bus_addr == 0)
            {
                ALOGE("failed to get phys address\n");
                return -1;
            }
            else
            {
                phy_addr = MsOS_BA2PA((MS_U32)bus_addr);
                ALOGD("the contiguous memory bus addr is %llx", (long long unsigned int)bus_addr);
                ALOGD("the contiguous memory phy addr is %llx", (long long unsigned int)phy_addr);
            }
        }

        ret = _ion_map(m->ion_client, ion_hnd, &shared_fd);

        if (ret != 0)
        {
            ALOGE("_ion_map( %d ) failed", m->ion_client);
            return -1;
        }

        if (usage_contiguous)
        {
            allocated_mem_type = usage_to_mem_type(usage & (GRALLOC_USAGE_ION_CMA_MIU_MASK | GRALLOC_USAGE_ION_CMA_CONTIGUOUS));
        }
        else
        {
            allocated_mem_type = usage_to_mem_type(usage & (GRALLOC_USAGE_ION_CMA_MIU_MASK | GRALLOC_USAGE_ION_CMA_DISCRETE));
        }
    }
    else
    {
#endif
    ret = ion_share( m->ion_client, ion_hnd, &shared_fd );
    if ( ret != 0 )
    {
        AERR( "ion_share( %d ) failed", m->ion_client );
        if ( 0 != ion_free( m->ion_client, ion_hnd ) ) AERR( "ion_free( %d ) failed", m->ion_client );
        return -1;
    }

#ifdef MSTAR_GRALLOC_CMA
#if defined(ION_HEAP_SECURE_MASK)
        if (usage & GRALLOC_USAGE_PROTECTED)
        {
            allocated_mem_type = usage_to_mem_type(GRALLOC_USAGE_PROTECTED);
        }
        else
#endif
        {
            allocated_mem_type = usage_to_mem_type(GRALLOC_USAGE_ION_SYSTEM_HEAP);
        }
    }
#endif

    if (!(usage & GRALLOC_USAGE_PROTECTED))
    {
        cpu_ptr = (unsigned char*)mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0 );

        if ( MAP_FAILED == cpu_ptr )
        {
            AERR( "ion_map( %d ) failed", m->ion_client );
            if ( 0 != ion_free( m->ion_client, ion_hnd ) ) AERR( "ion_free( %d ) failed", m->ion_client );
            close( shared_fd );
            return -1;
        }
        lock_state = private_handle_t::LOCK_STATE_MAPPED;

#if GRALLOC_INIT_AFBC == 1
        if (fmt & (GRALLOC_ARM_INTFMT_AFBC | GRALLOC_ARM_INTFMT_AFBC_SPLITBLK | GRALLOC_ARM_INTFMT_AFBC_WIDEBLK))
        {
            init_afbc(cpu_ptr, fmt, w, h);
        }
#endif /* GRALLOC_INIT_AFBC == 1 */
    }

    private_handle_t *hnd = new private_handle_t( private_handle_t::PRIV_FLAGS_USES_ION | priv_heap_flag, usage, size, cpu_ptr,
                                                  lock_state );

    if ( NULL != hnd )
    {
#ifdef MSTAR_GRALLOC_USAGE_PRIVATE
        if (usage & GRALLOC_USAGE_PRIVATE_0)
        {
            hnd->mstaroverlay = private_handle_t::PRIV_FLAGS_MM_OVERLAY;
        }
        else if (usage & GRALLOC_USAGE_PRIVATE_1)
        {
            hnd->mstaroverlay = private_handle_t::PRIV_FLAGS_TV_OVERLAY;
        }
        else if (usage & GRALLOC_USAGE_PRIVATE_2)
        {
            hnd->mstaroverlay = private_handle_t::PRIV_FLAGS_NW_OVERLAY;
        }
        else
        {
            hnd->mstaroverlay = 0;
        }
#endif

        hnd->share_fd = shared_fd;
        hnd->ion_hnd = ion_hnd;
#ifndef MSTAR_GRALLOC
        hnd->min_pgsz = min_pgsz;
#endif
#ifdef MSTAR_GRALLOC_CMA
        hnd->hw_base = phy_addr;
        hnd->memTypeInfo = get_request_mem_type(original_usage) | (allocated_mem_type << 16);
#endif
        *pHandle = hnd;
        return 0;
    }
    else
    {
        AERR( "Gralloc out of mem for ion_client:%d", m->ion_client );
    }

    close( shared_fd );

    if(!(usage & GRALLOC_USAGE_PROTECTED))
    {
        ret = munmap( cpu_ptr, size );
        if ( 0 != ret ) AERR( "munmap failed for base:%p size: %zd", cpu_ptr, size );
    }

    ret = ion_free( m->ion_client, ion_hnd );
    if ( 0 != ret ) AERR( "ion_free( %d ) failed", m->ion_client );
    return -1;
}

int alloc_backend_alloc_framebuffer(private_module_t* m, private_handle_t* hnd)
{
    // MStar: we use the hw_base version of private_handle_t to share framebuffers
#ifdef MSTAR_FAKE_FBDEV
    return 0;
#else
    struct fb_dmabuf_export fb_dma_buf;
    int res;
    res = ioctl( m->framebuffer->fd, FBIOGET_DMABUF, &fb_dma_buf );
    if(res == 0)
    {
        hnd->share_fd = fb_dma_buf.fd;
        return 0;
    }
    else
    {
        AINF("FBIOGET_DMABUF ioctl failed(%d). See gralloc_priv.h and the integration manual for vendor framebuffer integration", res);
#if MALI_ARCHITECTURE_UTGARD
        /* On Utgard we do not have a strict requirement of DMA-BUF integration */
        return 0;
#else
        return -1;
#endif
    }
#endif
}

void alloc_backend_alloc_free(private_handle_t const* hnd, private_module_t* m)
{
    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
    {
#ifdef MSTAR_FAKE_FBDEV
        if (hnd->share_fd >= 0 && m->framebuffer->share_fd < 0)
        {
            close(hnd->share_fd);
        }
#endif

        return;
    }
    else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
    {
        AERR( "Can't free ump memory for handle:%p. Not supported.", hnd );
    }
    else if ( hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION )
    {
        /* Buffer might be unregistered already so we need to assure we have a valid handle*/
        if ( 0 != hnd->base )
        {
            if ( 0 != munmap( (void*)hnd->base, hnd->size ) ) AERR( "Failed to munmap handle %p", hnd );
        }
#ifdef MSTAR_GRALLOC
        if (hnd->share_fd >= 0)
        {
#endif
        close( hnd->share_fd );
#ifdef MSTAR_GRALLOC
        }
#endif
        if ( 0 != ion_free( m->ion_client, hnd->ion_hnd ) ) AERR( "Failed to ion_free( ion_client: %d ion_hnd: %p )", m->ion_client, hnd->ion_hnd );
        memset( (void*)hnd, 0, sizeof( *hnd ) );
    }
}

int alloc_backend_open(alloc_device_t *dev)
{
    private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);
    m->ion_client = ion_open();
    if ( m->ion_client < 0 )
    {
        AERR( "ion_open failed with %s", strerror(errno) );
        return -1;
    }

    return 0;
}

int alloc_backend_close(struct hw_device_t *device)
{
    alloc_device_t* dev = reinterpret_cast<alloc_device_t*>(device);
    if (dev)
    {
        private_module_t *m = reinterpret_cast<private_module_t*>(dev->common.module);
        if ( 0 != ion_close(m->ion_client) ) AERR( "Failed to close ion_client: %d err=%s", m->ion_client , strerror(errno));
        delete dev;
    }
    return 0;
}
