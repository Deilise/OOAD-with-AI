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
    rvc::ProbePose probePose{rvc::ProbePose::none};
    rvc::DustSignal dust{rvc::DustSignal::TBD};
    bool frontBlocked{false};
    bool leftBlocked{false};
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
    void MotionCommand(rvc::MotionCommand command,
                       rvc::ProbeSensor /*probeSensor*/ = rvc::ProbeSensor::TBD) override {
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
    case rvc::MotionCommand::reverse: return "MotionCommand(reverse)";
    case rvc::MotionCommand::forwardOrReversePerToggle: return "MotionCommand(forwardOrReversePerToggle)";
    case rvc::MotionCommand::turnRight: return "MotionCommand(turnRight)";
    case rvc::MotionCommand::turnLeft: return "MotionCommand(turnLeft)";
    case rvc::MotionCommand::reverseEscapeSegment: return "MotionCommand(reverseEscapeSegment)";
    case rvc::MotionCommand::probeRightSide: return "MotionCommand(probeRightSide)";
    case rvc::MotionCommand::restoreHeading: return "MotionCommand(restoreHeading)";
    case rvc::MotionCommand::spin540Clockwise: return "MotionCommand(spin540Clockwise)";
    case rvc::MotionCommand::spin540CounterClockwise: return "MotionCommand(spin540CounterClockwise)";
    case rvc::MotionCommand::stop: return "MotionCommand(stop)";
    case rvc::MotionCommand::stopOrFallbackTBD: return "MotionCommand(stopOrFallbackTBD)";
    case rvc::MotionCommand::stopOrSafeHold: return "MotionCommand(stopOrSafeHold)";
    case rvc::MotionCommand::fallbackOrEscalateTBD: return "MotionCommand(fallbackOrEscalateTBD)";
    case rvc::MotionCommand::suppressMotionTBD: return "MotionCommand(suppressMotionTBD)";
    case rvc::MotionCommand::holdOrReManeuverTBD: return "MotionCommand(holdOrReManeuverTBD)";
    case rvc::MotionCommand::TBD: return "MotionCommand(TBD)";
    }
    return "MotionCommand(unknown)";
}

const char* toString(rvc::CleaningCommand command) {
    switch (command) {
    case rvc::CleaningCommand::normal: return "CleaningCommand(normal)";
    case rvc::CleaningCommand::boost: return "CleaningCommand(boost)";
    case rvc::CleaningCommand::suspend: return "CleaningCommand(suspend)";
    case rvc::CleaningCommand::TBD: return "CleaningCommand(TBD)";
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
    case rvc::ObstacleEventKind::frontSample: return "frontSample";
    case rvc::ObstacleEventKind::leftSample: return "leftSample";
    case rvc::ObstacleEventKind::backSample: return "backSample";
    case rvc::ObstacleEventKind::probePoseRightSample: return "probePoseRightSample";
    case rvc::ObstacleEventKind::leadingSectorBlocked: return "leadingSectorBlocked";
    case rvc::ObstacleEventKind::leadingSectorSafe: return "leadingSectorSafe";
    case rvc::ObstacleEventKind::dustDetected: return "dustDetected";
    case rvc::ObstacleEventKind::dustCleared: return "dustCleared";
    case rvc::ObstacleEventKind::surrounded: return "surrounded";
    case rvc::ObstacleEventKind::leftOpening: return "leftOpening";
    case rvc::ObstacleEventKind::lateralOpening: return "lateralOpening";
    case rvc::ObstacleEventKind::noLateralOpening: return "noLateralOpening";
    case rvc::ObstacleEventKind::reverseReadings: return "reverseReadings";
    case rvc::ObstacleEventKind::reverseCycleSample: return "reverseCycleSample";
    case rvc::ObstacleEventKind::noLateralOpeningWithinLimits: return "noLateralOpeningWithinLimits";
    case rvc::ObstacleEventKind::invalidOrTimeout: return "invalidOrTimeout";
    case rvc::ObstacleEventKind::invalidOrStale: return "invalidOrStale";
    case rvc::ObstacleEventKind::partialStale: return "partialStale";
    case rvc::ObstacleEventKind::recovered: return "recovered";
    case rvc::ObstacleEventKind::ambiguous: return "ambiguous";
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

Action obstacle(rvc::ObstacleEventKind event,
                rvc::ProbePose probePose = rvc::ProbePose::none,
                bool frontBlocked = false,
                bool leftBlocked = false) {
    Action action{ActionKind::Obstacle};
    action.obstacle = event;
    action.probePose = probePose;
    action.frontBlocked = frontBlocked;
    action.leftBlocked = leftBlocked;
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
        case ActionKind::Obstacle: {
            rvc::ObstacleEvent event;
            event.kind = action.obstacle;
            event.probePose = action.probePose;
            event.sampleTime = time++;
            event.frontBlocked = action.frontBlocked;
            event.leftBlocked = action.leftBlocked;
            if (action.obstacle == rvc::ObstacleEventKind::probePoseRightSample) {
                event.probeSensor = rvc::ProbeSensor::front;
            }
            controller.obstacleInputPort().ObstacleStateChanged(event);
            break;
        }
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
         {start(), obstacle(OE::leadingSectorSafe)},
         {MC::forward, MC::forward}, {CC::normal, CC::normal}, {}},
        {"TC-03", "Dust signal stores without immediate boost command",
         {start(), dust(DS::aboveThreshold)}, {MC::forward}, {}, {}},
        {"TC-04", "Dust signal is ignored while session is inactive",
         {dust(DS::aboveThreshold)}, {}, {}, {}},
        {"TC-05", "Invalid dust signal stores without cleaning command",
         {start(), dust(DS::invalid)}, {MC::forward}, {}, {}},
        {"TC-06", "Stop session stops motion and suspends cleaning",
         {start(), stop()}, {MC::forward, MC::stop}, {CC::normal, CC::suspend}, {}},
        {"TC-07", "Resume after stop re-enters forward cleaning",
         {start(), stop(), resume(), obstacle(OE::leadingSectorSafe)},
         {MC::forward, MC::stop, MC::forward, MC::forward},
         {CC::normal, CC::suspend, CC::normal}, {}},
        {"TC-08", "Service/reset request enters inactive safe state",
         {start(), serviceReset()}, {MC::forward, MC::stop}, {CC::normal, CC::suspend}, {}},
        {"TC-09", "Invalid obstacle data triggers full fault stop",
         {start(), obstacle(OE::invalidOrStale)}, {MC::forward, MC::stop}, {CC::normal, CC::suspend}, {}},
        {"TC-10", "Partial stale obstacle data uses safe hold",
         {start(), obstacle(OE::partialStale)}, {MC::forward, MC::stopOrSafeHold}, {}, {}},
        {"TC-11", "Recovery snapshot is accepted without new command",
         {start(), obstacle(OE::recovered)}, {MC::forward}, {}, {}},
        {"TC-12", "Leading sector blocked probes right then chooses right turn",
         {start(), obstacle(OE::leadingSectorBlocked),
          obstacle(OE::probePoseRightSample, rvc::ProbePose::right, false)},
         {MC::forward, MC::probeRightSide, MC::restoreHeading, MC::turnRight}, {CC::normal, CC::suspend}, {}},
        {"TC-13", "Probed right blocked chooses left turn",
         {start(), obstacle(OE::leadingSectorBlocked),
          obstacle(OE::probePoseRightSample, rvc::ProbePose::right, true)},
         {MC::forward, MC::probeRightSide, MC::restoreHeading, MC::turnLeft}, {CC::normal, CC::suspend}, {}},
        {"TC-14", "Probe timeout uses stop or fallback",
         {start(), obstacle(OE::invalidOrTimeout)}, {MC::forward, MC::stopOrFallbackTBD}, {CC::normal, CC::suspend}, {}},
        {"TC-15", "Front left and probed right blocked starts reverse escape",
         {start(), obstacle(OE::leadingSectorBlocked, rvc::ProbePose::none, false, true),
          obstacle(OE::probePoseRightSample, rvc::ProbePose::right, true)},
         {MC::forward, MC::probeRightSide, MC::restoreHeading, MC::reverseEscapeSegment},
         {CC::normal, CC::suspend}, {}},
        {"TC-16", "Reverse readings continue backing up",
         {start(), obstacle(OE::reverseReadings)}, {MC::forward, MC::reverseEscapeSegment}, {}, {}},
        {"TC-17", "Reverse cycle sample continues reverse segment",
         {start(), obstacle(OE::reverseCycleSample)},
         {MC::forward, MC::reverseEscapeSegment, MC::probeRightSide}, {}, {}},
        {"TC-18", "Lateral opening turns right",
         {start(), obstacle(OE::lateralOpening)},
         {MC::forward, MC::restoreHeading, MC::turnRight}, {}, {}},
        {"TC-19", "Left opening turns left",
         {start(), obstacle(OE::leftOpening)},
         {MC::forward, MC::restoreHeading, MC::turnLeft}, {}, {}},
        {"TC-20", "Max backup without opening uses fallback",
         {start(), obstacle(OE::noLateralOpening)},
         {MC::forward, MC::fallbackOrEscalateTBD}, {}, {}},
        {"TC-21", "No lateral opening within limits uses fallback",
         {start(), obstacle(OE::noLateralOpeningWithinLimits)},
         {MC::forward, MC::fallbackOrEscalateTBD}, {}, {}},
        {"TC-22", "Invalid data during reverse enters UC-08 style stop",
         {start(), obstacle(OE::invalidOrStale)}, {MC::forward, MC::stop}, {CC::normal, CC::suspend}, {}},
        {"TC-23", "Ambiguous obstacle snapshot waits for later policy",
         {start(), obstacle(OE::ambiguous)}, {MC::forward}, {}, {}},
        {"TC-24", "Front sample builds valid snapshot only",
         {start(), obstacle(OE::frontSample, rvc::ProbePose::none, true)}, {MC::forward}, {}, {}},
        {"TC-25", "Dust detected starts dust maneuver spin and boost",
         {start(), obstacle(OE::dustDetected)},
         {MC::forward, MC::stop, MC::spin540Clockwise}, {CC::normal, CC::boost}, {}},
        {"TC-26", "Leading sector safe resumes cleaning",
         {start(), obstacle(OE::leadingSectorSafe)},
         {MC::forward, MC::forward}, {CC::normal, CC::normal}, {}},
        {"TC-27", "Leading sector blocked requests right-side probe",
         {start(), obstacle(OE::leadingSectorBlocked)},
         {MC::forward, MC::probeRightSide}, {CC::normal, CC::suspend}, {}},
        {"TC-28", "Going back then turning: reverse before right escape",
         {start(), obstacle(OE::surrounded), obstacle(OE::reverseReadings), obstacle(OE::lateralOpening)},
         {MC::forward, MC::restoreHeading, MC::reverseEscapeSegment, MC::reverseEscapeSegment,
          MC::restoreHeading, MC::turnRight},
         {CC::normal, CC::suspend},
         {"Required going back -> turning test: reverse happens before turnRight."}},
        {"TC-29", "Going back then left turning after left opening",
         {start(), obstacle(OE::surrounded), obstacle(OE::reverseCycleSample), obstacle(OE::leftOpening)},
         {MC::forward, MC::restoreHeading, MC::reverseEscapeSegment, MC::reverseEscapeSegment,
          MC::restoreHeading, MC::turnLeft},
         {CC::normal, CC::suspend},
         {"Covers reverse segment followed by left turn."}},
        {"TC-30", "Full mission path with dust maneuver, trap escape, resume, stop",
         {initialize(), start(), obstacle(OE::leadingSectorSafe), obstacle(OE::dustDetected),
          obstacle(OE::dustCleared), obstacle(OE::surrounded), obstacle(OE::reverseCycleSample),
          obstacle(OE::lateralOpening), obstacle(OE::leadingSectorSafe), stop()},
         {MC::stop, MC::forward, MC::forward, MC::stop, MC::spin540Clockwise, MC::restoreHeading,
          MC::reverseEscapeSegment, MC::reverseEscapeSegment, MC::probeRightSide, MC::restoreHeading,
          MC::turnRight, MC::forward, MC::stop},
         {CC::suspend, CC::normal, CC::normal, CC::boost, CC::normal, CC::suspend, CC::normal, CC::suspend},
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
