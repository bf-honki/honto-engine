param(
    [string]$BuildDir = "",
    [string]$OutputDir = "",
    [string]$Generator = "Visual Studio 18 2026",
    [string]$InstallerName = "HonToEngine-SDK-Setup.exe"
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

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Path
if ([string]::IsNullOrWhiteSpace($cmake))
{
    $fallback = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (Test-Path $fallback)
    {
        $cmake = $fallback
    }
}

if ([string]::IsNullOrWhiteSpace($cmake))
{
    throw "cmake was not found."
}

$cpack = (Get-Command cpack -ErrorAction SilentlyContinue).Path
if ([string]::IsNullOrWhiteSpace($cpack))
{
    $cpackFallback = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cpack.exe"
    if (Test-Path $cpackFallback)
    {
        $cpack = $cpackFallback
    }
}

if ([string]::IsNullOrWhiteSpace($cpack))
{
    throw "cpack was not found."
}

$workingRoot = Join-Path $OutputDir "installer-work"
$packageRoot = Join-Path $workingRoot "package"
$payloadZip = Join-Path $packageRoot "honto-sdk-payload.zip"
$installScript = Join-Path $packageRoot "Install-HonToSdk.ps1"
$installCmd = Join-Path $packageRoot "install.cmd"
$sedPath = Join-Path $workingRoot "honto-sdk-installer.sed"
$installerPath = Join-Path $OutputDir $InstallerName
$repoInstallerPath = Join-Path $repoRoot $InstallerName

if (Test-Path $workingRoot)
{
    Remove-Item -Path $workingRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null

& $cmake -S $repoRoot -B $BuildDir -G $Generator -A x64
if ($LASTEXITCODE -ne 0)
{
    exit $LASTEXITCODE
}

& $cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0)
{
    exit $LASTEXITCODE
}

Push-Location $BuildDir
try
{
    & $cpack -G ZIP -C Release
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}
finally
{
    Pop-Location
}

$generatedPayload = Get-ChildItem -Path $BuildDir -Filter "HonToEngine-SDK-*-win64.zip" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($null -eq $generatedPayload)
{
    throw "Could not find generated SDK zip in $BuildDir"
}

Copy-Item -Path $generatedPayload.FullName -Destination $payloadZip -Force
Copy-Item -Path (Join-Path $scriptRoot "Install-HonToSdk.ps1") -Destination $installScript -Force

@'
@echo off
powershell -ExecutionPolicy Bypass -File "%~dp0Install-HonToSdk.ps1" -PayloadZip "%~dp0honto-sdk-payload.zip"
exit /b %errorlevel%
'@ | Set-Content -Path $installCmd -Encoding ASCII

$installerEscaped = $installerPath.Replace("\", "\\")
$packageEscaped = $packageRoot.Replace("\", "\\")

$sed = @"
[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=1
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=
DisplayLicense=
FinishMessage=HonTo SDK installer has finished.
TargetName=$installerEscaped
FriendlyName=HonTo Engine SDK Setup
AppLaunched=install.cmd
PostInstallCmd=<None>
AdminQuietInstCmd=install.cmd
UserQuietInstCmd=install.cmd
SourceFiles=SourceFiles
[Strings]
FILE0=Install-HonToSdk.ps1
FILE1=install.cmd
FILE2=honto-sdk-payload.zip
[SourceFiles]
SourceFiles0=$packageEscaped
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
"@

Set-Content -Path $sedPath -Value $sed -Encoding ASCII

& iexpress /N /Q $sedPath
if ($LASTEXITCODE -ne 0)
{
    exit $LASTEXITCODE
}

Copy-Item -Path $installerPath -Destination $repoInstallerPath -Force

Write-Output "Created HonTo SDK installer: $installerPath"
Write-Output "Copied HonTo SDK installer to repo root: $repoInstallerPath"
