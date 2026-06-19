# Roomba RVC — Domain model

Aligned with **`../../01_requirements/RVC_SW_Controller_SRS.md` §3.2** and **`../../02_use_cases/RVC_SW_Controller_UseCases.md`**. This version follows SRS/use case **v0.7.2**: direct **front / left / back** obstacle sensing, **`travelToggle`** (leading travel edge swaps front vs back), **Normal / Boost / Suspended** cleaning, **dust maneuver**, and **right-side probe** using the **front** sensor when Forward toggle and the **back** sensor when Backward toggle.

**Source:** `RVC_domain.puml` · **Re-render:** `powershell -NoProfile -ExecutionPolicy Bypass -File ..\render-diagrams.ps1`

![Domain model](RVC_domain.png)

## SRS §3.2 alignment check

| SRS object (§3.2) | On domain model | Attributes (SRS AT-*) | Status |
|-------------------|-----------------|-------------------------|--------|
| **AutomaticCleaningSession** (§3.2.1) | Yes | `sessionActive`, `sessionSource` | Match |
| **SurfaceCleaningController** (§3.2.2) | Yes | `cleaningMode`, `powerLevel`, `dustSignal` | Match |
| **ObstaclePerceptionContext** (§3.2.3) | Yes | `frontObstacle`, `leftObstacle`, **`backObstacle`**, `rightObstacleInferred`, `rightProbeStatus`, `lastUpdateTime` | Match |
| **NavigationAndEscapeCoordinator** (§3.2.4) | Yes | `motionState`, **`travelToggle`**, `backupDistanceRemaining` | Match |

### Messages on links (SRS §3.2.x.3)

| Link | SRS message(s) | Status |
|------|----------------|--------|
| User → AutomaticCleaningSession | `StartSession`, `StopSession` | Match |
| Direct front/left/back sensors → ObstaclePerceptionContext | `ObstacleStateChanged` | Match |
| Dust → SurfaceCleaningController | `DustSignalUpdated` | Match |
| AutomaticCleaningSession → Navigation / Cleaning | notifies session state; init Forward toggle + Normal mode | Match |
| ObstaclePerceptionContext → NavigationAndEscapeCoordinator | `FusedObstacleSnapshot`, `RequestRightSideProbe` | Match |
| NavigationAndEscapeCoordinator → SurfaceCleaningController | `SuspendCleaningForManeuver`, `ResumeCleaningAfterManeuver`, **`StartBoostCleaning`**, **`EndBoostCleaning`** | Match |
| NavigationAndEscapeCoordinator → wheels | `MotionCommand` (forward, reverse, turn, probe, **spin540**) | Match |
| SurfaceCleaningController → hardware | `CleaningCommand` (**normal / boost / suspend**) | Match |
| Navigation (internal) | **`ToggleTravelDirection`** on dust-maneuver completion | Match |

### What changed from v0.6.0 / v0.7.1

| Was | Now |
|-----|-----|
| Direct sensors front / left only | Direct sensors **front / left / back** |
| Always forward cruise | **`travelToggle`** on NavigationAndEscapeCoordinator |
| Bounded dust boost | **Normal / Boost / Suspended**; dust maneuver + toggle |
| Probe always via front sensor | Probe via **front** or **back** per `travelToggle` |
| No `backObstacle` attribute | **`backObstacle`** on ObstaclePerceptionContext |

`FusedObstacleSnapshot` is an SRS **message** (§3.2.3.3 → §3.2.4.3), not a fifth §3.2 object. Its fields are noted on the diagram (derived by OBJ3-FR-3 and OBJ3-FR-5).

## Multiplicities

| Link | Mult | Note |
|------|------|------|
| User → AutomaticCleaningSession | 0..* — 1 | |
| Direct obstacle sensors → ObstaclePerceptionContext | **3 — 1** | front / left / back (SRS HI-1, HI-1B; back gated by toggle/maneuver context) |
| Dust sensor → SurfaceCleaningController | 1 — 1 | |
| Session → Navigation / Cleaning | 1 — 1 | OBJ1-FR-1, OBJ1-FR-4 |
| ObstaclePerceptionContext → NavigationAndEscapeCoordinator | 1 — 1 | fused picture + probe requests |
| NavigationAndEscapeCoordinator → SurfaceCleaningController | 1 — 1 | |
| NavigationAndEscapeCoordinator → wheel motors | 1 — 2 | MotionCommand incl. dust spin |
| SurfaceCleaningController → cleaning hardware | 1 — 1 | |

## Change summary for SRS/use case/SSD v0.7.2 alignment

### Changed

- External obstacle sensors now **front / left / back** (multiplicity **3 — 1**).
- `ObstaclePerceptionContext` adds **`backObstacle`**.
- `NavigationAndEscapeCoordinator` adds **`travelToggle`**.
- Probe note: **front** vs **back** sensor per toggle.
- Cleaning and motion link labels include **normal/boost/suspend** and **spin540**.

### Added

- Session-start note: initial **Forward** toggle and **Normal** cleaning mode.
- **`StartBoostCleaning` / `EndBoostCleaning`** on navigation → cleaning link.
- **`ToggleTravelDirection`** note on navigation (internal, dust-maneuver completion).

### Preserved

- Four SRS §3.2 domain classes inside software boundary.
- Inferred right via probe (no dedicated right sensor).
- External actors remain outside the software package.
