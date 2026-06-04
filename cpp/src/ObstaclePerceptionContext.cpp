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
    case ObstacleEventKind::frontLeftSample:
        frontObstacle_ = event.frontBlocked;
        leftObstacle_ = event.leftBlocked;
        snapshot.kind = FusedObstacleSnapshotKind::consistencyApplied;
        snapshot.forwardBlocked = frontObstacle_;
        snapshot.forwardSafe = !frontObstacle_;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::forwardSafe:
    case ObstacleEventKind::forwardSafeAfterManeuver:
    case ObstacleEventKind::recovered:
        frontObstacle_ = false;
        leftObstacle_ = false;
        snapshot.kind = event.kind == ObstacleEventKind::recovered
                            ? FusedObstacleSnapshotKind::valid
                            : FusedObstacleSnapshotKind::forwardSafe;
        snapshot.forwardSafe = true;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::forwardBlocked:
        frontObstacle_ = true;
        leftObstacle_ = event.leftBlocked;
        snapshot.forwardBlocked = true;
        snapshot.leftTurnViable = !leftObstacle_;
        if (rightProbeStatus_ != ProbeStatus::Valid) {
            rightProbeStatus_ = ProbeStatus::Pending;
            navigation_.RequestRightSideProbe(leftObstacle_
                                                  ? ProbeReason::surroundedCheck
                                                  : ProbeReason::rightTurnViability);
            snapshot.kind = FusedObstacleSnapshotKind::ambiguous;
        } else if (leftObstacle_ && rightObstacleInferred_) {
            snapshot.kind = FusedObstacleSnapshotKind::surrounded;
            snapshot.surrounded = true;
        } else if (rightObstacleInferred_) {
            snapshot.kind = snapshot.leftTurnViable
                                ? FusedObstacleSnapshotKind::leftTurnViable
                                : FusedObstacleSnapshotKind::noLateralTurnViable;
        } else {
            snapshot.kind = FusedObstacleSnapshotKind::rightTurnViable;
            snapshot.rightTurnViable = true;
        }
        break;
    case ObstacleEventKind::probePoseRightSample:
        rightProbeStatus_ = ProbeStatus::Valid;
        rightObstacleInferred_ = event.frontBlocked;
        snapshot.forwardBlocked = frontObstacle_;
        if (frontObstacle_ && leftObstacle_ && rightObstacleInferred_) {
            snapshot.kind = FusedObstacleSnapshotKind::surrounded;
            snapshot.surrounded = true;
        } else {
            snapshot.kind = rightObstacleInferred_
                                ? FusedObstacleSnapshotKind::rightTurnInvalid
                                : FusedObstacleSnapshotKind::rightTurnViable;
        }
        snapshot.rightTurnViable = !rightObstacleInferred_;
        snapshot.leftTurnViable = !leftObstacle_;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::surrounded:
        frontObstacle_ = true;
        leftObstacle_ = true;
        rightObstacleInferred_ = true;
        rightProbeStatus_ = ProbeStatus::Valid;
        snapshot.kind = FusedObstacleSnapshotKind::surrounded;
        snapshot.forwardBlocked = true;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::leftOpening:
        leftObstacle_ = false;
        snapshot.kind = FusedObstacleSnapshotKind::leftTurnViable;
        snapshot.lateralOpening = true;
        snapshot.leftTurnViable = true;
        break;
    case ObstacleEventKind::lateralOpening:
        rightObstacleInferred_ = false;
        rightProbeStatus_ = ProbeStatus::Valid;
        snapshot.kind = FusedObstacleSnapshotKind::rightTurnViable;
        snapshot.lateralOpening = true;
        snapshot.rightTurnViable = true;
        break;
    case ObstacleEventKind::reverseReadings:
        snapshot.kind = FusedObstacleSnapshotKind::reverseReadings;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::reverseCycleSample:
        snapshot.kind = FusedObstacleSnapshotKind::reverseCycleSample;
        snapshot.surrounded = true;
        if (rightProbeStatus_ != ProbeStatus::Valid) {
            rightProbeStatus_ = ProbeStatus::Pending;
            navigation_.RequestRightSideProbe(ProbeReason::lateralOpeningCheck);
        }
        break;
    case ObstacleEventKind::noLateralOpening:
    case ObstacleEventKind::noLateralOpeningWithinLimits:
        snapshot.kind = FusedObstacleSnapshotKind::noLateralOpening;
        snapshot.surrounded = true;
        break;
    case ObstacleEventKind::partialStale:
        snapshot.kind = FusedObstacleSnapshotKind::partialStale;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::invalidOrTimeout:
        rightProbeStatus_ = ProbeStatus::Timeout;
        snapshot.kind = FusedObstacleSnapshotKind::invalid;
        snapshot.forwardBlocked = true;
        snapshot.valid = false;
        break;
    case ObstacleEventKind::invalidOrStale:
    case ObstacleEventKind::dropoutDuringReverse:
        snapshot.kind = FusedObstacleSnapshotKind::invalid;
        snapshot.valid = false;
        break;
    case ObstacleEventKind::ambiguous:
        snapshot.kind = FusedObstacleSnapshotKind::ambiguous;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::TBD:
        snapshot.kind = FusedObstacleSnapshotKind::snapshot;
        break;
    }

    return snapshot;
}

} // namespace rvc
