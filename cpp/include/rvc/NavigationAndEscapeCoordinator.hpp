#pragma once

#include "rvc/Ports.hpp"

namespace rvc {

class SurfaceCleaningController;

class NavigationAndEscapeCoordinator final {
public:
    NavigationAndEscapeCoordinator(MotionCommandSink& motionSink,
                                   SurfaceCleaningController& cleaning);

    void SessionStateChanged(bool active);
    void SessionStateChanged(bool active, TravelToggle travelToggle);
    void RequestRightSideProbe(ProbeReason reason);
    void DustManeuverRequested();
    void DustManeuverComplete();
    void FusedObstacleSnapshot(const rvc::FusedObstacleSnapshot& snapshot);

    TravelToggle travelToggle() const noexcept { return travelToggle_; }

private:
    void send(MotionCommand command, ProbeSensor probeSensor = ProbeSensor::TBD);
    void handleInvalidOrStale(const rvc::FusedObstacleSnapshot& snapshot);
    void handleSurrounded(const rvc::FusedObstacleSnapshot& snapshot);
    void handleLeadingSectorBlocked(const rvc::FusedObstacleSnapshot& snapshot);
    void handleLeadingSectorSafe();
    void resumeCruisePerToggle();
    void ToggleTravelDirection();
    ProbeSensor currentProbeSensor() const;

    MotionState motionState_{MotionState::Stopped};
    TravelToggle travelToggle_{TravelToggle::Forward};
    Distance backupDistanceRemaining_{0.0};
    bool sessionActive_{false};
    MotionCommandSink& motionSink_;
    SurfaceCleaningController& cleaning_;
};

} // namespace rvc
