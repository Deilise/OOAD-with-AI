#include "rvc/RvcSoftwareController.hpp"

#include "TestDoubles.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

struct RvcSoftwareControllerFixture {
    rvc_test::RecordingMotionSink motionSink;
    rvc_test::RecordingCleaningSink cleaningSink;
    rvc::RvcSoftwareController controller{motionSink, cleaningSink};
};

} // namespace

TEST(RvcSoftwareControllerTest, InitializePutsHardwareOutputsInSafeInactiveState) {
    RvcSoftwareControllerFixture fixture;

    fixture.controller.initialize();

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::stop,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                             }));
}

TEST(RvcSoftwareControllerTest, PortAccessorsExposeUsableInputPorts) {
    RvcSoftwareControllerFixture fixture;

    fixture.controller.sessionIntentPort().StartSession(rvc::SessionSource::User);
    fixture.controller.obstacleInputPort().ObstacleStateChanged(
        {rvc::ObstacleEventKind::forwardSafe, rvc::ProbePose::none, 1});
    fixture.controller.dustInputPort().DustSignalUpdated(rvc::DustSignal::aboveThreshold);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::forward,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::active,
                                                 rvc::CleaningCommand::boost,
                                                 rvc::CleaningCommand::normal,
                                             }));
}

TEST(RvcSoftwareControllerTest, DomainObjectAccessorsExposeOwnedObjects) {
    RvcSoftwareControllerFixture fixture;

    fixture.controller.session().StartSession(rvc::SessionSource::User);
    fixture.controller.perception().ObstacleStateChanged(
        {rvc::ObstacleEventKind::surrounded, rvc::ProbePose::right, 2});
    fixture.controller.cleaning().DustSignalUpdated(rvc::DustSignal::invalid);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::forbidForward,
                                               rvc::MotionCommand::reverse,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::suspend,
                                                 rvc::CleaningCommand::normal,
                                             }));
}
