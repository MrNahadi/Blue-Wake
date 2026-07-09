# Troubleshooting

## Plugin Does Not Appear In OpenCPN

- If the Plugins page only shows entries such as ChartDownloader, WMM,
  Dashboard, and GRIB, OpenCPN is working but Blue Wake/EEXI-CII is not loaded.
  Import the plugin tarball or copy the Windows DLLs into OpenCPN's plugin
  folder first.
- The Dashboard entry is OpenCPN's built-in dashboard plugin, not Blue Wake.
- Confirm the imported tarball contains `eexi_cii_pi.dll`.
- Confirm the metadata target is `msvc-wx32` and target architecture is `x86`.
- Check the OpenCPN log for load errors or missing runtime DLLs.

## Monitor Shows No GPS Fix

- Confirm OpenCPN is receiving valid own-ship RMC sentences.
- Confirm the RMC status field is `A`, not `V`.
- Confirm RMC checksum validation passes.
- Use `demo/nmea_demo.ps1` for a repeatable no-hardware replay.

## Totals Do Not Increase

- Confirm SOG is above the configured SOG threshold.
- Confirm the SOG value is below the outlier limit.
- Confirm at least two valid fixes have arrived; the first valid fix is only a baseline.

## CSV Export Is Unavailable

- Annual export requires accumulated distance and CO2 so an AER/cgDIST result exists.
- Voyage export requires at least one completed voyage record.

## Regulatory Use

The plugin is an operational-awareness aid. Use a verified platform and class
society process for official DCS, SEEMP, EEXI, or CII submissions.
