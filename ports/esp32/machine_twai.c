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
#include "hal/twai_types.h"

// TWAI configuration constants
#define TWAI_MAX_DATA_LEN               (8)
#define TWAI_DEFAULT_BAUDRATE           (125000)  // 125 kbit/s
#define TWAI_DEFAULT_TX_PIN             (21)
#define TWAI_DEFAULT_RX_PIN             (22)

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
    twai_mode_t mode;
    bool is_active;
    twai_filter_config_t filter_config;
    twai_general_config_t general_config;
    twai_timing_config_t timing_config;
} machine_twai_obj_t;

static machine_twai_obj_t machine_twai_obj = {
    .base = {&machine_twai_type},
    .tx_pin = TWAI_DEFAULT_TX_PIN,
    .rx_pin = TWAI_DEFAULT_RX_PIN,
    .baudrate = TWAI_DEFAULT_BAUDRATE,
    .mode = TWAI_MODE_NORMAL,
    .is_active = false,
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
    // Set timing configuration based on baudrate
    switch (baudrate) {
        case 1000000:
            self->timing_config = TWAI_TIMING_CONFIG_1MBITS();
            break;
        case 800000:
            self->timing_config = TWAI_TIMING_CONFIG_800KBITS();
            break;
        case 500000:
            self->timing_config = TWAI_TIMING_CONFIG_500KBITS();
            break;
        case 250000:
            self->timing_config = TWAI_TIMING_CONFIG_250KBITS();
            break;
        case 125000:
            self->timing_config = TWAI_TIMING_CONFIG_125KBITS();
            break;
        case 100000:
            self->timing_config = TWAI_TIMING_CONFIG_100KBITS();
            break;
        case 50000:
            self->timing_config = TWAI_TIMING_CONFIG_50KBITS();
            break;
        case 25000:
            self->timing_config = TWAI_TIMING_CONFIG_25KBITS();
            break;
        default:
            // Default to 125kbit/s for unsupported rates
            self->timing_config = TWAI_TIMING_CONFIG_125KBITS();
            break;
    }
    self->baudrate = baudrate;
}

static mp_obj_t machine_twai_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_tx, ARG_rx, ARG_baudrate, ARG_mode };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_tx, MP_ARG_INT, {.u_int = TWAI_DEFAULT_TX_PIN} },
        { MP_QSTR_rx, MP_ARG_INT, {.u_int = TWAI_DEFAULT_RX_PIN} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = TWAI_DEFAULT_BAUDRATE} },
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = TWAI_MODE_NORMAL} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_twai_obj_t *self = &machine_twai_obj;

    self->tx_pin = args[ARG_tx].u_int;
    self->rx_pin = args[ARG_rx].u_int;
    
    // Set timing configuration based on baudrate
    twai_set_timing_config(self, args[ARG_baudrate].u_int);

    // Configure mode
    switch (args[ARG_mode].u_int) {
        case TWAI_MODE_NORMAL:
            self->mode = TWAI_MODE_NORMAL;
            break;
        case TWAI_MODE_NO_ACK:
            self->mode = TWAI_MODE_NO_ACK;
            break;
        case TWAI_MODE_LISTEN_ONLY:
            self->mode = TWAI_MODE_LISTEN_ONLY;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid mode"));
    }

    // Set up general configuration
    self->general_config = (twai_general_config_t){
        .mode = self->mode,
        .tx_io = self->tx_pin,
        .rx_io = self->rx_pin,
        .clkout_io = TWAI_IO_UNUSED,
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = 10,
        .rx_queue_len = 10,
        .alerts_enabled = TWAI_ALERT_NONE,
        .clkout_divider = 0,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    // Set up default filter configuration (accept all)
    self->filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t machine_twai_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (self->is_active) {
        mp_raise_OSError(MP_EALREADY);
    }

    // Install TWAI driver
    esp_err_t err = twai_driver_install(&self->general_config, &self->timing_config, &self->filter_config);
    twai_check_esp_err(err);

    // Start TWAI driver
    err = twai_start();
    twai_check_esp_err(err);

    self->is_active = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_init_obj, 1, machine_twai_init);

static mp_obj_t machine_twai_deinit(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        return mp_const_none;
    }

    // Stop TWAI driver
    esp_err_t err = twai_stop();
    twai_check_esp_err(err);

    // Uninstall TWAI driver
    err = twai_driver_uninstall();
    twai_check_esp_err(err);

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

    // Prepare message
    twai_message_t message = {0};
    
    // Set message ID and frame format
    message.identifier = args[ARG_id].u_int;
    message.flags = args[ARG_extframe].u_bool ? TWAI_MSG_FLAG_EXTD : 0;
    
    if (args[ARG_rtr].u_bool) {
        message.flags |= TWAI_MSG_FLAG_RTR;
        message.data_length_code = 0;
    } else {
        // Get data buffer
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);
        
        if (bufinfo.len > TWAI_MAX_DATA_LEN) {
            mp_raise_ValueError(MP_ERROR_TEXT("data too long"));
        }
        
        message.data_length_code = bufinfo.len;
        memcpy(message.data, bufinfo.buf, bufinfo.len);
    }

    // Send message
    TickType_t timeout = pdMS_TO_TICKS(args[ARG_timeout].u_int);
    esp_err_t err = twai_transmit(&message, timeout);
    twai_check_esp_err(err);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_send_obj, 2, machine_twai_send);

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

    // Receive message
    twai_message_t message;
    TickType_t timeout = pdMS_TO_TICKS(args[ARG_timeout].u_int);
    esp_err_t err = twai_receive(&message, timeout);
    twai_check_esp_err(err);

    // Create return tuple: (data, id, extframe, rtr)
    mp_obj_t tuple[4];
    
    // Data
    if (message.flags & TWAI_MSG_FLAG_RTR) {
        tuple[0] = mp_const_none;
    } else {
        tuple[0] = mp_obj_new_bytes(message.data, message.data_length_code);
    }
    
    // ID
    tuple[1] = mp_obj_new_int(message.identifier);
    
    // Extended frame
    tuple[2] = mp_obj_new_bool(message.flags & TWAI_MSG_FLAG_EXTD);
    
    // RTR
    tuple[3] = mp_obj_new_bool(message.flags & TWAI_MSG_FLAG_RTR);

    return mp_obj_new_tuple(4, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_recv_obj, 1, machine_twai_recv);

static mp_obj_t machine_twai_any(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Get receive queue status
    twai_status_info_t status_info;
    esp_err_t err = twai_get_status_info(&status_info);
    twai_check_esp_err(err);

    return mp_obj_new_int(status_info.msgs_to_rx);
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_any_obj, machine_twai_any);

static mp_obj_t machine_twai_setfilter(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_mask, ARG_id1, ARG_id2 };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = TWAI_FILTER_SINGLE} },
        { MP_QSTR_mask, MP_ARG_INT, {.u_int = 0x7FF} },
        { MP_QSTR_id1, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_id2, MP_ARG_INT, {.u_int = 0} },
    };

    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Configure filter
    switch (args[ARG_mode].u_int) {
        case TWAI_FILTER_SINGLE:
            self->filter_config.acceptance_code = args[ARG_id1].u_int << 21;
            self->filter_config.acceptance_mask = ~(args[ARG_mask].u_int << 21);
            self->filter_config.single_filter = true;
            break;
        case TWAI_FILTER_DUAL:
            self->filter_config.acceptance_code = (args[ARG_id1].u_int << 21) | (args[ARG_id2].u_int << 5);
            self->filter_config.acceptance_mask = ~((args[ARG_mask].u_int << 21) | (args[ARG_mask].u_int << 5));
            self->filter_config.single_filter = false;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid filter mode"));
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_setfilter_obj, 1, machine_twai_setfilter);

static mp_obj_t machine_twai_state(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        return mp_obj_new_int(-1);  // Stopped
    }

    twai_status_info_t status_info;
    esp_err_t err = twai_get_status_info(&status_info);
    twai_check_esp_err(err);

    // Return state based on status
    if (status_info.state == TWAI_STATE_RUNNING) {
        return mp_obj_new_int(0);  // Running
    } else if (status_info.state == TWAI_STATE_BUS_OFF) {
        return mp_obj_new_int(1);  // Bus off
    } else {
        return mp_obj_new_int(2);  // Error
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_state_obj, machine_twai_state);

static mp_obj_t machine_twai_info(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    twai_status_info_t status_info;
    esp_err_t err = twai_get_status_info(&status_info);
    twai_check_esp_err(err);

    // Create info tuple: (tx_pending, rx_pending, tx_error_count, rx_error_count, arb_lost_count, bus_error_count)
    mp_obj_t tuple[6];
    tuple[0] = mp_obj_new_int(status_info.msgs_to_tx);
    tuple[1] = mp_obj_new_int(status_info.msgs_to_rx);
    tuple[2] = mp_obj_new_int(status_info.tx_error_counter);
    tuple[3] = mp_obj_new_int(status_info.rx_error_counter);
    tuple[4] = mp_obj_new_int(status_info.arb_lost_count);
    tuple[5] = mp_obj_new_int(status_info.bus_error_count);

    return mp_obj_new_tuple(6, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_info_obj, machine_twai_info);

static mp_obj_t machine_twai_restart(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_active) {
        mp_raise_OSError(MP_EPERM);
    }

    // Initiate bus recovery
    esp_err_t err = twai_initiate_recovery();
    twai_check_esp_err(err);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_restart_obj, machine_twai_restart);

static const mp_rom_map_elem_t machine_twai_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_twai_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_twai_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&machine_twai_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&machine_twai_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&machine_twai_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_setfilter), MP_ROM_PTR(&machine_twai_setfilter_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&machine_twai_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&machine_twai_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&machine_twai_restart_obj) },

    // Mode constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL), MP_ROM_INT(TWAI_MODE_NORMAL) },
    { MP_ROM_QSTR(MP_QSTR_NO_ACK), MP_ROM_INT(TWAI_MODE_NO_ACK) },
    { MP_ROM_QSTR(MP_QSTR_LISTEN_ONLY), MP_ROM_INT(TWAI_MODE_LISTEN_ONLY) },

    // Filter constants
    { MP_ROM_QSTR(MP_QSTR_FILTER_SINGLE), MP_ROM_INT(TWAI_FILTER_SINGLE) },
    { MP_ROM_QSTR(MP_QSTR_FILTER_DUAL), MP_ROM_INT(TWAI_FILTER_DUAL) },
};
static MP_DEFINE_CONST_DICT(machine_twai_locals_dict, machine_twai_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_twai_type,
    MP_QSTR_TWAI,
    MP_TYPE_FLAG_NONE,
    make_new, machine_twai_make_new,
    locals_dict, &machine_twai_locals_dict
);
