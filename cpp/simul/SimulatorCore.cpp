#include "SimulatorCore.hpp"

#include "rvc/RvcSoftwareController.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace rvc_simul {
namespace {

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

Action makeAction(ActionKind kind, BoardPosition position = {9, 1}, std::string note = {}) {
    Action action;
    action.kind = kind;
    action.robotPosition = position;
    action.note = std::move(note);
    return action;
}

Action initialize() {
    return makeAction(ActionKind::Initialize);
}

Action start() {
    return makeAction(ActionKind::Start);
}

Action stop() {
    return makeAction(ActionKind::Stop);
}

Action resume() {
    return makeAction(ActionKind::Resume);
}

Action serviceReset() {
    return makeAction(ActionKind::ServiceReset);
}

Action initializeAt(std::size_t row, std::size_t column, std::string note) {
    return makeAction(ActionKind::Initialize, {row, column}, std::move(note));
}

Action startAt(std::size_t row, std::size_t column, std::string note) {
    return makeAction(ActionKind::Start, {row, column}, std::move(note));
}

Action stopAt(std::size_t row, std::size_t column, std::string note) {
    return makeAction(ActionKind::Stop, {row, column}, std::move(note));
}

Action obstacle(rvc::ObstacleEventKind event,
                rvc::ProbePose probePose = rvc::ProbePose::none,
                bool frontBlocked = false,
                bool leftBlocked = false) {
    Action action = makeAction(ActionKind::Obstacle);
    action.obstacle = event;
    action.probePose = probePose;
    action.frontBlocked = frontBlocked;
    action.leftBlocked = leftBlocked;
    return action;
}

Action obstacleAt(rvc::ObstacleEventKind event, std::size_t row, std::size_t column, std::string note) {
    Action action = obstacle(event);
    action.robotPosition = {row, column};
    action.note = std::move(note);
    return action;
}

Action dust(rvc::DustSignal signal) {
    Action action = makeAction(ActionKind::Dust);
    action.dust = signal;
    return action;
}

Action dustAt(rvc::DustSignal signal, std::size_t row, std::size_t column, std::string note) {
    Action action = dust(signal);
    action.robotPosition = {row, column};
    action.note = std::move(note);
    return action;
}

Scenario makeScenario(std::string id,
                      std::string name,
                      std::vector<Action> actions,
                      std::vector<rvc::MotionCommand> expectedMotion,
                      std::vector<rvc::CleaningCommand> expectedCleaning,
                      std::vector<std::string> notes = {}) {
    return {std::move(id), std::move(name), referenceBoard(), std::move(actions),
            std::move(expectedMotion), std::move(expectedCleaning), std::move(notes)};
}

void applyAction(const Action& action, rvc::RvcSoftwareController& controller, rvc::TimeStamp& time) {
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
        controller.obstacleInputPort().ObstacleStateChanged(
            {action.obstacle, action.probePose, time++, action.frontBlocked, action.leftBlocked});
        break;
    case ActionKind::Dust:
        controller.dustInputPort().DustSignalUpdated(action.dust);
        break;
    }
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

ScenarioResult runScenarioUntil(const Scenario& scenario, std::size_t stepsToRun) {
    RecordingMotionSink motionSink;
    RecordingCleaningSink cleaningSink;
    rvc::RvcSoftwareController controller{motionSink, cleaningSink};
    rvc::TimeStamp time = 1;

    ScenarioResult result;
    const std::size_t boundedSteps = std::min(stepsToRun, scenario.actions.size());
    for (std::size_t i = 0; i < boundedSteps; ++i) {
        const auto previousMotionSize = motionSink.commands.size();
        const auto previousCleaningSize = cleaningSink.commands.size();

        applyAction(scenario.actions[i], controller, time);

        StepTrace trace;
        trace.stepIndex = i;
        trace.action = scenario.actions[i];
        trace.newMotion.assign(motionSink.commands.begin() + static_cast<std::ptrdiff_t>(previousMotionSize),
                               motionSink.commands.end());
        trace.newCleaning.assign(cleaningSink.commands.begin() + static_cast<std::ptrdiff_t>(previousCleaningSize),
                                 cleaningSink.commands.end());
        result.steps.push_back(std::move(trace));
    }

    result.actualMotion = motionSink.commands;
    result.actualCleaning = cleaningSink.commands;
    result.passed = boundedSteps == scenario.actions.size() &&
                    result.actualMotion == scenario.expectedMotion &&
                    result.actualCleaning == scenario.expectedCleaning;
    return result;
}

} // namespace

BoardLayout emptyBoard() {
    return {std::vector<std::vector<BoardCell>>(10, std::vector<BoardCell>(10, BoardCell::Floor)), {{9, 1}}};
}

BoardLayout referenceBoard() {
    constexpr BoardCell F = BoardCell::Floor;
    constexpr BoardCell W = BoardCell::Obstacle;
    constexpr BoardCell D = BoardCell::Dust;
    constexpr BoardCell R = BoardCell::RobotStart;

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
            },
            {{9, 1}, {8, 1}, {7, 1}, {6, 1}, {5, 1}, {4, 1},
             {3, 1}, {2, 1}, {1, 1}, {1, 2}, {1, 3}}};
}

std::vector<Scenario> scenarios() {
    using CC = rvc::CleaningCommand;
    using DS = rvc::DustSignal;
    using MC = rvc::MotionCommand;
    using OE = rvc::ObstacleEventKind;

    std::vector<Scenario> all{
        makeScenario("TC-01", "Initialize enters safe inactive state",
                     {initialize()}, {MC::stop}, {CC::suspend}),
        makeScenario("TC-02", "Start and cruise forward while cleaning",
                     {start(), obstacle(OE::forwardSafe)}, {MC::forward}, {CC::active}),
        makeScenario("TC-03", "Dust boost returns to normal while active",
                     {start(), dust(DS::aboveThreshold)}, {}, {CC::boost, CC::normal}),
        makeScenario("TC-04", "Dust boost is deferred while session is inactive",
                     {dust(DS::aboveThreshold)}, {}, {CC::unchangedOrDeferredTBD}),
        makeScenario("TC-05", "Invalid dust signal keeps normal cleaning",
                     {start(), dust(DS::invalid)}, {}, {CC::normal}),
        makeScenario("TC-06", "Stop session stops motion and suspends cleaning",
                     {start(), stop()}, {MC::stop}, {CC::suspend}),
        makeScenario("TC-07", "Resume after stop re-enters forward cleaning",
                     {start(), stop(), resume(), obstacle(OE::forwardSafe)}, {MC::stop, MC::forward},
                     {CC::suspend, CC::active}),
        makeScenario("TC-08", "Service/reset request enters inactive safe state",
                     {start(), serviceReset()}, {MC::stop}, {CC::suspend}),
        makeScenario("TC-09", "Invalid obstacle data triggers full fault stop",
                     {start(), obstacle(OE::invalidOrStale)}, {MC::stop}, {CC::suspend}),
        makeScenario("TC-10", "Partial stale obstacle data uses gradual stop",
                     {start(), obstacle(OE::partialStale)}, {MC::gradualOrPartialStopTBD}, {}),
        makeScenario("TC-11", "Recovery snapshot is accepted without new command",
                     {start(), obstacle(OE::recovered)}, {}, {}),
        makeScenario("TC-12", "Forward blocked probes right then chooses right turn",
                     {start(), obstacle(OE::forwardBlocked),
                      obstacle(OE::probePoseRightSample, rvc::ProbePose::right, false)},
                     {MC::probeRightSide, MC::restoreHeading, MC::turnRight}, {CC::suspend}),
        makeScenario("TC-13", "Probed right blocked chooses left turn",
                     {start(), obstacle(OE::forwardBlocked),
                      obstacle(OE::probePoseRightSample, rvc::ProbePose::right, true)},
                     {MC::probeRightSide, MC::restoreHeading, MC::turnLeft}, {CC::suspend}),
        makeScenario("TC-14", "Probe timeout uses stop or fallback",
                     {start(), obstacle(OE::invalidOrTimeout)}, {MC::stopOrFallbackTBD}, {CC::suspend}),
        makeScenario("TC-15", "Front left and probed right blocked starts reverse escape",
                     {start(), obstacle(OE::forwardBlocked, rvc::ProbePose::none, false, true),
                      obstacle(OE::probePoseRightSample, rvc::ProbePose::right, true)},
                     {MC::probeRightSide, MC::restoreHeading, MC::forbidForward, MC::reverse},
                     {CC::suspend}),
        makeScenario("TC-16", "Reverse readings continue backing up",
                     {start(), obstacle(OE::reverseReadings)}, {MC::restoreEscapeHeading, MC::continueReverse}, {}),
        makeScenario("TC-17", "Reverse cycle sample continues reverse segment",
                     {start(), obstacle(OE::reverseCycleSample)},
                     {MC::probeRightSide, MC::restoreEscapeHeading, MC::continueReverse}, {}),
        makeScenario("TC-18", "Lateral opening escapes right",
                     {start(), obstacle(OE::lateralOpening)}, {MC::lateralEscapeRight}, {}),
        makeScenario("TC-19", "Left opening escapes left",
                     {start(), obstacle(OE::leftOpening)}, {MC::lateralEscapeLeft}, {}),
        makeScenario("TC-20", "Max backup without opening uses fallback",
                     {start(), obstacle(OE::noLateralOpening)}, {MC::fallbackTBD}, {}),
        makeScenario("TC-21", "No lateral opening within limits uses fallback",
                     {start(), obstacle(OE::noLateralOpeningWithinLimits)}, {MC::fallbackTBD}, {}),
        makeScenario("TC-22", "Dropout during reverse enters UC-08 style stop",
                     {start(), obstacle(OE::dropoutDuringReverse)}, {MC::stop}, {CC::suspend}),
        makeScenario("TC-23", "Ambiguous obstacle snapshot waits for later policy",
                     {start(), obstacle(OE::ambiguous)}, {}, {}),
        makeScenario("TC-24", "Front left sample builds consistency snapshot only",
                     {start(), obstacle(OE::frontLeftSample)}, {}, {}),
        makeScenario("TC-25", "Ambiguous probe alignment waits for later policy",
                     {start(), obstacle(OE::ambiguous)}, {}, {}),
        makeScenario("TC-26", "Forward safe after maneuver resumes cleaning",
                     {start(), obstacle(OE::forwardSafeAfterManeuver)}, {MC::forward}, {CC::active}),
        makeScenario("TC-27", "Forward blocked after maneuver requests right-side probe",
                     {start(), obstacle(OE::forwardBlocked)}, {MC::probeRightSide}, {CC::suspend}),
        makeScenario("TC-28", "Going back then turning: reverse before right escape",
                     {start(), obstacle(OE::surrounded), obstacle(OE::reverseReadings),
                      obstacle(OE::lateralOpening)},
                     {MC::restoreHeading, MC::forbidForward, MC::reverse, MC::restoreEscapeHeading,
                      MC::continueReverse, MC::lateralEscapeRight},
                     {CC::suspend},
                     {"Required going back -> turning test: reverse happens before lateralEscapeRight."}),
        makeScenario("TC-29", "Going back then left turning after left opening",
                     {start(), obstacle(OE::surrounded), obstacle(OE::reverseCycleSample),
                      obstacle(OE::leftOpening)},
                     {MC::restoreHeading, MC::forbidForward, MC::reverse, MC::restoreEscapeHeading,
                      MC::continueReverse, MC::lateralEscapeLeft},
                     {CC::suspend},
                     {"Covers reverse segment followed by left lateral escape."}),
        makeScenario("TC-30", "Full mission path with boost, trap escape, resume, stop",
                     {initialize(), start(), obstacle(OE::forwardSafe), dust(DS::aboveThreshold),
                      obstacle(OE::surrounded), obstacle(OE::reverseCycleSample), obstacle(OE::lateralOpening),
                      obstacle(OE::forwardSafe), stop()},
                     {MC::stop, MC::forward, MC::restoreHeading, MC::forbidForward, MC::reverse,
                      MC::restoreEscapeHeading, MC::continueReverse, MC::lateralEscapeRight, MC::forward,
                      MC::stop},
                     {CC::suspend, CC::active, CC::boost, CC::normal, CC::suspend, CC::active,
                      CC::suspend},
                     {"End-to-end simulator smoke test."}),
    };

    all.push_back({"TC-31",
                   "Photo board route with dust and obstacle decision",
                   referenceBoard(),
                   {initializeAt(9, 1, "Start from the green cell"),
                    startAt(8, 1, "Move up the corridor"),
                    obstacleAt(OE::forwardSafe, 7, 1, "Forward corridor is clear"),
                    dustAt(DS::aboveThreshold, 2, 1, "Dust cell on the approved board"),
                    obstacleAt(OE::forwardBlocked, 1, 1, "Top wall blocks forward movement"),
                    obstacleAt(OE::probePoseRightSample, 1, 2, "Right probe sees an opening"),
                    obstacleAt(OE::forwardSafeAfterManeuver, 1, 3, "Resume after right turn"),
                    stopAt(1, 3, "End photo-board scenario")},
                   {MC::stop, MC::forward, MC::probeRightSide, MC::restoreHeading, MC::turnRight,
                    MC::forward, MC::stop},
                   {CC::suspend, CC::active, CC::boost, CC::normal, CC::suspend, CC::active,
                    CC::suspend},
                   {"Uses the approved 10x10 board image as the visual simulation map."}});

    return all;
}

ScenarioResult runScenario(const Scenario& scenario) {
    return runScenarioUntil(scenario, scenario.actions.size());
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

const char* toString(rvc::MotionCommand command) {
    switch (command) {
    case rvc::MotionCommand::forward: return "MotionCommand(forward)";
    case rvc::MotionCommand::turnRight: return "MotionCommand(turnRight)";
    case rvc::MotionCommand::turnLeft: return "MotionCommand(turnLeft)";
    case rvc::MotionCommand::reverse: return "MotionCommand(reverse)";
    case rvc::MotionCommand::continueReverse: return "MotionCommand(continueReverse)";
    case rvc::MotionCommand::probeRightSide: return "MotionCommand(probeRightSide)";
    case rvc::MotionCommand::restoreHeading: return "MotionCommand(restoreHeading)";
    case rvc::MotionCommand::restoreEscapeHeading: return "MotionCommand(restoreEscapeHeading)";
    case rvc::MotionCommand::lateralEscapeRight: return "MotionCommand(lateralEscapeRight)";
    case rvc::MotionCommand::lateralEscapeLeft: return "MotionCommand(lateralEscapeLeft)";
    case rvc::MotionCommand::forbidForward: return "MotionCommand(forbidForward)";
    case rvc::MotionCommand::stop: return "MotionCommand(stop)";
    case rvc::MotionCommand::fallbackTBD: return "MotionCommand(fallbackTBD)";
    case rvc::MotionCommand::fallbackOrEscalateTBD: return "MotionCommand(fallbackOrEscalateTBD)";
    case rvc::MotionCommand::stopOrFallbackTBD: return "MotionCommand(stopOrFallbackTBD)";
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

const char* toString(rvc::ObstacleEventKind event) {
    switch (event) {
    case rvc::ObstacleEventKind::frontLeftSample: return "frontLeftSample";
    case rvc::ObstacleEventKind::probePoseRightSample: return "probePoseRightSample";
    case rvc::ObstacleEventKind::forwardBlocked: return "forwardBlocked";
    case rvc::ObstacleEventKind::forwardSafe: return "forwardSafe";
    case rvc::ObstacleEventKind::forwardSafeAfterManeuver: return "forwardSafeAfterManeuver";
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

std::string describeAction(const Action& action) {
    std::ostringstream stream;
    stream << toString(action.kind);
    if (action.kind == ActionKind::Obstacle) {
        stream << '(' << toString(action.obstacle) << ')';
    } else if (action.kind == ActionKind::Dust) {
        stream << '(' << toString(action.dust) << ')';
    } else {
        stream << "()";
    }
    if (!action.note.empty()) {
        stream << " - " << action.note;
    }
    return stream.str();
}

std::string joinMotionCommands(const std::vector<rvc::MotionCommand>& commands) {
    return joinCommands(commands, toString);
}

std::string joinCleaningCommands(const std::vector<rvc::CleaningCommand>& commands) {
    return joinCommands(commands, toString);
}

ScenarioRunner::ScenarioRunner(const Scenario& scenario) : scenario_(scenario) {
    reset();
}

void ScenarioRunner::reset() {
    completedSteps_ = 0;
    result_ = runScenarioUntil(scenario_, 0);
}

bool ScenarioRunner::canAdvance() const {
    return completedSteps_ < scenario_.actions.size();
}

StepTrace ScenarioRunner::advance() {
    if (!canAdvance()) {
        throw std::out_of_range("scenario has no remaining steps");
    }

    result_ = runScenarioUntil(scenario_, completedSteps_ + 1);
    completedSteps_ += 1;
    return result_.steps.back();
}

void ScenarioRunner::rewindTo(std::size_t completedSteps) {
    completedSteps_ = std::min(completedSteps, scenario_.actions.size());
    result_ = runScenarioUntil(scenario_, completedSteps_);
}

const Scenario& ScenarioRunner::scenario() const {
    return scenario_;
}

std::size_t ScenarioRunner::completedSteps() const {
    return completedSteps_;
}

BoardPosition ScenarioRunner::currentRobotPosition() const {
    if (scenario_.actions.empty()) {
        return {0, 0};
    }
    if (completedSteps_ == 0) {
        return scenario_.actions.front().robotPosition;
    }
    return scenario_.actions[completedSteps_ - 1].robotPosition;
}

ScenarioResult ScenarioRunner::result() const {
    return result_;
}

} // namespace rvc_simul
