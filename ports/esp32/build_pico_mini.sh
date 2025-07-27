#!/bin/bash

# ESP32-PICO-MINI-02-N8R2 Build Script for Linux/macOS

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ESP32-PICO-MINI-02-N8R2 Firmware Build${NC}"
echo -e "${BLUE}========================================${NC}"

# Check if ESP-IDF is setup
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}FEHLER: ESP-IDF ist nicht konfiguriert!${NC}"
    echo ""
    echo "Bitte führen Sie zuerst aus:"
    echo "  source ~/esp/esp-idf/export.sh"
    echo ""
    echo "Oder installieren Sie ESP-IDF:"
    echo "  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
    echo ""
    exit 1
fi

echo -e "${GREEN}ESP-IDF Pfad: $IDF_PATH${NC}"
echo -e "${GREEN}ESP-IDF Version:${NC}"
idf.py --version

# Check if we're in the ESP32 port directory
if [ ! -f "mpconfigport.h" ]; then
    echo -e "${RED}FEHLER: Bitte führen Sie dieses Skript aus dem ports/esp32 Verzeichnis aus${NC}"
    exit 1
fi

# Set board name
BOARD=ESP32_PICO_MINI_02_N8R2
echo -e "${GREEN}Board: $BOARD${NC}"

# Clean previous build if requested
if [ "$1" == "clean" ]; then
    echo ""
    echo -e "${YELLOW}Vorherigen Build löschen...${NC}"
    rm -rf build
    echo "Build-Verzeichnis gelöscht."
fi

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Firmware kompilieren...${NC}"
echo -e "${BLUE}========================================${NC}"

# Build using idf.py
idf.py -D MICROPY_BOARD=$BOARD build

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Build erfolgreich abgeschlossen!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Firmware-Dateien:"
    if [ -f "build/firmware.bin" ]; then
        SIZE=$(du -h build/firmware.bin | cut -f1)
        echo -e "${GREEN}  Firmware: $SIZE - build/firmware.bin${NC}"
    fi
    if [ -f "build/bootloader/bootloader.bin" ]; then
        echo "  Bootloader: build/bootloader/bootloader.bin"
    fi
    if [ -f "build/partition_table/partition-table.bin" ]; then
        echo "  Partitionstabelle: build/partition_table/partition-table.bin"
    fi
    echo ""
    echo "Flashen mit:"
    echo "  idf.py -D MICROPY_BOARD=$BOARD -p /dev/ttyUSB0 flash"
    echo ""
    echo "Monitor öffnen:"
    echo "  idf.py -p /dev/ttyUSB0 monitor"
    echo ""
    echo "TWAI-Funktionalität testen:"
    echo "  >>> from machine import TWAI"
    echo "  >>> can = TWAI(tx=21, rx=22, baudrate=500000)"
    echo "  >>> can.init()"
    echo ""
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}✗ Build fehlgeschlagen!${NC}"
    echo -e "${RED}========================================${NC}"
    echo ""
    echo "Mögliche Lösungen:"
    echo "1. Prüfen Sie die ESP-IDF Installation"
    echo "2. Build-Verzeichnis löschen: $0 clean"
    echo "3. ESP-IDF Umgebung neu laden: source ~/esp/esp-idf/export.sh"
    echo ""
    exit 1
fi

echo ""
echo -e "${BLUE}ESP32-PICO-MINI-02-N8R2 Spezifikationen:${NC}"
echo "- Chip: ESP32-PICO-D4"
echo "- Flash: 8MB"
echo "- PSRAM: 2MB QSPI"
echo "- TWAI/CAN: GPIO 21 (TX), GPIO 22 (RX)"
echo "- Package: 7x7mm LGA"
