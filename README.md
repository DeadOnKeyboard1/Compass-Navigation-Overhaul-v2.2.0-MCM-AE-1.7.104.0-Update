# Compass Navigation Overhaul v2.2.0 - Skyrim 1.7.104.0 Update

Unofficial source update of [Compass Navigation Overhaul](https://github.com/alexsylex/CompassNavigationOverhaul) by alexsylex for Skyrim Special Edition / Anniversary Edition runtime **1.7.104.0**.

This repository preserves the original project and contains the source used for the tested 1.7.104.0 release. The update includes:

- Skyrim 1.7.104.0 and SKSE 2.3.1 compatibility work.
- MCM configuration support included with the release package.
- Live reload of supported MCM settings.
- A fix for undiscovered locations using quest-marker icons instead of question-mark icons.
- Compatibility cleanup for Alternate Perspective start-room state.

## Requirements

- Skyrim Special Edition / Anniversary Edition 1.7.104.0
- SKSE 2.3.1
- Address Library for SKSE Plugins for runtime 1.7.104.0
- Infinity UI
- SkyUI and MCM Helper for the included MCM configuration

Refer to the [original Nexus page](https://www.nexusmods.com/skyrimspecialedition/mods/74484) for the complete mod description, installation requirements, compatibility information, and original permissions.

## Build

The project uses CMake and vcpkg. Its manifest pins vcpkg baseline `8d3649ba34aab36914ddd897958599aa0a91b08e`. The custom CommonLib port pins:

- Repository: `alandtse/CommonLibVR`
- Commit: `2b983f5281bfadd26ee20787390d2513e8ffe38a`
- Package version: `4.0.0#1`
- OpenVR commit: `ebdea152f8aac77e9a6db29682b81d762159df7e`

Configure and build with a Visual Studio 2022 preset from `CMakePresets.json`:

```powershell
cmake --preset vs2022-windows-vcpkg
cmake --build build --config Release
```

The exact preset name may be changed to another compatible preset already defined in the project.

## Packaging Licenses

`tools/Add-LicensesToPackage.ps1` adds this repository's license, notice, and dependency license files to an existing release ZIP without changing the mod payload.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\Add-LicensesToPackage.ps1 -ZipPath ".\CompassNavigationOverhaul.zip"
```

## Credits

- Original mod and source: alexsylex
- Skyrim 1.7.104.0 update and MCM fixes: DeadOnKeyboard
- CommonLibSSE/CommonLibVR and all dependency authors listed in `THIRD_PARTY_NOTICES.md`

## License

The project source is licensed under the MIT License. Original copyright and permission notices are preserved in `LICENSE`. Third-party licenses and build dependency provenance are documented in `THIRD_PARTY_NOTICES.md` and `licenses/`.

Game data, Bethesda assets, and externally distributed mod assets are not relicensed by this repository. Their original licenses and permissions continue to apply.
