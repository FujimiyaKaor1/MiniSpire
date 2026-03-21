# Download game assets script for MiniSpireSFML
# All assets are from safe, free, commercially usable sources

$ErrorActionPreference = "Stop"
$baseDir = "c:\Users\csj\Documents\vscode_projects\cts\assets"

# Create directories
$dirs = @(
    "$baseDir\audio\bgm",
    "$baseDir\audio\sfx",
    "$baseDir\images\backgrounds"
)

foreach ($d in $dirs) {
    if (!(Test-Path $d)) {
        New-Item -ItemType Directory -Path $d -Force | Out-Null
        Write-Host "Created: $d"
    }
}

# Use Invoke-WebRequest for downloading
function Download-Asset {
    param(
        [string]$Url,
        [string]$Output
    )
    if (Test-Path $Output) {
        Write-Host "SKIP (exists): $Output"
        return
    }
    Write-Host "Downloading: $Url -> $Output"
    try {
        # Try with TLS1.2
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $Url -OutFile $Output -UseBasicParsing
        Write-Host "OK: $Output"
    } catch {
        Write-Host "FAILED: $Url -> $_"
    }
}

Write-Host "=== Downloading Background Images ==="

# Background images from OpenGameArt and similar CC0 sources
# Main Menu background - dark fantasy castle/spire theme
Download-Asset -Url "https://raw.githubusercontent.com/nicholasgasior/gfx/master/backgrounds/castle.png" -Output "$baseDir\images\backgrounds\main_menu.png"

# Battle background - dungeon interior
Download-Asset -Url "https://opengameart.org/sites/default/files/dungeon_background_0.png" -Output "$baseDir\images\backgrounds\battle.png"

Write-Host "`n=== Phase 1 Complete: Directory setup done ==="
Write-Host "Note: Some downloads may fail due to direct linking restrictions."
Write-Host "We will use procedurally generated assets as fallback."
