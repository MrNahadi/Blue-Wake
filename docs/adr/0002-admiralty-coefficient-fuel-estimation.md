# Admiralty Coefficient method for fuel estimation

Fuel consumption is estimated using the Admiralty Coefficient method rather than direct fuel flow measurement. The Admiralty formula (P = Δ^(2/3) × V³ / C) estimates shaft power from displacement and speed, then derives fuel consumption via SFOC. This avoids requiring additional hardware (fuel flow sensors) beyond the GPS/AIS equipment already present. The trade-off is accuracy: the method assumes calm water conditions, constant hull resistance, and does not account for weather, sea state, hull fouling, or auxiliary engine consumption. For the plugin's purpose — providing operational awareness of CII trends — this approximation is sufficient. Operators with fuel flow sensors should use their flag state's commercial monitoring platform for regulatory submission; this plugin provides an accessible, zero-hardware-cost alternative.

Considered alternatives:
- **Direct fuel flow sensor integration**: More accurate but requires NMEA 2000 or proprietary sensor protocols and additional hardware cost. Out of scope.
- **Holtrop-Mennen resistance estimation**: More physically rigorous but requires detailed hull geometry data that most operators don't have readily available.
