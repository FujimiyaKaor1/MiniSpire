# Safe UI emoji downloader (OpenMoji CC BY-SA 4.0)
# Usage:
#   pwsh -File scripts/download_ui_emoji_openmoji.ps1
# or
#   powershell -ExecutionPolicy Bypass -File scripts/download_ui_emoji_openmoji.ps1

$ErrorActionPreference = 'Stop'

$base = 'https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618'
$targets = @(
  @{ code='1FA99'; file='stat_gold.png'; note='Coin' },
  @{ code='1F9EA'; file='stat_potion.png'; note='Test Tube' },
  @{ code='1F4DA'; file='stat_deck.png'; note='Books' },
  @{ code='1F48E'; file='stat_relic.png'; note='Gem Stone' }
)

$destDir = Join-Path $PSScriptRoot '..\assets\images\ui'
$destDir = [System.IO.Path]::GetFullPath($destDir)
if (-not (Test-Path $destDir)) {
  throw "Destination folder does not exist: $destDir"
}

Write-Output "Downloading UI emoji assets to: $destDir"
foreach ($t in $targets) {
  $url = "$base/$($t.code).png"
  $out = Join-Path $destDir $t.file
  Invoke-RestMethod -Uri $url -OutFile $out
  Write-Output ("OK {0} <- {1} ({2})" -f $t.file, $t.code, $t.note)
}

Write-Output "Done. Remember to keep assets/THIRD_PARTY_LICENSES.md in sync."
