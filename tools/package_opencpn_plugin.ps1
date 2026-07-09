param(
    [string]$BuildDir = "build-opencpn-vcpkg-x86-release",
    [string]$OutputDir = "dist",
    [string]$Version = "0.1.0",
    [string]$Release = "1",
    [string]$TarballUrl = "",
    [string]$SourceUrl = "https://github.com/OpenCPN/eexi_cii_pi",
    [string]$Author = "EEXI-CII Plugin Project",
    [string]$Target = "msvc-wx32",
    [string]$TargetVersion = "msvc-wx32",
    [string]$TargetArch = "x86",
    [string]$SchemaPath = ""
)

$ErrorActionPreference = "Stop"

$pluginName = "eexi_cii_pi"
$dllPath = Join-Path $BuildDir "$pluginName.dll"
if (-not (Test-Path -LiteralPath $dllPath)) {
    throw "Plugin DLL not found: $dllPath"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$packageName = "$pluginName-$Version-$Release-$Target"
$stageParent = Join-Path $OutputDir "stage"
$stageRoot = Join-Path $stageParent $packageName
if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stageRoot | Out-Null
Copy-Item -LiteralPath $dllPath -Destination (Join-Path $stageRoot "$pluginName.dll") -Force

$tarballPath = Join-Path $OutputDir "$packageName.tar.gz"
if (Test-Path -LiteralPath $tarballPath) {
    Remove-Item -LiteralPath $tarballPath -Force
}
tar -czf $tarballPath -C $stageParent $packageName

$checksum = (Get-FileHash -Algorithm SHA256 -LiteralPath $tarballPath).Hash.ToLowerInvariant()
if ([string]::IsNullOrWhiteSpace($TarballUrl)) {
    $TarballUrl = "https://example.invalid/releases/$packageName.tar.gz"
}

$xmlPath = Join-Path $OutputDir "$pluginName-$Target.xml"
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($xmlPath, $settings)
try {
    $writer.WriteStartDocument()
    $writer.WriteStartElement("opencpn-plugin")
    $writer.WriteAttributeString("version", "1")
    $writer.WriteElementString("name", $pluginName)
    $writer.WriteElementString("version", $Version)
    $writer.WriteElementString("release", $Release)
    $writer.WriteElementString("summary", "Real-time EEXI/CII emissions monitor")
    $writer.WriteElementString("api-version", "1.21")
    $writer.WriteElementString("open-source", "yes")
    $writer.WriteElementString("author", $Author)
    $writer.WriteElementString("source", $SourceUrl)
    $writer.WriteElementString("info-url", $SourceUrl)
    $writer.WriteElementString(
        "description",
        "Operational-awareness OpenCPN plugin for own-ship EEXI estimate and running CII/AER monitoring from RMC data."
    )
    $writer.WriteElementString("target", $Target)
    $writer.WriteElementString("target-version", $TargetVersion)
    $writer.WriteElementString("target-arch", $TargetArch)
    $writer.WriteElementString("tarball-url", $TarballUrl)
    $writer.WriteElementString("tarball-checksum", $checksum)
    $writer.WriteEndElement()
    $writer.WriteEndDocument()
} finally {
    $writer.Close()
}

if (-not [string]::IsNullOrWhiteSpace($SchemaPath)) {
    $schemaFile = Resolve-Path -LiteralPath $SchemaPath
    $xmlFile = Resolve-Path -LiteralPath $xmlPath
    $validationErrors = New-Object System.Collections.Generic.List[string]
    $readerSettings = New-Object System.Xml.XmlReaderSettings
    $readerSettings.ValidationType = [System.Xml.ValidationType]::Schema
    [void]$readerSettings.Schemas.Add("", $schemaFile.Path)
    $readerSettings.add_ValidationEventHandler({
        param($Sender, $EventArgs)
        $validationErrors.Add($EventArgs.Message)
    })

    $reader = [System.Xml.XmlReader]::Create($xmlFile.Path, $readerSettings)
    try {
        while ($reader.Read()) {
        }
    } finally {
        $reader.Close()
    }

    if ($validationErrors.Count -gt 0) {
        throw "Metadata XML failed schema validation: $($validationErrors -join '; ')"
    }
}

Write-Output "Tarball: $tarballPath"
Write-Output "Metadata: $xmlPath"
Write-Output "SHA256: $checksum"
