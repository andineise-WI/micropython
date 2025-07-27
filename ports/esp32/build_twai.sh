#!/bin/bash

# Build script for MicroPython ESP32 with TWAI support
# This script helps build MicroPython for ESP32 with the new TWAI implementation

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== MicroPython ESP32 TWAI Build Script ===${NC}"

# Check if ESP-IDF is setup
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}Error: ESP-IDF environment not set up!${NC}"
    echo "Please run: source \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

echo -e "${GREEN}ESP-IDF Path: $IDF_PATH${NC}"

# Check if we're in the ESP32 port directory
if [ ! -f "mpconfigport.h" ]; then
    echo -e "${RED}Error: Please run this script from the ports/esp32 directory${NC}"
    exit 1
fi

# Set default board if not specified
BOARD=${1:-ESP32_GENERIC}
echo -e "${GREEN}Building for board: $BOARD${NC}"

# Clean previous build if requested
if [ "$2" == "clean" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf build
fi

# Check if TWAI is supported on this target
echo -e "${YELLOW}Checking TWAI support...${NC}"
if grep -q "CONFIG_SOC_TWAI_SUPPORTED=y" boards/$BOARD/sdkconfig.board 2>/dev/null; then
    echo -e "${GREEN}✓ TWAI is supported on $BOARD${NC}"
elif [ -f "boards/$BOARD/sdkconfig.board" ]; then
    echo -e "${YELLOW}! TWAI support unknown for $BOARD, building anyway...${NC}"
else
    echo -e "${YELLOW}! Board directory not found, using defaults...${NC}"
fi

# Build the firmware
echo -e "${YELLOW}Building MicroPython firmware...${NC}"
make BOARD=$BOARD

if [ $? -eq 0 ]; then
    echo -e "${GREEN}=== Build completed successfully! ===${NC}"
    echo -e "${GREEN}Firmware location: build/firmware.bin${NC}"
    echo -e "${GREEN}Flash with: make BOARD=$BOARD deploy${NC}"
    echo -e "${GREEN}Or manually: esptool.py write_flash 0x1000 build/firmware.bin${NC}"
else
    echo -e "${RED}=== Build failed! ===${NC}"
    exit 1
fi

# Optional: Display firmware size
if [ -f "build/firmware.bin" ]; then
    SIZE=$(du -h build/firmware.bin | cut -f1)
    echo -e "${GREEN}Firmware size: $SIZE${NC}"
fi

echo -e "${YELLOW}=== TWAI Usage Example ===${NC}"
echo "After flashing, you can test TWAI with:"
echo ">>> from machine import TWAI"
echo ">>> can = TWAI(tx=21, rx=22, baudrate=500000)"
echo ">>> can.init()"
echo ">>> # Now ready for CAN communication!"
