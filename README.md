## v5.1.6 Quest target context audit

v5.1.6 is the follow-up full audit of the indoor/Miscellaneous quest fixes. The quest-marker hook now accepts both target-context layouts seen around the original CNO hook site: a direct `TESQuestTarget*` and the original slot/wrapper form whose first qword contains the target pointer. CNO never calls `GetTrackingRef()` on the raw hook context; it first matches a candidate by pointer identity against a known displayed objective target. The quest-payload refresh also fingerprints the expanded objective text, preventing stale radiant/alias text while the same marker remains focused.

## v5.1.5 Quest payload refresh

v5.1.5 fixes a stale quest-prompt edge case that could remain after changing quest tracking in the Journal while the same indoor/door marker stayed focused. CNO now compares the currently collected quest payload with the payload last rendered to Scaleform and rebuilds the quest list only when that payload changes. If no tracked quest remains on the same marker, the old prompt is cleared once.

## v5.1.4 Miscellaneous tracking fix

v5.1.4 corrects Miscellaneous quest prompts that could show multiple untracked tasks when several objectives resolved to the same interior door/location marker. CNO now associates quest metadata with the exact `TESQuestTarget` being processed by Skyrim's tracked-marker path. Vanilla marker creation remains authoritative and is never suppressed if the extra CNO target context cannot be matched.

## v5.1.3 runtime robustness / diagnostics

v5.1.3 is a conservative robustness pass over v5.1.2. It resets transient marker/focus state across save-load and HUD lifecycle boundaries, ignores malformed INI values instead of converting them to zero, writes a concise HUD Compatibility Report after final InfinityUI binding, and adds marker-index diagnostics that never modify runtime marker state. The v5.1.2 interior focus fix and all existing hooks/offsets remain unchanged.

## v5.1.1 Windows build fix

v5.1.1 is build-script-only. It fixes a Windows `cmd.exe` quoting bug in the CMake version probe that could repeatedly report an empty `""""` command instead of reaching CMake/MSVC. CMake is now invoked directly for validation, and `vswhere.exe` discovery avoids direct quoted-executable `FOR /F` execution. No runtime C++ code, hooks, HUD behavior, MCM data, ESP, translations, or packaging contents changed from v5.1.

## v5.1 Safe audit hardening

v5.1 is a conservative safety pass over the v5 universal-HUD work. It does not change marker hook offsets or intended gameplay/UI behaviour. The update reduces CTD and edge-case risk by deferring Scaleform initialization until InfinityUI finishes patching, clearing provisional HUD bindings when final discovery fails, guarding optional third-party ActionScript functions, rejecting non-finite settings/transforms, removing debug-time Scaleform traversal from patch callbacks, and avoiding duplicate MCM/HUD refresh work.

The ESP, MCM IDs/defaults, ten translations, v4.5 settings migration, v4.6 PDB packaging, v4.3 fast-travel baseline logic, and normal no-SWF release policy remain unchanged. A fresh Windows build and in-game test are still required before treating the v5.1 runtime changes as fully validated.

## v5.0 Universal HUD compatibility

v5 keeps the installed HUD/compass skin in control of the visuals. The SKSE plugin no longer assumes one exact Vanilla/CNO `HUDMovieBaseInstance.CompassShoutMeterHolder.Compass` hierarchy for runtime binding.

- The live compass and CNO quest-list overlay are discovered by Scaleform capabilities and behaviour, with the old canonical paths retained only as fast fallbacks.
- Compass MCM offset/scale is applied to the actual runtime parent/holder and uses that skin's native transform as the 0/0/100 baseline.
- A renamed or reparented compass can therefore keep its own native size and position.
- The quest-list overlay is optional; failure to find it no longer disables focused-marker/compass processing.
- If the active compass does not expose CNO's ActionScript API, CNO preserves native marker rendering and falls back to layout-only compatibility instead of forcing CNO marker code onto an unknown HUD.
- The normal Vortex package intentionally contains **no Compass/QuestItemList SWFs**, so it never bakes Vanilla, Nordic UI, SkyHUD, or another user's skin into the update.

This is designed for Vanilla-style CNO layouts, Nordic UI/CNO skins, SkyHUD-style layouts, and other UI replacers that preserve Skyrim's normal compass semantics. A HUD that completely replaces/removes the expected marker data and CNO API may still need a dedicated adapter; v5 should fail safely rather than crash or overwrite that HUD.

### Optional local adaptive SWF tool

`Build-Adaptive-SWF-1.7.104.cmd` is provided as an **optional local compatibility tool**. It takes the currently installed CNO/UI SWFs as its base, updates only CNO ActionScript hooks, and preserves that base SWF's graphics, timelines, symbols and Scaleform metadata. Its output goes to `adaptive-output/Interface/InfinityUI`.

Those generated SWFs are deliberately not included by `Build-CNO-1.7.104.bat`. They are tied to the UI design present on the machine that generated them and are intended for local testing/compatibility work only.

# Compass Navigation Overhaul v2.2.0 - Skyrim 1.7.104.0 Update

Unofficial source update of [Compass Navigation Overhaul](https://github.com/alexsylex/CompassNavigationOverhaul) by alexsylex for Skyrim Special Edition / Anniversary Edition runtime **1.7.104.0**.

This repository builds on the previously MSVC-built and in-game-tested Skyrim 1.7.104.0 update plus the stability fixes documented in `FIX_NOTES_2026-09-23.md`. The new v5 universal-HUD binding changes require a fresh Windows/MSVC build and in-game validation. This repository is an **update/patch**, not a replacement for the original Compass Navigation Overhaul package. Install the original CNO 2.2.0 first, then install this update after it and let this update win the DLL conflict. The update includes:

- Skyrim 1.7.104.0 and SKSE 2.3.1 compatibility work.
- Your original CNO MCM layout is preserved and extended with the one additional DLL-supported focus-angle setting; the proven `CompassNavigationOverhaul.esp` MCM registration plugin is restored, and all nine official Skyrim Special Edition interface languages plus the existing Czech translation are included.
- Live reload of MCM settings when the Journal/MCM is closed.
- The MCM writes user overrides to `Data\MCM\Settings\CompassNavigationOverhaul.ini`; the original CNO INI remains intact.
- A fix for undiscovered locations: the question-mark icon is preserved, the undiscovered name stays hidden, and distance/height information remains visible.
- Safe suppression of the lingering Alternate Perspective start-room compass marker without modifying Alternate Perspective quest state.
- CTD fix for the InfinityUI `QuestItemList.swf` patch path: CNO no longer converts Scaleform display objects to strings during HUD replacement callbacks.
- Fast-travel/HUD-reload layout fix: compass MCM position and scale are applied from a stable native holder baseline and no longer accumulate after repeated HUD rebuilds.
- Universal HUD runtime discovery: renamed/reparented compass layouts are detected by capabilities, and the active UI skin owns its native visuals/transform.

## Requirements

- Skyrim Special Edition / Anniversary Edition 1.7.104.0
- SKSE 2.3.1
- Address Library for SKSE Plugins for runtime 1.7.104.0
- Infinity UI
- SkyUI and MCM Helper for the included MCM configuration

The **original Compass Navigation Overhaul 2.2.0 is required**. This update intentionally does not redistribute the original Compass/QuestItemList SWFs or other original runtime assets.

Refer to the [original Nexus page](https://www.nexusmods.com/skyrimspecialedition/mods/74484) for the complete mod description, installation requirements, compatibility information, and original permissions.

## Build

For the easiest Windows build, run:

```text
Build-CNO-1.7.104.bat
```

The batch detects Visual Studio 2022/MSVC, CMake and Ninja, creates a pinned local vcpkg toolchain under `_toolchain\vcpkg` when needed, performs a RelWithDebInfo SE/AE build, writes a diagnostic log to `build-logs`, and places the resulting DLL/PDB plus a **Vortex-ready patch ZIP containing the DLL, matching PDB, ESL-flagged MCM ESP, MCM configuration and translations** under `dist`. The normal v5 release does not package Compass/QuestItemList SWFs, so the user's installed HUD design remains authoritative. If Git/CMake/Ninja/VS Build Tools are missing and `winget` is available, the batch attempts to install them automatically.

The project uses CMake and vcpkg. Its manifest pins vcpkg baseline `00c5775211f45cd08b37fce0484b4cb940e422ab`. The custom CommonLib port pins:

- Repository: `alandtse/CommonLibSSE-NG`
- Version: `7.5.4`
- Commit: `c5424463bba9af0d75cde8640ba7ddd4cacb9e39`

Configure and build the SE/AE target with the presets from `CMakePresets.json`:

```powershell
cmake --preset build-relwithdebinfo-se-only
cmake --build --preset relwithdebinfo-se-only
```

For a debug build use `build-debug-se-only` followed by `cmake --build --preset debug-se-only`. This 1.7.104.0 update intentionally does not expose VR presets.


## MCM package

The update includes the same lightweight MCM registration plugin that was present in the previously working 1.7.104 MCM package. `CompassNavigationOverhaul.esp` is ESL-flagged (light plugin), contains the Start Game Enabled `CompassNavigationOverhaul_MCM_Quest`, and attaches MCM Helper's existing `MCM_ConfigBase` with `ModName = CompassNavigationOverhaul`. No separate CNO `.pex` is required.


The release ZIP generated by `Build-CNO-1.7.104.bat` contains only the files owned by this update:

```text
CompassNavigationOverhaul.esp
SKSE/
└─ Plugins/
   ├─ CompassNavigationOverhaul.dll
   └─ CompassNavigationOverhaul.pdb
MCM/
└─ Config/
   └─ CompassNavigationOverhaul/
      ├─ config.json
      └─ settings.ini
Interface/
└─ Translations/
   ├─ CompassNavigationOverhaul_ENGLISH.txt
   ├─ CompassNavigationOverhaul_FRENCH.txt
   ├─ CompassNavigationOverhaul_ITALIAN.txt
   ├─ CompassNavigationOverhaul_GERMAN.txt
   ├─ CompassNavigationOverhaul_SPANISH.txt
   ├─ CompassNavigationOverhaul_POLISH.txt
   ├─ CompassNavigationOverhaul_RUSSIAN.txt
   ├─ CompassNavigationOverhaul_JAPANESE.txt
   ├─ CompassNavigationOverhaul_CHINESE.txt
   └─ CompassNavigationOverhaul_CZECH.txt
LICENSE
NOTICE.md
THIRD_PARTY_NOTICES.md
licenses/
```

The original CNO SWFs and original `SKSE/Plugins/CompassNavigationOverhaul.ini` are deliberately not copied into this patch. The small `CompassNavigationOverhaul.esp` is included because it is the MCM registration plugin, not an original CNO gameplay plugin. The MCM uses localized `$CNO_*` keys; Skyrim/MCM Helper automatically selects the matching `Interface/Translations` file for the current game language (all nine official Skyrim SE language files plus the retained Czech translation are included). MCM Helper stores user changes in `Data/MCM/Settings/CompassNavigationOverhaul.ini`. The DLL reads the shipped MCM defaults first, then the legacy/original CNO INI, then the user's MCM settings, so user MCM values have highest priority. Closing the Journal/MCM reloads the values and refreshes the CNO HUD layout.

### Supported MCM languages

The patch ships localized MCM text for all nine interface languages officially supported by Skyrim Special Edition: English, French, Italian, German, Spanish (Spain), Polish, Russian, Japanese, and Traditional Chinese. The Czech translation from the previously working MCM package is retained as an additional tenth file. Skyrim/SKSE chooses the matching `Interface/Translations/CompassNavigationOverhaul_<LANGUAGE>.txt` file from the current `sLanguage` setting. Translation files are stored as UTF-16 LE with BOM and CRLF line endings.

## Packaging Licenses

`tools/Add-LicensesToPackage.ps1` adds this repository's license, notice, and dependency license files to an existing release ZIP without changing the mod payload.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\Add-LicensesToPackage.ps1 -ZipPath ".\CompassNavigationOverhaul.zip"
```

## Credits

- Original mod and source: alexsylex
- Skyrim 1.7.104.0 update and MCM fixes: DeadOnKeyboard
- CommonLibSSE-NG and all dependency authors listed in `THIRD_PARTY_NOTICES.md`

## License

This repository and the combined Skyrim 1.7.104.0 update are distributed under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. The complete GPL text is the root `LICENSE` file so GitHub can identify the repository license correctly.

Compass Navigation Overhaul was originally released by alexsylex under the MIT License. That original copyright and permission notice is preserved in `licenses/CompassNavigationOverhaul-Original-MIT.txt`. The MIT-licensed original portions remain subject to that notice; distribution of the combined update is additionally governed by the GPL terms stated above.

The source build is pinned to CommonLibSSE-NG 7.5.4. Its GPL-3.0-or-later text and upstream Modding/Linking Exceptions are retained in `licenses/CommonLibSSE-NG-GPL-3.0-or-later.txt` and `licenses/CommonLibSSE-NG-EXCEPTIONS.md`.

Third-party licenses and build dependency provenance are documented in `THIRD_PARTY_NOTICES.md` and `licenses/`.

Game data, Bethesda assets, original SWFs, and externally distributed mod assets are not relicensed by this repository. Their original licenses and permissions continue to apply.

## Upgrading from an older CNO/MCM build

v4.5 preserves existing layout settings when updating from older Compass Navigation Overhaul builds.

Settings are loaded in this order (later entries override earlier ones):

1. `Data/MCM/Config/CompassNavigationOverhaul/settings.ini` (shipped defaults)
2. `Data/SKSE/Plugins/CompassNavigationOverhaul.ini` (legacy/original CNO settings)
3. `Data/MCM/Settings/CompassNavigationOverhaul.ini` (MCM Helper user settings)

If the MCM Helper user-settings file does not exist yet but the legacy CNO INI does, CNO copies the legacy INI to the MCM user-settings location once before loading settings. Existing MCM user settings are never overwritten by this migration.

This prevents an update from unexpectedly restoring the Compass scale to the 100% package default.
