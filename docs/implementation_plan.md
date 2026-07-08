# Implementation Plan: EEXI-CII OpenCPN Plugin

## Goal

Build the `eexi_cii_pi` OpenCPN plugin from scratch, implementing all 35 user stories from the [PRD](file:///C:/Users/muigu/Documents/Projects/EEXI-CII/docs/PRD.md). The plugin calculates EEXI compliance and provides real-time CII monitoring for the Own-Ship using live NMEA 0183 data and the Admiralty Coefficient fuel estimation method.

All domain terms follow the [CONTEXT.md](file:///C:/Users/muigu/Documents/Projects/EEXI-CII/CONTEXT.md) glossary. Architectural decisions are recorded in [docs/adr/](file:///C:/Users/muigu/Documents/Projects/EEXI-CII/docs/adr).

## User Review Required

> [!IMPORTANT]
> **Development environment**: This plan assumes you have the following installed on your Windows machine:
> - Visual Studio 2022 (or Build Tools) with C++ desktop workload
> - CMake 3.15+
> - Git
> - A working OpenCPN 5.x installation (for testing the built plugin)
>
> Please confirm these are available, or let me know what's missing.

> [!WARNING]
> **OpenCPN plugin template**: The first implementation slice bootstraps a standalone pure C++ domain/test harness so calculations can be verified without OpenCPN installed. The OpenCPN plugin scaffold should then be based on the ShipDriver template or an existing OpenCPN managed-plugin environment.

## Open Questions

- Confirm the local OpenCPN managed-plugin template before packaging the DLL.
- Source-tag any CII reduction factors for 2027 onward before implementing future-year scenario planning.
- Complete the Required EEXI reference table before treating EEXI pass/fail as more than a simplified academic estimate.

---

## Proposed Changes

### Phase 0: Project Scaffold

Set up a buildable calculation/test harness first, then add the OpenCPN plugin shell.

#### [NEW] Standalone CMake domain harness

The first committed slice builds without wxWidgets or OpenCPN:

- `src/domain/` contains pure C++ calculation code.
- `tests/` contains self-contained CTest executables.
- OpenCPN-specific source files are added after the calculation seam is green.

This keeps the hardest math testable on machines that do not yet have Visual Studio/OpenCPN plugin dependencies installed.

#### [NEW] CMakeLists.txt
Standard OpenCPN managed plugin CMake — cloned from ShipDriver template, modified for our plugin name.

#### [NEW] Plugin.cmake
Plugin-specific configuration:

```cmake
# Plugin metadata
set(PKG_NAME "eexi_cii_pi")
set(PKG_VERSION "0.1.0")
set(PKG_PRERELEASE "alpha")
set(PKG_NAME_SUFFIX "")

set(PKG_SUMMARY "Real-time EEXI and CII emissions monitoring")
set(PKG_DESCRIPTION "Calculates EEXI compliance status and running CII rating
for the own-ship using live NMEA 0183 data and the Admiralty Coefficient
fuel estimation method.")

set(PKG_AUTHOR "Farid Nahadi")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE "https://github.com/farid-nahadi/eexi-cii-pi")
set(PKG_INFO_URL "https://github.com/farid-nahadi/eexi-cii-pi")

# Source files (populated incrementally per phase)
set(SRC
    src/eexi_cii_pi.cpp
)

set(HDR
    src/eexi_cii_pi.h
)
```

#### [NEW] src/eexi_cii_pi.h
Minimal plugin entry point — just enough to load:

```cpp
#ifndef _EEXI_CII_PI_H_
#define _EEXI_CII_PI_H_

#include "ocpn_plugin.h"

class eexi_cii_pi : public opencpn_plugin_118 {
public:
    eexi_cii_pi(void* ppimgr);
    ~eexi_cii_pi();

    // Required overrides
    int Init() override;
    bool DeInit() override;
    int GetAPIVersionMajor() override;
    int GetAPIVersionMinor() override;
    int GetPlugInVersionMajor() override;
    int GetPlugInVersionMinor() override;
    wxBitmap* GetPlugInBitmap() override;
    wxString GetCommonName() override;
    wxString GetShortDescription() override;
    wxString GetLongDescription() override;

    // NMEA callback (wired in Phase 3)
    void SetNMEASentence(wxString& sentence) override;

    // Toolbar
    int GetToolbarToolCount() override;
    void OnToolbarToolCallback(int id) override;

private:
    int m_toolbar_id;
    bool m_dashboard_visible;
};

#endif
```

#### [NEW] src/eexi_cii_pi.cpp
Skeleton implementation returning plugin metadata and registering `WANTS_NMEA_SENTENCES`.

#### [NEW] opencpn-libs/ (git submodule)

```bash
git submodule add https://github.com/OpenCPN/opencpn-libs.git
```

**Verification gate:**
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```
Plugin `.dll` must load in OpenCPN without crashing.

---

### Phase 1: Domain Layer (Pure C++, Zero Dependencies)

All calculation logic, fully unit-tested, with no wxWidgets or OpenCPN dependency. This is the primary testing seam.

#### [NEW] src/domain/vessel_profile.h
Data model — structs and enums only, no logic:

```cpp
#ifndef _VESSEL_PROFILE_H_
#define _VESSEL_PROFILE_H_

#include <string>

enum class ShipType {
    BULK_CARRIER,
    GAS_CARRIER_GTE_65K,    // ≥ 65,000 DWT
    GAS_CARRIER_LT_65K,     // < 65,000 DWT
    TANKER,
    CONTAINER_SHIP,
    GENERAL_CARGO,
    REFRIGERATED_CARGO,
    COMBINATION_CARRIER,
    LNG_CARRIER_GTE_100K,   // ≥ 100,000 DWT
    LNG_CARRIER_LT_100K,    // < 100,000 DWT
    RORO_VEHICLE_CARRIER,   // cgDIST types below
    RORO_CARGO,
    RORO_PASSENGER,
    CRUISE_PASSENGER,
    COUNT
};

enum class FuelType {
    HFO,    // 3.114
    LFO,    // 3.151
    MDO,    // 3.206
    LNG,    // 2.750
    METHANOL // 1.375
};

// Returns true if this ship type uses cgDIST (GT-based) instead of AER (DWT-based)
bool uses_cgdist(ShipType type);

// Returns the C_F value for a fuel type
double emission_factor(FuelType fuel);

struct VesselProfile {
    std::string name;
    std::string imo_number;
    double gt = 0.0;
    ShipType ship_type = ShipType::BULK_CARRIER;
    double dwt = 0.0;              // For AER types
    double admiralty_coefficient = 0.0;
    double sfoc = 190.0;           // g/kWh default
    FuelType fuel_type = FuelType::HFO;
    double displacement = 0.0;     // Current, updated per voyage

    // EEXI inputs
    double installed_power_mco = 0.0; // kW
    double reference_speed = 0.0;     // knots

    // Derived: capacity metric for CII formula
    double cii_capacity() const;   // Returns DWT or GT based on ship type
};

#endif
```

#### [NEW] src/domain/cii_reference.h / cii_reference.cpp
IMO lookup tables — modeled ship type entries and size brackets:

```cpp
#ifndef _CII_REFERENCE_H_
#define _CII_REFERENCE_H_

#include "vessel_profile.h"

struct CIICoefficients {
    double a;       // Reference line: CII_ref = a * capacity^(-c)
    double c;
    double d1, d2, d3, d4;  // Rating boundary exp-vectors
};

// Lookup coefficients for a ship type and capacity bracket
CIICoefficients cii_coefficients(ShipType type, double capacity);

// Reference-line capacity, including IMO caps/floors such as 279,000 DWT for large bulk carriers
double cii_reference_capacity(ShipType type, double capacity);

// Reduction factor Z% for a given year
double reduction_factor_z(int year);

// Reference CII value for a ship type, capacity, and year
double required_cii(ShipType type, double capacity, int year);

// Rating band boundaries for a ship type, capacity, and year
struct RatingBoundaries {
    double ab;  // AER <= ab → A
    double bc;  // AER <= bc → B
    double cd;  // AER <= cd → C
    double de;  // AER <= de → D, else E
};

RatingBoundaries rating_boundaries(ShipType type, double capacity, int year);

// Determine CII rating (A-E) from attained AER/cgDIST
char cii_rating(double attained, ShipType type, double capacity, int year);

#endif
```

Key data (from MEPC.353(78) and MEPC.354(78)):

```cpp
// Reduction factor table (MEPC.338(76), confirmed values)
static const std::map<int, double> Z_TABLE = {
    {2023, 5.0},  {2024, 7.0},   {2025, 9.0},   {2026, 11.0}
};

// Reference line coefficients per ship type/bracket
// CII_ref = a * reference_capacity^(-c)
// Data must be copied from MEPC.353(78) and MEPC.354(78), not inferred from approximate text.
```

#### [NEW] src/domain/fuel_estimator.h / fuel_estimator.cpp
The core Admiralty formula — pure function:

```cpp
#ifndef _FUEL_ESTIMATOR_H_
#define _FUEL_ESTIMATOR_H_

#include "vessel_profile.h"

struct FuelEstimate {
    double shaft_power_kw;        // P = Δ^(2/3) × V³ / C
    double fuel_rate_tonnes_hr;   // P × SFOC / 1,000,000
    double co2_rate_tonnes_hr;    // fuel_rate × C_F
    bool   below_threshold;       // true if SOG < threshold
};

// Estimate fuel consumption from current SOG and vessel profile
// Returns zeroes if sog_knots < sog_threshold
FuelEstimate estimate_fuel(
    double sog_knots,
    const VesselProfile& profile,
    double sog_threshold = 1.0
);

#endif
```

#### [NEW] src/domain/aer_engine.h / aer_engine.cpp
AER/cgDIST computation and CII rating:

```cpp
#ifndef _AER_ENGINE_H_
#define _AER_ENGINE_H_

#include "vessel_profile.h"
#include "cii_reference.h"

struct AERResult {
    double aer;                    // grams CO₂ / (capacity × nm)
    char   rating;                 // 'A' through 'E'
    double required_cii;           // Reference value for comparison
    RatingBoundaries boundaries;   // All band boundaries
    bool   uses_cgdist;            // true if GT-based
};

// Compute AER (or cgDIST) and determine CII rating
// total_co2_tonnes is converted to grams internally
AERResult compute_aer(
    double total_co2_tonnes,
    double total_distance_nm,
    const VesselProfile& profile,
    int year
);

#endif
```

#### [NEW] src/domain/eexi_calc.h / eexi_calc.cpp
One-shot EEXI calculation:

```cpp
#ifndef _EEXI_CALC_H_
#define _EEXI_CALC_H_

#include "vessel_profile.h"

struct EEXIResult {
    double attained_eexi;   // Simplified: (P × C_F × SFOC) / (DWT × V_ref)
    double required_eexi;   // From lookup tables
    bool   compliant;       // attained <= required
    bool   is_simplified;   // Always true — flags the disclaimer
};

EEXIResult calculate_eexi(const VesselProfile& profile);

#endif
```

#### [NEW] tests/test_fuel_estimator.cpp

```cpp
TEST(FuelEstimator, BulkCarrierAtServiceSpeed) {
    VesselProfile p;
    p.displacement = 90000.0;
    p.admiralty_coefficient = 680.0;
    p.sfoc = 175.0;
    p.fuel_type = FuelType::HFO;

    auto est = estimate_fuel(12.5, p);

    // Hand-calculated: P = 90000^(2/3) x 12.5^3 / 680 = 5768.321606 kW
    EXPECT_NEAR(est.shaft_power_kw, 5768.321606, 1.0);
    EXPECT_FALSE(est.below_threshold);
    EXPECT_GT(est.co2_rate_tonnes_hr, 0.0);
}

TEST(FuelEstimator, BelowThresholdReturnsZero) {
    VesselProfile p;
    p.displacement = 90000.0;
    p.admiralty_coefficient = 680.0;

    auto est = estimate_fuel(0.5, p);  // Below 1.0 knot

    EXPECT_DOUBLE_EQ(est.shaft_power_kw, 0.0);
    EXPECT_DOUBLE_EQ(est.co2_rate_tonnes_hr, 0.0);
    EXPECT_TRUE(est.below_threshold);
}
```

#### [NEW] tests/test_aer_engine.cpp
#### [NEW] tests/test_cii_reference.cpp
#### [NEW] tests/test_eexi_calc.cpp

**Verification gate:**
```bash
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --target eexi_cii_tests
ctest --output-on-failure
```
All domain tests must pass. AER validation test must match hand-calculated spreadsheet value within 0.01%.

---

### Phase 2: Data Layer

NMEA parsing, distance calculation, state accumulation, and persistence.

#### [NEW] src/data/nmea_parser.h / nmea_parser.cpp

```cpp
#ifndef _NMEA_PARSER_H_
#define _NMEA_PARSER_H_

#include <string>

struct RMCData {
    bool valid = false;          // False if parse failed, void status, or bad checksum
    double utc_seconds = 0.0;    // Seconds since midnight
    double latitude = 0.0;       // Decimal degrees (+ = N, - = S)
    double longitude = 0.0;      // Decimal degrees (+ = E, - = W)
    double sog_knots = -1.0;     // -1 = field was empty
    double cog_degrees = 0.0;
    int day = 0, month = 0, year = 0;
};

// Parse a $xxRMC sentence. Accepts all talker IDs (GP, GN, GL, GA, GB).
// Returns valid=false for: bad checksum, void status (V), malformed input.
RMCData parse_rmc(const std::string& sentence);

// Validate NMEA checksum (XOR of bytes between $ and *)
bool validate_checksum(const std::string& sentence);

#endif
```

#### [NEW] src/data/haversine.h / haversine.cpp

```cpp
#ifndef _HAVERSINE_H_
#define _HAVERSINE_H_

// Distance in nautical miles between two positions (decimal degrees)
double haversine_nm(double lat1, double lon1, double lat2, double lon2);

#endif
```

#### [NEW] src/data/accumulator.h / accumulator.cpp
The stateful accumulation engine — second testing seam:

```cpp
#ifndef _ACCUMULATOR_H_
#define _ACCUMULATOR_H_

#include "nmea_parser.h"
#include "domain/fuel_estimator.h"
#include "domain/vessel_profile.h"
#include <string>
#include <vector>

struct VoyageState {
    std::string name;
    double displacement = 0.0;
    double distance_nm = 0.0;
    double co2_tonnes = 0.0;
    double elapsed_seconds = 0.0;
    int64_t start_timestamp = 0;
    bool active = false;
};

struct VoyageRecord {
    std::string name;           // Empty string = unassigned period
    int64_t start_timestamp = 0;
    int64_t end_timestamp = 0;
    double displacement = 0.0;
    double distance_nm = 0.0;
    double co2_tonnes = 0.0;
    double aer = 0.0;
    bool is_unassigned = false;
};

struct YTDState {
    int year = 0;
    double distance_nm = 0.0;
    double co2_tonnes = 0.0;
    // YTD Seed values (added once, included in totals)
    double seed_distance_nm = 0.0;
    double seed_co2_tonnes = 0.0;
};

class Accumulator {
public:
    // Voyage lifecycle
    void start_voyage(const std::string& name, double displacement);
    void end_voyage(const VesselProfile& profile, int year);
    bool has_active_voyage() const;

    // Core data ingestion
    // Returns true if the point was accumulated (not filtered)
    bool add_data_point(
        const RMCData& rmc,
        const FuelEstimate& estimate,
        double sog_threshold = 1.0
    );

    // YTD Seed
    void set_ytd_seed(double co2_tonnes, double distance_nm);

    // Year rollover — call periodically
    // Returns true if a rollover occurred
    bool check_year_rollover(int current_year, const VesselProfile& profile);

    // Accessors
    VoyageState current_voyage() const;
    YTDState year_to_date() const;
    const std::vector<VoyageRecord>& voyage_history() const;
    const std::vector<YTDState>& archived_years() const;

private:
    VoyageState m_current_voyage;
    VoyageState m_unassigned;     // Between-voyage accumulation
    YTDState m_ytd;
    RMCData m_last_fix;           // For haversine delta
    bool m_has_last_fix = false;
    std::vector<VoyageRecord> m_history;
    std::vector<YTDState> m_archived_years;
};

#endif
```

#### [NEW] src/data/persistence.h / persistence.cpp
JSON serialisation for Accumulator state:

```cpp
#ifndef _PERSISTENCE_H_
#define _PERSISTENCE_H_

#include "accumulator.h"
#include "domain/vessel_profile.h"
#include <string>

// JSON persistence for voyage data
void save_accumulator(const std::string& filepath, const Accumulator& acc);
void load_accumulator(const std::string& filepath, Accumulator& acc);

// wxFileConfig persistence for vessel profile (Phase 3 — uses wxString)
// Forward-declared here, implemented with wxWidgets dependency

#endif
```

#### [NEW] tests/test_nmea_parser.cpp
#### [NEW] tests/test_haversine.cpp
#### [NEW] tests/test_accumulator.cpp

The accumulator test is the most comprehensive — tests the full stateful lifecycle:

```cpp
TEST(Accumulator, VoyageStartEndCreatesRecord) {
    Accumulator acc;
    VesselProfile profile;
    profile.dwt = 80000;
    profile.ship_type = ShipType::BULK_CARRIER;

    acc.start_voyage("Test Voyage", 90000.0);

    // Feed 10 data points
    for (int i = 0; i < 10; i++) {
        RMCData rmc;
        rmc.valid = true;
        rmc.latitude = 1.0 + i * 0.1;
        rmc.longitude = 104.0;
        rmc.sog_knots = 12.0;

        FuelEstimate est;
        est.co2_rate_tonnes_hr = 1.5;
        est.below_threshold = false;

        acc.add_data_point(rmc, est);
    }

    acc.end_voyage(profile, 2026);

    ASSERT_EQ(acc.voyage_history().size(), 1);
    EXPECT_EQ(acc.voyage_history()[0].name, "Test Voyage");
    EXPECT_GT(acc.voyage_history()[0].distance_nm, 0.0);
    EXPECT_GT(acc.voyage_history()[0].co2_tonnes, 0.0);
    EXPECT_FALSE(acc.has_active_voyage());
}

TEST(Accumulator, UnassignedDataAccumulatesToYTD) {
    Accumulator acc;
    // No voyage started — data goes to unassigned pool
    RMCData rmc;
    rmc.valid = true;
    rmc.latitude = 1.0;
    rmc.longitude = 104.0;
    rmc.sog_knots = 12.0;

    FuelEstimate est;
    est.co2_rate_tonnes_hr = 1.5;
    est.below_threshold = false;

    acc.add_data_point(rmc, est);

    EXPECT_GT(acc.year_to_date().co2_tonnes, 0.0);
    EXPECT_FALSE(acc.has_active_voyage());
}

TEST(Accumulator, YearRolloverArchivesAndResets) {
    Accumulator acc;
    // ... populate with 2025 data ...

    VesselProfile profile;
    profile.dwt = 80000;
    profile.ship_type = ShipType::BULK_CARRIER;

    bool rolled = acc.check_year_rollover(2026, profile);

    EXPECT_TRUE(rolled);
    EXPECT_EQ(acc.year_to_date().year, 2026);
    EXPECT_DOUBLE_EQ(acc.year_to_date().co2_tonnes, 0.0);
    EXPECT_EQ(acc.archived_years().size(), 1);
    EXPECT_EQ(acc.archived_years()[0].year, 2025);
}
```

**Verification gate:**
```bash
cmake --build . --target eexi_cii_tests
ctest --output-on-failure
```
All data layer tests pass. Persistence round-trip test must save/load identical state.

---

### Phase 3: OpenCPN Integration

Wire domain and data layers into the plugin lifecycle.

#### [MODIFY] src/eexi_cii_pi.h
Add private members for the domain/data objects and the data flow chain:

```diff
 private:
     int m_toolbar_id;
     bool m_dashboard_visible;
+
+    // Domain
+    VesselProfile m_profile;
+
+    // Data
+    Accumulator m_accumulator;
+    RMCData m_last_rmc;
+
+    // UI (Phase 4)
+    DashboardPanel* m_dashboard_panel;
+
+    // Persistence paths
+    wxString m_data_dir;
+    wxString m_json_path;
+
+    // Auto-save timer
+    wxTimer m_autosave_timer;
+    void OnAutoSaveTimer(wxTimerEvent& event);
+
+    // NMEA → calculation → accumulation → UI pipeline
+    void ProcessNMEA(const std::string& sentence);
```

#### [MODIFY] src/eexi_cii_pi.cpp
Implement the full data flow in `SetNMEASentence()`:

```cpp
void eexi_cii_pi::SetNMEASentence(wxString& sentence) {
    ProcessNMEA(sentence.ToStdString());
}

void eexi_cii_pi::ProcessNMEA(const std::string& sentence) {
    // 1. Parse
    RMCData rmc = parse_rmc(sentence);
    if (!rmc.valid || rmc.sog_knots < 0) return;

    // 2. Estimate fuel
    FuelEstimate est = estimate_fuel(rmc.sog_knots, m_profile);

    // 3. Accumulate
    m_accumulator.add_data_point(rmc, est);

    // 4. Compute AER & rating
    auto ytd = m_accumulator.year_to_date();
    AERResult aer = compute_aer(
        ytd.co2_tonnes, ytd.distance_nm,
        m_profile, ytd.year
    );

    // 5. Update dashboard (Phase 4)
    if (m_dashboard_panel) {
        m_dashboard_panel->Update(rmc, est, aer,
            m_accumulator.current_voyage(), ytd);
    }
}
```

Implement `Init()`:

```cpp
int eexi_cii_pi::Init() {
    // Register for NMEA sentences
    m_cap_flag = WANTS_NMEA_SENTENCES;

    // Set up data directory
    m_data_dir = GetpPrivateApplicationDataLocation()
                 + wxFileName::GetPathSeparator() + "eexi_cii_pi";
    wxFileName::Mkdir(m_data_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    m_json_path = m_data_dir + wxFileName::GetPathSeparator() + "voyagedata.json";

    // Load vessel profile from wxFileConfig
    load_profile(GetOCPNConfigObject(), m_profile);

    // Load accumulator from JSON
    load_accumulator(m_json_path.ToStdString(), m_accumulator);

    // Check year rollover
    m_accumulator.check_year_rollover(
        wxDateTime::Now().GetYear(), m_profile);

    // Start auto-save timer (5 minutes = 300,000 ms)
    m_autosave_timer.Start(300000);

    // Add toolbar button
    m_toolbar_id = InsertPlugInTool(
        _T(""), GetPlugInBitmap(), GetPlugInBitmap(),
        wxITEM_CHECK, _("EEXI/CII Monitor"), _T(""), NULL,
        -1, -1, this);

    return (WANTS_NMEA_SENTENCES | WANTS_TOOLBAR_CALLBACK);
}
```

**Verification gate:** Build the full plugin, load in OpenCPN, verify:
- Plugin appears in plugin list
- Toolbar button appears
- NMEA data is received (add temporary logging in `SetNMEASentence`)
- Auto-save creates `voyagedata.json` in the data directory

---

### Phase 4: UI Layer (wxWidgets)

#### [NEW] src/ui/setup_dialog.h / setup_dialog.cpp
Tabbed wxDialog with three tabs:

**Tab 1 — Vessel Profile:**
- wxTextCtrl: Ship Name, IMO Number
- wxSpinCtrlDouble: GT, DWT, Admiralty Coefficient (with Ship Type default hint), SFOC (default 190.0), Displacement
- wxChoice: Ship Type entries/brackets, Fuel Type (5 items)
- On Ship Type change: show default Admiralty Coefficient range hint, toggle DWT/GT label

**Tab 2 — EEXI:**
- wxSpinCtrlDouble: Installed Power (MCO kW), Reference Speed (knots)
- Read-only wxStaticText: Attained EEXI, Required EEXI
- wxStaticBitmap: ✅/❌ compliance icon
- wxStaticText: "Simplified estimate — consult class society for regulatory submission"
- Auto-recalculates on any input change

**Tab 3 — Advanced:**
- wxSpinCtrlDouble: SOG Threshold (default 1.0)
- wxChoice: Target Year Override ("Auto" + 2023–2035)
- YTD Seed section: wxSpinCtrlDouble for prior CO₂ and prior distance

#### [NEW] src/ui/dashboard_panel.h / dashboard_panel.cpp
wxPanel docked in OpenCPN, implementing this layout:

```
┌──────────────────────────────────────────────────┐
│  MV Example Ship (IMO 1234567)  EEXI: ✅          │  Vessel Header
├──────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────────────┐     │
│  │ SOG           │  │ CO₂ Rate              │     │
│  │ 12.4 kts      │  │ 2.31 t/hr             │     │  Live Gauges
│  └──────────────┘  └──────────────────────┘     │
├──────────────────────────────────────────────────┤
│  CURRENT VOYAGE: "Rotterdam → Singapore"          │
│  Dist: 4,892 nm   CO₂: 1,247 t   AER: 3.19      │  Voyage Panel
│  [▶ Start Voyage]  [■ End Voyage]                 │
├──────────────────────────────────────────────────┤
│  YEAR TO DATE (2026)                               │
│  Dist: 48,210 nm   CO₂: 12,847 t                  │
│  Running AER: 3.33   CII Rating: ██ B ██          │  YTD Panel
│  Margin to C: 0.45 (AER < 3.78)                   │
├──────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────┐   │
│  │ AER trend chart (Rating Chart widget)     │   │  Rating Chart
│  └──────────────────────────────────────────┘   │
├──────────────────────────────────────────────────┤
│  [⚙ Settings]  [📥 Export CSV]  [📋 History]     │  Action Bar
└──────────────────────────────────────────────────┘
```

Key implementation:
- `Update(RMCData, FuelEstimate, AERResult, VoyageState, YTDState)` — called from the plugin's NMEA pipeline
- Colour-coded CII rating: A=green, B=lime, C=yellow, D=orange, E=red
- Error states: "No GPS signal" overlay when no NMEA for >60s (wxTimer watchdog), "Setup required" when no Vessel Profile
- Start/End Voyage buttons open a small wxDialog for name + displacement

#### [NEW] src/ui/rating_chart.h / rating_chart.cpp
Custom-drawn wxPanel using `wxGraphicsContext`:
- X axis: Jan–Dec (or month of installation to Dec for first year)
- Y axis: AER value
- Colour-coded horizontal band regions (A=green, B=lime, C=yellow, D=orange, E=red)
- Running AER plotted as a line with current position dot
- Rating boundary lines drawn as dashed horizontals
- Stores monthly AER snapshots for the trend line (sampled from YTD state on the 1st of each month, or from persistence)

**Verification gate:** Build, load in OpenCPN. With OpenCPN's GPS simulator running:
- Dashboard shows and updates SOG/CO₂ rate in real-time
- Start/End Voyage buttons work
- CII rating colour matches band
- Rating chart draws correctly
- Settings dialog opens and persists changes
- "No GPS signal" appears when simulator is stopped

---

### Phase 5: Export & Polish

#### [NEW] src/export/csv_exporter.h / csv_exporter.cpp

```cpp
#ifndef _CSV_EXPORTER_H_
#define _CSV_EXPORTER_H_
#include "data/accumulator.h"
#include "domain/vessel_profile.h"
#include <string>

// Build annual summary CSV text. UI/file layer writes it to disk.
std::string export_annual_csv(
    const YTDState& ytd,
    const VesselProfile& profile,
    const AERResult& aer_result
);

// Build per-voyage breakdown CSV text. UI/file layer writes it to disk.
std::string export_voyage_csv(const std::vector<VoyageRecord>& records);

#endif
```

Annual CSV format:
```csv
Year,Ship Name,IMO Number,Ship Type,DWT,Total Distance (nm),Total CO2 (tonnes),AER,CII Rating,Required CII
2026,MV Example Ship,1234567,Bulk Carrier,80000,48210,12847,3.33,B,3.78
```

Voyage CSV format:
```csv
Voyage,Start,End,Displacement,Distance (nm),CO2 (tonnes),AER,Type
Rotterdam-Singapore,2026-01-15T08:00:00,2026-02-12T14:30:00,92000,4892,1247,3.19,Named
Unassigned,2026-02-12T14:30:00,2026-02-15T09:00:00,92000,3.2,0.8,—,Unassigned
```

#### [MODIFY] src/ui/dashboard_panel.cpp
Wire Export CSV button to `wxFileDialog` -> `export_annual_csv()` / `export_voyage_csv()` -> file write.

#### [NEW] src/ui/history_dialog.h / history_dialog.cpp
wxDialog showing:
- wxListCtrl with archived years (year, distance, CO₂, AER, rating)
- wxListCtrl with voyage records for selected year
- Export button per year

**Verification gate:**
- Export CSV, open in Excel/LibreCalc — data is correct
- Year rollover: set system clock to Dec 31 23:59, wait, verify auto-archive
- History dialog shows archived years correctly

---

### Phase 6: Validation

End-to-end validation against known data.

#### [NEW] tests/test_data/sample_voyage.csv
Decoded voyage position data for a known vessel - columns: timestamp, lat, lon, sog.
AIS-derived data is acceptable for replay, distance, and rating-flow validation, but it does
not validate true fuel-consumption accuracy because AIS does not contain fuel-flow data.

#### [NEW] tests/test_validation.cpp
Integration test that feeds the sample voyage through the full calculation chain:

```cpp
TEST(Validation, BulkCarrierSampleVoyage) {
    // Load sample CSV data points
    auto data_points = load_test_csv("test_data/sample_voyage.csv");

    VesselProfile profile;
    profile.ship_type = ShipType::BULK_CARRIER;
    profile.dwt = 80000;
    profile.admiralty_coefficient = 680.0;
    profile.sfoc = 175.0;
    profile.fuel_type = FuelType::HFO;
    profile.displacement = 92000.0;

    Accumulator acc;
    acc.start_voyage("Validation", 92000.0);

    for (const auto& dp : data_points) {
        RMCData rmc;
        rmc.valid = true;
        rmc.latitude = dp.lat;
        rmc.longitude = dp.lon;
        rmc.sog_knots = dp.sog;

        auto est = estimate_fuel(rmc.sog_knots, profile);
        acc.add_data_point(rmc, est);
    }

    acc.end_voyage(profile, 2026);
    auto ytd = acc.year_to_date();
    auto result = compute_aer(ytd.co2_tonnes, ytd.distance_nm, profile, 2026);

    // Compare against hand-calculated spreadsheet values
    EXPECT_NEAR(ytd.distance_nm, EXPECTED_DISTANCE, EXPECTED_DISTANCE * 0.01);
    EXPECT_NEAR(result.aer, EXPECTED_AER, EXPECTED_AER * 0.01);
    EXPECT_EQ(result.rating, EXPECTED_RATING);
}
```

**Verification gate:**
```bash
cmake --build . --target eexi_cii_tests
ctest --output-on-failure
```
Validation test must match hand-calculated values within 1% tolerance.

---

## Verification Plan

### Automated Tests

```bash
# Build everything including tests
cd build
cmake .. -DBUILD_TESTS=ON -G "Visual Studio 17 2022"
cmake --build . --config Release

# Run all tests
ctest --output-on-failure --config Release

# Expected test count per phase:
# Phase 1: ~20 tests (fuel estimator, AER engine, CII reference, EEXI calc)
# Phase 2: ~15 tests (NMEA parser, haversine, accumulator)
# Phase 6: ~3 tests (validation/integration)
# Total: ~38 tests
```

### Manual Verification

After each phase, load the plugin in OpenCPN and verify:

| Phase | Manual check |
|-------|-------------|
| 0 | Plugin loads, toolbar icon appears, no crash |
| 1 | N/A — domain layer has no UI (tests only) |
| 2 | N/A — data layer has no UI (tests only) |
| 3 | NMEA sentences are received (check OpenCPN message log), auto-save creates JSON |
| 4 | Dashboard shows live data from GPS simulator, start/end voyage works, CII rating colour correct, settings persist |
| 5 | CSV export opens in spreadsheet correctly, history dialog shows data |
| 6 | Replay `.nmea` file through OpenCPN, compare dashboard AER against spreadsheet |

### Final End-to-End Test

1. Configure a Vessel Profile for a bulk carrier (DWT 80,000, C=680, SFOC=175)
2. Start OpenCPN GPS simulator
3. Start a Voyage with displacement 92,000 t
4. Let it run for 10 minutes
5. End Voyage — verify Voyage Record appears in history
6. Export CSV — verify data is correct
7. Close and reopen OpenCPN — verify all data persists
8. Verify CII rating matches manual calculation
