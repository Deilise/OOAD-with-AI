#pragma once

#include "rvc/AutomaticCleaningSession.hpp"
#include "rvc/NavigationAndEscapeCoordinator.hpp"
#include "rvc/ObstaclePerceptionContext.hpp"
#include "rvc/SurfaceCleaningController.hpp"

namespace rvc {

class RvcSoftwareController final {
public:
    RvcSoftwareController(MotionCommandSink& motionSink,
                          CleaningCommandSink& cleaningSink);

    void initialize();

    [[nodiscard]] SessionIntentPort& sessionIntentPort() noexcept;
    [[nodiscard]] ObstacleInputPort& obstacleInputPort() noexcept;
    [[nodiscard]] DustInputPort& dustInputPort() noexcept;

    [[nodiscard]] AutomaticCleaningSession& session() noexcept;
    [[nodiscard]] ObstaclePerceptionContext& perception() noexcept;
    [[nodiscard]] NavigationAndEscapeCoordinator& navigation() noexcept;
    [[nodiscard]] SurfaceCleaningController& cleaning() noexcept;

private:
    SurfaceCleaningController cleaning_;
    NavigationAndEscapeCoordinator navigation_;
    ObstaclePerceptionContext perception_;
    AutomaticCleaningSession session_;
};

} // namespace rvc
