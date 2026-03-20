# Implementation Notes

## Implemented Scope

- Character: Ironclad-like warrior (single playable character)
- Cards: 20 warrior cards
- Enemies: 3 normal, 1 elite, 1 boss
- Battles: fixed sequence of 5 battles (3 normal + 1 elite + 1 boss)
- Win condition: defeat boss in battle 5

## Core Combat Rules

- Turn-based combat
- Start of player turn:
  - Energy set to 3
  - Block resets to 0
  - Draw up to 5 cards (max hand size 10)
- End player turn:
  - Remaining hand discarded
  - Enemy executes telegraphed intent
- Deck flow:
  - Draw pile -> hand -> discard
  - If draw pile empty, shuffle discard into draw
  - Exhaust cards go to exhaust pile

## Statuses

- Strength: adds to outgoing attack damage
- Weak: outgoing attack damage * 0.75
- Vulnerable: incoming attack damage * 1.5
- Block: absorbs damage before HP

## Controls

- Left click a card to play (if enough energy)
- Left click End Turn to pass
- During reward phase, click one of 3 cards to add to deck
- Press R to restart run

## UI Style

- Simple SFML rectangles + text (placeholder-first)
- No external sprites required for base gameplay
- Font loading order:
  - assets/font.ttf
  - C:/Windows/Fonts/arial.ttf

## Build Notes

- CMake pulls SFML 2.6.1 source via FetchContent to avoid local prebuilt binary ABI mismatch.
- This keeps MinGW toolchain and SFML binary ABI compatible.
