#include "rvc/RvcSoftwareController.hpp"

#include <iostream>

namespace {

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

class ConsoleMotionSink final : public rvc::MotionCommandSink {
public:
    void MotionCommand(rvc::MotionCommand command) override {
        std::cout << toString(command) << '\n';
    }
};

class ConsoleCleaningSink final : public rvc::CleaningCommandSink {
public:
    void CleaningCommand(rvc::CleaningCommand command) override {
        std::cout << toString(command) << '\n';
    }
};

} // namespace

int main() {
    ConsoleMotionSink motionSink;
    ConsoleCleaningSink cleaningSink;
    rvc::RvcSoftwareController controller{motionSink, cleaningSink};

    controller.initialize();
    controller.sessionIntentPort().StartSession(rvc::SessionSource::User);
    controller.obstacleInputPort().ObstacleStateChanged(
        {rvc::ObstacleEventKind::forwardSafe, rvc::ProbePose::none, 1});
    controller.dustInputPort().DustSignalUpdated(rvc::DustSignal::aboveThreshold);
    controller.obstacleInputPort().ObstacleStateChanged(
        {rvc::ObstacleEventKind::surrounded, rvc::ProbePose::right, 2});
    controller.obstacleInputPort().ObstacleStateChanged(
        {rvc::ObstacleEventKind::lateralOpening, rvc::ProbePose::right, 3});
    controller.obstacleInputPort().ObstacleStateChanged(
        {rvc::ObstacleEventKind::forwardSafe, rvc::ProbePose::none, 4});
    controller.sessionIntentPort().StopSession();

    std::cout << "Press Enter to exit..." << '\n';
    std::cin.get();

    return 0;
}
