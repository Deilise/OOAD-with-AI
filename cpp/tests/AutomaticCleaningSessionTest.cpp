#include "rvc/AutomaticCleaningSession.hpp"
#include "rvc/NavigationAndEscapeCoordinator.hpp"
#include "rvc/SurfaceCleaningController.hpp"

#include "TestDoubles.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

struct AutomaticCleaningSessionFixture {
    rvc_test::RecordingMotionSink motionSink;
    rvc_test::RecordingCleaningSink cleaningSink;
    rvc::SurfaceCleaningController cleaning{cleaningSink};
    rvc::NavigationAndEscapeCoordinator navigation{motionSink, cleaning};
    rvc::AutomaticCleaningSession session{navigation, cleaning};

    void clearSinks() {
        motionSink.clear();
        cleaningSink.clear();
    }
};

} // namespace

TEST(AutomaticCleaningSessionTest, StartSessionEnablesNavigationAndCleaningPolicies) {
    AutomaticCleaningSessionFixture fixture;

    fixture.session.StartSession(rvc::SessionSource::User);
    fixture.navigation.FusedObstacleSnapshot({rvc::FusedObstacleSnapshotKind::leadingSectorSafe});
    fixture.cleaning.DustSignalUpdated(rvc::DustSignal::invalid);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::forward,
                                               rvc::MotionCommand::forward,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::normal,
                                                 rvc::CleaningCommand::normal,
                                             }));
}

TEST(AutomaticCleaningSessionTest, StopSessionStopsMotionAndSuspendsCleaning) {
    AutomaticCleaningSessionFixture fixture;

    fixture.session.StartSession(rvc::SessionSource::User);
    fixture.clearSinks();
    fixture.session.StopSession();

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stop,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(AutomaticCleaningSessionTest, ResumeSessionEnablesPoliciesAfterStop) {
    AutomaticCleaningSessionFixture fixture;

    fixture.session.StartSession(rvc::SessionSource::User);
    fixture.session.StopSession();
    fixture.clearSinks();
    fixture.session.ResumeSession(rvc::SessionSource::User);
    fixture.navigation.FusedObstacleSnapshot({rvc::FusedObstacleSnapshotKind::leadingSectorSafe});

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::forward,
                                               rvc::MotionCommand::forward,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::normal,
                                             }));
}

TEST(AutomaticCleaningSessionTest, RequestServiceOrResetStopsMotionAndSuspendsCleaning) {
    AutomaticCleaningSessionFixture fixture;

    fixture.session.StartSession(rvc::SessionSource::User);
    fixture.clearSinks();
    fixture.session.requestServiceOrReset();

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stop,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}
