# ESP32 TWAI/CAN Build Scripts

Dieses Repository enthält bereits fertige Build-Scripts zum Kompilieren der ESP32-Firmware mit TWAI/CAN-Support für den TCAN332-Transceiver.

## 📁 Verfügbare Scripts

### 1. WSL Build-Script (Hauptscript)
**Datei:** `wsl_build_esp32_twai.sh`
- **Zweck:** Vollständiger Build-Prozess in WSL
- **ESP-IDF:** v5.1.2 (automatische Installation)
- **Board:** ESP32_GENERIC mit SPIRAM
- **TWAI:** GPIO4 (TX), GPIO5 (RX) für TCAN332
- **Ausführung:** `wsl bash wsl_build_esp32_twai.sh`

### 2. Windows PowerShell Launcher
**Datei:** `build_esp32_twai.ps1`
- **Zweck:** Startet WSL Build-Script von Windows aus
- **Voraussetzung:** WSL installiert
- **Ausführung:** `PowerShell -ExecutionPolicy Bypass -File build_esp32_twai.ps1`

### 3. Windows Batch Launcher
**Datei:** `build_esp32_twai.bat`  
- **Zweck:** Einfacher Batch-Starter für WSL Build-Script
- **Voraussetzung:** WSL installiert
- **Ausführung:** Doppelklick oder `build_esp32_twai.bat`

## 🚀 Schnellstart

### Option A: PowerShell (empfohlen)
```powershell
cd C:\Users\admin\git\micropython-1
PowerShell -ExecutionPolicy Bypass -File build_esp32_twai.ps1
```

### Option B: Batch-Datei
```cmd
cd C:\Users\admin\git\micropython-1
build_esp32_twai.bat
```

### Option C: Direkt in WSL
```bash
wsl
cd /mnt/c/Users/admin/git/micropython-1
chmod +x wsl_build_esp32_twai.sh
./wsl_build_esp32_twai.sh
```

## 📋 Was die Scripts tun

1. **System Update:** WSL-System aktualisieren
2. **Dependencies:** Build-Tools installieren (git, cmake, ninja, etc.)
3. **ESP-IDF Setup:** ESP-IDF v5.1.2 installieren und konfigurieren
4. **MicroPython:** Repository klonen und vorbereiten
5. **TWAI-Code:** Ihre TWAI-Implementation kopieren/aktualisieren
6. **Build:** ESP32-Firmware kompilieren
7. **Output:** Firmware-Dateien bereitstellen

## 📦 Build-Output

Nach erfolgreichem Build finden Sie in WSL:

```
~/micropython/ports/esp32/build/
├── firmware.bin              # Haupt-Firmware
├── bootloader/
│   └── bootloader.bin        # ESP32 Bootloader
└── partition_table/
    └── partition-table.bin   # Partition-Tabelle
```

## 📡 Flash-Befehle

Nach dem Build können Sie die Firmware flashen:

```bash
# In WSL
cd ~/micropython/ports/esp32

# ESP32 über USB verbinden, dann:
idf.py -p /dev/ttyUSB0 flash

# Serial Monitor starten
idf.py -p /dev/ttyUSB0 monitor
```

**Windows COM-Port Mapping:**
- COM3 → `/dev/ttyS3` in WSL
- COM4 → `/dev/ttyS4` in WSL
- USB-Serial → `/dev/ttyUSB0` in WSL

## 🧪 TCAN332 Test

Nach dem Flash können Sie TWAI/CAN testen:

```python
from machine import TWAI

# TCAN332 auf GPIO4/5 initialisieren
can = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
can.init()

# Test-Nachricht senden
can.send(b'\x01\x02\x03\x04', id=0x123, timeout=1000)
print("TCAN332 Test-Nachricht gesendet!")

# Status prüfen
print(f"Bus State: {can.state()}")
print(f"Statistics: {can.stats()}")
```

## 🔧 Troubleshooting

### WSL nicht verfügbar
```powershell
# WSL installieren
wsl --install

# Nach Neustart WSL aktualisieren
wsl --update
```

### Build-Fehler
```bash
# In WSL: Build-Cache löschen
rm -rf ~/micropython/ports/esp32/build

# ESP-IDF neu aktivieren
. ~/esp/esp-idf/export.sh

# Erneut builden
cd ~/micropython/ports/esp32
idf.py build
```

### Git-Updates
```bash
# In WSL: Neuesten Code holen
cd ~/micropython
git pull origin master

# Submodule aktualisieren
git submodule update --init --recursive
```

### COM-Port finden
```powershell
# In Windows PowerShell
Get-WmiObject -Class Win32_SerialPort | Select-Object Name,DeviceID
```

## 📖 Weitere Dokumentation

- **TCAN332 Hardware:** `docs/esp32/README_TCAN332.md`
- **Pinout-Diagramm:** `docs/esp32/ESP32_TCAN332_PINOUT.md`
- **API-Migration:** `docs/esp32/TWAI_MIGRATION_GUIDE.md`
- **Beispiele:** `examples/esp32/twai_*.py`

## 🎯 Hardware-Setup

Stellen Sie sicher, dass Ihr TCAN332 korrekt angeschlossen ist:

```
ESP32-PICO-MINI    TCAN332 (TI SO-8)
----------------   ------------------
GPIO4         ->   Pin 1 (TXD)
GPIO5         ->   Pin 4 (RXD)
3.3V          ->   Pin 3 (VCC)
GND           ->   Pin 2 (GND)
                   Pin 6 (CAN_L) -> CAN Bus Low
                   Pin 7 (CAN_H) -> CAN Bus High
```

Vergessen Sie nicht die 120Ω-Terminierung an beiden Enden des CAN-Bus!

## ✅ Erfolg

Bei erfolgreichem Build sehen Sie:
```
🎉 BUILD ERFOLGREICH!
📱 Firmware: build/firmware.bin (XXX KB)
🧪 TCAN332 ready!
```

Viel Erfolg mit Ihrem ESP32 TWAI/CAN-Projekt! 🚗
