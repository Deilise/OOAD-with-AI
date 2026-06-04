#include "rvc/ObstaclePerceptionContext.hpp"

#include "rvc/NavigationAndEscapeCoordinator.hpp"

namespace rvc {

ObstaclePerceptionContext::ObstaclePerceptionContext(NavigationAndEscapeCoordinator& navigation)
    : navigation_(navigation) {}

FusedObstacleSnapshot ObstaclePerceptionContext::ObstacleStateChanged(const ObstacleEvent& event) {
    lastUpdateTime_ = event.sampleTime;
    const auto snapshot = fuse(event);
    navigation_.FusedObstacleSnapshot(snapshot);
    return snapshot;
}

FusedObstacleSnapshot ObstaclePerceptionContext::fuse(const ObstacleEvent& event) {
    FusedObstacleSnapshot snapshot{};

    switch (event.kind) {
    case ObstacleEventKind::frame:
    case ObstacleEventKind::forwardSafe:
    case ObstacleEventKind::forwardSafeAfterManeuver:
    case ObstacleEventKind::recovered:
        frontObstacle_ = false;
        leftObstacle_ = false;
        rightObstacle_ = false;
        snapshot.kind = event.kind == ObstacleEventKind::recovered
                            ? FusedObstacleSnapshotKind::valid
                            : FusedObstacleSnapshotKind::forwardSafe;
        snapshot.forwardSafe = true;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::forwardBlocked:
    case ObstacleEventKind::forwardBlockedNotSurrounded:
    case ObstacleEventKind::frontUpdate:
        frontObstacle_ = true;
        snapshot.kind = event.kind == ObstacleEventKind::forwardBlockedNotSurrounded
                            ? FusedObstacleSnapshotKind::notSurrounded
                            : FusedObstacleSnapshotKind::forwardBlocked;
        snapshot.forwardBlocked = true;
        snapshot.rightTurnViable = !rightObstacle_;
        snapshot.leftTurnViable = !leftObstacle_;
        break;
    case ObstacleEventKind::surrounded:
        frontObstacle_ = true;
        leftObstacle_ = true;
        rightObstacle_ = true;
        snapshot.kind = FusedObstacleSnapshotKind::surrounded;
        snapshot.forwardBlocked = true;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::lateralOpening:
        snapshot.kind = FusedObstacleSnapshotKind::rightTurnViable;
        snapshot.lateralOpening = true;
        snapshot.rightTurnViable = true;
        break;
    case ObstacleEventKind::rightInvalidForOpening:
    case ObstacleEventKind::rightBlocked:
        rightObstacle_ = true;
        snapshot.kind = event.kind == ObstacleEventKind::rightBlocked
                            ? FusedObstacleSnapshotKind::rightTurnInvalid
                            : FusedObstacleSnapshotKind::leftTurnViable;
        snapshot.forwardBlocked = event.kind == ObstacleEventKind::rightBlocked;
        snapshot.lateralOpening = true;
        snapshot.leftTurnViable = true;
        break;
    case ObstacleEventKind::readingsDuringReverse:
    case ObstacleEventKind::reverseReadings:
    case ObstacleEventKind::flickerFrame:
        snapshot.kind = FusedObstacleSnapshotKind::reverseReadings;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::reverseCycleSample:
        snapshot.kind = FusedObstacleSnapshotKind::reverseCycleSample;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::maxBackupNoLateralOpening:
    case ObstacleEventKind::noLateralOpeningWithinLimits:
        snapshot.kind = FusedObstacleSnapshotKind::noLateralOpening;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::partialStale:
        snapshot.kind = FusedObstacleSnapshotKind::partialStale;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::invalidOrStale:
    case ObstacleEventKind::dropoutDuringReverse:
        snapshot.kind = FusedObstacleSnapshotKind::invalid;
        snapshot.valid = false;
        break;
    case ObstacleEventKind::oscillatingLateralReadings:
        snapshot.kind = FusedObstacleSnapshotKind::unstable;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::ambiguous:
        snapshot.kind = FusedObstacleSnapshotKind::ambiguous;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::rawUpdates:
        snapshot.kind = FusedObstacleSnapshotKind::consistencyApplied;
        break;
    case ObstacleEventKind::sideUpdate:
        snapshot.kind = FusedObstacleSnapshotKind::alignedSnapshotTBD;
        break;
    case ObstacleEventKind::sensorSnapshot:
    case ObstacleEventKind::TBD:
        snapshot.kind = FusedObstacleSnapshotKind::snapshot;
        break;
    }

    return snapshot;
}

} // namespace rvc
