#include <gtest/gtest.h>
#include "utils/log/logger.h"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ezplayer::log::init("log");
    return RUN_ALL_TESTS();
}
