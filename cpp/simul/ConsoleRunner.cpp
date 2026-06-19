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
