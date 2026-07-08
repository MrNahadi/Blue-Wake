**Project Proposal**

**Development of a Real-Time EEXI and CII Emissions Monitoring Plugin
for OpenCPN**

  ------------------ ----------------------------------------------------
  **Student:**       Farid Nahadi

  **Student ID:**    ENM241-0143/2021

  **Supervisor:**    Dr Adenya

  **Programme:**     BSc. Marine Engineering

  **Academic Year:** 2025 / 2026

  **Date:**          June 2026
  ------------------ ----------------------------------------------------

## 1. Introduction

The international shipping industry is responsible for approximately
2.89% of global greenhouse gas (GHG) emissions, making it a significant
contributor to anthropogenic climate change (IMO, 2023). In recognition
of this, the International Maritime Organization (IMO) has adopted a
series of increasingly stringent technical and operational measures
under MARPOL Annex VI aimed at decarbonising maritime transport. Chief
among these are the Energy Efficiency Existing Ship Index (EEXI) and the
Carbon Intensity Indicator (CII), both of which became mandatory for
applicable vessels on 1 January 2023.

Despite the regulatory imperative, the maritime industry faces a
significant practical challenge: the tools available for real-time
emissions monitoring aboard vessels remain either prohibitively
expensive, proprietary, or inaccessible to smaller operators. Most
existing CII tracking solutions are cloud-based software-as-a-service
platforms requiring subscription fees, and none are integrated with
widely used open-source navigation software. This gap represents both a
technical and operational problem, particularly for operators of vessels
in developing maritime economies who may lack the resources to procure
commercial compliance solutions.

This project proposes the design and development of a functional
software plugin for OpenCPN --- a widely used, open-source chart plotter
and navigation application maintained under the GNU General Public
Licence v2 --- that calculates and displays the own-ship's EEXI
compliance status and running CII rating in real time, using live NMEA
0183 RMC sentences from the vessel's GPS/GNSS input. The plugin will be
implemented in C++ using the OpenCPN Plugin Application Programming
Interface (API) and the wxWidgets cross-platform graphical framework.

## 2. Background and Motivation

The EEXI is a design-based metric that quantifies the energy efficiency
of a ship relative to its installed propulsion power, deadweight
capacity, and service speed. Regulated under MARPOL Annex VI Regulation
23, each ship must demonstrate that its attained EEXI --- calculated
once --- is below a required EEXI threshold defined per ship type and
size category. The required EEXI is broadly aligned with the Energy
Efficiency Design Index (EEDI) Phase 2 standard applicable to new ships
as of 2023 (IMO, 2022).

The CII, regulated under MARPOL Annex VI Regulation 28, is an
operational metric that assesses how efficiently a vessel transports
cargo or passengers relative to the GHG emissions it produces over a
calendar year. The IMO accepts the Annual Efficiency Ratio (AER) as the
primary CII calculation method for most commercial vessel types. AER is
expressed as the ratio of total annual CO₂ emissions to the product of
the vessel\'s deadweight tonnage (DWT) and distance sailed in nautical
miles (IMO, 2021). A ship is assigned a rating from A (most efficient)
to E (least efficient) based on comparison of its attained CII against a
required CII reference line. Critically, the required CII tightens
according to annual reduction factors relative to the 2019 reference
line. These factors are defined in IMO guidance as lookup values
(for example, 5% in 2023, 7% in 2024, 9% in 2025, and 11% in 2026),
meaning that maintaining a given rating becomes progressively more
demanding over time.

A vessel rated D for three consecutive years, or E in any single year,
is required to submit a corrective action plan as part of its Ship
Energy Efficiency Management Plan (SEEMP Part III), which must be
verified by a recognised organisation (RO) or flag administration. This
regulatory consequence underscores the importance of continuous
operational monitoring rather than retrospective annual reporting alone.

OpenCPN is a cross-platform, ship-borne navigation application actively
maintained on GitHub and in widespread use globally across commercial
and recreational maritime sectors. It supports a mature plugin
architecture through which third-party developers can extend its
functionality by implementing a standardised C++ interface. Existing
plugins include engine dashboards, AIS radar overlays, tide predictors,
and voyage loggers. Notably, no existing plugin provides EEXI or CII
compliance monitoring, representing a clear and addressable gap in the
open-source maritime software ecosystem.

## 3. Problem Statement

Although the EEXI and CII regulatory framework has been mandatory since
January 2023, real-time operational awareness of CII performance during
a voyage remains limited in practice. Compliance is typically assessed
retrospectively through annual DCS (Data Collection System) submissions,
providing little opportunity for masters or operators to make informed
speed or routing adjustments that could improve a vessel\'s end-of-year
rating. Commercial monitoring platforms exist, but their cost and
proprietary nature place them beyond the reach of many operators,
particularly in emerging maritime markets.

Furthermore, no open-source tool currently integrates CII tracking with
a live navigation environment. This project seeks to address this
problem by delivering a freely available, IMO-informed operational
awareness plugin that leverages the own-ship GPS/GNSS data stream
already present in a vessel's navigation suite to compute and display
real-time CII performance, enabling proactive voyage management without
additional hardware cost. The plugin is not intended to replace class
society verification or official regulatory submission systems.

## 4. Aim and Objectives

The aim of this project is to design, implement, and validate a
functional OpenCPN plugin that provides simplified EEXI compliance
status for vessels of 400 gross tonnage and above, and running CII
rating estimation for vessels of 5,000 gross tonnage and above, using
live own-ship NMEA 0183 RMC data available within the OpenCPN navigation
environment.

The specific objectives of this project are:

1.  To review and critically analyse the IMO regulatory framework
    governing EEXI and CII, including the AER calculation methodology
    and the required CII reference lines for applicable vessel types.

2.  To design a modular CII calculation engine in C++ that estimates
    fuel consumption using the Admiralty Coefficient method and computes
    AER from accumulated voyage data.

3.  To implement the plugin within the OpenCPN Plugin API, integrating
    NMEA 0183 RMC sentence parsing for speed over ground (SOG),
    position, and UTC timestamp from the own-ship navigation feed.

4.  To develop a real-time graphical dashboard within the OpenCPN
    interface displaying current vessel speed, estimated CO₂ emission
    rate, accumulated AER, and CII rating (A--E).

5.  To validate the plugin's parsing, distance accumulation, and CII
    calculation flow by replaying publicly available voyage position
    data, and to cross-check computed CII values against independent
    manual calculations performed in accordance with IMO guidelines.

6.  To evaluate the practical applicability and limitations of the
    plugin, including the accuracy of the Admiralty-based fuel
    estimation model and its suitability for integration into a
    vessel\'s existing navigation workflow.

## 5. Significance of the Project

This project makes a tangible contribution to the intersection of
maritime regulatory compliance and open-source software development. By
embedding CII monitoring within a freely available navigation platform,
the plugin has the potential to democratise access to emissions
compliance tools, particularly for smaller operators and those in
developing maritime economies who rely on open-source navigation
systems.

From an academic standpoint, the project integrates knowledge from
marine engineering systems, thermodynamics, maritime law and regulation,
software engineering, and signal processing --- reflecting the
interdisciplinary nature of modern marine engineering practice. The
deliverable is a working software artefact that can be demonstrated,
tested, and independently validated, providing a robust basis for
academic examination and professional portfolio development.

The project is also timely. As the IMO\'s reduction factor for CII
tightens annually and the consequences of poor ratings escalate, the
demand for accessible, real-time compliance monitoring tools will only
increase. A validated, open-source solution contributes directly to the
body of knowledge and tooling available to the global maritime
community.

## References

International Maritime Organization (IMO) (2021). Resolution
MEPC.337(76): 2021 Guidelines on the Operational Carbon Intensity
Indicators and the Rating of Ships. London: IMO.

International Maritime Organization (IMO) (2022). Resolution
MEPC.333(76): 2021 Guidelines on the Energy Efficiency Existing Ship
Index (EEXI). London: IMO.

International Maritime Organization (IMO) (2023). Fourth IMO GHG Study
2020 --- Update. London: IMO.

Fossen, T.I. (2021). Handbook of Marine Craft Hydrodynamics and Motion
Control. 2nd edn. Chichester: Wiley.

OpenCPN Development Team (2025). OpenCPN: A Concise Chart
Plotter/Navigator. Available at: https://github.com/OpenCPN/OpenCPN
\[Accessed: June 2026\].

MARPOL Annex VI, Regulation 23 --- Attained Energy Efficiency Existing
Ship Index (EEXI). Consolidated edition, 2022.

MARPOL Annex VI, Regulation 28 --- Operational Carbon Intensity
Indicators. Consolidated edition, 2022.
