#pragma once

#include "../common/time.h"
#include "../utils/format_string.h"

#include <cstdint>
#include <string>
#include <utility>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_USER 3

namespace engine {
    template <typename P, typename... Args>
    inline void print(P && p, const char* fmt, Args &&...args) {
        p(fmt, std::forward<Args>(args)...);    
    }

    enum class LogLevel : uint8_t {
        eDebug = LOG_LEVEL_DEBUG,
        eInfo = LOG_LEVEL_INFO,
        eError = LOG_LEVEL_ERROR,
        eUser = LOG_LEVEL_USER
    };

    inline const char * logLevelToStr(LogLevel level) noexcept {
        switch (level) {
            case LogLevel::eDebug:
                return "debug:";
            case LogLevel::eInfo:
                return "i:";
            case LogLevel::eError:
                return "error:";
            case LogLevel::eUser:
                return "u:";
        }
        return "";
    }

    template <typename P>
    class Logger final {
    public:
        Logger(P && p) : _printer(std::move(p)) {}
        Logger() = default;

        template <typename... Args>
        inline void log(const char* fmt, Args &&...args) const {
            const auto format = fmt_string("[%lu] %s\n", time::unixUTCTime(), fmt);
            _printer(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline void log(LogLevel level, const char* fmt, Args &&...args) const {
            const auto format = fmt_string("[%lu] (%s) %s\n", time::unixUTCTime(), _printer.level(level), fmt);
            _printer(format, std::forward<Args>(args)...);
        }

    private:
        P _printer;
    };

    class ConsoleLog {
    public:
        template <typename...Args>
        inline void operator() (const char* fmt, Args&&...args) const {
	        printf(fmt, std::forward<Args>(args)...);
        }

        inline constexpr const char * level(LogLevel l) const noexcept {
            const char * const kYellow  = "\x1B[33m";
            const char * const kRed     = "\x1B[31m";
            const char * const kGreen   = "\x1B[32m";
            const char * const kCyan    = "\x1B[36m";
            const char * const kBlack   = "\033[0m";

            switch (l) {
                case LogLevel::eDebug:
                    return fmt_string("%s%s%s", kYellow, logLevelToStr(l), kBlack);
                case LogLevel::eInfo:
                    return fmt_string("%s%s%s", kCyan, logLevelToStr(l), kBlack);
                case LogLevel::eError:
                    return fmt_string("%s%s%s", kRed, logLevelToStr(l), kBlack);
                case LogLevel::eUser: [[fallthrough]];
                default:
                    return fmt_string("%s%s%s", kGreen, logLevelToStr(l), kBlack);
            }
        }
    };

    using Console_Logger = Logger<ConsoleLog>;
}

#ifndef MIN_LOG_LEVEL
    #ifdef _DEBUG
        #define MIN_LOG_LEVEL LOG_LEVEL_DEBUG
    #else
        #define MIN_LOG_LEVEL LOG_LEVEL_INFO
    #endif
#endif

#define LOG(...) engine::Console_Logger{}.log(__VA_ARGS__)

#if MIN_LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_D(...) engine::Console_Logger{}.log(engine::LogLevel::eDebug, __VA_ARGS__)
#else
#define LOG_D(...)
#endif

#if MIN_LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_I(...) engine::Console_Logger{}.log(engine::LogLevel::eInfo, __VA_ARGS__)
#else
#define LOG_I(...)
#endif

#if MIN_LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_E(...) engine::Console_Logger{}.log(engine::LogLevel::eError, __VA_ARGS__)
#else
#define LOG_E(...)
#endif

#if MIN_LOG_LEVEL <= LOG_LEVEL_USER
#define LOG_U(...) engine::Console_Logger{}.log(engine::LogLevel::eUser, __VA_ARGS__)
#else
#define LOG_U(...)
#endif