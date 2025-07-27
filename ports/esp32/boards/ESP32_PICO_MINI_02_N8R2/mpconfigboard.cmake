# ESP32-PICO-MINI-02-N8R2 board configuration

set(IDF_TARGET esp32)

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.spiram
    boards/ESP32_PICO_MINI_02_N8R2/sdkconfig.board
)
