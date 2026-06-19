#ifdef _WIN32

#include "Win32Gui.hpp"

#include "SimulatorCore.hpp"

#include <windows.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int kCellSize = 42;
constexpr int kBoardLeft = 24;
constexpr int kBoardTop = 72;
constexpr int kPanelLeft = 476;
constexpr int kButtonTop = 524;
constexpr int kWindowWidth = 900;
constexpr int kWindowHeight = 650;
constexpr UINT_PTR kAutoTimer = 1;

constexpr int kScenarioComboId = 100;
constexpr int kPrevButtonId = 101;
constexpr int kNextButtonId = 102;
constexpr int kAutoButtonId = 103;
constexpr int kResetButtonId = 104;
constexpr int kRunAllButtonId = 105;

HMENU controlId(int id) {
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

struct GuiState {
    GuiState() : scenarios(rvc_simul::scenarios()),
                 selectedScenario(scenarios.empty() ? 0 : scenarios.size() - 1),
                 runner(scenarios[selectedScenario]) {}

    void selectScenario(std::size_t index) {
        if (index >= scenarios.size()) {
            return;
        }
        selectedScenario = index;
        runner = rvc_simul::ScenarioRunner{scenarios[selectedScenario]};
        autoRunning = false;
    }

    std::vector<rvc_simul::Scenario> scenarios;
    std::size_t selectedScenario{0};
    rvc_simul::ScenarioRunner runner;
    bool autoRunning{false};
    std::vector<rvc_simul::ScenarioResult> runAllResults;
};

std::wstring widen(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

GuiState* stateFrom(HWND hwnd) {
    return reinterpret_cast<GuiState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

void fillRect(HDC hdc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void drawText(HDC hdc, RECT rect, const std::wstring& text, UINT format, COLORREF color = RGB(30, 30, 30)) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text.c_str(), -1, &rect, format);
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
                fillRect(hdc, rect, RGB(125, 157, 69));
                drawText(hdc, rect, L"1", DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(240, 245, 210));
            } else if (board[row][column] == rvc_simul::BoardCell::Obstacle) {
                fillRect(hdc, rect, RGB(63, 66, 71));
                drawText(hdc, rect, L"\uC7A5", DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(242, 242, 240));
            } else if (board[row][column] == rvc_simul::BoardCell::Dust) {
                fillRect(hdc, rect, RGB(233, 195, 75));
                drawText(hdc, rect, L"\uBA3C", DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(101, 80, 24));
            } else {
                fillRect(hdc, rect, RGB(221, 221, 220));
            }

            FrameRect(hdc, &rect, static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
        }
    }
}

std::wstring scenarioStatusText(const GuiState& state) {
    const auto result = state.runner.result();
    std::wostringstream stream;
    stream << L"Step " << state.runner.completedSteps() << L" / "
           << state.runner.scenario().actions.size() << L"\n";

    if (state.runner.completedSteps() == state.runner.scenario().actions.size()) {
        stream << L"Status: " << (result.passed ? L"PASS" : L"FAIL") << L"\n";
    } else {
        stream << L"Status: RUNNING\n";
    }

    if (state.runner.completedSteps() > 0 && !result.steps.empty()) {
        stream << L"Current: " << widen(rvc_simul::describeAction(result.steps.back().action)) << L"\n";
    }

    stream << L"\nExpected motion:\n" << widen(rvc_simul::joinMotionCommands(state.runner.scenario().expectedMotion));
    stream << L"\n\nActual motion:\n" << widen(rvc_simul::joinMotionCommands(result.actualMotion));
    stream << L"\n\nExpected cleaning:\n"
           << widen(rvc_simul::joinCleaningCommands(state.runner.scenario().expectedCleaning));
    stream << L"\n\nActual cleaning:\n" << widen(rvc_simul::joinCleaningCommands(result.actualCleaning));

    if (!state.runAllResults.empty()) {
        const auto passed = std::count_if(state.runAllResults.begin(), state.runAllResults.end(),
                                          [](const rvc_simul::ScenarioResult& result) {
                                              return result.passed;
                                          });
        stream << L"\n\nRun All: " << passed << L" / " << state.runAllResults.size() << L" passed";
    }

    return stream.str();
}

void drawPanel(HDC hdc, const GuiState& state) {
    RECT title{kBoardLeft, 20, kWindowWidth - 24, 54};
    drawText(hdc, title, L"RVC GUI Simulator", DT_LEFT | DT_VCENTER | DT_SINGLELINE, RGB(20, 20, 20));

    RECT scenarioRect{kPanelLeft, 72, kWindowWidth - 24, 122};
    const auto& scenario = state.runner.scenario();
    drawText(hdc, scenarioRect, widen(scenario.id + " - " + scenario.name), DT_LEFT | DT_WORDBREAK);

    RECT statusRect{kPanelLeft, 136, kWindowWidth - 24, 500};
    drawText(hdc, statusRect, scenarioStatusText(state), DT_LEFT | DT_TOP | DT_WORDBREAK);
}

void resetAuto(HWND hwnd, GuiState& state) {
    state.autoRunning = false;
    KillTimer(hwnd, kAutoTimer);
}

void runAll(GuiState& state) {
    state.runAllResults.clear();
    for (const auto& scenario : state.scenarios) {
        state.runAllResults.push_back(rvc_simul::runScenario(scenario));
    }
}

void createControls(HWND hwnd, GuiState& state) {
    HWND combo = CreateWindowW(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                              kPanelLeft, 24, 370, 240, hwnd,
                              controlId(kScenarioComboId), GetModuleHandleW(nullptr), nullptr);
    for (const auto& scenario : state.scenarios) {
        const std::wstring item = widen(scenario.id + " - " + scenario.name);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    }
    SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(state.selectedScenario), 0);

    CreateWindowW(L"BUTTON", L"Previous", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                  24, kButtonTop, 100, 32, hwnd, controlId(kPrevButtonId),
                  GetModuleHandleW(nullptr), nullptr);
    CreateWindowW(L"BUTTON", L"Next", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                  132, kButtonTop, 100, 32, hwnd, controlId(kNextButtonId),
                  GetModuleHandleW(nullptr), nullptr);
    CreateWindowW(L"BUTTON", L"Auto", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                  240, kButtonTop, 100, 32, hwnd, controlId(kAutoButtonId),
                  GetModuleHandleW(nullptr), nullptr);
    CreateWindowW(L"BUTTON", L"Reset", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                  348, kButtonTop, 100, 32, hwnd, controlId(kResetButtonId),
                  GetModuleHandleW(nullptr), nullptr);
    CreateWindowW(L"BUTTON", L"Run All", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                  456, kButtonTop, 100, 32, hwnd, controlId(kRunAllButtonId),
                  GetModuleHandleW(nullptr), nullptr);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        auto state = std::make_unique<GuiState>();
        GuiState* rawState = state.get();
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state.release()));
        createControls(hwnd, *rawState);
        return 0;
    }
    case WM_COMMAND: {
        GuiState* state = stateFrom(hwnd);
        if (!state) {
            return 0;
        }

        const int controlId = LOWORD(wParam);
        if (controlId == kScenarioComboId && HIWORD(wParam) == CBN_SELCHANGE) {
            HWND combo = reinterpret_cast<HWND>(lParam);
            const auto selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
            resetAuto(hwnd, *state);
            state->selectScenario(static_cast<std::size_t>(selected));
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }

        switch (controlId) {
        case kPrevButtonId:
            if (state->runner.completedSteps() > 0) {
                state->runner.rewindTo(state->runner.completedSteps() - 1);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        case kNextButtonId:
            if (state->runner.canAdvance()) {
                state->runner.advance();
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        case kAutoButtonId:
            state->autoRunning = !state->autoRunning;
            if (state->autoRunning) {
                SetTimer(hwnd, kAutoTimer, 700, nullptr);
            } else {
                KillTimer(hwnd, kAutoTimer);
            }
            return 0;
        case kResetButtonId:
            resetAuto(hwnd, *state);
            state->runner.reset();
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        case kRunAllButtonId:
            runAll(*state);
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        default:
            return 0;
        }
    }
    case WM_TIMER: {
        GuiState* state = stateFrom(hwnd);
        if (state && wParam == kAutoTimer) {
            if (state->runner.canAdvance()) {
                state->runner.advance();
            } else {
                resetAuto(hwnd, *state);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        GuiState* state = stateFrom(hwnd);
        if (state) {
            drawBoard(hdc, *state);
            drawPanel(hdc, *state);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: {
        if (GuiState* state = stateFrom(hwnd)) {
            resetAuto(hwnd, *state);
            delete state;
        }
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

} // namespace

namespace rvc_simul {

int runWindowsGuiSimulator() {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"RvcGuiSimulatorWindow";

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&windowClass)) {
        return 1;
    }

    HWND hwnd = CreateWindowExW(0, className, L"RVC GUI Simulator",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, kWindowWidth, kWindowHeight,
                                nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

} // namespace rvc_simul

#endif
