# Roomba RVC — Sequence diagram (SD) gallery

Companion to the [SSD gallery](../ssd/RVC_SSD_Index.md) and [domain model](../domain/RVC_Domain_Diagram.md). These diagrams open the SSD black box and show object interactions between SRS/domain objects. This set is aligned with SRS/use case/SSD **v0.7.2**: **`travelToggle`** swaps the leading travel edge (front vs back), **Normal / Boost / Suspended** cleaning, **dust maneuver** (540° spin + toggle), **back** obstacle sensor, and **right-side probe** using the **front** sensor when Forward toggle and the **back** sensor when Backward toggle.

**Source folder:** `03_diagrams/sd/`  
**Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File ..\render-diagrams.ps1`

| UC | Use case | Sequence diagram |
|----|----------|------------------|
| UC-01 | Start automatic cleaning session | [UC-01 SD](UC-01_SD.md) |
| UC-02 | Cruise in normal mode while cleaning | [UC-02 SD](UC-02_SD.md) |
| UC-03 | Avoid obstacle (leading sector blocked, not surrounded) | [UC-03 SD](UC-03_SD.md) |
| UC-04 | Avoid obstacle when right turn is blocked | [UC-04 SD](UC-04_SD.md) |
| UC-05 | Escape when surrounded | [UC-05 SD](UC-05_SD.md) |
| UC-06 | Perform dust maneuver (spin, boost, toggle travel) | [UC-06 SD](UC-06_SD.md) |
| UC-07 | Resume normal cruise after a maneuver | [UC-07 SD](UC-07_SD.md) |
| UC-08 | Handle missing / invalid / stale obstacle or probe data | [UC-08 SD](UC-08_SD.md) |
| UC-09 | Build consistent fused obstacle picture | [UC-09 SD](UC-09_SD.md) |

## Rules

- Show interactions between actors, domain objects, and hardware actors.
- Do not show internal self-processing inside a single object (except UC-09 fusion steps on `ObstaclePerceptionContext`).
- Use the same domain objects as the domain model / SRS Section 3.2.
- **Leading travel sector** follows `travelToggle`: front when Forward, back when Backward.
- Right-side checking is shown as `RequestRightSideProbe(...)`, `MotionCommand(action=probeRightSide)`, and probe-pose observations: `ObstacleStateChanged(probePose=right, front=...)` or `back=...` per toggle.

## Change summary for SRS/use case/SSD v0.7.2 alignment

### Changed

- All affected SDs now include **back** obstacle sensor where `travelToggle` is Backward.
- **Leading travel sector** replaces forward-only cruise/avoidance/resume wording.
- **UC-01** initializes `travelToggle=Forward` and `cleaningMode=Normal` on session start.
- **UC-02** shows Forward vs Backward cruise branches; removed concurrent dust-boost cruise path (handoff to UC-06).
- **UC-03/04** probe and resume branches split by toggle (`front=` vs `back=`).
- **UC-05** probe sensor by toggle during escape; exit `forward` vs `reverse` per preserved toggle; back not leading during escape reverse.
- **UC-06** rewritten for dust maneuver: Nav + Motors (stop/spin540), Clean (Boost/normal), `ToggleTravelDirection`.
- **UC-07/08/09** updated for leading sector, back-sensor paths, and toggle-aware probe fusion.

### Preserved

- No dedicated right-side sensor; right inferred via probe pose.
- Surrounded trigger still body-fixed front + left + right (from probe).
- PlantUML source + PNG render workflow unchanged.

### Prior revisions

- **v0.6.0:** right-side probe replaced dedicated right sensor; surrounded escape reverse segment.
