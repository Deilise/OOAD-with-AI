#include "rvc/SurfaceCleaningController.hpp"

namespace rvc {

SurfaceCleaningController::SurfaceCleaningController(CleaningCommandSink& cleaningSink)
    : cleaningSink_(cleaningSink) {}

void SurfaceCleaningController::SessionStateChanged(bool active) {
    sessionActive_ = active;

    if (active) {
        cleaningMode_ = CleaningMode::Active;
        powerLevel_ = PowerLevel::Normal;
        return;
    }

    cleaningMode_ = CleaningMode::Suspended;
    powerLevel_ = PowerLevel::Off;
    send(CleaningCommand::suspend);
}

void SurfaceCleaningController::DustSignalUpdated(DustSignal signal) {
    dustSignal_ = signal;

    if (!sessionActive_) {
        send(CleaningCommand::unchangedOrDeferredTBD);
        return;
    }

    switch (signal) {
    case DustSignal::aboveThreshold:
        cleaningMode_ = CleaningMode::Boosted;
        powerLevel_ = PowerLevel::Boost;
        send(CleaningCommand::boost);
        cleaningMode_ = CleaningMode::Active;
        powerLevel_ = PowerLevel::Normal;
        send(CleaningCommand::normal);
        break;
    case DustSignal::normal:
        cleaningMode_ = CleaningMode::Active;
        powerLevel_ = PowerLevel::Normal;
        send(CleaningCommand::normal);
        break;
    case DustSignal::invalid:
        cleaningMode_ = CleaningMode::Active;
        powerLevel_ = PowerLevel::Normal;
        send(CleaningCommand::normal);
        break;
    case DustSignal::TBD:
        send(CleaningCommand::unchangedOrDeferredTBD);
        break;
    }
}

void SurfaceCleaningController::SuspendCleaningForManeuver() {
    cleaningMode_ = CleaningMode::Suspended;
    send(CleaningCommand::suspend);
}

void SurfaceCleaningController::ResumeCleaningAfterManeuver() {
    if (!sessionActive_) {
        send(CleaningCommand::unchangedOrDeferredTBD);
        return;
    }

    cleaningMode_ = CleaningMode::Active;
    powerLevel_ = PowerLevel::Normal;
    send(CleaningCommand::active);
}

void SurfaceCleaningController::send(CleaningCommand command) {
    cleaningSink_.CleaningCommand(command);
}

} // namespace rvc
