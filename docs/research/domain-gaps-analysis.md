# Domain Gaps Analysis: EEXI-CII Plugin

> Research conducted against IMO resolutions, OpenCPN source code/docs, NMEA specs, and marine engineering references.
> Date: 2026-07-08

---

## 1. IMO CII Formulas & Reference Lines

### 1a. Reference Line Coefficients — Resolution Identifiers

- **What we say**: The glossary and brief reference MEPC.337(76) for CII guidelines.
- **What the source says**: The CII framework spans **multiple resolutions**, each covering a specific aspect:
  - **MEPC.352(78)** — G1: Operational CII calculation methods (the AER formula itself)
  - **MEPC.353(78)** — G2: CII Reference Lines (the `a`, `c` coefficients)
  - **MEPC.354(78)** — G4: CII Rating boundaries (the `d1`–`d4` vectors)
  - **MEPC.338(76)** — G3: Reduction factors (`Z%` values per year)
  - **MEPC.355(78)** — G5: Correction factors and voyage adjustments
  - MEPC.337(76) is the earlier (2021) version; the 2022 updates at MEPC 78 supersede or supplement it.
- **Gap/Action**: The brief references only MEPC.337(76). The glossary should reference the full G1–G5 resolution suite, especially the 2022 updates from MEPC 78. Consider adding a "Regulatory References" section listing all applicable resolutions.

### 1b. CII Reference Line Formula — "b" Coefficient Error

- **What we say**: CONTEXT.md states "distinct CII reference line coefficients (a, b, c)."
- **What the source says**: The CII reference line formula is **CII_ref = a × Capacity^(−c)** per MEPC.353(78). There are only **two parameters: `a` and `c`**. There is no `b` parameter.
- **Gap/Action**: **ERROR** — Remove `b` from the glossary. The reference line has only two coefficients per ship type: `a` and `c`.

### 1c. Ship Type Count

- **What we say**: CONTEXT.md says "~15 IMO-defined vessel categories."
- **What the source says**: MEPC.353(78) and MEPC.354(78) define the following distinct categories (counting size sub-categories separately):

  | # | Ship Type | Capacity | CII Metric |
  |---|-----------|----------|------------|
  | 1 | Bulk carrier | DWT | AER |
  | 2 | Gas carrier (≥ 65,000 DWT) | DWT | AER |
  | 3 | Gas carrier (< 65,000 DWT) | DWT | AER |
  | 4 | Tanker | DWT | AER |
  | 5 | Container ship | DWT | AER |
  | 6 | General cargo ship | DWT | AER |
  | 7 | Refrigerated cargo carrier | DWT | AER |
  | 8 | Combination carrier | DWT | AER |
  | 9 | LNG carrier (≥ 100,000 DWT) | DWT | AER |
  | 10 | LNG carrier (< 100,000 DWT) | DWT | AER |
  | 11 | Ro-ro cargo ship (vehicle carrier) | GT | cgDIST |
  | 12 | Ro-ro cargo ship | GT | cgDIST |
  | 13 | Ro-ro passenger ship | GT | cgDIST |
  | 14 | Cruise passenger ship | GT | cgDIST |

  Some sources also list **High-speed craft (SOLAS Ch X)** as applicable, bringing the total to 15 line items in the d-vector tables, though this is rarely encountered.

- **Gap/Action**: "~15" is approximately correct but imprecise. The glossary should state **14 distinct categories** (or 15 including high-speed craft) with size sub-categories. More importantly, the glossary lists only "(bulk carrier, tanker, container ship, gas carrier, LNG carrier, general cargo, etc.)" — **missing ro-ro cargo, ro-ro passenger, cruise passenger, refrigerated cargo, and combination carrier** from the explicit list. Consider listing all types in a table.

### 1d. AER Formula Units

- **What we say**: CONTEXT.md says "grams CO₂ per tonne-nautical-mile."
- **What the source says**: Per MEPC.352(78), AER = ΣM / Σ(DWT × D), where M is total mass of CO₂ in **grams**, DWT is summer deadweight tonnage, and D is distance in nautical miles. The unit is **g CO₂ / (dwt·nm)**.
- **Gap/Action**: **Confirmed correct** — the glossary accurately states grams. However, note that in the actual CO₂ mass calculation from fuel: M = fuel consumed (tonnes) × C_F (t-CO₂/t-fuel) × 10⁶ to convert to grams. The plugin code must handle this unit conversion carefully.

### 1e. Reduction Factors — NOT 2% Per Year

- **What we say**: CONTEXT.md says "reduction factor of 2% per year." Brief says "2% per annum from 2023 onwards." CONTEXT.md also says "increasing by ~2% per year."
- **What the source says**: Per MEPC.338(76) and subsequent updates, the reduction factors (Z%) relative to the **2019 reference line** are:

  | Year | Z (%) |
  |------|-------|
  | 2023 | 5% |
  | 2024 | 7% |
  | 2025 | 9% |
  | 2026 | 11% |
  | 2027 | 13.625% |
  | 2028 | 16.250% |
  | 2029 | 18.875% |
  | 2030 | 21.500% |

  The **increment** from 2023→2024 is 2%, from 2024→2025 is 2%, from 2025→2026 is 2%, so the step is ~2% per year. But the **cumulative** factor is NOT 2% — it's 5% in the first year (2023) already. After 2026, the annual step increases to ~2.625%.

- **Gap/Action**: **SIGNIFICANT ERROR** — "Reduction factor of 2%" is misleading. The glossary must clarify:
  1. The reduction factor Z is cumulative from the 2019 baseline (5% in 2023, not 2%)
  2. The Required CII = CII_ref × (1 − Z/100)
  3. The annual increment is ~2% (2023–2026), then ~2.625% (2027–2030)
  4. The full Z value table should be stored as a lookup in the plugin, not computed from a simple "2% per year" rule

### 1f. Ship Types Using cgDIST Instead of AER

- **What we say**: The glossary defines AER as "the primary CII calculation method" and doesn't mention cgDIST.
- **What the source says**: Per MEPC.352(78):
  - **AER** (using DWT): Bulk carriers, tankers, container ships, gas carriers, LNG carriers, general cargo, refrigerated cargo, combination carriers
  - **cgDIST** (using GT): Cruise passenger ships, ro-ro cargo ships (vehicle carriers), ro-ro cargo ships, ro-ro passenger ships
  - cgDIST formula: cgDIST = ΣM / Σ(GT × D) — same structure, but uses GT instead of DWT
- **Gap/Action**: **MISSING CONCEPT** — The glossary has no entry for **cgDIST**. If the plugin supports cruise ships, ro-ro vessels, or vehicle carriers, it must use GT instead of DWT and the cgDIST formula. At minimum, add a glossary entry for cgDIST and note which ship types use it. The Vessel Profile needs a GT field for these types.

---

## 2. EEXI Formula

### 2a. Simplified vs Full EEXI Formula

- **What we say**: The brief describes EEXI as "a one-time, design-based metric calculated from the ship's technical specifications."
- **What the source says**: Per MEPC.333(76), the full EEXI formula is substantially more complex than EEXI = (P × C_F × SFOC) / (DWT × V_ref). The full formula includes:
  - **Numerator**: Main engine power (P_ME at 75% MCR or 83% of MCR_lim if EPL installed), auxiliary engine power (P_AE), each with their own SFOC and C_F
  - **Correction factors**: f_w (weather), f_j (design elements for ice class/shuttle tankers), f_i (capacity correction for ice class), f_c (cubic capacity for chemical/gas carriers), f_l (cargo gear for general cargo), f_eff (innovative energy technologies), f_m (ice class)
  - **Innovative technology credit**: P_AEeff, P_eff for wind, solar, etc.
- **Gap/Action**: The glossary's implied simplified formula is adequate for a first-pass estimation tool, but the plugin should document that it uses a **simplified EEXI approximation** and list what correction factors are omitted. The EEXI result screen should note: "Simplified estimate — consult class society for regulatory submission."

### 2b. EEXI Applicability Threshold

- **What we say**: The brief says "vessels of 5,000 gross tonnage and above."
- **What the source says**: **EEXI applies to ships of 400 GT and above**, not 5,000 GT. The 5,000 GT threshold applies to **CII** only. EEXI and CII have different applicability:
  - EEXI: ≥ 400 GT (Regulation 23)
  - CII: ≥ 5,000 GT (Regulation 28)
- **Gap/Action**: **ERROR** — The brief conflates the two thresholds. The sentence should read "EEXI for vessels ≥ 400 GT and CII for vessels ≥ 5,000 GT." The plugin should handle both thresholds appropriately.

### 2c. Required EEXI Reference Values

- **What we say**: Not explicitly stated in glossary/brief.
- **What the source says**: Required EEXI is determined by applying a reduction factor to the EEDI Phase 2/3 reference line, which varies by ship type and size. The required EEXI reference values are generally aligned with EEDI tables from MEPC.231(65) and subsequent amendments.
- **Gap/Action**: If the plugin implements EEXI pass/fail, it needs a lookup table of Required EEXI reference values per ship type and size bracket. This is not currently documented anywhere in the project files.

---

## 3. Admiralty Coefficient Method

### 3a. Formula Verification

- **What we say**: ADR-0002 states "P = Δ^(2/3) × V³ / C"
- **What the source says**: The standard Admiralty Coefficient formula is: **C = Δ^(2/3) × V³ / P**, which rearranges to **P = Δ^(2/3) × V³ / C**. This is confirmed by multiple naval architecture references (Wärtsilä, navaltoolbox.com).
- **Gap/Action**: **Confirmed correct**. The formula and exponents are standard.

### 3b. Typical Admiralty Coefficient Values

- **What we say**: The glossary says "derived from sea trials" and "entered manually by the user."
- **What the source says**: Typical ranges from naval architecture references:

  | Ship Type | Typical C Value |
  |-----------|----------------|
  | Tankers / Bulk Carriers | 600 – 750 |
  | Reefers | 550 – 700 |
  | General Cargo Ships | 400 – 600 |
  | Feederships/Container | 350 – 500 |
  | Warships | ~150 |

- **Gap/Action**: The plugin should provide **default C values** per ship type in the Vessel Profile setup, with a note about the approximate nature. Users without sea trial data need guidance. Consider showing the typical range when the user selects a ship type.

### 3c. Known Limitations

- **What we say**: ADR-0002 lists: "calm water conditions, constant hull resistance, does not account for weather, sea state, hull fouling, or auxiliary engine consumption."
- **What the source says**: Additional limitations from engineering references include: no accounting for propeller-hull interaction, wave-making resistance, viscous effects, or bulbous bow effectiveness. The method also assumes geometric similarity with the reference vessel, and accuracy degrades significantly at speeds far from the design speed (high Froude number differences).
- **Gap/Action**: ADR-0002's limitations list is good but could add "speed regime sensitivity" — the Admiralty method is less accurate at speeds significantly different from design speed. This is relevant because vessels slow-steaming for CII compliance will be operating far below design speed, which is exactly when the method is least accurate. Consider documenting expected accuracy bounds (±15–25% is typical for this method).

---

## 4. Emission Factors (C_F)

### 4a. C_F Values Verification

- **What we say**: CONTEXT.md says "HFO (3.114)" and states C_F is "t-CO₂/t-fuel."
- **What the source says**: Per MEPC.364(79), the values are:

  | Fuel Type | C_F (t-CO₂/t-fuel) |
  |-----------|-------------------|
  | HFO | **3.114** ✓ |
  | MDO/Diesel/Gas Oil | **3.206** ✓ |
  | LNG | **2.750** ✓ |
  | Methanol | **1.375** ✓ |
  | LFO (Light Fuel Oil) | **3.151** |
  | LPG (Propane) | **3.000** |
  | LPG (Butane) | **3.030** |
  | Ethane | **2.927** |
  | Ethanol | **1.913** |

- **Gap/Action**: The four values mentioned in the project scope (HFO, MDO, LNG, Methanol) are **all correct**. However, the glossary only mentions HFO as the default. The IMO recognizes **9 fuel types** total. Consider whether the plugin should support at minimum: HFO, LFO, MDO, LNG, and Methanol (the most common). LPG/Ethane/Ethanol are increasingly relevant for dual-fuel vessels. At minimum, document the omitted fuel types as a known limitation.

---

## 5. NMEA 0183 RMC Sentence

### 5a. RMC Format Verification

- **What we say**: CONTEXT.md says "$xxRMC. Contains SOG, position (lat/lon), and UTC timestamp."
- **What the source says**: The standard format is:
  ```
  $aaRMC,hhmmss.ss,A,ddmm.mmmm,N,dddmm.mmmm,E,SOG,COG,ddmmyy,mag_var,mag_dir,mode*checksum<CR><LF>
  ```
  Fields: (1) UTC time, (2) Status (A=valid, V=void), (3-4) Latitude+dir, (5-6) Longitude+dir, (7) SOG in knots, (8) COG in degrees true, (9) Date, (10-11) Magnetic variation, (12) Mode indicator.
- **Gap/Action**: **Confirmed correct** — RMC does contain SOG, position, and timestamp. However, the glossary should note:
  1. Field 2 (Status) must be checked — `V` means no fix and data should be discarded
  2. The Mode indicator (field 12) was added in NMEA 2.3; older devices may not include it
  3. SOG field can be empty/null when no fix is available

### 5b. Talker ID Prefixes

- **What we say**: The glossary says "$xxRMC" using "xx" as a placeholder.
- **What the source says**: Valid talker IDs include:
  - **GP** — GPS
  - **GN** — Multi-constellation GNSS
  - **GL** — GLONASS
  - **GA** — Galileo
  - **GB** — BeiDou
- **Gap/Action**: The plugin's NMEA parser must accept all five prefixes (GP, GN, GL, GA, GB) before "RMC". Using a regex like `^\$[A-Z]{2}RMC,` is safest. This should be documented in the glossary or implementation notes.

### 5c. SOG Field Precision

- **What we say**: Not explicitly stated.
- **What the source says**: SOG is in knots with 0–3 decimal places typically. Precision varies by receiver. The field can also be **empty** (null) when no fix is available.
- **Gap/Action**: The parser must handle:
  1. Variable decimal precision (0–3 decimal places)
  2. Empty/null SOG field (treat as no data, not zero)
  3. The `V` status flag indicating navigation receiver warning

---

## 6. OpenCPN Plugin API

### 6a. NMEA Callback

- **What we say**: The brief mentions "NMEA 0183 sentence parsing" but doesn't specify the API method.
- **What the source says**: The correct callback is `SetNMEASentence`:
  ```cpp
  virtual void SetNMEASentence(const wxString &sentence);
  ```
  The plugin must register interest by including `WANTS_NMEA_SENTENCES` in its capability flag bitmask (`m_cap_flag`). OpenCPN's `PlugInManager` checks this flag and dispatches sentences to the plugin.
- **Gap/Action**: Add `SetNMEASentence` and `WANTS_NMEA_SENTENCES` to the domain glossary or an implementation reference document.

### 6b. Plugin API Version

- **What we say**: Not stated in the project files.
- **What the source says**: The API is defined in `ocpn_plugin.h`. Current versions are in the API 1.18+ range. The API is stable but evolving.
- **Gap/Action**: The project should specify a minimum API version target. Recommend targeting API 1.16+ for broad compatibility with OpenCPN 5.x releases.

### 6c. Plugin Template

- **What we say**: Not stated.
- **What the source says**: The recommended templates are:
  - **Testplugin** (https://github.com/OpenCPN/Testplugin_pi) — baseline template
  - **ShipDriver** (https://github.com/Rasbats/shipdriver_pi) — recommended for complex plugins with CI/CD
  Both use CMake and the managed plugin workflow with `update-templates` script.
- **Gap/Action**: The project should document which template it's based on. ShipDriver is the better choice for this project's complexity.

### 6d. Settings Persistence

- **What we say**: ADR-0003 says "wxFileConfig for settings."
- **What the source says**: `GetpPrivateApplicationDataLocation()` is indeed the recommended API function for plugin-private data storage paths. `wxFileConfig` stored in opencpn.conf/registry is the established convention for plugin preferences.
- **Gap/Action**: **Confirmed correct**. The ADR's approach aligns with OpenCPN conventions.

---

## 7. Missing Domain Concepts

### 7a. SEEMP (Ship Energy Efficiency Management Plan)

- **What we say**: The brief mentions "SEEMP Part III" but there is no glossary entry.
- **What the source says**: SEEMP has three parts:
  - **Part I**: General energy efficiency management (mandatory for all ships ≥ 400 GT)
  - **Part II**: DCS methodology — how the ship collects fuel/distance data
  - **Part III**: CII operational plan — required for ships ≥ 5,000 GT, must include required CII for next 3 years, implementation plan, and corrective action plan if rated D (3 years) or E (1 year)
- **Gap/Action**: **Add SEEMP to the glossary**. While the plugin doesn't generate SEEMP documents, users need to understand that the plugin's CII output feeds into their SEEMP Part III obligations.

### 7b. DCS (Data Collection System)

- **What we say**: Brief references "DCS (Data Collection System)" but no glossary entry.
- **What the source says**: The IMO DCS (MARPOL Annex VI, Regulation 27) requires ships ≥ 5,000 GT to collect and report annual fuel consumption data since 2019. This is the mandatory reporting mechanism whose data forms the basis for CII calculations.
- **Gap/Action**: **Add DCS to the glossary**. The plugin could potentially help users prepare DCS submissions by exporting annual fuel/distance summaries.

### 7c. Correction Factors (MEPC.355(78))

- **What we say**: Not mentioned in glossary or brief.
- **What the source says**: MEPC.355(78) defines correction factors and voyage adjustments:
  - **Voyage Exclusions**: Sailing in ice, emergency deviations, rescue operations
  - **Shuttle Tanker correction**: AF_Tanker,STS for STS operations
  - **Cargo heating**: Fuel for dedicated cargo heating may be adjustable
  - **Reefer containers**: Electrical consumption for refrigerated containers
  - All adjustments require logbook evidence and verification
- **Gap/Action**: **Add a glossary entry for "Correction Factors"** (MEPC.355(78)) even if the plugin doesn't implement them in v1. Document as a known limitation: "v1 does not apply IMO correction factors for ice navigation, cargo heating, or STS operations."

### 7d. Voyage Adjustment Factors

- **What we say**: ADR-0004 excludes data below 1 knot SOG as a plugin-level decision.
- **What the source says**: The IMO framework in MEPC.355(78) allows **excluding specific voyage segments** (ice navigation, emergency deviations) from CII calculations, with both fuel AND distance removed. This is different from the plugin's 1-knot threshold, which is an estimation model limitation rather than an IMO-defined exclusion.
- **Gap/Action**: ADR-0004's 1-knot threshold is a reasonable engineering decision but should be clearly distinguished from IMO voyage adjustments. The glossary could add: "This plugin-level threshold is distinct from the IMO's voyage adjustment provisions under MEPC.355(78), which allow excluding specific operational situations."

### 7e. Gross Tonnage (GT)

- **What we say**: GT is not in the glossary.
- **What the source says**: GT is needed for: (1) determining CII/EEXI applicability thresholds, (2) as the capacity metric for cgDIST calculations for cruise/ro-ro ship types.
- **Gap/Action**: **Add GT to the glossary** if supporting cruise/ro-ro ship types.

---

## 8. Scope & Applicability

### 8a. CII Applicability Threshold

- **What we say**: Brief says "vessels of 5,000 gross tonnage and above."
- **What the source says**: CII (Regulation 28) applies to ships of **5,000 GT and above** engaged in international voyages. This is correct for CII.
- **Gap/Action**: **CII threshold is correct** (5,000 GT). But the brief conflates this with EEXI — see finding 2b above.

### 8b. Exemptions

- **What we say**: Not mentioned.
- **What the source says**: MARPOL Annex VI exempts certain vessels:
  - Ships solely engaged in domestic voyages (unless flag state opts in)
  - Ships with non-conventional propulsion (e.g., diesel-electric, turbine, hybrid) may have different calculation methods but are **not exempt** — they just use different formula parameters
  - Ships built for specific purposes (e.g., floating storage units) may have class-specific considerations
- **Gap/Action**: The plugin should note that CII ratings are for **international voyages** and that domestic-only vessels may not be required to comply. However, for an awareness tool, this distinction may not matter operationally.

---

## Summary: Priority Actions

### Critical Errors to Fix
1. **Reduction factor is NOT 2%** — Z starts at 5% in 2023, increments ~2% per year
2. **"a, b, c" should be "a, c"** — there is no `b` coefficient in the CII reference line
3. **EEXI threshold is 400 GT, not 5,000 GT** — the brief conflates EEXI and CII thresholds

### Missing Glossary Entries to Add
4. **cgDIST** — alternative CII metric for cruise/ro-ro ships using GT
5. **SEEMP** — referenced in brief but no glossary entry
6. **DCS** — referenced in brief but no glossary entry
7. **GT (Gross Tonnage)** — needed for applicability thresholds and cgDIST
8. **Correction Factors / Voyage Adjustments** — MEPC.355(78) provisions

### Data Tables Needed for Implementation
9. **Complete CII reference line coefficients** (a, c) for all 14 ship types
10. **Complete d-vector table** (exp(d1)–exp(d4)) for all 14 ship types
11. **Full reduction factor table** (Z% for 2023–2030)
12. **Expanded emission factor table** (at least HFO, LFO, MDO, LNG, Methanol)
13. **Default Admiralty Coefficient values** per ship type (guidance for users)
14. **Required EEXI reference values** per ship type/size (if EEXI pass/fail is implemented)

### Documentation Improvements
15. Document that simplified EEXI formula omits correction factors (f_w, f_j, f_i, f_c, f_l, f_eff)
16. Note accuracy limitations of Admiralty method at slow-steaming speeds
17. NMEA parser must handle: all talker IDs (GP/GN/GL/GA/GB), `V` status, empty SOG fields
18. Specify target OpenCPN API version (recommend 1.16+)
19. Document chosen plugin template (recommend ShipDriver)
20. Distinguish plugin's 1-knot threshold from IMO voyage adjustments (MEPC.355(78))
