
cmake_minimum_required(VERSION 3.20)
project(DisasterReliefSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(disaster_relief
    src/main.cpp
    src/PriorityManager.cpp
)

target_include_directories(disaster_relief PRIVATE include)

include(CTest)

if(BUILD_TESTING)
    add_executable(priority_manager_tests
        tests/PriorityManagerTests.cpp
        src/PriorityManager.cpp
    )

    target_include_directories(priority_manager_tests PRIVATE include)

    add_test(
        NAME PriorityManagerTests
        COMMAND priority_manager_tests
    )
endif()
