#pragma once

#include <cstdint>

namespace rvc {

using TimeStamp = std::uint64_t;
using Distance = double;

enum class SessionSource {
    User,
    Scheduler,
    Remote,
    TBD
};

enum class MotionState {
    ForwardCruise,
    Avoiding,
    RightSideProbing,
    Reversing,
    Turning,
    Stopped,
    TBD
};

enum class CleaningMode {
    Active,
    Suspended,
    Boosted,
    TBD
};

enum class PowerLevel {
    Normal,
    Boost,
    Off,
    TBD
};

enum class MotionCommand {
    forward,
    turnRight,
    turnLeft,
    reverse,
    continueReverse,
    probeRightSide,
    restoreHeading,
    restoreEscapeHeading,
    lateralEscapeRight,
    lateralEscapeLeft,
    forbidForward,
    stop,
    fallbackTBD,
    fallbackOrEscalateTBD,
    stopOrFallbackTBD,
    gradualOrPartialStopTBD,
    suppressMotionTBD
};

enum class CleaningCommand {
    normal,
    boost,
    active,
    suspend,
    unchangedOrDeferredTBD
};

enum class ObstacleEventKind {
    frontLeftSample,
    probePoseRightSample,
    forwardBlocked,
    forwardSafe,
    forwardSafeAfterManeuver,
    surrounded,
    leftOpening,
    lateralOpening,
    noLateralOpening,
    reverseReadings,
    reverseCycleSample,
    noLateralOpeningWithinLimits,
    invalidOrTimeout,
    invalidOrStale,
    partialStale,
    recovered,
    ambiguous,
    dropoutDuringReverse,
    TBD
};

enum class FusedObstacleSnapshotKind {
    forwardSafe,
    forwardBlocked,
    notSurrounded,
    surrounded,
    invalid,
    valid,
    ambiguous,
    partialStale,
    snapshot,
    reverseReadings,
    reverseCycleSample,
    lateralOpening,
    rightTurnViable,
    rightTurnInvalid,
    leftTurnViable,
    noLateralTurnViable,
    noLateralOpening,
    unstable,
    consistencyApplied,
    alignedSnapshotTBD,
    incoherent,
    TBD
};

enum class ProbeStatus {
    Pending,
    Valid,
    Invalid,
    Stale,
    Timeout,
    TBD
};

enum class ProbeReason {
    classifyObstacle,
    rightTurnViability,
    surroundedCheck,
    lateralOpeningCheck,
    rightStatusNeeded,
    alignRightProbe,
    TBD
};

enum class ProbePose {
    none,
    right,
    TBD
};

enum class DustSignal {
    aboveThreshold,
    invalid,
    normal,
    TBD
};

struct ObstacleEvent {
    ObstacleEventKind kind{ObstacleEventKind::TBD};
    ProbePose probePose{ProbePose::none};
    TimeStamp sampleTime{0};
    bool frontBlocked{false};
    bool leftBlocked{false};
    bool leftOpening{false};
};

struct FusedObstacleSnapshot {
    FusedObstacleSnapshotKind kind{FusedObstacleSnapshotKind::TBD};
    bool forwardBlocked{false};
    bool surrounded{false};
    bool lateralOpening{false};
    bool forwardSafe{false};
    bool rightTurnViable{false};
    bool leftTurnViable{false};
    bool valid{true};
};

} // namespace rvc
