param(
    [string]$PayloadZip = "",
    [string]$InstallRoot = "",
    [switch]$Quiet
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if ([string]::IsNullOrWhiteSpace($PayloadZip))
{
    $PayloadZip = Join-Path $scriptRoot "honto-sdk-payload.zip"
}

if (-not (Test-Path $PayloadZip))
{
    throw "Could not find SDK payload zip: $PayloadZip"
}

if ([string]::IsNullOrWhiteSpace($InstallRoot))
{
    $InstallRoot = Join-Path $env:LOCALAPPDATA "HonToSDK"
}

if (-not (Test-Path $InstallRoot))
{
    New-Item -ItemType Directory -Path $InstallRoot -Force | Out-Null
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("honto-sdk-install-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null

try
{
    Expand-Archive -Path $PayloadZip -DestinationPath $tempRoot -Force

    $sourceRoot = $tempRoot
    $topEntries = Get-ChildItem -Path $tempRoot -Force
    if ($topEntries.Count -eq 1 -and $topEntries[0].PSIsContainer)
    {
        $sourceRoot = $topEntries[0].FullName
    }

    Get-ChildItem -Path $sourceRoot -Force | ForEach-Object {
        $target = Join-Path $InstallRoot $_.Name
        if ($_.PSIsContainer)
        {
            Copy-Item -Path $_.FullName -Destination $target -Recurse -Force
        }
        else
        {
            Copy-Item -Path $_.FullName -Destination $target -Force
        }
    }

    [System.Environment]::SetEnvironmentVariable("HONTO_SDK_ROOT", $InstallRoot, "User")

    $existingPath = [System.Environment]::GetEnvironmentVariable("PATH", "User")
    $binPath = Join-Path $InstallRoot "bin"
    if ([string]::IsNullOrWhiteSpace($existingPath))
    {
        [System.Environment]::SetEnvironmentVariable("PATH", $binPath, "User")
    }
    else
    {
        $segments = $existingPath.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries)
        if (-not ($segments | Where-Object { $_.TrimEnd('\') -ieq $binPath.TrimEnd('\') }))
        {
            [System.Environment]::SetEnvironmentVariable("PATH", ($existingPath.TrimEnd(';') + ";" + $binPath), "User")
        }
    }

    $installNotes = @"
HonTo SDK installed to:
$InstallRoot

HONTO_SDK_ROOT has been set for the current user.

Restart Visual Studio before opening a HonTo project.
"@

    if ($Quiet)
    {
        Write-Output $installNotes
    }
    else
    {
        Add-Type -AssemblyName PresentationFramework
        [System.Windows.MessageBox]::Show($installNotes, "HonTo SDK Installer") | Out-Null
    }
}
finally
{
    if (Test-Path $tempRoot)
    {
        Remove-Item -Path $tempRoot -Recurse -Force
    }
}
