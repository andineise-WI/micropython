# ESP32 TWAI mit TCAN332 Transceiver (Texas Instruments)

Diese Implementierung ist optimiert für die Verwendung mit dem TCAN332 CAN-Transceiver von Texas Instruments auf GPIO 4 (TX) und GPIO 5 (RX).

## Hardware Setup

### TCAN332 Anschlüsse (Texas Instruments SO-8)
```
ESP32-PICO-MINI    TCAN332 (SO-8)
----------------   ----------------
GPIO4         ->   Pin 1 (TXD)
GPIO5         ->   Pin 4 (RXD)
3.3V          ->   Pin 3 (VCC)
GND           ->   Pin 2 (GND)
                   Pin 6 (CAN_L) -> CAN Bus Low
                   Pin 7 (CAN_H) -> CAN Bus High
                   Pin 5 (VREF)  -> Optional reference
                   Pin 8 (Rs)    -> Optional slope control
```

**Datenblatt:** https://www.ti.com/product/TCAN332

### Schaltungsaufbau
```
     ESP32-PICO-MINI-02-N8R2
    ┌─────────────────────────┐
    │                   GPIO4 ├──┐
    │                   GPIO5 ├──┼──┐
    │                    3.3V ├──┼──┼──┐
    │                     GND ├──┼──┼──┼──┐
    └─────────────────────────┘  │  │  │  │
                                 │  │  │  │
         TCAN332 (SO-8)         │  │  │  │
        ┌─────────────────┐      │  │  │  │
     1──┤TXD         CAN_H├──7───┼──┼──┼──┼── CAN High
        │                 │      │  │  │  │
     2──┤GND         CAN_L├──6───┼──┼──┼──┼── CAN Low  
        │                 │      │  │  │  │
     3──┤VCC          VREF├──5   │  │  │  │
        │                 │      │  │  │  │
     4──┤RXD            Rs├──8   │  │  │  │
        └─────────────────┘      │  │  │  │
              │  │  │  │          │  │  │  │
              │  │  └──┼──────────┘  │  │  │
              │  └─────┼─────────────┘  │  │
              └────────┼────────────────┘  │
                       └───────────────────┘

    CAN Bus (mit 120Ω Terminierung an beiden Enden)
    ═══════════════════════════════════════════════
```

## Software Konfiguration

### Standard-Initialisierung
```python
from machine import TWAI

# Standardkonfiguration für TCAN332
twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
twai.init()
```

### Erweiterte Konfiguration
```python
# Mit spezifischen Parametern
twai = TWAI(
    tx=4,           # GPIO4 -> TCAN332 TXD
    rx=5,           # GPIO5 -> TCAN332 RXD  
    baudrate=250000, # 250 kbit/s
    mode=0          # Normal mode
)
```

## Beispielcode

### Einfacher Send/Receive Test
```python
from machine import TWAI
import time

# TCAN332 Setup
twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
twai.init()

# Message callback
def on_message(status):
    if status == 0:  # New message
        while twai.any():
            data, msg_id, ext, rtr = twai.recv(timeout=0)
            print(f"RX: ID=0x{msg_id:03X}, Data={data.hex()}")

twai.rxcallback(on_message)

# Send test message
test_data = b'\x01\x02\x03\x04'
twai.send(test_data, id=0x123, timeout=1000)
print("Message sent!")

time.sleep(2)
twai.deinit()
```

### OBD-II Beispiel
```python
from machine import TWAI
import time

def obd2_request(twai, pid):
    """OBD-II PID request"""
    # OBD-II request: Service 01 (Show current data) + PID
    request = bytes([0x02, 0x01, pid, 0x00, 0x00, 0x00, 0x00, 0x00])
    twai.send(request, id=0x7DF, timeout=1000)  # Functional request
    
def setup_obd2():
    twai = TWAI(tx=4, rx=5, baudrate=500000, mode=0)
    twai.init()
    
    responses = []
    def obd_rx(status):
        if status == 0:
            while twai.any():
                data, msg_id, ext, rtr = twai.recv(timeout=0)
                if 0x7E8 <= msg_id <= 0x7EF:  # OBD response range
                    responses.append((msg_id, data))
                    print(f"OBD Response: ID=0x{msg_id:03X}, Data={data.hex()}")
    
    twai.rxcallback(obd_rx)
    
    # Request engine RPM (PID 0x0C)
    obd2_request(twai, 0x0C)
    time.sleep(1)
    
    # Request vehicle speed (PID 0x0D)  
    obd2_request(twai, 0x0D)
    time.sleep(1)
    
    twai.deinit()
    return responses
```

## Baudrate-Einstellungen

### Standard CAN-Baudraten
| Baudrate | Verwendung | Timing |
|----------|------------|---------|
| 125000   | Automotive low speed | tseg1=13, tseg2=2 |
| 250000   | Automotive standard | tseg1=13, tseg2=2 |
| 500000   | Automotive high speed | tseg1=13, tseg2=2 |
| 1000000  | Industrial CAN | tseg1=6, tseg2=1 |

### Custom Baudrate
```python
# Beispiel für 800 kbit/s
twai = TWAI(tx=4, rx=5, baudrate=800000, mode=0)
```

## Filter-Konfiguration

### Accept All (Standard)
```python
twai.setfilter(mode=0, mask=0x000, id1=0x000)  # Alle Nachrichten
```

### Spezifische ID Range
```python
# Nur IDs 0x100-0x1FF akzeptieren
twai.setfilter(mode=0, mask=0x700, id1=0x100)
```

### Dual Filter
```python
# Zwei spezifische IDs: 0x123 und 0x456
twai.setfilter(mode=1, mask=0x7FF, id1=0x123, id2=0x456)
```

## Troubleshooting

### Häufige Probleme

1. **Keine Nachrichten empfangen**
   - CAN-Bus Terminierung prüfen (120Ω an beiden Enden)
   - TRCan332 Stromversorgung prüfen (3.3V oder 5V)
   - GPIO-Verbindungen prüfen

2. **Send Timeouts**
   - Bus-off Status prüfen: `twai.state()`
   - Andere Nodes im Netzwerk aktiv?
   - Baudrate korrekt?

3. **TCAN332 funktioniert nicht**
   - VCC-Spannung messen (sollte 3.3V oder 5V sein)
   - CAN_H/CAN_L Spannung messen (sollte ~2.5V Ruhepegel sein)
   - Datenblatt TCAN332 von Texas Instruments konsultieren

### Debug-Commands
```python
# Status prüfen
print(f"State: {twai.state()}")
print(f"Info: {twai.info()}")
print(f"Stats: {twai.stats()}")

# Bus restart bei Fehlern
if twai.state() == 3:  # BUS_OFF
    twai.restart()
```

## Kompatibilität

- **ESP-IDF Version:** v5.0 oder neuer
- **MicroPython:** ESP32 Port
- **Hardware:** ESP32-PICO-MINI-02-N8R2 getestet
- **Transceiver:** TCAN332 (3.3V/5V kompatibel, Texas Instruments)

## Weitere Beispiele

Siehe auch:
- `twai_tcan332_demo.py` - Umfassende Demo mit TCAN332
- `twai_simple_v5.py` - Einfacher Start
- `twai_enhanced_demo.py` - Erweiterte Features

## Lizenz

MIT License - siehe LICENSE file.
