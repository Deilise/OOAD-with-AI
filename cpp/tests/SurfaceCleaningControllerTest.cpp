#include "rvc/SurfaceCleaningController.hpp"

#include "TestDoubles.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

struct SurfaceCleaningControllerFixture {
    rvc_test::RecordingCleaningSink cleaningSink;
    rvc::SurfaceCleaningController cleaning{cleaningSink};
};

} // namespace

TEST(SurfaceCleaningControllerTest, SessionStateChangedActiveEnablesPolicyWithoutImmediateCommand) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.SessionStateChanged(true);

    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(SurfaceCleaningControllerTest, SessionStateChangedInactiveSuspendsCleaning) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.SessionStateChanged(false);

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::suspend,
                                            }));
}

TEST(SurfaceCleaningControllerTest, SessionStateChangedWithNormalModeSendsNormal) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.SessionStateChanged(true, rvc::CleaningMode::Normal);

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::normal,
                                            }));
}

TEST(SurfaceCleaningControllerTest, DustSignalUpdatedStoresSignalWithoutImmediateCommand) {
    SurfaceCleaningControllerFixture fixture;
    fixture.cleaning.SessionStateChanged(true);

    fixture.cleaning.DustSignalUpdated(rvc::DustSignal::aboveThreshold);

    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}

TEST(SurfaceCleaningControllerTest, StartBoostCleaningSendsBoostWhenSessionActive) {
    SurfaceCleaningControllerFixture fixture;
    fixture.cleaning.SessionStateChanged(true);

    fixture.cleaning.StartBoostCleaning();

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::boost,
                                            }));
}

TEST(SurfaceCleaningControllerTest, SuspendCleaningForManeuverSendsSuspend) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.SuspendCleaningForManeuver();

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::suspend,
                                            }));
}

TEST(SurfaceCleaningControllerTest, ResumeCleaningAfterManeuverSendsNormalWhenSessionActive) {
    SurfaceCleaningControllerFixture fixture;
    fixture.cleaning.SessionStateChanged(true);

    fixture.cleaning.ResumeCleaningAfterManeuver();

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::normal,
                                            }));
}

TEST(SurfaceCleaningControllerTest, ResumeCleaningAfterManeuverDefersWhenSessionInactive) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.ResumeCleaningAfterManeuver();

    EXPECT_TRUE(fixture.cleaningSink.commands.empty());
}
