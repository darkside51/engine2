#pragma once

#include "image_frame.h"
#include "math/math.h"

#include <span>
#include <string>
#include <utility>
#include <vector>

namespace engine::graphics {

enum class AtlasFormat : uint8_t {
    eUnknown = 0u,
    eEngine2 = 1u,
    eSpine = 2u,
    eGodot = 3u,
    ePixiJs = 4u,
    eMapBox = 5u
};

class FramesParser {
public:
    struct Result {
        std::vector<std::pair<std::string, Frame2D>> frames;
    };

    static Result loadFrames(std::span<const std::byte> rawBytes, vec2u size, AtlasFormat format = AtlasFormat::eUnknown);
    static Result loadFramesCommon(std::span<const std::byte> rawBytes, vec2u size);
    static Result loadFramesSpine(std::span<const std::byte> rawBytes, vec2u size);
    static Result loadFramesGodot(std::span<const std::byte> rawBytes, vec2u size);
    static Result loadFramesPixiJs(std::span<const std::byte> rawBytes, vec2u size);
    static Result loadFramesMapBox(std::span<const std::byte> rawBytes, vec2u size);
};

}