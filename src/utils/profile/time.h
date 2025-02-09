#pragma once

#include "../../common/time.h"
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

namespace engine::profiler {

template <typename P = std::chrono::microseconds>
class ExecutionTime {
	public:
        ExecutionTime() : _startTime(time::steady_time<P>()) {}
		explicit ExecutionTime(const char* name) : _startTime(time::steady_time<P>()), _name(name) { }
		explicit ExecutionTime(std::string name) : _startTime(time::steady_time<P>()), _name(std::move(name)) { }

        inline std::string_view name() const noexcept { return _name; }

        inline uint64_t time() const noexcept {
            return time::steady_time<P>() - _startTime;
        }

        inline void restart() noexcept {
            _startTime = time::steady_time<P>();
        }

        inline void print() const noexcept {
            printf("%s execution time: %lu %s\n", _name.c_str(), time(), pointsFormat().data());
        }

		~ExecutionTime() = default;

	private:
        std::string_view pointsFormat() const noexcept {
            if constexpr (std::is_same_v<P, std::chrono::microseconds>) {
                return "microseconds";
            }

            if constexpr (std::is_same_v<P, std::chrono::milliseconds>) {
                return "milliseconds";
            }
            
            if constexpr (std::is_same_v<P, std::chrono::seconds>) {
                return "seconds";
            }

            return "";
        }

		uint64_t _startTime = 0u;
		std::string _name = "";
	};

}