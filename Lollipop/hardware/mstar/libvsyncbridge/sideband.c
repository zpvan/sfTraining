//<MStar Software>
//******************************************************************************
//MStar Software
//Copyright (c) 2015 - 2016 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party's software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party's software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar's confidential information and you agree to keep MStar's
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer's product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

//#define LOG_NDEBUG 0
#define LOG_TAG "SideBand"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <cutils/log.h>
#include <cutils/native_handle.h>
#include <hardware/hardware.h>
#include "synchronizer.h"
#include "sideband.h"
#include "tv_input.h"
#include "disp_api.h"

typedef struct sideband_info
{
    uint32_t                        used;
    uint32_t                        slot;
    uint32_t                        index;
    pthread_t                       cmd_task;
    pthread_t                       sync_task;
    uint32_t                        audio_hw_sync;
    uint32_t                        cmd_exit;
    uint32_t                        sync_exit;
    uint32_t                        win_resize;
    pthread_mutex_t                 win_lock;
    pthread_mutex_t                 wait_win_info;
    pthread_mutex_t                 wait_context;
    sideband_handle_t               *handle;
    void                            *win_info;
    void                            *callback_context;
    sideband_get_frame_callback     get_frame;
    sideband_free_frame_callback    free_frame;
    sideband_flip_frame_callback    flip_frame;
    sideband_drop_frame_callback    drop_frame;
    sideband_update_wininfo_callback update_wininfo;
    sideband_init_wininfo_callback  init_wininfo;
    sideband_deinit_wininfo_callback deinit_wininfo;
    struct synchronizer_device_t    *sync_device;
    uint32_t                        resync;
    uint64_t                        resync_base;
    uint64_t                        frame_time;
    bool                            vonly;
} sideband_info_t;

typedef enum {
    E_SIDEBAND_SYNC_OK,
    E_SIDEBAND_SYNC_WAIT,
    E_SIDEBAND_SYNC_DROP,
} EN_SIDEBAND_SYNC_RET;

static sideband_info_t sideband[MAX_SIDEBAND_COUNT];
static pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;

static EN_SIDEBAND_SYNC_RET MsSideBandWaitSync(sideband_info_t *si, int64_t timestamp)
{
    int64_t cur_time, upbound, lowbound;

    lowbound = timestamp - si->frame_time;
    upbound = timestamp + si->frame_time;

    si->sync_device->get_current_time(si->sync_device, &cur_time);

    ALOGV("time: %lld %lld", cur_time, timestamp);

    if (cur_time > upbound) {
        return E_SIDEBAND_SYNC_DROP;
    } else if (cur_time > lowbound) {
        return E_SIDEBAND_SYNC_OK;
    }

    lowbound -= cur_time;
    if (lowbound > 30000)
        lowbound = 30000;
    usleep(lowbound);

    return E_SIDEBAND_SYNC_WAIT;
}

#define QUEUE_DEPTH 3
#define INC_QUEUE_IDX(idx) ((idx + 1) % QUEUE_DEPTH)

static void *MsSideBandSyncThread(void* threadData)
{
    sideband_info_t *si = (sideband_info_t *)threadData;
    sideband_handle_t *pSideBand = si->handle;
    MS_DispFrameFormat sDispFrm[QUEUE_DEPTH];
    MS_DispFrameFormat *pDispFrm = &sDispFrm[0];
    MS_WinInfo info;
    EN_SIDEBAND_SYNC_RET sync_ret;
    uint32_t fidx = 0;
    uint32_t ridx = 0;
    uint32_t inited = FALSE;
    uint32_t sync_count = QUEUE_DEPTH - 1;
    bool got_frame = FALSE;
    bool first_got = FALSE;
    bool check_sync = FALSE;

    prctl(PR_SET_NAME, (unsigned long)__FUNCTION__, 0, 0, 0);

    pthread_mutex_lock(&si->wait_context);
    pthread_mutex_lock(&si->wait_win_info);

    while (!si->sync_exit) {
        if (got_frame == FALSE &&
            si->get_frame(si->callback_context, pDispFrm) == E_SIDEBAND_STATUS_OK) {

            if (!inited) {
                pthread_mutex_lock(&si->win_lock);
                info = *((MS_WinInfo *)si->win_info);
                pthread_mutex_unlock(&si->win_lock);
                info.crop_win.x = pDispFrm->sFrames[0].u32CropLeft;
                info.crop_win.y = pDispFrm->sFrames[0].u32CropTop;
                info.crop_win.width = pDispFrm->sFrames[0].u32Width - pDispFrm->sFrames[0].u32CropLeft - pDispFrm->sFrames[0].u32CropRight;
                info.crop_win.height = pDispFrm->sFrames[0].u32Height - pDispFrm->sFrames[0].u32CropTop - pDispFrm->sFrames[0].u32CropBottom;
                info.default_zoom_mode = TRUE;
                MS_Video_Disp_Open(si->index, info, pDispFrm);
                si->frame_time = 1000000000ULL / pDispFrm->u32FrameRate;
                inited = TRUE;
            }

            got_frame = TRUE;
            first_got = TRUE;
        }

        if (si->win_resize && first_got) {
            pthread_mutex_lock(&si->win_lock);
            info = *((MS_WinInfo *)si->win_info);
            pthread_mutex_unlock(&si->win_lock);
            info.crop_win.x = pDispFrm->sFrames[0].u32CropLeft;
            info.crop_win.y = pDispFrm->sFrames[0].u32CropTop;
            info.crop_win.width = pDispFrm->sFrames[0].u32Width - pDispFrm->sFrames[0].u32CropLeft - pDispFrm->sFrames[0].u32CropRight;
            info.crop_win.height = pDispFrm->sFrames[0].u32Height - pDispFrm->sFrames[0].u32CropTop - pDispFrm->sFrames[0].u32CropBottom;
            info.default_zoom_mode = TRUE;
            MS_Video_Disp_Resize(si->index, info, pDispFrm);
            si->win_resize = FALSE;
        }

        if (si->resync && got_frame) {
            if (check_sync) {
                si->resync_base = pDispFrm->u64Pts;
                si->resync = FALSE;
                check_sync = FALSE;
                ALOGV("resync time: %llu", si->resync_base);
            } else {
                uint32_t cnt = QUEUE_DEPTH - 1 - sync_count;
                ALOGV("resync");
                if (cnt > 0) {
                    si->free_frame(si->callback_context, pDispFrm);
                    ridx = fidx;
                    while (--cnt > 0 && (!si->sync_exit)) {
                        ridx = INC_QUEUE_IDX(ridx);
                        MS_Video_Disp_Wait(si->index, MS_WAIT_FRAME_DONE, &sDispFrm[ridx]);
                        si->free_frame(si->callback_context, &sDispFrm[ridx]);
                    }
                    sync_count = QUEUE_DEPTH - 2;
                    got_frame = FALSE;
                } else {
                    sync_count = QUEUE_DEPTH - 1;
                }
                check_sync = TRUE;
                continue;
            }
        }

        if (got_frame) {
            sync_ret = MsSideBandWaitSync(si, pDispFrm->u64Pts);
            if (sync_ret == E_SIDEBAND_SYNC_WAIT) {
                continue;
            } else if (sync_ret == E_SIDEBAND_SYNC_DROP) {
                si->free_frame(si->callback_context, pDispFrm);
                if (si->drop_frame) si->drop_frame(si->callback_context, pDispFrm);
                got_frame = FALSE;
                continue;
            }

            MS_Video_Disp_Flip(si->index, pDispFrm);
            if (si->flip_frame) si->flip_frame(si->callback_context, pDispFrm);

            if (sync_count == 0) {
                ridx = INC_QUEUE_IDX(fidx);
                MS_Video_Disp_Wait(si->index, MS_WAIT_FRAME_DONE, &sDispFrm[ridx]);
                si->free_frame(si->callback_context, &sDispFrm[ridx]);
            }

            fidx = INC_QUEUE_IDX(fidx);
            pDispFrm = &sDispFrm[fidx];
            got_frame = FALSE;

            if (sync_count > 0) sync_count--;
        } else {
            usleep(1000);
        }
    }

    return NULL;
}

static void *MsSideBandCmdThread(void* threadData)
{
    sideband_info_t *si = (sideband_info_t *)threadData;
    sideband_handle_t *pSideBand = si->handle;
    uint32_t cmd;

    prctl(PR_SET_NAME, (unsigned long)__FUNCTION__, 0, 0, 0);

    while (!si->cmd_exit) {
        read(pSideBand->pipe_in[READ_END], &cmd, sizeof(cmd));

        switch (cmd) {
            case E_SIDEBAND_COMMAND_INIT:
            {
                if (si->vonly) {
                    free(si->win_info);
                    MS_WinInfo *info = calloc(1, sizeof(MS_WinInfo));
                    read(pSideBand->pipe_in[READ_END], info, sizeof(MS_WinInfo));
                    si->win_info = info;
                    si->init_wininfo(si->callback_context, si->win_info);
                } else {
                    pthread_mutex_lock(&si->win_lock);
                    free(si->win_info);
                    MS_WinInfo *info = calloc(1, sizeof(MS_WinInfo));
                    read(pSideBand->pipe_in[READ_END], info, sizeof(MS_WinInfo));
                    MS_Video_Disp_Init(si->index);
                    si->win_info = info;
                    pthread_mutex_unlock(&si->win_lock);
                    pthread_mutex_unlock(&si->wait_win_info);
                }
            }
                break;
            case E_SIDEBAND_COMMAND_RESIZE:
            {
                if (si->vonly) {
                    free(si->win_info);
                    MS_WinInfo *info = calloc(1, sizeof(MS_WinInfo));
                    read(pSideBand->pipe_in[READ_END], info, sizeof(MS_WinInfo));
                    si->win_info = info;
                    si->update_wininfo(si->callback_context, si->win_info);
                } else {
                    pthread_mutex_lock(&si->win_lock);
                    free(si->win_info);
                    MS_WinInfo *info = calloc(1, sizeof(MS_WinInfo));
                    read(pSideBand->pipe_in[READ_END], info, sizeof(MS_WinInfo));
                    si->win_info = info;
                    si->win_resize = TRUE;
                    pthread_mutex_unlock(&si->win_lock);
                }
            }
                break;
            case E_SIDEBAND_COMMAND_DEINIT:
            {
                if (si->vonly) {
                    free(si->win_info);
                    MS_WinInfo *info = calloc(1, sizeof(MS_WinInfo));
                    read(pSideBand->pipe_in[READ_END], info, sizeof(MS_WinInfo));
                    si->win_info = info;
                    if (si->deinit_wininfo) {
                        si->deinit_wininfo(si->callback_context, si->win_info);
                    }
                }
            }
                break;
            default:
                break;
        }
    }

    return NULL;
}

static void MsSideBandLeaveCmdThread(sideband_info_t *si)
{
    sideband_handle_t *pSideBand = si->handle;
    uint32_t cmd = E_SIDEBAND_COMMAND_EXIT;

    si->cmd_exit = TRUE;
    write(pSideBand->pipe_in[WRITE_END], &cmd, sizeof(cmd));
    pthread_join(si->cmd_task, NULL);
    si->cmd_exit = FALSE;
}

static void MsSideBandLeaveSyncThread(sideband_info_t *si)
{
    sideband_handle_t *pSideBand = si->handle;

    pthread_mutex_unlock(&si->wait_win_info);
    pthread_mutex_unlock(&si->wait_context);
    si->sync_exit = TRUE;
    pthread_join(si->sync_task, NULL);
    si->sync_exit = FALSE;
}

EN_SIDEBAND_STATUS MsSideBandFlush(sideband_info_t *si)
{
    EN_SIDEBAND_STATUS ret = E_SIDEBAND_STATUS_OK;

    if (si->index >= MAX_SIDEBAND_COUNT) {
        ret = E_SIDEBAND_STATUS_BAD_INDEX;
        goto EXIT;
    }

    if (!sideband[si->index].used) {
        ret = E_SIDEBAND_STATUS_ALREADY_CLOSED;
        goto EXIT;
    }

    ALOGV("flush");
    si->resync = TRUE;

    EXIT:

    return ret;
}

EN_SIDEBAND_STATUS MsSideBandDeinit(sideband_info_t *si)
{
    EN_SIDEBAND_STATUS ret = E_SIDEBAND_STATUS_OK;
    sideband_handle_t *pSideBand = NULL;

    pthread_mutex_lock(&main_lock);
    if (!sideband[si->slot].used) {
        ret = E_SIDEBAND_STATUS_ALREADY_CLOSED;
        goto EXIT;
    }
    pSideBand = si->handle;
    if (!si->vonly) {
        MsSideBandLeaveSyncThread(si);
    }
    MsSideBandLeaveCmdThread(si);
    if (!si->vonly) {
        pthread_mutex_destroy(&si->win_lock);
        pthread_mutex_destroy(&si->wait_win_info);
        pthread_mutex_destroy(&si->wait_context);
        MS_Video_Disp_Close(si->index);
    }
    free(si->win_info);
    si->win_info = NULL;

    close(pSideBand->pipe_in[READ_END]);
    close(pSideBand->pipe_in[WRITE_END]);
    close(pSideBand->pipe_out[READ_END]);
    close(pSideBand->pipe_out[WRITE_END]);
    if (!si->vonly)
        sync_close(si->sync_device);

    free(si->handle);
    si->handle = NULL;
    si->used = FALSE;

    EXIT:
    pthread_mutex_unlock(&main_lock);

    return ret;
}

static int MsSideBandGetSlot(void)
{
    int i = 0, slot = -1;
    for (i = 0; i < MAX_SIDEBAND_COUNT; i++) {
        if (sideband[i].used == FALSE) {
            slot = i;
            break;
        }
    }
    return slot;
}

EN_SIDEBAND_STATUS MsSideBandUpdateVdecInfo(sideband_info_t *si, uint32_t stream_index, void *context)
{
    EN_SIDEBAND_STATUS ret = E_SIDEBAND_STATUS_OK;
    si->index = stream_index;
    si->callback_context = context;
    pthread_mutex_unlock(&si->wait_context);
    return ret;
}

EN_SIDEBAND_STATUS MsSideBandInit(sideband_info_t **handle, sideband_parameter *sp)
{
    EN_SIDEBAND_STATUS ret = E_SIDEBAND_STATUS_OK;
    sideband_info_t *si = NULL;
    sideband_handle_t *pSideBand = NULL;
    bool pipe_in = FALSE;
    bool pipe_out = FALSE;
    bool win_lock = FALSE;
    bool wait_win_info = FALSE;
    bool wait_context = FALSE;
    bool cmd_thread = FALSE;
    bool sync_thread = FALSE;
    int slot = -1;
    int result = 0;

    pthread_mutex_lock(&main_lock);

    slot = MsSideBandGetSlot();

    if (slot == -1) {
        pthread_mutex_unlock(&main_lock);
        return E_SIDEBAND_STATUS_FULL;
    }

    si = &sideband[slot];
    memset(si, 0, sizeof(sideband_info_t));

    // FIXME: Following definitely can't pass codereview.
    si->vonly = sp->vonly;
    si->slot = slot;
    si->resync = TRUE;

    pSideBand = calloc(1, sizeof(sideband_handle_t));
    pSideBand->nativeHandle.version = sizeof(native_handle_t);
    pSideBand->nativeHandle.numFds = 4;
    pSideBand->nativeHandle.numInts = (sizeof(sideband_handle_t)
        - sizeof(native_handle_t))/4 - pSideBand->nativeHandle.numFds;

    if (sp->sidebandType == E_SIDEBAND_TYPE_XC) {
        pSideBand->sideband_type = TV_INPUT_SIDEBAND_TYPE_MM_XC;
    } else {
        pSideBand->sideband_type = TV_INPUT_SIDEBAND_TYPE_MM_GOP;
    }

    if (si->vonly) {
        pSideBand->device_id = slot;
        si->handle = pSideBand;
        si->update_wininfo = sp->update_wininfo;
        si->init_wininfo = sp->init_wininfo;
        si->deinit_wininfo = sp->deinit_wininfo;
        si->callback_context = sp->callback_context;
    } else {
        pSideBand->device_id = sp->audio_hw_sync;
        si->handle = pSideBand;
        si->audio_hw_sync = sp->audio_hw_sync;
        si->get_frame = sp->get_frame;
        si->free_frame = sp->free_frame;
        si->flip_frame = sp->flip_frame;
        si->drop_frame = sp->drop_frame;
    }

    if (pipe(pSideBand->pipe_in) == -1) {
        ret = E_SIDEBAND_STATUS_PIPE_FAILED;
        goto EXIT;
    }
    pipe_in = TRUE;

    if (pipe(pSideBand->pipe_out) == -1) {
        ret = E_SIDEBAND_STATUS_PIPE_FAILED;
        goto EXIT;
    }
    pipe_out = TRUE;
    if (!si->vonly) {
        result = pthread_mutex_init(&si->win_lock, NULL);
        if (result != 0) {
            ret = E_SIDEBAND_STATUS_MUTEX_FAILED;
            goto EXIT;
        }
        win_lock = TRUE;

        result = pthread_mutex_init(&si->wait_win_info, NULL);
        if (result != 0) {
            ret = E_SIDEBAND_STATUS_MUTEX_FAILED;
            goto EXIT;
        }
        wait_win_info = TRUE;

        result = pthread_mutex_init(&si->wait_context, NULL);
        if (result != 0) {
            ret = E_SIDEBAND_STATUS_MUTEX_FAILED;
            goto EXIT;
        }
        wait_context = TRUE;
    }
    result = pthread_create(&si->cmd_task, NULL, MsSideBandCmdThread, si);
    if (result != 0) {
        ret = E_SIDEBAND_STATUS_THREAD_FAILED;
        goto EXIT;
    }
    cmd_thread = TRUE;
    if (si->vonly) {
        si->sync_device = NULL;
    } else {
        pthread_mutex_lock(&si->wait_win_info);
        pthread_mutex_lock(&si->wait_context);
        result = pthread_create(&si->sync_task, NULL, MsSideBandSyncThread, si);
        if (result != 0) {
            ret = E_SIDEBAND_STATUS_THREAD_FAILED;
            goto EXIT;
        }
        sync_thread = TRUE;

        const struct hw_module_t *module;
        struct synchronizer_device_t *device;

        if (hw_get_module(SYNCHRONIZER_HARDWARE_MODULE_ID, &module)) {
            ret = E_SIDEBAND_STATUS_HW_SYNC_MODULE_FAILED;
            goto EXIT;
        }

        ALOGD("audio_hw_sync: %x", sp->audio_hw_sync);

        if (sync_open(module, sp->audio_hw_sync, &device)) {
            ret = E_SIDEBAND_STATUS_HW_SYNC_DEVICE_FAILED;
            goto EXIT;
        }

        si->sync_device = device;
    }
    si->used = TRUE;
    sp->sideband_handle = pSideBand;
    *handle = si;

    EXIT:
    if (ret != E_SIDEBAND_STATUS_OK) {
        if (!si->vonly) {
            if (sync_thread)
                MsSideBandLeaveSyncThread(si);
        }
        if (cmd_thread)
            MsSideBandLeaveCmdThread(si);
        if (!si->vonly) {
            if (win_lock)
                pthread_mutex_destroy(&si->win_lock);
            if (wait_win_info)
                pthread_mutex_destroy(&si->wait_win_info);
            if (wait_context)
                pthread_mutex_destroy(&si->wait_context);
        }
        if (pipe_out) {
            close(pSideBand->pipe_out[READ_END]);
            close(pSideBand->pipe_out[WRITE_END]);
        }
        if (pipe_in) {
            close(pSideBand->pipe_in[READ_END]);
            close(pSideBand->pipe_in[WRITE_END]);
        }
        if (si) {
            free(si->handle);
        }
    }

    pthread_mutex_unlock(&main_lock);

    return ret;
}
