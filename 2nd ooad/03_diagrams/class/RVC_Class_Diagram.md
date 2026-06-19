# Roomba RVC — Class Diagram

Companion to the [SD gallery](../sd/RVC_SD_Index.md), [SSD gallery](../ssd/RVC_SSD_Index.md), and [domain model](../domain/RVC_Domain_Diagram.md). This version is aligned with SRS/use case/SD **v0.7.2**: **`travelToggle`** swaps the leading travel edge (front vs back), **Normal / Boost / Suspended** cleaning, **dust maneuver** (540° spin + toggle), **back** obstacle sensor, and **right-side probe** using the **front** sensor when Forward toggle and the **back** sensor when Backward toggle.

This class diagram is based on:

- SRS §3.2 object names and attributes
- SD messages (`SessionStateChanged`, `ObstacleStateChanged`, `FusedObstacleSnapshot`, `DustManeuverRequested`, `ToggleTravelDirection`, `MotionCommand`, `CleaningCommand`, etc.)
- External actors / hardware as interfaces, not internal classes

**Source:** `RVC_class.puml`  
**Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File ..\render-diagrams.ps1`

![RVC class diagram](RVC_class.png)

## Main Classes

| Class | Source |
|-------|--------|
| `AutomaticCleaningSession` | SRS §3.2.1, UC-01 SD — init `travelToggle=Forward`, `cleaningMode=Normal` |
| `SurfaceCleaningController` | SRS §3.2.2, UC-02 / UC-06 SD — Normal, Boost, Suspended |
| `ObstaclePerceptionContext` | SRS §3.2.3, UC-03 / UC-05 / UC-06 / UC-08 / UC-09 SD — includes `backObstacle` |
| `NavigationAndEscapeCoordinator` | SRS §3.2.4, UC-02–UC-07 SD — includes `travelToggle`, dust maneuver, escape |
| `FusedObstacleSnapshot` | SRS message payload — **leading sector** semantics |

## External Interfaces

| Interface | Represents |
|-----------|------------|
| `SessionIntentPort` | Home user / scheduler intent |
| `ObstacleInputPort` | Direct **front / left / back** updates and probe-pose observations |
| `DustInputPort` | Dust or debris-load signal |
| `MotionCommandSink` | Wheel motors / lower motion layer |
| `CleaningCommandSink` | Vacuum / brush / mop hardware command sink |

## Notes

- `FusedObstacleSnapshot` is modeled as a value object because the SDs pass it repeatedly from `ObstaclePerceptionContext` to `NavigationAndEscapeCoordinator`.
- **Leading travel sector** replaces forward-only fields: `leadingSectorSafe`, `leadingSectorBlocked` (not `forwardSafe` / `forwardBlocked`).
- **`TravelToggle`** and **`ProbeSensor`** enums model toggle-aware cruise, resume, and right-side probe behavior.
- **`ToggleTravelDirection()`** is internal to `NavigationAndEscapeCoordinator`; toggled only on dust-maneuver completion (OBJ4-FR-12/13).
- Dust maneuver uses `StartBoostCleaning` / `EndBoostCleaning` (SRS) and SD paths `ResumeCleaningAfterManeuver(mode=Boost)`, `SetCleaningMode(Normal)`.
- `RvcSoftwareController` is a composition root for the four SRS §3.2 objects.
- Realization (`..|>`) is used where a class provides an input port.
- TBD variants remain explicit in enums/commands instead of inventing final product behavior.

## Change summary for SRS/use case/SSD/SD v0.7.2 alignment

### Changed

- `ObstaclePerceptionContext` now stores **`backObstacle`** and sends **`DustManeuverRequested`** / **`DustManeuverComplete`**.
- `NavigationAndEscapeCoordinator` now stores **`travelToggle`**, receives session state with toggle, and exposes **`ToggleTravelDirection()`**.
- `SessionStateChanged` on navigation/cleaning now carries initial **`travelToggle=Forward`** and **`cleaningMode=Normal`**.
- `CleaningMode` enum: **Normal / Boost / Suspended** (replaces Active / Boosted).
- `FusedObstacleSnapshot` uses **leading-sector** fields; snapshot kinds updated accordingly.
- `MotionCommand` adds **`spin540Clockwise`**, **`spin540CounterClockwise`**, **`reverseEscapeSegment`**, **`forwardOrReversePerToggle`**.
- `MotionState` adds **`BackwardCruise`**, **`SurroundedReversing`**, **`DustManeuvering`**.
- `ObstacleInputPort` and `ObstacleEventKind` cover **front / left / back** direct samples.
- `MotionCommandSink` accepts optional **`probeSensor`** for toggle-aware probe commands.

### Added

- **`TravelToggle`** enum (Forward, Backward).
- **`ProbeSensor`** enum (front, back) on `ObstacleEvent` and motion sink.
- **`StartBoostCleaning`** / **`EndBoostCleaning`** / **`SetCleaningMode`** on cleaning controller.
- **`DustManeuverRequested`** / **`DustManeuverComplete`** message paths perception → navigation.

### Removed

- Direct **`forwardSafe`** / **`forwardBlocked`** snapshot fields and kinds.
- **`CleaningMode.Active`** / **`Boosted`** and **`CleaningCommand.active`**.
- Obsolete motion commands superseded by v0.7.2 SDs (`restoreEscapeHeading`, `forbidForward`, etc.).

### Prior revisions

- **v0.6.0:** right-side probe replaced dedicated right sensor; surrounded escape reverse segment.
