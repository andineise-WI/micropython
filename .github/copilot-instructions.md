# Copilot Instructions for AI Agents

## Projektüberblick
Dieses Repository enthält eine MicroPython-Implementierung für verschiedene Mikrocontroller und Plattformen. Die Architektur ist modular aufgebaut:
- **py/**: Kernimplementierung von Python (Compiler, Laufzeit, Standardbibliothek)
- **mpy-cross/**: Cross-Compiler für .mpy Bytecode
- **ports/**: Plattform- und Hardware-spezifische Implementierungen (z.B. ESP32, STM32, Unix, Windows)
- **extmod/**: Zusätzliche C-Module (z.B. Hardwaretreiber)
- **lib/**: Externe Abhängigkeiten als Submodule
- **tools/**: Hilfsprogramme, z.B. `mpremote` für die Gerätekommunikation
- **examples/**: Beispielskripte
- **tests/**: Testframework und Tests
- **docs/**: Sphinx-Dokumentation

## Wichtige Workflows
- **Build:**
  - Die meisten Komponenten werden mit `make` gebaut (z.B. `cd mpy-cross; make`).
  - Für einige Ports (z.B. ESP32, RP2) wird zusätzlich CMake verwendet.
  - Vor dem Kompilieren eines Ports: `make submodules` im jeweiligen Port-Verzeichnis ausführen.
- **Testen:**
  - Tests liegen in `tests/` und können mit den jeweiligen Makefiles ausgeführt werden.
  - Für Tools wie `mpremote` gibt es eigene Tests in `tools/mpremote/tests/`.
- **Gerätekommunikation:**
  - Mit `mpremote` können MicroPython-Geräte über USB/seriell gesteuert werden:
    - Beispiele: `mpremote connect <device>`, `mpremote exec "import micropython; micropython.mem_info()"`, `mpremote fs ls`, `mpremote mip install <package>`
  - Benutzerdefinierte Makros können in `.config/mpremote/config.py` definiert werden.

## Projektkonventionen
- **Namensgebung:**
  - Ports und Hardware-spezifische Komponenten sind in `ports/<plattform>/` organisiert.
  - Zusätzliche Module liegen in `extmod/`.
- **Beitragende:**
  - Bitte die [Contributors' Guidelines](https://github.com/micropython/micropython/wiki/ContributorGuidelines) und [CODECONVENTIONS.md](../CODECONVENTIONS.md) beachten.
- **Lizenz:**
  - MIT-Lizenz für alle Beiträge.

## Integration & Abhängigkeiten
- Externe Bibliotheken werden als Submodule in `lib/` eingebunden und müssen vor dem Build initialisiert werden.
- Die Dokumentation wird mit Sphinx aus `docs/` generiert.

## Beispiele für typische Aufgaben
- **Build für ESP32:**
  ```sh
  cd ports/esp32
  make submodules
  make
  ```
- **Cross-Compiler bauen:**
  ```sh
  cd mpy-cross
  make
  ```
- **Datei auf Gerät ausführen:**
  ```sh
  mpremote run examples/esp32_can.py
  ```

## Weitere Hinweise
- Die Architektur und Build-Prozesse sind auf Erweiterbarkeit und Portierbarkeit ausgelegt.
- Für neue Hardware-Portierungen empfiehlt sich ein Blick in `ports/minimal` und `ports/bare-arm`.
- Die Community ist über GitHub Discussions und Discord erreichbar.

---
*Bitte Feedback geben, falls wichtige Workflows, Konventionen oder Integrationspunkte fehlen oder unklar sind!*