# Safe asset downloader for MiniSpireSFML game beautification
# All sources are CC0, CC BY-SA 4.0, or free-to-use commercially
# Usage: powershell -ExecutionPolicy Bypass -File scripts/download_beautify_assets.ps1

$ErrorActionPreference = "Continue"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$base = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$imgDir = Join-Path $base "assets\images"
$audioDir = Join-Path $base "assets\audio"

# Create directories
@("$imgDir\backgrounds", "$audioDir\bgm", "$audioDir\sfx", "$imgDir\ui") | ForEach-Object {
    if (!(Test-Path $_)) { New-Item -ItemType Directory -Path $_ -Force | Out-Null; Write-Host "Created: $_" }
}

function Download-File {
    param([string]$Url, [string]$Out)
    if (Test-Path $Out) { Write-Host "SKIP $([System.IO.Path]::GetFileName($Out))"; return }
    Write-Host "Downloading $([System.IO.Path]::GetFileName($Out)) from $Url"
    try {
        Invoke-WebRequest -Uri $Url -OutFile $Out -UseBasicParsing -TimeoutSec 30
        if ((Test-Path $Out) -and (Get-Item $Out).Length -gt 100) {
            Write-Host "  OK"
        } else {
            Remove-Item $Out -ErrorAction SilentlyContinue
            Write-Host "  FAILED (empty)"
        }
    } catch { Write-Host "  FAILED: $_" }
}

Write-Host "`n=== OpenMoji Icons (CC BY-SA 4.0) ==="
# HP heart icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/2764.png" "$imgDir\ui\stat_hp.png"
# Energy/bolt icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/26A1.png" "$imgDir\ui\stat_energy.png"
# Shield/block icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F6E1.png" "$imgDir\ui\stat_block.png"
# Sword attack icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/2694.png" "$imgDir\ui\icon_sword.png"
# Fire/campfire icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F525.png" "$imgDir\ui\icon_fire.png"
# Skull defeat icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F480.png" "$imgDir\ui\icon_skull.png"
# Trophy victory icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F3C6.png" "$imgDir\ui\icon_trophy.png"
# Shop/merchant icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F3EA.png" "$imgDir\ui\icon_shop.png"
# Event/scroll icon
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F4DC.png" "$imgDir\ui\icon_scroll.png"
# Strength/flexed arm
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F4AA.png" "$imgDir\ui\stat_strength.png"
# Weak/dizzy face
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F635.png" "$imgDir\ui\stat_weak.png"
# Vulnerable/target
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F3AF.png" "$imgDir\ui\stat_vulnerable.png"
# Card upgrade star
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/2B50.png" "$imgDir\ui\icon_star.png"

Write-Host "`n=== Additional Enemy Portraits (CC BY-SA 4.0) ==="
# Enemy 4: Knight/Swordsman - crossed swords
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/2694.png" "$imgDir\enemies\enemy4.png"
# Elite 1: Armor Knight - shield
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F6E1.png" "$imgDir\enemies\elite1.png"
# Elite 2: Blood Fighter - boxing glove / anger
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F94A.png" "$imgDir\enemies\elite2.png"
# Elite 3: Curse Priest - crystal ball
Download-File "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F52E.png" "$imgDir\enemies\elite3.png"

Write-Host "`n=== Sound Effects ==="
# From Mixkit (free, license: Mixkit Free License)
Download-File "https://assets.mixkit.co/active_storage/sfx/2570/2570-preview.mp3" "$audioDir\sfx\sword_attack.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/2000/2000-preview.mp3" "$audioDir\sfx\player_hit.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/446/446-preview.mp3" "$audioDir\sfx\enemy_hit.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/2567/2567-preview.mp3" "$audioDir\sfx\card_play.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/2002/2002-preview.mp3" "$audioDir\sfx\block.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/2556/2556-preview.mp3" "$audioDir\sfx\heal.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/2869/2869-preview.mp3" "$audioDir\sfx\defeat.mp3"
Download-File "https://assets.mixkit.co/active_storage/sfx/2875/2875-preview.mp3" "$audioDir\sfx\victory.mp3"

Write-Host "`n=== Background Music ==="
Download-File "https://assets.mixkit.co/music/preview/mixkit-deep-dark-ambient-cinematic-background-2410.mp3" "$audioDir\bgm\main_menu.mp3"
Download-File "https://assets.mixkit.co/music/preview/mixkit-epic-cinematic-orchestral-trailer-690.mp3" "$audioDir\bgm\battle.mp3"
Download-File "https://assets.mixkit.co/music/preview/mixkit-winding-clock-ticking-3060.mp3" "$audioDir\bgm\campfire.mp3"
Download-File "https://assets.mixkit.co/music/preview/mixkit-serenity-340.mp3" "$audioDir\bgm\shop.mp3"
Download-File "https://assets.mixkit.co/music/preview/mixkit-mystery-cinematic-trailer-784.mp3" "$audioDir\bgm\event.mp3"
Download-File "https://assets.mixkit.co/music/preview/mixkit-deep-dark-ominous-trailer-784.mp3" "$audioDir\bgm\defeat.mp3"
Download-File "https://assets.mixkit.co/music/preview/mixkit-achievement-celebration-melody-469.mp3" "$audioDir\bgm\victory.mp3"

Write-Host "`n=== Done! ==="
Write-Host "Background images will be generated procedurally in code."
Write-Host "Remember to update assets/THIRD_PARTY_LICENSES.md"
