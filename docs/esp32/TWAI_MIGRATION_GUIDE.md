# ESP32 TWAI Migration Guide: ESP-IDF v4.x to v5.x+

This guide helps you migrate your ESP32 TWAI (CAN) code from the old ESP-IDF v4.x API to the new ESP-IDF v5.x+ API.

## Hardware Configuration

### Default GPIO Assignment
The ESP32 TWAI implementation uses the following default GPIO pins optimized for TCAN332 transceiver (Texas Instruments):

- **TX Pin:** GPIO 4 (connects to TCAN332 TXD pin 1)
- **RX Pin:** GPIO 5 (connects to TCAN332 RXD pin 4)

### TCAN332 Transceiver Wiring (Texas Instruments SO-8)
```
ESP32        TCAN332
------       --------
GPIO4   ->   TXD (pin 1)
GPIO5   ->   RXD (pin 4)  
3.3V    ->   VCC (pin 3)
GND     ->   GND (pin 2)
             CAN_H (pin 7) -> CAN Bus High
             CAN_L (pin 6) -> CAN Bus Low
```

**Important:** 120Ω termination resistors are required at each end of the CAN bus.
**Datasheet:** https://www.ti.com/product/TCAN332

## API Changes Overview

### Header Files
**Old API (ESP-IDF v4.x):**
```c
#include "driver/twai.h"
```

**New API (ESP-IDF v5.x+):**
```c
#include "esp_driver_twai.h"
#include "driver/twai_onchip.h"  // For on-chip TWAI controller
```

### Driver Installation/Initialization

**Old API:**
```c
// Old configuration structures
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

// Install driver
esp_err_t ret = twai_driver_install(&g_config, &t_config, &f_config);
```

**New API:**
```c
// New node-based configuration
twai_onchip_node_config_t node_config = {
    .general_config = {
        .tx_io = GPIO_NUM_21,
        .rx_io = GPIO_NUM_22,
        .mode = TWAI_MODE_NORMAL,
        .intr_priority = 0
    },
    .timing_config = {
        .baudrate = 500000,
        .tseg_1 = 13,
        .tseg_2 = 2,
        .sjw = 1,
        .triple_sampling = false
    },
    .filter_config = {
        .acceptance_filter = {
            .code = 0,
            .mask = 0x1FFFFFFF,
            .single_filter = true
        }
    }
};

// Create node handle
twai_node_handle_t node_handle;
esp_err_t ret = twai_new_node_onchip(&node_config, &node_handle);
```

### Message Structures

**Old API:**
```c
twai_message_t message = {
    .identifier = 0x123,
    .data_length_code = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};
```

**New API:**
```c
twai_frame_t frame = {
    .id = 0x123,
    .dlc = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};
```

### Send/Receive Operations

**Old API:**
```c
// Send
esp_err_t ret = twai_transmit(&message, pdMS_TO_TICKS(1000));

// Receive
twai_message_t rx_message;
esp_err_t ret = twai_receive(&rx_message, pdMS_TO_TICKS(1000));
```

**New API:**
```c
// Send
esp_err_t ret = twai_transmit(node_handle, &frame, pdMS_TO_TICKS(1000));

// Receive (requires callback setup)
twai_frame_t rx_frame;
// Receiving is now primarily callback-based
```

### Callback System (New Feature)

**New API Only:**
```c
// Event callback structure
twai_event_callbacks_t callbacks = {
    .on_tx_done = tx_done_callback,
    .on_rx_done = rx_done_callback,
    .on_state_change = state_change_callback,
    .on_error = error_callback
};

// Register callbacks
esp_err_t ret = twai_register_event_callbacks(node_handle, &callbacks, NULL);
```

### Status and Information

**Old API:**
```c
twai_status_info_t status_info;
esp_err_t ret = twai_get_status_info(&status_info);
```

**New API:**
```c
twai_node_status_t node_status;
esp_err_t ret = twai_get_node_status(node_handle, &node_status);
```

### Driver Control

**Old API:**
```c
// Start
esp_err_t ret = twai_start();

// Stop
esp_err_t ret = twai_stop();

// Uninstall
esp_err_t ret = twai_driver_uninstall();
```

**New API:**
```c
// Start
esp_err_t ret = twai_start(node_handle);

// Stop
esp_err_t ret = twai_stop(node_handle);

// Delete node
esp_err_t ret = twai_del_node(node_handle);
```

## MicroPython API Changes

### Constructor

**Old Style:**
```python
from machine import TWAI

twai = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
```

**New Style (ESP-IDF v5.x+ with TCAN332):**
```python
from machine import TWAI

# Default pins optimized for TCAN332: GPIO4 (TX), GPIO5 (RX)
# Mode constants changed: 0=NORMAL, 1=NO_ACK, 2=LISTEN_ONLY
twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)  # 0 = NORMAL
```

### State Constants

**Old Constants:**
```python
TWAI.STATE_STOPPED
TWAI.STATE_RUNNING
TWAI.STATE_BUS_OFF
TWAI.STATE_RECOVERING
```

**New Constants:**
```python
TWAI.ERROR_ACTIVE
TWAI.ERROR_WARNING  
TWAI.ERROR_PASSIVE
TWAI.ERROR_BUS_OFF
```

### Filter Configuration

**Old API:**
```python
# Single filter
twai.setfilter(0, 0x7FF, 0x123)
# Dual filter
twai.setfilter(1, 0x7FF, 0x123, 0x456)
```

**New API:**
```python
# Single filter (mode 0)
twai.setfilter(mode=0, mask=0x7FF, id1=0x123)
# Dual filter (mode 1)
twai.setfilter(mode=1, mask=0x7FF, id1=0x123, id2=0x456)
```

### Statistics

**Old Format:**
```python
stats = twai.stats()
# Returns dictionary with old key names
```

**New Format:**
```python
stats = twai.stats()
# Returns dictionary with new key names compatible with ESP-IDF v5.x+
# Key names may have changed to match new API
```

## Migration Checklist

### For C Code:
- [ ] Update header includes from `driver/twai.h` to `esp_driver_twai.h`
- [ ] Replace driver installation with node creation
- [ ] Update configuration structures to new format
- [ ] Change `twai_message_t` to `twai_frame_t`
- [ ] Update function calls to include node handle
- [ ] Implement callback system for events
- [ ] Update status checking code
- [ ] Test with ESP-IDF v5.x+

### For MicroPython Code:
- [ ] Update mode constants (use integers: 0, 1, 2)
- [ ] Update state checking to use new constants
- [ ] Update filter configuration to use named parameters
- [ ] Test callback functionality
- [ ] Verify statistics format
- [ ] Update error handling for new exceptions
- [ ] Test with ESP-IDF v5.x+ firmware

## Common Issues and Solutions

### 1. Compilation Errors
**Problem:** `fatal error: driver/twai.h: No such file or directory`

**Solution:** Update includes to:
```c
#include "esp_driver_twai.h"
#include "driver/twai_onchip.h"
```

### 2. Link Errors
**Problem:** Undefined references to TWAI functions

**Solution:** Add to CMakeLists.txt:
```cmake
idf_component_register(
    SRCS "..."
    REQUIRES esp_driver_twai
)
```

### 3. Runtime Initialization Errors
**Problem:** TWAI initialization fails

**Solution:** Check that:
- GPIO pins are correctly configured
- Node configuration structure is properly initialized
- ESP-IDF version is v5.0 or later

### 4. Callback Not Working
**Problem:** RX callbacks not triggered

**Solution:** Ensure:
- Callbacks are properly registered
- Node is started after callback registration
- Proper message filtering is configured

## Version Compatibility

| ESP-IDF Version | API Type | Status |
|----------------|----------|---------|
| v4.x and earlier | Old driver/twai.h | ⚠️ Legacy |
| v5.0 - v5.2 | New esp_driver_twai | ✅ Supported |
| v5.3+ | New esp_driver_twai | ✅ Recommended |

## Testing Your Migration

1. **Compile Test:** Ensure code compiles without errors
2. **Basic Functionality:** Test send/receive operations
3. **Callback System:** Verify event callbacks work
4. **Error Handling:** Test bus error recovery
5. **Performance:** Check if timing requirements are met

## Additional Resources

- [ESP-IDF TWAI Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html)
- [ESP-IDF Migration Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/migration-guides/index.html)
- [MicroPython ESP32 Documentation](https://docs.micropython.org/en/latest/esp32/quickref.html)

---

**Note:** This migration is required for ESP-IDF v5.x+ compatibility. The old API is deprecated and will not work with newer ESP-IDF versions.
