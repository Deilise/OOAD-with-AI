# Roomba RVC - Sequence Diagram (SD) Gallery

Companion to the [SSD gallery](../ssd/RVC_SSD_Index.md) and [domain model](../domain/RVC_Domain_Diagram.md). These diagrams open the SSD black box and show object interactions between SRS/domain objects. This set is aligned with SRS/use case/SSD v0.6.0: direct front/left obstacle sensing, no dedicated right-side sensor, and right-side inference through a probe-pose observation.

**Source folder:** `03_diagrams/sd/`  
**Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File ..\render-diagrams.ps1`

| UC | Sequence diagram |
|----|------------------|
| UC-01 | [UC-01 SD](UC-01_SD.md) |
| UC-02 | [UC-02 SD](UC-02_SD.md) |
| UC-03 | [UC-03 SD](UC-03_SD.md) |
| UC-04 | [UC-04 SD](UC-04_SD.md) |
| UC-05 | [UC-05 SD](UC-05_SD.md) |
| UC-06 | [UC-06 SD](UC-06_SD.md) |
| UC-07 | [UC-07 SD](UC-07_SD.md) |
| UC-08 | [UC-08 SD](UC-08_SD.md) |
| UC-09 | [UC-09 SD](UC-09_SD.md) |

## Rule Used

- Show interactions between actors, domain objects, and hardware actors.
- Do not show internal self-processing inside a single object.
- Use the same domain objects as the domain model / SRS Section 3.2.
- Right-side checking is shown as `RequestRightSideProbe(...)`, high-level probe re-orientation via `MotionCommand(action=probeRightSide)`, and direct sensor observation while in the probe pose: `ObstacleStateChanged(probePose=right, front=...)`.

## Change summary for SRS/use case/SSD v0.6.0 alignment

### Changed

- UC-02, UC-03, UC-04, UC-05, UC-07, UC-08, and UC-09 now use direct **front / left** obstacle sensors rather than generic front/left/right obstacle sensors.
- UC-03 and UC-04 now show `ObstaclePerceptionContext` requesting a right-side probe before right/left turn selection.
- UC-05 now shows surrounded detection as direct front/left blocked plus probed right blocked, then a required reverse segment.
- UC-08 now handles invalid/stale probe-pose observations in addition to invalid/stale direct sensor data.
- UC-09 now shows direct front/left samples combined with right-side probe-pose observations before `FusedObstacleSnapshot(...)`.

### Added

- `RequestRightSideProbe(...)` object interactions.
- `MotionCommand(action=probeRightSide)`, `MotionCommand(action=restoreHeading)`, and `MotionCommand(action=restoreEscapeHeading)` interactions.
- Probe-pose observations using `ObstacleStateChanged(probePose=right, front=...)`.
- Notes that surrounded escape requires reverse and is not replaced by a 180-degree turn followed by forward travel.

### Removed

- Direct right-side obstacle sensor assumptions.
- Direct `rightBlocked`, `rightInvalid`, `rightTurnViable` sensor-update wording.
- Generic `Obstacle sensors` participant names where the diagram specifically depends on front/left direct sensing.
