#include "rvc/NavigationAndEscapeCoordinator.hpp"

#include "rvc/SurfaceCleaningController.hpp"

namespace rvc {

NavigationAndEscapeCoordinator::NavigationAndEscapeCoordinator(MotionCommandSink& motionSink,
                                                               SurfaceCleaningController& cleaning)
    : motionSink_(motionSink), cleaning_(cleaning) {}

void NavigationAndEscapeCoordinator::SessionStateChanged(bool active) {
    sessionActive_ = active;

    if (active) {
        motionState_ = MotionState::ForwardCruise;
        return;
    }

    motionState_ = MotionState::Stopped;
    send(MotionCommand::stop);
}

void NavigationAndEscapeCoordinator::FusedObstacleSnapshot(const rvc::FusedObstacleSnapshot& snapshot) {
    if (!sessionActive_) {
        send(MotionCommand::suppressMotionTBD);
        return;
    }

    if (!snapshot.valid || snapshot.kind == FusedObstacleSnapshotKind::invalid ||
        snapshot.kind == FusedObstacleSnapshotKind::partialStale) {
        handleInvalidOrStale(snapshot);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::ambiguous ||
        snapshot.kind == FusedObstacleSnapshotKind::incoherent ||
        snapshot.kind == FusedObstacleSnapshotKind::valid ||
        snapshot.kind == FusedObstacleSnapshotKind::consistencyApplied ||
        snapshot.kind == FusedObstacleSnapshotKind::alignedSnapshotTBD) {
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::unstable) {
        send(MotionCommand::suppressMotionTBD);
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

    if (snapshot.forwardBlocked || snapshot.kind == FusedObstacleSnapshotKind::forwardBlocked ||
        snapshot.kind == FusedObstacleSnapshotKind::notSurrounded ||
        snapshot.kind == FusedObstacleSnapshotKind::rightTurnInvalid) {
        handleForwardBlocked(snapshot);
        return;
    }

    if (snapshot.forwardSafe || snapshot.kind == FusedObstacleSnapshotKind::forwardSafe) {
        handleForwardSafe();
        return;
    }

    send(MotionCommand::probeOrBackupTBD);
}

void NavigationAndEscapeCoordinator::send(MotionCommand command) {
    motionSink_.MotionCommand(command);
}

void NavigationAndEscapeCoordinator::handleInvalidOrStale(const rvc::FusedObstacleSnapshot& snapshot) {
    motionState_ = MotionState::Stopped;

    if (snapshot.kind == FusedObstacleSnapshotKind::partialStale) {
        send(MotionCommand::gradualOrPartialStopTBD);
        return;
    }

    send(MotionCommand::stop);
    cleaning_.SuspendCleaningForManeuver();
}

void NavigationAndEscapeCoordinator::handleSurrounded(const rvc::FusedObstacleSnapshot& snapshot) {
    if (snapshot.kind == FusedObstacleSnapshotKind::surrounded) {
        motionState_ = MotionState::Reversing;
        backupDistanceRemaining_ = 1.0;
        send(MotionCommand::forbidForward);
        send(MotionCommand::reverse);
        cleaning_.SuspendCleaningForManeuver();
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::reverseReadings ||
        snapshot.kind == FusedObstacleSnapshotKind::reverseCycleSample) {
        motionState_ = MotionState::Reversing;
        send(MotionCommand::continueReverse);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::rightTurnViable || snapshot.rightTurnViable) {
        motionState_ = MotionState::Turning;
        send(MotionCommand::lateralEscapeRight);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::leftTurnViable || snapshot.leftTurnViable) {
        motionState_ = MotionState::Turning;
        send(MotionCommand::lateralEscapeLeft);
        return;
    }

    if (snapshot.kind == FusedObstacleSnapshotKind::noLateralOpening) {
        send(MotionCommand::fallbackTBD);
        return;
    }

    send(MotionCommand::probeOrBackupTBD);
}

void NavigationAndEscapeCoordinator::handleForwardBlocked(const rvc::FusedObstacleSnapshot& snapshot) {
    motionState_ = MotionState::Avoiding;
    cleaning_.SuspendCleaningForManeuver();

    if (snapshot.kind == FusedObstacleSnapshotKind::rightTurnInvalid) {
        send(MotionCommand::turnLeft);
    } else if (snapshot.rightTurnViable) {
        send(MotionCommand::turnRight);
    } else if (snapshot.leftTurnViable) {
        send(MotionCommand::turnLeft);
    } else {
        send(MotionCommand::probeOrBackupTBD);
    }
}

void NavigationAndEscapeCoordinator::handleForwardSafe() {
    motionState_ = MotionState::ForwardCruise;
    send(MotionCommand::forward);
    cleaning_.ResumeCleaningAfterManeuver();
}

} // namespace rvc
