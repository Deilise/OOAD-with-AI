#pragma once

#include "rvc/Ports.hpp"

namespace rvc {

class NavigationAndEscapeCoordinator;

class ObstaclePerceptionContext final : public ObstacleInputPort {
public:
    explicit ObstaclePerceptionContext(NavigationAndEscapeCoordinator& navigation);

    FusedObstacleSnapshot ObstacleStateChanged(const ObstacleEvent& event) override;

private:
    FusedObstacleSnapshot fuse(const ObstacleEvent& event);
    bool leadingSectorBlocked() const;
    ProbeSensor currentProbeSensor() const;

    bool frontObstacle_{false};
    bool leftObstacle_{false};
    bool backObstacle_{false};
    bool rightObstacleInferred_{false};
    ProbeStatus rightProbeStatus_{ProbeStatus::TBD};
    TimeStamp lastUpdateTime_{0};
    NavigationAndEscapeCoordinator& navigation_;
};

} // namespace rvc
