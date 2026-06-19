# Roomba RVC — System sequence diagram (SSD) gallery

Companion to **`../../02_use_cases/RVC_SW_Controller_UseCases.md`**. Each use case has **one SSD** with **typical**, **alternative**, and **exceptional** courses in **`alt` / `else` / `loop`** frames (PlantUML). This SSD set is aligned with SRS/use cases **v0.7.2**: **`travelToggle`** swaps the leading travel edge (front vs back), **Normal / Boost / Suspended** cleaning, **dust maneuver** (540° spin + toggle), and **right-side probe** using the **front** sensor when Forward toggle and the **back** sensor when Backward toggle.

**Domain model (middle layer):** [RVC_Domain_Diagram.md](../domain/RVC_Domain_Diagram.md)

**Sequence diagrams (SD):** [SD gallery](../sd/RVC_SD_Index.md)

**Class diagram:** [RVC_Class_Diagram.md](../class/RVC_Class_Diagram.md)

**Source:** `UCxx_system_sequence.puml` · **Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File ..\render-diagrams.ps1`

## By use case

| UC | Use case | Diagram |
|----|----------|---------|
| UC-01 | Start automatic cleaning session | [UC-01 SSD](UC-01_SSD.md) |
| UC-02 | Cruise in normal mode while cleaning | [UC-02 SSD](UC-02_SSD.md) |
| UC-03 | Avoid obstacle (leading sector blocked, not surrounded) | [UC-03 SSD](UC-03_SSD.md) |
| UC-04 | Avoid obstacle when right turn is blocked | [UC-04 SSD](UC-04_SSD.md) |
| UC-05 | Escape when surrounded | [UC-05 SSD](UC-05_SSD.md) |
| UC-06 | Perform dust maneuver (spin, boost, toggle travel) | [UC-06 SSD](UC-06_SSD.md) |
| UC-07 | Resume normal cruise after a maneuver | [UC-07 SSD](UC-07_SSD.md) |
| UC-08 | Handle missing / invalid / stale obstacle or probe data | [UC-08 SSD](UC-08_SSD.md) |
| UC-09 | Build consistent fused obstacle picture | [UC-09 SSD](UC-09_SSD.md) |

## Frame legend (on diagrams)

| Label prefix | Meaning |
|--------------|---------|
| `[typical]` | Typical course of events |
| `[A1]`, `[A2]` | Alternative courses |
| `[E1]`, `[E2]`, … | Exceptional courses |

Cross–use-case handoffs appear in **notes** only (e.g. `next: UC-02`).

## Change summary for SRS/use case v0.7.2 alignment

### Changed

- All affected SSDs now include **back** obstacle sensor where `travelToggle` is Backward.
- **Leading travel sector** replaces forward-only cruise/avoidance/resume wording.
- **Right-side probe** observations use `front=` when Forward toggle and `back=` when Backward toggle.
- **UC-05** exit frames split by toggle: `MotionCommand(forward)` vs `MotionCommand(reverse)`; back used for probe during escape reverse when toggled Backward.
- **UC-06** rewritten for dust maneuver: stop, 540° spin, Boost loop, `ToggleTravelDirection`, resume per toggle.
- **UC-01** notes initial `travelToggle=Forward`, `cleaningMode=Normal`.

### Preserved

- System remains a **black box**; external actors only.
- Surrounded trigger still body-fixed front + left + right (from probe).
- PlantUML source + PNG render workflow unchanged.

### Prior revisions

- **v0.6.0:** right-side probe replaced dedicated right sensor; surrounded escape reverse segment.
