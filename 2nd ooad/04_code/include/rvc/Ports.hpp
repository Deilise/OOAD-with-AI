#pragma once

#include "rvc/Types.hpp"

namespace rvc {

class SessionIntentPort {
public:
    virtual ~SessionIntentPort() = default;

    virtual void StartSession(SessionSource source) = 0;
    virtual void StopSession() = 0;
    virtual void ResumeSession(SessionSource source) = 0;
    virtual void requestServiceOrReset() = 0;
};

class ObstacleInputPort {
public:
    virtual ~ObstacleInputPort() = default;

    virtual FusedObstacleSnapshot ObstacleStateChanged(const ObstacleEvent& event) = 0;
};

class DustInputPort {
public:
    virtual ~DustInputPort() = default;

    virtual void DustSignalUpdated(DustSignal signal) = 0;
};

class MotionCommandSink {
public:
    virtual ~MotionCommandSink() = default;

    virtual void MotionCommand(rvc::MotionCommand command) = 0;
};

class CleaningCommandSink {
public:
    virtual ~CleaningCommandSink() = default;

    virtual void CleaningCommand(rvc::CleaningCommand command) = 0;
};

} // namespace rvc
