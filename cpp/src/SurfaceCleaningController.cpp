#include "rvc/SurfaceCleaningController.hpp"

namespace rvc {

SurfaceCleaningController::SurfaceCleaningController(CleaningCommandSink& cleaningSink)
    : cleaningSink_(cleaningSink) {}

void SurfaceCleaningController::SessionStateChanged(bool active) {
    sessionActive_ = active;

    if (active) {
        return;
    }

    cleaningMode_ = CleaningMode::Suspended;
    powerLevel_ = PowerLevel::Off;
    send(CleaningCommand::suspend);
}

void SurfaceCleaningController::SessionStateChanged(bool active, CleaningMode cleaningMode) {
    sessionActive_ = active;

    if (!active) {
        cleaningMode_ = CleaningMode::Suspended;
        powerLevel_ = PowerLevel::Off;
        send(CleaningCommand::suspend);
        return;
    }

    SetCleaningMode(cleaningMode);
}

void SurfaceCleaningController::DustSignalUpdated(DustSignal signal) {
    dustSignal_ = signal;
}

void SurfaceCleaningController::SuspendCleaningForManeuver() {
    cleaningMode_ = CleaningMode::Suspended;
    send(CleaningCommand::suspend);
}

void SurfaceCleaningController::ResumeCleaningAfterManeuver(CleaningMode mode) {
    if (!sessionActive_) {
        return;
    }

    SetCleaningMode(mode);
}

void SurfaceCleaningController::StartBoostCleaning() {
    if (!sessionActive_) {
        return;
    }

    cleaningMode_ = CleaningMode::Boost;
    powerLevel_ = PowerLevel::Boost;
    send(CleaningCommand::boost);
}

void SurfaceCleaningController::EndBoostCleaning() {
    SetCleaningMode(CleaningMode::Normal);
}

void SurfaceCleaningController::SetCleaningMode(CleaningMode mode) {
    cleaningMode_ = mode;

    switch (mode) {
    case CleaningMode::Normal:
        powerLevel_ = PowerLevel::Normal;
        send(CleaningCommand::normal);
        break;
    case CleaningMode::Boost:
        powerLevel_ = PowerLevel::Boost;
        send(CleaningCommand::boost);
        break;
    case CleaningMode::Suspended:
        powerLevel_ = PowerLevel::Off;
        send(CleaningCommand::suspend);
        break;
    case CleaningMode::TBD:
        break;
    }
}

void SurfaceCleaningController::send(CleaningCommand command) {
    cleaningSink_.CleaningCommand(command);
}

} // namespace rvc
