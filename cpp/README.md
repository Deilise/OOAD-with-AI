# Roomba RVC Software Controller C++ Skeleton

This directory contains a C++17 skeleton generated from the SSDs, SDs, and class diagram. It is aligned with the v0.6.0 design where the RVC has direct front/left obstacle sensing and infers right-side status through a high-level right-side probe.

## Structure

- `include/rvc/Types.hpp` defines the value types and enums from the class diagram.
- `include/rvc/Ports.hpp` defines the external input ports and output command sinks.
- `include/rvc/*Controller.hpp` and `include/rvc/*Session.hpp` define the four main SRS objects plus the composition root.
- `src/*.cpp` implements basic control flow that follows the SSD/SD messages.
- `examples/demo.cpp` shows one simple run through start, cruise, dust boost, surrounded escape, and stop messages.
- `simul/` contains the shared simulator core, Windows GUI runner, and non-Windows console runner for 31 scripted RVC scenarios.

## Build

Recommended CMake build:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build
.\cpp\build\rvc_demo.exe
```

If you want to compile with `g++` directly, run one of these from `cpp/`:

```powershell
.\build-demo.ps1
```

or:

```cmd
build-demo.bat
```

The direct compile command must include the header path and the implementation files:

```powershell
g++ -std=c++17 -I include examples/demo.cpp src/AutomaticCleaningSession.cpp src/NavigationAndEscapeCoordinator.cpp src/ObstaclePerceptionContext.cpp src/RvcSoftwareController.cpp src/SurfaceCleaningController.cpp -o build/demo.exe
```

## Simulator

The simulator target is `rvc_simulator`.

On Windows, running the simulator with no arguments opens the Win32/GDI GUI from `cpp/simul`. The GUI shows the approved 10x10 board, lets the user select one of 31 scenarios, advances steps manually or automatically, and displays PASS/FAIL plus expected/actual command traces.

On non-Windows builds, the same target runs as a console simulator for CI. It executes all 31 scenarios and exits with code `0` only when every scenario passes.

The 31 scenarios include the existing controller scenarios plus `TC-31`, the photo-board route with dust and obstacle decision. Key examples:

- `TC-12`: forward blocked, right-side probe clear, then turn right.
- `TC-13`: forward blocked, right-side probe blocked, then turn left.
- `TC-28`: going back then turning right (`reverse` before `lateralEscapeRight`).
- `TC-29`: going back then turning left (`reverse` before `lateralEscapeLeft`).
- `TC-31`: approved 10x10 board route with dust and top-wall obstacle decision.

From `cpp/`, run:

```powershell
.\run-simulator.ps1
```

Or use:

```cmd
run-simulator.bat
```

For detailed action and command traces:

```powershell
.\run-simulator.ps1 --verbose
```

On Windows, `--verbose` uses the console verification path instead of opening the GUI.

## Test

The project includes GoogleTest tests in `tests/`, with one test file per main class:

- `AutomaticCleaningSessionTest.cpp`
- `ObstaclePerceptionContextTest.cpp`
- `NavigationAndEscapeCoordinatorTest.cpp`
- `SimulatorCoreTest.cpp`
- `SurfaceCleaningControllerTest.cpp`
- `RvcSoftwareControllerTest.cpp`

CMake downloads GoogleTest automatically during configure.

From the repository root:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build
ctest --test-dir cpp/build --output-on-failure
```

For more human-readable GoogleTest output with colored pass/fail lines, run:

```cmd
cpp\run-tests.bat
```

Or run the test executable directly:

```powershell
.\cpp\build\rvc_controller_tests.exe
```

## CI

GitHub Actions is configured in `.github/workflows/cpp-ci.yml`.

On pushes and pull requests, CI runs `cppcheck` static analysis on the RVC headers and source files. It then builds this project, runs GoogleTest, runs the 31-scenario simulator as the system-test replacement, generates coverage, and uploads artifacts.

## Coverage

Coverage is available for GCC/Clang builds through `gcov` and `lcov`. The coverage script removes and recreates `build-coverage` each time so CMake does not reuse an old non-coverage compiler cache.

On Windows, use an MSYS2 UCRT64 or CLANG64 compiler. Older MinGW Win32-thread builds may compile the demo but cannot build GoogleTest because their standard library lacks `std::mutex`.

From `cpp/`, run:

```powershell
.\run-coverage.ps1
```

or:

```cmd
run-coverage.bat
```

The coverage build uses `cpp/build-coverage` and leaves the normal build directory alone. When `lcov`, `genhtml`, and `gcov` are installed, the script writes:

- `build-coverage/coverage/coverage.filtered.info`
- `build-coverage/coverage/html/index.html`

On Windows with MSYS2 UCRT64, install coverage tools with:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-lcov
```

On Linux, install `lcov` from your package manager (for example `sudo apt-get install lcov`).

If coverage still reports `0%`, check the configure output. Coverage requires `g++` or Clang; MSVC builds are not supported by this gcov/lcov setup.

If the script says no compiler with `std::mutex` support was found, install MSYS2:

```powershell
winget install MSYS2.MSYS2
```

Then open `MSYS2 UCRT64` and run:

```sh
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-lcov
```

After that, rerun `.\run-coverage.ps1`. It will automatically prefer `C:\msys64\ucrt64\bin\g++.exe`.

You can also use the CMake target directly:

```powershell
cmake -S cpp -B cpp/build-coverage -DRVC_ENABLE_COVERAGE=ON -DCMAKE_CXX_COMPILER=C:\msys64\ucrt64\bin\g++.exe
cmake --build cpp/build-coverage --target rvc_controller_tests
cmake --build cpp/build-coverage --target coverage
```

The CMake `coverage` target writes the same `coverage.filtered.info` and `coverage/html/index.html` files under the chosen build directory.

## Trace Notes

The class and operation names intentionally keep the PlantUML naming style:

- `AutomaticCleaningSession`
- `ObstaclePerceptionContext`
- `NavigationAndEscapeCoordinator`
- `SurfaceCleaningController`
- `StartSession(...)`, `StopSession()`, `ObstacleStateChanged(...)`, `RequestRightSideProbe(...)`, `FusedObstacleSnapshot(...)`, `DustSignalUpdated(...)`
- `MotionCommand(...)` and `CleaningCommand(...)` on external sink interfaces

The code does not model a direct right-side obstacle sensor. `ObstaclePerceptionContext` stores `rightObstacleInferred` and `rightProbeStatus`; `ObstacleEvent` can carry `probePose=right` plus the front-sensor result observed in that probe pose. `NavigationAndEscapeCoordinator` treats the probe as a separate maneuver (`probeRightSide`, then `restoreHeading` or `restoreEscapeHeading`) before issuing the final turn or reverse command.

Policy marked `TBD` in the diagrams/SRS is represented with corresponding `*TBD` enum values and conservative placeholder handling.
