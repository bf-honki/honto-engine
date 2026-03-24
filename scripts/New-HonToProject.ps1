param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectName,
    [string]$DestinationPath = "."
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$templateCandidates = @(
    (Join-Path $scriptRoot "..\templates\HonToStarter"),
    (Join-Path $scriptRoot "..\share\HonToEngine\templates\HonToStarter")
)

$templateRoot = $null
foreach ($candidate in $templateCandidates)
{
    if (Test-Path $candidate)
    {
        $templateRoot = (Resolve-Path $candidate).Path
        break
    }
}

if ($null -eq $templateRoot)
{
    throw "Could not locate the HonTo starter template."
}

if (-not (Test-Path $DestinationPath))
{
    New-Item -ItemType Directory -Path $DestinationPath | Out-Null
}

$destinationRoot = (Resolve-Path $DestinationPath).Path
$projectRoot = Join-Path $destinationRoot $ProjectName

if (Test-Path $projectRoot)
{
    throw "The destination project folder already exists: $projectRoot"
}

New-Item -ItemType Directory -Path $projectRoot | Out-Null
Copy-Item -Path (Join-Path $templateRoot "*") -Destination $projectRoot -Recurse

$textFiles = Get-ChildItem -Path $projectRoot -Recurse -File -Include *.txt,*.md,*.cmake,CMakeLists.txt,*.cpp,*.h
foreach ($file in $textFiles)
{
    $content = Get-Content -Path $file.FullName -Raw
    $content = $content.Replace("__HONTO_PROJECT_NAME__", $ProjectName)
    Set-Content -Path $file.FullName -Value $content -Encoding UTF8
}

Write-Output "Created HonTo project at $projectRoot"
