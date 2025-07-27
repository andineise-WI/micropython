# ESP-IDF Installation für Windows - Schritt-für-Schritt Anleitung

## Option 1: ESP-IDF Installer (Empfohlen für Windows)

### Schritt 1: ESP-IDF Installer herunterladen
1. Gehen Sie zu: https://dl.espressif.com/dl/esp-idf/
2. Laden Sie den neuesten Windows Installer herunter (esp-idf-tools-setup-X.X.X.exe)
3. Führen Sie den Installer aus und folgen Sie den Anweisungen

### Schritt 2: Installation prüfen
Nach der Installation öffnen Sie eine neue PowerShell und testen:
```powershell
# ESP-IDF Umgebung aktivieren (wird automatisch installiert)
# Suchen Sie im Startmenü nach "ESP-IDF X.X PowerShell"
# Oder navigieren Sie zu: C:\Espressif\frameworks\esp-idf-vX.X.X\export.ps1

# In der ESP-IDF PowerShell:
idf.py --version
```

## Option 2: Manuelle Installation

### Schritt 1: Voraussetzungen installieren
```powershell
# Git installieren (falls nicht vorhanden)
# Download von: https://git-scm.com/download/win

# Python 3.8+ installieren (falls nicht vorhanden)  
# Download von: https://www.python.org/downloads/
```

### Schritt 2: ESP-IDF klonen
```powershell
# Erstellen Sie ein Verzeichnis für ESP-IDF
mkdir C:\esp
cd C:\esp

# ESP-IDF Repository klonen
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Auf stabile Version wechseln (empfohlen: v5.1.2)
git checkout v5.1.2
git submodule update --init --recursive
```

### Schritt 3: ESP-IDF Tools installieren
```powershell
# Im esp-idf Verzeichnis:
.\install.bat esp32

# Oder für alle Targets:
.\install.bat all
```

### Schritt 4: Umgebung aktivieren
```powershell
# ESP-IDF Umgebung für aktuelle Session aktivieren
.\export.bat

# Testen
idf.py --version
```

## Option 3: ESP-IDF für VS Code Extension

### ESP-IDF Extension installieren
1. Öffnen Sie VS Code
2. Gehen Sie zu Extensions (Ctrl+Shift+X)
3. Suchen Sie nach "ESP-IDF"
4. Installieren Sie die offizielle Espressif ESP-IDF Extension
5. Folgen Sie dem Setup-Assistenten

## Umgebung dauerhaft verfügbar machen

### Automatische Aktivierung (PowerShell Profile)
```powershell
# PowerShell Profil bearbeiten
notepad $PROFILE

# Folgende Zeile hinzufügen (Pfad anpassen):
# C:\esp\esp-idf\export.ps1
```

### Oder: Batch-Datei erstellen
```batch
@echo off
echo ESP-IDF Umgebung aktivieren...
call C:\esp\esp-idf\export.bat
echo ESP-IDF bereit!
cmd /k
```

## Nach der Installation: MicroPython kompilieren

### Schritt 1: ESP-IDF Umgebung aktivieren
```powershell
# Option A: ESP-IDF PowerShell aus Startmenü
# Option B: Manuell aktivieren
C:\esp\esp-idf\export.bat
```

### Schritt 2: Zu MicroPython navigieren
```powershell
cd "C:\Users\admin\git\micropython-1\ports\esp32"
```

### Schritt 3: Kompilieren
```powershell
# ESP32-PICO-MINI-02-N8R2 Firmware bauen
idf.py -D MICROPY_BOARD=ESP32_PICO_MINI_02_N8R2 build

# Oder unser Build-Skript verwenden
.\build_pico_mini.bat
```

## Fehlerbehebung

### Problem: "idf.py wird nicht als interner oder externer Befehl erkannt"
**Lösung**: ESP-IDF Umgebung nicht aktiviert
```powershell
# ESP-IDF Export-Skript ausführen
C:\esp\esp-idf\export.bat
```

### Problem: "ESP-IDF ist nicht konfiguriert"
**Lösung**: ESP-IDF nicht installiert oder falsche Version
```powershell
# Installation prüfen
where.exe idf.py
echo $env:IDF_PATH
```

### Problem: Build-Fehler
**Lösung**: Build-Cache löschen
```powershell
# Im ports/esp32 Verzeichnis
rmdir /s build
idf.py build
```

## Erfolgreiche Installation prüfen

Nach erfolgreicher Installation sollten diese Befehle funktionieren:
```powershell
# ESP-IDF Version anzeigen
idf.py --version

# Umgebungsvariablen prüfen
echo $env:IDF_PATH
echo $env:IDF_TOOLS_PATH

# Python Pakete prüfen
python -c "import esptool; print('esptool verfügbar')"
```

## Nächste Schritte

Nach der ESP-IDF Installation können Sie:
1. **MicroPython kompilieren**: `idf.py -D MICROPY_BOARD=ESP32_PICO_MINI_02_N8R2 build`
2. **Firmware flashen**: `idf.py -p COM3 flash`
3. **TWAI/CAN verwenden**: Die implementierte TWAI-Funktionalität nutzen

## Weitere Hilfe

- **ESP-IDF Dokumentation**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
- **ESP-IDF Installation Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html
- **ESP-IDF GitHub**: https://github.com/espressif/esp-idf
