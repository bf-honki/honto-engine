param(
    [string]$InstallRoot = "",
    [string]$BuildDir = "",
    [string]$Generator = "Visual Studio 18 2026"
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir))
{
    $BuildDir = Join-Path $repoRoot "build\honto-sdk"
}

if ([string]::IsNullOrWhiteSpace($InstallRoot))
{
    $InstallRoot = Join-Path $repoRoot "dist\honto-sdk"
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

& $cmake --install $BuildDir --config Release --prefix $InstallRoot
if ($LASTEXITCODE -ne 0)
{
    exit $LASTEXITCODE
}

Write-Output "Installed HonTo SDK to $InstallRoot"
