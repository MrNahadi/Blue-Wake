# OpenCPN Plugin-Manager Release Guide

This project targets a stable OpenCPN community-plugin release first. It is not
a regulatory submission system and is not class-society validated.

## Build Target

Initial release target:

- OpenCPN target: `msvc-wx32`
- Architecture: `x86`
- API version: `1.21`
- Build output: `build-opencpn-vcpkg-x86-release/eexi_cii_pi.dll`

Use the existing Windows build from `docs/OpenCPN_BUILD.md`, then package:

```powershell
.\tools\package_opencpn_plugin.ps1 `
  -BuildDir build-opencpn-vcpkg-x86-release `
  -OutputDir dist `
  -Version 0.1.0 `
  -Release 1 `
  -TarballUrl https://example.invalid/releases/eexi_cii_pi-0.1.0-1-msvc-wx32.tar.gz `
  -SourceUrl https://github.com/OpenCPN/eexi_cii_pi `
  -SchemaPath C:\path\to\OpenCPN\plugins\ocpn-plugin.xsd
```

Replace `TarballUrl` and `SourceUrl` with the real release locations before
submitting metadata upstream.

## Release Artifacts

The packaging script creates:

- `dist/eexi_cii_pi-<version>-<release>-msvc-wx32.tar.gz`
- `dist/eexi_cii_pi-msvc-wx32.xml`

The XML includes the fields required by OpenCPN's plugin catalog schema:
plugin name, version, release, summary, API version, source URL, target,
target version, target architecture, tarball URL, and SHA256 checksum.

## Validation Before PR

1. Run the full local test suite:
   ```powershell
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```
2. Import the generated tarball directly in OpenCPN through the Plugins UI.
3. Confirm the plugin enables, shows the toolbar toggle, opens the monitor,
   accepts vessel setup, replays RMC data, exports CSV, and records diagnostics.
4. Submit generated metadata to the OpenCPN/plugins `Beta` branch first.
5. Promote to `master` only after Beta install/replay feedback is clean.

## Known Limitations

- Uses Admiralty Coefficient fuel estimation; no direct fuel-flow validation.
- Tracks own-ship only.
- Uses RMC position/SOG only.
- Excludes port/anchor time below the configured SOG threshold.
- Does not implement IMO correction factors or full EEXI correction factors.
- The validation pack is a release smoke test, not regulatory evidence.
