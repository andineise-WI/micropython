@echo off
REM ESP32-PICO-MINI-02-N8R2 Build Script for Windows

echo ========================================
echo ESP32-PICO-MINI-02-N8R2 Firmware Build
echo ========================================

REM Check if ESP-IDF is setup
if "%IDF_PATH%"=="" (
    echo.
    echo FEHLER: ESP-IDF ist nicht konfiguriert!
    echo.
    echo Bitte führen Sie zuerst aus:
    echo   C:\esp\esp-idf\export.bat
    echo.
    echo Oder installieren Sie ESP-IDF:
    echo   https://dl.espressif.com/dl/esp-idf/
    echo.
    pause
    exit /b 1
)

echo ESP-IDF Pfad: %IDF_PATH%
echo ESP-IDF Version:
idf.py --version

REM Check if we're in the ESP32 port directory
if not exist "mpconfigport.h" (
    echo FEHLER: Bitte führen Sie dieses Skript aus dem ports\esp32 Verzeichnis aus
    pause
    exit /b 1
)

REM Set board name
set BOARD=ESP32_PICO_MINI_02_N8R2
echo Board: %BOARD%

REM Clean previous build if requested
if "%1"=="clean" (
    echo.
    echo Vorherigen Build löschen...
    rmdir /s /q build 2>nul
    echo Build-Verzeichnis gelöscht.
)

echo.
echo ========================================
echo Firmware kompilieren...
echo ========================================

REM Build using idf.py
idf.py -D MICROPY_BOARD=%BOARD% build

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo ✓ Build erfolgreich abgeschlossen!
    echo ========================================
    echo.
    echo Firmware-Dateien:
    if exist "build\firmware.bin" (
        for %%F in (build\firmware.bin) do echo   Firmware: %%~zF Bytes - %%F
    )
    if exist "build\bootloader\bootloader.bin" (
        echo   Bootloader: build\bootloader\bootloader.bin
    )
    if exist "build\partition_table\partition-table.bin" (
        echo   Partitionstabelle: build\partition_table\partition-table.bin
    )
    echo.
    echo Flashen mit:
    echo   idf.py -D MICROPY_BOARD=%BOARD% -p COM^<port^> flash
    echo.
    echo Beispiel für COM3:
    echo   idf.py -D MICROPY_BOARD=%BOARD% -p COM3 flash
    echo.
    echo Monitor öffnen:
    echo   idf.py -p COM3 monitor
    echo.
    echo TWAI-Funktionalität testen:
    echo   ^>^>^> from machine import TWAI
    echo   ^>^>^> can = TWAI^(tx=21, rx=22, baudrate=500000^)
    echo   ^>^>^> can.init^(^)
    echo.
) else (
    echo.
    echo ========================================
    echo ✗ Build fehlgeschlagen!
    echo ========================================
    echo.
    echo Mögliche Lösungen:
    echo 1. Prüfen Sie die ESP-IDF Installation
    echo 2. Build-Verzeichnis löschen: %0 clean
    echo 3. ESP-IDF Umgebung neu laden: C:\esp\esp-idf\export.bat
    echo.
    pause
    exit /b 1
)

echo.
echo ESP32-PICO-MINI-02-N8R2 Spezifikationen:
echo - Chip: ESP32-PICO-D4
echo - Flash: 8MB
echo - PSRAM: 2MB QSPI
echo - TWAI/CAN: GPIO 21 ^(TX^), GPIO 22 ^(RX^)
echo - Package: 7x7mm LGA

pause
