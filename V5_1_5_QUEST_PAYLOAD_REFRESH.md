# v5.1.5 Quest Payload Refresh

## Problem

v5.1.4 correctly tied CNO quest metadata to Skyrim's exact `TESQuestTarget*`, so untracked Miscellaneous objectives were no longer rebuilt from a shared interior door/reference.

A separate presentation edge case remained: if the player changed quest tracking in the Journal while keeping the same indoor marker focused, `UpdateFocusedMarker()` intentionally reported no focus change because the marker reference itself was unchanged. The Scaleform quest list was therefore not rebuilt and could retain a stale prompt that had been rendered before the Journal change.

## Fix

v5.1.5 keeps a compact canonical snapshot of the quest payload that was actually rendered for the focused marker. The snapshot contains the quest/objective identity, radiant instance ID, objective order, quest type, age/order information, same-location flag, and quest label.

Each compass update compares the currently collected payload with the last rendered payload:

- same marker + same payload: no Scaleform rebuild;
- same marker + tracking/objective change: clear and rebuild the quest list;
- same marker + no quest remains: clear the stale list once;
- quest list unavailable/hidden: invalidate the rendered snapshot so it repopulates when available again;
- focus/HUD/save lifecycle reset: discard the rendered snapshot together with the existing marker state.

This avoids a per-frame `RemoveAllQuests`/`AddQuest` loop while still refreshing immediately after Journal tracking changes.

## Unchanged

- v5.1.4 exact `TESQuestTarget*` tracking fix
- v5.1.2 indoor marker focus alignment
- location/enemy/player marker hooks
- MCM IDs/defaults and settings migration
- `CompassNavigationOverhaul.esp`
- translations and SWFs
- DLL/PDB packaging
