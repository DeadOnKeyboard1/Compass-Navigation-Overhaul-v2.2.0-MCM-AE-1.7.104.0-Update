# Third-Party Notices

This project uses third-party software through its CMake and vcpkg build configuration. License texts distributed with this repository are in `licenses/`.

## Build Dependency Record

The committed custom port pins CommonLibSSE-NG/CommonLibVR to the following revision:

- Implementation: CommonLibSSE-NG / CommonLibVR
- Repository: https://github.com/alandtse/CommonLibVR
- Commit: `2b983f5281bfadd26ee20787390d2513e8ffe38a`
- Declared package version: `4.0.0#1`
- License at the pinned revision: MIT
- Bundled OpenVR repository: https://github.com/ValveSoftware/openvr
- Bundled OpenVR commit: `ebdea152f8aac77e9a6db29682b81d762159df7e`
- vcpkg baseline: `8d3649ba34aab36914ddd897958599aa0a91b08e`

The existing tested release DLL was not recompiled during this GitHub and licensing pass. Generated build metadata shows it was linked from a prebuilt vcpkg package prefix outside this repository. The committed port and manifest are therefore the authoritative reproducible dependency definition for future builds; the original external package checkout is not part of this repository.

## Components

| Component | License | License file |
| --- | --- | --- |
| CommonLibSSE-NG / CommonLibVR 4.0.0#1 | MIT | `licenses/CommonLibSSE-NG-MIT.txt` |
| OpenVR | BSD 3-Clause | `licenses/OpenVR-BSD-3-Clause.txt` |
| DirectXMath | MIT | `licenses/DirectXMath-MIT.txt` |
| DirectX Tool Kit | MIT | `licenses/DirectXTK-MIT.txt` |
| fmt | MIT | `licenses/fmt-MIT.txt` |
| rapidcsv | BSD 3-Clause | `licenses/rapidcsv-BSD-3-Clause.txt` |
| spdlog | MIT | `licenses/spdlog-MIT.txt` |
| Xbyak | BSD 3-Clause | `licenses/Xbyak-BSD-3-Clause.txt` |

The table documents the dependencies declared by the pinned CommonLib port, including header-only and transitive build components. Inclusion of a license notice does not imply endorsement by its authors.
