# MiniSpireSFML

A minimal Slay-the-Spire-like prototype in C++ + SFML.

Current stage: demo version with internet-sourced art + font assets.

Implemented scope:
- 1 playable character: Ironclad-like warrior
- 20 warrior cards
- 3 normal enemies
- 1 elite enemy
- 1 boss enemy
- Fixed 5 battles (3 normal + 1 elite + 1 boss)
- Beat boss to win

## Build (Windows + vcpkg recommended)

1. Install dependencies:
   - CMake
   - Ninja
   - A C++ toolchain (g++, clang++, or MSVC)
2. Configure:
   - `cmake --preset default`
3. Build:
   - `cmake --build --preset default`
4. Run:
   - `./build/MiniSpireSFML.exe`

## Controls

- Click a card to play it (if enough energy)
- Click `End Turn` to pass
- During rewards, click one of the 3 cards to add it
- Press `R` to restart the run

## Notes

- UI uses simple rectangles and text to prioritize gameplay.
- Demo visuals use packaged web assets (OpenMoji icons + Noto CJK font).
- If packaged fonts fail to load, the game tries `C:/Windows/Fonts/arial.ttf`.
- Deck growth is persistent within a run: reward cards are added to your master deck for later battles.
- Combat-only temporary effects reset between battles.
- Minimal meta progress is saved to `save_progress.dat` (total boss wins).
- SFML is fetched and built from source by CMake to avoid local binary ABI mismatches.

## Asset Attribution

- See `assets/THIRD_PARTY_LICENSES.md` for exact sources and licenses.
