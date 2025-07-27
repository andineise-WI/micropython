@echo off
echo ==========================================
echo ESP32 Generic 8MB+2MB PSRAM Build
echo ==========================================

echo Setting up ESP-IDF environment...
call "C:\Users\admin\git\esp-idf\export.bat"

echo.
echo Board Configuration:
echo - Generic ESP32
echo - 8MB Flash
echo - 2MB PSRAM
echo - TWAI/CAN Support enabled
echo - Pins: TX=GPIO21, RX=GPIO22

echo.
echo Current directory: %CD%
echo IDF_PATH: %IDF_PATH%

echo.
echo Testing idf.py...
idf.py --version

echo.
echo Starting build process...
echo Board: ESP32_GENERIC_8MB_2MB
idf.py -D MICROPY_BOARD=ESP32_GENERIC_8MB_2MB build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================
    echo BUILD SUCCESSFUL!
    echo ==========================================
    echo.
    if exist build\firmware.bin (
        echo Firmware Information:
        dir build\firmware.bin
        echo.
        echo Firmware size details:
        python -c "import os; size = os.path.getsize('build/firmware.bin'); print(f'Firmware: {size:,} bytes ({size/1024/1024:.2f} MB)')"
    )
    if exist build\bootloader\bootloader.bin (
        echo Bootloader: Available
    )
    if exist build\partition_table\partition-table.bin (
        echo Partition Table: Available
    )
    echo.
    echo Flash Commands:
    echo ==========================================
    echo Full flash with firmware:
    echo   idf.py -D MICROPY_BOARD=ESP32_GENERIC_8MB_2MB -p COM3 flash
    echo.
    echo Monitor serial output:
    echo   idf.py -p COM3 monitor
    echo.
    echo Test TWAI/CAN functionality:
    echo   ^> from machine import TWAI
    echo   ^> can = TWAI(tx=21, rx=22, baudrate=500000)
    echo   ^> can.init()
    echo   ^> print("TWAI/CAN ready!")
    echo.
) else (
    echo.
    echo ==========================================
    echo BUILD FAILED!
    echo ==========================================
    echo Error level: %ERRORLEVEL%
    echo.
    echo Checking for build logs...
    if exist build\log\ (
        echo Build logs available in: build\log\
        dir build\log\*.log /b 2>nul
    )
    echo.
    echo Troubleshooting:
    echo 1. Check submodules: git submodule update --init --recursive
    echo 2. Clean build: rmdir /s build
    echo 3. Check ESP-IDF version: idf.py --version
)

echo.
echo ESP32 Hardware Specifications:
echo ==========================================
echo - MCU: ESP32-D0WDQ6 (Dual-core Tensilica LX6)
echo - Flash: 8MB SPI Flash
echo - PSRAM: 2MB External SPI SRAM
echo - TWAI/CAN: GPIO 21 (TX), GPIO 22 (RX)
echo - WiFi: IEEE 802.11 b/g/n (2.4 GHz)
echo - Bluetooth: v4.2 BR/EDR and BLE
echo.
echo TWAI/CAN Transceiver needed:
echo - MCP2551/MCP2561 (5V automotive)
echo - SN65HVD230 (3.3V low-power)
echo - TJA1050 (automotive grade)
echo.
pause
