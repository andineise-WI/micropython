# ESP32-PICO-MINI + TCAN332 Pinout Guide (Texas Instruments)

## ESP32-PICO-MINI-02-N8R2 Pinout (relevant für TWAI)

```
                     ESP32-PICO-MINI-02-N8R2
                  ┌─────────────────────────────┐
                  │                             │
               1──┤ GND                   GPIO0 ├──49
               2──┤ 3V3                   GPIO1 ├──48 (UART0 TX)
               3──┤ EN                    GPIO2 ├──47
               4──┤ SENSOR_VP             GPIO3 ├──46 (UART0 RX)
               5──┤ SENSOR_VN       ★ -> GPIO4 ├──45 <- TCAN332 TXD
               6──┤ GPIO34          ★ -> GPIO5 ├──44 <- TCAN332 RXD
               7──┤ GPIO35               GPIO6 ├──43
               8──┤ GPIO32               GPIO7 ├──42
               9──┤ GPIO33               GPIO8 ├──41
              10──┤ GPIO25               GPIO9 ├──40
              11──┤ GPIO26              GPIO10 ├──39
              12──┤ GPIO27              GPIO11 ├──38
              13──┤ MTMS                GPIO12 ├──37
              14──┤ MTDI                GPIO13 ├──36
              15──┤ VDD3P3              GPIO14 ├──35
              16──┤ MTCK                GPIO15 ├──34
              17──┤ MTDO                GPIO16 ├──33
              18──┤ GPIO2               GPIO17 ├──32
              19──┤ GPIO0               GPIO18 ├──31
              20──┤ GPIO4               GPIO19 ├──30
              21──┤ GPIO16              GPIO21 ├──29
              22──┤ GPIO17              GPIO22 ├──28
              23──┤ GPIO5               GPIO23 ├──27
              24──┤ GPIO18               CLK ├──26
              25──┤ GPIO19                SD0 ├──25
                  │                             │
                  └─────────────────────────────┘
                           Bottom View

★ = TWAI Pins für TCAN332 Connection
```

## TCAN332 SO-8 Pinout (Texas Instruments)

```
           TCAN332 (SO-8 Package)
           ┌─────────────────────┐
        1──┤ TXD             Rs  ├──8  -> Optional slope control
           │                     │
        2──┤ GND           CAN_H ├──7  -> CAN Bus High
           │                     │  
        3──┤ VCC           CAN_L ├──6  -> CAN Bus Low
           │                     │
        4──┤ RXD            VREF ├──5  -> Optional
           └─────────────────────┘
           
Pin Funktionen:
1 (TXD)   - Transmit Data Input       <- ESP32 GPIO4
2 (GND)   - Ground                    <- ESP32 GND
3 (VCC)   - Supply Voltage (3.3V/5V)  <- ESP32 3.3V
4 (RXD)   - Receive Data Output       -> ESP32 GPIO5
5 (VREF)  - Reference Voltage         (Optional)
6 (CAN_L) - CAN Bus Low              <-> CAN Bus
7 (CAN_H) - CAN Bus High             <-> CAN Bus  
8 (Rs)    - Slope Control            (Optional, meist offen)

Datenblatt: https://www.ti.com/product/TCAN332
```

## Verdrahtungsschema

```
ESP32-PICO-MINI-02-N8R2                    TCAN332 SO-8
┌─────────────────────┐                    ┌─────────────┐
│               GPIO4 ├────────────────────┤1 TXD        │
│               GPIO5 ├────────────────────┤4 RXD        │
│                3.3V ├────────────────────┤3 VCC        │
│                 GND ├────────────────────┤2 GND        │
└─────────────────────┘                    │             │
                                           │6 CAN_L ├────┼── CAN Bus Low
                                           │7 CAN_H ├────┼── CAN Bus High
                                           │5 VREF  │ (NC)
                                           │8 Rs    │ (NC)
                                           └─────────────┘
```

## Breadboard Layout Beispiel

```
                    ESP32-PICO-MINI-02-N8R2
                    ┌─────────────────────┐
                    │                     │
            ┌───────┤ GPIO4               │
            │   ┌───┤ GPIO5               │  
            │   │   │ 3.3V                │
            │   │   │ GND                 │
            │   │   └─────────────────────┘
            │   │              │ │
            │   │              │ └─── GND Rail
            │   │              └───── 3.3V Rail
            │   │
            │   │      TCAN332 (auf Breakout Board)
            │   │      ┌─────────────────────────┐
            │   └──────┤ RXD               VCC ├───── 3.3V Rail
            └──────────┤ TXD               GND ├───── GND Rail
                       │                       │
               120Ω ───┤ CAN_L         CAN_H ├─── 120Ω
                    │  └─────────────────────────┘  │
                    │                               │
                    └───── CAN Bus ─────────────────┘
                           (Twisted Pair)
```

## Layout-Tipps

### PCB Layout (empfohlen)
1. **Kurze Verbindungen:** GPIO4/5 zu TCAN332 so kurz wie möglich
2. **Ground Plane:** Durchgehende Massefläche unter den Bauteilen
3. **Entstörung:** 100nF Keramikkondensator nahe TCAN332 VCC
4. **CAN-Leitungen:** Differenzielle Führung, 120Ω Impedanz

### Breadboard/Prototyping
1. **Kurze Jumper:** Besonders für GPIO4/5 Verbindungen
2. **Solide Masse:** Mehrere GND-Verbindungen
3. **Stabile Spannungsversorgung:** Gute 3.3V-Verteilung
4. **Schirmung:** CAN-Leitungen verdrillt halten

## Elektrische Spezifikationen

### ESP32 GPIO
- **Ausgangsstrom:** Max 40mA pro Pin
- **Eingangsspannung:** 0V - 3.6V
- **Pull-up/Pull-down:** 45kΩ intern verfügbar

### TCAN332 (Texas Instruments)
- **Versorgungsspannung:** 3.0V - 5.5V
- **Stromaufnahme:** ~60mA aktiv
- **CAN-Spezifikation:** ISO 11898-2
- **Datenrate:** Bis 1 Mbit/s
- **Temperaturbereich:** -40°C bis +125°C
- **Datenblatt:** https://www.ti.com/product/TCAN332

### CAN Bus
- **Terminierung:** 120Ω an beiden Enden
- **Kabellänge:** Max abhängig von Baudrate
  - 1 Mbit/s: 25m
  - 500 kbit/s: 100m  
  - 250 kbit/s: 250m
  - 125 kbit/s: 500m

## Test-Setup

### Minimaler Test (2-Node Netzwerk)
```
Node 1: ESP32 + TCAN332 ──── CAN Bus ──── Node 2: ESP32 + TCAN332
                 │                                        │
                120Ω                                    120Ω
                Termination                         Termination
```

### Erweiterte Konfiguration
```
        ┌── Node 1 (ESP32 + TCAN332)
        │
CAN Bus ├── Node 2 (ESP32 + TCAN332)  
        │
        ├── Node 3 (Commercial ECU)
        │
        └── CAN Analyzer/Logger
        
120Ω ────────────────────────────────── 120Ω
Termination                        Termination
```

## Debugging Hardware

### Oszilloskop Messpunkte
1. **GPIO4 (TXD):** Digitale Daten vom ESP32
2. **GPIO5 (RXD):** Digitale Daten zum ESP32  
3. **CAN_H:** Differenzielles High-Signal
4. **CAN_L:** Differenzielles Low-Signal
5. **VCC/GND:** Stromversorgung

### Erwartete Signalpegel
- **Dominant (0):** CAN_H ~3.5V, CAN_L ~1.5V, Diff ~2V
- **Rezessiv (1):** CAN_H ~2.5V, CAN_L ~2.5V, Diff ~0V
- **GPIO Logic:** 3.3V High, 0V Low

Diese Konfiguration bietet eine robuste und zuverlässige CAN-Bus-Implementierung für ESP32-basierte Projekte mit dem TCAN332 von Texas Instruments.
