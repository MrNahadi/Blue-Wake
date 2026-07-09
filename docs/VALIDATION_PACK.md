# Validation Pack

The validation pack supports plugin-manager release confidence. It does not
establish regulatory accuracy.

## Automated Replay Fixture

`tests/fixtures/replay_validation_route.nmea` is a deterministic synthetic RMC
track:

- 4 valid RMC fixes
- 3 one-hour underway intervals
- 180.121382199 nm expected distance
- 8.343361800 tonnes expected CO2 with the test bulk-carrier profile
- 0.579009672 expected AER
- Expected CII rating: `A`

`eexi_cii_replay_validation_tests` runs this fixture through `PluginCore`, the
same parser/fuel/accumulator/AER path used by the OpenCPN adapter.

## Dataset Provenance Rules

Each replay dataset added after this must document:

- Source and license
- Whether it is synthetic, public AIS-derived, or real vessel data
- Expected distance, CO2, AER or cgDIST, and rating
- Tolerance and reason for that tolerance
- Known limitations, especially whether fuel consumption is measured or only estimated

## Validation Boundary

Public AIS-derived data can validate replay mechanics, distance accumulation,
year rollover, rating flow, and UI/export behavior. It cannot validate real fuel
consumption accuracy. Full regulatory validation requires real vessel fuel and
activity data plus external maritime/class-society review.
