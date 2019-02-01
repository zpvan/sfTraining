/************************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/
#ifndef ANDROID_INCLUDE_BT_3D_H
#define ANDROID_INCLUDE_BT_3D_H

__BEGIN_DECLS

/**
 * NOTE: Focus has been on the SIG 3D Sync Profile. TBD for Legacy mode.
 */

/* Bluetooth 3D Mode */
typedef enum {
    BT3D_MODE_IDLE = 0,
    BT3D_MODE_MASTER,
    BT3D_MODE_SLAVE,
    BT3D_MODE_SHOWROOM
} bt3d_mode_t;

/* Bluetooth 3D Broadcast data */
typedef struct {
    uint16_t left_open_offset;
    uint16_t left_close_offset;
    uint16_t right_open_offset;
    uint16_t right_close_offset;
    uint16_t delay;
    uint8_t dual_view;
} bt3d_data_t;

/** Callback for UCD Assoc Notification
 */
typedef void (* bt3d_assos_notif_callback)(bt_bdaddr_t *bd_addr);

/** Callback for UCD Battery Report
 */
typedef void (* bt3d_batt_level_callback)(bt_bdaddr_t *bd_addr,
                                          int8_t battery_level);

/** Callback for Frame period
 */
typedef void (*bt3d_frame_period_callback)(uint16_t frame_period);

/** Callback for Slave synchronization with master (Only relevant in slave mode)
 * If lock_status is TRUE, then the slave is locked to Master, otherwise lost sync with the master*/
typedef void (*bt3d_master_sync_callback)(uint8_t lock_status);

/** BT-3D callback structure. */
typedef struct {
    /** set to sizeof(bt3d_callbacks_t) */
    size_t      size;
    bt3d_assos_notif_callback assos_notif_cb;
    bt3d_batt_level_callback  batt_level_cb;
    bt3d_frame_period_callback frame_period_cb;
    bt3d_master_sync_callback master_sync_cb;
} bt3d_callbacks_t;

/**
 * NOTE:
 *
 * 1.) 3D Config parameters such as path_loss_threshold, EIR data etc will be managed
 *     via the bt_stack.conf and Platform specific buildcfg.h
 */
/** Represents the standard BT-3DS interface. */
typedef struct {

    /** set to sizeof(bt3d_interface_t) */
    size_t          size;
    /**
     * Register the Bt3d callbacks
     */
    bt_status_t (*init)( bt3d_callbacks_t* callbacks );

    /**
     * Sets the desired 3D mode: Idle, Master, Slave or ShowRoom
     *
     * master_bd_addr - Bluetooth Device Address to lock to for slave mode.
     *                  Ignored for other modes.
     */
    bt_status_t (*set_mode)(bt3d_mode_t mode, bt_bdaddr_t *master_bd_addr);

    /**
     * Broadcasts the 3DS data left/right shutter offset and delay
     * to the 3DG as ConnectionLessBroadcast
     */
    bt_status_t (*broadcast_3d_data)(bt3d_data_t data);

    /** Closes the interface. */
    void  (*cleanup)( void );
} bt3d_interface_t;

__END_DECLS

#endif /* ANDROID_INCLUDE_BT_3D_H */
