@echo off
echo ==========================================
echo ESP32-PICO-MINI-02-N8R2 Build Test
echo ==========================================

echo Setting up ESP-IDF environment...
call "C:\Users\admin\git\esp-idf\export.bat"

echo.
echo Current directory: %CD%
echo IDF_PATH: %IDF_PATH%
echo MICROPY_BOARD: ESP32_PICO_MINI_02_N8R2

echo.
echo Testing idf.py version...
idf.py --version

echo.
echo Starting build process...
idf.py -D MICROPY_BOARD=ESP32_PICO_MINI_02_N8R2 build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================
    echo BUILD SUCCESSFUL!
    echo ==========================================
    echo.
    echo Checking build output:
    if exist build\firmware.bin (
        echo Firmware size:
        dir build\firmware.bin
    )
    echo.
    echo To flash firmware:
    echo idf.py -D MICROPY_BOARD=ESP32_PICO_MINI_02_N8R2 -p COM3 flash
) else (
    echo.
    echo ==========================================
    echo BUILD FAILED!
    echo ==========================================
    echo Error level: %ERRORLEVEL%
)

echo.
echo Press any key to continue...
pause >nul
