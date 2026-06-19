# RVC GUI 시뮬레이터 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `cpp/simul` 아래에 사진과 동일한 10x10 보드를 표시하는 Windows GUI 시뮬레이터를 만들고, 비-Windows에서는 같은 31개 시나리오를 콘솔 PASS/FAIL runner로 실행한다.

**Architecture:** 시뮬레이터 코어는 `Scenario`, `BoardLayout`, `ScenarioRunner`를 제공하는 플랫폼 중립 라이브러리로 분리한다. Windows GUI와 콘솔 runner는 같은 코어를 사용하므로 PASS/FAIL 판정과 step 진행은 한 곳에서만 구현된다. CMake target 이름은 기존처럼 `rvc_simulator`로 유지한다.

**Tech Stack:** C++17, CMake, GoogleTest, Windows의 Win32/GDI, 비-Windows 콘솔 runner.

---

## 파일 구조

- Create: `cpp/simul/SimulatorCore.hpp`
  - 보드 셀 타입, 시나리오 action, 시나리오 정의, step trace, 실행 결과, `ScenarioRunner` API를 선언한다.
- Create: `cpp/simul/SimulatorCore.cpp`
  - 승인된 10x10 보드, 기존 30개 시나리오, 신규 `TC-31`, command formatter, step/full 실행 로직을 구현한다.
- Create: `cpp/simul/ConsoleRunner.hpp`
  - 콘솔 runner entry function을 선언한다.
- Create: `cpp/simul/ConsoleRunner.cpp`
  - 비-Windows CI용 PASS/FAIL 출력과 `--verbose` 출력을 구현한다.
- Create: `cpp/simul/Win32Gui.hpp`
  - Windows GUI entry function을 선언한다.
- Create: `cpp/simul/Win32Gui.cpp`
  - Win32/GDI 창, 보드 renderer, 시나리오 선택, step controls, timer auto-run, PASS/FAIL 패널을 구현한다.
- Create: `cpp/simul/rvc_simulator.cpp`
  - 플랫폼별 entry point를 제공한다. Windows는 GUI를 실행하고, 비-Windows는 콘솔 runner를 실행한다.
- Create: `cpp/tests/SimulatorCoreTest.cpp`
  - 플랫폼 중립 코어의 보드, 시나리오 개수, PASS/FAIL, step replay를 검증한다.
- Modify: `cpp/CMakeLists.txt`
  - `rvc_simulator_core` library를 추가하고, `rvc_simulator` source path를 `cpp/simul`로 변경하며, `SimulatorCoreTest.cpp`를 GoogleTest target에 포함한다.
- Modify: `.gitignore`
  - `cpp/simul` 소스가 추적되도록 allow rule을 추가한다.
- Modify: `cpp/README.md`
  - GUI/fallback 동작과 31개 시나리오 실행 방법을 설명한다.
- Modify: `2nd ooad/06_simulator/RVC_Simulator.md`
  - active simulator source와 신규 GUI/TC-31 내용을 설명한다.
- Delete: `cpp/simulator/rvc_simulator.cpp`
  - 새 target이 `cpp/simul/rvc_simulator.cpp`를 사용하므로 기존 단일 파일 시뮬레이터를 제거한다.

## Task 1: 시뮬레이터 코어 테스트 추가

**Files:**
- Create: `cpp/tests/SimulatorCoreTest.cpp`
- Modify: `cpp/CMakeLists.txt`
- Modify: `.gitignore`

- [ ] **Step 1: `cpp/simul` allow rule 추가**

Update `.gitignore` near the existing simulator allow rules:

```gitignore
!/cpp/simul/
!/cpp/simul/*.hpp
!/cpp/simul/*.cpp
```

- [ ] **Step 2: 실패하는 코어 테스트 작성**

Create `cpp/tests/SimulatorCoreTest.cpp`:

```cpp
#include "../simul/SimulatorCore.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

std::size_t countCells(const rvc_simul::BoardLayout& board, rvc_simul::BoardCell cell) {
    std::size_t count = 0;
    for (const auto& row : board.cells) {
        count += static_cast<std::size_t>(std::count(row.begin(), row.end(), cell));
    }
    return count;
}

} // namespace

TEST(SimulatorCoreTest, ReferenceBoardMatchesApprovedImageLayout) {
    const auto board = rvc_simul::referenceBoard();

    ASSERT_EQ(board.cells.size(), 10U);
    for (const auto& row : board.cells) {
        EXPECT_EQ(row.size(), 10U);
    }

    EXPECT_EQ(board.cells[9][1], rvc_simul::BoardCell::RobotStart);
    EXPECT_EQ(board.cells[1][1], rvc_simul::BoardCell::Dust);
    EXPECT_EQ(board.cells[1][8], rvc_simul::BoardCell::Dust);
    EXPECT_EQ(board.cells[8][3], rvc_simul::BoardCell::Dust);
    EXPECT_EQ(board.cells[0][0], rvc_simul::BoardCell::Obstacle);
    EXPECT_EQ(board.cells[9][9], rvc_simul::BoardCell::Obstacle);

    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::Obstacle), 60U);
    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::Dust), 8U);
    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::RobotStart), 1U);
    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::Floor), 31U);
}

TEST(SimulatorCoreTest, ScenarioCatalogContainsExistingScenariosAndPhotoScenario) {
    const auto scenarios = rvc_simul::scenarios();

    ASSERT_EQ(scenarios.size(), 31U);
    EXPECT_EQ(scenarios.front().id, "TC-01");
    EXPECT_EQ(scenarios[29].id, "TC-30");
    EXPECT_EQ(scenarios.back().id, "TC-31");
    EXPECT_EQ(scenarios.back().board.cells[9][1], rvc_simul::BoardCell::RobotStart);
}

TEST(SimulatorCoreTest, AllScenariosPassExpectedCommandTraces) {
    for (const auto& scenario : rvc_simul::scenarios()) {
        const auto result = rvc_simul::runScenario(scenario);
        EXPECT_TRUE(result.passed) << scenario.id << " " << scenario.name;
    }
}

TEST(SimulatorCoreTest, StepExecutionMatchesFullExecutionForPhotoScenario) {
    const auto scenarios = rvc_simul::scenarios();
    const auto photoScenario = std::find_if(
        scenarios.begin(),
        scenarios.end(),
        [](const rvc_simul::Scenario& scenario) { return scenario.id == "TC-31"; });

    ASSERT_NE(photoScenario, scenarios.end());

    rvc_simul::ScenarioRunner runner{*photoScenario};
    while (runner.canAdvance()) {
        runner.advance();
    }

    const auto stepped = runner.result();
    const auto full = rvc_simul::runScenario(*photoScenario);

    EXPECT_TRUE(stepped.passed);
    EXPECT_EQ(stepped.actualMotion, full.actualMotion);
    EXPECT_EQ(stepped.actualCleaning, full.actualCleaning);
}
```

- [ ] **Step 3: CMake에 테스트 파일을 연결**

Modify `cpp/CMakeLists.txt` inside `add_executable(rvc_controller_tests ...)`:

```cmake
        tests/SimulatorCoreTest.cpp
```

Do not add `rvc_simulator_core` yet. This keeps the first test run red because the new header and target do not exist.

- [ ] **Step 4: 실패 확인**

Run:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --target rvc_controller_tests
```

Expected: build fails because `../simul/SimulatorCore.hpp` does not exist.

## Task 2: 플랫폼 중립 시뮬레이터 코어 구현

**Files:**
- Create: `cpp/simul/SimulatorCore.hpp`
- Create: `cpp/simul/SimulatorCore.cpp`
- Modify: `cpp/CMakeLists.txt`

- [ ] **Step 1: 코어 header 작성**

Create `cpp/simul/SimulatorCore.hpp`:

```cpp
#pragma once

#include "rvc/Types.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace rvc_simul {

enum class BoardCell {
    Floor,
    Obstacle,
    Dust,
    RobotStart
};

struct BoardPosition {
    std::size_t row{0};
    std::size_t column{0};
};

struct BoardLayout {
    std::vector<std::vector<BoardCell>> cells;
    std::vector<BoardPosition> route;
};

enum class ActionKind {
    Initialize,
    Start,
    Stop,
    Resume,
    ServiceReset,
    Obstacle,
    Dust
};

struct Action {
    ActionKind kind{ActionKind::Initialize};
    rvc::ObstacleEventKind obstacle{rvc::ObstacleEventKind::forwardSafe};
    rvc::ProbePose probePose{rvc::ProbePose::none};
    rvc::DustSignal dust{rvc::DustSignal::normal};
    bool frontBlocked{false};
    bool leftBlocked{false};
    BoardPosition robotPosition{9, 1};
    std::string note;
};

struct Scenario {
    std::string id;
    std::string name;
    BoardLayout board;
    std::vector<Action> actions;
    std::vector<rvc::MotionCommand> expectedMotion;
    std::vector<rvc::CleaningCommand> expectedCleaning;
    std::vector<std::string> notes;
};

struct StepTrace {
    std::size_t stepIndex{0};
    Action action;
    std::vector<rvc::MotionCommand> newMotion;
    std::vector<rvc::CleaningCommand> newCleaning;
};

struct ScenarioResult {
    bool passed{false};
    std::vector<rvc::MotionCommand> actualMotion;
    std::vector<rvc::CleaningCommand> actualCleaning;
    std::vector<StepTrace> steps;
};

BoardLayout emptyBoard();
BoardLayout referenceBoard();
std::vector<Scenario> scenarios();
ScenarioResult runScenario(const Scenario& scenario);

const char* toString(ActionKind kind);
const char* toString(rvc::MotionCommand command);
const char* toString(rvc::CleaningCommand command);
const char* toString(rvc::ObstacleEventKind event);
const char* toString(rvc::DustSignal signal);
std::string describeAction(const Action& action);
std::string joinMotionCommands(const std::vector<rvc::MotionCommand>& commands);
std::string joinCleaningCommands(const std::vector<rvc::CleaningCommand>& commands);

class ScenarioRunner {
public:
    explicit ScenarioRunner(const Scenario& scenario);

    void reset();
    bool canAdvance() const;
    StepTrace advance();
    void rewindTo(std::size_t completedSteps);
    const Scenario& scenario() const;
    std::size_t completedSteps() const;
    BoardPosition currentRobotPosition() const;
    ScenarioResult result() const;

private:
    Scenario scenario_;
    std::size_t completedSteps_{0};
    ScenarioResult result_;
};

} // namespace rvc_simul
```

- [ ] **Step 2: CMake core library 추가**

Modify `cpp/CMakeLists.txt` after `rvc_controller`:

```cmake
add_library(rvc_simulator_core
    simul/SimulatorCore.cpp
)

target_include_directories(rvc_simulator_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(rvc_simulator_core PUBLIC cxx_std_17)
target_link_libraries(rvc_simulator_core PUBLIC rvc_controller)
rvc_enable_coverage(rvc_simulator_core)
```

Modify `target_link_libraries(rvc_controller_tests ...)`:

```cmake
            rvc_simulator_core
```

- [ ] **Step 3: 실패 확인**

Run:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --target rvc_controller_tests
```

Expected: build fails because `cpp/simul/SimulatorCore.cpp` does not exist.

- [ ] **Step 4: 코어 구현 작성**

Create `cpp/simul/SimulatorCore.cpp`. Move the existing scenario action helpers, command formatters, and 30 scenario definitions from `cpp/simulator/rvc_simulator.cpp` into this file, then add `TC-31` with this data:

```cpp
{"TC-31", "Photo board route with dust and obstacle decision",
 referenceBoard(),
 {initializeAt(9, 1, "Start from the green cell"),
  startAt(8, 1, "Move up the corridor"),
  obstacleAt(rvc::ObstacleEventKind::forwardSafe, 7, 1, "Forward corridor is clear"),
  dustAt(rvc::DustSignal::aboveThreshold, 2, 1, "Dust cell on the approved board"),
  obstacleAt(rvc::ObstacleEventKind::forwardBlocked, 1, 1, "Top wall blocks forward movement"),
  obstacleAt(rvc::ObstacleEventKind::probePoseRightSample, 1, 2, "Right probe sees an opening"),
  obstacleAt(rvc::ObstacleEventKind::forwardSafeAfterManeuver, 1, 3, "Resume after right turn"),
  stopAt(1, 3, "End photo-board scenario")},
 {rvc::MotionCommand::stop,
  rvc::MotionCommand::forward,
  rvc::MotionCommand::probeRightSide,
  rvc::MotionCommand::restoreHeading,
  rvc::MotionCommand::turnRight,
  rvc::MotionCommand::forward,
  rvc::MotionCommand::stop},
 {rvc::CleaningCommand::suspend,
  rvc::CleaningCommand::active,
  rvc::CleaningCommand::boost,
  rvc::CleaningCommand::normal,
  rvc::CleaningCommand::suspend,
  rvc::CleaningCommand::active,
  rvc::CleaningCommand::suspend},
 {"Uses the approved 10x10 board image as the visual simulation map."}}
```

The helper functions used by the scenario should construct `Action` values with `robotPosition` and `note` filled:

```cpp
Action initializeAt(std::size_t row, std::size_t column, std::string note);
Action startAt(std::size_t row, std::size_t column, std::string note);
Action stopAt(std::size_t row, std::size_t column, std::string note);
Action obstacleAt(rvc::ObstacleEventKind event, std::size_t row, std::size_t column, std::string note);
Action dustAt(rvc::DustSignal signal, std::size_t row, std::size_t column, std::string note);
```

The `referenceBoard()` implementation must return the approved 10x10 layout:

```cpp
return {{
    {W, W, W, W, W, W, W, W, W, W},
    {W, D, F, F, F, F, F, F, D, W},
    {W, D, W, W, W, W, W, W, F, W},
    {W, F, W, F, F, F, F, W, F, W},
    {W, F, W, F, W, W, F, W, F, W},
    {W, F, W, F, W, W, F, W, F, W},
    {W, F, W, F, W, D, D, W, F, W},
    {W, F, W, F, W, W, W, W, F, W},
    {W, F, W, D, D, F, F, F, D, W},
    {W, R, W, W, W, W, W, W, W, W},
}, {{9, 1}, {8, 1}, {7, 1}, {6, 1}, {5, 1}, {4, 1}, {3, 1}, {2, 1}, {1, 1}, {1, 2}, {1, 3}}};
```

- [ ] **Step 5: 코어 테스트 통과 확인**

Run:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --target rvc_controller_tests
.\cpp\build\rvc_controller_tests.exe --gtest_filter=SimulatorCoreTest.*
```

Expected: all `SimulatorCoreTest` tests pass.

## Task 3: 콘솔 runner와 simulator target 교체

**Files:**
- Create: `cpp/simul/ConsoleRunner.hpp`
- Create: `cpp/simul/ConsoleRunner.cpp`
- Create: `cpp/simul/rvc_simulator.cpp`
- Modify: `cpp/CMakeLists.txt`
- Delete: `cpp/simulator/rvc_simulator.cpp`

- [ ] **Step 1: 콘솔 runner header 작성**

Create `cpp/simul/ConsoleRunner.hpp`:

```cpp
#pragma once

namespace rvc_simul {

int runConsoleSimulator(int argc, char** argv);

} // namespace rvc_simul
```

- [ ] **Step 2: 콘솔 runner 구현**

Create `cpp/simul/ConsoleRunner.cpp` with these responsibilities:

```cpp
#include "ConsoleRunner.hpp"

#include "SimulatorCore.hpp"

#include <iomanip>
#include <iostream>
#include <string>

namespace rvc_simul {

int runConsoleSimulator(int argc, char** argv) {
    const bool verbose = argc > 1 && std::string(argv[1]) == "--verbose";
    const auto allScenarios = scenarios();

    std::cout << "RVC Software Controller Simulator" << '\n';
    std::cout << "Running " << allScenarios.size() << " scripted scenarios" << '\n';
    std::cout << "Use --verbose to print actions and command traces for every scenario." << '\n';
    std::cout << '\n';

    int passed = 0;
    for (const auto& scenario : allScenarios) {
        const auto result = runScenario(scenario);
        if (result.passed) {
            ++passed;
        }

        std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] "
                  << std::setw(5) << scenario.id << "  " << scenario.name << '\n';

        if (verbose || !result.passed) {
            std::cout << "  Expected motion:  " << joinMotionCommands(scenario.expectedMotion) << '\n';
            std::cout << "  Actual motion:    " << joinMotionCommands(result.actualMotion) << '\n';
            std::cout << "  Expected cleaning:" << joinCleaningCommands(scenario.expectedCleaning) << '\n';
            std::cout << "  Actual cleaning:  " << joinCleaningCommands(result.actualCleaning) << '\n';
            for (const auto& step : result.steps) {
                std::cout << "  Step " << step.stepIndex + 1 << ": "
                          << describeAction(step.action) << '\n';
            }
            for (const auto& note : scenario.notes) {
                std::cout << "  Note: " << note << '\n';
            }
        }
    }

    std::cout << '\n';
    std::cout << "Summary: " << passed << " / " << allScenarios.size() << " scenarios passed." << '\n';

    return passed == static_cast<int>(allScenarios.size()) ? 0 : 1;
}

} // namespace rvc_simul
```

- [ ] **Step 3: platform entry point 작성**

Create `cpp/simul/rvc_simulator.cpp`:

```cpp
#include "ConsoleRunner.hpp"

#ifdef _WIN32
#include "Win32Gui.hpp"
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    return rvc_simul::runWindowsGuiSimulator();
#else
    return rvc_simul::runConsoleSimulator(argc, argv);
#endif
}
```

- [ ] **Step 4: CMake simulator target 교체**

Modify `cpp/CMakeLists.txt`:

```cmake
set(RVC_SIMULATOR_SOURCES
    simul/rvc_simulator.cpp
    simul/ConsoleRunner.cpp
)

if(WIN32)
    list(APPEND RVC_SIMULATOR_SOURCES simul/Win32Gui.cpp)
    add_executable(rvc_simulator WIN32 ${RVC_SIMULATOR_SOURCES})
else()
    add_executable(rvc_simulator ${RVC_SIMULATOR_SOURCES})
endif()

target_link_libraries(rvc_simulator PRIVATE rvc_simulator_core)
rvc_enable_coverage(rvc_simulator)
```

Remove the old block:

```cmake
add_executable(rvc_simulator simulator/rvc_simulator.cpp)
target_link_libraries(rvc_simulator PRIVATE rvc_controller)
rvc_enable_coverage(rvc_simulator)
```

- [ ] **Step 5: 기존 단일 파일 삭제**

Delete `cpp/simulator/rvc_simulator.cpp`.

- [ ] **Step 6: 콘솔 target 검증**

Run:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --target rvc_simulator
.\cpp\build\rvc_simulator.exe --verbose
```

Expected: output includes `Running 31 scripted scenarios`, `[PASS] TC-31`, and `Summary: 31 / 31 scenarios passed.`

## Task 4: Windows GUI 구현

**Files:**
- Create: `cpp/simul/Win32Gui.hpp`
- Create: `cpp/simul/Win32Gui.cpp`

- [ ] **Step 1: GUI header 작성**

Create `cpp/simul/Win32Gui.hpp`:

```cpp
#pragma once

namespace rvc_simul {

int runWindowsGuiSimulator();

} // namespace rvc_simul
```

- [ ] **Step 2: GUI 상태 모델 구현**

In `cpp/simul/Win32Gui.cpp`, define a file-local `GuiState`:

```cpp
#ifdef _WIN32

#include "Win32Gui.hpp"

#include "SimulatorCore.hpp"

#include <windows.h>

#include <string>
#include <vector>

namespace {

constexpr int kCellSize = 42;
constexpr int kBoardLeft = 24;
constexpr int kBoardTop = 72;
constexpr int kPanelLeft = 500;
constexpr int kButtonTop = 520;
constexpr UINT_PTR kAutoTimer = 1;

struct GuiState {
    std::vector<rvc_simul::Scenario> scenarios{rvc_simul::scenarios()};
    std::size_t selectedScenario{30};
    rvc_simul::ScenarioRunner runner{scenarios[selectedScenario]};
    bool autoRunning{false};
    std::vector<rvc_simul::ScenarioResult> runAllResults;
};

} // namespace

#endif
```

- [ ] **Step 3: 보드 renderer 구현**

Implement `drawBoard(HDC hdc, const GuiState& state)`:

```cpp
void fillCell(HDC hdc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void drawCenteredText(HDC hdc, const RECT& rect, const wchar_t* text, COLORREF color) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text, -1, const_cast<RECT*>(&rect), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void drawBoard(HDC hdc, const GuiState& state) {
    const auto& board = state.runner.scenario().board.cells;
    const auto robot = state.runner.currentRobotPosition();

    for (std::size_t row = 0; row < board.size(); ++row) {
        for (std::size_t column = 0; column < board[row].size(); ++column) {
            RECT rect{
                kBoardLeft + static_cast<LONG>(column) * kCellSize,
                kBoardTop + static_cast<LONG>(row) * kCellSize,
                kBoardLeft + static_cast<LONG>(column + 1) * kCellSize,
                kBoardTop + static_cast<LONG>(row + 1) * kCellSize};

            const bool robotHere = robot.row == row && robot.column == column;
            if (robotHere) {
                fillCell(hdc, rect, RGB(125, 157, 69));
                drawCenteredText(hdc, rect, L"1", RGB(240, 245, 210));
            } else if (board[row][column] == rvc_simul::BoardCell::Obstacle) {
                fillCell(hdc, rect, RGB(63, 66, 71));
                drawCenteredText(hdc, rect, L"장", RGB(242, 242, 240));
            } else if (board[row][column] == rvc_simul::BoardCell::Dust) {
                fillCell(hdc, rect, RGB(233, 195, 75));
                drawCenteredText(hdc, rect, L"먼", RGB(101, 80, 24));
            } else {
                fillCell(hdc, rect, RGB(221, 221, 220));
            }

            FrameRect(hdc, &rect, static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
        }
    }
}
```

- [ ] **Step 4: 컨트롤과 window procedure 구현**

Create buttons and a scenario combo box in `WM_CREATE`; handle `WM_COMMAND` for `Previous`, `Next`, `Auto`, `Reset`, and `Run All`; handle `WM_TIMER` to call `runner.advance()` while `canAdvance()` is true; handle `WM_PAINT` to draw the board and status panel.

Control IDs:

```cpp
constexpr int kScenarioComboId = 100;
constexpr int kPrevButtonId = 101;
constexpr int kNextButtonId = 102;
constexpr int kAutoButtonId = 103;
constexpr int kResetButtonId = 104;
constexpr int kRunAllButtonId = 105;
```

Use this behavior:

```cpp
case kPrevButtonId:
    if (state.runner.completedSteps() > 0) {
        state.runner.rewindTo(state.runner.completedSteps() - 1);
    }
    InvalidateRect(hwnd, nullptr, TRUE);
    break;
case kNextButtonId:
    if (state.runner.canAdvance()) {
        state.runner.advance();
    }
    InvalidateRect(hwnd, nullptr, TRUE);
    break;
case kAutoButtonId:
    state.autoRunning = !state.autoRunning;
    if (state.autoRunning) {
        SetTimer(hwnd, kAutoTimer, 700, nullptr);
    } else {
        KillTimer(hwnd, kAutoTimer);
    }
    break;
case kResetButtonId:
    state.autoRunning = false;
    KillTimer(hwnd, kAutoTimer);
    state.runner.reset();
    InvalidateRect(hwnd, nullptr, TRUE);
    break;
case kRunAllButtonId:
    state.runAllResults.clear();
    for (const auto& scenario : state.scenarios) {
        state.runAllResults.push_back(rvc_simul::runScenario(scenario));
    }
    InvalidateRect(hwnd, nullptr, TRUE);
    break;
```

- [ ] **Step 5: Windows GUI 빌드 확인**

Run:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --target rvc_simulator
.\cpp\build\Debug\rvc_simulator.exe
```

Expected: a Windows window opens with the 10x10 board, scenario selector, PASS/FAIL status, command traces, and working manual/automatic controls.

## Task 5: 문서와 OOAD simulator 요약 갱신

**Files:**
- Modify: `cpp/README.md`
- Modify: `2nd ooad/06_simulator/RVC_Simulator.md`
- Modify: `2nd ooad/06_simulator/rvc_simulator.cpp`

- [ ] **Step 1: `cpp/README.md` simulator section 갱신**

Replace the simulator section with this Korean text:

```markdown
## 시뮬레이터

시뮬레이터 target은 `rvc_simulator`이다.

Windows에서는 `cpp/simul`의 Win32/GDI GUI를 연다. GUI는 승인된 10x10 보드를 표시하고, 31개 시나리오 중 하나를 선택할 수 있으며, 각 step을 수동 또는 자동으로 진행한다. 선택된 시나리오의 PASS/FAIL과 기대/실제 command trace도 함께 보여준다.

비-Windows 빌드에서는 같은 target이 CI용 콘솔 시뮬레이터로 실행된다. 콘솔 runner는 31개 전체 시나리오를 실행하고 모든 시나리오가 통과할 때만 exit code `0`을 반환한다.
```

- [ ] **Step 2: `2nd ooad/06_simulator/RVC_Simulator.md` 갱신**

Update these facts:

```markdown
- Active simulator source: `../../cpp/simul`
- Scenario count: `31`
- Windows behavior: GUI board simulator
- Non-Windows behavior: console PASS/FAIL runner
- New scenario: `TC-31`, photo board route with dust and obstacle decision
```

- [ ] **Step 3: OOAD artifact source 복사**

Copy the new platform entry source into `2nd ooad/06_simulator/rvc_simulator.cpp` as a readable artifact snapshot. The first lines should be:

```cpp
#include "../../cpp/simul/ConsoleRunner.hpp"

#ifdef _WIN32
#include "../../cpp/simul/Win32Gui.hpp"
#endif
```

- [ ] **Step 4: 문서 확인**

Run:

```powershell
rg -n "30 scripted|30 / 30|cpp/simulator/rvc_simulator|Running 30" cpp/README.md "2nd ooad/06_simulator/RVC_Simulator.md"
```

Expected: no stale sentence claims that the active simulator has only 30 scenarios or uses `cpp/simulator/rvc_simulator.cpp`.

## Task 6: 전체 검증과 마무리

**Files:**
- All files changed by previous tasks

- [ ] **Step 1: 전체 C++ 테스트 실행**

Run:

```powershell
cmake -S cpp -B cpp/build
cmake --build cpp/build --target rvc_controller_tests
ctest --test-dir cpp/build --output-on-failure
```

Expected: all GoogleTest tests pass, including `SimulatorCoreTest`.

- [ ] **Step 2: simulator target 실행**

Run:

```powershell
cmake --build cpp/build --target rvc_simulator
.\cpp\build\rvc_simulator.exe --verbose
```

Expected on non-GUI console build: `Summary: 31 / 31 scenarios passed.`

Expected on Windows GUI build: the GUI opens. Use `Run All` and confirm the summary shows `31 / 31`.

- [ ] **Step 3: git 상태 확인**

Run:

```powershell
git -c safe.directory=C:/Users/yhj/Documents/OOAD-with-AI status --short
```

Expected: only intentional simulator, test, CMake, ignore, and documentation files are modified.

- [ ] **Step 4: 구현 커밋 생성**

Run:

```powershell
git -c safe.directory=C:/Users/yhj/Documents/OOAD-with-AI add -f .gitignore cpp/CMakeLists.txt cpp/simul cpp/tests/SimulatorCoreTest.cpp cpp/README.md "2nd ooad/06_simulator/RVC_Simulator.md" "2nd ooad/06_simulator/rvc_simulator.cpp"
git -c safe.directory=C:/Users/yhj/Documents/OOAD-with-AI rm cpp/simulator/rvc_simulator.cpp
git -c safe.directory=C:/Users/yhj/Documents/OOAD-with-AI commit -m "feat: replace simulator with gui board runner"
```

Expected: commit succeeds with only the intended simulator replacement files.

## Self-Review

- Spec coverage: The plan covers `cpp/simul`, Windows GUI, non-Windows console fallback, 31 scenarios, approved board layout, PASS/FAIL display, manual/automatic stepping, CMake target preservation, scripts staying valid, and documentation updates.
- Placeholder scan: The plan does not use unresolved placeholder language. Mentions of platform-specific behavior are concrete.
- Type consistency: `BoardCell`, `BoardLayout`, `Action`, `Scenario`, `ScenarioResult`, `StepTrace`, `ScenarioRunner`, `runScenario`, `scenarios`, `referenceBoard`, `runConsoleSimulator`, and `runWindowsGuiSimulator` are named consistently across tasks.
