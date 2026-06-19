# Roomba RVC — Unit Test Summary

Companion to the updated [code summary](../04_code/RVC_Code.md), [class diagram](../03_diagrams/class/RVC_Class_Diagram.md), and [SRS](../01_requirements/RVC_SW_Controller_SRS.md) **v0.7.2**.

The actual C++ unit tests live in [`../../cpp/tests`](../../cpp/tests). They verify session control, **leading-sector** cruise/avoidance (Forward and Backward), right-side probe inference (front/back sensor), **reverse escape segments**, **dust maneuver** (540° spin + toggle), **Normal / Boost / Suspended** cleaning, and safe behavior on invalid data.

**Last verified:** 59 / 59 tests passing (v0.7.2 full unit coverage).

## Unit test files

| File | Focus |
|------|--------|
| `AutomaticCleaningSessionTest.cpp` | Start / stop / resume / service-reset; init **Normal** cleaning + forward cruise on start |
| `SurfaceCleaningControllerTest.cpp` | **Normal / Boost / Suspended**; `StartBoostCleaning`; dust signal storage (no auto-boost) |
| `NavigationAndEscapeCoordinatorTest.cpp` | Toggle-forward/backward cruise, **`reverseEscapeSegment`**, dust maneuver spin, turn selection, safe hold |
| `ObstaclePerceptionContextTest.cpp` | **Leading-sector** fusion, back-sensor probe, dust maneuver events, surrounded detection |
| `RvcSoftwareControllerTest.cpp` | Composition root, port wiring, end-to-end surrounded → escape commands |

---

## Unit tests added or updated for v0.7.2 fixes

These changes were made in `cpp/tests/` when aligning code with class diagram / SD **v0.7.2**. Count: **59 tests** (52 after backward/dust batch + **7 new** priority-gap tests).

### New tests (v0.7.2 surface cleaning)

| Test | File | What it verifies |
|------|------|------------------|
| `SessionStateChangedWithNormalModeSendsNormal` | `SurfaceCleaningControllerTest.cpp` | OBJ1-FR-4: session start path sends **`CleaningCommand(normal)`** via `SessionStateChanged(active, Normal)` |
| `DustSignalUpdatedStoresSignalWithoutImmediateCommand` | `SurfaceCleaningControllerTest.cpp` | OBJ4-FR-10: dust maneuver is **not** triggered directly from `DustSignalUpdated` (signal stored only) |
| `StartBoostCleaningSendsBoostWhenSessionActive` | `SurfaceCleaningControllerTest.cpp` | Dust maneuver Boost path: **`StartBoostCleaning()`** → **`CleaningCommand(boost)`** |

### New tests (backward toggle + dust maneuver)

| Test | File | What it verifies |
|------|------|------------------|
| `SessionStateChangedActiveWithBackwardToggleStartsReverseCruise` | `NavigationAndEscapeCoordinatorTest.cpp` | Session active with **`travelToggle=Backward`** → **`MotionCommand(reverse)`** |
| `LeadingSectorSafeResumesReverseCleaningWhenToggledBackward` | `NavigationAndEscapeCoordinatorTest.cpp` | **`leadingSectorSafe`** with Backward toggle → reverse + **`CleaningCommand(normal)`** |
| `RequestRightSideProbeUsesBackSensorWhenToggledBackward` | `NavigationAndEscapeCoordinatorTest.cpp` | Right-side probe uses **`ProbeSensor::back`** when toggled Backward |
| `DustManeuverRequestedForwardSpinsClockwiseAndBoosts` | `NavigationAndEscapeCoordinatorTest.cpp` | UC-06 Forward: stop + **`spin540Clockwise`** + Boost |
| `DustManeuverRequestedBackwardSpinsCounterClockwiseAndBoosts` | `NavigationAndEscapeCoordinatorTest.cpp` | UC-06 Backward: stop + **`spin540CounterClockwise`** + Boost |
| `DustManeuverCompleteTogglesTravelAndResumesReverseAfterLeadingSectorSafe` | `NavigationAndEscapeCoordinatorTest.cpp` | **`DustManeuverComplete()`** → Normal + toggle; resume reverse after safe snapshot |
| `BackSampleSetsLeadingSectorBlockedWhenToggledBackward` | `ObstaclePerceptionContextTest.cpp` | **`backSample`** blocked → **`leadingSectorBlocked`** when Backward |
| `LeadingSectorBlockedRequestsProbeWithBackSensorWhenToggledBackward` | `ObstaclePerceptionContextTest.cpp` | Leading sector blocked → probe with **`ProbeSensor::back`** |
| `ProbePoseRightSampleUsesBackBlockedWhenToggledBackward` | `ObstaclePerceptionContextTest.cpp` | Probe-pose right sample uses **`backBlocked`** when Backward |
| `DustDetectedTriggersDustManeuverSpinAndBoost` | `ObstaclePerceptionContextTest.cpp` | **`dustDetected`** → navigation **`DustManeuverRequested()`** + spin + Boost |
| `DustClearedTogglesTravelAndSetsNormalCleaning` | `ObstaclePerceptionContextTest.cpp` | **`dustCleared`** → **`DustManeuverComplete()`** + Normal + toggle |

### New tests (priority gaps closed)

| Test | File | What it verifies |
|------|------|------------------|
| `SurroundedEscapeExitResumesReverseWhenToggledBackward` | `NavigationAndEscapeCoordinatorTest.cpp` | UC-05 exit with preserved Backward toggle → **`reverse`** + Normal |
| `DustManeuverDeferredWhileAvoiding` | `NavigationAndEscapeCoordinatorTest.cpp` | OBJ4-FR-9: no spin/Boost while **`Avoiding`** |
| `DustManeuverDeferredWhileSurroundedReversing` | `NavigationAndEscapeCoordinatorTest.cpp` | OBJ4-FR-9: no spin/Boost while **`SurroundedReversing`** |
| `DustManeuverDeferredWhileRightSideProbing` | `NavigationAndEscapeCoordinatorTest.cpp` | OBJ4-FR-9: no spin/Boost while **`RightSideProbing`** |
| `DustDetectedDeferredWhileAvoiding` | `ObstaclePerceptionContextTest.cpp` | Perception path: **`dustDetected`** during avoidance emits no commands |
| `BackSampleIgnoredDuringDustManeuver` | `ObstaclePerceptionContextTest.cpp` | OBJ3-FR-7: **`backSample`** ignored during dust spin |
| `StartSessionResetsTravelToggleForwardAndNormalCleaning` | `AutomaticCleaningSessionTest.cpp` | OBJ1-FR-4: **`StartSession`** resets Forward + Normal |

### Renamed / rewritten tests (v0.7.2 semantics)

| Was (v0.6.0) | Now (v0.7.2) | File | v0.7.2 fix covered |
|--------------|--------------|------|-------------------|
| `SessionStateChangedActiveEnablesPolicyWithoutImmediateCommand` | `SessionStateChangedActiveStartsForwardCruise` | `NavigationAndEscapeCoordinatorTest.cpp` | Session active starts **`MotionCommand(forward)`** (leading sector Forward) |
| `PartialStaleSnapshotUsesPartialStopCommand` | `PartialStaleSnapshotUsesSafeHoldCommand` | `NavigationAndEscapeCoordinatorTest.cpp` | Stale data → **`stopOrSafeHold`** (UC-08) |
| `SurroundedSnapshotForbidsForwardThenStartsReverse` | `SurroundedSnapshotRestoresHeadingThenReverseEscape` | `NavigationAndEscapeCoordinatorTest.cpp` | UC-05: **`restoreHeading` + `reverseEscapeSegment`** (not `forbidForward` + `reverse`) |
| `ReverseReadingsContinueReverse` | `ReverseReadingsContinueReverseEscape` | `NavigationAndEscapeCoordinatorTest.cpp` | Escape segment continues with **`reverseEscapeSegment`** |
| `LateralOpeningsEscapeToAvailableSide` | `LateralOpeningsTurnTowardAvailableSide` | `NavigationAndEscapeCoordinatorTest.cpp` | Escape turn uses **`turnRight` / `turnLeft`** after **`restoreHeading`** |
| `ForwardBlockedPrefersRightWhenBothSidesAreViable` | `LeadingSectorBlockedPrefersRightWhenBothSidesAreViable` | `NavigationAndEscapeCoordinatorTest.cpp` | UC-03/04: **`leadingSectorBlocked`** + prefer-right |
| `ForwardBlockedTurnsLeftWhenOnlyLeftIsViable` | `LeadingSectorBlockedTurnsLeftWhenOnlyLeftIsViable` | `NavigationAndEscapeCoordinatorTest.cpp` | Leading sector blocked → left turn |
| `ForwardSafeResumesForwardCleaning` | `LeadingSectorSafeResumesForwardCleaning` | `NavigationAndEscapeCoordinatorTest.cpp` | UC-07: **`leadingSectorSafe`** → forward + **`CleaningCommand(normal)`** |
| `ConsistencyOnlySnapshotsDoNotEmitCommands` | `ValidSnapshotsDoNotEmitCommands` | `NavigationAndEscapeCoordinatorTest.cpp` | Fusion **`valid`** snapshot is a no-op at navigation |
| `ObstacleStateChangedMapsForwardSafeToForwardSafeSnapshot` | `ObstacleStateChangedMapsLeadingSectorSafeSnapshot` | `ObstaclePerceptionContextTest.cpp` | **`leadingSectorSafe`** snapshot kind/fields |
| `ForwardBlockedRequestsRightSideProbeWhenRightStatusIsMissing` | `LeadingSectorBlockedRequestsRightSideProbeWhenRightStatusIsMissing` | `ObstaclePerceptionContextTest.cpp` | **`leadingSectorBlocked`** triggers probe |
| `ObstacleStateChangedMapsFusionEvents` (`frontLeftSample`) | `ObstacleStateChangedMapsFrontSample` | `ObstaclePerceptionContextTest.cpp` | Direct **`frontSample`** → **`valid`** fusion |

### Updated expectations (same test name, new assertions)

| Test | File | v0.7.2 change |
|------|------|---------------|
| `StartSessionEnablesNavigationAndCleaningPolicies` | `AutomaticCleaningSessionTest.cpp` | Expect **`normal`** (not `active`) and **double `forward`** (start cruise + leading-sector safe) |
| `ResumeSessionEnablesPoliciesAfterStop` | `AutomaticCleaningSessionTest.cpp` | Expect **`normal`** resume cleaning |
| `ProbePoseRightBlockedWithFrontAndLeftBlockedMapsToSurrounded` | `ObstaclePerceptionContextTest.cpp` | Expect **`reverseEscapeSegment`** (not `forbidForward` + `reverse`); assert **`leadingSectorBlocked`** |
| `ResumeCleaningAfterManeuverSendsActiveWhenSessionActive` | `SurfaceCleaningControllerTest.cpp` | Renamed behavior: expects **`normal`** (not `active`) |
| `DustSignalUpdatedBoostsThenReturnsToNormalWhenActive` | *(removed)* | Replaced by **`DustSignalUpdatedStoresSignalWithoutImmediateCommand`** |
| `DustSignalUpdatedDefersBoostWhenSessionInactive` | *(removed)* | Replaced: inactive session → **no cleaning command** on dust update |
| `PortAccessorsExposeUsableInputPorts` | `RvcSoftwareControllerTest.cpp` | **`leadingSectorSafe`** event; **`normal`** cleaning; no dust auto-boost |
| `DomainObjectAccessorsExposeOwnedObjects` | `RvcSoftwareControllerTest.cpp` | Surrounded → **`reverseEscapeSegment`** |

### Still covered from v0.6.0 (unchanged intent)

- Right-side probe before turn selection (`RequestRightSideProbe`, `probeRightSide`)
- Probed right clear → right turn; probed right blocked + left open → left turn
- Front + left + probed-right blocked → **`surrounded`**
- Invalid / partial-stale / ambiguous obstacle handling
- Session stop, service reset, initialization safe state

---

## Unit test recommendations

Simulator scenarios (TC-25–TC-30) cover **integration smoke** for dust and surrounded escape on **Forward** toggle only. Unit tests now cover all four previously listed priority gaps at the **domain-object** level with test doubles.

Optional future additions (lower priority):

1. **Simulator backward-toggle scenarios** — TC for Backward cruise, dust CCW spin, surrounded exit in reverse.
2. **ResumeSession vs StartSession** — assert **`ResumeSession`** does **not** reset `travelToggle` (only **`StartSession`** does).

---

## Major improvements remaining

No major unit-test gaps remain against SRS / SD **v0.7.2** behavior at the domain-object level.

**Code fix with tests:** `ObstaclePerceptionContext` now skips **`backSample`** fusion/navigation while navigation is **`DustManeuvering`** (OBJ3-FR-7).

**Synced:** `2nd ooad/05_unit_tests/*.cpp` mirrors `cpp/tests/` (59 tests).

---

## Changed (from v0.6.0 documentation)

- Expectations use **`leadingSectorSafe` / `leadingSectorBlocked`** instead of **`forwardSafe` / `forwardBlocked`**.
- Surrounded escape expects **`reverseEscapeSegment`**, not **`forbidForward` + `continueReverse`**.
- Lateral escape expects **`restoreHeading` + `turnRight/turnLeft`**, not **`lateralEscapeRight/Left`**.
- Cleaning resume expects **`CleaningCommand(normal)`**, not **`active`**.
- Partial stale expects **`stopOrSafeHold`**, not **`gradualOrPartialStopTBD`**.

## Removed from test expectations

- **`CleaningCommand::active`**, **`unchangedOrDeferredTBD`** on dust/resume paths.
- **`forwardSafe`**, **`forwardBlocked`**, **`frontLeftSample`** event kinds in assertions.
- **`forbidForward`**, **`continueReverse`**, **`restoreEscapeHeading`**, **`lateralEscapeRight/Left`**, **`fallbackTBD`**, **`gradualOrPartialStopTBD`**.

---

## Coverage

Re-run after code changes:

```powershell
cd ..\..\cpp
powershell -NoProfile -ExecutionPolicy Bypass -File .\run-coverage.ps1
```

Previous v0.6.0 baseline (re-run recommended after v0.7.2 edits):

- Unit tests: **59 / 59** passed.
- Total line coverage: **87%** (minimum **75%**).
- By source: `AutomaticCleaningSession.cpp` 100%, `NavigationAndEscapeCoordinator.cpp` 96%, `ObstaclePerceptionContext.cpp` 76%, `RvcSoftwareController.cpp` 90%, `SurfaceCleaningController.cpp` 84%.

HTML report (when generated): `cpp/build-coverage/coverage/html/index.html` · optional copy: [`coverage.html`](coverage.html).

## Readable test runner

```cmd
run-tests.bat
```

Or from repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ..\..\cpp\run-tests.ps1
```
