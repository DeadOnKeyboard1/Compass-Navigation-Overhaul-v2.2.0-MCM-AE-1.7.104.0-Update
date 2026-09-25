# v5.1.3 Safe Runtime Robustness

This update intentionally implements only four low-risk robustness/diagnostic changes on top of v5.1.2.

## 1. Runtime state reset

`HUDMarkerManager::ResetRuntimeState()` clears transient focus pointers, marker/quest aggregation containers, and focus timers. It is invoked:

- immediately before a savegame is read (`kPreLoadGame`),
- after load and on new game (`kPostLoadGame` / `kNewGame`), and
- when InfinityUI begins rebuilding the HUD.

No Scaleform functions are invoked by this reset, so the InfinityUI patch phase remains free of re-entrant UI calls.

## 2. Strict INI parsing

Recognized float, boolean and unsigned-integer settings are committed only after a complete valid parse. Invalid values such as `abc`, `nan`, `inf`, malformed booleans, trailing junk, negative unsigned values, or overflow are ignored and logged. Because CNO parses defaults first and user settings last, ignoring an invalid override preserves the last valid lower-priority value.

## 3. HUD Compatibility Report

After `FinishLoadInstances` and the normal layout refresh, CNO writes a compact report containing:

- HUD movie URL,
- compatibility mode (`full`, `layout-only`, or `no-compass`),
- detected compass path and capability score,
- whether the full CNO ActionScript API is present,
- detected QuestItemList path and score,
- HUD-root detection,
- whether the CoMAP compatibility hook is active, and
- the native compass-holder baseline when available.

The report runs once per final HUD binding, not per frame.

## 4. Marker-index diagnostics

On focused-marker changes only, CNO checks:

- engine marker count vs `CompassMarkerList` size,
- focused marker index vs `CompassMarkerList` bounds, and
- the focused Scaleform clip vs the clip referenced by the matching marker-list entry.

Each mismatch is logged at most once per runtime lifecycle. The diagnostic path never rewrites an index, clip, quest, hook, or marker buffer.

## Explicitly unchanged

- Skyrim 1.7.104 hook offsets and relocation IDs
- v5.1.2 interior marker focus math
- quest/location/enemy/player-set marker hook behavior
- MCM IDs/defaults and settings priority
- `CompassNavigationOverhaul.esp`
- all translation files
- SWF sources/release no-SWF policy
- v4.6 DLL/PDB packaging requirements
