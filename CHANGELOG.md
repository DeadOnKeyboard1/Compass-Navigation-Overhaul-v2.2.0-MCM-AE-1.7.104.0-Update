# Changelog

## MCM localization update - all Skyrim SE languages

- Added MCM translations for all nine official Skyrim Special Edition interface languages: English, French, Italian, German, Spanish, Polish, Russian, Japanese, and Traditional Chinese.
- Build packaging now validates and includes all nine translation files automatically.
- Translation files use Skyrim/SKSE `$CNO_*` keys and UTF-16 LE BOM encoding.


## AE 1.7.104.0 update - 2026-09-23

- Updated the SKSE DLL for Skyrim 1.7.104.0 / SKSE 2.3.1.
- Restored quest, location, enemy and player-set compass markers.
- Hardened HUD/Scaleform, hook, signature-scanning and compatibility code against CTDs.
- Fixed multi-objective quest aggregation, quest-list visibility and compass layout handling.
- Fixed undiscovered locations so `?` stays visible while distance/height information remains available and the name stays hidden.
- Preserved the existing CNO MCM layout and settings.
- Added `fAngleToKeepMarkerDetailsShown` to the MCM because the DLL already supports it.
- Added automatic English/German MCM localization through `Interface/Translations`.
- Added an automated Windows build/package batch that produces a Vortex-ready DLL + MCM patch ZIP.
