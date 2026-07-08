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

## OpenCPN Wrapper Configure

Once wxWidgets/OpenCPN build dependencies are installed, configure the wrapper
with:

```powershell
cmake -S . -B build-opencpn -DEEXI_CII_BUILD_OPENCPN_PLUGIN=ON -DEEXI_CII_BUILD_TESTS=OFF
cmake --build build-opencpn
```

On this machine, the configure probe currently stops at missing wxWidgets:

```text
Could NOT find wxWidgets
```

That means the source/template side is now present, and the next environment
step is installing/configuring wxWidgets in the same toolchain used for the
OpenCPN plugin build.
