/**
 * @file test_main.cpp
 * @brief GoogleTest entry point for DOPE-ASS native tests.
 */

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
