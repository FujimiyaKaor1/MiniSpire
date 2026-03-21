@echo off
setlocal enabledelayedexpansion
set "BASE=c:\Users\csj\Documents\vscode_projects\cts\assets"
set "IMG=%BASE%\images"
set "AUD=%BASE%\audio"

REM Create directories
mkdir "%IMG%\backgrounds" 2>nul
mkdir "%AUD%\bgm" 2>nul
mkdir "%AUD%\sfx" 2>nul

echo === Downloading UI Icons (OpenMoji CC BY-SA 4.0) ===
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/2764.png" "%IMG%\ui\stat_hp.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/26A1.png" "%IMG%\ui\stat_energy.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F6E1.png" "%IMG%\ui\stat_block.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F4AA.png" "%IMG%\ui\stat_strength.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F635.png" "%IMG%\ui\stat_weak.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/2B50.png" "%IMG%\ui\icon_star.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F525.png" "%IMG%\ui\icon_fire.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F480.png" "%IMG%\ui\icon_skull.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F3C6.png" "%IMG%\ui\icon_trophy.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F3EA.png" "%IMG%\ui\icon_shop.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F4DC.png" "%IMG%\ui\icon_scroll.png"

echo === Downloading Enemy Portraits (OpenMoji CC BY-SA 4.0) ===
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F4AA.png" "%IMG%\enemies\enemy4.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F6E1.png" "%IMG%\enemies\elite1.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F94A.png" "%IMG%\enemies\elite2.png"
call :DL "https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/1F52E.png" "%IMG%\enemies\elite3.png"

echo === Downloading Sound Effects (Mixkit Free License) ===
call :DL "https://assets.mixkit.co/active_storage/sfx/2570/2570-preview.mp3" "%AUD%\sfx\sword_attack.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/2000/2000-preview.mp3" "%AUD%\sfx\player_hit.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/446/446-preview.mp3" "%AUD%\sfx\enemy_hit.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/2567/2567-preview.mp3" "%AUD%\sfx\card_play.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/2002/2002-preview.mp3" "%AUD%\sfx\block.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/2556/2556-preview.mp3" "%AUD%\sfx\heal.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/2869/2869-preview.mp3" "%AUD%\sfx\defeat.mp3"
call :DL "https://assets.mixkit.co/active_storage/sfx/2875/2875-preview.mp3" "%AUD%\sfx\victory.mp3"

echo === Downloading Background Music (Mixkit Free License) ===
call :DL "https://assets.mixkit.co/music/preview/mixkit-deep-dark-ambient-cinematic-background-2410.mp3" "%AUD%\bgm\main_menu.mp3"
call :DL "https://assets.mixkit.co/music/preview/mixkit-epic-cinematic-orchestral-trailer-690.mp3" "%AUD%\bgm\battle.mp3"
call :DL "https://assets.mixkit.co/music/preview/mixkit-winding-clock-ticking-3060.mp3" "%AUD%\bgm\campfire.mp3"
call :DL "https://assets.mixkit.co/music/preview/mixkit-serenity-340.mp3" "%AUD%\bgm\shop.mp3"
call :DL "https://assets.mixkit.co/music/preview/mixkit-mystery-cinematic-trailer-784.mp3" "%AUD%\bgm\event.mp3"
call :DL "https://assets.mixkit.co/music/preview/mixkit-deep-dark-ominous-trailer-784.mp3" "%AUD%\bgm\defeat.mp3"
call :DL "https://assets.mixkit.co/music/preview/mixkit-achievement-celebration-melody-469.mp3" "%AUD%\bgm\victory.mp3"

echo.
echo === Download Complete ===
echo.
echo Checking downloaded files...
dir /b /s "%IMG%\ui\stat_hp.png" "%IMG%\ui\stat_energy.png" "%AUD%\sfx\*.mp3" "%AUD%\bgm\*.mp3" 2>nul
goto :eof

:DL
if exist "%~2" (
    echo   SKIP [exists] %~nx2
    exit /b 0
)
echo   Downloading %~nx2 ...
powershell -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%~1' -OutFile '%~2' -UseBasicParsing -TimeoutSec 60" 2>nul
if exist "%~2" (
    echo   OK %~nx2
) else (
    echo   FAILED %~nx2
)
exit /b 0
