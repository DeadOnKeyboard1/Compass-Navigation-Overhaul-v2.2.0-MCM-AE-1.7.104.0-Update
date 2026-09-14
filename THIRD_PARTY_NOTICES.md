# Third-Party Notices

This project uses third-party software through its CMake and vcpkg build configuration. License texts distributed with this repository are in `licenses/`.

## Source Manifest Record

The committed custom port pins CommonLibSSE-NG/CommonLibVR to the following revision:

- Implementation: CommonLibSSE-NG / CommonLibVR
- Repository: https://github.com/alandtse/CommonLibVR
- Commit: `2b983f5281bfadd26ee20787390d2513e8ffe38a`
- Declared package version: `4.0.0#1`
- License at the pinned revision: MIT
- Bundled OpenVR repository: https://github.com/ValveSoftware/openvr
- Bundled OpenVR commit: `ebdea152f8aac77e9a6db29682b81d762159df7e`
- vcpkg baseline: `8d3649ba34aab36914ddd897958599aa0a91b08e`

The custom port committed in this repository is the project's legacy source dependency definition. It describes an older MIT-licensed CommonLib revision, but it does not identify the prebuilt CommonLib package used by the already-tested DLL.

## Tested Binary Record

The existing tested release DLL was not recompiled during this GitHub and licensing pass. Its matching PDB and generated linker metadata show that it was linked against a newer prebuilt `commonlibsse-ng` package from an external vcpkg prefix. The PDB records the package source directory prefix `b2f24ebf25`, but it does not retain the complete Git commit ID.

Because the binary dependency differs from the legacy MIT port committed here, binary distributions conservatively include and comply with CommonLibSSE-NG's current GNU GPL v3-or-later license and the accompanying Modding/Linking Exceptions. The original MIT notice is retained because CommonLib is derived in part from MIT-licensed code and the Compass Navigation Overhaul source itself remains MIT-licensed.

## Components

| Component | License | License file |
| --- | --- | --- |
| CommonLibSSE-NG / CommonLibVR legacy source port | MIT | `licenses/CommonLibSSE-NG-MIT.txt` |
| CommonLibSSE-NG used by the tested binary | GPL-3.0-or-later with Modding and Linking Exceptions | `licenses/CommonLibSSE-NG-GPL-3.0-or-later.txt`, `licenses/CommonLibSSE-NG-EXCEPTIONS.md` |
| OpenVR | BSD 3-Clause | `licenses/OpenVR-BSD-3-Clause.txt` |
| DirectXMath | MIT | `licenses/DirectXMath-MIT.txt` |
| DirectX Tool Kit | MIT | `licenses/DirectXTK-MIT.txt` |
| fmt | MIT | `licenses/fmt-MIT.txt` |
| rapidcsv | BSD 3-Clause | `licenses/rapidcsv-BSD-3-Clause.txt` |
| spdlog | MIT | `licenses/spdlog-MIT.txt` |
| Xbyak | BSD 3-Clause | `licenses/Xbyak-BSD-3-Clause.txt` |

The table documents the dependencies declared by the pinned CommonLib port, including header-only and transitive build components. Inclusion of a license notice does not imply endorsement by its authors.
