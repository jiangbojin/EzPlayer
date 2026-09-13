cmake_minimum_required(VERSION 3.20)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GoogleTest 未在系统中发现，通过 FetchContent 自动引入...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(ezplayer_tests
    test_message_queue.cpp
    test_sonic_speed.cpp
    test_common_looper.cpp
    test_sync_queues.cpp
    ${CMAKE_SOURCE_DIR}/src/core/messagequeue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/sonic.cpp
    ${CMAKE_SOURCE_DIR}/src/core/commonlooper.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ff_ffplay_def.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/log/easylogging++.cc
)

target_include_directories(ezplayer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/utils/log
    ${FFMPEG_INCLUDE_DIRS}
)

target_compile_definitions(ezplayer_tests PRIVATE
    ELPP_THREAD_SAFE
    ELPP_FEATURE_CRASH_LOG
    ELPP_STL_LOGGING
)

target_link_libraries(ezplayer_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    FFmpeg::FFmpeg
    SDL2::SDL2
    Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ezplayer_tests PRIVATE ${CMAKE_DL_LIBS})
endif()

include(GoogleTest)
gtest_discover_tests(ezplayer_tests)
