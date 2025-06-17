#pragma once

#include "types.h"

#include <algorithm>
#include <cstdint>

namespace engine::rhi {

inline Extent2D getMipLevelSize(Extent2D size, uint8_t level) noexcept {
    return {std::max(size.width >> level, 1u), 
            std::max(size.height >> level, 1u)};
}

class FrameIndex {
public:
    inline static constexpr uint8_t kUndefinedFrame = 0xffu;
    
    FrameIndex() : _framesCount(2u) {}

    explicit FrameIndex(uint8_t framesCount) : _framesCount(framesCount) {}

    inline void setFramesCount(uint8_t framesCount) noexcept { _framesCount = framesCount; }

    inline uint8_t operator()() const noexcept { return _idx; }
    inline uint8_t size() const noexcept { return _framesCount; }
    
    inline uint8_t operator++() noexcept { 
        _idx = (_idx + 1u) % _framesCount;
        return _idx; 
    }
    
private:
    uint8_t _framesCount = 1u;
    uint8_t _idx = 0u;
};

}