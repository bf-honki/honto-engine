param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectName,
    [string]$DestinationPath = "."
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$templateCandidates = @(
    (Join-Path $scriptRoot "..\templates\HonToStarterVS"),
    (Join-Path $scriptRoot "..\share\HonToEngine\templates\HonToStarterVS")
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
    throw "Could not locate the HonTo Visual Studio starter template."
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

$sourceSln = Join-Path $projectRoot "HonToStarterVS.sln"
$targetSln = Join-Path $projectRoot "$ProjectName.sln"
if (Test-Path $sourceSln)
{
    Move-Item -Path $sourceSln -Destination $targetSln
}

$sourceProj = Join-Path $projectRoot "HonToStarterVS.vcxproj"
$targetProj = Join-Path $projectRoot "$ProjectName.vcxproj"
if (Test-Path $sourceProj)
{
    Move-Item -Path $sourceProj -Destination $targetProj
}

$sourceFilters = Join-Path $projectRoot "HonToStarterVS.vcxproj.filters"
$targetFilters = Join-Path $projectRoot "$ProjectName.vcxproj.filters"
if (Test-Path $sourceFilters)
{
    Move-Item -Path $sourceFilters -Destination $targetFilters
}

$projectGuid = [System.Guid]::NewGuid().ToString().ToUpperInvariant()
$solutionGuid = [System.Guid]::NewGuid().ToString().ToUpperInvariant()

$textFiles = Get-ChildItem -Path $projectRoot -Recurse -File -Include *.sln,*.vcxproj,*.filters,*.md,*.cpp,*.h,*.txt
foreach ($file in $textFiles)
{
    $content = Get-Content -Path $file.FullName -Raw
    $content = $content.Replace("__HONTO_PROJECT_NAME__", $ProjectName)
    $content = $content.Replace("__HONTO_PROJECT_GUID__", $projectGuid)
    $content = $content.Replace("__HONTO_SOLUTION_GUID__", $solutionGuid)
    Set-Content -Path $file.FullName -Value $content -Encoding UTF8
}

Write-Output "Created HonTo Visual Studio project at $projectRoot"
