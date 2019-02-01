/*
 *  ion.c
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

//#include <linux/ion.h>
//#include <ion/ion.h>
#include "ion.h"
#include "gralloc_priv.h"

int ion_open()
{
        int fd = open("/dev/ion", O_RDWR);
        if (fd < 0)
                ALOGE("open /dev/ion failed!\n");
        return fd;
}

int ion_close(int fd)
{
        return close(fd);
}

static int ion_ioctl(int fd, int req, void *arg)
{
        int ret = ioctl(fd, req, arg);
        if (ret < 0) {
                ALOGE("ioctl %x failed with code %d: %s\n", req,
                       ret, strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
          unsigned int flags, ion_user_handle_t *handle)
{
        int ret;
        /*
        struct ion_allocation_data data = {
                .len = len,
                .align = align,
                .heap_id_mask = heap_mask,
                .flags = flags,
        };
        */

        ion_allocation_data data ;
        memset(&data,0,sizeof(ion_allocation_data));
        data.len = len;
        data.align = align;
        data.heap_id_mask = heap_mask;
        data.flags = flags;

        ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
        if (ret < 0) {
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
            return ret;
        }
        *handle = data.handle;
        return ret;
}

int ion_cust_alloc(int fd, size_t start, size_t len, size_t align, unsigned int heap_mask,
          unsigned int flags, ion_user_handle_t *handle)
{
        int ret;
        /*
        struct ion_allocation_data data = {
                .len = len,
                .align = align,
                .heap_id_mask = heap_mask,
                .flags = flags,
        };
        */

        ion_cust_allocation_data data ;
        memset(&data,0,sizeof(ion_allocation_data));
        data.start = start;
        data.len = len;
        data.align = align;
        data.heap_id_mask = heap_mask;
        data.flags = flags;

        ret = ion_ioctl(fd, ION_IOC_CUST_ALLOC, &data);
        if (ret < 0) {
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
            return ret;
        }
        *handle = data.handle;
        return ret;
}

int ion_free(int fd, ion_user_handle_t handle)
{

        struct ion_handle_data data = {
                .handle = handle,
        };
        int ret = ion_ioctl(fd, ION_IOC_FREE, &data);
        if (ret < 0)
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
        return ret;
}

int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
/*
        struct ion_fd_data data = {
                .handle = handle,
        };
*/
        ion_fd_data data;
        memset(&data,0,sizeof(ion_fd_data));
        data.handle = handle;

        int ret = ion_ioctl(fd, ION_IOC_MAP, &data);
        if (ret < 0) {
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
            return ret;
        }
        *map_fd = data.fd;
        if (*map_fd < 0) {
                ALOGE("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = (unsigned char *)mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                ALOGE("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_share(int fd, ion_user_handle_t handle, int *share_fd)
{
        int map_fd;
        /*
        struct ion_fd_data data = {
                .handle = handle,
        };
        */
        ion_fd_data data;
        memset(&data,0,sizeof(ion_fd_data));
        data.handle = handle;
        int ret = ion_ioctl(fd, ION_IOC_SHARE, &data);
        if (ret < 0) {
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
            return ret;
        }
        *share_fd = data.fd;
        if (*share_fd < 0) {
                ALOGE("share ioctl returned negative fd\n");
                return -EINVAL;
        }
        return ret;
}

int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
         unsigned int flags, int *handle_fd) {
    ion_user_handle_t handle;
    int ret;

    ret = ion_alloc(fd, len, align, heap_mask, flags, &handle);
    if (ret < 0) {
        ALOGE("ion_alloc failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
        return ret;
    }
    ret = ion_share(fd, handle, handle_fd);
    ion_free(fd, handle);
    return ret;
}

int ion_import(int fd, int share_fd, ion_user_handle_t *handle)
{
/*
        struct ion_fd_data data = {
                .fd = share_fd,
        };
*/
        ion_fd_data data;
        memset(&data,0,sizeof(ion_fd_data));
        data.fd = share_fd;

        int ret = ion_ioctl(fd, ION_IOC_IMPORT, &data);
        if (ret < 0) {
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
            return ret;
        }
        *handle = data.handle;
        return ret;
}

int ion_sync_fd(int fd, int handle_fd)
{
/*
    struct ion_fd_data data = {
        .fd = handle_fd,
    };
*/
    ion_fd_data data;
    memset(&data,0,sizeof(ion_fd_data));
    data.fd = handle_fd;
    int ret = ion_ioctl(fd, ION_IOC_SYNC, &data);
    if (ret < 0)
        ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, fd);
    return ret;
}

int ion_flushcache_fd(private_handle_t* hnd,size_t start, size_t len)
{
    ion_cache_flush_data data;
    memset(&data,0,sizeof(ion_cache_flush_data));
    data.start = start;
    data.len = len;
    hw_module_t * pmodule = NULL;
    private_module_t *m=NULL;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&pmodule) == 0)
    {
        m = reinterpret_cast<private_module_t *>(pmodule);
        int ret = ion_ioctl(m->ion_client, ION_IOC_CACHE_FLUSH, &data);
        if (ret < 0)
            ALOGE("ion_ioctl failed in %s:%d, with fd=%d\n",__FUNCTION__,__LINE__, m->ion_client);
        return ret;
    }
    else
    {
        AERR("Could not get gralloc module for handle %p\n", hnd);
        return -EINVAL;
    }
}

unsigned int  ion_get_user_data(int fd, ion_user_handle_t handle)
{
    int res;
    struct ion_user_data user_data;

    if (fd < 0) {
        return 0;
    }

    user_data.handle = handle;
    user_data.bus_addr = 0;

    res = ion_ioctl(fd, ION_IOC_GET_CMA_BUFFER_INFO, &user_data);
    if (res < 0)
    {
        ALOGE("ion get user data failed");
        return 0;
    }
    else
    {
        ALOGD("ion get user data success\n");
    }

    return user_data.bus_addr;
}

int _ion_map(int fd, ion_user_handle_t handle, int *shared_fd)
{
    int res;
    struct ion_fd_data fd_data;

    if (fd < 0) {
        return fd;
    }

    fd_data.handle = handle;
    res = ion_ioctl(fd, ION_IOC_MAP, &fd_data);
    if (res < 0)
    {
        ALOGE("ion map failed");
        return res;
    }
    else
    {
        ALOGD("ion map handle %u success, fd=%d\n",(unsigned int)handle, fd_data.fd);
        *shared_fd = fd_data.fd;
    }

    return 0;
}
