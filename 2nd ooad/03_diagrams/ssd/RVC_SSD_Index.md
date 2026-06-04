# Roomba RVC — System sequence diagram (SSD) gallery

Companion to **`../../02_use_cases/RVC_SW_Controller_UseCases.md`**. Each use case has **one SSD** with **typical**, **alternative**, and **exceptional** courses in **`alt` / `else` / `loop`** frames (PlantUML). This SSD set is aligned with SRS/use cases v0.6.0: the RVC has direct front/left obstacle sensing, no dedicated right-side sensor, and infers right-side status through a high-level right-side probe.

**Domain model (middle layer):** [RVC_Domain_Diagram.md](../domain/RVC_Domain_Diagram.md)

**Sequence diagrams (SD):** [SD gallery](../sd/RVC_SD_Index.md)

**Class diagram:** [RVC_Class_Diagram.md](../class/RVC_Class_Diagram.md)

**Source:** `UCxx_system_sequence.puml` · **Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File ..\render-diagrams.ps1`

## By use case

| UC | Use case | Diagram |
|----|----------|---------|
| UC-01 | Start automatic cleaning session | [UC-01 SSD](UC-01_SSD.md) |
| UC-02 | Cruise forward while cleaning | [UC-02 SSD](UC-02_SSD.md) |
| UC-03 | Avoid obstacle (forward blocked, not surrounded) | [UC-03 SSD](UC-03_SSD.md) |
| UC-04 | Avoid obstacle when right turn is blocked | [UC-04 SSD](UC-04_SSD.md) |
| UC-05 | Escape when surrounded | [UC-05 SSD](UC-05_SSD.md) |
| UC-06 | Boost cleaning when dust is high | [UC-06 SSD](UC-06_SSD.md) |
| UC-07 | Resume forward cleaning after a maneuver | [UC-07 SSD](UC-07_SSD.md) |
| UC-08 | Handle missing / invalid / stale obstacle or probe data | [UC-08 SSD](UC-08_SSD.md) |
| UC-09 | Build consistent fused obstacle picture | [UC-09 SSD](UC-09_SSD.md) |

## Frame legend (on diagrams)

| Label prefix | Meaning |
|--------------|---------|
| `[typical]` | Typical course of events |
| `[A1]`, `[A2]` | Alternative courses |
| `[E1]`, `[E2]`, … | Exceptional courses |

Cross–use-case handoffs appear in **notes** only (e.g. `next: UC-02`).

## Change summary for SRS/use case v0.6.0 alignment

### Changed

- UC-03, UC-04, UC-05, UC-08, and UC-09 SSDs now show direct **front / left** obstacle sensors instead of direct front/left/right sensing.
- Right-side decisions now come from direct sensor observations taken during the probe pose, shown as `ObstacleStateChanged(probePose=right, front=...)`, requested through high-level `MotionCommand(action=probeRightSide)` and `MotionCommand(action=restoreHeading)` interactions.
- UC-05 still treats surrounded as front + left + right blocked, but right is now inferred by probe.
- UC-05 now explicitly notes that surrounded escape requires a reverse segment and is not satisfied by a 180-degree turn followed by forward travel.
- UC-08 now covers stale/invalid observations during the right-side probe as well as stale/invalid direct obstacle data.
- UC-09 now shows fusion/snapshot logic combining direct front/left samples with probe-pose observations.

### Added

- Right-side probe messages in UC-03, UC-04, UC-05, and UC-09.
- Probe timeout/invalid path in UC-04 and UC-08.
- Markdown wrapper descriptions for probe-related frames.
- Organized local links for SSD wrapper pages and source/rendered diagram files.

### Removed

- Direct right-side obstacle sensor assumptions from affected SSDs.
- Wording that treated right-turn viability as direct right sensor validity.
