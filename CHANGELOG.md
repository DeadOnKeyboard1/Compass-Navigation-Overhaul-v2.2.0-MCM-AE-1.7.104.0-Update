## v5.1.6 - Quest target context audit

- Corrected the 1.7.104 quest-hook context handling: the RBX context may be either a direct `TESQuestTarget*` or the original CNO slot/wrapper whose first qword stores that pointer. Both forms are now supported, with the direct form preferred.
- `GetTrackingRef()` is still invoked only after pointer identity proves that the candidate belongs to a known displayed objective, preserving the CTD-safe v5.1.4 filtering.
- Extended the v5.1.5 rendered-payload fingerprint with the actual expanded objective text, so radiant/alias/tag text changes refresh while the same marker stays focused.
- Skips objective-text expansion while the quest list is unavailable or intentionally hidden.
- No changes to marker hook addresses, interior focus math, ESP, MCM IDs/defaults, translations, SWFs, build scripts, dependency pins, or release packaging policy.

## v5.1.5 - Quest payload refresh

- Refreshes the quest prompt when Journal tracking changes while the same compass/indoor marker remains focused.
- Clears stale quest prompts when the focused reference stops representing a tracked quest.
- Avoids per-frame Scaleform rebuilds by comparing the current quest payload with the last payload actually rendered.
- Invalidates the rendered payload cache across HUD/save lifecycle resets and while the quest list is unavailable/hidden.

## v5.1.4 - Miscellaneous quest tracking fix

- Fixed Miscellaneous task prompts aggregating every visible objective that shared the same interior door/location marker.
- Quest metadata now follows the exact `TESQuestTarget` from Skyrim's tracked-marker `AddMarker` call instead of reconstructing objective ownership only from the marker reference.
- Fully untracked Miscellaneous tasks no longer get pulled into CNO's quest list merely because their objective is journal-visible.
- The vanilla marker call still runs first; if the target context cannot be matched safely, CNO skips only its extra quest metadata and leaves Skyrim's marker untouched.
- No location/enemy/player marker hooks, interior focus math, MCM, ESP, translations, SWFs, or layout behavior changed.

## v5.1.3 - Runtime robustness and diagnostics

- Resets focused-marker, quest aggregation and focus timers before savegame load teardown, after load/new game, and when InfinityUI rebuilds the HUD.
- Invalid INI values are ignored instead of silently becoming zero; the last valid lower-priority/default value remains active and a warning identifies the bad key/file.
- Adds one compact Compatibility Report after each final HUD binding with detected compass/quest-list paths, capability scores, full-vs-layout-only mode, HUD-root detection, active CoMAP compatibility, and the captured native holder baseline.
- Adds diagnostics for engine marker-count vs `CompassMarkerList`, out-of-range focused marker indices, and focused Scaleform clip mismatches. Diagnostics run only on focus changes and never alter marker data.
- No hook offsets, relocation IDs, marker selection behavior, interior-focus math, MCM IDs/defaults, ESP data, translations, SWFs, or packaging policy changed.

## v5.1.2 - Interior marker focus alignment

- Fixed quest/detail focus being horizontally offset from the visible marker in rotated interior cells.
- Marker focus now uses the player/camera angle, matching Skyrim HUDMenu `UpdateCompassMarkers`, instead of adding the cell north rotation a second time.
- No marker hooks, quest data, MCM settings, SWF assets, ESP data, or layout offsets were changed.

## v5.1.1 - Windows build-script quoting fix

- Fixed the CMake version probe that could make `cmd.exe` repeatedly try to execute an empty `""""` command before CMake/MSVC were reached.
- CMake is now validated directly with `cmake --version`; parsing its version string is no longer required for the build.
- Hardened Visual Studio discovery by capturing `vswhere.exe` output before reading it, avoiding the same quoted-executable `FOR /F` edge case for paths under `Program Files`.
- No DLL source, hooks, marker behavior, MCM settings, ESP, translations, or runtime compatibility logic changed from v5.1.

## v5.1 - Safe CTD / edge-case hardening

- Deferred Compass and QuestItemList initialization until InfinityUI finishes its HUD patch batch, reducing re-entrant Scaleform work while the tree is being modified.
- Removed debug member traversal and coordinate introspection from InfinityUI replacement callbacks.
- Invalidates provisional Compass/QuestItemList bindings when final universal HUD discovery fails or the FinishLoadInstances message is incomplete.
- Journal/MCM close now performs exactly one settings/layout refresh instead of an immediate refresh plus a second next-frame refresh.
- Added defensive checks for partial/third-party QuestItemList implementations before invoking optional ActionScript functions.
- Prevents duplicate QuestItemList registration in `HudElements`.
- Rejects NaN/Infinity settings and non-finite Scaleform coordinates/transforms before they can reach HUD layout code.
- Refreshes the live PlayerCamera pointer at marker-processing time instead of relying on a previously cached frame pointer.
- Tightened hook-site memory validation to require a readable committed page before inspecting a CALL opcode.
- Optional adaptive SWF sources re-resolve the HUD root at Compass initialization and avoid duplicate quest-list HUD registration.
- No marker hook offsets, relocation IDs, marker behavior, MCM IDs/defaults, ESP, translations, or release SWF policy were changed.

## v5.0 - Universal HUD compatibility

- Added runtime compass discovery by Scaleform capabilities instead of requiring one exact Vanilla/CNO instance path.
- Retained canonical Vanilla/CNO paths only as fast fallbacks.
- Compass layout now targets the actual runtime parent/holder, so MCM 0/0/100 preserves the active UI skin's native transform.
- QuestItemList binding is optional and no longer blocks the core compass/focused-marker path.
- Unknown compass layouts without the CNO ActionScript API keep native marker rendering and use safe layout-only compatibility.
- InfinityUI callbacks compare live Scaleform objects rather than relying on hardcoded path strings.
- Added bounded/cycle-safe HUD tree discovery without `GFxValue::ToString()` on display objects.
- Added optional `Build-Adaptive-SWF-1.7.104.cmd` for local skin-preserving ActionScript compatibility overlays.
- Normal Vortex packaging intentionally ships no Compass/QuestItemList SWFs, preventing one user's Vanilla/Nordic/SkyHUD design from being baked into the public update.
- Preserves v4.2 CTD fix, v4.3 fast-travel baseline fix, v4.4 MCM ESP, v4.5 settings migration, and v4.6 matching PDB packaging.

## v4.6 - PDB packaging

- Require the freshly built `CompassNavigationOverhaul.pdb`.
- Ship the matching PDB next to the DLL in `SKSE/Plugins` inside the Vortex ZIP.
- Abort packaging if the PDB cannot be found or staged.
- No gameplay, MCM, marker, fast-travel, or settings-migration behavior changed from v4.5.

# Changelog

## v4.4 - Restore MCM light plugin

- Restored `CompassNavigationOverhaul.esp` from the previously working Skyrim 1.7.104 MCM package.
- The plugin is an ESL-flagged ESP (light plugin) and therefore uses an FE load-order slot.
- Verified the plugin contains the Start Game Enabled `CompassNavigationOverhaul_MCM_Quest`, attaches `MCM_ConfigBase`, and sets `ModName` to `CompassNavigationOverhaul`.
- No separate CNO Papyrus `.pex` is required; MCM Helper provides `MCM_ConfigBase`.
- The build now fails instead of creating an incomplete release if `CompassNavigationOverhaul.esp` is missing.
- The Vortex ZIP now places `CompassNavigationOverhaul.esp` at the Data root alongside `SKSE`, `MCM`, and `Interface`.
- Restored the existing Czech MCM localization as an additional translation and added the new keep-details-angle strings, for ten translation files total.
- Preserves all v4.3 fast-travel layout fixes and v4.2 InfinityUI CTD fixes.

## v4.3 - Fast-travel compass layout fix

- Fixed the compass position/scale drifting after fast travel or repeated InfinityUI HUD rebuilds.
- CNO no longer recaptures an already-offset `CompassShoutMeterHolder` as the new native baseline.
- The original holder position and scale are stored on the live Scaleform holder and reused across Compass-child replacements.
- A completely new HUD holder still captures its own native baseline once, preserving compatibility with alternate HUD layouts.
- Compass layout application is deferred until InfinityUI finishes the full HUD patch batch, preventing intermediate patch state from becoming the baseline.
- MCM offset/scale values are now idempotent: reloading the HUD or fast travelling repeatedly produces the same visual position and scale.

## v4.2 - InfinityUI / QuestItemList startup CTD fix

- Fixed an early startup CTD while InfinityUI patched `HUDMovieBaseInstance.QuestItemList`.
- Removed unsafe `GFxValue::ToString()` calls on Scaleform display objects from the InfinityUI message listener.
- CNO now identifies Compass and QuestItemList instances by resolving the known movie path and comparing the actual `GFxValue` object.
- Removed object-to-string conversion from the GFx debug loggers so debug logging cannot re-enter the same unsafe Scaleform conversion path.
- Preserves the v3.7/v4.1 marker fixes, MCM, all nine language files and GPL repository layout unchanged.

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

### Repository licensing

- Set the repository root `LICENSE` to GPL-3.0-or-later so GitHub detects the combined update under GPL.
- Preserved the original Compass Navigation Overhaul MIT notice in `licenses/CompassNavigationOverhaul-Original-MIT.txt`.
- Retained CommonLibSSE-NG GPL and Modding/Linking Exception texts under `licenses/`.

## v4.5 - Settings migration / upgrade compatibility
- Preserves Compass and Quest List settings when upgrading from older CNO builds.
- Corrected INI priority to: MCM defaults -> legacy SKSE CNO INI -> MCM user settings.
- If `Data/MCM/Settings/CompassNavigationOverhaul.ini` does not exist, the legacy `Data/SKSE/Plugins/CompassNavigationOverhaul.ini` is copied there once before settings are loaded.
- Existing MCM user settings are never overwritten by the migration.
- Fixes upgrades unexpectedly returning the Compass to the 100% default scale.
