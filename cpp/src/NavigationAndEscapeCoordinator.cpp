#include "rvc/NavigationAndEscapeCoordinator.hpp"

#include "rvc/SurfaceCleaningController.hpp"

namespace rvc {

NavigationAndEscapeCoordinator::NavigationAndEscapeCoordinator(MotionCommandSink& motionSink,
                                                               SurfaceCleaningController& cleaning)
    : motionSink_(motionSink), cleaning_(cleaning) {}

void NavigationAndEscapeCoordinator::SessionStateChanged(bool active) {
    sessionActive_ = active;

    if (active) {
        resumeCruisePerToggle();
        return;
    }

    motionState_ = MotionState::Stopped;
    send(MotionCommand::stop);
}

void NavigationAndEscapeCoordinator::SessionStateChanged(bool active, TravelToggle travelToggle) {
    travelToggle_ = travelToggle;
    SessionStateChanged(active);
}

void NavigationAndEscapeCoordinator::RequestRightSideProbe(ProbeReason reason) {
    if (!sessionActive_) {
        send(MotionCommand::suppressMotionTBD);
        return;
    }

    motionState_ = MotionState::RightSideProbing;
    if (reason == ProbeReason::rightTurnViability || reason == ProbeReason::surroundedCheck) {
        cleaning_.SuspendCleaningForManeuver();
    }
    send(MotionCommand::probeRightSide, currentProbeSensor());
}

void NavigationAndEscapeCoordinator::DustManeuverRequested() {
    if (!sessionActive_ || motionState_ == MotionState::SurroundedReversing ||
        motionState_ == MotionState::Avoiding || motionState_ == MotionState::RightSideProbing) {
        return;
    }

    motionState_ = MotionState::DustManeuvering;
    send(MotionCommand::stop);
    send(travelToggle_ == TravelToggle::Forward ? MotionCommand::spin540Clockwise
                                                : MotionCommand::spin540CounterClockwise);
    cleaning_.StartBoostCleaning();
}

void NavigationAndEscapeCoordinator::DustManeuverComplete() {
    cleaning_.SetCleaningMode(CleaningMode::Normal);
    ToggleTravelDirection();
}

void NavigationAndEscapeCoordinator::FusedObstacleSnapshot(const rvc::FusedObstacleSnapshot& snapshot) {
    if (!sessionActive_) {
        send(MotionCommand::suppressMotionTBD);
        return;
    }

    if (!snapshot.valid || snapshot.kind == FusedObstacleSnapshotKind::invalid ||
        snapshot.kind == FusedObstacleSnapshotKind::stale ||
        snapshot.kind == FusedObstacleSnapshotKind::probeStale ||
        snapshot.kind == FusedObstacleSnapshotKind::persistentInvalid ||
        snapshot.kind == FusedObstacleSnapshotKind::partialStale) {
        handleInvalidOrStale(snapshot);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::ambiguous ||
        snapshot.kind == FusedObstacleSnapshotKind::incoherent ||
        snapshot.kind == FusedObstacleSnapshotKind::valid) {
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::unstable) {
        send(MotionCommand::suppressMotionTBD);
        return;
    }

    if (snapshot.leadingSectorBlocked &&
        (snapshot.kind == FusedObstacleSnapshotKind::rightTurnViable ||
         snapshot.kind == FusedObstacleSnapshotKind::leftTurnViable ||
         snapshot.kind == FusedObstacleSnapshotKind::rightTurnInvalid ||
         snapshot.kind == FusedObstacleSnapshotKind::noLateralTurnViable)) {
        handleLeadingSectorBlocked(snapshot);
        return;
    }

    if (snapshot.surrounded || snapshot.kind == FusedObstacleSnapshotKind::surrounded ||
        snapshot.kind == FusedObstacleSnapshotKind::noLateralOpening ||
        snapshot.kind == FusedObstacleSnapshotKind::reverseReadings ||
        snapshot.kind == FusedObstacleSnapshotKind::reverseCycleSample ||
        snapshot.kind == FusedObstacleSnapshotKind::lateralOpening ||
        snapshot.kind == FusedObstacleSnapshotKind::rightTurnViable ||
        snapshot.kind == FusedObstacleSnapshotKind::leftTurnViable) {
        handleSurrounded(snapshot);
        return;
    }

    if (snapshot.leadingSectorBlocked ||
        snapshot.kind == FusedObstacleSnapshotKind::leadingSectorBlocked ||
        snapshot.kind == FusedObstacleSnapshotKind::notSurrounded ||
        snapshot.kind == FusedObstacleSnapshotKind::rightTurnInvalid) {
        handleLeadingSectorBlocked(snapshot);
        return;
    }

    if (snapshot.leadingSectorSafe ||
        snapshot.kind == FusedObstacleSnapshotKind::leadingSectorSafe) {
        handleLeadingSectorSafe();
        return;
    }

    send(MotionCommand::fallbackOrEscalateTBD);
}

void NavigationAndEscapeCoordinator::send(MotionCommand command, ProbeSensor probeSensor) {
    motionSink_.MotionCommand(command, probeSensor);
}

void NavigationAndEscapeCoordinator::handleInvalidOrStale(
    const rvc::FusedObstacleSnapshot& snapshot) {
    motionState_ = MotionState::Stopped;

    if (snapshot.kind == FusedObstacleSnapshotKind::partialStale) {
        send(MotionCommand::stopOrSafeHold);
        return;
    }

    send(snapshot.leadingSectorBlocked ? MotionCommand::stopOrFallbackTBD : MotionCommand::stop);
    cleaning_.SuspendCleaningForManeuver();
}

void NavigationAndEscapeCoordinator::handleSurrounded(const rvc::FusedObstacleSnapshot& snapshot) {
    if (snapshot.kind == FusedObstacleSnapshotKind::surrounded) {
        motionState_ = MotionState::SurroundedReversing;
        backupDistanceRemaining_ = 1.0;
        send(MotionCommand::restoreHeading);
        cleaning_.SuspendCleaningForManeuver();
        send(MotionCommand::reverseEscapeSegment);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::reverseReadings ||
        snapshot.kind == FusedObstacleSnapshotKind::reverseCycleSample) {
        motionState_ = MotionState::SurroundedReversing;
        send(MotionCommand::reverseEscapeSegment);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::rightTurnViable || snapshot.rightTurnViable) {
        motionState_ = MotionState::Turning;
        send(MotionCommand::restoreHeading);
        send(MotionCommand::turnRight);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::leftTurnViable || snapshot.leftTurnViable) {
        motionState_ = MotionState::Turning;
        send(MotionCommand::restoreHeading);
        send(MotionCommand::turnLeft);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::noLateralOpening) {
        send(MotionCommand::fallbackOrEscalateTBD);
        return;
    }

    send(MotionCommand::fallbackOrEscalateTBD);
}

void NavigationAndEscapeCoordinator::handleLeadingSectorBlocked(
    const rvc::FusedObstacleSnapshot& snapshot) {
    motionState_ = MotionState::Avoiding;

    if (snapshot.kind == FusedObstacleSnapshotKind::rightTurnInvalid && snapshot.leftTurnViable) {
        send(MotionCommand::restoreHeading);
        send(MotionCommand::turnLeft);
    } else if (snapshot.rightTurnViable) {
        send(MotionCommand::restoreHeading);
        send(MotionCommand::turnRight);
    } else if (snapshot.leftTurnViable) {
        send(MotionCommand::restoreHeading);
        send(MotionCommand::turnLeft);
    } else {
        send(MotionCommand::restoreHeading);
        send(MotionCommand::fallbackOrEscalateTBD);
    }
}

void NavigationAndEscapeCoordinator::handleLeadingSectorSafe() {
    resumeCruisePerToggle();
    cleaning_.ResumeCleaningAfterManeuver(CleaningMode::Normal);
}

void NavigationAndEscapeCoordinator::resumeCruisePerToggle() {
    if (travelToggle_ == TravelToggle::Backward) {
        motionState_ = MotionState::BackwardCruise;
        send(MotionCommand::reverse);
        return;
    }

    motionState_ = MotionState::ForwardCruise;
    send(MotionCommand::forward);
}

void NavigationAndEscapeCoordinator::ToggleTravelDirection() {
    travelToggle_ = travelToggle_ == TravelToggle::Forward ? TravelToggle::Backward
                                                           : TravelToggle::Forward;
}

ProbeSensor NavigationAndEscapeCoordinator::currentProbeSensor() const {
    return travelToggle_ == TravelToggle::Backward ? ProbeSensor::back : ProbeSensor::front;
}

} // namespace rvc
