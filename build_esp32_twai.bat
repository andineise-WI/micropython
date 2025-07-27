@echo off
REM ESP32 TWAI Build Script für Windows
REM Startet den Build-Prozess in WSL

echo ===============================================
echo ESP32 TWAI/CAN Build mit TCAN332 Transceiver
echo ===============================================
echo.

REM WSL prüfen
echo Pruefe WSL...
wsl --list >nul 2>&1
if %errorlevel% neq 0 (
    echo FEHLER: WSL ist nicht installiert oder verfuegbar
    echo Installieren Sie WSL mit: wsl --install
    pause
    exit /b 1
)
echo WSL ist verfuegbar

REM Build-Script prüfen
set BUILD_SCRIPT=C:\Users\admin\git\micropython-1\wsl_build_esp32_twai.sh
if not exist "%BUILD_SCRIPT%" (
    echo FEHLER: Build-Script nicht gefunden: %BUILD_SCRIPT%
    pause
    exit /b 1
)
echo Build-Script gefunden

echo.
echo Starte ESP32 TWAI Build in WSL...
echo TCAN332: GPIO4 (TX), GPIO5 (RX)
echo ESP-IDF: v5.x+ API
echo.

REM Script ausführbar machen und starten
wsl chmod +x /mnt/c/Users/admin/git/micropython-1/wsl_build_esp32_twai.sh
wsl bash /mnt/c/Users/admin/git/micropython-1/wsl_build_esp32_twai.sh

if %errorlevel% equ 0 (
    echo.
    echo ===============================================
    echo BUILD ERFOLGREICH ABGESCHLOSSEN!
    echo ===============================================
    echo.
    echo Firmware-Dateien in WSL:
    echo   ~/micropython/ports/esp32/build/firmware.bin
    echo.
    echo Flash-Befehle in WSL:
    echo   cd ~/micropython/ports/esp32
    echo   idf.py -p /dev/ttyUSB0 flash
    echo   idf.py -p /dev/ttyUSB0 monitor
    echo.
    echo TCAN332 Test:
    echo   from machine import TWAI
    echo   can = TWAI(tx=4, rx=5, baudrate=500000)
    echo   can.init()
    echo.
) else (
    echo.
    echo ===============================================
    echo BUILD FEHLGESCHLAGEN
    echo ===============================================
    echo.
    echo Fehlerbehebung:
    echo 1. WSL neu starten: wsl --shutdown
    echo 2. Build-Cache loeschen: wsl rm -rf ~/micropython/ports/esp32/build
    echo 3. Script manuell: wsl bash /mnt/c/Users/admin/git/micropython-1/wsl_build_esp32_twai.sh
)

echo.
echo Druecken Sie eine Taste zum Beenden...
pause >nul
