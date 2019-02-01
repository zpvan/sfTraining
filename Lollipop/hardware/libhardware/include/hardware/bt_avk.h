/************************************************************************************
 *
 *  Copyright (C) 2009-2014 Broadcom Corporation
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


#ifndef ANDROID_INCLUDE_BT_AVK_H
#define ANDROID_INCLUDE_BT_AVK_H

__BEGIN_DECLS

/* Bluetooth AV connection states */


typedef enum {
    BTAVK_CONNECTION_STATE_DISCONNECTED = 0,
    BTAVK_CONNECTION_STATE_CONNECTING,
    BTAVK_CONNECTION_STATE_CONNECTED,
    BTAVK_CONNECTION_STATE_DISCONNECTING
} btavk_connection_state_t;

/* Bluetooth AV datapath states */
typedef enum {
    BTAVK_AUDIO_STATE_REMOTE_SUSPEND = 0,
    BTAVK_AUDIO_STATE_STOPPED,
    BTAVK_AUDIO_STATE_STARTED,
} btavk_audio_state_t;


/** Callback for connection state change.
 *  state will have one of the values from btav_connection_state_t
 */
typedef void (* btavk_connection_state_callback)(btavk_connection_state_t state,
                                                        bt_bdaddr_t *bd_addr);

/** Callback for audiopath state change.
 *  state will have one of the values from btav_audio_state_t
 */
typedef void (* btavk_audio_state_callback)(btavk_audio_state_t state,
                                                  bt_bdaddr_t *bd_addr);

/** Callback for audio configuration change.
 *  Used only for the A2DP sink interface.
 *  state will have one of the values from btav_audio_state_t
 *  sample_rate: sample rate in Hz
 *  channel_count: number of channels (1 for mono, 2 for stereo)
 */
typedef void (* btavk_audio_config_callback)(bt_bdaddr_t *bd_addr,
                                                uint32_t sample_rate,
                                                uint8_t channel_count);

/** BT-AVK callback structure. */
typedef struct {
    /** set to sizeof(btavk_callbacks_t) */
    size_t      size;
    btavk_connection_state_callback  connection_state_cb;
    btavk_audio_state_callback audio_state_cb;
    btavk_audio_config_callback audio_config_cb;
} btavk_callbacks_t;

/**
 * NOTE:
 *
 * 1. AVRCP 1.0 shall be supported initially. AVRCP passthrough commands
 *    shall be handled internally via uinput
 *
 * 2. A2DP data path shall be handled via a socket pipe between the AudioFlinger
 *    android_audio_hw library and the Bluetooth stack.
 *
 */
/** Represents the standard BT-AVK interface. */
typedef struct {

    /** set to sizeof(btavk_interface_t) */
    size_t          size;
    /**
     * Register the BtAv callbacks
     */
    bt_status_t (*init)( btavk_callbacks_t* callbacks );

    /** connect to headset */
    bt_status_t (*connect)( bt_bdaddr_t *bd_addr );

    /** dis-connect from headset */
    bt_status_t (*disconnect)( bt_bdaddr_t *bd_addr );

    /** Closes the interface. */
    void  (*cleanup)( void );
} btavk_interface_t;

__END_DECLS

#endif /* ANDROID_INCLUDE_BT_AVK_H */
