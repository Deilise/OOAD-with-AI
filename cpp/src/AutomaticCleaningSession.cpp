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
    notifySessionStateChanged();
}

void AutomaticCleaningSession::StopSession() {
    sessionActive_ = false;
    notifySessionStateChanged();
}

void AutomaticCleaningSession::ResumeSession(SessionSource source) {
    sessionActive_ = true;
    sessionSource_ = source;
    notifySessionStateChanged();
}

void AutomaticCleaningSession::requestServiceOrReset() {
    sessionActive_ = false;
    notifySessionStateChanged();
}

void AutomaticCleaningSession::notifySessionStateChanged() {
    navigation_.SessionStateChanged(sessionActive_);
    cleaning_.SessionStateChanged(sessionActive_);
}

} // namespace rvc
