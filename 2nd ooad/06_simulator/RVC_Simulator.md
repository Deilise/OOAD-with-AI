# Roomba RVC — Simulator Summary

Companion to the updated [code summary](../04_code/RVC_Code.md), [unit test summary](../05_unit_tests/RVC_Unit_Tests.md), [SRS](../01_requirements/RVC_SW_Controller_SRS.md), and [sequence diagrams](../03_diagrams/sd/RVC_SD_Index.md).

The active simulator source is [`../../cpp/simulator/rvc_simulator.cpp`](../../cpp/simulator/rvc_simulator.cpp), and a copied artifact is stored here as [`rvc_simulator.cpp`](rvc_simulator.cpp). This folder also includes run scripts that build and run the simulator from the OOAD artifact folder.

## How the Simulator Works

- The simulator creates a `RvcSoftwareController` with recording motion and cleaning sinks.
- Each scenario sends scripted actions such as `StartSession`, `ObstacleStateChanged`, `DustSignalUpdated`, and `StopSession`.
- The simulator compares actual motion and cleaning commands against expected command lists.
- It reports `[PASS]` or `[FAIL]` for each scenario and exits with failure if any scenario fails.
- The updated scenarios model direct front/left obstacle input and right-side checking through `ProbePose::right`.

## Simulator Tests

### Positive / Negative Index

| Positive | Negative |
|----------|----------|
| `TC-01` | `TC-04` |
| `TC-02` | `TC-05` |
| `TC-03` | `TC-09` |
| `TC-06` | `TC-10` |
| `TC-07` | `TC-14` |
| `TC-08` | `TC-20` |
| `TC-11` | `TC-21` |
| `TC-12` | `TC-22` |
| `TC-13` | `TC-23` |
| `TC-15` | `TC-25` |
| `TC-16` | |
| `TC-17` | |
| `TC-18` | |
| `TC-19` | |
| `TC-24` | |
| `TC-26` | |
| `TC-27` | |
| `TC-28` | |
| `TC-29` | |
| `TC-30` | |

### Detailed Simulator Tests

| Test | Type | What It Tests |
|------|------|---------------|
| `TC-01` | Positive | Initialization enters inactive safe state with `stop` and `suspend`. |
| `TC-02` | Positive | Start plus safe forward state commands forward cleaning. |
| `TC-03` | Positive | Dust boost turns on and returns to normal while active. |
| `TC-04` | Negative | Dust boost is deferred when no session is active. |
| `TC-05` | Negative | Invalid dust signal keeps normal cleaning. |
| `TC-06` | Positive | Stop session stops motion and suspends cleaning. |
| `TC-07` | Positive | Resume after stop can re-enter forward cleaning. |
| `TC-08` | Positive | Service/reset request returns to inactive safe state. |
| `TC-09` | Negative | Invalid obstacle data triggers full fault stop. |
| `TC-10` | Negative | Partial stale obstacle data uses gradual stop behavior. |
| `TC-11` | Positive | Recovered obstacle snapshot is accepted without extra command. |
| `TC-12` | Positive | Forward blocked, right probe clear, then right turn. |
| `TC-13` | Positive | Forward blocked, probed right blocked, then left turn. |
| `TC-14` | Negative | Probe timeout uses `stopOrFallbackTBD`. |
| `TC-15` | Positive | Front + left + probed right blocked starts reverse escape. |
| `TC-16` | Positive | Reverse readings continue backing up. |
| `TC-17` | Positive | Reverse cycle sample can request right-side probe and continue reverse. |
| `TC-18` | Positive | Lateral opening escapes right. |
| `TC-19` | Positive | Left opening escapes left. |
| `TC-20` | Negative | Max backup without opening uses fallback. |
| `TC-21` | Negative | No lateral opening within limits uses fallback. |
| `TC-22` | Negative | Dropout during reverse enters UC-08 style stop. |
| `TC-23` | Negative | Ambiguous obstacle snapshot waits for later policy. |
| `TC-24` | Positive | Front/left sample builds consistency snapshot only. |
| `TC-25` | Negative | Ambiguous probe alignment waits for later policy. |
| `TC-26` | Positive | Forward safe after maneuver resumes cleaning. |
| `TC-27` | Positive | Forward blocked after maneuver requests right-side probe. |
| `TC-28` | Positive | Reverse occurs before right lateral escape. |
| `TC-29` | Positive | Reverse occurs before left lateral escape. |
| `TC-30` | Positive | Initialize, start, forward cleaning, dust boost, trap escape, resume, and stop. |

## Changed

- Simulator scenarios now use the right-side probe model instead of direct right-side sensor events.
- Forward-blocked scenarios now expect `probeRightSide` before the final turn decision when right status is needed.
- Avoidance scenarios now expect `restoreHeading` before `turnRight` or `turnLeft`.
- Surrounded escape scenarios now verify reverse movement before lateral escape.
- Scenario descriptions were updated to remove old direct-right-sensor wording.

## Added

- `ProbePose::right` data in simulator obstacle actions.
- Copied simulator source artifact: `rvc_simulator.cpp`.
- Scenario for right-side probe clear leading to right turn.
- Scenario for right-side probe blocked leading to left turn.
- Scenario for probe timeout using `stopOrFallbackTBD`.
- Scenario for front + left + probed-right blocked becoming surrounded and starting reverse.
- Run scripts in this folder: `run-simulator.ps1` and `run-simulator.bat`.

## Deleted / Removed

- Direct right-side sensor simulator events such as `rightBlocked` and `rightInvalidForOpening`.
- Old generic event names such as `frame`, `rawUpdates`, `frontUpdate`, `sideUpdate`, `sensorSnapshot`, and `forwardBlockedNotSurrounded`.
- Old `probeOrBackupTBD` simulator expectation.
- Scenario wording that implied a physical right-side obstacle sensor.

## Latest Run Result

The simulator was run after the probe-model update:

- Total scenarios: `30`
- Passed: `30`
- Failed: `0`
- Status: passed
