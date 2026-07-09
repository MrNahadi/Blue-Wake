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

Close OpenCPN, then copy the DLL into the installed OpenCPN plugin directory:

```powershell
Copy-Item -LiteralPath "C:\Users\muigu\Documents\Projects\EEXI-CII\build-opencpn-vcpkg-x86-release\eexi_cii_pi.dll" -Destination "C:\Program Files (x86)\OpenCPN\plugins\eexi_cii_pi.dll" -Force
```

`C:\Program Files (x86)\OpenCPN\plugins` requires Administrator rights on this machine, so run the copy from an elevated PowerShell window.

## Smoke Test

1. Start OpenCPN.
2. Open Options > Plugins.
3. Enable the EEXI-CII plugin.
4. Complete the first-run setup dialog with test vessel values.
5. Confirm the EEXI-CII Monitor window opens.
6. Feed valid RMC data and confirm SOG, voyage totals, YTD totals, AER, and CII rating update.
