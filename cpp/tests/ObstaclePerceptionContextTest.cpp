#include "rvc/NavigationAndEscapeCoordinator.hpp"
#include "rvc/ObstaclePerceptionContext.hpp"
#include "rvc/SurfaceCleaningController.hpp"

#include "TestDoubles.hpp"

#include <gtest/gtest.h>

namespace {

struct ObstaclePerceptionContextFixture {
    rvc_test::RecordingMotionSink motionSink;
    rvc_test::RecordingCleaningSink cleaningSink;
    rvc::SurfaceCleaningController cleaning{cleaningSink};
    rvc::NavigationAndEscapeCoordinator navigation{motionSink, cleaning};
    rvc::ObstaclePerceptionContext perception{navigation};
};

} // namespace

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsSafeFrameToForwardSafeSnapshot) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::frame, 1});

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::forwardSafe);
    EXPECT_TRUE(snapshot.forwardSafe);
    EXPECT_TRUE(snapshot.valid);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsSurroundedEventToSurroundedSnapshot) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::surrounded, 2});

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::surrounded);
    EXPECT_TRUE(snapshot.forwardBlocked);
    EXPECT_TRUE(snapshot.surrounded);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsForwardBlockedNotSurrounded) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::forwardBlockedNotSurrounded, 3});

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::notSurrounded);
    EXPECT_TRUE(snapshot.forwardBlocked);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsRightBlockedToRightTurnInvalid) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::rightBlocked, 4});

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::rightTurnInvalid);
    EXPECT_TRUE(snapshot.forwardBlocked);
    EXPECT_TRUE(snapshot.leftTurnViable);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsRightInvalidForOpeningToLeftTurnViable) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::rightInvalidForOpening, 5});

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::leftTurnViable);
    EXPECT_TRUE(snapshot.lateralOpening);
    EXPECT_TRUE(snapshot.leftTurnViable);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsReverseEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto reverseReadings = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::readingsDuringReverse, 6});
    const auto reverseCycle = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::reverseCycleSample, 7});

    EXPECT_EQ(reverseReadings.kind, rvc::FusedObstacleSnapshotKind::reverseReadings);
    EXPECT_TRUE(reverseReadings.surrounded);
    EXPECT_EQ(reverseCycle.kind, rvc::FusedObstacleSnapshotKind::reverseCycleSample);
    EXPECT_TRUE(reverseCycle.surrounded);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsNoLateralOpening) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::maxBackupNoLateralOpening, 8});

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::noLateralOpening);
    EXPECT_TRUE(snapshot.surrounded);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsFaultAndRecoveryEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto invalid = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::invalidOrStale, 9});
    const auto partialStale = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::partialStale, 10});
    const auto recovered = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::recovered, 11});

    EXPECT_EQ(invalid.kind, rvc::FusedObstacleSnapshotKind::invalid);
    EXPECT_FALSE(invalid.valid);
    EXPECT_EQ(partialStale.kind, rvc::FusedObstacleSnapshotKind::partialStale);
    EXPECT_EQ(recovered.kind, rvc::FusedObstacleSnapshotKind::valid);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsAmbiguousAndUnstableEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto ambiguous = fixture.perception.ObstacleStateChanged({rvc::ObstacleEventKind::ambiguous, 12});
    const auto unstable = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::oscillatingLateralReadings, 13});

    EXPECT_EQ(ambiguous.kind, rvc::FusedObstacleSnapshotKind::ambiguous);
    EXPECT_EQ(unstable.kind, rvc::FusedObstacleSnapshotKind::unstable);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsFusionEvents) {
    ObstaclePerceptionContextFixture fixture;

    const auto consistencyApplied = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::rawUpdates, 14});
    const auto alignedSnapshot = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::sideUpdate, 15});
    const auto snapshot = fixture.perception.ObstacleStateChanged(
        {rvc::ObstacleEventKind::sensorSnapshot, 16});

    EXPECT_EQ(consistencyApplied.kind, rvc::FusedObstacleSnapshotKind::consistencyApplied);
    EXPECT_EQ(alignedSnapshot.kind, rvc::FusedObstacleSnapshotKind::alignedSnapshotTBD);
    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::snapshot);
}
