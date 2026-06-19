#include "rvc/RvcSoftwareController.hpp"

#include <iostream>

namespace {

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
    case rvc::MotionCommand::fallbackOrEscalateTBD: return "MotionCommand(fallbackOrEscalateTBD)";
    case rvc::MotionCommand::suppressMotionTBD: return "MotionCommand(suppressMotionTBD)";
    case rvc::MotionCommand::stopOrSafeHold: return "MotionCommand(stopOrSafeHold)";
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

class ConsoleMotionSink final : public rvc::MotionCommandSink {
public:
    void MotionCommand(rvc::MotionCommand command,
                       rvc::ProbeSensor /*probeSensor*/ = rvc::ProbeSensor::TBD) override {
        std::cout << toString(command) << '\n';
    }
};

class ConsoleCleaningSink final : public rvc::CleaningCommandSink {
public:
    void CleaningCommand(rvc::CleaningCommand command) override {
        std::cout << toString(command) << '\n';
    }
};

rvc::ObstacleEvent obstacle(rvc::ObstacleEventKind kind, rvc::TimeStamp time) {
    rvc::ObstacleEvent event;
    event.kind = kind;
    event.sampleTime = time;
    return event;
}

} // namespace

int main() {
    ConsoleMotionSink motionSink;
    ConsoleCleaningSink cleaningSink;
    rvc::RvcSoftwareController controller{motionSink, cleaningSink};

    controller.initialize();
    controller.sessionIntentPort().StartSession(rvc::SessionSource::User);
    controller.obstacleInputPort().ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::leadingSectorSafe, 1));
    controller.obstacleInputPort().ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::dustDetected, 2));
    controller.obstacleInputPort().ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::dustCleared, 3));
    controller.obstacleInputPort().ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::surrounded, 4));
    controller.obstacleInputPort().ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::lateralOpening, 5));
    controller.obstacleInputPort().ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::leadingSectorSafe, 6));
    controller.sessionIntentPort().StopSession();

    std::cout << "Press Enter to exit..." << '\n';
    std::cin.get();

    return 0;
}
