# Third-Party Notices

This project uses third-party software through its CMake and vcpkg build configuration. License texts distributed with this repository are in `licenses/`.

## CommonLibSSE-NG build dependency

The committed custom port pins CommonLibSSE-NG to the exact revision used for this source tree:

- Repository: https://github.com/alandtse/CommonLibSSE-NG
- Version: `7.5.4`
- Commit: `c5424463bba9af0d75cde8640ba7ddd4cacb9e39`
- License: GNU GPL v3-or-later with the upstream Modding/Linking Exceptions
- vcpkg baseline: `00c5775211f45cd08b37fce0484b4cb940e422ab`
- Build target: Skyrim SE/AE only (`ENABLE_SKYRIM_VR=OFF`)

CommonLibSSE-NG 7.5.4 is intentionally used because the 7.5.x line contains the corrected Skyrim 1.7.x `PlayerCharacter` runtime-data accessors required by the 1.7.104.0 target.

The CommonLibSSE-NG port also pulls its normal build dependencies through vcpkg. Those packages keep their own upstream licenses and copyright notices. The license files already bundled in this repository are retained for distribution/notice purposes.

## Components with bundled notice files

| Component | License | License file |
| --- | --- | --- |
| CommonLibSSE-NG 7.5.4 | GPL-3.0-or-later with Modding/Linking Exceptions | `licenses/CommonLibSSE-NG-GPL-3.0-or-later.txt`, `licenses/CommonLibSSE-NG-EXCEPTIONS.md` |
| DirectXMath | MIT | `licenses/DirectXMath-MIT.txt` |
| DirectX Tool Kit | MIT | `licenses/DirectXTK-MIT.txt` |
| fmt | MIT | `licenses/fmt-MIT.txt` |
| rapidcsv | BSD 3-Clause | `licenses/rapidcsv-BSD-3-Clause.txt` |
| spdlog | MIT | `licenses/spdlog-MIT.txt` |
| Xbyak | BSD 3-Clause | `licenses/Xbyak-BSD-3-Clause.txt` |

`licenses/CommonLibSSE-NG-MIT.txt` and `licenses/OpenVR-BSD-3-Clause.txt` are retained as historical/upstream notices from the earlier dependency layout; the current SE/AE-only custom port does not build OpenVR.
