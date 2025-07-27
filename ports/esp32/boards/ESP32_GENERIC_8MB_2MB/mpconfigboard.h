// Generic ESP32 Board Configuration with 8MB Flash + 2MB PSRAM
#define MICROPY_HW_BOARD_NAME               "ESP32 Generic 8MB+2MB PSRAM"
#define MICROPY_HW_MCU_NAME                 "ESP32"

// Enable PSRAM support
#define CONFIG_SPIRAM_SUPPORT               1
#define CONFIG_SPIRAM_USE                   1
#define MICROPY_HW_PSRAM_SIZE               (2 * 1024 * 1024)  // 2MB PSRAM

// Flash size configuration  
#define MICROPY_HW_FLASH_SIZE               (8 * 1024 * 1024)  // 8MB Flash

// Enable TWAI/CAN support
#define MICROPY_PY_MACHINE_TWAI             (1)

// Default TWAI pins for generic ESP32
#define MICROPY_HW_TWAI_TX                  (21)
#define MICROPY_HW_TWAI_RX                  (22)

// Enable additional features with more memory
#define MICROPY_PY_BTREE                    (1)
#define MICROPY_PY_CRYPTOLIB                (1)
#define MICROPY_PY_HASHLIB_SHA1             (1)
#define MICROPY_PY_HASHLIB_SHA256           (1)
#define MICROPY_PY_MACHINE_PWM_DUTY_U16     (1)

// Networking support
#define MICROPY_PY_NETWORK_WLAN             (1)
#define MICROPY_PY_NETWORK_LAN              (0)

// USB support (if available on your board)
#define MICROPY_HW_USB_CDC                  (0)
