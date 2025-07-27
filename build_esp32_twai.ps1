# ESP32 TWAI Build Script für Windows PowerShell
# Startet den Build-Prozess in WSL automatisch

param(
    [switch]$Force = $false
)

# PowerShell Execution Policy temporär setzen
$originalPolicy = Get-ExecutionPolicy
if ($originalPolicy -eq "Restricted") {
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process -Force
}

Write-Host "===============================================" -ForegroundColor Green
Write-Host "ESP32 TWAI/CAN Build mit TCAN332 Transceiver" -ForegroundColor Green  
Write-Host "===============================================" -ForegroundColor Green
Write-Host ""

# Prüfen ob WSL verfügbar ist
Write-Host "🔍 WSL Verfügbarkeit prüfen..." -ForegroundColor Blue
try {
    $wslCheck = & wsl --list --verbose 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "WSL nicht verfügbar"
    }
    Write-Host "✅ WSL ist verfügbar" -ForegroundColor Green
} catch {
    Write-Host "❌ WSL ist nicht installiert oder nicht verfügbar" -ForegroundColor Red
    Write-Host "   Installieren Sie WSL mit: wsl --install" -ForegroundColor Yellow
    Read-Host "Drücken Sie Enter zum Beenden"
    exit 1
}

# Build-Script-Pfad prüfen
$currentPath = Get-Location
$buildScript = Join-Path $currentPath "wsl_build_esp32_twai.sh"

Write-Host "🔍 Build-Script suchen..." -ForegroundColor Blue
Write-Host "   Aktueller Pfad: $currentPath" -ForegroundColor Gray
Write-Host "   Script-Pfad: $buildScript" -ForegroundColor Gray

if (Test-Path $buildScript) {
    Write-Host "✅ Build-Script gefunden" -ForegroundColor Green
} else {
    Write-Host "❌ Build-Script nicht gefunden: $buildScript" -ForegroundColor Red
    Write-Host "   Stellen Sie sicher, dass Sie im richtigen Verzeichnis sind:" -ForegroundColor Yellow
    Write-Host "   cd C:\Users\admin\git\micropython-1" -ForegroundColor Yellow
    Read-Host "Drücken Sie Enter zum Beenden"
    exit 1
}

# WSL-Pfad konvertieren (dynamisch basierend auf aktuellem Pfad)
$windowsPath = $currentPath.Path
$wslBuildScript = $windowsPath -replace '^([A-Z]):', '/mnt/$1' -replace '\\', '/'
$wslBuildScript = $wslBuildScript.ToLower() + "/wsl_build_esp32_twai.sh"

Write-Host ""
Write-Host "🚀 ESP32 TWAI Build in WSL starten..." -ForegroundColor Blue
Write-Host "   Windows-Pfad: $windowsPath" -ForegroundColor Gray
Write-Host "   WSL-Pfad: $wslBuildScript" -ForegroundColor Gray
Write-Host "   TCAN332: GPIO4 (TX), GPIO5 (RX)" -ForegroundColor Gray
Write-Host "   ESP-IDF: v5.x+ API" -ForegroundColor Gray
Write-Host ""

# Build-Script ausführbar machen und starten
Write-Host "▶️  WSL Build-Prozess starten..." -ForegroundColor Yellow
Write-Host "   Mache Script ausführbar..." -ForegroundColor Gray

try {
    & wsl chmod +x `"$wslBuildScript`"
    if ($LASTEXITCODE -ne 0) {
        throw "chmod failed"
    }
    
    Write-Host "   Starte Build-Prozess..." -ForegroundColor Gray
    & wsl bash `"$wslBuildScript`"
    $buildExitCode = $LASTEXITCODE
} catch {
    Write-Host "❌ Fehler beim Ausführen des WSL-Befehls: $_" -ForegroundColor Red
    Read-Host "Drücken Sie Enter zum Beenden"
    exit 1
}

# Ergebnis prüfen
if ($buildExitCode -eq 0) {
    Write-Host ""
    Write-Host "🎉 BUILD ERFOLGREICH ABGESCHLOSSEN!" -ForegroundColor Green
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host ""
    
    Write-Host "📁 Firmware-Dateien (in WSL):" -ForegroundColor Blue
    Write-Host "   ~/micropython/ports/esp32/build/firmware.bin" -ForegroundColor Gray
    Write-Host "   ~/micropython/ports/esp32/build/bootloader/bootloader.bin" -ForegroundColor Gray
    Write-Host "   ~/micropython/ports/esp32/build/partition_table/partition-table.bin" -ForegroundColor Gray
    Write-Host ""
    
    Write-Host "📡 Flash-Befehle (in WSL):" -ForegroundColor Blue
    Write-Host "   cd ~/micropython/ports/esp32" -ForegroundColor Gray
    Write-Host "   idf.py -p /dev/ttyUSB0 flash" -ForegroundColor Gray
    Write-Host "   idf.py -p /dev/ttyUSB0 monitor" -ForegroundColor Gray
    Write-Host ""
    
    Write-Host "🧪 TCAN332 Test:" -ForegroundColor Blue
    Write-Host "   from machine import TWAI" -ForegroundColor Gray
    Write-Host "   can = TWAI(tx=4, rx=5, baudrate=500000)" -ForegroundColor Gray
    Write-Host "   can.init()" -ForegroundColor Gray
    Write-Host "   can.send(b'\x01\x02\x03\x04', id=0x123)" -ForegroundColor Gray
    Write-Host ""
    
    Write-Host "📖 Dokumentation: docs/esp32/README_TCAN332.md" -ForegroundColor Blue
    
} else {
    Write-Host ""
    Write-Host "❌ BUILD FEHLGESCHLAGEN" -ForegroundColor Red
    Write-Host "===============================================" -ForegroundColor Red
    Write-Host ""
    
    Write-Host "🔧 Fehlerbehebung:" -ForegroundColor Yellow
    Write-Host "1. WSL neu starten: wsl --shutdown && wsl" -ForegroundColor Gray
    Write-Host "2. Build-Cache löschen: wsl rm -rf ~/micropython/ports/esp32/build" -ForegroundColor Gray  
    Write-Host "3. Script manuell ausführen: wsl bash `"$wslBuildScript`"" -ForegroundColor Gray
    Write-Host "4. ESP-IDF manuell setup: wsl . ~/esp/esp-idf/export.sh" -ForegroundColor Gray
    Write-Host ""
}

Write-Host ""
Write-Host "💡 Nächste Schritte:" -ForegroundColor Cyan
Write-Host "1. ESP32 mit USB verbinden" -ForegroundColor Gray
Write-Host "2. TCAN332 auf GPIO4/5 anschließen" -ForegroundColor Gray
Write-Host "3. In WSL: idf.py flash && idf.py monitor" -ForegroundColor Gray
Write-Host "4. TWAI-Beispiele testen" -ForegroundColor Gray
Write-Host ""

# Auf Benutzereingabe warten
Read-Host "Drücken Sie Enter zum Beenden"

# ExecutionPolicy zurücksetzen
if ($originalPolicy -eq "Restricted") {
    Set-ExecutionPolicy -ExecutionPolicy $originalPolicy -Scope Process -Force
}
