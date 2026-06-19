cmake_minimum_required(VERSION 3.16)

project(RvcSoftwareController LANGUAGES CXX)

option(RVC_BUILD_TESTS "Build GoogleTest tests" ON)
option(RVC_ENABLE_COVERAGE "Enable GCC/Clang coverage instrumentation" OFF)

function(rvc_enable_coverage target_name)
    if(RVC_ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE -O0 -g --coverage)
            target_link_options(${target_name} PRIVATE --coverage)
        else()
            message(FATAL_ERROR "RVC_ENABLE_COVERAGE requires GCC or Clang. Current compiler is ${CMAKE_CXX_COMPILER_ID}.")
        endif()
    endif()
endfunction()

add_library(rvc_controller
    src/AutomaticCleaningSession.cpp
    src/NavigationAndEscapeCoordinator.cpp
    src/ObstaclePerceptionContext.cpp
    src/RvcSoftwareController.cpp
    src/SurfaceCleaningController.cpp
)

target_include_directories(rvc_controller
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(rvc_controller PUBLIC cxx_std_17)
rvc_enable_coverage(rvc_controller)

add_executable(rvc_demo examples/demo.cpp)
target_link_libraries(rvc_demo PRIVATE rvc_controller)
rvc_enable_coverage(rvc_demo)

add_executable(rvc_simulator simulator/rvc_simulator.cpp)
target_link_libraries(rvc_simulator PRIVATE rvc_controller)
rvc_enable_coverage(rvc_simulator)

if(RVC_BUILD_TESTS)
    include(CTest)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(rvc_controller_tests
        tests/AutomaticCleaningSessionTest.cpp
        tests/NavigationAndEscapeCoordinatorTest.cpp
        tests/ObstaclePerceptionContextTest.cpp
        tests/RvcSoftwareControllerTest.cpp
        tests/SurfaceCleaningControllerTest.cpp
    )

    target_link_libraries(rvc_controller_tests
        PRIVATE
            rvc_controller
            GTest::gtest_main
    )
    rvc_enable_coverage(rvc_controller_tests)

    include(GoogleTest)
    gtest_discover_tests(rvc_controller_tests)

    if(RVC_ENABLE_COVERAGE)
        find_program(GCOV_EXECUTABLE gcov)
        find_program(LCOV_EXECUTABLE lcov)
        find_program(GENHTML_EXECUTABLE genhtml)
        find_program(BASH_EXECUTABLE bash)

        if(GCOV_EXECUTABLE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE AND BASH_EXECUTABLE)
            add_custom_target(coverage
                COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
                COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-lcov-report.sh ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS rvc_controller_tests
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Running tests and generating gcov/lcov coverage report"
            )
        else()
            message(STATUS "gcov, lcov, genhtml, or bash not found; coverage instrumentation is enabled, but report generation is unavailable.")
        endif()
    endif()
endif()
