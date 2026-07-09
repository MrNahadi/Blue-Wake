# OpenCPN Plugin Build Notes

This project keeps the calculation engine buildable without OpenCPN, then adds
a thin OpenCPN adapter behind an opt-in CMake flag.

## OpenCPN Template/API Source

The project uses OpenCPN's shared plugin support repository as a Git submodule:

```powershell
git submodule update --init --recursive
```

The plugin API header is provided by:

```text
opencpn-libs/api-21/ocpn_plugin.h
```

`opencpn-libs/api-21/CMakeLists.txt` exports the `ocpn::api` target and sets
API version `1.21`, with minimum OpenCPN version `5.11.0`.

## Current Adapter

The first OpenCPN-facing files are:

```text
src/opencpn/eexi_cii_pi.h
src/opencpn/eexi_cii_pi.cpp
```

They implement the standard `create_pi` and `destroy_pi` entry points, return
the `WANTS_NMEA_SENTENCES` capability, and forward `SetNMEASentence()` into the
tested `eexi_cii::PluginCore`.

## Local Core Build

The normal development build does not require wxWidgets or OpenCPN:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

## Windows OpenCPN Wrapper Configure

The `opencpn-libs/api-21/msvc-wx32/opencpn.lib` import library is 32-bit.
Build this wrapper with the x86 MSVC toolchain and `x86-windows` vcpkg triplet.

The working local setup is:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\Users\muigu\vcpkg
C:\Users\muigu\vcpkg\bootstrap-vcpkg.bat -disableMetrics
C:\Users\muigu\vcpkg\vcpkg.exe install wxwidgets:x86-windows
```

Configure from a Visual Studio developer environment:

```powershell
cmd.exe /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x86 && cmake -S . -B build-opencpn-vcpkg-x86 -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/Users/muigu/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x86-windows -DEEXI_CII_BUILD_OPENCPN_PLUGIN=ON -DEEXI_CII_BUILD_TESTS=OFF"
cmd.exe /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"" -arch=x86 && cmake --build build-opencpn-vcpkg-x86"
```

The built plugin DLL is:

```text
build-opencpn-vcpkg-x86/eexi_cii_pi.dll
```

An x64 configure can find wxWidgets, but linking fails because the bundled
OpenCPN import library is `x86`.
