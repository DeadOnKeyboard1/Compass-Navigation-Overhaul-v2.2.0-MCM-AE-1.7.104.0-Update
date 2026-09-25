# v5.1.1 Windows build-script fix

This hotfix changes only `Build-CNO-1.7.104.bat`.

The v5.1 builder used a nested `CALL`/`FOR /F` command to parse the CMake version:

`call ""%CMAKE_EXE%" --version"`

On affected Windows `cmd.exe` parsing paths this can be interpreted as an empty quoted command and produce repeated messages that `""""` could not be found.

v5.1.1 removes that parsing step. The detected CMake executable is invoked directly with `--version` and the build aborts cleanly only if that executable cannot actually start. Visual Studio discovery was also changed to capture `vswhere.exe` output to a temporary file before reading it, avoiding a similar quoted-executable `FOR /F` path.

No C++ runtime code or release payload semantics changed from v5.1.
