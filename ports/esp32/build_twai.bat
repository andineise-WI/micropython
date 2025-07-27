@echo off
REM Build script for MicroPython ESP32 with TWAI support (Windows)

echo === MicroPython ESP32 TWAI Build Script ===

REM Check if ESP-IDF is setup
if "%IDF_PATH%"=="" (
    echo Error: ESP-IDF environment not set up!
    echo Please run the ESP-IDF setup script first
    pause
    exit /b 1
)

echo ESP-IDF Path: %IDF_PATH%

REM Check if we're in the ESP32 port directory
if not exist "mpconfigport.h" (
    echo Error: Please run this script from the ports\esp32 directory
    pause
    exit /b 1
)

REM Set default board if not specified
if "%1"=="" (
    set BOARD=ESP32_GENERIC
) else (
    set BOARD=%1
)
echo Building for board: %BOARD%

REM Clean previous build if requested
if "%2"=="clean" (
    echo Cleaning previous build...
    rmdir /s /q build 2>nul
)

REM Build the firmware
echo Building MicroPython firmware...
make BOARD=%BOARD%

if %errorlevel% equ 0 (
    echo === Build completed successfully! ===
    echo Firmware location: build\firmware.bin
    echo Flash with: make BOARD=%BOARD% deploy
) else (
    echo === Build failed! ===
    pause
    exit /b 1
)

echo === TWAI Usage Example ===
echo After flashing, you can test TWAI with:
echo ^>^>^> from machine import TWAI
echo ^>^>^> can = TWAI^(tx=21, rx=22, baudrate=500000^)
echo ^>^>^> can.init^(^)
echo ^>^>^> # Now ready for CAN communication!

pause
