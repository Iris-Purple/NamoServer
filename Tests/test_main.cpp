/**
 * @file test_main.cpp
 * @brief Google Test 진입점
 *
 * NamoServer 게임 서버 단위 테스트 메인 파일
 */

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
