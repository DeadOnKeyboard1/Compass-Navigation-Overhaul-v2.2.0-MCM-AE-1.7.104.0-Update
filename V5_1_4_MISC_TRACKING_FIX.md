# v5.1.4 Miscellaneous Quest Tracking Fix

## Problem

The v5.1.3 quest hook let Skyrim create its marker first, then CNO searched every journal-visible player objective for a target resolving to the same `RefHandle`. Miscellaneous tasks frequently share an interior door or location reference, so one tracked target could cause several unrelated/untracked Misc objectives to be aggregated into the prompt.

## Fix

The verified `HUDMarkerManager::UpdateQuests` AddMarker patch site retains the current `TESQuestTarget*` in RBX. v5.1.4 preserves that context as a sixth argument to CNO's hook, matching the original CNO strategy. CNO now:

1. calls Skyrim's `AddMarker` first;
2. keeps the exact target context from that marker call;
3. compares that pointer only against known objective target pointers;
4. dereferences it only after pointer identity succeeds;
5. confirms `GetTrackingRef()` still resolves to the intercepted marker; and
6. adds only that exact objective to the CNO quest prompt.

If the extra target context is null or cannot be matched, CNO simply skips its extra quest metadata. The already-created vanilla marker is preserved.

## Unchanged

- location, enemy and player-set marker hooks
- v5.1.2 interior focus alignment
- v5.1.3 lifecycle/INI/diagnostic hardening
- MCM IDs/defaults and settings migration
- `CompassNavigationOverhaul.esp`
- translations and SWFs
- DLL/PDB packaging
