param(
    [string]$BuildDir = "",
    [string]$OutputDir = "",
    [string]$Generator = "Visual Studio 18 2026"
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir))
{
    $BuildDir = Join-Path $repoRoot "build\honto-installer"
}

if ([string]::IsNullOrWhiteSpace($OutputDir))
{
    $OutputDir = Join-Path $repoRoot "dist"
}

if (-not (Test-Path $OutputDir))
{
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$cmakeText = Get-Content -Path (Join-Path $repoRoot "CMakeLists.txt") -Raw
$versionMatch = [regex]::Match($cmakeText, 'project\(HonToEngine VERSION ([0-9\.]+)')
if (-not $versionMatch.Success)
{
    throw "Could not determine HonTo version from CMakeLists.txt"
}

$version = $versionMatch.Groups[1].Value

& (Join-Path $scriptRoot "Build-HonToInstaller.ps1") -BuildDir $BuildDir -OutputDir $OutputDir -Generator $Generator -InstallerName "HonToEngine-SDK-Setup.exe"
if ($LASTEXITCODE -ne 0)
{
    exit $LASTEXITCODE
}

$portableSource = Get-ChildItem -Path $BuildDir -Filter "HonToEngine-SDK-*-win64.zip" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($null -eq $portableSource)
{
    throw "Could not find generated portable SDK zip."
}

$portableZip = Join-Path $OutputDir ("HonToEngine-SDK-Portable-" + $version + "-win64.zip")
Copy-Item -Path $portableSource.FullName -Destination $portableZip -Force

$releaseRoot = Join-Path $OutputDir "release-assets"
$setupRoot = Join-Path $releaseRoot "setup"
if (Test-Path $releaseRoot)
{
    Remove-Item -Path $releaseRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $setupRoot -Force | Out-Null

$setupExe = Join-Path $OutputDir "HonToEngine-SDK-Setup.exe"
$setupZip = Join-Path $OutputDir ("HonToEngine-SDK-Setup-" + $version + "-win64.zip")

Copy-Item -Path $setupExe -Destination (Join-Path $setupRoot "HonToEngine-SDK-Setup.exe") -Force

@"
HonTo Engine SDK Setup

1. Run HonToEngine-SDK-Setup.exe
2. Restart Visual Studio
3. Create a game with:
   %HONTO_SDK_ROOT%\bin\New-HonToProject.ps1
4. Or create a Visual Studio library project with:
   %HONTO_SDK_ROOT%\bin\New-HonToVsProject.ps1
"@ | Set-Content -Path (Join-Path $setupRoot "README.txt") -Encoding ASCII

if (Test-Path $setupZip)
{
    Remove-Item -Path $setupZip -Force
}

Compress-Archive -Path (Join-Path $setupRoot "*") -DestinationPath $setupZip -CompressionLevel Optimal

Write-Output "Portable SDK zip: $portableZip"
Write-Output "Setup zip: $setupZip"
