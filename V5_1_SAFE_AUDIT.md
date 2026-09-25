# v5.1 Safe CTD / bug audit

Target: Skyrim AE 1.7.104.0 / SKSE 2.3.1
Base: v5.0 Universal HUD Compatibility

This pass intentionally limits changes to defensive, low-risk corrections. It does **not** alter the known runtime hook offsets, relocation IDs, marker selection rules, MCM setting IDs/defaults, ESP records, translations, or the public release SWF policy.

## Safe runtime hardening

- InfinityUI PostPatch callbacks only bind candidates. Compass setup, quest-list initialization and layout work are deferred until `FinishLoadInstances`, after the HUD patch batch has completed.
- Debug-only Scaleform member walking and `LocalToGlobal` introspection were removed from replacement callbacks.
- If final universal discovery cannot identify the Compass or QuestItemList, provisional bindings are explicitly invalidated before any later layout refresh. Incomplete `FinishLoadInstances` messages invalidate all provisional HUD bindings and skip the refresh.
- Journal/MCM close schedules one next-frame refresh when the SKSE task interface is available, with one immediate fallback only when it is not.
- Partial/alternate QuestItemList implementations are guarded before optional ActionScript calls; direct fallback registration prevents duplicate entries in `HudElements`.
- Quest-list stage dimensions, converted coordinates and computed layout values are checked for finite numeric values.
- Compass native baseline values and final transforms are checked for finite/positive values before applying them.
- Settings loaded as textual `NaN`/`Inf` are replaced with shipped safe defaults before clamping/math.
- Marker processing retrieves the current `PlayerCamera` singleton at the point of use rather than depending on a cached pointer from an earlier update.
- Relative CALL-site validation now verifies that the entire instruction lies in a committed readable page before dereferencing it.
- Optional adaptive ActionScript re-resolves the HUD root during actual Compass initialization and avoids duplicate quest-list HUD registration.

## Deliberately unchanged

- Quest/location/enemy/player marker hook offsets and relocation IDs
- HUDMarkerManager private runtime layout and frame-offset handling
- CoMAP/Map Marker Framework hook/signature logic
- Marker focus thresholds/semantics and undiscovered-marker behaviour
- MCM IDs/default settings and settings migration priority
- `CompassNavigationOverhaul.esp`
- all ten translation files
- CommonLibSSE-NG/vcpkg pins
- normal release policy: no generated Compass/QuestItemList SWFs are bundled

## Validation status

Static/source and package checks were completed in the artifact environment. A Windows/MSVC build and in-game validation are still required. Recommended smoke tests are startup/HUD load, repeated fast travel, opening/closing Journal and MCM, 0/0/100 compass reset, Vanilla/CNO and Nordic-style HUDs, quest/location/enemy/player markers, and an InfinityUI HUD rebuild.
