# ESP32 TWAI Build Script - Clean Version
# Fuehrt den WSL Build-Prozess aus

[CmdletBinding()]
param()

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

Write-ColorOutput "=======================================" "Green"
Write-ColorOutput "ESP32 TWAI Build mit TCAN332" "Green"
Write-ColorOutput "=======================================" "Green"
Write-Host ""

Write-ColorOutput "WSL pruefen..." "Blue"
try {
    $null = wsl --list 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "WSL nicht verfuegbar"
    }
    Write-ColorOutput "WSL verfuegbar" "Green"
}
catch {
    Write-ColorOutput "WSL nicht verfuegbar" "Red"
    Write-ColorOutput "Installieren: wsl --install" "Yellow"
    pause
    exit 1
}

$currentDir = Get-Location
Write-ColorOutput "Aktueller Pfad: $currentDir" "Gray"

$scriptPath = Join-Path $currentDir "wsl_build_esp32_twai.sh"
if (-not (Test-Path $scriptPath)) {
    Write-ColorOutput "Build-Script nicht gefunden" "Red"
    Write-ColorOutput "Erwartet: $scriptPath" "Gray"
    pause
    exit 1
}
Write-ColorOutput "Build-Script gefunden" "Green"

$drive = $currentDir.Drive.Name.ToLower()
$path = $currentDir.Path -replace '^[A-Z]:', '' -replace '\\', '/'
$wslPath = "/mnt/$drive$path/wsl_build_esp32_twai.sh"

Write-Host ""
Write-ColorOutput "Build starten..." "Blue"
Write-ColorOutput "WSL-Pfad: $wslPath" "Gray"
Write-ColorOutput "TCAN332: GPIO4/5" "Gray"

try {
    Write-ColorOutput "Script ausfuehrbar machen..." "Gray"
    wsl chmod +x "$wslPath"
    
    Write-ColorOutput "Build-Prozess starten..." "Yellow"
    wsl bash "$wslPath"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-ColorOutput "BUILD ERFOLGREICH!" "Green"
        Write-Host ""
        Write-ColorOutput "Firmware: ~/micropython/ports/esp32/build/firmware.bin" "Cyan"
        Write-ColorOutput "Flash: wsl; cd ~/micropython/ports/esp32; idf.py flash" "Cyan"
        Write-Host ""
        Write-ColorOutput "TCAN332 Test:" "Blue"
        Write-ColorOutput "from machine import TWAI" "Gray"
        Write-ColorOutput "can = TWAI(tx=4, rx=5, baudrate=500000)" "Gray"
        Write-ColorOutput "can.init()" "Gray"
    }
    else {
        Write-Host ""
        Write-ColorOutput "BUILD FEHLGESCHLAGEN" "Red"
        Write-ColorOutput "Fehlercode: $LASTEXITCODE" "Gray"
    }
}
catch {
    Write-ColorOutput "Fehler: $_" "Red"
}

Write-Host ""
pause
