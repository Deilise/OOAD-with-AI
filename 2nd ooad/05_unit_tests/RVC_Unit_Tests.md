# Roomba RVC — Unit Test Summary

Companion to the updated [code summary](../04_code/RVC_Code.md), [class diagram](../03_diagrams/class/RVC_Class_Diagram.md), and [SRS](../01_requirements/RVC_SW_Controller_SRS.md).

The actual C++ unit tests are in [`../../cpp/tests`](../../cpp/tests). They verify the updated code behavior for direct front/left sensing, right-side probe inference, obstacle avoidance, surrounded reverse escape, cleaning control, and session control.

## Unit Test Files

- `AutomaticCleaningSessionTest.cpp` checks session start, stop, resume, and service/reset behavior.
- `SurfaceCleaningControllerTest.cpp` checks cleaning activation, suspension, resume, dust boost, invalid dust signal, and inactive-session behavior.
- `ObstaclePerceptionContextTest.cpp` checks obstacle event fusion, front/left samples, right-side probe observations, invalid/stale data, no lateral opening, and surrounded detection.
- `NavigationAndEscapeCoordinatorTest.cpp` checks motion command decisions, probe request behavior, reverse escape, turn selection, fallback, and cleaning suspend/resume coordination.
- `RvcSoftwareControllerTest.cpp` checks initialization and that the controller exposes usable session, obstacle, dust, and domain-object accessors.

## Changed

- Unit tests now use the updated probe-based obstacle model instead of direct right-sensor events.
- Forward-obstacle tests now expect a `probeRightSide` command when right-side status is not fresh or valid.
- Avoidance tests now expect `restoreHeading` before final right/left turn commands after a probe.
- Reverse-escape tests now expect `restoreEscapeHeading` before continuing reverse when the flow includes probe/escape heading restoration.
- Simulator-backed expectations were updated so surrounded escape still requires reverse before lateral escape.

## Added

- Tests for `ProbePose::right` observations through `ObstacleEvent`.
- Tests for probed right side clear leading to right-turn viability.
- Tests for probed right side blocked leading to left-turn viability when left is open.
- Test for front + left + probed-right blocked becoming `surrounded`.
- Test for `RequestRightSideProbe(...)` suspending cleaning before avoidance probing.
- Coverage run using the C++ coverage script.

## Deleted / Removed

- Test references to direct right-sensor events such as `rightBlocked` and `rightInvalidForOpening`.
- Test references to old generic event names such as `frame`, `rawUpdates`, `frontUpdate`, `sideUpdate`, `sensorSnapshot`, and `forwardBlockedNotSurrounded`.
- Old expectation for `probeOrBackupTBD`; tests now use explicit probe, restore, fallback, or stop/fallback commands.

## Coverage

Coverage was run with:

```powershell
cd ..\..\cpp
powershell -NoProfile -ExecutionPolicy Bypass -File .\run-coverage.ps1
```

Result:

- Unit tests passed: `41 / 41`.
- Total line coverage: `87%`.
- Required minimum from the unit-test skill: `75%`.
- Coverage status: passed.
- HTML report copied here: [`coverage.html`](coverage.html) (optional local copy).
- Generated report location: `cpp/build-coverage/coverage/html/index.html`.
- LCOV info file: `cpp/build-coverage/coverage/coverage.filtered.info`.

Coverage by main source area:

- `AutomaticCleaningSession.cpp`: `100%`
- `NavigationAndEscapeCoordinator.cpp`: `96%`
- `ObstaclePerceptionContext.cpp`: `76%`
- `RvcSoftwareController.cpp`: `90%`
- `SurfaceCleaningController.cpp`: `84%`
- Total: `87%`

## Readable Test Runner

For CMD output with GoogleTest color enabled, run:

```cmd
run-tests.bat
```

This builds `rvc_controller_tests` and runs it directly with `--gtest_color=yes`, so passed tests are easier to scan than the default `ctest` output.
