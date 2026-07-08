# PRD: Real-Time EEXI and CII Emissions Monitoring Plugin for OpenCPN

**Status:** `ready-for-agent`
**Date:** 2026-07-08
**Domain glossary:** [CONTEXT.md](file:///C:/Users/muigu/Documents/Projects/EEXI-CII/CONTEXT.md)
**Architectural decisions:** [docs/adr/](file:///C:/Users/muigu/Documents/Projects/EEXI-CII/docs/adr)
**Research:** [domain-gaps-analysis.md](file:///C:/Users/muigu/Documents/Projects/EEXI-CII/docs/research/domain-gaps-analysis.md)

---

## Problem Statement

Ship operators are required by IMO regulation (MARPOL Annex VI, Regulations 23 and 28) to demonstrate EEXI compliance for applicable ships of 400 GT and above, and to maintain acceptable CII ratings (A-C) for applicable ships of 5,000 GT and above. Currently, CII performance is assessed retrospectively through annual DCS submissions, giving masters and operators no opportunity to make informed speed or routing adjustments mid-voyage to improve their year-end rating. Commercial monitoring platforms exist but are proprietary, subscription-based, and inaccessible to smaller operators - particularly those in developing maritime economies who rely on open-source navigation software like OpenCPN. No existing OpenCPN plugin provides EEXI or CII monitoring.

## Solution

A freely available OpenCPN plugin (`eexi_cii_pi`) that calculates and displays in real time:

1. **EEXI compliance status** — a one-time pass/fail assessment computed from the Vessel Profile during setup.
2. **Running CII rating** — a continuously updated A–E rating derived from live NMEA 0183 RMC sentence data (SOG and position), using the Admiralty Coefficient method to estimate fuel consumption without additional hardware.

The plugin provides a dashboard showing live SOG, instantaneous CO₂ rate, current voyage and year-to-date AER (or cgDIST), the current CII rating with colour-coded band boundaries, and a trend chart tracking AER against rating boundaries over the calendar year. Data persists across sessions, auto-archives on January 1st, and can be exported as CSV for DCS reporting.

---

## User Stories

### Vessel Setup

1. As a ship operator, I want to enter my vessel's name, IMO number, GT, and Ship Type during initial setup, so that the plugin knows which IMO reference line coefficients and rating boundaries to use.
2. As a ship operator, I want to enter my vessel's DWT (or GT for cgDIST ship types), so that the AER or cgDIST denominator is correctly configured.
3. As a ship operator, I want to enter my vessel's Admiralty Coefficient, so that the plugin can estimate fuel consumption from SOG.
4. As a ship operator, I want to see default Admiralty Coefficient guidance values for my selected Ship Type (e.g. 600–750 for bulk carriers), so that I have a reasonable starting point if I don't have sea trial data.
5. As a ship operator, I want to set my SFOC value (defaulting to 190 g/kWh), so that fuel consumption estimation reflects my engine's actual efficiency.
6. As a ship operator, I want to select my fuel type from a dropdown (HFO, LFO, MDO, LNG, Methanol), so that the correct Emission Factor is used in CO₂ calculations.
7. As a ship operator, I want my Vessel Profile to persist across OpenCPN sessions, so that I don't have to re-enter ship data every time I launch the application.

### EEXI Compliance

8. As a ship operator, I want to enter my vessel's installed power (MCO) and reference speed, so that the plugin can compute my attained EEXI.
9. As a ship operator, I want to see my attained EEXI, required EEXI, and a clear pass/fail indicator, so that I know whether my vessel meets the EEXI requirement.
10. As a ship operator, I want the EEXI result to clearly state "Simplified estimate — consult class society for regulatory submission," so that I understand this is an approximation that omits correction factors.

### Voyage Management

11. As a ship officer, I want to start a new Voyage with a name and displacement value (e.g. "Rotterdam → Singapore, laden, 92,000 t"), so that accumulated data is grouped and the Admiralty formula uses the correct displacement.
12. As a ship officer, I want to end the current Voyage, so that a Voyage Record is created and I can update displacement for the next loading condition.
13. As a ship officer, I want the Accumulator to keep running between voyages using the last-known displacement, so that no operational data is lost from the year-to-date CII calculation.
14. As a ship officer, I want to see a prompt when displacement hasn't been updated for an extended period with no active Voyage, so that I'm reminded to start a new Voyage if loading condition has changed.
15. As a ship officer, I want data between voyages to appear as distinctly labelled "Unassigned" periods in my voyage history, so that I can see how much operational time fell outside of named voyages.

### Real-Time Monitoring

16. As a ship officer on watch, I want to see my current SOG on the dashboard updated every second, so that I have a live speed reference.
17. As a ship officer on watch, I want to see my instantaneous CO₂ rate (tonnes/hr) updated every second, so that I understand the emissions impact of my current speed.
18. As a ship officer on watch, I want to see my current voyage's accumulated distance, CO₂, and voyage AER, so that I can assess voyage-level efficiency.
19. As a ship officer on watch, I want to see my year-to-date distance, CO₂, Running AER, and current CII rating (A–E) with colour coding, so that I know where I stand relative to the annual CII requirement.
20. As a ship officer on watch, I want to see the numerical AER boundaries for each rating band (e.g. "Your AER: 3.33 | C boundary: 3.78 | D boundary: 5.10"), so that I know how much margin I have before my rating drops.
21. As a ship officer, I want to see a trend chart showing how my Running AER tracks against the A/B/C/D/E band boundaries over the calendar year, so that I can visualise my CII trajectory and anticipate year-end rating.
22. As a ship officer, I want the dashboard to show the EEXI pass/fail status in the vessel header, so that compliance status is always visible at a glance.

### Error Handling

23. As a ship officer, I want to see a clear "No GPS signal" warning when no valid NMEA data has been received for over 60 seconds, so that I know the Accumulator is frozen and data is not being lost silently.
24. As a ship officer, I want erroneous SOG values (> 50 knots for a bulk carrier) to be silently discarded, so that bad sensor data doesn't corrupt my CII calculations.
25. As a ship officer, I want data points below the SOG threshold (default 1.0 knot) to be excluded from accumulation with a visible indicator, so that port/anchor time doesn't distort my AER.
26. As a new user, I want the plugin to open the setup dialog on first launch when no Vessel Profile exists, so that I'm guided through configuration before seeing an empty dashboard.

### Year-End & Data Management

27. As a ship operator, I want the Accumulator to auto-archive on January 1st and reset for the new year, so that each calendar year's CII rating is computed independently per IMO requirements.
28. As a ship operator, I want to be notified when a year rollover occurs, so that I can review the previous year's final CII rating.
29. As a ship operator, I want to view historical yearly summaries and voyage records, so that I can track CII performance across years.
30. As a ship operator, I want to export an annual CSV summary (year, ship name, IMO number, DWT, total distance, total CO₂, AER, CII rating, Required CII), so that I can use the data for DCS submissions.
31. As a ship operator, I want to export a per-voyage breakdown CSV, so that I have detailed records for SEEMP Part III documentation.

### Mid-Year Installation

32. As a ship operator installing the plugin mid-year, I want to enter a YTD Seed (prior CO₂ in tonnes and distance in nautical miles from my logbook), so that the Running AER reflects the full calendar year.

### Scenario Planning

33. As a ship operator, I want to override the target year for CII calculation, so that I can see what my current AER would rate under future (tighter) Reduction Factors.

### Plugin Lifecycle

34. As an OpenCPN user, I want to toggle the dashboard visibility from the toolbar, so that I can show or hide it as needed without losing data.
35. As an OpenCPN user, I want the plugin to auto-save Accumulator state every 5 minutes and on shutdown, so that data survives unexpected application closures.

---

## Implementation Decisions

### Architecture: Greenfield C++ Plugin

- **Language:** C++, using the OpenCPN managed plugin API (v2) with the ShipDriver template as scaffold.
- **UI framework:** wxWidgets (provided by the OpenCPN build system).
- **Build system:** CMake with `opencpn-libs` as a git submodule, following the managed plugin workflow.
- **Target API:** OpenCPN Plugin API 1.16+ for broad compatibility with OpenCPN 5.x.
- **JSON library:** nlohmann/json (header-only) for voyage data persistence.
- **Test framework:** Google Test via CMake FetchContent.

### Module Structure (5 groups)

1. **Domain modules** (pure C++, zero external dependencies):
   - Fuel Estimator: Admiralty formula → shaft power → fuel rate → CO₂ rate. Pure function, no state.
   - AER Engine: Computes AER or cgDIST from accumulated totals; determines CII rating by comparing against Rating Band boundaries for the Ship Type and year.
   - CII Reference Data: Lookup tables for the modeled IMO Ship Type entries — reference line coefficients (a, c), rating boundary vectors (d1–d4), reference-capacity caps/floors where applicable, and Reduction Factor table values.
   - EEXI Calculator: One-shot simplified EEXI computation from Vessel Profile.

2. **Data modules** (minimal dependencies):
   - NMEA Parser: Parses $xxRMC sentences (all talker IDs: GP/GN/GL/GA/GB), validates checksum, checks status field, handles empty SOG. Returns structured data or invalid marker.
   - Haversine: Distance in nautical miles between two GPS fixes. Used for distance accumulation instead of SOG × Δt.
   - Accumulator: Stateful. Receives parsed data points + fuel estimates, accumulates CO₂ and distance per voyage and YTD. Manages voyage start/end, unassigned periods, SOG threshold filtering, YTD Seed, and calendar year rollover.
   - Persistence: JSON read/write for Accumulator state and Voyage Records. wxFileConfig for Vessel Profile settings (follows OpenCPN convention). Auto-save every 5 minutes via wxTimer. Data stored in plugin-private directory via `GetpPrivateApplicationDataLocation()`.

3. **Plugin entry point**: Implements the `opencpn_plugin` interface. Wires `SetNMEASentence()` callback (registered via `WANTS_NMEA_SENTENCES` capability flag) to the pure C++ `PluginCore`, which owns the NMEA parser -> fuel estimator -> accumulator -> AER engine chain. All processing is synchronous on the main GUI thread (microseconds per sentence).

4. **UI modules** (wxWidgets):
   - Setup Dialog: Tabbed dialog (Vessel Profile, EEXI, Advanced Settings). Opened from plugin preferences or on first launch.
   - Dashboard Panel: Docked wxPanel with vessel header, live gauges (SOG, CO₂ rate), voyage panel with start/end controls, YTD panel with Running AER and colour-coded CII rating, and action bar (Settings, Export, History).
   - Rating Chart: Custom-drawn wxPanel plotting Running AER against colour-coded rating band boundaries over the calendar year.

5. **Export module**: CSV export of annual summary and per-voyage breakdown.

### Data Flow (per NMEA sentence)

```
SetNMEASentence() -> PluginCore -> NMEAParser -> FuelEstimator -> Accumulator -> AEREngine -> DashboardPanel
```

Each step is a function call on the main thread. No worker threads needed.

### Persistence Strategy (ADR-0003)

- **Vessel Profile:** wxFileConfig in OpenCPN's shared config file. Follows established plugin convention.
- **Voyage data:** JSON files in plugin-private data directory. Includes current Accumulator state, active voyage, completed Voyage Records, and archived years.
- **Rationale:** JSON over SQLite because the data volume is small (hundreds of records/year) and human-readable files simplify debugging. wxFileConfig for settings because that's what every other OpenCPN plugin does.

### Key Calculation Details

- **AER units:** grams CO₂ / (tonne × nautical mile). Code must convert fuel (tonnes) × C_F (t-CO₂/t-fuel) × 10⁶ to grams.
- **cgDIST:** Same structure as AER but uses GT instead of DWT. Ship Type determines which formula is used.
- **Reduction Factor Z:** Lookup table, not a formula. MEPC.338(76) confirms 5% (2023), 7% (2024), 9% (2025), and 11% (2026). Years from 2027 onward must be source-tagged when implemented because MEPC.338(76) marks them for further development after the short-term-measure review.
- **Required CII:** CII_ref × (1 − Z/100), where CII_ref = a × Capacity^(−c).
- **SOG threshold:** Default 1.0 knot, configurable. Below threshold → data point excluded from accumulation (ADR-0004). This is a plugin-level model limitation, distinct from IMO Correction Factors (MEPC.355(78)).
- **Distance:** Computed via haversine between consecutive GPS fixes, not SOG × Δt.

### Own-Ship Only (ADR-0001)

The plugin tracks only the Own-Ship. AIS targets are not monitored — they lack continuous SOG data and have no associated Vessel Profile. This decision shapes the entire data model: single Vessel Profile, single Accumulator, single dashboard.

---

## Testing Decisions

### What makes a good test

Tests exercise **external behaviour through the module's interface**, not implementation details. A test should:
- Set up inputs (Vessel Profile values, NMEA sentences or parsed data points)
- Call the interface method
- Assert on outputs (fuel estimate, AER value, CII rating, accumulated totals)
- Never reach into private state, mock internal helpers, or assert on intermediate calculations

### Seam 1: Calculation Engine (primary testing seam)

All business logic is tested through the domain module interfaces as pure functions:

| Test area | Input | Expected output | Validation source |
|-----------|-------|-----------------|-------------------|
| Fuel Estimator | SOG + Vessel Profile | Shaft power (kW), fuel rate (t/hr), CO₂ rate (t/hr) | Hand calculation in spreadsheet |
| Fuel Estimator — sub-threshold | SOG < 1.0 knot | All zeros | Threshold behaviour |
| AER Engine | Accumulated CO₂ + distance + Vessel Profile + year | AER value, CII rating (A–E), band boundaries | IMO MEPC.354(78) worked examples |
| AER Engine — boundary cases | AER at exactly each d-vector boundary | Correct rating assignment | Rating boundary vectors |
| AER Engine — cgDIST | GT-based ship type | cgDIST value instead of AER | Formula verification |
| CII Reference Data | Each modeled Ship Type entry and size bracket | Correct a, c, d1–d4 coefficients and reference capacity | MEPC.353(78) and MEPC.354(78) tables |
| Reduction Factor | Confirmed years 2023-2026 | Correct Z% values | MEPC.338(76) table |
| EEXI Calculator | MCO, V_ref, DWT, SFOC, C_F | Attained EEXI, pass/fail | Hand calculation |
| Emission Factor | Each fuel type | Correct C_F value | MEPC.364(79) |

**Critical validation gate:** Hand-calculate AER and CII rating for a sample bulk carrier (DWT 80,000, 10,000 nm, 5,000 tonnes CO₂, year 2026) in a spreadsheet. The unit test must match within floating-point tolerance.

### Seam 2: Accumulator (stateful testing seam)

Tested by feeding sequences of data points and asserting on accumulated state:

| Test area | Scenario | Assertion |
|-----------|----------|-----------|
| Basic accumulation | Feed 100 data points with known SOG/position | Total distance matches haversine sum, total CO₂ matches fuel estimate sum |
| SOG threshold | Feed points above and below 1.0 knot | Below-threshold points excluded from totals |
| Voyage start/end | Start voyage, feed data, end voyage | Voyage Record created with correct totals; YTD includes voyage data |
| Unassigned periods | Feed data with no active Voyage | Data appears in unassigned pool; YTD totals still include it |
| YTD Seed | Set seed values, then accumulate | Running AER incorporates seed + new data |
| Year rollover | Simulate Jan 1 transition | Previous year archived; accumulators reset; new year starts fresh |
| Persistence round-trip | Save to JSON, load from JSON | All state survives: current voyage, YTD totals, voyage history |
| Empty SOG field | Feed RMC sentence with null SOG | Data point discarded, accumulator unchanged |
| Void status | Feed RMC sentence with V status | Data point discarded |

### NMEA Parser (targeted unit tests, not a separate seam)

A small set of focused tests for parsing edge cases:
- Valid $GPRMC sentence → correct lat, lon, SOG, timestamp
- All talker IDs (GP, GN, GL, GA, GB) → accepted
- Bad checksum → rejected
- Void status (V) → marked invalid
- Empty SOG field → marked invalid
- Malformed sentence → marked invalid
- NMEA 2.3 mode indicator present/absent → both handled

### Prior art

No existing tests in the codebase (greenfield). Test structure follows Google Test conventions with `TEST()` macros. Tests for domain modules have zero wxWidgets/OpenCPN dependency and can run independently of the plugin build.

---

## Out of Scope

| Feature | Reason |
|---------|--------|
| **Auxiliary engine fuel estimation** | No data source without fuel flow sensors. The Admiralty method estimates main engine consumption only. |
| **Speed Through Water (STW)** | Would require a log/speed sensor. SOG from GPS is sufficient for AER. |
| **Weather/sea state corrections** | Would require wave/wind data integration. The Admiralty method already assumes calm water. |
| **Multi-vessel fleet monitoring** | Own-Ship only (ADR-0001). AIS targets lack continuous data and Vessel Profiles. |
| **Automated DCS submission** | Would require API integration with classification societies and flag states. |
| **Route optimization / speed advice** | Different problem domain. The plugin provides awareness, not recommendations. |
| **IMO Correction Factors (MEPC.355(78))** | Ice navigation, emergency deviations, cargo heating adjustments — not in v1. Documented as known limitation. |
| **Full EEXI formula** | Correction factors (f_w, f_j, f_i, f_c, f_l, f_eff) omitted. Plugin provides simplified approximation with clear disclaimer. |
| **LPG/Ethane/Ethanol fuel types** | Only 5 common fuel types supported in v1 (HFO, LFO, MDO, LNG, Methanol). |
| **SEEMP document generation** | Plugin provides data that feeds SEEMP Part III but does not generate the document. |

---

## Further Notes

### Regulatory References

The plugin's calculations are grounded in the following IMO resolutions:

| Resolution | Subject | Used for |
|------------|---------|----------|
| MEPC.333(76) | EEXI Guidelines | EEXI formula (simplified) |
| MEPC.352(78) | G1: CII Calculation Methods | AER and cgDIST formulas |
| MEPC.353(78) | G2: CII Reference Lines | a, c coefficients per Ship Type |
| MEPC.354(78) | G4: CII Rating Boundaries | d1–d4 vectors per Ship Type |
| MEPC.338(76) | G3: Reduction Factors | Z% table by year |
| MEPC.355(78) | G5: Correction Factors | Documented as out of scope for v1 |
| MEPC.364(79) | Emission Factors | C_F values per fuel type |

### Accuracy Disclaimer

The plugin uses the Admiralty Coefficient method for fuel estimation (ADR-0002). This is a simplified model with typical accuracy of ±15–25%. It assumes calm water, constant hull resistance, and no auxiliary engine consumption. Accuracy degrades at speeds significantly different from design speed — which is precisely the slow-steaming regime where vessels operate to improve CII. The plugin is an **operational awareness tool**, not a regulatory submission system. Operators should use their classification society's verified monitoring platform for official DCS reporting.

### Academic Context

This plugin is being developed as a BSc Marine Engineering final-year project (2025/2026). The validation strategy includes:
1. Unit tests against hand-calculated values from IMO worked examples
2. Calculation flow validation against decoded voyage position data; AIS-derived data can validate replay, distance, and rating flow, but not real fuel-consumption accuracy
3. End-to-end testing via NMEA log replay through OpenCPN
4. Cross-checking against IMO MEPC.354(78) example calculations (must match within 1% tolerance)
