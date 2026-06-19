#include "../simul/SimulatorCore.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

std::size_t countCells(const rvc_simul::BoardLayout& board, rvc_simul::BoardCell cell) {
    std::size_t count = 0;
    for (const auto& row : board.cells) {
        count += static_cast<std::size_t>(std::count(row.begin(), row.end(), cell));
    }
    return count;
}

} // namespace

TEST(SimulatorCoreTest, ReferenceBoardMatchesApprovedImageLayout) {
    const auto board = rvc_simul::referenceBoard();

    ASSERT_EQ(board.cells.size(), 10U);
    for (const auto& row : board.cells) {
        EXPECT_EQ(row.size(), 10U);
    }

    EXPECT_EQ(board.cells[9][1], rvc_simul::BoardCell::RobotStart);
    EXPECT_EQ(board.cells[1][1], rvc_simul::BoardCell::Dust);
    EXPECT_EQ(board.cells[1][8], rvc_simul::BoardCell::Dust);
    EXPECT_EQ(board.cells[8][3], rvc_simul::BoardCell::Dust);
    EXPECT_EQ(board.cells[0][0], rvc_simul::BoardCell::Obstacle);
    EXPECT_EQ(board.cells[9][9], rvc_simul::BoardCell::Obstacle);

    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::Obstacle), 60U);
    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::Dust), 8U);
    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::RobotStart), 1U);
    EXPECT_EQ(countCells(board, rvc_simul::BoardCell::Floor), 31U);
}

TEST(SimulatorCoreTest, ScenarioCatalogContainsExistingScenariosAndPhotoScenario) {
    const auto allScenarios = rvc_simul::scenarios();

    ASSERT_EQ(allScenarios.size(), 31U);
    EXPECT_EQ(allScenarios.front().id, "TC-01");
    EXPECT_EQ(allScenarios[29].id, "TC-30");
    EXPECT_EQ(allScenarios.back().id, "TC-31");
    EXPECT_EQ(allScenarios.back().board.cells[9][1], rvc_simul::BoardCell::RobotStart);
}

TEST(SimulatorCoreTest, AllScenariosPassExpectedCommandTraces) {
    for (const auto& scenario : rvc_simul::scenarios()) {
        const auto result = rvc_simul::runScenario(scenario);
        EXPECT_TRUE(result.passed) << scenario.id << " " << scenario.name;
    }
}

TEST(SimulatorCoreTest, StepExecutionMatchesFullExecutionForPhotoScenario) {
    const auto allScenarios = rvc_simul::scenarios();
    const auto photoScenario = std::find_if(
        allScenarios.begin(),
        allScenarios.end(),
        [](const rvc_simul::Scenario& scenario) { return scenario.id == "TC-31"; });

    ASSERT_NE(photoScenario, allScenarios.end());

    rvc_simul::ScenarioRunner runner{*photoScenario};
    while (runner.canAdvance()) {
        runner.advance();
    }

    const auto stepped = runner.result();
    const auto full = rvc_simul::runScenario(*photoScenario);

    EXPECT_TRUE(stepped.passed);
    EXPECT_EQ(stepped.actualMotion, full.actualMotion);
    EXPECT_EQ(stepped.actualCleaning, full.actualCleaning);
}
