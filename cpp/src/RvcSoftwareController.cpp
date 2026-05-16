#include "rvc/RvcSoftwareController.hpp"

namespace rvc {

RvcSoftwareController::RvcSoftwareController(MotionCommandSink& motionSink,
                                             CleaningCommandSink& cleaningSink)
    : cleaning_(cleaningSink),
      navigation_(motionSink, cleaning_),
      perception_(navigation_),
      session_(navigation_, cleaning_) {}

void RvcSoftwareController::initialize() {
    session_.StopSession();
}

SessionIntentPort& RvcSoftwareController::sessionIntentPort() noexcept {
    return session_;
}

ObstacleInputPort& RvcSoftwareController::obstacleInputPort() noexcept {
    return perception_;
}

DustInputPort& RvcSoftwareController::dustInputPort() noexcept {
    return cleaning_;
}

AutomaticCleaningSession& RvcSoftwareController::session() noexcept {
    return session_;
}

ObstaclePerceptionContext& RvcSoftwareController::perception() noexcept {
    return perception_;
}

NavigationAndEscapeCoordinator& RvcSoftwareController::navigation() noexcept {
    return navigation_;
}

SurfaceCleaningController& RvcSoftwareController::cleaning() noexcept {
    return cleaning_;
}

} // namespace rvc
