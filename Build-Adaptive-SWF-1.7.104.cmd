@echo off
setlocal
cd /d "%~dp0"
set "CNO_PS1=%~dp0Build-Adaptive-SWF-1.7.104.ps1"

rem Parse the PowerShell file first. This catches quoting/syntax regressions before execution.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -Command "$tokens=$null;$errors=$null;[System.Management.Automation.Language.Parser]::ParseFile($env:CNO_PS1,[ref]$tokens,[ref]$errors) | Out-Null;if($errors.Count -gt 0){for($i=0;$i -lt $errors.Count;$i++){$e=$errors[$i];$x=$e.Extent;Write-Host ('PowerShell parser: {0}:{1}:{2}: {3}' -f $env:CNO_PS1,$x.StartLineNumber,$x.StartColumnNumber,$e.Message) -ForegroundColor Red};exit 1}"
if errorlevel 1 goto :parsefailed

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%CNO_PS1%" %*
set "ERR=%ERRORLEVEL%"
echo.
if not "%ERR%"=="0" (
  echo Adaptive SWF build failed with exit code %ERR%.
) else (
  echo Adaptive SWF build completed successfully.
)
pause
exit /b %ERR%

:parsefailed
echo.
echo Adaptive SWF build stopped because Build-Adaptive-SWF-1.7.104.ps1 contains a PowerShell parser error.
pause
exit /b 1
