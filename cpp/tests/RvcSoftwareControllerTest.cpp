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

rvc::ObstacleEvent event(rvc::ObstacleEventKind kind, rvc::TimeStamp time) {
    rvc::ObstacleEvent result;
    result.kind = kind;
    result.sampleTime = time;
    return result;
}

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
        event(rvc::ObstacleEventKind::leadingSectorSafe, 1));
    fixture.controller.dustInputPort().DustSignalUpdated(rvc::DustSignal::aboveThreshold);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::forward,
                                               rvc::MotionCommand::forward,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::normal,
                                                 rvc::CleaningCommand::normal,
                                             }));
}

TEST(RvcSoftwareControllerTest, DomainObjectAccessorsExposeOwnedObjects) {
    RvcSoftwareControllerFixture fixture;

    fixture.controller.session().StartSession(rvc::SessionSource::User);
    fixture.motionSink.clear();
    fixture.controller.perception().ObstacleStateChanged(event(rvc::ObstacleEventKind::surrounded, 2));
    fixture.controller.cleaning().DustSignalUpdated(rvc::DustSignal::invalid);

    EXPECT_EQ(fixture.motionSink.commands, (std::vector<rvc::MotionCommand>{
                                               rvc::MotionCommand::restoreHeading,
                                               rvc::MotionCommand::reverseEscapeSegment,
                                           }));
    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                 rvc::CleaningCommand::normal,
                                                 rvc::CleaningCommand::suspend,
                                             }));
}
