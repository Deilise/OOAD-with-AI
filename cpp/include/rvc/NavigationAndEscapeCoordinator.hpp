#pragma once

#include "rvc/Ports.hpp"

namespace rvc {

class SurfaceCleaningController;

class NavigationAndEscapeCoordinator final {
public:
    NavigationAndEscapeCoordinator(MotionCommandSink& motionSink,
                                   SurfaceCleaningController& cleaning);

    void SessionStateChanged(bool active);
    void RequestRightSideProbe(ProbeReason reason);
    void FusedObstacleSnapshot(const rvc::FusedObstacleSnapshot& snapshot);

private:
    void send(MotionCommand command);
    void handleInvalidOrStale(const rvc::FusedObstacleSnapshot& snapshot);
    void handleSurrounded(const rvc::FusedObstacleSnapshot& snapshot);
    void handleForwardBlocked(const rvc::FusedObstacleSnapshot& snapshot);
    void handleForwardSafe();

    MotionState motionState_{MotionState::Stopped};
    Distance backupDistanceRemaining_{0.0};
    bool sessionActive_{false};
    MotionCommandSink& motionSink_;
    SurfaceCleaningController& cleaning_;
};

} // namespace rvc
