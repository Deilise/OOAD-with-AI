#include "rvc/ObstaclePerceptionContext.hpp"

#include "rvc/NavigationAndEscapeCoordinator.hpp"

namespace rvc {

ObstaclePerceptionContext::ObstaclePerceptionContext(NavigationAndEscapeCoordinator& navigation)
    : navigation_(navigation) {}

FusedObstacleSnapshot ObstaclePerceptionContext::ObstacleStateChanged(const ObstacleEvent& event) {
    lastUpdateTime_ = event.sampleTime;

    if (event.kind == ObstacleEventKind::dustDetected) {
        navigation_.DustManeuverRequested();
        return FusedObstacleSnapshot{};
    }

    if (event.kind == ObstacleEventKind::dustCleared) {
        navigation_.DustManeuverComplete();
        return FusedObstacleSnapshot{};
    }

    if (event.kind == ObstacleEventKind::backSample && navigation_.isDustManeuvering()) {
        FusedObstacleSnapshot ignored{};
        ignored.valid = false;
        return ignored;
    }

    const auto snapshot = fuse(event);
    navigation_.FusedObstacleSnapshot(snapshot);
    return snapshot;
}

bool ObstaclePerceptionContext::leadingSectorBlocked() const {
    return navigation_.travelToggle() == TravelToggle::Backward ? backObstacle_ : frontObstacle_;
}

ProbeSensor ObstaclePerceptionContext::currentProbeSensor() const {
    return navigation_.travelToggle() == TravelToggle::Backward ? ProbeSensor::back
                                                                : ProbeSensor::front;
}

FusedObstacleSnapshot ObstaclePerceptionContext::fuse(const ObstacleEvent& event) {
    FusedObstacleSnapshot snapshot{};

    switch (event.kind) {
    case ObstacleEventKind::frontSample:
        frontObstacle_ = event.frontBlocked;
        snapshot.kind = FusedObstacleSnapshotKind::valid;
        snapshot.leadingSectorBlocked = leadingSectorBlocked();
        snapshot.leadingSectorSafe = !snapshot.leadingSectorBlocked;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::leftSample:
        leftObstacle_ = event.leftBlocked;
        snapshot.kind = FusedObstacleSnapshotKind::valid;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::backSample:
        backObstacle_ = event.backBlocked;
        snapshot.kind = FusedObstacleSnapshotKind::valid;
        snapshot.leadingSectorBlocked = leadingSectorBlocked();
        snapshot.leadingSectorSafe = !snapshot.leadingSectorBlocked;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::leadingSectorSafe:
    case ObstacleEventKind::recovered:
        if (navigation_.travelToggle() == TravelToggle::Backward) {
            backObstacle_ = false;
        } else {
            frontObstacle_ = false;
        }
        snapshot.kind = event.kind == ObstacleEventKind::recovered
                            ? FusedObstacleSnapshotKind::valid
                            : FusedObstacleSnapshotKind::leadingSectorSafe;
        snapshot.leadingSectorSafe = true;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::leadingSectorBlocked:
        if (navigation_.travelToggle() == TravelToggle::Backward) {
            backObstacle_ = true;
        } else {
            frontObstacle_ = true;
        }
        leftObstacle_ = event.leftBlocked;
        snapshot.leadingSectorBlocked = true;
        snapshot.leftTurnViable = !leftObstacle_;
        if (rightProbeStatus_ != ProbeStatus::Valid) {
            rightProbeStatus_ = ProbeStatus::Pending;
            navigation_.RequestRightSideProbe(leftObstacle_ ? ProbeReason::surroundedCheck
                                                            : ProbeReason::rightTurnViability);
            snapshot.kind = FusedObstacleSnapshotKind::ambiguous;
        } else if (frontObstacle_ && leftObstacle_ && rightObstacleInferred_) {
            snapshot.kind = FusedObstacleSnapshotKind::surrounded;
            snapshot.surrounded = true;
        } else if (rightObstacleInferred_) {
            snapshot.kind = snapshot.leftTurnViable ? FusedObstacleSnapshotKind::leftTurnViable
                                                  : FusedObstacleSnapshotKind::noLateralTurnViable;
        } else {
            snapshot.kind = FusedObstacleSnapshotKind::rightTurnViable;
            snapshot.rightTurnViable = true;
        }
        break;
    case ObstacleEventKind::probePoseRightSample: {
        const bool probeBlocked = event.probeSensor == ProbeSensor::back ? event.backBlocked
                                                                         : event.frontBlocked;
        rightProbeStatus_ = ProbeStatus::Valid;
        rightObstacleInferred_ = probeBlocked;
        snapshot.leadingSectorBlocked = leadingSectorBlocked();
        if (frontObstacle_ && leftObstacle_ && rightObstacleInferred_) {
            snapshot.kind = FusedObstacleSnapshotKind::surrounded;
            snapshot.surrounded = true;
        } else {
            snapshot.kind = rightObstacleInferred_ ? FusedObstacleSnapshotKind::rightTurnInvalid
                                                   : FusedObstacleSnapshotKind::rightTurnViable;
        }
        snapshot.rightTurnViable = !rightObstacleInferred_;
        snapshot.leftTurnViable = !leftObstacle_;
        snapshot.valid = true;
        break;
    }
    case ObstacleEventKind::surrounded:
        frontObstacle_ = true;
        leftObstacle_ = true;
        rightObstacleInferred_ = true;
        rightProbeStatus_ = ProbeStatus::Valid;
        snapshot.kind = FusedObstacleSnapshotKind::surrounded;
        snapshot.leadingSectorBlocked = true;
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
        snapshot.leadingSectorBlocked = leadingSectorBlocked();
        snapshot.valid = false;
        break;
    case ObstacleEventKind::invalidOrStale:
        snapshot.kind = FusedObstacleSnapshotKind::invalid;
        snapshot.valid = false;
        break;
    case ObstacleEventKind::ambiguous:
        snapshot.kind = FusedObstacleSnapshotKind::ambiguous;
        snapshot.valid = true;
        break;
    case ObstacleEventKind::TBD:
        snapshot.kind = FusedObstacleSnapshotKind::valid;
        break;
    default:
        break;
    }

    return snapshot;
}

} // namespace rvc
