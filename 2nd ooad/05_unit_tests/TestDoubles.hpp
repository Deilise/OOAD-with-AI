#pragma once

#include "rvc/Ports.hpp"

#include <vector>

namespace rvc_test {

class RecordingMotionSink final : public rvc::MotionCommandSink {
public:
    void MotionCommand(rvc::MotionCommand command,
                       rvc::ProbeSensor probeSensor = rvc::ProbeSensor::TBD) override {
        commands.push_back(command);
        if (command == rvc::MotionCommand::probeRightSide) {
            probeSensors.push_back(probeSensor);
        }
    }

    void clear() {
        commands.clear();
        probeSensors.clear();
    }

    std::vector<rvc::MotionCommand> commands;
    std::vector<rvc::ProbeSensor> probeSensors;
};

class RecordingCleaningSink final : public rvc::CleaningCommandSink {
public:
    void CleaningCommand(rvc::CleaningCommand command) override {
        commands.push_back(command);
    }

    void clear() {
        commands.clear();
    }

    std::vector<rvc::CleaningCommand> commands;
};

} // namespace rvc_test
