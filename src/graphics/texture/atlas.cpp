#include "atlas.h"
#include "utils/json.h"

namespace engine::graphics {

    FramesParser::Result FramesParser::loadFrames(std::span<const std::byte> rawBytes, vec2u size, AtlasFormat format) {
        switch (format) {
        case AtlasFormat::eUnknown:
        { // parse format from file content

        }
            break;
        case AtlasFormat::eEngine2:
            return loadFramesCommon(rawBytes, size);
        case AtlasFormat::eSpine:
            return loadFramesSpine(rawBytes, size);
        case AtlasFormat::eGodot:
            return loadFramesGodot(rawBytes, size);
        case AtlasFormat::ePixiJs:
            return loadFramesPixiJs(rawBytes, size);
        case AtlasFormat::eMapBox:
            return loadFramesMapBox(rawBytes, size);
        default:
            break;
        }
    }


    FramesParser::Result FramesParser::loadFramesCommon(std::span<const std::byte> rawBytes, vec2u size) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesSpine(std::span<const std::byte> rawBytes, vec2u size) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesGodot(std::span<const std::byte> rawBytes, vec2u size) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesPixiJs(std::span<const std::byte> rawBytes, vec2u size) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesMapBox(std::span<const std::byte> rawBytes, vec2u size) {
        return {};
    }

}