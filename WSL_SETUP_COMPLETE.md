# WSL Setup für ESP32 TWAI/CAN Entwicklung - Schritt für Schritt

## VS Code WSL Remote Extension - Jetzt installiert! ✅

Die WSL Remote Extension ist jetzt installiert und ermöglicht es Ihnen, direkt in WSL zu entwickeln.

## Nächste Schritte:

### 1. WSL Ubuntu starten
```powershell
# In PowerShell oder cmd
wsl
```

### 2. VS Code mit WSL verbinden
```bash
# In WSL Terminal
code .
```

Oder verwenden Sie in VS Code:
- **Ctrl+Shift+P** → "WSL: New Window" oder "WSL: Reopen in WSL"
- Klicken Sie auf das grüne Icon unten links → "New WSL Window"

### 3. ESP-IDF in WSL installieren

```bash
# System aktualisieren
sudo apt update && sudo apt upgrade -y

# Build-Dependencies installieren
sudo apt install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# ESP-IDF installieren
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.1.2
git submodule update --init --recursive

# ESP-IDF Tools installieren
./install.sh esp32

# Umgebung aktivieren
. ./export.sh
```

### 4. MicroPython in WSL vorbereiten

```bash
# Zum Windows MicroPython Verzeichnis navigieren
cd /mnt/c/Users/admin/git/micropython-1

# Oder MicroPython in WSL klonen (empfohlen für bessere Performance)
cd ~/
git clone https://github.com/micropython/micropython.git
cd micropython

# Ihre TWAI-Implementation aus Windows kopieren
cp -r /mnt/c/Users/admin/git/micropython-1/extmod/machine_twai.* extmod/
cp -r /mnt/c/Users/admin/git/micropython-1/ports/esp32/boards/ESP32_GENERIC_8MB_2MB ports/esp32/boards/
# etc. für alle Ihre Änderungen

# Submodule initialisieren
git submodule update --init --recursive

# Cross-Compiler bauen
make -C mpy-cross
```

### 5. ESP32 Firmware kompilieren

```bash
# ESP-IDF Umgebung aktivieren (falls nicht aktiv)
. ~/esp/esp-idf/export.sh

# Zum ESP32 Port wechseln
cd ports/esp32

# ESP32 mit SPIRAM kompilieren
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM build

# Oder Ihre benutzerdefinierte Board-Konfiguration
idf.py -D MICROPY_BOARD=ESP32_GENERIC_8MB_2MB build
```

### 6. Firmware flashen

```bash
# COM-Port finden (Windows COM3 = WSL /dev/ttyS3)
ls /dev/ttyS*

# Firmware flashen
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM -p /dev/ttyS3 flash

# Monitor öffnen
idf.py -p /dev/ttyS3 monitor
```

## VS Code WSL Features

### Terminal in WSL
- **Ctrl+`** öffnet WSL Terminal direkt in VS Code
- Alle Befehle laufen nativ in Linux

### Dateien bearbeiten
- Dateien werden direkt in WSL bearbeitet
- Bessere Git-Performance
- Native Linux-Tools verfügbar

### Extensions in WSL
Folgende Extensions sind besonders nützlich:

```bash
# In VS Code WSL-Modus installieren:
# - Python Extension (für MicroPython-Entwicklung)
# - C/C++ Extension (für ESP32 C-Code)
# - GitLens (für Git-Integration)
```

## Automatisiertes Setup-Skript

Erstellen Sie ein Setup-Skript für WSL:

```bash
#!/bin/bash
# save as: setup_esp32_twai.sh

echo "ESP32 TWAI/CAN Development Setup in WSL"
echo "========================================"

# ESP-IDF Setup
if [ ! -d "$HOME/esp/esp-idf" ]; then
    echo "Installing ESP-IDF..."
    mkdir -p ~/esp
    cd ~/esp
    git clone --recursive https://github.com/espressif/esp-idf.git
    cd esp-idf
    git checkout v5.1.2
    ./install.sh esp32
fi

# ESP-IDF aktivieren
. ~/esp/esp-idf/export.sh

# MicroPython Setup
if [ ! -d "$HOME/micropython" ]; then
    echo "Cloning MicroPython..."
    cd ~
    git clone https://github.com/micropython/micropython.git
    cd micropython
    
    # TWAI-Dateien von Windows kopieren
    echo "Copying TWAI implementation..."
    cp /mnt/c/Users/admin/git/micropython-1/extmod/machine_twai.* extmod/ 2>/dev/null || true
    
    git submodule update --init --recursive
    make -C mpy-cross
fi

echo "Setup complete!"
echo "To build ESP32 firmware:"
echo "  cd ~/micropython/ports/esp32"
echo "  . ~/esp/esp-idf/export.sh"
echo "  idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM build"
```

## TWAI/CAN Test nach WSL-Build

Nach erfolgreichem Build in WSL:

```python
# In MicroPython auf ESP32
from machine import TWAI

# TWAI initialisieren
can = TWAI(tx=21, rx=22, baudrate=500000)
can.init()

print("TWAI/CAN erfolgreich in WSL-kompilierter Firmware!")
print(f"Status: {can.state()}")

# Test-Nachricht
can.send(0x123, b'\x01\x02\x03\x04')
print("TWAI-Implementation funktioniert!")

can.deinit()
```

## Vorteile der WSL-Entwicklung

1. **Keine qstr-Probleme**: Linux-natives Build-System
2. **Bessere Performance**: Native Git und Build-Tools
3. **Konsistente Umgebung**: Gleich wie auf Linux-Servern
4. **VS Code Integration**: Nahtlose Entwicklung in WSL

## Troubleshooting

### VS Code erkennt WSL nicht
```bash
# WSL neu starten
wsl --shutdown
wsl
```

### ESP-IDF nicht gefunden
```bash
# ESP-IDF Umgebung neu laden
. ~/esp/esp-idf/export.sh
echo $IDF_PATH
```

### Serial Port Probleme
```bash
# USB-Berechtigung in WSL
sudo usermod -a -G dialout $USER
```

Die WSL Remote Extension ist jetzt installiert - Sie können sofort mit der ESP32 TWAI/CAN Entwicklung in einer stabilen Linux-Umgebung beginnen!
