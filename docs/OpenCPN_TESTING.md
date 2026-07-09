# OpenCPN Local Testing

Use this workflow to test the plugin in the local Windows OpenCPN install.

## Build

Build the 32-bit Release plugin from a Visual Studio developer environment:

```powershell
cmd.exe /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x86 && cmake --build build-opencpn-vcpkg-x86-release"
```

The plugin DLL is written to:

```text
build-opencpn-vcpkg-x86-release\eexi_cii_pi.dll
```

## Install

Close OpenCPN, then copy the DLLs into OpenCPN's per-user plugin directory:

```powershell
New-Item -ItemType Directory -Force -Path "C:\Users\muigu\AppData\Local\opencpn\plugins" | Out-Null
Copy-Item -LiteralPath (Get-ChildItem "C:\Users\muigu\Documents\Projects\EEXI-CII\build-opencpn-vcpkg-x86-release" -Filter "*.dll").FullName -Destination "C:\Users\muigu\AppData\Local\opencpn\plugins" -Force
```

OpenCPN checks `C:\Users\muigu\AppData\Local\opencpn\plugins` before the global `C:\Program Files (x86)\OpenCPN\plugins` folder, and the per-user folder does not require Administrator rights.

## Smoke Test

1. Start OpenCPN.
2. Open Options > Plugins.
3. Enable the EEXI-CII or Blue Wake plugin. If it is not listed and you only see
   catalog entries such as ChartDownloader, WMM, Dashboard, and GRIB, OpenCPN has
   not loaded the plugin yet; repeat the install step above.
4. Complete the first-run setup dialog with test vessel values.
5. Confirm the EEXI-CII Monitor window opens.
6. Feed valid RMC data and confirm SOG, voyage totals, YTD totals, AER, and CII rating update.

For a repeatable no-hardware demonstration, use `docs/PANEL_DEMO.md`.
