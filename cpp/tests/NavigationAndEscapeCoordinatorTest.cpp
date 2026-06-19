#include "rvc/NavigationAndEscapeCoordinator.hpp"
#include "rvc/SurfaceCleaningController.hpp"

#include "TestDoubles.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

struct NavigationAndEscapeCoordinatorFixture {
    rvc_test::RecordingMotionSink motionSink;
    rvc_test::RecordingCleaningSink cleaningSink;
    rvc::SurfaceCleaningController cleaning{cleaningSink};
    rvc::NavigationAndEscapeCoordinator navigation{motionSink, cleaning};

    void activateSession() {
        navigation.SessionStateChanged(true);
        cleaning.SessionStateChanged(true);
        motionSink.clear();
        cleaningSink.clear();
    }
};

rvc::FusedObstacleSnapshot snapshot(rvc::FusedObstacleSnapshotKind kind) {
    rvc::FusedObstacleSnapshot result;
    result.kind = kind;
    return result;
}

} // namespace

TEST(NavigationAndEscapeCoordinatorTest, SessionStateChangedActiveStartsForwardCruise) {
    NavigationAndEscapeCoordinatorFixture fixture;

    fixture.navigation.SessionStateChanged(true);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::forward,
                                           }));
}

TEST(NavigationAndEscapeCoordinatorTest, SessionStateChangedInactiveStopsMotion) {
    NavigationAndEscapeCoordinatorFixture fixture;

    fixture.navigation.SessionStateChanged(false);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stop,
                                           }));
}

TEST(NavigationAndEscapeCoordinatorTest, InvalidSnapshotStopsMotionAndSuspendsCleaning) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::invalid));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stop,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(NavigationAndEscapeCoordinatorTest, PartialStaleSnapshotUsesSafeHoldCommand) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::partialStale));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stopOrSafeHold,
                                           }));
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(NavigationAndEscapeCoordinatorTest, SurroundedSnapshotRestoresHeadingThenReverseEscape) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::surrounded));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::reverseEscapeSegment,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(NavigationAndEscapeCoordinatorTest, ReverseReadingsContinueReverseEscape) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::reverseReadings));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::reverseEscapeSegment,
                                           }));
}

TEST(NavigationAndEscapeCoordinatorTest, RequestRightSideProbeSuspendsCleaningBeforeProbeForAvoidance) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.RequestRightSideProbe(rvc::ProbeReason::rightTurnViability);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::probeRightSide,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(NavigationAndEscapeCoordinatorTest, LateralOpeningsTurnTowardAvailableSide) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::rightTurnViable));
    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::leftTurnViable));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::turnRight,
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::turnLeft,
                                           }));
}

TEST(NavigationAndEscapeCoordinatorTest, LeadingSectorBlockedPrefersRightWhenBothSidesAreViable) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();
    auto blocked = snapshot(rvc::FusedObstacleSnapshotKind::leadingSectorBlocked);
    blocked.leadingSectorBlocked = true;
    blocked.rightTurnViable = true;
    blocked.leftTurnViable = true;

    fixture.navigation.FusedObstacleSnapshot(blocked);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::turnRight,
                                           }));
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(NavigationAndEscapeCoordinatorTest, LeadingSectorBlockedTurnsLeftWhenOnlyLeftIsViable) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();
    auto blocked = snapshot(rvc::FusedObstacleSnapshotKind::leadingSectorBlocked);
    blocked.leadingSectorBlocked = true;
    blocked.leftTurnViable = true;

    fixture.navigation.FusedObstacleSnapshot(blocked);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::turnLeft,
                                           }));
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(NavigationAndEscapeCoordinatorTest, RightTurnInvalidAvoidsLeft) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();
    auto blocked = snapshot(rvc::FusedObstacleSnapshotKind::rightTurnInvalid);
    blocked.leadingSectorBlocked = true;
    blocked.leftTurnViable = true;

    fixture.navigation.FusedObstacleSnapshot(blocked);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::turnLeft,
                                           }));
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(NavigationAndEscapeCoordinatorTest, NoLateralOpeningUsesFallbackCommand) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::noLateralOpening));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::fallbackOrEscalateTBD,
                                           }));
}

TEST(NavigationAndEscapeCoordinatorTest, UnstableSnapshotSuppressesMotion) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::unstable));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::suppressMotionTBD,
                                           }));
}

TEST(NavigationAndEscapeCoordinatorTest, LeadingSectorSafeResumesForwardCleaning) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::leadingSectorSafe));

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::forward,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::normal,
                                             }));
}

TEST(NavigationAndEscapeCoordinatorTest, ValidSnapshotsDoNotEmitCommands) {
    NavigationAndEscapeCoordinatorFixture fixture;
    fixture.activateSession();

    fixture.navigation.FusedObstacleSnapshot(snapshot(rvc::FusedObstacleSnapshotKind::valid));

    EXPECT_TRUE(fixture.motionSink.commands.empty());
    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}
