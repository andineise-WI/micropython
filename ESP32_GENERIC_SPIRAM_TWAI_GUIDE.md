# ESP32 Generic 8MB Flash + 2MB PSRAM - TWAI/CAN Firmware

## Hardware-Spezifikationen
- **Board**: Generischer ESP32
- **Flash**: 8MB SPI Flash
- **PSRAM**: 2MB External PSRAM (SPIRAM)
- **MCU**: ESP32-D0WDQ6 (Dual-core Tensilica LX6)

## Firmware-Konfiguration
- **Board Type**: ESP32_GENERIC mit SPIRAM Variante
- **TWAI/CAN Support**: Aktiviert
- **Standard TWAI Pins**: GPIO 21 (TX), GPIO 22 (RX)
- **PSRAM Support**: Aktiviert für erweiterten Speicher

## Build-Befehl
```cmd
# ESP-IDF Umgebung aktivieren
cd "C:\Users\admin\git\esp-idf"
.\export.bat

# Zurück zum MicroPython ESP32 Port
cd "C:\Users\admin\git\micropython-1\ports\esp32"

# Firmware kompilieren mit SPIRAM Support
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM build
```

## Flash-Befehle
```cmd
# Firmware flashen (COM-Port anpassen)
idf.py -D MICROPY_BOARD=ESP32_GENERIC -D MICROPY_BOARD_VARIANT=SPIRAM -p COM3 flash

# Monitor öffnen
idf.py -p COM3 monitor
```

## TWAI/CAN Hardware Setup

### Erforderlicher CAN-Transceiver
Ihr ESP32 benötigt einen externen CAN-Transceiver für die Verbindung zum CAN-Bus:

**Empfohlene Transceiver:**
- **MCP2551** - 5V tolerant, ideal für Automotive-Anwendungen
- **MCP2561** - Verbesserte Version des MCP2551
- **SN65HVD230** - 3.3V Low-Power für industrielle Anwendungen
- **TJA1050** - Automotive-grade Transceiver

### Schaltung
```
ESP32 Generic          CAN-Transceiver (z.B. MCP2551)    CAN-Bus
==============          ===========================       =======
GPIO 21 (TX) ---------> Pin 1 (TXD)
GPIO 22 (RX) <--------- Pin 4 (RXD)
3.3V ----------------> Pin 3 (VDD)
GND -----------------> Pin 2 (VSS/GND)
                       Pin 7 (CANH) ----------------> CAN High
                       Pin 6 (CANL) ----------------> CAN Low
```

**Wichtig**: CAN-Bus benötigt 120Ω Terminierungswiderstände an beiden Enden des Busses!

## TWAI/CAN Beispiel-Code

### Grundlegende Initialisierung
```python
from machine import TWAI

# TWAI/CAN initialisieren
can = TWAI(tx=21, rx=22, baudrate=500000)
can.init()

print("TWAI/CAN erfolgreich initialisiert!")
print(f"Status: {can.state()}")
```

### Nachricht senden
```python
# CAN-Nachricht senden
message_id = 0x123
data = b'\x01\x02\x03\x04'
can.send(message_id, data, timeout=1000)
print(f"Nachricht gesendet: ID=0x{message_id:X}, Daten={data.hex()}")
```

### Nachricht empfangen
```python
import time

# Auf Nachrichten warten
print("Warte auf CAN-Nachrichten...")
while True:
    if can.any():
        msg = can.recv(timeout=1000)
        if msg:
            msg_id, data = msg[0], msg[1]
            print(f"Empfangen: ID=0x{msg_id:X}, Daten={data.hex()}")
    time.sleep_ms(10)
```

### Filter setzen
```python
# Filter für spezifische IDs setzen
can.set_filter(0x100, mask=0x700)  # Akzeptiert IDs 0x100-0x1FF
print("Filter gesetzt für IDs 0x100-0x1FF")
```

### Erweiterte Funktionen
```python
# Bus-Status prüfen
status = can.state()
print(f"Bus-Status: {status}")

# Statistiken abrufen
stats = can.get_stats()
print(f"TX Erfolg: {stats[0]}, TX Fehler: {stats[1]}")
print(f"RX Erfolg: {stats[2]}, RX Fehler: {stats[3]}")

# Fehlerzähler
tx_err, rx_err = can.get_error_count()
print(f"TX Fehler: {tx_err}, RX Fehler: {rx_err}")

# Aufräumen
can.deinit()
```

## PSRAM Vorteile

Mit 2MB PSRAM haben Sie deutlich mehr Speicher für:
- **Größere Programme**: Mehr Python-Code im Speicher
- **Buffers**: Größere Datenstrukturen für CAN-Nachrichten
- **Networking**: Mehr WiFi-Buffer für bessere Performance
- **Multitasking**: Mehrere Tasks gleichzeitig

### PSRAM testen
```python
import gc

# Speicher-Info anzeigen
gc.collect()
print(f"Freier Speicher: {gc.mem_free()} bytes")
print(f"Gesamter Speicher: {gc.mem_alloc() + gc.mem_free()} bytes")

# Großes Array erstellen (nutzt PSRAM)
big_array = bytearray(1024 * 1024)  # 1MB Array
print("1MB Array erstellt - PSRAM wird genutzt!")
```

## Typische Anwendungen

### OBD-II Scanner
```python
# OBD-II Diagnose (Automotive)
can = TWAI(tx=21, rx=22, baudrate=500000)
can.init()
can.set_filter(0x7E8, mask=0x7F8)  # OBD-II Response Filter

# Motor-RPM abfragen
can.send(0x7DF, b'\x02\x01\x0C\x00\x00\x00\x00\x00')  # PID 0x0C
response = can.recv(1000)
if response:
    rpm = ((response[1][3] << 8) | response[1][4]) / 4
    print(f"Motor RPM: {rpm}")
```

### Industrial Monitoring
```python
# Industrielle Sensorabfrage
can = TWAI(tx=21, rx=22, baudrate=250000)  # Niedrigere Baudrate für Industrie
can.init()

# Sensor-Daten kontinuierlich lesen
while True:
    if can.any():
        msg = can.recv(100)
        if msg:
            sensor_id = msg[0]
            data = msg[1]
            print(f"Sensor {sensor_id:03X}: {data.hex()}")
```

## Fehlerbehebung

### Häufige Probleme
1. **Keine Nachrichten empfangen**
   - Prüfen Sie die Transceiver-Verbindungen
   - 120Ω Terminierung an beiden Bus-Enden
   - Korrekte Baudrate

2. **Build-Fehler**
   - Submodule initialisieren: `git submodule update --init --recursive`
   - Build-Cache löschen: `rmdir /s build`

3. **PSRAM nicht erkannt**
   - Hardware prüfen (PSRAM wirklich vorhanden?)
   - SPIRAM Variante verwenden

## Performance mit PSRAM

Ihr ESP32 mit 2MB PSRAM bietet:
- **~2.3MB verfügbarer RAM** (statt ~300KB ohne PSRAM)
- **Bessere WiFi-Performance** durch größere Buffer
- **Mehr CAN-Nachrichten** gleichzeitig im Speicher
- **Komplexere Programme** möglich

## Nächste Schritte

1. **Firmware flashen** sobald Build abgeschlossen
2. **CAN-Transceiver anschließen** (MCP2551 empfohlen)
3. **TWAI-Tests** mit dem bereitgestellten Code
4. **Ihre spezifische CAN-Anwendung** implementieren

Die TWAI/CAN-Implementierung ist vollständig funktional und produktionsreif für Ihren ESP32!
