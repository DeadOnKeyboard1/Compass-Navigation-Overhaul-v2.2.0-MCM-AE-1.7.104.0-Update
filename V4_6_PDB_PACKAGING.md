# v4.6 PDB packaging

Target: Skyrim AE 1.7.104.0

- `Build-CNO-1.7.104.bat` now requires the freshly generated `CompassNavigationOverhaul.pdb`.
- The build fails if the matching PDB cannot be found after linking.
- The PDB is staged beside `CompassNavigationOverhaul.dll` at `SKSE/Plugins/CompassNavigationOverhaul.pdb`.
- The Vortex-ready ZIP therefore contains DLL + matching PDB + ESL-flagged ESP + MCM configuration + translations.
- An old PDB is never reused because symbols must match the exact DLL build.
- Runtime behavior is unchanged from v4.5.
