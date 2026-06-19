# RVC GUI Simulator Design

Date: 2026-06-19

## Goal

Replace the existing scripted console simulator with a simulator under `cpp/simul` that can show a photo-matched 10x10 board in a Windows GUI, step through each simulation manually or automatically, and report PASS/FAIL for every scenario.

The simulator must include all 30 existing scripted scenarios plus one new scenario based on the provided board image.

## Context

The current active simulator is `cpp/simulator/rvc_simulator.cpp`. It drives `rvc::RvcSoftwareController` through 30 scripted scenarios and compares recorded motion and cleaning commands with expected command lists.

The CMake target is currently named `rvc_simulator`, and CI builds and runs that target on Ubuntu. The replacement must preserve that target name so scripts and CI continue to work.

The requested replacement location is `cpp/simul`. The old `cpp/simulator` implementation will be replaced by the new simulator target source path.

## Chosen Approach

Use C++17 with a platform split:

- Windows builds: a Win32/GDI GUI executable.
- Non-Windows builds: a console runner using the same scenario engine.

This keeps the Windows user experience graphical without adding Qt, SDL, SFML, or other external GUI dependencies. It also keeps Ubuntu CI stable because it can continue to build and run `rvc_simulator` without needing a display server.

## Board Graphic

The GUI board is a 10x10 grid matching the provided reference image:

- Dark gray cells are obstacles and display the Korean obstacle label from the image.
- Yellow cells are dust and display the Korean dust label from the image.
- Light gray cells are free floor.
- The robot starts at row 10, column 2 with a green highlighted cell and numeric label `1`.

The approved board layout is:

```text
Row 01: W W W W W W W W W W
Row 02: W D . . . . . . D W
Row 03: W D W W W W W W . W
Row 04: W . W . . . . W . W
Row 05: W . W . W W . W . W
Row 06: W . W . W W . W . W
Row 07: W . W . W D D W . W
Row 08: W . W . W W W W . W
Row 09: W . W D D . . . D W
Row 10: W R W W W W W W W W
```

Legend:

- `W`: obstacle/wall cell
- `D`: dust cell
- `R`: robot start cell
- `.`: free floor

## Simulator Model

The simulator core is separated from presentation:

- Scenario definitions describe actions, expected motion commands, expected cleaning commands, optional board events, and notes.
- A scenario runner creates a fresh `RvcSoftwareController` for each scenario.
- Recording sinks capture motion and cleaning commands.
- Each step applies one action, captures any newly emitted commands, and updates the scenario result.
- The final PASS/FAIL result is computed by comparing actual command traces against expected traces.

This keeps GUI interaction deterministic and lets the console runner use the same behavior.

## Scenario Coverage

The replacement simulator includes:

- Existing `TC-01` through `TC-30` with the same action sequences and expected command traces.
- New `TC-31`, based on the approved reference board.

`TC-31` uses the photo-matched board as its visual state and exercises a path through the board with obstacles and dust cells. It must report PASS/FAIL like the existing scenarios and must be available in manual and automatic GUI stepping.

## Windows GUI Behavior

The Windows GUI contains:

- A photo-matched 10x10 board.
- A scenario selector covering all 31 scenarios.
- PASS/FAIL status for the selected scenario and aggregate run.
- Current step index and action name.
- Expected and actual motion command traces.
- Expected and actual cleaning command traces.
- Controls for `Previous`, `Next`, `Auto`, `Reset`, and `Run All`.

Manual stepping advances one action at a time. Automatic stepping uses a timer to advance through the selected scenario. `Run All` executes every scenario to completion and updates the PASS/FAIL summary.

The board renderer uses stable cell sizes, consistent colors, and centered labels to match the provided image. The GUI remains usable on a typical desktop window without requiring resizing.

## Console Fallback Behavior

On non-Windows builds, the same `rvc_simulator` target compiles as a console executable. It runs all 31 scenarios, prints PASS/FAIL lines, supports `--verbose`, and exits with status code `0` only when all scenarios pass.

This is the CI path and preserves the behavior expected by `.github/workflows/cpp-ci.yml`.

## Build And Script Integration

`cpp/CMakeLists.txt` will change the simulator source path from `simulator/rvc_simulator.cpp` to the new `simul` implementation. The executable target remains `rvc_simulator`.

The existing scripts remain valid:

- `cpp/run-simulator.ps1`
- `cpp/run-simulator.bat`
- `2nd ooad/06_simulator/run-simulator.ps1`
- `2nd ooad/06_simulator/run-simulator.bat`

The `cpp/README.md` and `2nd ooad/06_simulator/RVC_Simulator.md` will be updated to describe the new GUI/fallback behavior and the 31-scenario set.

## Error Handling

If a scenario action emits unexpected commands, the result is FAIL and the GUI displays both expected and actual traces.

If a user switches scenario while auto mode is active, auto mode stops and the selected scenario resets to its initial state.

If `Previous` is used, the runner rebuilds the selected scenario from the beginning up to the requested step. This avoids mutable rollback bugs and keeps the simulation deterministic.

## Testing Strategy

Tests focus on the platform-neutral simulator core:

- Verify the approved reference board has exactly 10 rows and 10 columns.
- Verify the approved board has the expected start, obstacle, and dust cells.
- Verify all 31 scenarios are present.
- Verify the scenario runner reports pass for the known expected traces.
- Verify step-by-step execution accumulates the same traces as full execution.

Existing CMake/CTest coverage remains in place for the RVC controller classes. The non-Windows console runner continues to provide CI smoke coverage for the simulator target.

## Out Of Scope

- Installing third-party GUI frameworks.
- Creating a web-based simulator.
- Changing the controller business logic to satisfy simulator visuals.
- Replacing the existing unit tests.
- Adding map editing or user-authored scenarios.
