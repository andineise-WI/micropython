# ESP32 TWAI Build Script - Einfache Version
# Führt den WSL Build-Prozess aus

[CmdletBinding()]
param()

# Fehlerbehandlung
$ErrorActionPreference = "Stop"

function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    
    switch ($Color) {
        "Green" { Write-Host $Message -ForegroundColor Green }
        "Red" { Write-Host $Message -ForegroundColor Red }
        "Yellow" { Write-Host $Message -ForegroundColor Yellow }
        "Blue" { Write-Host $Message -ForegroundColor Blue }
        "Cyan" { Write-Host $Message -ForegroundColor Cyan }
        "Gray" { Write-Host $Message -ForegroundColor Gray }
        default { Write-Host $Message }
    }
}

# Header
Write-ColorOutput "=======================================" "Green"
Write-ColorOutput "ESP32 TWAI Build mit TCAN332" "Green"
Write-ColorOutput "=======================================" "Green"
Write-Host ""

# WSL prüfen
Write-ColorOutput "WSL prüfen..." "Blue"
try {
    $null = wsl --list 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "WSL nicht verfügbar"
    }
    Write-ColorOutput "✓ WSL verfügbar" "Green"
} catch {
    Write-ColorOutput "✗ WSL nicht verfügbar" "Red"
    Write-ColorOutput "  Installieren: wsl --install" "Yellow"
    pause
    exit 1
}

# Aktuellen Pfad prüfen
$currentDir = Get-Location
Write-ColorOutput "Aktueller Pfad: $currentDir" "Gray"

# Build-Script suchen
$scriptPath = Join-Path $currentDir "wsl_build_esp32_twai.sh"
if (-not (Test-Path $scriptPath)) {
    Write-ColorOutput "✗ Build-Script nicht gefunden" "Red"
    Write-ColorOutput "  Erwartet: $scriptPath" "Gray"
    Write-ColorOutput "  Navigieren Sie zu: cd C:\Users\admin\git\micropython-1" "Yellow"
    pause
    exit 1
}
Write-ColorOutput "✓ Build-Script gefunden" "Green"

# WSL-Pfad erstellen
$drive = $currentDir.Drive.Name.ToLower()
$path = $currentDir.Path -replace '^[A-Z]:', '' -replace '\\', '/'
$wslPath = "/mnt/$drive$path/wsl_build_esp32_twai.sh"

Write-Host ""
Write-ColorOutput "Build starten..." "Blue"
Write-ColorOutput "  WSL-Pfad: $wslPath" "Gray"
Write-ColorOutput "  TCAN332: GPIO4/5" "Gray"

# Build ausführen
try {
    Write-ColorOutput "Script ausführbar machen..." "Gray"
    wsl chmod +x "$wslPath"
    
    Write-ColorOutput "Build-Prozess starten..." "Yellow"
    wsl bash "$wslPath"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-ColorOutput "🎉 BUILD ERFOLGREICH!" "Green"
        Write-Host ""
        Write-ColorOutput "Firmware: ~/micropython/ports/esp32/build/firmware.bin" "Cyan"
        Write-ColorOutput "Flash: wsl; cd ~/micropython/ports/esp32; idf.py flash" "Cyan"
        Write-Host ""
        Write-ColorOutput "TCAN332 Test:" "Blue"
        Write-ColorOutput "  from machine import TWAI" "Gray"
        Write-ColorOutput "  can = TWAI(tx=4, rx=5, baudrate=500000)" "Gray"
        Write-ColorOutput "  can.init()" "Gray"
    } else {
        Write-Host ""
        Write-ColorOutput "✗ BUILD FEHLGESCHLAGEN" "Red"
        Write-ColorOutput "Fehlercode: $LASTEXITCODE" "Gray"
    }
    
} catch {
    Write-ColorOutput "✗ Fehler: $_" "Red"
} finally {
    Write-Host ""
    pause
}
