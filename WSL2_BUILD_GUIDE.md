# MicroPython ESP32 TWAI Build - WSL2 Anleitung (Empfohlen für Windows)

Das qstr-Generierungsproblem auf Windows kann durch die Verwendung von WSL2 (Windows Subsystem for Linux) umgangen werden. Dies ist die empfohlene Lösung für eine zuverlässige ESP32-Firmware-Kompilierung.

## WSL2 Installation

### Schritt 1: WSL2 aktivieren
```powershell
# Als Administrator in PowerShell ausführen
wsl --install

# Oder spezifische Distribution installieren
wsl --install -d Ubuntu
```

### Schritt 2: Ubuntu in WSL2 starten
```bash
# WSL öffnen (aus Startmenü oder)
wsl

# Oder direkt Ubuntu starten
ubuntu
```

## ESP-IDF in WSL2 installieren

### Schritt 1: Abhängigkeiten installieren
```bash
# System aktualisieren
sudo apt update && sudo apt upgrade -y

# Build-Tools installieren
sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# Python-Pakete
pip3 install --user setuptools
```

### Schritt 2: ESP-IDF installieren
```bash
# Verzeichnis erstellen
mkdir -p ~/esp
cd ~/esp

# ESP-IDF klonen
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Auf stabile Version wechseln
git checkout v5.1.2
git submodule update --init --recursive

# ESP-IDF Tools installieren
./install.sh esp32

# Umgebung aktivieren
. ./export.sh
```

## MicroPython mit TWAI kompilieren

### Schritt 1: MicroPython vorbereiten
```bash
# Windows-Verzeichnis in WSL mounten
cd /mnt/c/Users/admin/git/micropython-1

# Oder MicroPython in WSL klonen
# git clone https://github.com/micropython/micropython.git
# cd micropython

# Submodule initialisieren
git submodule update --init --recursive

# Cross-Compiler bauen
make -C mpy-cross
```

### Schritt 2: ESP32 Port kompilieren
```bash
# Zum ESP32 Port wechseln
cd ports/esp32

# ESP-IDF Umgebung aktivieren (falls noch nicht aktiv)
. ~/esp/esp-idf/export.sh

# Board-spezifische Kompilierung
# Für Ihren ESP32 mit 8MB Flash + 2MB PSRAM:
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM build

# Oder Standard ESP32:
# idf.py -D MICROPY_BOARD=ESP32_GENERIC build
```

### Schritt 3: Firmware flashen
```bash
# Firmware flashen (COM-Port anpassen)
# Windows COM-Ports sind in WSL als /dev/ttyS[Nummer] verfügbar
# COM3 = /dev/ttyS3, COM4 = /dev/ttyS4, etc.

idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM -p /dev/ttyS3 flash

# Monitor öffnen
idf.py -p /dev/ttyS3 monitor
```

## TWAI/CAN in WSL-kompilierter Firmware

Nach erfolgreichem Flash können Sie die TWAI-Funktionalität testen:

```python
from machine import TWAI

# TWAI initialisieren
can = TWAI(tx=21, rx=22, baudrate=500000)
can.init()

print("TWAI/CAN erfolgreich initialisiert!")
print(f"Status: {can.state()}")

# Test-Nachricht senden (ohne Bus)
try:
    can.send(0x123, b'\x01\x02\x03\x04', timeout=100)
    print("Nachricht gesendet")
except OSError as e:
    print(f"Kein Bus angeschlossen: {e}")

can.deinit()
```

## Vorteile von WSL2

1. **Keine qstr-Probleme**: Linux-Environment umgeht Windows-spezifische Build-Issues
2. **Bessere Performance**: Native Linux-Tools sind oft schneller
3. **Konsistente Umgebung**: Gleiche Umgebung wie auf Linux-Servern
4. **Git-Integration**: Bessere Git-Performance und -Kompatibilität

## Datei-Synchronisation

```bash
# Windows-Dateien in WSL bearbeiten
cd /mnt/c/Users/admin/git/micropython-1

# Oder WSL-Dateien von Windows aus bearbeiten
# \\wsl$\Ubuntu\home\username\esp\micropython
```

## Serial Port Zugriff in WSL2

### USB-Serial mit usbipd (Windows 11)
```powershell
# In Windows PowerShell (als Administrator)
# USB-Geräte auflisten
usbipd wsl list

# USB-Serial-Adapter zu WSL weiterleiten
usbipd wsl attach --busid 1-1 --distribution Ubuntu
```

### Alternative: Windows COM-Ports verwenden
```bash
# WSL kann direkt auf Windows COM-Ports zugreifen
ls /dev/ttyS*

# COM3 = /dev/ttyS3
# COM4 = /dev/ttyS4
```

## Automatisiertes Build-Skript für WSL

```bash
#!/bin/bash
# save as: build_esp32_twai.sh

set -e

echo "ESP32 TWAI/CAN Firmware Build in WSL2"
echo "====================================="

# ESP-IDF aktivieren
. ~/esp/esp-idf/export.sh

# Verzeichnis prüfen
if [ ! -f "mpconfigport.h" ]; then
    echo "Fehler: Bitte im ports/esp32 Verzeichnis ausführen"
    exit 1
fi

# Build durchführen
echo "Kompiliere ESP32 Firmware mit TWAI/CAN Support..."
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM build

echo ""
echo "Build erfolgreich!"
echo "Firmware flashen mit:"
echo "idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM -p /dev/ttyS3 flash"
```

## Troubleshooting

### Problem: Serial Port nicht gefunden
```bash
# Prüfen Sie verfügbare Ports
ls /dev/tty*

# USB-Berechtigung setzen
sudo usermod -a -G dialout $USER
```

### Problem: ESP-IDF nicht gefunden
```bash
# ESP-IDF Umgebung neu laden
. ~/esp/esp-idf/export.sh

# Path prüfen
echo $IDF_PATH
```

Die WSL2-Lösung ist die zuverlässigste Methode für ESP32-Entwicklung unter Windows und umgeht alle bekannten qstr-Generierungsprobleme.
