[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$ZipPath
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$resolvedZip = (Resolve-Path -LiteralPath $ZipPath).Path
$temporaryZip = "$resolvedZip.codex.tmp"
$documentationRoot = 'Docs/Compass Navigation Overhaul'

$requiredFiles = @(
    'LICENSE',
    'NOTICE.md',
    'THIRD_PARTY_NOTICES.md'
)

foreach ($relativePath in $requiredFiles) {
    $sourcePath = Join-Path $repositoryRoot $relativePath
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Required legal file is missing: $sourcePath"
    }
}

$licenseDirectory = Join-Path $repositoryRoot 'licenses'
if (-not (Test-Path -LiteralPath $licenseDirectory -PathType Container)) {
    throw "License directory is missing: $licenseDirectory"
}

$licenseFiles = @(Get-ChildItem -LiteralPath $licenseDirectory -File | Sort-Object Name)
if ($licenseFiles.Count -eq 0) {
    throw "No dependency license files found in: $licenseDirectory"
}

if (Test-Path -LiteralPath $temporaryZip) {
    Remove-Item -LiteralPath $temporaryZip -Force
}

Copy-Item -LiteralPath $resolvedZip -Destination $temporaryZip

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Add-Or-ReplaceZipEntry {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.Compression.ZipArchive]$Archive,

        [Parameter(Mandatory = $true)]
        [string]$SourcePath,

        [Parameter(Mandatory = $true)]
        [string]$EntryName
    )

    $normalizedEntryName = $EntryName.Replace('\', '/')
    $existingEntry = $Archive.GetEntry($normalizedEntryName)
    if ($null -ne $existingEntry) {
        $existingEntry.Delete()
    }

    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $Archive,
        $SourcePath,
        $normalizedEntryName,
        [System.IO.Compression.CompressionLevel]::Optimal
    ) | Out-Null
}

try {
    $archive = [System.IO.Compression.ZipFile]::Open(
        $temporaryZip,
        [System.IO.Compression.ZipArchiveMode]::Update
    )

    try {
        foreach ($relativePath in $requiredFiles) {
            Add-Or-ReplaceZipEntry `
                -Archive $archive `
                -SourcePath (Join-Path $repositoryRoot $relativePath) `
                -EntryName "$documentationRoot/$relativePath"
        }

        foreach ($licenseFile in $licenseFiles) {
            Add-Or-ReplaceZipEntry `
                -Archive $archive `
                -SourcePath $licenseFile.FullName `
                -EntryName "$documentationRoot/licenses/$($licenseFile.Name)"
        }
    }
    finally {
        $archive.Dispose()
    }

    $validationArchive = [System.IO.Compression.ZipFile]::OpenRead($temporaryZip)
    try {
        $entryNames = @($validationArchive.Entries | ForEach-Object FullName)
        $expectedEntries = @(
            'SKSE/Plugins/CompassNavigationOverhaul.dll',
            "$documentationRoot/LICENSE",
            "$documentationRoot/NOTICE.md",
            "$documentationRoot/THIRD_PARTY_NOTICES.md"
        )

        foreach ($expectedEntry in $expectedEntries) {
            if ($entryNames -notcontains $expectedEntry) {
                throw "ZIP validation failed; entry is missing: $expectedEntry"
            }
        }

        foreach ($licenseFile in $licenseFiles) {
            $expectedLicenseEntry = "$documentationRoot/licenses/$($licenseFile.Name)"
            if ($entryNames -notcontains $expectedLicenseEntry) {
                throw "ZIP validation failed; dependency license is missing: $expectedLicenseEntry"
            }
        }
    }
    finally {
        $validationArchive.Dispose()
    }

    Move-Item -LiteralPath $temporaryZip -Destination $resolvedZip -Force
    Write-Output "Legal files added and ZIP validated: $resolvedZip"
}
catch {
    if (Test-Path -LiteralPath $temporaryZip) {
        Remove-Item -LiteralPath $temporaryZip -Force
    }
    throw
}
