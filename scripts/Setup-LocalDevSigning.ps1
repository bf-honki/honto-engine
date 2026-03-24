param(
    [string]$Subject = "CN=HonTo Engine Local Development",
    [string]$PropsPath = "C:\HonTo_Engine\HonTo_Engine\HonTo_Engine\LocalSigning.props"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Ensure-CertificateInStore {
    param(
        [System.Security.Cryptography.X509Certificates.X509Certificate2]$Certificate,
        [string]$StoreName
    )

    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store($StoreName, "CurrentUser")
    $store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)

    try {
        $existing = $store.Certificates | Where-Object Thumbprint -eq $Certificate.Thumbprint
        if (-not $existing) {
            $store.Add($Certificate)
        }
    }
    finally {
        $store.Close()
    }
}

$existingCert = Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.Subject -eq $Subject } |
    Sort-Object NotAfter -Descending |
    Select-Object -First 1

if (-not $existingCert) {
    $existingCert = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject $Subject `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -HashAlgorithm "SHA256" `
        -NotAfter (Get-Date).AddYears(2)
}

Ensure-CertificateInStore -Certificate $existingCert -StoreName "Root"
Ensure-CertificateInStore -Certificate $existingCert -StoreName "TrustedPublisher"

$propsDirectory = Split-Path -Parent $PropsPath
New-Item -ItemType Directory -Force $propsDirectory | Out-Null

$propsContent = @"
<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <EnableHonToSigning>true</EnableHonToSigning>
    <HonToSignToolExe></HonToSignToolExe>
    <HonToSignPfxPath></HonToSignPfxPath>
    <HonToSignPfxPassword></HonToSignPfxPassword>
    <HonToSignCertSha1>$($existingCert.Thumbprint)</HonToSignCertSha1>
    <HonToSignCertSubject></HonToSignCertSubject>
    <HonToSignMachineStore>0</HonToSignMachineStore>
    <HonToSignTimestampUrl></HonToSignTimestampUrl>
  </PropertyGroup>
</Project>
"@

Set-Content -Path $PropsPath -Value $propsContent -Encoding UTF8

[PSCustomObject]@{
    Subject = $existingCert.Subject
    Thumbprint = $existingCert.Thumbprint
    PropsPath = $PropsPath
}
