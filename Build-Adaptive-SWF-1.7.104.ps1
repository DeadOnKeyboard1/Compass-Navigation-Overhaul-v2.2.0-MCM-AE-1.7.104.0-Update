param(
    [string]$SkyrimData = "",
    [string]$CompassSwf = "",
    [string]$QuestItemListSwf = "",
    [string]$FfdecJar = "",
    [string]$JavaExe = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$FfdecVersion = "26.3.0"
$FfdecSha256 = "35f4930eb7c380afe66f2117f90b006deac0631473ad7500bb39c78f68645ecd"
$FfdecUrl = "https://github.com/jindrapetrik/jpexs-decompiler/releases/download/version$FfdecVersion/ffdec_$FfdecVersion.zip"

$CompassRelative = "Interface\InfinityUI\HUDMenu\HUDMovieBaseInstance\CompassShoutMeterHolder\Compass.swf"
$QuestRelative = "Interface\InfinityUI\HUDMenu\HUDMovieBaseInstance\QuestItemList.swf"

$logDir = Join-Path $Root "build-logs"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$logFile = Join-Path $logDir ("CNO-swf-" + $stamp + ".log")
$transcriptStarted = $false

function Write-Step([string]$Text) {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host " $Text" -ForegroundColor Cyan
    Write-Host "============================================================" -ForegroundColor Cyan
}

function Write-Utf8NoBom([string]$Path, [string]$Text) {
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Read-SourceText([string]$Path) {
    # File.ReadAllText handles UTF BOMs consistently on Windows PowerShell 5.1.
    # Strip a possible U+FEFF explicitly so a BOM can never end up in the middle
    # of a generated AS2 timeline script.
    $text = [System.IO.File]::ReadAllText($Path)
    if ($text.Length -gt 0 -and $text[0] -eq [char]0xFEFF) {
        $text = $text.Substring(1)
    }
    return $text
}

function Download-File([string]$Uri, [string]$Destination) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
    if (Test-Path -LiteralPath $Destination -PathType Leaf) {
        return
    }
    Write-Host "Downloading: $Uri"
    $oldProgress = $ProgressPreference
    try {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile $Destination
    }
    finally {
        $ProgressPreference = $oldProgress
    }
}

function Assert-Sha256([string]$Path, [string]$Expected) {
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Expected.ToLowerInvariant()) {
        throw "SHA256 mismatch for '$Path'. Expected $Expected, got $actual."
    }
}

function Find-Java([string]$Requested) {
    if ($Requested) {
        if (-not (Test-Path -LiteralPath $Requested -PathType Leaf)) {
            throw "Java executable not found: $Requested"
        }
        return (Resolve-Path -LiteralPath $Requested).Path
    }

    if ($env:JAVA_HOME) {
        $candidate = Join-Path $env:JAVA_HOME "bin\java.exe"
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }

    $cmd = Get-Command java.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # Portable fallback: Temurin JRE 21 from Adoptium. The API returns a ZIP for Windows x64.
    $toolchainRoot = Join-Path (Split-Path -Parent $Root) "_toolchain"
    $javaRoot = Join-Path $toolchainRoot "temurin-jre-21"
    $existing = Get-ChildItem -LiteralPath $javaRoot -Filter java.exe -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($existing) { return $existing.FullName }

    Write-Host "Java not found. Downloading portable Temurin JRE 21..." -ForegroundColor Yellow
    $downloads = Join-Path $toolchainRoot "downloads"
    New-Item -ItemType Directory -Force -Path $downloads | Out-Null
    $zip = Join-Path $downloads "temurin-jre-21-windows-x64.zip"
    Download-File "https://api.adoptium.net/v3/binary/latest/21/ga/windows/x64/jre/hotspot/normal/eclipse" $zip
    if (Test-Path -LiteralPath $javaRoot) { Remove-Item -LiteralPath $javaRoot -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $javaRoot | Out-Null
    Expand-Archive -LiteralPath $zip -DestinationPath $javaRoot -Force
    $existing = Get-ChildItem -LiteralPath $javaRoot -Filter java.exe -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $existing) { throw "Temurin JRE was extracted, but java.exe was not found." }
    return $existing.FullName
}

function Find-Ffdec([string]$Requested) {
    if ($Requested) {
        if (-not (Test-Path -LiteralPath $Requested -PathType Leaf)) {
            throw "FFDec jar not found: $Requested"
        }
        return (Resolve-Path -LiteralPath $Requested).Path
    }

    if ($env:FFDEC) {
        $envCandidate = $env:FFDEC.Trim('"')
        if (Test-Path -LiteralPath $envCandidate -PathType Leaf) {
            if ([IO.Path]::GetExtension($envCandidate) -ieq ".jar") {
                return (Resolve-Path -LiteralPath $envCandidate).Path
            }
        }
    }

    $toolchainRoot = Join-Path (Split-Path -Parent $Root) "_toolchain"
    $searchRoots = @(
        (Join-Path $Root "tools\ffdec"),
        (Join-Path $toolchainRoot "ffdec-$FfdecVersion"),
        (Join-Path $toolchainRoot "ffdec")
    )
    foreach ($searchRoot in $searchRoots) {
        if (-not (Test-Path -LiteralPath $searchRoot)) { continue }
        $found = Get-ChildItem -LiteralPath $searchRoot -Filter ffdec.jar -File -Recurse -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($found) { return $found.FullName }
    }

    Write-Host "FFDec not found. Downloading pinned FFDec $FfdecVersion..." -ForegroundColor Yellow
    $targetRoot = Join-Path $toolchainRoot "ffdec-$FfdecVersion"
    $downloads = Join-Path $toolchainRoot "downloads"
    $zip = Join-Path $downloads "ffdec_$FfdecVersion.zip"
    Download-File $FfdecUrl $zip
    Assert-Sha256 $zip $FfdecSha256
    if (Test-Path -LiteralPath $targetRoot) { Remove-Item -LiteralPath $targetRoot -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $targetRoot | Out-Null
    Expand-Archive -LiteralPath $zip -DestinationPath $targetRoot -Force
    $found = Get-ChildItem -LiteralPath $targetRoot -Filter ffdec.jar -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $found) { throw "FFDec $FfdecVersion was extracted, but ffdec.jar was not found." }
    return $found.FullName
}

function Add-UniquePath([System.Collections.Generic.List[string]]$List, [string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    try {
        $full = [IO.Path]::GetFullPath($Path)
    }
    catch { return }
    if (-not $List.Contains($full)) { $List.Add($full) }
}

function Get-SteamLibraryRoots {
    $roots = New-Object 'System.Collections.Generic.List[string]'

    $steamInstallCandidates = @()
    try { $steamInstallCandidates += (Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath } catch {}
    try { $steamInstallCandidates += (Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam' -ErrorAction SilentlyContinue).InstallPath } catch {}
    try { $steamInstallCandidates += (Get-ItemProperty 'HKLM:\SOFTWARE\Valve\Steam' -ErrorAction SilentlyContinue).InstallPath } catch {}
    $steamInstallCandidates += @(
        (Join-Path ${env:ProgramFiles(x86)} "Steam"),
        (Join-Path $env:ProgramFiles "Steam")
    )

    foreach ($steamRoot in $steamInstallCandidates) {
        if (-not $steamRoot) { continue }
        Add-UniquePath $roots $steamRoot
        $vdf = Join-Path $steamRoot "steamapps\libraryfolders.vdf"
        if (Test-Path -LiteralPath $vdf -PathType Leaf) {
            $raw = Get-Content -LiteralPath $vdf -Raw -ErrorAction SilentlyContinue
            if ($raw) {
                foreach ($match in [regex]::Matches($raw, '"path"\s+"([^"]+)"')) {
                    $library = $match.Groups[1].Value -replace '\\\\','\'
                    Add-UniquePath $roots $library
                }
            }
        }
    }

    foreach ($drive in Get-PSDrive -PSProvider FileSystem -ErrorAction SilentlyContinue) {
        if (-not $drive.Root) { continue }
        Add-UniquePath $roots (Join-Path $drive.Root "SteamLibrary")
        Add-UniquePath $roots (Join-Path $drive.Root "Steam")
    }

    return $roots
}

function Find-SkyrimData([string]$Requested) {
    if ($Requested) {
        $candidate = [IO.Path]::GetFullPath($Requested.Trim('"'))
        if ((Split-Path -Leaf $candidate) -ine "Data") {
            $dataCandidate = Join-Path $candidate "Data"
            if (Test-Path -LiteralPath $dataCandidate -PathType Container) { $candidate = $dataCandidate }
        }
        if (-not (Test-Path -LiteralPath $candidate -PathType Container)) {
            throw "Skyrim Data directory not found: $candidate"
        }
        return $candidate
    }

    if ($env:CNO_SKYRIM_DATA) {
        $candidate = $env:CNO_SKYRIM_DATA.Trim('"')
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    $candidates = New-Object 'System.Collections.Generic.List[string]'
    foreach ($library in Get-SteamLibraryRoots) {
        Add-UniquePath $candidates (Join-Path $library "steamapps\common\Skyrim Special Edition\Data")
    }

    foreach ($candidate in $candidates) {
        if ((Test-Path -LiteralPath $candidate -PathType Container) -and
            ((Test-Path -LiteralPath (Join-Path $candidate $CompassRelative) -PathType Leaf) -or
             (Test-Path -LiteralPath (Join-Path $candidate $QuestRelative) -PathType Leaf))) {
            return $candidate
        }
    }

    return $null
}

function Find-CnoSwfPair([string]$DataRoot, [string]$RequestedCompass, [string]$RequestedQuest) {
    $result = [ordered]@{ Compass = $null; Quest = $null; Source = $null }

    if ($RequestedCompass) {
        if (-not (Test-Path -LiteralPath $RequestedCompass -PathType Leaf)) { throw "Compass SWF not found: $RequestedCompass" }
        $result.Compass = (Resolve-Path -LiteralPath $RequestedCompass).Path
    }
    if ($RequestedQuest) {
        if (-not (Test-Path -LiteralPath $RequestedQuest -PathType Leaf)) { throw "QuestItemList SWF not found: $RequestedQuest" }
        $result.Quest = (Resolve-Path -LiteralPath $RequestedQuest).Path
    }
    if ($result.Compass -and $result.Quest) {
        $result.Source = "explicit arguments"
        return $result
    }

    if ($DataRoot) {
        $c = Join-Path $DataRoot $CompassRelative
        $q = Join-Path $DataRoot $QuestRelative
        if (-not $result.Compass -and (Test-Path -LiteralPath $c -PathType Leaf)) { $result.Compass = $c }
        if (-not $result.Quest -and (Test-Path -LiteralPath $q -PathType Leaf)) { $result.Quest = $q }
        if ($result.Compass -and $result.Quest) {
            $result.Source = "deployed Skyrim Data"
            return $result
        }
    }

    # Vortex default staging path fallback. Custom Vortex staging locations can still
    # be supplied explicitly via -CompassSwf / -QuestItemListSwf.
    if ($env:APPDATA) {
        $vortexMods = Join-Path $env:APPDATA "Vortex\skyrimse\mods"
        if (Test-Path -LiteralPath $vortexMods -PathType Container) {
            $mods = Get-ChildItem -LiteralPath $vortexMods -Directory -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -like "Compass Navigation Overhaul*" } |
                Sort-Object LastWriteTime -Descending
            foreach ($mod in $mods) {
                $c = Join-Path $mod.FullName $CompassRelative
                $q = Join-Path $mod.FullName $QuestRelative
                if ((Test-Path -LiteralPath $c -PathType Leaf) -and (Test-Path -LiteralPath $q -PathType Leaf)) {
                    $result.Compass = $c
                    $result.Quest = $q
                    $result.Source = "Vortex staging"
                    return $result
                }
            }
        }
    }

    return $result
}


function ConvertTo-WindowsCommandLineArgument([string]$Value) {
    if ($null -eq $Value -or $Value.Length -eq 0) {
        return '""'
    }

    $needsQuotes = ($Value.IndexOf(' ') -ge 0) -or ($Value.IndexOf("`t") -ge 0) -or ($Value.IndexOf('"') -ge 0)
    if (-not $needsQuotes) {
        return $Value
    }

    # Quote exactly as CommandLineToArgvW expects. This matters for paths with
    # spaces and for trailing backslashes before a closing quote.
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append([char]34)
    $backslashes = 0
    foreach ($ch in $Value.ToCharArray()) {
        if ([int]$ch -eq 92) {
            $backslashes++
            continue
        }

        if ([int]$ch -eq 34) {
            if ($backslashes -gt 0) {
                [void]$sb.Append([char]92, $backslashes * 2)
            }
            [void]$sb.Append([char]92)
            [void]$sb.Append([char]34)
            $backslashes = 0
            continue
        }

        if ($backslashes -gt 0) {
            [void]$sb.Append([char]92, $backslashes)
            $backslashes = 0
        }
        [void]$sb.Append($ch)
    }

    if ($backslashes -gt 0) {
        [void]$sb.Append([char]92, $backslashes * 2)
    }
    [void]$sb.Append([char]34)
    return $sb.ToString()
}

function Invoke-CapturedProcess([string]$FilePath, [string[]]$Arguments) {
    $quotedArguments = @($Arguments | ForEach-Object { ConvertTo-WindowsCommandLineArgument ([string]$_) })
    $argumentLine = $quotedArguments -join ' '

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $FilePath
    $startInfo.Arguments = $argumentLine
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) {
            throw "Could not start process: $FilePath"
        }

        # Read both pipes asynchronously. Reading only stdout first can deadlock
        # when Java writes enough data to stderr.
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = $stdoutTask.Result
        $stderr = $stderrTask.Result
        $exitCode = $process.ExitCode

        return [PSCustomObject]@{
            ExitCode = $exitCode
            StdOut = $stdout
            StdErr = $stderr
            CommandLine = ('"' + $FilePath + '" ' + $argumentLine)
        }
    }
    finally {
        $process.Dispose()
    }
}

function Write-CapturedProcessOutput($Result) {
    if ($Result.StdOut) {
        foreach ($line in [regex]::Split([string]$Result.StdOut, "\r?\n")) {
            if ($line.Length -gt 0) { Write-Host $line }
        }
    }
    if ($Result.StdErr) {
        foreach ($line in [regex]::Split([string]$Result.StdErr, "\r?\n")) {
            if ($line.Length -gt 0) { Write-Host $line -ForegroundColor DarkYellow }
        }
    }
}

function Get-CapturedProcessTail($Result, [int]$LineCount = 30) {
    $combined = @()
    if ($Result.StdOut) { $combined += [regex]::Split([string]$Result.StdOut, "\r?\n") }
    if ($Result.StdErr) { $combined += [regex]::Split([string]$Result.StdErr, "\r?\n") }
    return (@($combined | Where-Object { $_ -and $_.Trim().Length -gt 0 } | Select-Object -Last $LineCount) -join "`n")
}

function Invoke-FfdecExport([string]$Java, [string]$Jar, [string]$Swf, [string]$OutputDir) {
    if (Test-Path -LiteralPath $OutputDir) { Remove-Item -LiteralPath $OutputDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

    $result = Invoke-CapturedProcess $Java @('-jar', $Jar, '-export', 'script', $OutputDir, $Swf)
    Write-CapturedProcessOutput $result
    if ($result.ExitCode -ne 0) {
        $tail = Get-CapturedProcessTail $result 30
        throw "FFDec script export failed for $Swf (exit $($result.ExitCode)).`nCommand: $($result.CommandLine)`nFFDec tail:`n$tail"
    }
}

function Get-AsFiles([string]$RootDir) {
    return @(Get-ChildItem -LiteralPath $RootDir -Filter *.as -File -Recurse -ErrorAction SilentlyContinue)
}

function Find-ScriptByMarkers([string]$RootDir, [string[]]$Markers, [string]$Label) {
    # Never use a local variable named $matches here. PowerShell's -match operator
    # writes to the automatic $Matches hashtable and variable names are case-insensitive.
    $candidateFiles = @()
    foreach ($file in Get-AsFiles $RootDir) {
        $text = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction SilentlyContinue
        if ($null -eq $text) { continue }
        $ok = $true
        foreach ($marker in $Markers) {
            if ($text.IndexOf($marker, [StringComparison]::OrdinalIgnoreCase) -lt 0) { $ok = $false; break }
        }
        if ($ok) { $candidateFiles += $file }
    }
    if ($candidateFiles.Count -ne 1) {
        $paths = if ($candidateFiles.Count) { ($candidateFiles.FullName -join '; ') } else { '<none>' }
        throw "Could not uniquely identify $Label in exported SWF scripts. Matches=$($candidateFiles.Count): $paths"
    }
    return $candidateFiles[0].FullName
}

function Find-RootTimelineScript([string]$RootDir, [string[]]$FallbackMarkers, [string]$Label) {
    # FFDec exports the root movie's frame-1 action block at this stable path for
    # the shipped CNO 2.2.0 SWFs. Prefer structure over source-text fingerprints.
    $preferred = Join-Path $RootDir "scripts\_frame_1\DoAction.as"
    if (Test-Path -LiteralPath $preferred -PathType Leaf) {
        Write-Host "$Label timeline: $preferred"
        return (Resolve-Path -LiteralPath $preferred).Path
    }

    $frameOne = @(Get-AsFiles $RootDir | Where-Object {
        $_.Name -ieq "DoAction.as" -and [regex]::IsMatch($_.DirectoryName, '[\\/]_frame_1$')
    })
    if ($frameOne.Count -eq 1) {
        Write-Host "$Label timeline (structural fallback): $($frameOne[0].FullName)"
        return $frameOne[0].FullName
    }

    return Find-ScriptByMarkers $RootDir $FallbackMarkers $Label
}

function Find-ClassScriptByMethod([string]$RootDir, [string]$MethodMarker, [string]$Label, [string[]]$FallbackNames = @()) {
    # IMPORTANT: Never call this collection $matches. The PowerShell -match operator
    # populates the automatic $Matches hashtable (case-insensitive variable names),
    # which would turn an array into a hashtable mid-loop on Windows PowerShell 5.1.
    $candidateFiles = @()
    $classPattern = '(?im)^\s*class\s+[A-Za-z_][A-Za-z0-9_]*'
    $packagePattern = '[\\/]__Packages[\\/]'

    foreach ($file in Get-AsFiles $RootDir) {
        $text = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction SilentlyContinue
        if ($null -eq $text) { continue }
        if ($text.IndexOf($MethodMarker, [StringComparison]::OrdinalIgnoreCase) -ge 0 -and
            [regex]::IsMatch($text, $classPattern)) {
            $candidateFiles += $file
        }
    }

    if ($candidateFiles.Count -gt 1) {
        # FFDec can export both a symbol action and the real package class. Prefer
        # __Packages because that is the class definition compiled by the SWF.
        $packageCandidates = @($candidateFiles | Where-Object { [regex]::IsMatch($_.FullName, $packagePattern) })
        if ($packageCandidates.Count -eq 1) { $candidateFiles = $packageCandidates }
    }

    if ($candidateFiles.Count -eq 0 -and $FallbackNames.Count -gt 0) {
        foreach ($fallbackName in $FallbackNames) {
            $named = @(Get-AsFiles $RootDir | Where-Object { $_.Name -ieq $fallbackName })
            if ($named.Count -gt 1) {
                $packageNamed = @($named | Where-Object { [regex]::IsMatch($_.FullName, $packagePattern) })
                if ($packageNamed.Count -eq 1) { $named = $packageNamed }
            }
            if ($named.Count -eq 1) {
                Write-Host "$Label fallback by filename '$fallbackName': $($named[0].FullName)" -ForegroundColor Yellow
                $candidateFiles = $named
                break
            }
        }
    }

    if ($candidateFiles.Count -ne 1) {
        $paths = if ($candidateFiles.Count) { ($candidateFiles.FullName -join '; ') } else { '<none>' }
        throw "Could not uniquely identify $Label by method '$MethodMarker'. Matches=$($candidateFiles.Count): $paths"
    }

    Write-Host "${Label}: $($candidateFiles[0].FullName)"
    return $candidateFiles[0].FullName
}

function Get-As2ClassName([string]$Path) {
    $text = Get-Content -LiteralPath $Path -Raw -ErrorAction Stop
    $m = [regex]::Match($text, '(?im)^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)')
    if (-not $m.Success) { throw "Could not determine AS2 class name from: $Path" }
    return $m.Groups[1].Value
}

function Patch-CompassMarkerClassInPlace([string]$Path) {
    $text = Read-SourceText $Path
    if ($text.IndexOf('SetDistanceAndHeightDifference', [StringComparison]::OrdinalIgnoreCase) -lt 0) {
        throw "Focused-marker class does not contain SetDistanceAndHeightDifference: $Path"
    }

    if ($text.IndexOf('var markerAlpha', [StringComparison]::OrdinalIgnoreCase) -lt 0) {
        # FFDec does not preserve authored comments, so anchor on the actual AS2
        # method declaration and insert the guard immediately after its opening brace.
        $methodPattern = '(?is)function\s+SetDistanceAndHeightDifference\s*\([^)]*\)\s*(?::\s*[A-Za-z_][A-Za-z0-9_]*)?\s*\{'
        $m = [regex]::Match($text, $methodPattern)
        if (-not $m.Success) {
            throw "Could not locate SetDistanceAndHeightDifference method body in focused-marker class: $Path"
        }
        $guard = "`r`n      var markerAlpha:Number = (Movie != undefined && Movie._alpha != undefined) ? Movie._alpha : 100;"
        $text = $text.Insert($m.Index + $m.Length, $guard)
    }

    # JPEXS may emit either Movie._alpha or this.Movie._alpha and may normalize
    # whitespace, so replace both forms structurally rather than by exact text.
    $alphaPattern = 'Math\.max\(\s*(?:this\.)?Movie\._alpha\s*,\s*75\s*\)'
    $text = [regex]::Replace($text, $alphaPattern, 'Math.max(markerAlpha,75)')

    if ($text.IndexOf('Math.max(markerAlpha,75)', [StringComparison]::OrdinalIgnoreCase) -lt 0 -and
        $text.IndexOf('Math.max(markerAlpha, 75)', [StringComparison]::OrdinalIgnoreCase) -lt 0) {
        throw "Focused-marker class patch did not find/replace the Movie._alpha Math.max calls: $Path"
    }

    Write-Utf8NoBom $Path $text
}

function Get-As2FunctionSpan([string]$Text, [string]$FunctionName) {
    $pattern = '(?im)\bfunction\s+' + [regex]::Escape($FunctionName) + '\s*\('
    $m = [regex]::Match($Text, $pattern)
    if (-not $m.Success) {
        throw "Could not locate AS2 function '$FunctionName'."
    }

    $openBrace = $Text.IndexOf('{', $m.Index + $m.Length)
    if ($openBrace -lt 0) {
        throw "Could not locate opening brace for AS2 function '$FunctionName'."
    }

    $depth = 0
    $closeBrace = -1
    for ($i = $openBrace; $i -lt $Text.Length; $i++) {
        $ch = $Text[$i]
        if ($ch -eq '{') {
            $depth++
        }
        elseif ($ch -eq '}') {
            $depth--
            if ($depth -eq 0) {
                $closeBrace = $i
                break
            }
        }
    }

    if ($closeBrace -lt 0) {
        throw "Could not locate closing brace for AS2 function '$FunctionName'."
    }

    return [PSCustomObject]@{
        Start = $m.Index
        Length = ($closeBrace - $m.Index + 1)
        Text = $Text.Substring($m.Index, ($closeBrace - $m.Index + 1))
    }
}

function Convert-As2FunctionForTimeline([string]$FunctionText, [string]$FunctionName) {
    # FFDec's AS2 timeline parser accepts typed parameters but does not accept a
    # return-type annotation after the closing parenthesis. Flash/Animate source
    # commonly uses `function Foo(...):Void`; timeline DoAction source imported
    # by FFDec must instead be `function Foo(...) { ... }`.
    $pattern = '(?s)\A(\s*function\s+' + [regex]::Escape($FunctionName) + '\s*\([^)]*\))\s*:\s*[A-Za-z_$][A-Za-z0-9_$.]*'
    $m = [regex]::Match($FunctionText, $pattern)
    if ($m.Success) {
        return $m.Groups[1].Value + $FunctionText.Substring($m.Index + $m.Length)
    }
    return $FunctionText
}

function Replace-As2Function([string]$TargetText, [string]$SourceText, [string]$FunctionName) {
    $target = Get-As2FunctionSpan $TargetText $FunctionName
    $source = Get-As2FunctionSpan $SourceText $FunctionName
    $replacement = Convert-As2FunctionForTimeline $source.Text $FunctionName
    return $TargetText.Remove($target.Start, $target.Length).Insert($target.Start, $replacement)
}

function Assert-FfdecTimelineFunctionHeaders([string]$Path) {
    $text = Read-SourceText $Path
    $pattern = '(?ms)^\s*function\s+[A-Za-z_$][A-Za-z0-9_$]*\s*\([^)]*\)\s*:\s*[A-Za-z_$][A-Za-z0-9_$.]*'
    $bad = [regex]::Matches($text, $pattern)
    if ($bad.Count -gt 0) {
        $sample = ($bad | Select-Object -First 3 | ForEach-Object { $_.Value.Trim() }) -join ' | '
        throw "FFDec timeline syntax preflight failed for $Path. Return-type annotations after ')' are not supported in timeline DoAction scripts. Found: $sample"
    }
}


function Upsert-As2Function([string]$TargetText, [string]$SourceText, [string]$FunctionName) {
    $existing = [regex]::Match($TargetText, '(?m)^\s*function\s+' + [regex]::Escape($FunctionName) + '\s*\(')
    if ($existing.Success) {
        return Replace-As2Function $TargetText $SourceText $FunctionName
    }

    $source = Get-As2FunctionSpan $SourceText $FunctionName
    $replacement = Convert-As2FunctionForTimeline $source.Text $FunctionName
    return $replacement.Trim() + "`r`n`r`n" + $TargetText
}

function Use-RuntimeHudResolver([string]$TargetText) {
    $pattern = '(?im)^\s*var\s+HUDMenu(?:\s*:\s*MovieClip)?\s*=\s*[^;]+;\s*$'
    if ([regex]::IsMatch($TargetText, $pattern)) {
        return [regex]::Replace($TargetText, $pattern, 'var HUDMenu:MovieClip = ResolveHUDMenu();', 1)
    }

    if ($TargetText.IndexOf('HUDMenu', [StringComparison]::OrdinalIgnoreCase) -ge 0) {
        return "var HUDMenu:MovieClip = ResolveHUDMenu();`r`n" + $TargetText
    }
    return $TargetText
}

function Patch-CompassScripts([string]$ExportDir) {
    $timeline = Find-RootTimelineScript $ExportDir @("CompassTemperatureHolderInstance", "SetMarkers") "Compass timeline script"
    $class = Find-ClassScriptByMethod $ExportDir "SetDistanceAndHeightDifference" "Compass focused-marker class" @("CompassMarkerInfo.as", "FocusedMarker.as")
    $className = Get-As2ClassName $class
    Write-Host "Compass focused-marker AS2 class: $className"

    # Preserve the selected HUD skin. Patch only CNO behaviour and runtime HUD lookup;
    # no graphics, timelines, symbols, positions or textures are replaced here.
    $targetText = Read-SourceText $timeline
    $sourceText = Read-SourceText (Join-Path $Root "swf\Compass.as")

    $targetText = Upsert-As2Function $targetText $sourceText "ResolveHUDMenu"
    $targetText = Use-RuntimeHudResolver $targetText

    foreach ($functionName in @("Compass", "SetFocusedMarkerInfo", "UpdateFocusedMarker", "SetMarkers")) {
        $targetText = Upsert-As2Function $targetText $sourceText $functionName
        Write-Host "Patched Compass function: $functionName"
    }

    Write-Utf8NoBom $timeline $targetText
    Assert-FfdecTimelineFunctionHeaders $timeline

    # Keep the exact class/layout from the selected base SWF. Only preserve the
    # previously verified null-safe focused-marker class correction.
    Patch-CompassMarkerClassInPlace $class

    return [PSCustomObject]@{
        Files = @($timeline, $class)
        Timeline = $timeline
        Class = $class
        ClassName = $className
    }
}

function Ensure-QuestTopLevelDeclarations([string]$Text) {
    $result = $Text

    if ($result.IndexOf('questItemSerial', [StringComparison]::OrdinalIgnoreCase) -lt 0) {
        $anchor = [regex]::Match($result, '(?im)^\s*var\s+entries\s*:\s*Array\s*;\s*$')
        if ($anchor.Success) {
            $insertAt = $anchor.Index + $anchor.Length
            $result = $result.Insert($insertAt, "`r`nvar questItemSerial:Number = 0;")
        }
        else {
            $result = "var questItemSerial:Number = 0;`r`n" + $result
        }
    }

    if (-not [regex]::IsMatch($result, '(?im)^\s*var\s+positionX0\s*(?::\s*Number)?\s*;')) {
        $anchor = [regex]::Match($result, '(?im)^\s*var\s+positionY0\s*:\s*Number\s*;\s*$')
        if ($anchor.Success) {
            $result = $result.Insert($anchor.Index, "var positionX0:Number;`r`n")
        }
        else {
            $result = "var positionX0:Number;`r`n" + $result
        }
    }

    return $result
}

function Patch-QuestScripts([string]$ExportDir) {
    $timeline = Find-RootTimelineScript $ExportDir @("QuestItemList", "AddToHudElements", "ByAgeThenMiscellaneousQuests") "QuestItemList timeline script"

    # Preserve the selected quest-list skin and add only runtime resolver helpers plus
    # the CNO behavioural functions that need to follow the active compass holder.
    $targetText = Read-SourceText $timeline
    $sourceText = Read-SourceText (Join-Path $Root "swf\QuestItemList.as")
    $targetText = Ensure-QuestTopLevelDeclarations $targetText
    $targetText = Upsert-As2Function $targetText $sourceText "ResolveHUDMenu"
    $targetText = Upsert-As2Function $targetText $sourceText "ResolveCompassHolder"

    foreach ($functionName in @("AddToHudElements", "AddQuest", "SetQuestSide", "Update", "ShowQuest", "RemoveQuest")) {
        $targetText = Upsert-As2Function $targetText $sourceText $functionName
        Write-Host "Patched QuestItemList function: $functionName"
    }

    Write-Utf8NoBom $timeline $targetText
    Assert-FfdecTimelineFunctionHeaders $timeline

    return [PSCustomObject]@{
        Files = @($timeline)
        Timeline = $timeline
    }
}

function New-MinimalImportTree([string]$ExportDir, [string[]]$ChangedFiles, [string]$Destination) {
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    $scriptsRoot = Join-Path $ExportDir "scripts"
    if (-not (Test-Path -LiteralPath $scriptsRoot -PathType Container)) {
        throw "FFDec export did not create a scripts directory: $scriptsRoot"
    }
    $scriptsRootFull = [IO.Path]::GetFullPath($scriptsRoot).TrimEnd('\', '/')

    foreach ($changedFile in $ChangedFiles) {
        $changedFull = [IO.Path]::GetFullPath($changedFile)
        if (-not $changedFull.StartsWith($scriptsRootFull + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Patched AS2 file is outside the FFDec scripts tree: $changedFull"
        }

        $relative = $changedFull.Substring($scriptsRootFull.Length).TrimStart('\', '/')
        $destFile = Join-Path $Destination $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destFile) | Out-Null
        Copy-Item -LiteralPath $changedFull -Destination $destFile -Force
        Write-Host "Import script: $relative"
    }

    return $Destination
}

function Invoke-FfdecImport([string]$Java, [string]$Jar, [string]$InputSwf, [string]$OutputSwf, [string]$ScriptsDir) {
    $outputParent = Split-Path -Parent $OutputSwf
    New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
    if (([IO.Path]::GetFullPath($InputSwf)) -ne ([IO.Path]::GetFullPath($OutputSwf))) {
        Copy-Item -LiteralPath $InputSwf -Destination $OutputSwf -Force
    }

    Write-Host "FFDec import root: $ScriptsDir"
    $result = Invoke-CapturedProcess $Java @('-jar', $Jar, '-onerror', 'abort', '-importScript', $OutputSwf, $OutputSwf, $ScriptsDir)
    Write-CapturedProcessOutput $result
    if ($result.ExitCode -ne 0) {
        $tail = Get-CapturedProcessTail $result 30
        throw "FFDec script import failed for $OutputSwf (exit $($result.ExitCode)).`nCommand: $($result.CommandLine)`nFFDec tail:`n$tail"
    }
}

function Assert-ExportMarkers([string]$ExportDir, [string[]]$Markers, [string]$Label) {
    $allText = ""
    foreach ($file in Get-AsFiles $ExportDir) {
        $text = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction SilentlyContinue
        if ($text) { $allText += "`n" + $text }
    }
    foreach ($marker in $Markers) {
        if ($allText.IndexOf($marker, [StringComparison]::OrdinalIgnoreCase) -lt 0) {
            throw "$Label verification failed after recompilation. Missing marker: $marker"
        }
    }
}

function Build-OneSwf(
    [string]$Label,
    [string]$BaseSwf,
    [string]$OutputSwf,
    [scriptblock]$PatchFunction,
    [string[]]$VerifyMarkers,
    [string]$Java,
    [string]$Jar,
    [string]$WorkRoot
) {
    Write-Step "Building $Label"
    if (-not (Test-Path -LiteralPath $BaseSwf -PathType Leaf)) { throw "$Label base SWF not found: $BaseSwf" }

    $beforeHash = (Get-FileHash -LiteralPath $BaseSwf -Algorithm SHA256).Hash
    Write-Host "Base: $BaseSwf"
    Write-Host "Base SHA256: $beforeHash"

    $exportDir = Join-Path $WorkRoot ($Label + "-export")
    Invoke-FfdecExport $Java $Jar $BaseSwf $exportDir
    $patchResult = & $PatchFunction $exportDir
    if ($null -eq $patchResult -or $null -eq $patchResult.Files -or @($patchResult.Files).Count -eq 0) {
        throw "$Label patch function did not report any changed AS2 files."
    }

    $importDir = Join-Path $WorkRoot ($Label + "-import")
    New-MinimalImportTree $exportDir @($patchResult.Files) $importDir | Out-Null
    Invoke-FfdecImport $Java $Jar $BaseSwf $OutputSwf $importDir
    if (-not (Test-Path -LiteralPath $OutputSwf -PathType Leaf)) { throw "$Label output SWF was not created." }

    $afterHash = (Get-FileHash -LiteralPath $OutputSwf -Algorithm SHA256).Hash
    Write-Host "Output: $OutputSwf"
    Write-Host "Output SHA256: $afterHash"
    if ($afterHash -eq $beforeHash) {
        throw "$Label output is byte-identical to the base SWF. FFDec did not apply the source changes."
    }

    $verifyDir = Join-Path $WorkRoot ($Label + "-verify")
    Invoke-FfdecExport $Java $Jar $OutputSwf $verifyDir
    Assert-ExportMarkers $verifyDir $VerifyMarkers $Label

    Write-Host "$Label recompile + round-trip verification: OK" -ForegroundColor Green
}

try {
    Start-Transcript -LiteralPath $logFile -Force | Out-Null
    $transcriptStarted = $true

    Write-Step "CNO v5 - Skyrim 1.7.104 - optional adaptive HUD/SWF build"
    Write-Host "This creates a LOCAL compatibility overlay from the currently installed CNO/UI SWFs."
    Write-Host "Graphics, timelines, exported symbols and Scaleform metadata remain from the selected base SWFs; only CNO ActionScript hooks are updated."

    $java = Find-Java $JavaExe
    $ffdec = Find-Ffdec $FfdecJar
    Write-Host "Java : $java"
    Write-Host "FFDec: $ffdec"

    $dataRoot = Find-SkyrimData $SkyrimData
    if ($dataRoot) { Write-Host "Skyrim Data: $dataRoot" }

    $pair = Find-CnoSwfPair $dataRoot $CompassSwf $QuestItemListSwf
    if (-not $pair.Compass -or -not $pair.Quest) {
        throw @"
Could not find both original CNO 2.2.0 runtime SWFs.
Expected deployed paths:
  Data\$CompassRelative
  Data\$QuestRelative

Deploy/install original Compass Navigation Overhaul 2.2.0 once, or pass:
  -CompassSwf <path> -QuestItemListSwf <path>
"@
    }
    Write-Host "Base SWF source: $($pair.Source)"

    $workRoot = Join-Path $Root "build\swf-$stamp"
    if (Test-Path -LiteralPath $workRoot) { Remove-Item -LiteralPath $workRoot -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $workRoot | Out-Null

    $packageRoot = Join-Path $Root "adaptive-output"
    $compassOutput = Join-Path $packageRoot $CompassRelative
    $questOutput = Join-Path $packageRoot $QuestRelative

    $compassVerify = @(
        "ResolveHUDMenu",
        "SetFocusedMarkerInfo",
        "UpdateFocusedMarker",
        "SetMarkers",
        "SetDistanceAndHeightDifference"
    )
    $questVerify = @(
        "ResolveHUDMenu",
        "ResolveCompassHolder",
        "__CNO_CompassHolder",
        "AddToHudElements",
        "entries.splice",
        "SetQuestInfo"
    )

    Build-OneSwf "Compass" $pair.Compass $compassOutput ${function:Patch-CompassScripts} $compassVerify $java $ffdec $workRoot
    Build-OneSwf "QuestItemList" $pair.Quest $questOutput ${function:Patch-QuestScripts} $questVerify $java $ffdec $workRoot

    Write-Step "Final SWF validation"
    foreach ($required in @($compassOutput, $questOutput)) {
        if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing built SWF: $required" }
        $size = (Get-Item -LiteralPath $required).Length
        if ($size -lt 1024) { throw "Built SWF is unexpectedly small ($size bytes): $required" }
        Write-Host ("{0}  {1:N0} bytes  SHA256 {2}" -f $required, $size, (Get-FileHash -LiteralPath $required -Algorithm SHA256).Hash)
    }

    Write-Host ""
    Write-Host "SWF BUILD SUCCESSFUL" -ForegroundColor Green
    Write-Host "The verified LOCAL compatibility SWFs are inside adaptive-output\Interface\InfinityUI. They are intentionally NOT included by the normal v5 release build, so a public package never bakes one user's HUD skin into the mod."
    Write-Host "Log: $logFile"
}
catch {
    Write-Host ""
    Write-Host "SWF BUILD FAILED" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    if ($_.InvocationInfo -and $_.InvocationInfo.PositionMessage) {
        Write-Host $_.InvocationInfo.PositionMessage -ForegroundColor DarkRed
    }
    if ($_.ScriptStackTrace) {
        Write-Host "PowerShell stack:" -ForegroundColor DarkYellow
        Write-Host $_.ScriptStackTrace -ForegroundColor DarkYellow
    }
    Write-Host "Log: $logFile" -ForegroundColor Yellow
    exit 1
}
finally {
    if ($transcriptStarted) {
        try { Stop-Transcript | Out-Null } catch {}
    }
}
