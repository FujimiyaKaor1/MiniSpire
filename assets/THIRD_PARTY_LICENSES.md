# Third Party Assets / Licenses

This demo includes internet-sourced assets.

## Included Assets

### OpenMoji (CC BY-SA 4.0)
- Source repository: https://github.com/hfg-gmuend/openmoji
- License: https://github.com/hfg-gmuend/openmoji/blob/master/LICENSE.txt
- Website: https://openmoji.org

Used files:
- assets/images/cards/attack.png <- color/618x618/2694.png
- assets/images/cards/skill.png <- color/618x618/1F6E1.png
- assets/images/cards/power.png <- color/618x618/2728.png
- assets/images/ui/logo.png <- color/618x618/1F5E1.png
- assets/images/ui/intent.png <- color/618x618/1F3AF.png
- assets/images/ui/start.png <- color/618x618/25B6.png
- assets/images/ui/exit.png <- color/618x618/274C.png
- assets/images/ui/thanks.png <- color/618x618/1F64F.png
- assets/images/ui/stat_gold.png <- color/618x618/1FA99.png
- assets/images/ui/stat_potion.png <- color/618x618/1F9EA.png
- assets/images/ui/stat_deck.png <- color/618x618/1F4DA.png
- assets/images/ui/stat_relic.png <- color/618x618/1F48E.png
- assets/images/enemies/enemy1.png <- color/618x618/1F98A.png
- assets/images/enemies/enemy2.png <- color/618x618/1F479.png
- assets/images/enemies/enemy3.png <- color/618x618/1F40D.png
- assets/images/enemies/elite.png <- color/618x618/1F43A.png
- assets/images/enemies/boss.png <- color/618x618/1F47F.png

### Noto CJK (SIL Open Font License 1.1)
- Source repository: https://github.com/notofonts/noto-cjk
- License: https://github.com/notofonts/noto-cjk/blob/main/LICENSE

Used files:
- assets/fonts/NotoSansCJKsc-Regular.otf <- Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf

## Notes
- The project still supports a local fallback font path on Windows if the packaged font fails to load.
- Keep this file updated if any assets are replaced.

If external assets are added later, record exact file names, source URLs, and licenses in this file.

### Optional Enemy Replacement Script Mapping
- Script: scripts/download_enemy_assets_openmoji.ps1
- Purpose: Replace enemy textures from OpenMoji raw source with traceable codepoints.
- Mapping (codepoint -> destination file):
- 1F98A -> assets/images/enemies/enemy1.png
- 1F479 -> assets/images/enemies/enemy2.png
- 1F40D -> assets/images/enemies/enemy3.png
- 1F43A -> assets/images/enemies/elite.png
- 1F47F -> assets/images/enemies/boss.png

### Optional UI Emoji Script Mapping
- Script: scripts/download_ui_emoji_openmoji.ps1
- Purpose: Replace status panel emoji icons from OpenMoji raw source with traceable codepoints.
- Mapping (codepoint -> destination file):
- 1FA99 -> assets/images/ui/stat_gold.png
- 1F9EA -> assets/images/ui/stat_potion.png
- 1F4DA -> assets/images/ui/stat_deck.png
- 1F48E -> assets/images/ui/stat_relic.png
