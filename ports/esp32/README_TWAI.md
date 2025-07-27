# TWAI (Two-Wire Automotive Interface) Support for MicroPython ESP32

This implementation adds TWAI (Two-Wire Automotive Interface) support to MicroPython for ESP32 microcontrollers. TWAI is ESP32's implementation of the CAN 2.0 protocol, commonly used in automotive and industrial applications.

## Features

- Full CAN 2.0A and CAN 2.0B support (standard and extended frames)
- Multiple operating modes (Normal, No-ACK, Listen-Only)
- Configurable message filtering
- Hardware error handling and bus-off recovery
- Support for Remote Transmission Requests (RTR)
- Comprehensive status monitoring
- Standard baudrates from 25 kbit/s to 1 Mbit/s

## Hardware Requirements

The ESP32 TWAI peripheral requires external CAN transceivers to interface with a physical CAN bus. Common transceivers include:

- **MCP2551/MCP2561** - 5V tolerant, suitable for automotive applications
- **SN65HVD230/SN65HVD231** - 3.3V transceivers for low-power applications  
- **TJA1050/TJA1051** - Automotive-grade transceivers

### Typical Wiring

```
ESP32 GPIO21 (TX) -----> CTX (transceiver input)
ESP32 GPIO22 (RX) <----- CRX (transceiver output)
CAN_H <----------------> CAN bus H line  
CAN_L <----------------> CAN bus L line
```

**Note:** CAN bus requires 120Ω termination resistors at both ends of the bus.

## Files Added/Modified

### New Files

- `ports/esp32/machine_twai.c` - Main TWAI implementation
- `ports/esp32/machine_twai.h` - TWAI header file
- `docs/library/machine.TWAI.rst` - Documentation
- `tests/ports/esp32/test_twai.py` - Test suite
- `examples/esp32/twai_example.py` - Usage examples

### Modified Files

- `ports/esp32/modmachine.h` - Added TWAI type declaration
- `ports/esp32/modmachine.c` - Added TWAI to machine module
- `ports/esp32/mpconfigport.h` - Added TWAI configuration
- `ports/esp32/esp32_common.cmake` - Added TWAI source file
- `docs/library/machine.rst` - Added TWAI documentation link

## API Overview

### Basic Usage

```python
from machine import TWAI

# Initialize TWAI
can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
can.init()

# Send a message
can.send(b"Hello", 0x123)

# Receive a message  
data, msg_id, extframe, rtr = can.recv(timeout=1000)

# Clean up
can.deinit()
```

### Methods

- `TWAI(tx, rx, baudrate, mode)` - Constructor
- `init()` - Initialize the TWAI peripheral
- `deinit()` - Deinitialize and free resources
- `send(data, id, timeout, extframe, rtr)` - Send CAN message
- `recv(timeout)` - Receive CAN message
- `any()` - Check for pending messages
- `setfilter(mode, mask, id1, id2)` - Configure message filtering
- `state()` - Get bus state
- `info()` - Get detailed status information
- `restart()` - Restart after bus-off condition

### Constants

**Operating Modes:**
- `TWAI.NORMAL` - Normal operation with ACK
- `TWAI.NO_ACK` - No acknowledgment mode
- `TWAI.LISTEN_ONLY` - Receive-only mode

**Filter Modes:**
- `TWAI.FILTER_SINGLE` - Single acceptance filter
- `TWAI.FILTER_DUAL` - Dual acceptance filters

## Testing

Run the test suite with:

```python
import test_twai
test_twai.main()
```

For testing without a CAN bus, use NO_ACK mode:

```python
can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NO_ACK)
```

## Configuration

TWAI support is automatically enabled on ESP32 targets that support it (SOC_TWAI_SUPPORTED). 
It can be disabled by setting:

```c
#define MICROPY_PY_MACHINE_TWAI (0)
```

## Supported ESP32 Variants

- ESP32 (original)
- ESP32-S2 
- ESP32-S3
- ESP32-C3
- ESP32-C6

Note: Check ESP-IDF documentation for specific SOC support as it may vary by IDF version.

## Error Handling

The implementation raises `OSError` exceptions for various conditions:

- `ETIMEDOUT` - Operation timed out
- `EINVAL` - Invalid parameters or state
- `EPERM` - Operation not permitted
- `EIO` - General I/O error

Bus errors (bit errors, stuff errors, CRC errors, etc.) are automatically handled by the ESP32 TWAI hardware.

## Limitations

- Maximum data length: 8 bytes (CAN 2.0 limitation)
- No CAN-FD support (ESP32 hardware limitation)
- Single TWAI controller per ESP32
- Requires external transceiver for bus connection

## Future Enhancements

Potential improvements for future versions:

- Callback-based message reception
- Advanced error statistics
- Bus load monitoring
- Sleep/wake functionality
- Integration with ESP32's power management

## References

- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [ESP-IDF TWAI Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html)
- [CAN 2.0 Specification](https://www.kvaser.com/about-can/the-can-protocol/)
