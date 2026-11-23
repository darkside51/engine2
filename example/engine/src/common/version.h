#pragma once

#include <cstdint>

namespace engine::common {

    struct VersionParts {
        uint8_t major = 0u;
        uint8_t minor = 0u;
        uint8_t patch = 0u;
    };

    union Version {
    public:
        Version(uint8_t major, uint8_t minor, uint8_t patch) : _parts {major, minor, patch} { }
        explicit Version(uint32_t value) : _value {value} { }
        inline uint32_t operator()() const noexcept { return _value; }
        inline const VersionParts* operator->() const noexcept { return &_parts; }
    
    private:
        VersionParts _parts;
        uint32_t _value = 0u;
    };

}