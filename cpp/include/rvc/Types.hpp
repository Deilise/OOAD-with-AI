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

enum class TravelToggle {
    Forward,
    Backward,
    TBD
};

enum class MotionState {
    ForwardCruise,
    BackwardCruise,
    Avoiding,
    RightSideProbing,
    SurroundedReversing,
    DustManeuvering,
    Turning,
    Stopped,
    TBD
};

enum class CleaningMode {
    Normal,
    Boost,
    Suspended,
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
    reverse,
    forwardOrReversePerToggle,
    turnRight,
    turnLeft,
    reverseEscapeSegment,
    probeRightSide,
    restoreHeading,
    spin540Clockwise,
    spin540CounterClockwise,
    stop,
    holdOrReManeuverTBD,
    stopOrSafeHold,
    stopOrFallbackTBD,
    fallbackOrEscalateTBD,
    suppressMotionTBD,
    TBD
};

enum class CleaningCommand {
    normal,
    boost,
    suspend,
    TBD
};

enum class ObstacleEventKind {
    frontSample,
    leftSample,
    backSample,
    probePoseRightSample,
    leadingSectorBlocked,
    leadingSectorSafe,
    dustDetected,
    dustCleared,
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
    TBD
};

enum class FusedObstacleSnapshotKind {
    leadingSectorSafe,
    leadingSectorBlocked,
    notSurrounded,
    surrounded,
    invalid,
    stale,
    probeStale,
    persistentInvalid,
    valid,
    ambiguous,
    partialStale,
    rightTurnViable,
    rightTurnInvalid,
    leftTurnViable,
    noLateralTurnViable,
    noLateralOpening,
    reverseReadings,
    reverseCycleSample,
    lateralOpening,
    unstable,
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

enum class ProbeSensor {
    front,
    back,
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
    ProbeSensor probeSensor{ProbeSensor::TBD};
    TimeStamp sampleTime{0};
    bool frontBlocked{false};
    bool leftBlocked{false};
    bool backBlocked{false};
    bool leadingSectorBlocked{false};
};

struct FusedObstacleSnapshot {
    FusedObstacleSnapshotKind kind{FusedObstacleSnapshotKind::TBD};
    bool leadingSectorBlocked{false};
    bool leadingSectorSafe{false};
    bool surrounded{false};
    bool lateralOpening{false};
    bool rightTurnViable{false};
    bool leftTurnViable{false};
    bool valid{true};
};

} // namespace rvc
