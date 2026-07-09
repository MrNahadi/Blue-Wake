# Field-Test Gate

Use this checklist before treating a build as plugin-manager ready.

## Required Manual Scenarios

- Fresh install: import the tarball into OpenCPN, enable the plugin, and complete first-run setup.
- Upgrade: install over a previous `eexi_cii_pi` build and confirm settings and voyage data remain readable.
- Corrupted state: replace `voyagedata.json` with invalid JSON and confirm the dashboard reports a diagnostic instead of crashing.
- Missing GPS: stop RMC input for more than 60 seconds and confirm the monitor reports stale GPS.
- Invalid NMEA: feed malformed or checksum-invalid RMC sentences and confirm invalid-burst diagnostics appear.
- SOG outlier: feed a valid RMC sentence above the outlier limit and confirm it is rejected without changing totals.
- Long replay: run a 24-72 hour RMC replay and confirm memory use and UI responsiveness remain acceptable.
- Mid-year seed: configure YTD CO2 and distance seed values and confirm Running AER includes them once.
- Year rollover: replay data crossing into a later calendar year and confirm previous YTD state is archived.
- Restart recovery: close OpenCPN, reopen it, and confirm accumulator state and dashboard visibility persist.
- CSV export: export annual and voyage CSV files and open them in a spreadsheet.

## Pass Criteria

- No crash or OpenCPN startup warning.
- Toolbar toggle reliably shows/hides the monitor.
- Monitor status, GPS freshness, CII rating, and diagnostics update during replay.
- CSV output matches the values shown in the monitor.
- Any persistence or input failure is visible in Diagnostics and the OpenCPN log.

## Release Boundary

Passing this gate means the plugin is suitable for community Beta distribution.
It does not mean the plugin is validated for official DCS, SEEMP, EEXI, or CII
regulatory submission.
