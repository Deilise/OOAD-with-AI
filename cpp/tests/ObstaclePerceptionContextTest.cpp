#include "rvc/NavigationAndEscapeCoordinator.hpp"
#include "rvc/ObstaclePerceptionContext.hpp"
#include "rvc/SurfaceCleaningController.hpp"

#include "TestDoubles.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

struct ObstaclePerceptionContextFixture {
    rvc_test::RecordingMotionSink motionSink;
    rvc_test::RecordingCleaningSink cleaningSink;
    rvc::SurfaceCleaningController cleaning{cleaningSink};
    rvc::NavigationAndEscapeCoordinator navigation{motionSink, cleaning};
    rvc::ObstaclePerceptionContext perception{navigation};
};

rvc::ObstacleEvent obstacle(rvc::ObstacleEventKind kind,
                            rvc::TimeStamp time,
                            bool frontBlocked = false,
                            bool leftBlocked = false) {
    rvc::ObstacleEvent event;
    event.kind = kind;
    event.sampleTime = time;
    event.frontBlocked = frontBlocked;
    event.leftBlocked = leftBlocked;
    if (kind == rvc::ObstacleEventKind::probePoseRightSample) {
        event.probePose = rvc::ProbePose::right;
    }
    return event;
}

} // namespace

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsForwardSafeToForwardSafeSnapshot) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::forwardSafe, 1));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::forwardSafe);
    EXPECT_TRUE(snapshot.forwardSafe);
    EXPECT_TRUE(snapshot.valid);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsSurroundedEventToSurroundedSnapshot) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::surrounded, 2));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::surrounded);
    EXPECT_TRUE(snapshot.forwardBlocked);
    EXPECT_TRUE(snapshot.surrounded);
}

TEST(ObstaclePerceptionContextTest, ForwardBlockedRequestsRightSideProbeWhenRightStatusIsMissing) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::forwardBlocked, 3));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::ambiguous);
    EXPECT_TRUE(snapshot.forwardBlocked);
    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::probeRightSide,
                                           }));
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightBlockedMapsToRightTurnInvalid) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::forwardBlocked, 3));
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::probePoseRightSample, 4, true));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::rightTurnInvalid);
    EXPECT_TRUE(snapshot.forwardBlocked);
    EXPECT_TRUE(snapshot.leftTurnViable);
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightBlockedWithFrontAndLeftBlockedMapsToSurrounded) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::forwardBlocked, 4, false, true));
    fixture.motionSink.clear();
    fixture.cleaningSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::probePoseRightSample, 5, true));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::surrounded);
    EXPECT_TRUE(snapshot.forwardBlocked);
    EXPECT_TRUE(snapshot.surrounded);
    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::forbidForward,
                                               rvc::MotionCommand::reverse,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightClearMapsToRightTurnViable) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::forwardBlocked, 4));
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::probePoseRightSample, 5, false));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::rightTurnViable);
    EXPECT_TRUE(snapshot.rightTurnViable);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsReverseEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto reverseReadings = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::reverseReadings, 6));
    const auto reverseCycle = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::reverseCycleSample, 7));

    EXPECT_EQ(reverseReadings.kind, rvc::FusedObstacleSnapshotKind::reverseReadings);
    EXPECT_TRUE(reverseReadings.surrounded);
    EXPECT_EQ(reverseCycle.kind, rvc::FusedObstacleSnapshotKind::reverseCycleSample);
    EXPECT_TRUE(reverseCycle.surrounded);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsNoLateralOpening) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::noLateralOpening, 8));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::noLateralOpening);
    EXPECT_TRUE(snapshot.surrounded);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsFaultAndRecoveryEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto invalid = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::invalidOrStale, 9));
    const auto partialStale = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::partialStale, 10));
    const auto recovered = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::recovered, 11));

    EXPECT_EQ(invalid.kind, rvc::FusedObstacleSnapshotKind::invalid);
    EXPECT_FALSE(invalid.valid);
    EXPECT_EQ(partialStale.kind, rvc::FusedObstacleSnapshotKind::partialStale);
    EXPECT_EQ(recovered.kind, rvc::FusedObstacleSnapshotKind::valid);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsAmbiguousEvent) {
    ObstaclePerceptionContextFixture fixture;

    const auto ambiguous = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::ambiguous, 12));

    EXPECT_EQ(ambiguous.kind, rvc::FusedObstacleSnapshotKind::ambiguous);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsFusionEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto consistencyApplied = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::frontLeftSample, 14));
    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::TBD, 16));

    EXPECT_EQ(consistencyApplied.kind, rvc::FusedObstacleSnapshotKind::consistencyApplied);
    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::snapshot);
}
