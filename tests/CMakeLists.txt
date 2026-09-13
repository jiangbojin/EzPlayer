# 补充 Conan 2.x 生成器搜索路径
foreach(CONAN_GEN_DIR
    "${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/generators"
    "${CMAKE_BINARY_DIR}/build/Release/generators"
    "${CMAKE_BINARY_DIR}/build/Debug/generators"
    "${CMAKE_BINARY_DIR}/generators"
)
    if(EXISTS "${CONAN_GEN_DIR}")
        list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GEN_DIR}")
    endif()
endforeach()

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

find_package(spdlog REQUIRED)

add_executable(ezplayer_tests
    test_message_queue.cpp
    test_sonic_speed.cpp
    test_common_looper.cpp
    test_sync_queues.cpp
    ${CMAKE_SOURCE_DIR}/src/core/messagequeue.cpp
    ${CMAKE_SOURCE_DIR}/src/core/sonic.cpp
    ${CMAKE_SOURCE_DIR}/src/core/commonlooper.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ff_ffplay_def.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/log/logger.cpp
)

target_include_directories(ezplayer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/utils/log
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(ezplayer_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    spdlog::spdlog
    FFmpeg::FFmpeg
    SDL2::SDL2
    Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ezplayer_tests PRIVATE ${CMAKE_DL_LIBS})
endif()

include(GoogleTest)
gtest_discover_tests(ezplayer_tests)
