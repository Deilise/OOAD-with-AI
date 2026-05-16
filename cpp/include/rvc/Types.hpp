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
    lateralEscapeRight,
    lateralEscapeLeft,
    forbidForward,
    stop,
    fallbackTBD,
    probeOrBackupTBD,
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
    frame,
    rawUpdates,
    frontUpdate,
    sideUpdate,
    forwardBlockedNotSurrounded,
    forwardBlocked,
    forwardSafe,
    forwardSafeAfterManeuver,
    sensorSnapshot,
    surrounded,
    lateralOpening,
    rightBlocked,
    rightInvalidForOpening,
    flickerFrame,
    readingsDuringReverse,
    reverseReadings,
    reverseCycleSample,
    maxBackupNoLateralOpening,
    noLateralOpeningWithinLimits,
    oscillatingLateralReadings,
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
    noLateralOpening,
    unstable,
    consistencyApplied,
    alignedSnapshotTBD,
    incoherent,
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
    TimeStamp sampleTime{0};
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
