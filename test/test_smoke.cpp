/**
 * @file test_smoke.cpp
 * @brief Smoke test — verifies DOPE library links and basic init works.
 */

#include <gtest/gtest.h>
#include "dope/dope_api.h"
#include "dope/dope_types.h"

TEST(Smoke, BCE_InitAndModeIsIdle)
{
    BCE_Init();
    EXPECT_EQ(BCE_GetMode(), BCE_Mode::IDLE);
}

TEST(Smoke, BCE_FaultsAfterUpdateWithNoSensors)
{
    BCE_Init();

    // Feed an empty SensorFrame — no valid sensor data
    SensorFrame frame{};
    BCE_Update(&frame);

    // Engine should be in IDLE or FAULT with no range data
    BCE_Mode mode = BCE_GetMode();
    EXPECT_TRUE(mode == BCE_Mode::IDLE || mode == BCE_Mode::FAULT);
}
