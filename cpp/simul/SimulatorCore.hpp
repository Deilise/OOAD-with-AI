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
