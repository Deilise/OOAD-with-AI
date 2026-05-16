#include "rvc/RvcSoftwareController.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

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
    ActionKind kind;
    rvc::ObstacleEventKind obstacle{rvc::ObstacleEventKind::TBD};
    rvc::DustSignal dust{rvc::DustSignal::TBD};
};

struct Scenario {
    std::string id;
    std::string name;
    std::vector<Action> actions;
    std::vector<rvc::MotionCommand> expectedMotion;
    std::vector<rvc::CleaningCommand> expectedCleaning;
    std::vector<std::string> notes;
};

class RecordingMotionSink final : public rvc::MotionCommandSink {
public:
    void MotionCommand(rvc::MotionCommand command) override {
        commands.push_back(command);
    }

    std::vector<rvc::MotionCommand> commands;
};

class RecordingCleaningSink final : public rvc::CleaningCommandSink {
public:
    void CleaningCommand(rvc::CleaningCommand command) override {
        commands.push_back(command);
    }

    std::vector<rvc::CleaningCommand> commands;
};

const char* toString(rvc::MotionCommand command) {
    switch (command) {
    case rvc::MotionCommand::forward: return "MotionCommand(forward)";
    case rvc::MotionCommand::turnRight: return "MotionCommand(turnRight)";
    case rvc::MotionCommand::turnLeft: return "MotionCommand(turnLeft)";
    case rvc::MotionCommand::reverse: return "MotionCommand(reverse)";
    case rvc::MotionCommand::continueReverse: return "MotionCommand(continueReverse)";
    case rvc::MotionCommand::lateralEscapeRight: return "MotionCommand(lateralEscapeRight)";
    case rvc::MotionCommand::lateralEscapeLeft: return "MotionCommand(lateralEscapeLeft)";
    case rvc::MotionCommand::forbidForward: return "MotionCommand(forbidForward)";
    case rvc::MotionCommand::stop: return "MotionCommand(stop)";
    case rvc::MotionCommand::fallbackTBD: return "MotionCommand(fallbackTBD)";
    case rvc::MotionCommand::probeOrBackupTBD: return "MotionCommand(probeOrBackupTBD)";
    case rvc::MotionCommand::gradualOrPartialStopTBD: return "MotionCommand(gradualOrPartialStopTBD)";
    case rvc::MotionCommand::suppressMotionTBD: return "MotionCommand(suppressMotionTBD)";
    }
    return "MotionCommand(unknown)";
}

const char* toString(rvc::CleaningCommand command) {
    switch (command) {
    case rvc::CleaningCommand::normal: return "CleaningCommand(normal)";
    case rvc::CleaningCommand::boost: return "CleaningCommand(boost)";
    case rvc::CleaningCommand::active: return "CleaningCommand(active)";
    case rvc::CleaningCommand::suspend: return "CleaningCommand(suspend)";
    case rvc::CleaningCommand::unchangedOrDeferredTBD:
        return "CleaningCommand(unchangedOrDeferredTBD)";
    }
    return "CleaningCommand(unknown)";
}

const char* toString(ActionKind kind) {
    switch (kind) {
    case ActionKind::Initialize: return "initialize";
    case ActionKind::Start: return "StartSession";
    case ActionKind::Stop: return "StopSession";
    case ActionKind::Resume: return "ResumeSession";
    case ActionKind::ServiceReset: return "requestServiceOrReset";
    case ActionKind::Obstacle: return "ObstacleStateChanged";
    case ActionKind::Dust: return "DustSignalUpdated";
    }
    return "unknown";
}

const char* toString(rvc::ObstacleEventKind event) {
    switch (event) {
    case rvc::ObstacleEventKind::frame: return "frame";
    case rvc::ObstacleEventKind::rawUpdates: return "rawUpdates";
    case rvc::ObstacleEventKind::frontUpdate: return "frontUpdate";
    case rvc::ObstacleEventKind::sideUpdate: return "sideUpdate";
    case rvc::ObstacleEventKind::forwardBlockedNotSurrounded: return "forwardBlockedNotSurrounded";
    case rvc::ObstacleEventKind::forwardBlocked: return "forwardBlocked";
    case rvc::ObstacleEventKind::forwardSafe: return "forwardSafe";
    case rvc::ObstacleEventKind::forwardSafeAfterManeuver: return "forwardSafeAfterManeuver";
    case rvc::ObstacleEventKind::sensorSnapshot: return "sensorSnapshot";
    case rvc::ObstacleEventKind::surrounded: return "surrounded";
    case rvc::ObstacleEventKind::lateralOpening: return "lateralOpening";
    case rvc::ObstacleEventKind::rightBlocked: return "rightBlocked";
    case rvc::ObstacleEventKind::rightInvalidForOpening: return "rightInvalidForOpening";
    case rvc::ObstacleEventKind::flickerFrame: return "flickerFrame";
    case rvc::ObstacleEventKind::readingsDuringReverse: return "readingsDuringReverse";
    case rvc::ObstacleEventKind::reverseReadings: return "reverseReadings";
    case rvc::ObstacleEventKind::reverseCycleSample: return "reverseCycleSample";
    case rvc::ObstacleEventKind::maxBackupNoLateralOpening: return "maxBackupNoLateralOpening";
    case rvc::ObstacleEventKind::noLateralOpeningWithinLimits: return "noLateralOpeningWithinLimits";
    case rvc::ObstacleEventKind::oscillatingLateralReadings: return "oscillatingLateralReadings";
    case rvc::ObstacleEventKind::invalidOrStale: return "invalidOrStale";
    case rvc::ObstacleEventKind::partialStale: return "partialStale";
    case rvc::ObstacleEventKind::recovered: return "recovered";
    case rvc::ObstacleEventKind::ambiguous: return "ambiguous";
    case rvc::ObstacleEventKind::dropoutDuringReverse: return "dropoutDuringReverse";
    case rvc::ObstacleEventKind::TBD: return "TBD";
    }
    return "unknown";
}

const char* toString(rvc::DustSignal signal) {
    switch (signal) {
    case rvc::DustSignal::aboveThreshold: return "aboveThreshold";
    case rvc::DustSignal::invalid: return "invalid";
    case rvc::DustSignal::normal: return "normal";
    case rvc::DustSignal::TBD: return "TBD";
    }
    return "unknown";
}

Action initialize() {
    return {ActionKind::Initialize};
}

Action start() {
    return {ActionKind::Start};
}

Action stop() {
    return {ActionKind::Stop};
}

Action resume() {
    return {ActionKind::Resume};
}

Action serviceReset() {
    return {ActionKind::ServiceReset};
}

Action obstacle(rvc::ObstacleEventKind event) {
    Action action{ActionKind::Obstacle};
    action.obstacle = event;
    return action;
}

Action dust(rvc::DustSignal signal) {
    Action action{ActionKind::Dust};
    action.dust = signal;
    return action;
}

template <typename Command>
std::string joinCommands(const std::vector<Command>& commands, const char* (*formatter)(Command)) {
    if (commands.empty()) {
        return "(none)";
    }

    std::string result;
    for (std::size_t i = 0; i < commands.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += formatter(commands[i]);
    }
    return result;
}

std::string describeAction(const Action& action) {
    std::string result = toString(action.kind);
    if (action.kind == ActionKind::Obstacle) {
        result += "(";
        result += toString(action.obstacle);
        result += ")";
    } else if (action.kind == ActionKind::Dust) {
        result += "(";
        result += toString(action.dust);
        result += ")";
    } else {
        result += "()";
    }
    return result;
}

std::string describeActions(const std::vector<Action>& actions) {
    std::string result;
    for (std::size_t i = 0; i < actions.size(); ++i) {
        if (i > 0) {
            result += " -> ";
        }
        result += describeAction(actions[i]);
    }
    return result;
}

void runActions(const std::vector<Action>& actions, rvc::RvcSoftwareController& controller) {
    rvc::TimeStamp time = 1;
    for (const auto& action : actions) {
        switch (action.kind) {
        case ActionKind::Initialize:
            controller.initialize();
            break;
        case ActionKind::Start:
            controller.sessionIntentPort().StartSession(rvc::SessionSource::User);
            break;
        case ActionKind::Stop:
            controller.sessionIntentPort().StopSession();
            break;
        case ActionKind::Resume:
            controller.sessionIntentPort().ResumeSession(rvc::SessionSource::User);
            break;
        case ActionKind::ServiceReset:
            controller.sessionIntentPort().requestServiceOrReset();
            break;
        case ActionKind::Obstacle:
            controller.obstacleInputPort().ObstacleStateChanged({action.obstacle, time++});
            break;
        case ActionKind::Dust:
            controller.dustInputPort().DustSignalUpdated(action.dust);
            break;
        }
    }
}

struct ScenarioResult {
    bool passed{false};
    std::vector<rvc::MotionCommand> actualMotion;
    std::vector<rvc::CleaningCommand> actualCleaning;
};

ScenarioResult runScenario(const Scenario& scenario) {
    RecordingMotionSink motionSink;
    RecordingCleaningSink cleaningSink;
    rvc::RvcSoftwareController controller{motionSink, cleaningSink};

    runActions(scenario.actions, controller);

    const bool motionPassed = motionSink.commands == scenario.expectedMotion;
    const bool cleaningPassed = cleaningSink.commands == scenario.expectedCleaning;
    return {motionPassed && cleaningPassed, motionSink.commands, cleaningSink.commands};
}

std::vector<Scenario> scenarios() {
    using CC = rvc::CleaningCommand;
    using DS = rvc::DustSignal;
    using OE = rvc::ObstacleEventKind;
    using MC = rvc::MotionCommand;

    return {
        {"TC-01", "Initialize enters safe inactive state",
         {initialize()}, {MC::stop}, {CC::suspend}, {}},
        {"TC-02", "Start and cruise forward while cleaning",
         {start(), obstacle(OE::frame)}, {MC::forward}, {CC::active}, {}},
        {"TC-03", "Dust boost returns to normal while active",
         {start(), dust(DS::aboveThreshold)}, {}, {CC::boost, CC::normal}, {}},
        {"TC-04", "Dust boost is deferred while session is inactive",
         {dust(DS::aboveThreshold)}, {}, {CC::unchangedOrDeferredTBD}, {}},
        {"TC-05", "Invalid dust signal keeps normal cleaning",
         {start(), dust(DS::invalid)}, {}, {CC::normal}, {}},
        {"TC-06", "Stop session stops motion and suspends cleaning",
         {start(), stop()}, {MC::stop}, {CC::suspend}, {}},
        {"TC-07", "Resume after stop re-enters forward cleaning",
         {start(), stop(), resume(), obstacle(OE::forwardSafe)}, {MC::stop, MC::forward}, {CC::suspend, CC::active}, {}},
        {"TC-08", "Service/reset request enters inactive safe state",
         {start(), serviceReset()}, {MC::stop}, {CC::suspend}, {}},
        {"TC-09", "Invalid obstacle data triggers full fault stop",
         {start(), obstacle(OE::invalidOrStale)}, {MC::stop}, {CC::suspend}, {}},
        {"TC-10", "Partial stale obstacle data uses gradual stop",
         {start(), obstacle(OE::partialStale)}, {MC::gradualOrPartialStopTBD}, {}, {}},
        {"TC-11", "Recovery snapshot is accepted without new command",
         {start(), obstacle(OE::recovered)}, {}, {}, {}},
        {"TC-12", "Forward blocked not surrounded chooses right turn",
         {start(), obstacle(OE::forwardBlockedNotSurrounded)}, {MC::turnRight}, {CC::suspend}, {}},
        {"TC-13", "Right blocked chooses left turn",
         {start(), obstacle(OE::rightBlocked)}, {MC::turnLeft}, {CC::suspend}, {}},
        {"TC-14", "Oscillating lateral readings suppress motion",
         {start(), obstacle(OE::oscillatingLateralReadings)}, {MC::suppressMotionTBD}, {}, {}},
        {"TC-15", "Surrounded starts reverse escape",
         {start(), obstacle(OE::surrounded)}, {MC::forbidForward, MC::reverse}, {CC::suspend}, {}},
        {"TC-16", "Reverse readings continue backing up",
         {start(), obstacle(OE::reverseReadings)}, {MC::continueReverse}, {}, {}},
        {"TC-17", "Reverse cycle sample continues reverse segment",
         {start(), obstacle(OE::reverseCycleSample)}, {MC::continueReverse}, {}, {}},
        {"TC-18", "Lateral opening escapes right",
         {start(), obstacle(OE::lateralOpening)}, {MC::lateralEscapeRight}, {}, {}},
        {"TC-19", "Right invalid for opening escapes left",
         {start(), obstacle(OE::rightInvalidForOpening)}, {MC::lateralEscapeLeft}, {}, {}},
        {"TC-20", "Max backup without opening uses fallback",
         {start(), obstacle(OE::maxBackupNoLateralOpening)}, {MC::fallbackTBD}, {}, {}},
        {"TC-21", "No lateral opening within limits uses fallback",
         {start(), obstacle(OE::noLateralOpeningWithinLimits)}, {MC::fallbackTBD}, {}, {}},
        {"TC-22", "Dropout during reverse enters UC-08 style stop",
         {start(), obstacle(OE::dropoutDuringReverse)}, {MC::stop}, {CC::suspend}, {}},
        {"TC-23", "Ambiguous obstacle snapshot waits for later policy",
         {start(), obstacle(OE::ambiguous)}, {}, {}, {}},
        {"TC-24", "Raw updates build consistency snapshot only",
         {start(), obstacle(OE::rawUpdates)}, {}, {}, {}},
        {"TC-25", "Asynchronous side update aligns snapshot only",
         {start(), obstacle(OE::sideUpdate)}, {}, {}, {}},
        {"TC-26", "Forward safe after maneuver resumes cleaning",
         {start(), obstacle(OE::forwardSafeAfterManeuver)}, {MC::forward}, {CC::active}, {}},
        {"TC-27", "Forward blocked after maneuver trends to obstacle avoidance",
         {start(), obstacle(OE::forwardBlocked)}, {MC::turnRight}, {CC::suspend}, {}},
        {"TC-28", "Going back then turning: reverse before right escape",
         {start(), obstacle(OE::surrounded), obstacle(OE::reverseReadings), obstacle(OE::lateralOpening)},
         {MC::forbidForward, MC::reverse, MC::continueReverse, MC::lateralEscapeRight},
         {CC::suspend},
         {"Required going back -> turning test: reverse happens before lateralEscapeRight."}},
        {"TC-29", "Going back then left turning after right opening invalid",
         {start(), obstacle(OE::surrounded), obstacle(OE::reverseCycleSample), obstacle(OE::rightInvalidForOpening)},
         {MC::forbidForward, MC::reverse, MC::continueReverse, MC::lateralEscapeLeft},
         {CC::suspend},
         {"Covers reverse segment followed by left lateral escape."}},
        {"TC-30", "Full mission path with boost, trap escape, resume, stop",
         {initialize(), start(), obstacle(OE::frame), dust(DS::aboveThreshold), obstacle(OE::surrounded),
          obstacle(OE::reverseCycleSample), obstacle(OE::lateralOpening), obstacle(OE::forwardSafe), stop()},
         {MC::stop, MC::forward, MC::forbidForward, MC::reverse, MC::continueReverse,
          MC::lateralEscapeRight, MC::forward, MC::stop},
         {CC::suspend, CC::active, CC::boost, CC::normal, CC::suspend, CC::active, CC::suspend},
         {"End-to-end simulator smoke test."}},
    };
}

void printScenario(const Scenario& scenario, const ScenarioResult& result, bool verbose) {
    std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] "
              << std::setw(5) << scenario.id << "  " << scenario.name << '\n';

    if (verbose || !result.passed) {
        std::cout << "  Actions: " << describeActions(scenario.actions) << '\n';
        std::cout << "  Expected motion:  " << joinCommands(scenario.expectedMotion, toString) << '\n';
        std::cout << "  Actual motion:    " << joinCommands(result.actualMotion, toString) << '\n';
        std::cout << "  Expected cleaning:" << joinCommands(scenario.expectedCleaning, toString) << '\n';
        std::cout << "  Actual cleaning:  " << joinCommands(result.actualCleaning, toString) << '\n';
        for (const auto& note : scenario.notes) {
            std::cout << "  Note: " << note << '\n';
        }
    }
}

} // namespace

int main(int argc, char** argv) {
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
        printScenario(scenario, result, verbose);
    }

    std::cout << '\n';
    std::cout << "Summary: " << passed << " / " << allScenarios.size() << " scenarios passed." << '\n';

    return passed == static_cast<int>(allScenarios.size()) ? 0 : 1;
}
