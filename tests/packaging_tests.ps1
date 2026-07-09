$ErrorActionPreference = "Stop"

function Assert-True($Condition, $Message) {
    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal($Actual, $Expected, $Message) {
    if ($Actual -ne $Expected) {
        throw "$Message expected '$Expected' got '$Actual'"
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$workRoot = Join-Path $repoRoot "build\packaging-test"
$fakeBuild = Join-Path $workRoot "build-opencpn"
$dist = Join-Path $workRoot "dist"

New-Item -ItemType Directory -Force -Path $fakeBuild | Out-Null
Set-Content -LiteralPath (Join-Path $fakeBuild "eexi_cii_pi.dll") -Value "fake plugin dll" -Encoding ASCII

& (Join-Path $repoRoot "tools\package_opencpn_plugin.ps1") `
    -BuildDir $fakeBuild `
    -OutputDir $dist `
    -Version "0.1.0" `
    -Release "test" `
    -TarballUrl "https://example.invalid/eexi_cii_pi-0.1.0-test-msvc-wx32.tar.gz" `
    -SchemaPath (Join-Path $repoRoot "tests\fixtures\opencpn-plugin-minimal.xsd") `
    | Out-Null

$tarball = Join-Path $dist "eexi_cii_pi-0.1.0-test-msvc-wx32.tar.gz"
$metadata = Join-Path $dist "eexi_cii_pi-msvc-wx32.xml"

Assert-True (Test-Path -LiteralPath $tarball) "Tarball was not created"
Assert-True (Test-Path -LiteralPath $metadata) "Metadata XML was not created"

[xml]$xml = Get-Content -LiteralPath $metadata
$plugin = $xml.'opencpn-plugin'
Assert-Equal $plugin.SelectSingleNode("name").InnerText "eexi_cii_pi" "Plugin name"
Assert-Equal $plugin.SelectSingleNode("version").InnerText "0.1.0" "Plugin version"
Assert-Equal $plugin.SelectSingleNode("release").InnerText "test" "Plugin release"
Assert-Equal $plugin.SelectSingleNode("api-version").InnerText "1.21" "API version"
Assert-Equal $plugin.SelectSingleNode("target").InnerText "msvc-wx32" "Target"
Assert-Equal $plugin.SelectSingleNode("target-arch").InnerText "x86" "Target architecture"

$expectedChecksum = (Get-FileHash -Algorithm SHA256 -LiteralPath $tarball).Hash.ToLowerInvariant()
Assert-Equal $plugin.SelectSingleNode("tarball-checksum").InnerText $expectedChecksum "Tarball checksum"

$tarListing = tar -tf $tarball
Assert-True (
    $tarListing -contains "eexi_cii_pi-0.1.0-test-msvc-wx32/eexi_cii_pi.dll"
) "Tarball layout did not include the plugin DLL under the package directory"
