#!/bin/bash
# ESP32 TWAI/CAN Build Script für WSL
# Führen Sie dieses Skript in WSL aus

set -e  # Bei Fehlern stoppen

echo "================================================="
echo "ESP32 TWAI/CAN MicroPython Build in WSL"
echo "Mit TCAN332 Transceiver (Texas Instruments)"
echo "GPIO4 (TX) und GPIO5 (RX)"
echo "================================================="
echo

# Schritt 1: System aktualisieren
echo "🔄 System aktualisieren..."
sudo apt update && sudo apt upgrade -y

# Schritt 2: Dependencies installieren
echo "📦 Build-Dependencies installieren..."
sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Schritt 3: ESP-IDF installieren
echo "🛠️  ESP-IDF installieren..."
if [ ! -d "$HOME/esp/esp-idf" ]; then
    mkdir -p ~/esp
    cd ~/esp
    git clone --recursive https://github.com/espressif/esp-idf.git
    cd esp-idf
    git checkout v5.1.2
    git submodule update --init --recursive
    ./install.sh esp32
    echo "✅ ESP-IDF installiert"
else
    echo "ℹ️  ESP-IDF bereits vorhanden"
fi

# Schritt 4: ESP-IDF Umgebung aktivieren
echo "🔧 ESP-IDF Umgebung aktivieren..."
cd ~/esp/esp-idf
. ./export.sh

# Schritt 5: MicroPython vorbereiten
echo "🐍 MicroPython vorbereiten..."
if [ ! -d "$HOME/micropython" ]; then
    cd ~
    git clone https://github.com/micropython/micropython.git
    cd micropython
    git submodule update --init --recursive
    make -C mpy-cross
    echo "✅ MicroPython vorbereitet"
else
    echo "ℹ️  MicroPython bereits vorhanden"
    cd ~/micropython
fi

# Schritt 6: TWAI-Implementation kopieren
echo "🚗 TWAI-Implementation mit TCAN332-Support von Windows kopieren..."
echo "Kopiere machine_twai.c und machine_twai.h (ESP-IDF v5.x+ kompatibel)..."

# Prüfen ob Windows-Dateien verfügbar sind
WINDOWS_MP="/mnt/c/Users/admin/git/micropython-1"
if [ -d "$WINDOWS_MP" ]; then
    echo "Windows MicroPython gefunden: $WINDOWS_MP"
    
    # TWAI-Hauptdateien kopieren (neue Pfade in ports/esp32)
    if [ -f "$WINDOWS_MP/ports/esp32/machine_twai.c" ]; then
        cp "$WINDOWS_MP/ports/esp32/machine_twai.c" ports/esp32/
        echo "✅ machine_twai.c kopiert (mit TCAN332 GPIO4/5)"
    fi
    
    if [ -f "$WINDOWS_MP/ports/esp32/machine_twai.h" ]; then
        cp "$WINDOWS_MP/ports/esp32/machine_twai.h" ports/esp32/
        echo "✅ machine_twai.h kopiert"
    fi
    
    # Port-Modifikationen kopieren
    if [ -f "$WINDOWS_MP/ports/esp32/modmachine.c" ]; then
        cp "$WINDOWS_MP/ports/esp32/modmachine.c" ports/esp32/
        echo "✅ modmachine.c kopiert"
    fi
    
    if [ -f "$WINDOWS_MP/ports/esp32/mpconfigport.h" ]; then
        cp "$WINDOWS_MP/ports/esp32/mpconfigport.h" ports/esp32/
        echo "✅ mpconfigport.h kopiert (TWAI aktiviert)"
    fi
    
    # Makefile kopieren
    if [ -f "$WINDOWS_MP/ports/esp32/Makefile" ]; then
        cp "$WINDOWS_MP/ports/esp32/Makefile" ports/esp32/
        echo "✅ Makefile kopiert (machine_twai.c eingebunden)"
    fi
    
    # CMake-Konfiguration kopieren (wichtig für machine_twai.c)
    if [ -f "$WINDOWS_MP/ports/esp32/esp32_common.cmake" ]; then
        cp "$WINDOWS_MP/ports/esp32/esp32_common.cmake" ports/esp32/
        echo "✅ esp32_common.cmake kopiert (machine_twai.c in Build eingebunden)"
    fi
    
    # Beispiele und Dokumentation kopieren
    if [ -d "$WINDOWS_MP/examples/esp32" ]; then
        mkdir -p examples
        cp -r "$WINDOWS_MP/examples/esp32" examples/
        echo "✅ TCAN332-Beispiele kopiert"
    fi
    
    if [ -d "$WINDOWS_MP/docs/esp32" ]; then
        mkdir -p docs
        cp -r "$WINDOWS_MP/docs/esp32" docs/
        echo "✅ TCAN332-Dokumentation kopiert"
    fi
    
    echo "✅ Alle TWAI-Dateien mit TCAN332-Support erfolgreich kopiert"
else
    echo "⚠️  Windows MicroPython nicht gefunden. Git Repository verwenden..."
    echo "ℹ️  Code bereits ins Git gepusht - Update durchführen..."
    git pull origin master
    echo "✅ TWAI-Code aus Git Repository aktualisiert"
fi

# Schritt 7: ESP32 kompilieren
echo "🔨 ESP32 Firmware kompilieren..."
cd ~/micropython/ports/esp32

# ESP-IDF Umgebung sicherstellen
. ~/esp/esp-idf/export.sh

echo "Board: ESP32_GENERIC mit SPIRAM"
echo "TWAI-Support: Aktiviert mit TCAN332 (GPIO4/5)"
echo "ESP-IDF: v5.x+ API kompatibel"
echo

# Build ausführen
echo "🚀 Build starten..."
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM build

if [ $? -eq 0 ]; then
    echo
    echo "================================================="
    echo "🎉 BUILD ERFOLGREICH!"
    echo "================================================="
    echo
    
    # Firmware-Info anzeigen
    if [ -f "build/firmware.bin" ]; then
        SIZE=$(stat -c%s build/firmware.bin)
        echo "📱 Firmware: build/firmware.bin ($(($SIZE / 1024)) KB)"
    fi
    
    if [ -f "build/bootloader/bootloader.bin" ]; then
        echo "🔧 Bootloader: build/bootloader/bootloader.bin"
    fi
    
    if [ -f "build/partition_table/partition-table.bin" ]; then
        echo "📋 Partition Table: build/partition_table/partition-table.bin"
    fi
    
    echo
    echo "📡 Flash-Befehle:"
    echo "  idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM -p /dev/ttyS3 flash"
    echo "  idf.py -p /dev/ttyS3 monitor"
    echo
    echo "🧪 TWAI/CAN Test mit TCAN332:"
    echo "  from machine import TWAI"
    echo "  can = TWAI(tx=4, rx=5, baudrate=500000)  # TCAN332 GPIO4/5"
    echo "  can.init()"
    echo "  print('TCAN332 ready!')"
    echo "  can.send(b'\\x01\\x02\\x03\\x04', id=0x123)"
    echo
    echo "🎯 Ihr ESP32 mit TCAN332 (Texas Instruments) ist bereit!"
    echo "📖 Siehe: docs/esp32/README_TCAN332.md für Details"
    
else
    echo
    echo "================================================="
    echo "❌ BUILD FEHLGESCHLAGEN"
    echo "================================================="
    echo
    echo "Mögliche Lösungen:"
    echo "1. ESP-IDF erneut aktivieren: . ~/esp/esp-idf/export.sh"
    echo "2. Build-Cache löschen: rm -rf build"
    echo "3. Submodule aktualisieren: git submodule update --init --recursive"
    echo
    exit 1
fi
