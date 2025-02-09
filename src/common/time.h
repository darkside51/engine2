#pragma once

#include <chrono>
#include <ctime>
#include <cstdlib>

namespace engine::time {
    using SteadyClock = std::chrono::steady_clock;
    using SteadyTimePoint = SteadyClock::time_point;

    using SystemClock = std::chrono::system_clock;
    using SystemTimePoint = SystemClock::time_point;

    using HighResolutionClock = std::chrono::high_resolution_clock;
    using HighResolutionTimePoint = HighResolutionClock::time_point;

    template <typename TimePoint, typename T = std::chrono::milliseconds>
    [[maybe_unused]] inline uint64_t current_time(const TimePoint t) noexcept {
        const auto timePoint = std::chrono::time_point_cast<T>(t);
        return static_cast<uint64_t>(timePoint.time_since_epoch().count());
    }

	template <typename T = std::chrono::milliseconds>
    [[maybe_unused]] inline uint64_t steady_time() noexcept {
		return current_time<SteadyTimePoint, T>(SteadyClock::now());
	}

	template <typename T = std::chrono::milliseconds>
    [[maybe_unused]] inline uint64_t system_time() noexcept {
        return current_time<SystemTimePoint, T>(SystemClock::now());
	}

	template <typename T = std::chrono::milliseconds>
    [[maybe_unused]] inline uint64_t high_resoution_time() noexcept {
        return current_time<HighResolutionTimePoint, T>(HighResolutionClock::now());
	}

    inline uint64_t unixUTCTime() noexcept {
#ifdef OS_WINDOWS
		// https://stackoverflow.com/questions/20370920/convert-current-time-from-windows-to-unix-timestamp-in-c-or-c
		// c++20 introduced a guarantee that time_since_epoch is relative to the UNIX epoch (but c++20)

		time_t ltime;
		tm timeinfo;

		time(&ltime); // get time
		gmtime_s(&timeinfo, &ltime); /* Convert to UTC */

		return static_cast<uint64_t>(mktime(&timeinfo));
#elif defined(OS_LINUX)
        return system_time();
#endif
        return system_time();
	}
}