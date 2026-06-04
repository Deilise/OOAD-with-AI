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

TEST(SurfaceCleaningControllerTest, DustSignalUpdatedBoostsThenReturnsToNormalWhenActive) {
    SurfaceCleaningControllerFixture fixture;
    fixture.cleaning.SessionStateChanged(true);

    fixture.cleaning.DustSignalUpdated(rvc::DustSignal::aboveThreshold);

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::boost,
                                                rvc::CleaningCommand::normal,
                                            }));
}

TEST(SurfaceCleaningControllerTest, DustSignalUpdatedKeepsNormalForInvalidSignal) {
    SurfaceCleaningControllerFixture fixture;
    fixture.cleaning.SessionStateChanged(true);

    fixture.cleaning.DustSignalUpdated(rvc::DustSignal::invalid);

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::normal,
                                            }));
}

TEST(SurfaceCleaningControllerTest, DustSignalUpdatedDefersBoostWhenSessionInactive) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.DustSignalUpdated(rvc::DustSignal::aboveThreshold);

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::unchangedOrDeferredTBD,
                                            }));
}

TEST(SurfaceCleaningControllerTest, SuspendCleaningForManeuverSendsSuspend) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.SuspendCleaningForManeuver();

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::suspend,
                                            }));
}

TEST(SurfaceCleaningControllerTest, ResumeCleaningAfterManeuverSendsActiveWhenSessionActive) {
    SurfaceCleaningControllerFixture fixture;
    fixture.cleaning.SessionStateChanged(true);

    fixture.cleaning.ResumeCleaningAfterManeuver();

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::active,
                                            }));
}

TEST(SurfaceCleaningControllerTest, ResumeCleaningAfterManeuverDefersWhenSessionInactive) {
    SurfaceCleaningControllerFixture fixture;

    fixture.cleaning.ResumeCleaningAfterManeuver();

    EXPECT_EQ(fixture.cleaningSink.commands, (std::vector<rvc::CleaningCommand>{
                                                rvc::CleaningCommand::unchangedOrDeferredTBD,
                                            }));
}
