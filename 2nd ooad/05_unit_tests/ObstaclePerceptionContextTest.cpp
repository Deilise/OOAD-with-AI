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
                            bool leftBlocked = false,
                            bool backBlocked = false,
                            rvc::ProbeSensor probeSensor = rvc::ProbeSensor::front) {
    rvc::ObstacleEvent event;
    event.kind = kind;
    event.sampleTime = time;
    event.frontBlocked = frontBlocked;
    event.leftBlocked = leftBlocked;
    event.backBlocked = backBlocked;
    if (kind == rvc::ObstacleEventKind::probePoseRightSample) {
        event.probePose = rvc::ProbePose::right;
        event.probeSensor = probeSensor;
    }
    return event;
}

} // namespace

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsLeadingSectorSafeSnapshot) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::leadingSectorSafe, 1));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::leadingSectorSafe);
    EXPECT_TRUE(snapshot.leadingSectorSafe);
    EXPECT_TRUE(snapshot.valid);
}

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsSurroundedEventToSurroundedSnapshot) {
    ObstaclePerceptionContextFixture fixture;

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::surrounded, 2));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::surrounded);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_TRUE(snapshot.surrounded);
}

TEST(ObstaclePerceptionContextTest, LeadingSectorBlockedRequestsRightSideProbeWhenRightStatusIsMissing) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 3));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::ambiguous);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::probeRightSide,
                                           }));
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightBlockedMapsToRightTurnInvalid) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.motionSink.clear();
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 3));
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::probePoseRightSample, 4, true));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::rightTurnInvalid);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_TRUE(snapshot.leftTurnViable);
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightBlockedWithFrontAndLeftBlockedMapsToSurrounded) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.motionSink.clear();
    fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 4, false, true));
    fixture.motionSink.clear();
    fixture.cleaningSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::probePoseRightSample, 5, true));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::surrounded);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_TRUE(snapshot.surrounded);
    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::reverseEscapeSegment,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightClearMapsToRightTurnViable) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.motionSink.clear();
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 4));
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

TEST(ObstaclePerceptionContextTest, ObstacleStateChangedMapsFrontSample) {
    ObstaclePerceptionContextFixture fixture;

    const auto frontSample = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::frontSample, 14, true));
    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::TBD, 16));

    EXPECT_EQ(frontSample.kind, rvc::FusedObstacleSnapshotKind::valid);
    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::valid);
}

TEST(ObstaclePerceptionContextTest, BackSampleSetsLeadingSectorBlockedWhenToggledBackward) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true, rvc::TravelToggle::Backward);
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::backSample, 20, false, false, true));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::valid);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_FALSE(snapshot.leadingSectorSafe);
}

TEST(ObstaclePerceptionContextTest, LeadingSectorBlockedRequestsProbeWithBackSensorWhenToggledBackward) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true, rvc::TravelToggle::Backward);
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 21));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::ambiguous);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::probeRightSide,
                                           }));
    ASSERT_EQ(fixture.motionSink.probeSensors.size(), 1u);
    EXPECT_EQ(fixture.motionSink.probeSensors[0], rvc::ProbeSensor::back);
}

TEST(ObstaclePerceptionContextTest, ProbePoseRightSampleUsesBackBlockedWhenToggledBackward) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true, rvc::TravelToggle::Backward);
    fixture.motionSink.clear();
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 22));
    fixture.motionSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(obstacle(
        rvc::ObstacleEventKind::probePoseRightSample, 23, false, false, true, rvc::ProbeSensor::back));

    EXPECT_EQ(snapshot.kind, rvc::FusedObstacleSnapshotKind::rightTurnInvalid);
    EXPECT_TRUE(snapshot.leadingSectorBlocked);
    EXPECT_TRUE(snapshot.leftTurnViable);
}

TEST(ObstaclePerceptionContextTest, DustDetectedTriggersDustManeuverSpinAndBoost) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true, rvc::TravelToggle::Forward);
    fixture.cleaning.SessionStateChanged(true);
    fixture.motionSink.clear();
    fixture.cleaningSink.clear();

    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::dustDetected, 30));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stop,
                                               rvc::MotionCommand::spin540Clockwise,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::boost,
                                             }));
}

TEST(ObstaclePerceptionContextTest, DustClearedTogglesTravelAndSetsNormalCleaning) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true, rvc::TravelToggle::Forward);
    fixture.cleaning.SessionStateChanged(true);
    fixture.motionSink.clear();
    fixture.cleaningSink.clear();

    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::dustCleared, 31));

    EXPECT_EQ(fixture.navigation.travelToggle(), rvc::TravelToggle::Backward);
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::normal,
                                             }));
}

TEST(ObstaclePerceptionContextTest, DustDetectedDeferredWhileAvoiding) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true);
    fixture.cleaning.SessionStateChanged(true);
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::leadingSectorBlocked, 40));
    fixture.motionSink.clear();
    fixture.cleaningSink.clear();

    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::dustDetected, 41));

    EXPECT_TRUE(fixture.motionSink.commands.empty());
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(ObstaclePerceptionContextTest, BackSampleIgnoredDuringDustManeuver) {
    ObstaclePerceptionContextFixture fixture;
    fixture.navigation.SessionStateChanged(true, rvc::TravelToggle::Backward);
    fixture.cleaning.SessionStateChanged(true);
    fixture.perception.ObstacleStateChanged(obstacle(rvc::ObstacleEventKind::dustDetected, 50));
    ASSERT_TRUE(fixture.navigation.isDustManeuvering());
    fixture.motionSink.clear();
    fixture.cleaningSink.clear();

    const auto snapshot = fixture.perception.ObstacleStateChanged(
        obstacle(rvc::ObstacleEventKind::backSample, 51, false, false, true));

    EXPECT_FALSE(snapshot.valid);
    EXPECT_FALSE(snapshot.leadingSectorBlocked);
    EXPECT_TRUE(fixture.motionSink.commands.empty());
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}
