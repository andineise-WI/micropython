@echo off
echo ==========================================
echo MicroPython ESP32 Build Workaround
echo ==========================================

echo Fixing Windows-specific qstr generation issues...

REM Setze ESP-IDF Umgebung
call "C:\Users\admin\git\esp-idf\export.bat"

REM Navigiere zum MicroPython ESP32 Port
cd "C:\Users\admin\git\micropython-1\ports\esp32"

echo.
echo Attempting build with workarounds for Windows...

REM Versuche 1: Standard Generic ESP32 ohne SPIRAM
echo [1/3] Trying ESP32_GENERIC build...
idf.py -D MICROPY_BOARD=ESP32_GENERIC clean build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================
    echo SUCCESS: ESP32_GENERIC Build Complete!
    echo ==========================================
    goto :build_success
)

echo [1/3] ESP32_GENERIC build failed, trying with SPIRAM...

REM Versuche 2: Mit SPIRAM
echo [2/3] Trying ESP32_GENERIC with SPIRAM...
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM clean build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================
    echo SUCCESS: ESP32_GENERIC+SPIRAM Build Complete!
    echo ==========================================
    goto :build_success
)

echo [2/3] SPIRAM build failed, trying manual qstr generation...

REM Versuche 3: Manuelle qstr Generation
echo [3/3] Trying manual qstr pre-generation...

REM Erstelle leere qstr Dateien falls sie nicht existieren
if not exist build\genhdr\ mkdir build\genhdr\
echo // Auto-generated empty qstr file > build\genhdr\qstr.i.last
echo // Auto-generated empty qstr file > build\genhdr\qstrdefs.generated.h

REM Versuche Build erneut
idf.py -D MICROPY_BOARD=ESP32_GENERIC build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ==========================================
    echo SUCCESS: Manual qstr workaround worked!
    echo ==========================================
    goto :build_success
)

echo.
echo ==========================================
echo ALL BUILD ATTEMPTS FAILED
echo ==========================================
echo.
echo Windows-spezifische Build-Probleme erkannt.
echo.
echo Lösungsvorschläge:
echo 1. WSL2 verwenden (empfohlen)
echo 2. ESP-IDF neu installieren
echo 3. Vorkompilierte Firmware verwenden
echo.
echo Für WSL2 Setup:
echo   wsl --install
echo   wsl --install -d Ubuntu
echo.
goto :end

:build_success
echo.
echo Firmware-Dateien:
if exist build\firmware.bin (
    dir build\firmware.bin
    echo.
    echo Firmware-Größe:
    for %%I in (build\firmware.bin) do echo Firmware: %%~zI bytes
)

if exist build\bootloader\bootloader.bin (
    echo Bootloader: Verfügbar
)

if exist build\partition_table\partition-table.bin (
    echo Partition Table: Verfügbar
)

echo.
echo Flash-Befehle:
echo ==========================================
echo idf.py -p COM3 flash
echo idf.py -p COM3 monitor
echo.
echo TWAI/CAN Test:
echo from machine import TWAI
echo can = TWAI(tx=21, rx=22, baudrate=500000)
echo can.init()
echo print("TWAI ready!")

:end
echo.
pause
