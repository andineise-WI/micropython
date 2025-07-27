/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 ESP32 TWAI Implementation for TCAN332
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/machine_mem.h"
#include "modmachine.h"
#include "machine_twai.h"
#include "mphalport.h"

#include "driver/twai.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_task.h"

// TCAN332 Configuration for TWAI
#define TWAI_MAX_DATA_LEN               (8)
#define TWAI_DEFAULT_BAUDRATE           (125000)  // 125 kbit/s
#define TWAI_DEFAULT_TX_PIN             (4)       // GPIO4 for TCAN332 TX
#define TWAI_DEFAULT_RX_PIN             (5)       // GPIO5 for TCAN332 RX

// Task configuration
#define TWAI_TASK_PRIORITY              (ESP_TASK_PRIO_MIN + 1)
#define TWAI_TASK_STACK_SIZE            (2048)

// TWAI modes
#define TWAI_MODE_NORMAL                (0)
#define TWAI_MODE_NO_ACK                (1)
#define TWAI_MODE_LISTEN_ONLY           (2)

// Filter types
#define TWAI_FILTER_SINGLE              (0)
#define TWAI_FILTER_DUAL                (1)

typedef struct _machine_twai_obj_t {
    mp_obj_base_t base;
    
    // Pin configuration for TCAN332
    int tx_pin;
    int rx_pin;
    uint32_t baudrate;
    
    // ESP-IDF v5.1.2 compatible structures
    twai_general_config_t general_config;
    twai_timing_config_t timing_config; 
    twai_filter_config_t filter_config;
    
    // State tracking
    bool installed;
    bool started;
    
    // Callback functions for interrupts
    mp_obj_t callback_rx;
    mp_obj_t callback_error;
    mp_obj_t callback_tx;
    mp_obj_t callback_bus_error;
    
    // Statistics
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    
    // Filter settings
    uint32_t filter_id;
    uint32_t filter_mask;
    uint8_t filter_type;
} machine_twai_obj_t;

static machine_twai_obj_t machine_twai_obj = {
    .base = {&machine_twai_type},
    .tx_pin = TWAI_DEFAULT_TX_PIN,
    .rx_pin = TWAI_DEFAULT_RX_PIN,
    .baudrate = TWAI_DEFAULT_BAUDRATE,
    .installed = false,
    .started = false,
    .callback_rx = mp_const_none,
    .callback_error = mp_const_none,
    .callback_tx = mp_const_none,
    .callback_bus_error = mp_const_none,
    .tx_count = 0,
    .rx_count = 0,
    .error_count = 0,
    .filter_id = 0,
    .filter_mask = 0xFFFFFFFF,
    .filter_type = TWAI_FILTER_SINGLE
};

// Helper function to convert baudrate to timing config
static esp_err_t twai_get_timing_config(uint32_t baudrate, twai_timing_config_t *timing_config) {
    switch (baudrate) {
        case 25000:   *timing_config = TWAI_TIMING_CONFIG_25KBITS(); break;
        case 50000:   *timing_config = TWAI_TIMING_CONFIG_50KBITS(); break;
        case 100000:  *timing_config = TWAI_TIMING_CONFIG_100KBITS(); break;
        case 125000:  *timing_config = TWAI_TIMING_CONFIG_125KBITS(); break;
        case 250000:  *timing_config = TWAI_TIMING_CONFIG_250KBITS(); break;
        case 500000:  *timing_config = TWAI_TIMING_CONFIG_500KBITS(); break;
        case 800000:  *timing_config = TWAI_TIMING_CONFIG_800KBITS(); break;
        case 1000000: *timing_config = TWAI_TIMING_CONFIG_1MBITS(); break;
        default:
            return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

// Initialize TWAI with TCAN332 configuration
static mp_obj_t machine_twai_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_tx, ARG_rx, ARG_baudrate, ARG_mode };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_tx, MP_ARG_INT, {.u_int = TWAI_DEFAULT_TX_PIN} },
        { MP_QSTR_rx, MP_ARG_INT, {.u_int = TWAI_DEFAULT_RX_PIN} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = TWAI_DEFAULT_BAUDRATE} },
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = TWAI_MODE_NORMAL} },
    };
    
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    // Stop if already running
    if (self->started) {
        twai_stop();
        self->started = false;
    }
    
    if (self->installed) {
        twai_driver_uninstall();
        self->installed = false;
    }
    
    // Update configuration
    self->tx_pin = args[ARG_tx].u_int;
    self->rx_pin = args[ARG_rx].u_int;
    self->baudrate = args[ARG_baudrate].u_int;
    
    // Validate TCAN332 pin configuration
    if (self->tx_pin < 0 || self->tx_pin > 39) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid tx pin for TCAN332"));
    }
    if (self->rx_pin < 0 || self->rx_pin > 39) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid rx pin for TCAN332"));
    }
    
    // Configure timing
    esp_err_t ret = twai_get_timing_config(self->baudrate, &self->timing_config);
    if (ret != ESP_OK) {
        mp_raise_ValueError(MP_ERROR_TEXT("unsupported baudrate for TCAN332"));
    }
    
    // General configuration for TCAN332
    self->general_config = (twai_general_config_t) {
        .mode = (args[ARG_mode].u_int == TWAI_MODE_LISTEN_ONLY) ? TWAI_MODE_LISTEN_ONLY : TWAI_MODE_NORMAL,
        .tx_io = self->tx_pin,
        .rx_io = self->rx_pin,
        .clkout_io = TWAI_IO_UNUSED,
        .bus_off_io = TWAI_IO_UNUSED,
        .tx_queue_len = 10,
        .rx_queue_len = 10,
        .alerts_enabled = TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR,
        .clkout_divider = 0,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    
    // Filter configuration (accept all messages by default)
    self->filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install TWAI driver
    ret = twai_driver_install(&self->general_config, &self->timing_config, &self->filter_config);
    if (ret != ESP_OK) {
        mp_raise_OSError(MP_EIO);
    }
    
    self->installed = true;
    
    // Start TWAI driver
    ret = twai_start();
    if (ret != ESP_OK) {
        twai_driver_uninstall();
        self->installed = false;
        mp_raise_OSError(MP_EIO);
    }
    
    self->started = true;
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_init_obj, 1, machine_twai_init);

// Deinitialize TWAI
static mp_obj_t machine_twai_deinit(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (self->started) {
        twai_stop();
        self->started = false;
    }
    
    if (self->installed) {
        twai_driver_uninstall();
        self->installed = false;
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_deinit_obj, machine_twai_deinit);

// Send CAN message
static mp_obj_t machine_twai_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_data, ARG_id, ARG_rtr, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 0x123} },
        { MP_QSTR_rtr, MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 1000} },
    };
    
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    if (!self->started) {
        mp_raise_OSError(MP_EPERM);
    }
    
    // Prepare message
    twai_message_t message = {0};
    message.identifier = args[ARG_id].u_int;
    message.flags = args[ARG_rtr].u_bool ? TWAI_MSG_FLAG_RTR : 0;
    
    if (!args[ARG_rtr].u_bool) {
        // Data frame
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);
        
        if (bufinfo.len > TWAI_MAX_DATA_LEN) {
            mp_raise_ValueError(MP_ERROR_TEXT("data too long for TCAN332"));
        }
        
        message.data_length_code = bufinfo.len;
        memcpy(message.data, bufinfo.buf, bufinfo.len);
    } else {
        // RTR frame
        message.data_length_code = 0;
    }
    
    // Send message
    esp_err_t ret = twai_transmit(&message, args[ARG_timeout].u_int / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_TIMEOUT) {
            mp_raise_OSError(MP_ETIMEDOUT);
        } else {
            mp_raise_OSError(MP_EIO);
        }
    }
    
    self->tx_count++;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_send_obj, 1, machine_twai_send);

// Receive CAN message
static mp_obj_t machine_twai_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 1000} },
    };
    
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    if (!self->started) {
        mp_raise_OSError(MP_EPERM);
    }
    
    twai_message_t message;
    esp_err_t ret = twai_receive(&message, args[ARG_timeout].u_int / portTICK_PERIOD_MS);
    
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_TIMEOUT) {
            mp_raise_OSError(MP_ETIMEDOUT);
        } else {
            mp_raise_OSError(MP_EIO);
        }
    }
    
    self->rx_count++;
    
    // Create return tuple: (id, data, rtr)
    mp_obj_t tuple[3];
    tuple[0] = mp_obj_new_int(message.identifier);
    
    if (message.flags & TWAI_MSG_FLAG_RTR) {
        tuple[1] = mp_const_none;
        tuple[2] = mp_const_true;
    } else {
        tuple[1] = mp_obj_new_bytes(message.data, message.data_length_code);
        tuple[2] = mp_const_false;
    }
    
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_twai_recv_obj, 1, machine_twai_recv);

// Get statistics
static mp_obj_t machine_twai_stats(mp_obj_t self_in) {
    machine_twai_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    mp_obj_t dict = mp_obj_new_dict(5);
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_tx_count), mp_obj_new_int(self->tx_count));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_rx_count), mp_obj_new_int(self->rx_count));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_error_count), mp_obj_new_int(self->error_count));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_tx_pin), mp_obj_new_int(self->tx_pin));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_rx_pin), mp_obj_new_int(self->rx_pin));
    
    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_twai_stats_obj, machine_twai_stats);

// Constructor
static mp_obj_t machine_twai_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Return singleton object
    return MP_OBJ_FROM_PTR(&machine_twai_obj);
}

static const mp_rom_map_elem_t machine_twai_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_twai_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_twai_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&machine_twai_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&machine_twai_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_stats), MP_ROM_PTR(&machine_twai_stats_obj) },
    
    // Mode constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL), MP_ROM_INT(TWAI_MODE_NORMAL) },
    { MP_ROM_QSTR(MP_QSTR_LISTEN_ONLY), MP_ROM_INT(TWAI_MODE_LISTEN_ONLY) },
    { MP_ROM_QSTR(MP_QSTR_NO_ACK), MP_ROM_INT(TWAI_MODE_NO_ACK) },
};
static MP_DEFINE_CONST_DICT(machine_twai_locals_dict, machine_twai_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_twai_type,
    MP_QSTR_TWAI,
    MP_TYPE_FLAG_NONE,
    make_new, machine_twai_make_new,
    locals_dict, &machine_twai_locals_dict
);
