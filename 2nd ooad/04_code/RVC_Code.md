# Roomba RVC — Code Summary

Companion to the updated [SRS](../01_requirements/RVC_SW_Controller_SRS.md), [class diagram](../03_diagrams/class/RVC_Class_Diagram.md), and [sequence diagrams](../03_diagrams/sd/RVC_SD_Index.md).

The actual C++ project is in [`../../cpp`](../../cpp). This document summarizes how the code now works and what changed from the prior code version.

## How the Updated Code Works

- `RvcSoftwareController` owns the main software objects and exposes input ports for session, obstacle, and dust events.
- `ObstaclePerceptionContext` receives `ObstacleStateChanged(...)`, stores direct `frontObstacle` / `leftObstacle` state, and stores inferred right-side state as `rightObstacleInferred` with `rightProbeStatus`.
- When right-side knowledge is needed, `ObstaclePerceptionContext` asks `NavigationAndEscapeCoordinator` through `RequestRightSideProbe(...)`.
- `NavigationAndEscapeCoordinator` commands `probeRightSide`, then later restores heading with `restoreHeading` or `restoreEscapeHeading` before final avoidance or escape.
- If front, left, and inferred right are all blocked, the code treats the robot as surrounded and commands reverse escape. This is real backward movement, not a 180-degree turn followed by forward travel.
- `SurfaceCleaningController` suspends cleaning during maneuvers and resumes cleaning when forward motion is safe again.

## Changed

- Direct right-side obstacle state was changed to inferred right-side state.
- Forward-obstacle avoidance now probes the right side before choosing right or left.
- Surrounded detection now uses `frontObstacle && leftObstacle && rightObstacleInferred`.
- Probe-pose observations are represented with `ObstacleStateChanged(...)` using `ProbePose::right` and the front sensor result from that pose.
- Simulator and tests now include probe-based right-side checking and surrounded reverse escape.

## Added

- `ProbeStatus`, `ProbeReason`, and `ProbePose` in `Types.hpp`.
- `rightObstacleInferred_` and `rightProbeStatus_` in `ObstaclePerceptionContext`.
- `RequestRightSideProbe(...)` in `NavigationAndEscapeCoordinator`.
- Motion commands for `probeRightSide`, `restoreHeading`, `restoreEscapeHeading`, `fallbackOrEscalateTBD`, and `stopOrFallbackTBD`.
- Unit test coverage for front + left + probed-right blocked becoming `surrounded`.
- Simulator scenarios for right-side probe clear, right-side probe blocked, probe timeout, and reverse-before-turn escape.

## Deleted / Removed

- Direct `rightObstacle_` storage.
- Direct right-sensor event names such as `rightBlocked` and `rightInvalidForOpening`.
- Old generic obstacle event names such as `rawUpdates`, `frontUpdate`, `sideUpdate`, `sensorSnapshot`, and `forwardBlockedNotSurrounded`.
- `probeOrBackupTBD`, replaced by clearer probe and fallback commands.

## Verification

- Build passed with CMake/MSBuild.
- Unit tests passed: `41 / 41`.
- Simulator scenarios passed: `30 / 30`.
