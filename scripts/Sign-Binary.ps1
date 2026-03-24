param(
    [Parameter(Mandatory = $true)]
    [string]$TargetPath,

    [string]$SignToolPath = $env:HONTO_SIGNTOOL_EXE,
    [string]$PfxPath = $env:HONTO_SIGN_PFX_PATH,
    [string]$PfxPassword = $env:HONTO_SIGN_PFX_PASSWORD,
    [string]$CertSha1 = $env:HONTO_SIGN_CERT_SHA1,
    [string]$CertSubject = $env:HONTO_SIGN_CERT_SUBJECT,
    [string]$MachineStore = $env:HONTO_SIGN_MACHINE_STORE,
    [string]$TimestampUrl = $env:HONTO_SIGN_TIMESTAMP_URL
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-SignToolPath {
    param([string]$PreferredPath)

    if ($PreferredPath -and (Test-Path $PreferredPath)) {
        return $PreferredPath
    }

    $candidates = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName "x64\signtool.exe" } |
        Where-Object { Test-Path $_ }

    if ($candidates.Count -gt 0) {
        return $candidates[0]
    }

    throw "signtool.exe was not found. Set HONTO_SIGNTOOL_EXE or install the Windows SDK."
}

if (-not (Test-Path $TargetPath)) {
    throw "Target file was not found: $TargetPath"
}

$resolvedSignTool = Resolve-SignToolPath -PreferredPath $SignToolPath
$signArgs = @("sign", "/fd", "SHA256")

if ($TimestampUrl) {
    $signArgs += @("/tr", $TimestampUrl, "/td", "SHA256")
}

if ($PfxPath) {
    if (-not (Test-Path $PfxPath)) {
        throw "PFX certificate was not found: $PfxPath"
    }

    $signArgs += @("/f", $PfxPath)

    if ($PfxPassword) {
        $signArgs += @("/p", $PfxPassword)
    }
}
elseif ($CertSha1) {
    $signArgs += @("/sha1", $CertSha1, "/s", "My")

    if ($MachineStore -eq "1") {
        $signArgs += "/sm"
    }
}
elseif ($CertSubject) {
    $signArgs += @("/n", $CertSubject, "/s", "My")

    if ($MachineStore -eq "1") {
        $signArgs += "/sm"
    }
}
else {
    throw "No certificate configuration was provided. Set a PFX path, certificate thumbprint, or certificate subject."
}

$signArgs += $TargetPath

Write-Host "Signing $TargetPath"
$signProcess = Start-Process -FilePath $resolvedSignTool -ArgumentList $signArgs -NoNewWindow -Wait -PassThru

if ($signProcess.ExitCode -ne 0) {
    throw "signtool sign failed with exit code $($signProcess.ExitCode)"
}

$verifyArgs = @("verify", "/pa", $TargetPath)
Write-Host "Verifying signature for $TargetPath"
$verifyProcess = Start-Process -FilePath $resolvedSignTool -ArgumentList $verifyArgs -NoNewWindow -Wait -PassThru

if ($verifyProcess.ExitCode -ne 0) {
    throw "signtool verify failed with exit code $($verifyProcess.ExitCode)"
}

Write-Host "Signing completed successfully."
