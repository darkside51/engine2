#include "atlas.h"
#include "log/log.h"
#include "utils/json.h"
#include "utils/string_utils.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>

namespace engine::graphics {
    FramesParser::Result FramesParser::loadFrames(std::span<const std::byte> rawBytes, vec2u size, std::string_view prefix, AtlasFormat format) {
        switch (format) {
        case AtlasFormat::eUnknown:
        { // parse format from file content

        }
            break;
        case AtlasFormat::eEngine2:
            return loadFramesCommon(rawBytes, size, prefix);
        case AtlasFormat::eSpine:
            return loadFramesSpine(rawBytes, size, prefix);
        case AtlasFormat::eGodot:
            return loadFramesGodot(rawBytes, size, prefix);
        case AtlasFormat::ePixiJs:
            return loadFramesPixiJs(rawBytes, size, prefix);
        case AtlasFormat::eMapBox:
            return loadFramesMapBox(rawBytes, size, prefix);
        default:
            break;
        }

        return {};
    }

    FramesParser::Result FramesParser::loadFramesCommon(std::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesSpine(std::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        auto readLine = [](std::string_view data, size_t & begin) noexcept -> std::string_view {
            auto end = data.find('\n', begin);
            if (end != std::string_view::npos) {
                auto result = data.substr(begin, end - begin);
                begin = end + 1u; 
                return result;
            }
            return {};
        };

        auto readEntry = [](std::string_view data, std::array<std::string_view, 5u> & result) noexcept -> uint8_t {
            size_t begin = 0u;
            while (true) {
                if (data[begin] == '\t' || data[begin] == ' ') {
                    ++begin;
                } else {
                    break;
                }
            }
            auto dots = data.find(':', begin);
            if (dots == std::string_view::npos) {
                return 0u;
            }
            result[0u] = data.substr(begin, dots - begin);
            uint8_t count = 0u;

            auto pos = dots + 1u;
            auto delim = data.find(',', pos);
            uint8_t i = 1u;
            while (delim != std::string_view::npos) {
                ++count;
                result[i] = data.substr(pos + 1u, delim - pos - 1u);
                pos = delim + 1u;
                delim = data.find(',', pos);
                ++i;
            }

            ++count;
            delim = data.find('\n', pos);
            result[i] = data.substr(pos + 1u, delim - pos - 1u);

            return count;
        };

        std::string_view sv(reinterpret_cast<const char*>(rawBytes.data()), rawBytes.size());
        std::array<std::string_view, 5u> entry = {};

        size_t pos = 0u;
        auto line = readLine(sv, pos);
        // read texture metadata
        while (true) {
            line = readLine(sv, pos);
            if (readEntry(line, entry) == 0u) break;
            if (entry[0u] == "size") {
                size = {from_chars<uint32_t>(entry[1u]), from_chars<uint32_t>(entry[2u])};
            }
        }

        assert(size.x != 0u || size.y != 0u);

        // read keys:
        FramesParser::Result result;
        while (true) {
            if (line.empty()) {
                break;
            }

            auto & frame = result.frames.emplace_back(std::string{prefix}.append(line), Frame2D{}).second;

            vec2f xy = vec2f{0.0f};
            vec2f regionSize = vec2f{0.0f};
            bool rotated = false;

            while (true) {
                line = readLine(sv, pos);
                if (line.empty()) {
                    break;
                }

                const auto count = readEntry(line, entry);
                if (count == 0u) {
                    break;
                }

                if (entry[0u] == "xy") {
					xy = {from_chars<float>(entry[1u]), from_chars<float>(entry[2u])};
				} else if (entry[0u] == "size") {
					regionSize = {from_chars<float>(entry[1u]), from_chars<float>(entry[2u])};
				} else if (entry[0u] == "bounds") {
                    xy = {from_chars<float>(entry[1u]), from_chars<float>(entry[2u])};
                    regionSize = {from_chars<float>(entry[3u]), from_chars<float>(entry[4u])};
                } else if (entry[0] == "rotate") {
                    if (entry[1] != "false") {
						rotated = true;
					}
                }
            }

            frame.uvMin = {xy.x / size.x, xy.y / size.y};

            if (rotated) {
                frame.uvMax = {(xy.x + regionSize.y) / size.x,
                               (xy.y + regionSize.x) / size.y};
            } else {
                frame.uvMax = {(xy.x + regionSize.x) / size.x,
                               (xy.y + regionSize.y) / size.y};
            }
        }

        return result;
    }

    FramesParser::Result FramesParser::loadFramesGodot(std::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesPixiJs(std::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesMapBox(std::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

}