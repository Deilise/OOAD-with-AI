#include "rvc/AutomaticCleaningSession.hpp"

#include "rvc/NavigationAndEscapeCoordinator.hpp"
#include "rvc/SurfaceCleaningController.hpp"

namespace rvc {

AutomaticCleaningSession::AutomaticCleaningSession(NavigationAndEscapeCoordinator& navigation,
                                                   SurfaceCleaningController& cleaning)
    : navigation_(navigation), cleaning_(cleaning) {}

void AutomaticCleaningSession::StartSession(SessionSource source) {
    sessionActive_ = true;
    sessionSource_ = source;
    notifySessionStateChanged(true);
}

void AutomaticCleaningSession::StopSession() {
    sessionActive_ = false;
    notifySessionStateChanged(false);
}

void AutomaticCleaningSession::ResumeSession(SessionSource source) {
    sessionActive_ = true;
    sessionSource_ = source;
    notifySessionStateChanged(false);
}

void AutomaticCleaningSession::requestServiceOrReset() {
    sessionActive_ = false;
    notifySessionStateChanged(false);
}

void AutomaticCleaningSession::notifySessionStateChanged(bool initializeDefaults) {
    if (sessionActive_ && initializeDefaults) {
        navigation_.SessionStateChanged(true, TravelToggle::Forward);
        cleaning_.SessionStateChanged(true, CleaningMode::Normal);
        return;
    }

    navigation_.SessionStateChanged(sessionActive_);
    cleaning_.SessionStateChanged(sessionActive_);
}

} // namespace rvc
