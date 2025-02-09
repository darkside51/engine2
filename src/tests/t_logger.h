#pragma once

#include "../log/log.h"

namespace engine::tests {

    struct LoggerTest {
        void main() {
            engine::print(engine::ConsoleLog{}, "engine::print test value: %d\n", 1);

            const auto logger = engine::Logger(engine::ConsoleLog{});
            logger.log("create logger test: %d", 2);

            auto cLogger = engine::Console_Logger{};

            cLogger.log("engine::Console_Logger test: %d %s %d", 3, "abcd", 4);
            cLogger.log(engine::LogLevel::eInfo, "engine::Console_Logger size = %d (bytes)", sizeof(engine::Console_Logger));

            LOG_D("LOG_D test: %d%s", 123, "(units)");
            LOG_U("LOG_U test: %d%s", 1234567, "(units)");
        }
    };

}