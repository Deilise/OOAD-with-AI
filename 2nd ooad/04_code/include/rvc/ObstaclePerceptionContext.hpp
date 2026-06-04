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

    bool frontObstacle_{false};
    bool leftObstacle_{false};
    bool rightObstacle_{false};
    TimeStamp lastUpdateTime_{0};
    NavigationAndEscapeCoordinator& navigation_;
};

} // namespace rvc
