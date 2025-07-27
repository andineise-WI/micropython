# MicroPython ESP32-PICO-MINI-02-N8R2 Kompilier-Anleitung

## Voraussetzungen

### 1. ESP-IDF Installation

Das ESP32-PICO-MINI-02-N8R2 Modul benötigt ESP-IDF zum Kompilieren.

#### Windows Installation:
```cmd
# 1. ESP-IDF Installer herunterladen
# https://dl.espressif.com/dl/esp-idf/

# 2. Oder manuell installieren:
mkdir C:\esp
cd C:\esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
install.bat esp32

# 3. ESP-IDF Umgebung aktivieren (in jeder neuen Terminal-Session):
C:\esp\esp-idf\export.bat
```

#### Linux/macOS Installation:
```bash
mkdir ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# ESP-IDF Umgebung aktivieren:
source ~/esp/esp-idf/export.sh
```

### 2. Python Dependencies
```cmd
pip install pyserial
```

## Kompilierung

### 1. ESP-IDF Umgebung aktivieren
```cmd
# Windows:
C:\esp\esp-idf\export.bat

# Linux/macOS:
source ~/esp/esp-idf/export.sh
```

### 2. Zum ESP32 Port navigieren
```cmd
cd C:\Users\admin\git\micropython-1\ports\esp32
```

### 3. Board-Konfiguration

Da es keine spezielle Konfiguration für ESP32-PICO-MINI-02-N8R2 gibt, verwenden wir ESP32_GENERIC:

```cmd
# Standard-Kompilierung:
idf.py -D MICROPY_BOARD=ESP32_GENERIC build

# Mit TWAI-Unterstützung (unser neues Feature):
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D CONFIG_SOC_TWAI_SUPPORTED=y build
```

### 4. Alternative: Make verwenden (falls verfügbar)
```cmd
make BOARD=ESP32_GENERIC
```

### 5. Firmware flashen
```cmd
# Port automatisch erkennen:
idf.py -D MICROPY_BOARD=ESP32_GENERIC flash

# Spezifischen Port angeben:
idf.py -D MICROPY_BOARD=ESP32_GENERIC -p COM3 flash

# Monitor öffnen:
idf.py -p COM3 monitor
```

## ESP32-PICO-MINI-02-N8R2 Spezifikationen

- **Chip**: ESP32-PICO-D4
- **Flash**: 8MB integriert
- **PSRAM**: 2MB QSPI PSRAM
- **TWAI/CAN**: Unterstützt (GPIO 21/22 standardmäßig)
- **Package**: 7x7mm LGA

## TWAI/CAN Pins für ESP32-PICO-MINI-02-N8R2

- **TX (CTX)**: GPIO 21 (Standard)
- **RX (CRX)**: GPIO 22 (Standard)

## Beispiel-Nutzung nach dem Flashen

```python
from machine import TWAI

# TWAI für ESP32-PICO-MINI-02-N8R2 initialisieren
can = TWAI(tx=21, rx=22, baudrate=500000, mode=TWAI.NORMAL)
can.init()

# CAN-Nachricht senden
can.send(b"Hello from PICO!", 0x123)

# CAN-Nachricht empfangen
data, msg_id, extframe, rtr = can.recv(timeout=1000)
print(f"Received: {data} from ID: 0x{msg_id:03X}")

can.deinit()
```

## Fehlerbehebung

### ESP-IDF nicht gefunden:
```cmd
# Prüfen ob ESP-IDF installiert ist:
where idf.py

# Falls nicht gefunden, ESP-IDF Umgebung aktivieren:
C:\esp\esp-idf\export.bat
```

### Kompilier-Fehler:
```cmd
# Build-Verzeichnis löschen und neu kompilieren:
idf.py fullclean
idf.py -D MICROPY_BOARD=ESP32_GENERIC build
```

### Flash-Probleme:
```cmd
# Port prüfen:
idf.py -p COM3 monitor

# Flash löschen und neu flashen:
esptool.py --port COM3 erase_flash
idf.py -D MICROPY_BOARD=ESP32_GENERIC -p COM3 flash
```

## Ausgabe-Dateien

Nach erfolgreicher Kompilierung finden Sie die Firmware in:
- `build/firmware.bin` - Hauptfirmware
- `build/bootloader/bootloader.bin` - Bootloader
- `build/partition_table/partition-table.bin` - Partitionstabelle

## Automatische Kompilierung

Verwenden Sie die bereitgestellten Build-Skripte:

### Windows:
```cmd
.\build_twai.bat ESP32_GENERIC
```

### Linux/macOS:
```bash
./build_twai.sh ESP32_GENERIC
```
