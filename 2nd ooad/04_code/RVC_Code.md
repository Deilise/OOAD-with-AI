# Roomba RVC — Code Summary

Companion to the updated [SRS](../01_requirements/RVC_SW_Controller_SRS.md), [class diagram](../03_diagrams/class/RVC_Class_Diagram.md), and [sequence diagrams](../03_diagrams/sd/RVC_SD_Index.md).

The actual C++ project is in [`../../cpp`](../../cpp). This document summarizes how the code works after the **v0.7.2** alignment.

## How the Updated Code Works

- `RvcSoftwareController` owns the four SRS §3.2 objects and exposes session, obstacle, and dust input ports.
- `AutomaticCleaningSession` initializes **`travelToggle=Forward`** and **`cleaningMode=Normal`** on `StartSession` (OBJ1-FR-4).
- `ObstaclePerceptionContext` fuses **front / left / back** samples and probe-pose observations; **`backObstacle`** is the leading sector when toggled Backward.
- Right-side probe sensor follows **`travelToggle`**: front when Forward, back when Backward.
- `NavigationAndEscapeCoordinator` cruises along the **leading travel sector** (`forward` vs `reverse`), runs **dust maneuver** (stop → 540° spin → Boost), and calls **`ToggleTravelDirection()`** on dust-maneuver completion.
- Surrounded escape uses **`reverseEscapeSegment`** (not toggled cruise); exit resumes per preserved toggle.
- `SurfaceCleaningController` uses **Normal / Boost / Suspended**; dust boost is driven by navigation (`StartBoostCleaning`), not direct `DustSignalUpdated` side effects.

## Changed (v0.7.2)

- Forward-only cruise replaced by **toggle-aware** cruise and resume.
- **`CleaningMode::Active/Boosted`** replaced by **Normal / Boost / Suspended**.
- **`FusedObstacleSnapshot`** uses **leading-sector** fields instead of `forwardSafe` / `forwardBlocked`.
- Surrounded escape commands: **`restoreHeading` + `reverseEscapeSegment`** (removed `forbidForward` / `continueReverse`).
- Dust handling moved to **`DustManeuverRequested` / `DustManeuverComplete`** path via perception → navigation.

## Added

- **`TravelToggle`**, **`ProbeSensor`** enums and `travelToggle` state on navigation.
- **`backObstacle`** on perception; **`SetCleaningMode`**, **`StartBoostCleaning`**, **`EndBoostCleaning`** on cleaning.
- Motion commands: **`spin540Clockwise`**, **`spin540CounterClockwise`**, **`reverseEscapeSegment`**, **`forwardOrReversePerToggle`**.

## Removed

- **`CleaningCommand::active`**, direct dust-boost flip in `DustSignalUpdated`.
- Forward-only snapshot fields and obsolete motion commands (`forbidForward`, `lateralEscapeRight`, etc.).

## Verification

- Build passed with CMake/MSBuild.
- Unit tests passed: **59 / 59**.
- Simulator scenarios passed: **30 / 30**.

Full unit-test inventory, v0.7.2 test deltas, recommendations, and remaining gaps: **[RVC_Unit_Tests.md](../05_unit_tests/RVC_Unit_Tests.md)**.

### Unit tests added for v0.7.2 fixes

Three **new** tests in `cpp/tests/SurfaceCleaningControllerTest.cpp`:

- `SessionStateChangedWithNormalModeSendsNormal` — OBJ1-FR-4 init cleaning mode
- `DustSignalUpdatedStoresSignalWithoutImmediateCommand` — dust port no longer auto-triggers Boost
- `StartBoostCleaningSendsBoostWhenSessionActive` — navigation-driven Boost command

**Eleven additional tests** (backward toggle + dust maneuver) in `NavigationAndEscapeCoordinatorTest.cpp` and `ObstaclePerceptionContextTest.cpp` — see [RVC_Unit_Tests.md](../05_unit_tests/RVC_Unit_Tests.md#new-tests-backward-toggle--dust-maneuver).

Many existing tests were **renamed or rewritten** for leading-sector semantics, `reverseEscapeSegment`, and `CleaningCommand(normal)`. See the tables in [RVC_Unit_Tests.md](../05_unit_tests/RVC_Unit_Tests.md#unit-tests-added-or-updated-for-v072-fixes).

**Seven additional tests** closed the remaining priority gaps (surrounded Backward exit, dust deferral, back ignored during dust, session-init toggle reset) — see [RVC_Unit_Tests.md](../05_unit_tests/RVC_Unit_Tests.md#new-tests-priority-gaps-closed).

### Unit test recommendations

1. **Simulator backward-toggle scenarios** — integration smoke for Backward cruise / dust CCW / surrounded reverse exit.
2. **`ResumeSession` vs `StartSession`** — unit test that resume preserves toggle while start resets it.

### Major improvements remaining

No major unit-test gaps remain at the domain-object level. Optional simulator scenarios above would add integration coverage only.

`2nd ooad/05_unit_tests/*.cpp` is synced with `cpp/tests/` (59 tests).

## Not in class diagram (implementation detail)

- **`FusedObstacleSnapshotKind::reverseReadings`**, **`reverseCycleSample`**, and **`lateralOpening`** retained in code for surrounded-escape intermediate states used by UC-05 SD paths.
