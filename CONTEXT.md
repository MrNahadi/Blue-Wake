# EEXI-CII Plugin

A real-time emissions compliance plugin for OpenCPN that calculates EEXI compliance status and running CII rating for the own-ship, using live NMEA 0183 data.

## Language

### Ship & Regulatory

**Own-Ship**:
The single vessel running OpenCPN with this plugin installed. The only vessel the plugin tracks.
_Avoid_: target, AIS target, tracked vessel

**Ship Type**:
One of the IMO-defined CII ship type entries, with size brackets where the IMO reference table requires them. Each entry has CII reference line coefficients (a, c), rating boundaries, and in some cases a reference-capacity cap or floor. Selected by the user during vessel profile setup. Ship types using AER (DWT-based): bulk carrier, gas carrier (≥/< 65k DWT), tanker, container ship, general cargo (≥/< 20k DWT), refrigerated cargo carrier, combination carrier, LNG carrier (≥100k DWT, 65k-100k DWT, <65k DWT). Ship types using cgDIST (GT-based): ro-ro cargo ship (vehicle carrier, with GT brackets), ro-ro cargo ship, ro-ro passenger ship, cruise passenger ship.
_Avoid_: vessel category, class

**GT (Gross Tonnage)**:
A dimensionless measure of a vessel's overall internal volume, used to determine regulatory applicability thresholds (EEXI ≥ 400 GT, CII ≥ 5,000 GT) and as the capacity metric in the cgDIST formula for cruise and ro-ro ship types.
_Avoid_: gross tons, GRT (different measurement)

**DWT (Deadweight Tonnage)**:
The vessel's maximum cargo-carrying capacity in tonnes — a fixed property of the vessel (the difference between loaded displacement and lightship displacement). Entered once during vessel setup. Used as the denominator in the AER formula. Does not change between voyages.
_Avoid_: displacement (different physical quantity), tonnage (ambiguous)

**EEXI (Energy Efficiency Existing Ship Index)**:
A one-time, design-based energy efficiency metric calculated from the ship's technical specifications. Applicable to vessels ≥ 400 GT. Computed once during plugin setup using a simplified formula; result is pass or fail against the Required EEXI for the ship's type and size. The plugin uses a simplified approximation — the full IMO formula (MEPC.333(76)) includes correction factors (f_w, f_j, f_i, f_c, f_l, f_eff) not implemented here.
_Avoid_: efficiency score, design index

**CII (Carbon Intensity Indicator)**:
An operational metric that rates how efficiently the own-ship transports cargo relative to CO₂ emitted, accumulated over a calendar year. Applicable to vessels ≥ 5,000 GT. Rated A (best) through E (worst).
_Avoid_: emissions score, carbon rating

**AER (Annual Efficiency Ratio)**:
The CII calculation method for DWT-based ship types. Ratio of total CO₂ emissions to the product of DWT and distance sailed (grams CO₂ per tonne-nautical-mile). The running AER is the continuously updated value; the final AER determines the year-end CII rating.
_Avoid_: efficiency ratio (too vague)

**cgDIST**:
The CII calculation method for GT-based ship types (cruise passenger, ro-ro cargo, ro-ro passenger). Same structure as AER but uses GT instead of DWT as the capacity metric: cgDIST = Σ(CO₂ grams) / Σ(GT × distance in nm).
_Avoid_: AER (different capacity metric)

**Required CII**:
The reference value against which the own-ship's attained CII (AER or cgDIST) is compared to determine the A–E rating. Calculated as: Required CII = CII_ref × (1 − Z/100), where CII_ref = a × Capacity^(−c) per MEPC.353(78), and Z is the Reduction Factor for the current year.
_Avoid_: target CII, CII limit

**Reduction Factor (Z)**:
The cumulative annual tightening of the Required CII, expressed as a percentage relative to the 2019 baseline. Not a fixed 2% - the values are defined per year in a lookup table (e.g. Z=5% in 2023, 7% in 2024, 9% in 2025, 11% in 2026). Defined by MEPC.338(76), which marks 2027-2030 factors for further development after the short-term-measure review; future-year values must therefore be source-tagged before implementation. Auto-determined from the current system year, with optional manual year override for scenario planning.
_Avoid_: correction factor, annual adjustment, "2% per year" (inaccurate)

**Rating Band**:
The A–E classification boundaries for CII, defined by IMO rating boundary vectors (d1–d4) specific to the own-ship's ship type per MEPC.354(78). The running AER or cgDIST is compared against these boundaries to produce the current CII rating.
_Avoid_: rating level, grade

**SEEMP (Ship Energy Efficiency Management Plan)**:
A mandatory three-part document. Part III (required for ships ≥ 5,000 GT) must include the Required CII for the next three years, an implementation plan, and a corrective action plan if rated D for three consecutive years or E in any single year. The plugin's CII output feeds into the vessel's SEEMP Part III obligations but does not generate the SEEMP document itself.
_Avoid_: energy plan, efficiency plan

**DCS (Data Collection System)**:
The IMO mandatory reporting mechanism (MARPOL Annex VI, Regulation 27) requiring ships ≥ 5,000 GT to collect and report annual fuel consumption data. The plugin's CSV export can help operators prepare DCS submissions.
_Avoid_: reporting system, data system

**Correction Factors**:
IMO-defined adjustments (MEPC.355(78)) that allow excluding specific voyage segments from CII calculations — including ice navigation, emergency deviations, and rescue operations — with both fuel and distance removed. Not implemented in plugin v1. Distinct from the plugin's SOG threshold, which is an estimation model limitation.
_Avoid_: voyage adjustments (ambiguous with plugin-level threshold)

### Estimation & Calculation

**Admiralty Coefficient (C)**:
A vessel-specific constant derived from sea trials that relates speed, displacement, and power. Used to estimate fuel consumption from SOG when direct fuel flow measurement is unavailable. Entered manually by the user; default guidance values are shown per ship type (e.g. 600–750 for bulk carriers/tankers, 350–500 for container ships). Accuracy degrades at speeds significantly different from design speed (relevant for slow-steaming vessels).
_Avoid_: propulsion constant, ship coefficient

**Displacement (Δ)**:
The vessel's actual total mass including hull, cargo, fuel, water, and stores, in tonnes. Varies with loading condition — updated by the user when starting a new Voyage. Carries forward between voyages until next updated; estimation accuracy degrades if loading condition has changed materially since the last update. Used in the Admiralty Coefficient formula alongside SOG to estimate shaft power and fuel consumption. Distinct from DWT, which is fixed.
_Avoid_: weight, loading, DWT (different concept)

**SFOC (Specific Fuel Oil Consumption)**:
The mass of fuel consumed per unit of power per hour (g/kWh). Defaults to 190 g/kWh (typical slow-speed diesel) but overridable by the user in the Vessel Profile.
_Avoid_: fuel rate, consumption rate

**Emission Factor (C_F)**:
The CO₂ produced per tonne of fuel burned (t-CO₂/t-fuel). Determined by the fuel type selected in the Vessel Profile. IMO-defined values per MEPC.364(79): HFO 3.114 (default), LFO 3.151, MDO 3.206, LNG 2.750, Methanol 1.375. Additional fuel types (LPG, Ethane, Ethanol) exist but are not supported in v1.
_Avoid_: carbon factor, CO₂ multiplier

**Instantaneous CO₂ Rate**:
The estimated CO₂ emission rate at the current moment, derived from current SOG and the Admiralty Coefficient model. Displayed as a live "speedometer-style" indicator.
_Avoid_: current emissions, emission speed

**Running AER**:
The cumulative AER (or cgDIST) calculated from all data accumulated since January 1st of the current calendar year. Updated continuously as new NMEA data arrives. If the plugin is installed mid-year, the user may enter a YTD Seed to include prior operational data.
_Avoid_: live AER, real-time AER

**YTD Seed**:
Optional prior year-to-date totals (CO₂ in tonnes, distance in nautical miles) that the user enters when installing the plugin mid-year, so the Running AER reflects the full calendar year rather than only the period since installation.
_Avoid_: initial values, starting data

### Data & Integration

**NMEA 0183 Sentence**:
A standardised text-based data sentence from navigation instruments. The plugin's specific input requirement is defined under RMC Sentence.
_Avoid_: NMEA string, GPS string, data packet

**RMC Sentence**:
The primary NMEA 0183 sentence parsed by the plugin ($xxRMC, where xx is any valid talker ID: GP, GN, GL, GA, GB). Contains SOG, position (lat/lon), and UTC timestamp in a single sentence — sufficient as the sole data input for the entire calculation chain. The parser must check the status field (A=valid, V=void) and handle empty SOG fields.
_Avoid_: GPS sentence, position sentence

**SOG (Speed Over Ground)**:
The vessel's speed relative to the earth's surface, in knots, extracted from NMEA sentences. The primary speed input for fuel estimation.
_Avoid_: vessel speed (ambiguous — could mean speed through water)

### Voyage & State

**Vessel Profile**:
The stored configuration for the own-ship: ship name, IMO number, GT, ship type, DWT (or GT for cgDIST ship types), Admiralty Coefficient, SFOC, fuel type, and EEXI input parameters. Persists across OpenCPN sessions.
_Avoid_: ship config, vessel settings

**Voyage**:
An optional labelling period the user applies to a stretch of continuous operation, typically corresponding to a loading condition (e.g. "Rotterdam → Singapore, laden"). Started and ended manually via dashboard controls. Does not control accumulation — the Accumulator always runs regardless of whether a Voyage is active. Displacement is assumed constant within a Voyage and carries forward after it ends.
_Avoid_: trip, passage, leg

**Accumulator**:
The running totals that feed the AER calculation — cumulative CO₂ emitted (tonnes) and cumulative distance sailed (nautical miles). Always active whenever valid NMEA data arrives and SOG ≥ threshold, regardless of whether a Voyage is active. Data is attributed to the current Voyage if one is active, and to an "unassigned" pool otherwise — both contribute to the year-to-date totals. Persists across sessions; auto-archived and reset on January 1st.
_Avoid_: running total, counter

**Voyage Record**:
A completed accumulation period's summary data: start/end timestamps, distance sailed, CO₂ emitted, displacement used, and computed AER. Covers both named Voyages and Unassigned periods (time between voyages). Stored permanently for audit trail and year-to-date aggregation. Unassigned periods are labelled distinctly so the user can see how much operational data fell outside of named voyages.
_Avoid_: voyage log (ambiguous with NMEA logs), trip summary

### Dashboard

**Live Gauge**:
A real-time indicator updated on every NMEA sentence (~1/sec): current SOG and instantaneous CO₂ rate.
_Avoid_: meter, dial
