/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2025
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ESP32 TWAI (CAN) implementation for MicroPython
 * Based on ESP-IDF TWAI driver
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/obj.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "modmachine.h"
#include "machine_twai.h"
#include "mphalport.h"

#include "driver/twai.h"
#include "driver/gpio.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_twai_types.h"
#include "freertos/task.h"
#include "esp_task.h"

// TWAI configuration constants
#define TWAI_MAX_DATA_LEN               (8)
#define TWAI_DEFAULT_BAUDRATE           (125000)  // 125 kbit/s
#define TWAI_DEFAULT_TX_PIN             (4)       // GPIO4 for TCAN332 TX
#define TWAI_DEFAULT_RX_PIN             (5)       // GPIO5 for TCAN332 RX

// TWAI task configuration
#define TWAI_TASK_PRIORITY              (ESP_TASK_PRIO_MIN + 1)
#define TWAI_TASK_STACK_SIZE            (2048)

// TWAI mode constants
#define TWAI_MODE_NORMAL                (0)
#define TWAI_MODE_NO_ACK                (1)
#define TWAI_MODE_LISTEN_ONLY           (2)

// TWAI filter constants  
#define TWAI_FILTER_SINGLE              (0)
#define TWAI_FILTER_DUAL                (1)

typedef struct _machine_twai_obj_t {
    mp_obj_base_t base;
    int tx_pin;
    int rx_pin;
    uint32_t baudrate;
    uint32_t mode;
    bool is_active;
    
    // New ESP-IDF API structures
    twai_node_handle_t node_handle;
    twai_onchip_node_config_t node_config;
    
    // Callback functions for interrupts (inspired by STM32 implementation)
    mp_obj_t callback_rx;
    mp_obj_t callback_error;
    mp_obj_t callback_tx;
    mp_obj_t callback_bus_error;
    
    // Status tracking
    volatile bool last_tx_success;
    volatile bool bus_recovery_success;
    
    // Statistics counters
    uint32_t num_error_warning;
    uint32_t num_error_passive;
    uint32_t num_bus_off;
    uint32_t num_msg_rx;
    uint32_t num_msg_tx;
} machine_twai_obj_t;

static machine_twai_obj_t machine_twai_obj = {
    .base = {&machine_twai_type},
    .tx_pin = TWAI_DEFAULT_TX_PIN,
    .rx_pin = TWAI_DEFAULT_RX_PIN,
    .baudrate = TWAI_DEFAULT_BAUDRATE,
    .mode = 0,  // TWAI_MODE_NORMAL equivalent
    .is_active = false,
    .node_handle = NULL,
    
    // Initialize callback functions
    .callback_rx = mp_const_none,
    .callback_error = mp_const_none,
    .callback_tx = mp_const_none,
    .callback_bus_error = mp_const_none,
    
    // Initialize status tracking
    .last_tx_success = false,
    .bus_recovery_success = false,
    
    // Initialize statistics
    .num_error_warning = 0,
    .num_error_passive = 0,
    .num_bus_off = 0,
    .num_msg_rx = 0,
    .num_msg_tx = 0,
};

static void twai_check_esp_err(esp_err_t code) {
    switch (code) {
        case ESP_OK:
            return;
        case ESP_ERR_INVALID_ARG:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid argument"));
        case ESP_ERR_INVALID_STATE:
            mp_raise_OSError(MP_EINVAL);
        case ESP_ERR_NOT_SUPPORTED:
            mp_raise_OSError(MP_EOPNOTSUPP);
        case ESP_ERR_TIMEOUT:
            mp_raise_OSError(MP_ETIMEDOUT);
        case ESP_ERR_NO_MEM:
            mp_raise_OSError(MP_ENOMEM);
        default:
            mp_raise_OSError(MP_EIO);
    }
}

static void twai_set_timing_config(machine_twai_obj_t *self, uint32_t baudrate) {
    // Set timing configuration using new basic timing API
    self->node_config.bit_timing.bitrate = baudrate;
    self->node_config.bit_timing.sp_permill = 750;  // 75% sample point
    self->node_config.bit_timing.ssp_permill = 0;   // No secondary sample point
    self->baudrate = baudrate;
}

// Event callbacks for new ESP-IDF TWAI API
static bool twai_tx_done_cb(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx) {
    machine_twai_obj_t *self = (machine_twai_obj_t *)user_ctx;
    self->last_tx_success = edata->is_tx_success;
    self->num_msg_tx++;
    
    if (self->callback_tx != mp_const_none) {
        mp_sched_schedule(self->callback_tx, mp_obj_new_bool(edata->is_tx_success));
    }
    
    return false;
}

static bool twai_state_change_cb(twai_node_handle_t handle, const twai_state_change_event_data_t *edata, void *user_ctx) {
    machine_twai_obj_t *self = (machine_twai_obj_t *)user_ctx;
    
    switch (edata->new_sta) {
        case TWAI_ERROR_WARNING:
            self->num_error_warning++;
            break;
        case TWAI_ERROR_PASSIVE:
            self->num_error_passive++;
            break;
        case TWAI_ERROR_BUS_OFF:
            self->num_bus_off++;
            break;
        case TWAI_ERROR_ACTIVE:
            if (edata->old_sta == TWAI_ERROR_BUS_OFF) {
                self->bus_recovery_success = true;
            }
            break;
    }
    
    if (self->callback_bus_error != mp_const_none) {
        mp_sched_schedule(self->callback_bus_error, mp_obj_new_int(edata->new_sta));
    }
    
    return false;
}

static bool twai_error_cb(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx) {
    machine_twai_obj_t *self = (machine_twai_obj_t *)user_ctx;
    
    if (self->callback_error != mp_const_none) {
        mp_sched_schedule(self->callback_error, mp_obj_new_int(edata->err_flags.val));
    }
    
    return false;
}

static mp_obj_t machine_twai_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_tx, ARG_rx, ARG_baudrate, ARG_mode };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_tx, MP_ARG_INT, {.u_int = TWAI_DEFAULT_TX_PIN} },
        { MP_QSTR_rx, MP_ARG_INT, {.u_int = TWAI_DEFAULT_RX_PIN} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = TWAI_DEFAULT_BAUDRATE} },
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = 0} },  // 0 = Normal mode
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_twai_obj_t *self = &machine_twai_obj;

    self->tx_pin = args[ARG_tx].u_int;
    self->rx_pin = args[ARG_rx].u_int;
    self->mode = args[ARG_mode].u_int;
    
    // Set timing configuration
    twai_set_timing_config(self, args[ARG_baudrate].u_int);

    // Set up node configuration for new API
    self->node_config.io_cfg.tx = self->tx_pin;
    self->node_config.io_cfg.rx = self->rx_pin;
    self->node_config.io_cfg.quanta_clk_out = -1;
    self->node_config.io_cfg.bus_off_indicator = -1;
    self->node_config.clk_src = 0;  // Use default clock source
    self->node_config.bit_timing = (twai_timing_basic_config_t){
        .bitrate = self->baudrate,
        .sp_permill = 750,
        .ssp_permill = 0
    };
    self->node_config.data_timing = (twai_timing_basic_config_t){0};  // Not used for classic CAN
    self->node_config.fail_retry_cnt = -1;  // Retry forever
    self->node_config.tx_queue_depth = 5;
    self->node_config.intr_priority = 1;
    
    // Configure mode flags
    self->node_config.flags.enable_self_test = (self->mode == 1);     // NO_ACK mode
    self->node_config.flags.enable_loopback = false;
    self->node_config.flags.enable_listen_only = (self->mode == 2);   // LISTEN_ONLY mode
    self->node_config.flags.no_receive_rtr = false;

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t machine_twai_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (self->is_active) {
        mp_raise_OSError(MP_EALREADY);
    }

    // Create TWAI node using new API
    esp_err_t err = twai_new_node_onchip(&self->node_config, &self->node_handle);
    twai_check_esp_err(err);

    // Register event callbacks
    twai_event_callbacks_t callbacks = {
        .on_tx_done = twai_tx_done_cb,
        .on_rx_done = twai_rx_done_cb,
        .on_state_change = twai_state_change_cb,
        .on_error = twai_error_cb,
    };
    
    err = twai_node_register_event_callbacks(self->node_handle, &callbacks, self);
    if (err != ESP_OK) {
        twai_node_delete(self->node_handle);
        self->node_handle = NULL;
        twai_check_esp_err(err);
    }

    // Enable the TWAI node
    err = twai_node_enable(self->node_handle);
    if (err != ESP_OK) {
        twai_node_delete(self->node_handle);
        self->node_handle = NULL;
        twai_check_esp_err(err);
    }

    self->is_active = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_init_obj, 1, machine_twai_init);

static mp_obj_t machine_twai_deinit(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        return mp_const_none;
    }

    // Disable and delete TWAI node using new API
    if (self->node_handle != NULL) {
        twai_node_disable(self->node_handle);
        twai_node_delete(self->node_handle);
        self->node_handle = NULL;
    }

    self->is_active = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_deinit_obj, machine_twai_deinit);

static mp_obj_t machine_twai_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_data, ARG_id, ARG_timeout, ARG_extframe, ARG_rtr };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_extframe, MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_rtr, MP_ARG_BOOL, {.u_bool = false} },
    };

    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Prepare frame using new API
    twai_frame_t tx_frame = {0};
    uint8_t data_buffer[TWAI_MAX_DATA_LEN];
    
    // Set message ID and frame format
    tx_frame.header.id = args[ARG_id].u_int;
    tx_frame.header.ide = args[ARG_extframe].u_bool;
    tx_frame.header.rtr = args[ARG_rtr].u_bool;
    tx_frame.header.fdf = false;  // Classic CAN frame
    tx_frame.header.brs = false;
    tx_frame.header.esi = false;
    
    if (args[ARG_rtr].u_bool) {
        tx_frame.header.dlc = 0;
        tx_frame.buffer = NULL;
        tx_frame.buffer_len = 0;
    } else {
        // Get data buffer
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);
        
        if (bufinfo.len > TWAI_MAX_DATA_LEN) {
            mp_raise_ValueError(MP_ERROR_TEXT("data too long"));
        }
        
        tx_frame.header.dlc = bufinfo.len;
        memcpy(data_buffer, bufinfo.buf, bufinfo.len);
        tx_frame.buffer = data_buffer;
        tx_frame.buffer_len = bufinfo.len;
    }

    // Send frame using new API
    esp_err_t err = twai_node_transmit(self->node_handle, &tx_frame, args[ARG_timeout].u_int);
    twai_check_esp_err(err);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_send_obj, 2, machine_twai_send);

// Queue for received frames (since new API requires callback-based RX)
#define RX_QUEUE_SIZE 10
static twai_frame_t rx_queue[RX_QUEUE_SIZE];
static uint8_t rx_queue_data[RX_QUEUE_SIZE][TWAI_MAX_DATA_LEN];
static volatile int rx_queue_head = 0;
static volatile int rx_queue_tail = 0;
static volatile int rx_queue_count = 0;

static bool twai_rx_done_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
    machine_twai_obj_t *self = (machine_twai_obj_t *)user_ctx;
    
    // Try to receive frame from ISR
    if (rx_queue_count < RX_QUEUE_SIZE) {
        twai_frame_t *frame = &rx_queue[rx_queue_head];
        frame->buffer = rx_queue_data[rx_queue_head];
        frame->buffer_len = TWAI_MAX_DATA_LEN;
        
        if (twai_node_receive_from_isr(handle, frame) == ESP_OK) {
            rx_queue_head = (rx_queue_head + 1) % RX_QUEUE_SIZE;
            rx_queue_count++;
            self->num_msg_rx++;
            
            if (self->callback_rx != mp_const_none) {
                mp_sched_schedule(self->callback_rx, MP_OBJ_NEW_SMALL_INT(0));
            }
        }
    } else {
        // Queue overflow
        if (self->callback_rx != mp_const_none) {
            mp_sched_schedule(self->callback_rx, MP_OBJ_NEW_SMALL_INT(2));
        }
    }
    
    return false;
}

static mp_obj_t machine_twai_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 1000} },
    };

    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Wait for message with timeout
    uint32_t timeout_ms = args[ARG_timeout].u_int;
    uint32_t start_time = mp_hal_ticks_ms();
    
    while (rx_queue_count == 0) {
        if (timeout_ms != 0 && (mp_hal_ticks_ms() - start_time) >= timeout_ms) {
            mp_raise_OSError(MP_ETIMEDOUT);
        }
        MICROPY_EVENT_POLL_HOOK;
    }

    // Get frame from queue
    twai_frame_t *frame = &rx_queue[rx_queue_tail];
    rx_queue_tail = (rx_queue_tail + 1) % RX_QUEUE_SIZE;
    rx_queue_count--;

    // Create return tuple: (data, id, extframe, rtr)
    mp_obj_t tuple[4];
    
    // Data
    if (frame->header.rtr) {
        tuple[0] = mp_const_none;
    } else {
        tuple[0] = mp_obj_new_bytes(frame->buffer, frame->header.dlc);
    }
    
    // ID
    tuple[1] = mp_obj_new_int(frame->header.id);
    
    // Extended frame
    tuple[2] = mp_obj_new_bool(frame->header.ide);
    
    // RTR
    tuple[3] = mp_obj_new_bool(frame->header.rtr);

    return mp_obj_new_tuple(4, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_recv_obj, 1, machine_twai_recv);

static mp_obj_t machine_twai_any(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    return mp_obj_new_int(rx_queue_count);
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_any_obj, machine_twai_any);

static mp_obj_t machine_twai_setfilter(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_mask, ARG_id1, ARG_id2 };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = 0} },  // 0 = single filter
        { MP_QSTR_mask, MP_ARG_INT, {.u_int = 0x7FF} },
        { MP_QSTR_id1, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_id2, MP_ARG_INT, {.u_int = 0} },
    };

    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Configure filter using new API
    twai_mask_filter_config_t filter_config;
    
    if (args[ARG_mode].u_int == 1) {
        // Dual filter mode
        filter_config = twai_make_dual_filter(
            args[ARG_id1].u_int, args[ARG_mask].u_int,
            args[ARG_id2].u_int, args[ARG_mask].u_int,
            false  // Standard 11-bit IDs
        );
    } else {
        // Single filter mode
        filter_config.id = args[ARG_id1].u_int;
        filter_config.mask = args[ARG_mask].u_int;
        filter_config.is_ext = false;
        filter_config.dual_filter = false;
        filter_config.no_classic = false;
        filter_config.no_fd = true;  // Exclude FD frames
        filter_config.id_list = NULL;
        filter_config.num_of_ids = 0;
    }

    esp_err_t err = twai_node_config_mask_filter(self->node_handle, 0, &filter_config);
    twai_check_esp_err(err);

    return mp_const_none;
}
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_setfilter_obj, 1, machine_twai_setfilter);

static mp_obj_t machine_twai_state(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        return mp_obj_new_int(-1);  // Stopped
    }

    twai_node_status_t status;
    twai_node_record_t stats;
    esp_err_t err = twai_node_get_info(self->node_handle, &status, &stats);
    twai_check_esp_err(err);

    return mp_obj_new_int(status.state);
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_state_obj, machine_twai_state);

static mp_obj_t machine_twai_info(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    twai_node_status_t status;
    twai_node_record_t stats;
    esp_err_t err = twai_node_get_info(self->node_handle, &status, &stats);
    twai_check_esp_err(err);

    // Create info tuple: (tx_pending, rx_pending, tx_error_count, rx_error_count, arb_lost_count, bus_error_count)
    mp_obj_t tuple[6];
    tuple[0] = mp_obj_new_int(0);  // No direct TX pending info in new API
    tuple[1] = mp_obj_new_int(rx_queue_count);
    tuple[2] = mp_obj_new_int(status.tx_error_count);
    tuple[3] = mp_obj_new_int(status.rx_error_count);
    tuple[4] = mp_obj_new_int(0);  // Arb lost not directly available
    tuple[5] = mp_obj_new_int(stats.bus_err_num);

    return mp_obj_new_tuple(6, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_info_obj, machine_twai_info);

// Extended statistics method inspired by STM32 implementation
static mp_obj_t machine_twai_stats(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    twai_node_status_t status;
    twai_node_record_t stats;
    esp_err_t err = twai_node_get_info(self->node_handle, &status, &stats);
    twai_check_esp_err(err);

    // Create comprehensive statistics dictionary
    mp_obj_t dict = mp_obj_new_dict(0);
    
    // ESP-IDF driver statistics
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_tx_error_counter), mp_obj_new_int(status.tx_error_count));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_rx_error_counter), mp_obj_new_int(status.rx_error_count));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_bus_error_count), mp_obj_new_int(stats.bus_err_num));
    
    // Our internal statistics
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_error_warning_count), mp_obj_new_int(self->num_error_warning));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_error_passive_count), mp_obj_new_int(self->num_error_passive));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_bus_off_count), mp_obj_new_int(self->num_bus_off));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_msg_rx_count), mp_obj_new_int(self->num_msg_rx));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_msg_tx_count), mp_obj_new_int(self->num_msg_tx));

    return dict;
}
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_error_warning_count), mp_obj_new_int(self->num_error_warning));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_error_passive_count), mp_obj_new_int(self->num_error_passive));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_bus_off_count), mp_obj_new_int(self->num_bus_off));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_msg_rx_count), mp_obj_new_int(self->num_msg_rx));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_msg_tx_count), mp_obj_new_int(self->num_msg_tx));

    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_stats_obj, machine_twai_stats);

static mp_obj_t machine_twai_restart(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Check if we're in bus-off state
    twai_node_status_t status;
    twai_node_record_t stats;
    esp_err_t err = twai_node_get_info(self->node_handle, &status, &stats);
    twai_check_esp_err(err);

    if (status.state != TWAI_ERROR_BUS_OFF) {
        mp_raise_ValueError(MP_ERROR_TEXT("not in bus-off state"));
    }

    // Initiate bus recovery
    self->bus_recovery_success = false;
    err = twai_node_recover(self->node_handle);
    twai_check_esp_err(err);

    // Wait for recovery with timeout
    uint32_t start = mp_hal_ticks_ms();
    while (!self->bus_recovery_success) {
        if (mp_hal_ticks_ms() - start > 5000) {  // 5 second timeout
            mp_raise_OSError(MP_ETIMEDOUT);
        }
        MICROPY_EVENT_POLL_HOOK;
    }

    return mp_const_none;
}

// Set RX callback function
static mp_obj_t machine_twai_rxcallback(mp_obj_t self_in, mp_obj_t callback_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (callback_in == mp_const_none) {
        // Disable callback
        self->callback_rx = mp_const_none;
    } else if (mp_obj_is_callable(callback_in)) {
        // Set up callback
        self->callback_rx = callback_in;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("callback must be callable or None"));
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_restart_obj, machine_twai_restart);
static MP_DEFINE_CONST_FUN_OBJ_2(machine_twai_rxcallback_obj, machine_twai_rxcallback);

static const mp_rom_map_elem_t machine_twai_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_twai_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_twai_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&machine_twai_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&machine_twai_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&machine_twai_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_setfilter), MP_ROM_PTR(&machine_twai_setfilter_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&machine_twai_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&machine_twai_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_stats), MP_ROM_PTR(&machine_twai_stats_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&machine_twai_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_rxcallback), MP_ROM_PTR(&machine_twai_rxcallback_obj) },

    // Mode constants (new API uses different values)
    { MP_ROM_QSTR(MP_QSTR_NORMAL), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_NO_ACK), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_LISTEN_ONLY), MP_ROM_INT(2) },

    // Filter constants  
    { MP_ROM_QSTR(MP_QSTR_FILTER_SINGLE), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_FILTER_DUAL), MP_ROM_INT(1) },
    
    // State constants (using new API values)
    { MP_ROM_QSTR(MP_QSTR_ERROR_ACTIVE), MP_ROM_INT(TWAI_ERROR_ACTIVE) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_WARNING), MP_ROM_INT(TWAI_ERROR_WARNING) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_PASSIVE), MP_ROM_INT(TWAI_ERROR_PASSIVE) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_BUS_OFF), MP_ROM_INT(TWAI_ERROR_BUS_OFF) },
};
static MP_DEFINE_CONST_DICT(machine_twai_locals_dict, machine_twai_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_twai_type,
    MP_QSTR_TWAI,
    MP_TYPE_FLAG_NONE,
    make_new, machine_twai_make_new,
    locals_dict, &machine_twai_locals_dict
);
