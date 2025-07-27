/**
 * ESP32 TWAI (CAN) MicroPython Implementation - ESP-IDF v5.x+ Compatible
 * 
 * This implementation is updated for the new ESP-IDF TWAI API introduced in v5.x.
 * The API has been significantly changed from the old driver/twai.h to the new
 * esp_twai.h and esp_twai_onchip.h architecture.
 * 
 * MAJOR API CHANGES IN ESP-IDF v5.x:
 * 
 * 1. DRIVER ARCHITECTURE:
 *    - Old: twai_driver_install(), twai_start(), twai_stop()
 *    - New: twai_new_node_onchip(), twai_node_enable(), twai_node_disable()
 * 
 * 2. CONFIGURATION STRUCTURES:
 *    - Old: twai_general_config_t, twai_timing_config_t, twai_filter_config_t
 *    - New: twai_onchip_node_config_t with embedded timing and I/O config
 * 
 * 3. MESSAGE HANDLING:
 *    - Old: twai_message_t with embedded data array
 *    - New: twai_frame_t with pointer-based buffer system
 * 
 * 4. INTERRUPT HANDLING:
 *    - Old: Polling-based with twai_read_alerts()
 *    - New: Event callback system with twai_event_callbacks_t
 * 
 * 5. TIMING CONFIGURATION:
 *    - Old: Predefined macros like TWAI_TIMING_CONFIG_500KBITS()
 *    - New: Basic timing config with bitrate, sample point percentages
 * 
 * 6. FILTER CONFIGURATION:
 *    - Old: twai_filter_config_t with acceptance_code/acceptance_mask
 *    - New: twai_mask_filter_config_t with id/mask pairs
 * 
 * 7. STATUS INFORMATION:
 *    - Old: twai_status_info_t from twai_get_status_info()
 *    - New: twai_node_status_t and twai_node_record_t from twai_node_get_info()
 * 
 * COMPATIBILITY REQUIREMENTS:
 * - ESP-IDF v5.0 or later
 * - Component dependency: esp_driver_twai
 * - Header files: esp_twai.h, esp_twai_onchip.h, esp_twai_types.h
 * 
 * FEATURES IMPLEMENTED:
 * ✅ Node creation and management
 * ✅ Event callback system (TX, RX, state change, error)
 * ✅ Frame transmission and reception
 * ✅ Filter configuration (single and dual mode)
 * ✅ Bus error handling and recovery
 * ✅ Statistics and status monitoring
 * ✅ Queue management (software-based for RX)
 * ✅ Multiple operating modes (normal, no-ack, listen-only)
 * 
 * FEATURES NOT AVAILABLE IN NEW API:
 * ❌ Direct queue clearing (twai_clear_transmit_queue/receive_queue)
 * ❌ Alert-based interrupt handling
 * ❌ Direct arbitration lost count
 * ❌ Manual timing parameter configuration
 * 
 * BACKWARD COMPATIBILITY NOTES:
 * - Mode constants changed from TWAI_MODE_* to simple integers
 * - State constants now use TWAI_ERROR_* enum values
 * - Filter constants simplified to 0/1 for single/dual
 * - Some statistics may not be available or have different meanings
 * 
 * MIGRATION GUIDE:
 * 1. Update ESP-IDF to v5.0+
 * 2. Add esp_driver_twai to component dependencies
 * 3. Replace old header includes with new ones
 * 4. Update code to use new callback-based reception
 * 5. Adjust filter configuration for new API
 * 6. Update error handling for new error types
 * 
 * TESTING REQUIREMENTS:
 * - Verify callback functionality with real CAN traffic
 * - Test filter configuration with various ID patterns
 * - Validate bus recovery under error conditions
 * - Confirm statistics accuracy across all modes
 * - Test queue overflow handling
 * 
 * PERFORMANCE NOTES:
 * - Callback-based RX is more efficient than polling
 * - Software RX queue adds minimal overhead
 * - Event system reduces CPU usage during idle periods
 * - Filter processing is now handled in hardware driver
 */

#ifndef ESP_IDF_VERSION_MAJOR
#error "ESP-IDF version information not available"
#endif

#if ESP_IDF_VERSION_MAJOR < 5
#error "This TWAI implementation requires ESP-IDF v5.0 or later"
#endif

// Version compatibility check for specific features
#if ESP_IDF_VERSION_MAJOR == 5 && ESP_IDF_VERSION_MINOR == 0
#warning "ESP-IDF v5.0 detected - some advanced features may not be available"
#endif

/**
 * USAGE EXAMPLES:
 * 
 * Basic setup:
 * ```python
 * from machine import TWAI
 * 
 * # Create and initialize
 * can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
 * can.init()
 * 
 * # Send message
 * can.send(b'\x01\x02\x03\x04', id=0x123)
 * 
 * # Receive message
 * data, msg_id, ext, rtr = can.recv(timeout=1000)
 * 
 * # With callback
 * def on_rx(status):
 *     if status == 0:  # New message
 *         data, msg_id, ext, rtr = can.recv(timeout=0)
 *         print(f"RX: ID=0x{msg_id:X}, Data={data}")
 * 
 * can.rxcallback(on_rx)
 * ```
 * 
 * Advanced features:
 * ```python
 * # Filter configuration
 * can.setfilter(mode=0, mask=0x7F0, id1=0x100)  # Single filter
 * can.setfilter(mode=1, mask=0x7F0, id1=0x100, id2=0x200)  # Dual filter
 * 
 * # Statistics monitoring
 * stats = can.stats()
 * print(f"TX: {stats['msg_tx_count']}, RX: {stats['msg_rx_count']}")
 * 
 * # Bus recovery
 * if can.state() == TWAI.ERROR_BUS_OFF:
 *     can.restart()
 * ```
 */
