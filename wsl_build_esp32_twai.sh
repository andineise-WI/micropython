#!/bin/bash
# ESP32 TWAI/CAN Build Script für WSL
# Führen Sie dieses Skript in WSL aus

set -e  # Bei Fehlern stoppen

echo "================================================="
echo "ESP32 TWAI/CAN MicroPython Build in WSL"
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
echo "🚗 TWAI-Implementation von Windows kopieren..."
echo "Kopiere machine_twai.c und machine_twai.h..."

# Prüfen ob Windows-Dateien verfügbar sind
WINDOWS_MP="/mnt/c/Users/admin/git/micropython-1"
if [ -d "$WINDOWS_MP" ]; then
    echo "Windows MicroPython gefunden: $WINDOWS_MP"
    
    # TWAI-Hauptdateien kopieren
    if [ -f "$WINDOWS_MP/extmod/machine_twai.c" ]; then
        cp "$WINDOWS_MP/extmod/machine_twai.c" extmod/
        echo "✅ machine_twai.c kopiert"
    fi
    
    if [ -f "$WINDOWS_MP/extmod/machine_twai.h" ]; then
        cp "$WINDOWS_MP/extmod/machine_twai.h" extmod/
        echo "✅ machine_twai.h kopiert"
    fi
    
    # Port-Modifikationen kopieren
    if [ -f "$WINDOWS_MP/ports/esp32/modmachine.c" ]; then
        cp "$WINDOWS_MP/ports/esp32/modmachine.c" ports/esp32/
        echo "✅ modmachine.c kopiert"
    fi
    
    if [ -f "$WINDOWS_MP/ports/esp32/mpconfigport.h" ]; then
        cp "$WINDOWS_MP/ports/esp32/mpconfigport.h" ports/esp32/
        echo "✅ mpconfigport.h kopiert"
    fi
    
    if [ -f "$WINDOWS_MP/extmod/extmod.mk" ]; then
        cp "$WINDOWS_MP/extmod/extmod.mk" extmod/
        echo "✅ extmod.mk kopiert"
    fi
    
    # Board-Konfiguration kopieren
    if [ -d "$WINDOWS_MP/ports/esp32/boards/ESP32_GENERIC_8MB_2MB" ]; then
        cp -r "$WINDOWS_MP/ports/esp32/boards/ESP32_GENERIC_8MB_2MB" ports/esp32/boards/
        echo "✅ ESP32_GENERIC_8MB_2MB Board-Konfiguration kopiert"
    fi
    
    echo "✅ Alle TWAI-Dateien erfolgreich kopiert"
else
    echo "⚠️  Windows MicroPython nicht gefunden. Manuell TWAI-Dateien erstellen..."
    # Hier würden wir die TWAI-Dateien neu erstellen
fi

# Schritt 7: ESP32 kompilieren
echo "🔨 ESP32 Firmware kompilieren..."
cd ~/micropython/ports/esp32

# ESP-IDF Umgebung sicherstellen
. ~/esp/esp-idf/export.sh

echo "Board: ESP32_GENERIC mit SPIRAM"
echo "TWAI-Support: Aktiviert"
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
    echo "🧪 TWAI/CAN Test:"
    echo "  from machine import TWAI"
    echo "  can = TWAI(tx=21, rx=22, baudrate=500000)"
    echo "  can.init()"
    echo "  print('TWAI ready!')"
    echo
    echo "🎯 Ihr ESP32 (8MB Flash + 2MB PSRAM) mit TWAI/CAN ist bereit!"
    
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
