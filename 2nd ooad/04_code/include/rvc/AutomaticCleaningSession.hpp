#pragma once

#include "rvc/Ports.hpp"

namespace rvc {

class NavigationAndEscapeCoordinator;
class SurfaceCleaningController;

class AutomaticCleaningSession final : public SessionIntentPort {
public:
    AutomaticCleaningSession(NavigationAndEscapeCoordinator& navigation,
                             SurfaceCleaningController& cleaning);

    void StartSession(SessionSource source) override;
    void StopSession() override;
    void ResumeSession(SessionSource source) override;
    void requestServiceOrReset() override;

private:
    void notifySessionStateChanged();

    bool sessionActive_{false};
    SessionSource sessionSource_{SessionSource::TBD};
    NavigationAndEscapeCoordinator& navigation_;
    SurfaceCleaningController& cleaning_;
};

} // namespace rvc
