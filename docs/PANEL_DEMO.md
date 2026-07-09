# Panel Demo Runbook

This runbook shows the EEXI-CII plugin without a real ship, GPS receiver, or live voyage.

## Demo Story

The demo simulates a vessel leaving Mombasa and sends normal NMEA RMC sentences to OpenCPN over UDP. OpenCPN treats the feed like live navigation data, and the EEXI-CII plugin calculates speed-based fuel burn, CO2, voyage totals, YTD totals, AER, and CII rating.

## Before the Demo

1. Build and install the plugin using `docs/OpenCPN_TESTING.md`.
2. Start OpenCPN.
3. Open Options > Connections.
4. Add a Network input connection:
   - Protocol: UDP
   - Address: `0.0.0.0` or blank, depending on the OpenCPN dialog
   - Data port: `10110`
   - Input filtering: off
5. Open Options > Plugins and enable EEXI-CII.

If the setup dialog appears, use a consistent demo profile:

```text
Ship name: Demo Vessel
IMO number: 9999999
Gross tonnage: 50000
Deadweight: 80000
EEXI attained: 6.80
Main engine SFOC: 175
Fuel type: HFO
Displacement: 92000
Minimum SOG: 1.0
Target CII: Auto
```

## Run the Simulated Voyage

From the project root, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\demo\nmea_demo.ps1
```

Optional shorter demo:

```powershell
powershell -ExecutionPolicy Bypass -File .\demo\nmea_demo.ps1 -DurationSeconds 120 -SpeedKnots 14.2
```

The script sends one `$GPRMC` sentence per second to `udp://127.0.0.1:10110`.

## What the Panel Should See

1. OpenCPN's own status bar should change from no GPS/SOG to a live SOG value.
2. The EEXI-CII Monitor should move from waiting state to accumulating state.
3. SOG and CO2 rate should update immediately.
4. Voyage distance and voyage CO2 should climb during the demo.
5. Running AER and CII rating should appear once enough distance has accumulated.

## Presentation Talk Track

Use this order:

1. "OpenCPN is receiving a standard NMEA navigation stream, the same class of data it would receive from shipboard equipment."
2. "The plugin does not need a separate manual voyage entry to start measuring movement; it reacts to the live RMC stream."
3. "From speed over ground and the vessel profile, it estimates fuel use and CO2 in near real time."
4. "It keeps both current-voyage totals and year-to-date totals, so operators can see the effect of speed and route decisions."
5. "The same core calculation is covered by automated tests, while OpenCPN provides the operational display."

## If Nothing Updates

Check these first:

1. Confirm the simulator is still running in PowerShell.
2. Confirm OpenCPN has a UDP network input on port `10110`.
3. Restart OpenCPN after installing or replacing plugin DLLs.
4. Check `C:\ProgramData\opencpn\opencpn.log` for plugin load messages.
