#ifndef _ION_EXPORT_H
#define _ION_EXPORT_H
#include "ion.h"
#include "gralloc_priv.h"
int ion_open();
int ion_close(int fd);
int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
            unsigned int flags, ion_user_handle_t *handle);
int ion_cust_alloc(int fd, size_t start, size_t len, size_t align, unsigned int heap_mask,
            unsigned int flags, ion_user_handle_t *handle);
int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
                 unsigned int flags, int *handle_fd);
int ion_sync_fd(int fd, int handle_fd);
int ion_flushcache_fd(private_handle_t* hnd,size_t start, size_t len);
int ion_free(int fd, ion_user_handle_t handle);
int ion_map(int fd, ion_user_handle_t handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd);
int ion_share(int fd, ion_user_handle_t handle, int *share_fd);
int ion_import(int fd, int share_fd, ion_user_handle_t *handle);
unsigned int  ion_get_user_data(int fd, ion_user_handle_t handle);
int _ion_map(int fd, ion_user_handle_t handle, int *shared_fd);
#endif
