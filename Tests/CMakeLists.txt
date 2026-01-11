cmake_minimum_required(VERSION 3.14)
project(NamoServerTests LANGUAGES CXX)

# C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent를 사용하여 의존성 다운로드
include(FetchContent)

# Google Test 다운로드
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Windows: gtest를 공유 CRT로 빌드
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# nlohmann/json 다운로드
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# 테스트 실행 파일 생성
add_executable(NamoServerTests
    test_main.cpp
    test_vector2int.cpp
    test_map.cpp
    test_rate_limiter.cpp
    test_data.cpp
)

# 라이브러리 링크
target_link_libraries(NamoServerTests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

# 테스트 등록
include(GoogleTest)
gtest_discover_tests(NamoServerTests)

# 컴파일러 경고 설정
if(MSVC)
    target_compile_options(NamoServerTests PRIVATE /W4)
else()
    target_compile_options(NamoServerTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
