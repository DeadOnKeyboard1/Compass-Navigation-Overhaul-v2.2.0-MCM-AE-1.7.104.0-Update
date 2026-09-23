@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ============================================================================
rem Compass Navigation Overhaul - Skyrim 1.7.104.0 build helper
rem Builds the SE/AE RelWithDebInfo target and creates a Vortex-ready patch package with DLL + MCM.
rem ============================================================================

cd /d "%~dp0"
set "ROOT=%CD%"
set "PROJECT=CompassNavigationOverhaul"
set "VCPKG_COMMIT=00c5775211f45cd08b37fce0484b4cb940e422ab"
set "BUILD_DIR=%ROOT%\build\relwithdebinfo-se-only"
set "DIST_DIR=%ROOT%\dist"
set "TOOLCHAIN_DIR=%ROOT%\_toolchain"
set "LOCAL_VCPKG=%TOOLCHAIN_DIR%\vcpkg"

for /f %%I in ('powershell.exe -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss" 2^>nul') do set "STAMP=%%I"
if not defined STAMP set "STAMP=build"
set "LOG_DIR=%ROOT%\build-logs"
set "LOG=%LOG_DIR%\CNO-build-%STAMP%.log"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%" >nul 2>&1
if not exist "%TOOLCHAIN_DIR%" mkdir "%TOOLCHAIN_DIR%" >nul 2>&1

call :say "Compass Navigation Overhaul - Skyrim 1.7.104.0"
call :say "Log: %LOG%"
call :say ""

rem ---- PowerShell ------------------------------------------------------------
where powershell.exe >nul 2>&1
if errorlevel 1 (
    call :fail "Windows PowerShell was not found."
    goto :failed
)

rem ---- Git -------------------------------------------------------------------
call :find_git
if not defined GIT_EXE (
    call :say "Git not found. Trying to install Git with winget..."
    where winget.exe >nul 2>&1
    if errorlevel 1 (
        call :fail "Git is required for the pinned vcpkg toolchain, and winget is unavailable."
        goto :failed
    )
    winget install --id Git.Git -e --source winget --accept-package-agreements --accept-source-agreements --silent >>"%LOG%" 2>&1
    call :find_git
    if not defined GIT_EXE (
        call :fail "Git installation completed/was attempted, but git.exe still cannot be found. Reopen the terminal and run this batch again."
        goto :failed
    )
)
call :say "Git: %GIT_EXE%"

rem ---- Visual Studio / MSVC --------------------------------------------------
call :find_vs
if not defined VS_PATH (
    call :say "Visual Studio C++ Build Tools not found. Trying to install VS 2022 Build Tools..."
    where winget.exe >nul 2>&1
    if errorlevel 1 (
        call :fail "Visual Studio 2022 with the C++ x64 toolset is required."
        goto :failed
    )
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget --accept-package-agreements --accept-source-agreements --silent --override "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" >>"%LOG%" 2>&1
    call :find_vs
    if not defined VS_PATH (
        call :fail "Visual Studio Build Tools installation completed/was attempted, but a usable VC toolset still cannot be found."
        goto :failed
    )
)
call :say "Visual Studio: %VS_PATH%"

if not exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
    call :fail "VsDevCmd.bat is missing from the detected Visual Studio installation."
    goto :failed
)

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -arch=x64 -host_arch=x64 >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Failed to initialize the Visual Studio x64 developer environment."
    goto :failed
)

where cl.exe >nul 2>&1
if errorlevel 1 (
    call :fail "MSVC cl.exe is not available after initializing Visual Studio."
    goto :failed
)

rem ---- CMake -----------------------------------------------------------------
call :find_cmake
if not defined CMAKE_EXE (
    call :say "CMake not found. Trying to install CMake with winget..."
    where winget.exe >nul 2>&1
    if errorlevel 1 (
        call :fail "CMake 3.21 or newer is required."
        goto :failed
    )
    winget install --id Kitware.CMake -e --source winget --accept-package-agreements --accept-source-agreements --silent >>"%LOG%" 2>&1
    call :find_cmake
    if not defined CMAKE_EXE (
        call :fail "CMake installation completed/was attempted, but cmake.exe still cannot be found."
        goto :failed
    )
)
call :say "CMake: %CMAKE_EXE%"

set "CMAKE_VERSION=unknown"
for /f "tokens=3" %%V in ('call ""%CMAKE_EXE%" --version" 2^>nul ^| findstr /b /c:"cmake version"') do set "CMAKE_VERSION=%%V"
call :say "CMake version: !CMAKE_VERSION!"
"%CMAKE_EXE%" --version >>"%LOG%" 2>&1

rem ---- Ninja -----------------------------------------------------------------
call :find_ninja
if not defined NINJA_EXE (
    call :say "Ninja not found. Trying to install Ninja with winget..."
    where winget.exe >nul 2>&1
    if errorlevel 1 (
        call :fail "Ninja is required by CMakePresets.json."
        goto :failed
    )
    winget install --id Ninja-build.Ninja -e --source winget --accept-package-agreements --accept-source-agreements --silent >>"%LOG%" 2>&1
    call :find_ninja
    if not defined NINJA_EXE (
        call :fail "Ninja installation completed/was attempted, but ninja.exe still cannot be found."
        goto :failed
    )
)
for %%D in ("%NINJA_EXE%") do set "PATH=%%~dpD;%PATH%"
call :say "Ninja: %NINJA_EXE%"

rem ---- Reproducible local vcpkg ---------------------------------------------
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    call :say "Using existing VCPKG_ROOT: %VCPKG_ROOT%"
) else (
    set "VCPKG_ROOT="
)

if not defined VCPKG_ROOT (
    set "VCPKG_ROOT=%LOCAL_VCPKG%"
    if not exist "%LOCAL_VCPKG%\.git" (
        call :say "Creating pinned vcpkg toolchain at _toolchain\vcpkg..."
        if exist "%LOCAL_VCPKG%" rmdir /s /q "%LOCAL_VCPKG%"
        mkdir "%LOCAL_VCPKG%" >>"%LOG%" 2>&1
        "%GIT_EXE%" -C "%LOCAL_VCPKG%" init >>"%LOG%" 2>&1
        if errorlevel 1 goto :vcpkg_failed
        "%GIT_EXE%" -C "%LOCAL_VCPKG%" remote add origin https://github.com/microsoft/vcpkg.git >>"%LOG%" 2>&1
        if errorlevel 1 goto :vcpkg_failed
        "%GIT_EXE%" -C "%LOCAL_VCPKG%" fetch --depth 1 origin %VCPKG_COMMIT% >>"%LOG%" 2>&1
        if errorlevel 1 goto :vcpkg_failed
        "%GIT_EXE%" -C "%LOCAL_VCPKG%" checkout --detach FETCH_HEAD >>"%LOG%" 2>&1
        if errorlevel 1 goto :vcpkg_failed
    ) else (
        call :say "Checking local vcpkg revision..."
        "%GIT_EXE%" -C "%LOCAL_VCPKG%" cat-file -e %VCPKG_COMMIT%^{commit} >nul 2>&1
        if errorlevel 1 (
            "%GIT_EXE%" -C "%LOCAL_VCPKG%" fetch --depth 1 origin %VCPKG_COMMIT% >>"%LOG%" 2>&1
            if errorlevel 1 goto :vcpkg_failed
        )
        "%GIT_EXE%" -C "%LOCAL_VCPKG%" checkout --detach %VCPKG_COMMIT% >>"%LOG%" 2>&1
        if errorlevel 1 goto :vcpkg_failed
    )

    if not exist "%LOCAL_VCPKG%\vcpkg.exe" (
        call :say "Bootstrapping vcpkg..."
        call "%LOCAL_VCPKG%\bootstrap-vcpkg.bat" -disableMetrics >>"%LOG%" 2>&1
        if errorlevel 1 goto :vcpkg_failed
    )
)

set "VCPKG_ROOT=%VCPKG_ROOT%"
set "VCPKG_DISABLE_METRICS=1"
call :say "vcpkg: %VCPKG_ROOT%"

rem ---- Incremental build by default ------------------------------------------
rem Reusing the build tree keeps the already-built CommonLib/vcpkg packages and
rem makes source-fix retries much faster. Set CNO_CLEAN=1 before launching this
rem batch if you explicitly want a completely fresh configure/build.
if /i "%CNO_CLEAN%"=="1" (
    call :say "CNO_CLEAN=1: removing previous RelWithDebInfo build tree..."
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
) else (
    if exist "%BUILD_DIR%\CMakeCache.txt" (
        call :say "Reusing existing RelWithDebInfo build tree and vcpkg packages..."
    ) else (
        call :say "No existing RelWithDebInfo build tree found; creating a fresh one..."
    )
)

rem ---- Configure -------------------------------------------------------------
call :say "Configuring RelWithDebInfo SE/AE build..."
"%CMAKE_EXE%" --preset build-relwithdebinfo-se-only >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "CMake configure failed."
    goto :failed
)

rem ---- Build -----------------------------------------------------------------
call :say "Building CompassNavigationOverhaul..."
"%CMAKE_EXE%" --build --preset relwithdebinfo-se-only --parallel >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "CMake build failed."
    goto :failed
)

rem ---- Locate output ---------------------------------------------------------
rem IMPORTANT: FOR /R with a literal filename can leave a non-existent candidate
rem path in the variable. Use DIR so only files that actually exist are returned.
set "DLL_PATH="
set "PDB_PATH="
for /f "usebackq delims=" %%F in (`dir /b /s /a-d "%BUILD_DIR%\CompassNavigationOverhaul.dll" 2^>nul`) do if not defined DLL_PATH set "DLL_PATH=%%~fF"
for /f "usebackq delims=" %%F in (`dir /b /s /a-d "%BUILD_DIR%\CompassNavigationOverhaul.pdb" 2^>nul`) do if not defined PDB_PATH set "PDB_PATH=%%~fF"

if not defined DLL_PATH (
    call :fail "Build finished but CompassNavigationOverhaul.dll was not found under the build directory."
    goto :failed
)
if not exist "%DLL_PATH%" (
    call :fail "The detected DLL path does not exist: %DLL_PATH%"
    goto :failed
)
call :say "Built DLL found: %DLL_PATH%"
if defined PDB_PATH call :say "Built PDB found: %PDB_PATH%"

rem ---- Package ---------------------------------------------------------------
call :say "Creating dist package..."
if exist "%DIST_DIR%\SKSE" rmdir /s /q "%DIST_DIR%\SKSE"
if not exist "%DIST_DIR%\SKSE\Plugins" mkdir "%DIST_DIR%\SKSE\Plugins" >nul 2>&1

copy /y "%DLL_PATH%" "%DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.dll" >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Could not copy the built DLL to dist\SKSE\Plugins."
    goto :failed
)
if not exist "%DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.dll" (
    call :fail "DLL copy reported success, but dist\SKSE\Plugins\CompassNavigationOverhaul.dll does not exist."
    goto :failed
)

rem Also place convenient copies directly in dist so the files are easy to find.
copy /y "%DLL_PATH%" "%DIST_DIR%\CompassNavigationOverhaul.dll" >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Could not copy the built DLL to the dist root."
    goto :failed
)

if defined PDB_PATH if exist "%PDB_PATH%" (
    copy /y "%PDB_PATH%" "%DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.pdb" >>"%LOG%" 2>&1
    if errorlevel 1 (
        call :fail "Could not copy the PDB to dist\SKSE\Plugins."
        goto :failed
    )
    copy /y "%PDB_PATH%" "%DIST_DIR%\CompassNavigationOverhaul.pdb" >>"%LOG%" 2>&1
)

set "PACKAGE_ROOT=%DIST_DIR%\package"
if exist "%PACKAGE_ROOT%" rmdir /s /q "%PACKAGE_ROOT%"
mkdir "%PACKAGE_ROOT%\SKSE\Plugins" >nul 2>&1
copy /y "%DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.dll" "%PACKAGE_ROOT%\SKSE\Plugins\" >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Could not stage the DLL for ZIP packaging."
    goto :failed
)

rem Ship our MCM Helper config next to SKSE. The original CNO SWFs/INI are not
rem duplicated: the original mod remains a requirement and this stays a patch.
if not exist "%ROOT%\MCM\Config\CompassNavigationOverhaul\config.json" (
    call :fail "MCM config.json is missing from the source tree."
    goto :failed
)
if not exist "%ROOT%\MCM\Config\CompassNavigationOverhaul\settings.ini" (
    call :fail "MCM settings.ini is missing from the source tree."
    goto :failed
)
mkdir "%PACKAGE_ROOT%\MCM\Config\CompassNavigationOverhaul" >nul 2>&1
copy /y "%ROOT%\MCM\Config\CompassNavigationOverhaul\config.json" "%PACKAGE_ROOT%\MCM\Config\CompassNavigationOverhaul\config.json" >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Could not stage MCM config.json for ZIP packaging."
    goto :failed
)
copy /y "%ROOT%\MCM\Config\CompassNavigationOverhaul\settings.ini" "%PACKAGE_ROOT%\MCM\Config\CompassNavigationOverhaul\settings.ini" >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "Could not stage MCM settings.ini for ZIP packaging."
    goto :failed
)

rem Ship localized MCM strings. Skyrim/MCM Helper selects the matching file from
rem Interface\Translations according to the current game language. Skyrim SE ships
rem nine official interface languages; require and package all nine here.
mkdir "%PACKAGE_ROOT%\Interface\Translations" >nul 2>&1
for %%L in (ENGLISH FRENCH ITALIAN GERMAN SPANISH POLISH RUSSIAN JAPANESE CHINESE) do (
    if not exist "%ROOT%\Interface\Translations\CompassNavigationOverhaul_%%L.txt" (
        call :fail "MCM translation file is missing: CompassNavigationOverhaul_%%L.txt"
        goto :failed
    )
    copy /y "%ROOT%\Interface\Translations\CompassNavigationOverhaul_%%L.txt" "%PACKAGE_ROOT%\Interface\Translations\" >>"%LOG%" 2>&1
    if errorlevel 1 (
        call :fail "Could not stage MCM translation file: CompassNavigationOverhaul_%%L.txt"
        goto :failed
    )
)

rem PDB stays in dist for debugging but is intentionally not put into the Vortex ZIP.
copy /y "%ROOT%\LICENSE" "%PACKAGE_ROOT%\LICENSE" >>"%LOG%" 2>&1
copy /y "%ROOT%\NOTICE.md" "%PACKAGE_ROOT%\NOTICE.md" >>"%LOG%" 2>&1
copy /y "%ROOT%\THIRD_PARTY_NOTICES.md" "%PACKAGE_ROOT%\THIRD_PARTY_NOTICES.md" >>"%LOG%" 2>&1
if exist "%ROOT%\licenses" xcopy /e /i /y "%ROOT%\licenses" "%PACKAGE_ROOT%\licenses" >>"%LOG%" 2>&1

set "ZIP_PATH=%DIST_DIR%\CompassNavigationOverhaul-2.2.0-MCM-AE-1.7.104.0-Update.zip"
if exist "%ZIP_PATH%" del /q "%ZIP_PATH%"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%PACKAGE_ROOT%\*' -DestinationPath '%ZIP_PATH%' -CompressionLevel Optimal -Force" >>"%LOG%" 2>&1
if errorlevel 1 (
    call :fail "DLL built successfully, but the Vortex patch ZIP packaging failed."
    goto :failed
)
rmdir /s /q "%PACKAGE_ROOT%" >nul 2>&1

call :say ""
call :say "BUILD SUCCESSFUL"
call :say "DLL: %DIST_DIR%\CompassNavigationOverhaul.dll"
call :say "Vortex path: %DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.dll"
if defined PDB_PATH call :say "PDB: %DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.pdb"
call :say "ZIP: %ZIP_PATH%"
call :say "Log: %LOG%"
echo.
echo Build successful.
echo DLL: "%DIST_DIR%\CompassNavigationOverhaul.dll"
echo Vortex path: "%DIST_DIR%\SKSE\Plugins\CompassNavigationOverhaul.dll"
echo ZIP: "%ZIP_PATH%"
echo Log: "%LOG%"
echo.
pause
exit /b 0

:vcpkg_failed
call :fail "Failed to create/update the pinned vcpkg toolchain."
goto :failed

:failed
call :say ""
call :say "BUILD FAILED"
call :say "See log: %LOG%"
echo.
echo BUILD FAILED.
echo Log: "%LOG%"
echo.
echo Last log lines:
powershell.exe -NoProfile -Command "if (Test-Path -LiteralPath '%LOG%') { Get-Content -LiteralPath '%LOG%' -Tail 45 }"
echo.
pause
exit /b 1

:find_git
set "GIT_EXE="
for /f "delims=" %%G in ('where git.exe 2^>nul') do if not defined GIT_EXE set "GIT_EXE=%%G"
if not defined GIT_EXE if exist "%ProgramFiles%\Git\cmd\git.exe" set "GIT_EXE=%ProgramFiles%\Git\cmd\git.exe"
if not defined GIT_EXE if exist "%ProgramFiles(x86)%\Git\cmd\git.exe" set "GIT_EXE=%ProgramFiles(x86)%\Git\cmd\git.exe"
exit /b 0

:find_vs
set "VS_PATH="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%V in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do if not defined VS_PATH set "VS_PATH=%%V"
)
if not defined VS_PATH if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
if not defined VS_PATH if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
if not defined VS_PATH if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
if not defined VS_PATH if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise"
exit /b 0

:find_cmake
set "CMAKE_EXE="
for /f "delims=" %%C in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%C"
if not defined CMAKE_EXE if defined VS_PATH if exist "%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"
exit /b 0

:find_ninja
set "NINJA_EXE="
for /f "delims=" %%N in ('where ninja.exe 2^>nul') do if not defined NINJA_EXE set "NINJA_EXE=%%N"
if not defined NINJA_EXE if defined VS_PATH if exist "%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" set "NINJA_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not defined NINJA_EXE if exist "%ProgramFiles%\Ninja\ninja.exe" set "NINJA_EXE=%ProgramFiles%\Ninja\ninja.exe"
exit /b 0

:say
set "MSG=%~1"
echo %MSG%
>>"%LOG%" echo [%date% %time%] %MSG%
exit /b 0

:fail
set "ERRMSG=%~1"
call :say "ERROR: %ERRMSG%"
exit /b 0
