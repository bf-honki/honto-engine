param(
    [string]$Subject = "CN=HonTo Engine Local Development",
    [string]$PropsPath = "C:\HonTo_Engine\HonTo_Engine\HonTo_Engine\LocalSigning.props"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$thumbprints = Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.Subject -eq $Subject } |
    Select-Object -ExpandProperty Thumbprint

foreach ($storeName in @("TrustedPublisher", "Root", "My")) {
    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store($storeName, "CurrentUser")
    $store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)

    try {
        foreach ($thumbprint in $thumbprints) {
            $matches = @($store.Certificates | Where-Object Thumbprint -eq $thumbprint)
            foreach ($certificate in $matches) {
                $store.Remove($certificate)
            }
        }
    }
    finally {
        $store.Close()
    }
}

if (Test-Path $PropsPath) {
    Remove-Item $PropsPath -Force
}

[PSCustomObject]@{
    RemovedThumbprints = ($thumbprints -join ", ")
    PropsDeleted = -not (Test-Path $PropsPath)
}
