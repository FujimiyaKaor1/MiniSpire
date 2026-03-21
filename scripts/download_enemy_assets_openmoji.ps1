# Safe enemy asset downloader (OpenMoji CC BY-SA 4.0)
# Usage (PowerShell):
#   pwsh -File scripts/download_enemy_assets_openmoji.ps1
# or
#   powershell -ExecutionPolicy Bypass -File scripts/download_enemy_assets_openmoji.ps1

$ErrorActionPreference = 'Stop'

$base = 'https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618'
$targets = @(
  @{ code='1F98A'; file='enemy1.png'; note='Fox Face' },
  @{ code='1F479'; file='enemy2.png'; note='Ogre' },
  @{ code='1F40D'; file='enemy3.png'; note='Snake' },
  @{ code='1F43A'; file='elite.png'; note='Wolf Face' },
  @{ code='1F47F'; file='boss.png'; note='Angry Face With Horns' }
)

$destDir = Join-Path $PSScriptRoot '..\assets\images\enemies'
$destDir = [System.IO.Path]::GetFullPath($destDir)
if (-not (Test-Path $destDir)) {
  throw "Destination folder does not exist: $destDir"
}

Write-Output "Downloading enemy assets to: $destDir"
foreach ($t in $targets) {
  $url = "$base/$($t.code).png"
  $out = Join-Path $destDir $t.file
  Invoke-RestMethod -Uri $url -OutFile $out
  Write-Output ("OK {0} <- {1} ({2})" -f $t.file, $t.code, $t.note)
}

Write-Output "Done. Remember to keep assets/THIRD_PARTY_LICENSES.md in sync."
