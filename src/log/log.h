#pragma once

#include "../common/time.h"
#include "../utils/format_string.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_USER 4

namespace engine {
    template <typename P, typename... Args>
    inline void print(P && p, const char* fmt, Args &&...args) {
        p(fmt, std::forward<Args>(args)...);    
    }

    enum class LogLevel : uint8_t {
        eDebug = LOG_LEVEL_DEBUG,
        eInfo = LOG_LEVEL_INFO,
        eWarning = LOG_LEVEL_WARNING,
        eError = LOG_LEVEL_ERROR,
        eUser = LOG_LEVEL_USER
    };

    inline const char * logLevelToStr(LogLevel level) noexcept {
        switch (level) {
            case LogLevel::eDebug:
                return "debug:";
            case LogLevel::eInfo:
                return "i:";
            case LogLevel::eWarning:
                return "warning:";
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
            const auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
            const auto format = fmt_string("[%lu] <%lu> %s\n", time::unixUTCTime(), threadId, fmt);
            _printer(format, std::forward<Args>(args)...);
        }

        template <typename... Args>
        inline void log(LogLevel level, const char* fmt, Args &&...args) const {
            const auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
            const auto format = fmt_string("[%lu] <%lu> (%s) %s\n", time::unixUTCTime(), threadId, _printer.level(level), fmt);
            _printer(format, std::forward<Args>(args)...);
        }

    private:
        P _printer;
    };

    class ConsoleLog {
    public:
        template <typename... Args>
        inline void operator() (const char* fmt, Args &&...args) const {
	        printf(fmt, std::forward<Args>(args)...);
        }

        inline constexpr const char * level(LogLevel l) const noexcept {
            constexpr std::string_view kYellow  = "\x1B[33m";
            constexpr std::string_view kRed     = "\x1B[31m";
            constexpr std::string_view kGreen   = "\x1B[32m";
            constexpr std::string_view kCyan    = "\x1B[36m";
            constexpr std::string_view kBlack   = "\033[0m";
            constexpr std::string_view kOrange  = "\x1b[38;5;203m";
            
            switch (l) {
                case LogLevel::eDebug:
                    return fmt_string("%s%s%s", kYellow.data(), logLevelToStr(l), kBlack.data());
                case LogLevel::eInfo:
                    return fmt_string("%s%s%s", kCyan.data(), logLevelToStr(l), kBlack.data());
                case LogLevel::eWarning:
                    return fmt_string("%s%s%s", kOrange.data(), logLevelToStr(l), kBlack.data());
                case LogLevel::eError:
                    return fmt_string("%s%s%s", kRed.data(), logLevelToStr(l), kBlack.data());
                case LogLevel::eUser: [[fallthrough]];
                default:
                    return fmt_string("%s%s%s", kGreen.data(), logLevelToStr(l), kBlack.data());
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

#if MIN_LOG_LEVEL <= LOG_LEVEL_WARNING
#define LOG_W(...) engine::Console_Logger{}.log(engine::LogLevel::eWarning, __VA_ARGS__)
#else
#define LOG_W(...)
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