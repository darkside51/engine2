#include "atlas.h"
#include "log/log.h"
#include "utils/format_string.h"
#include "utils/json.h"
#include "utils/string_utils.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>

namespace engine::graphics {
    FramesParser::Result FramesParser::loadFrames(stl::span<const std::byte> rawBytes, vec2u size, std::string_view prefix, AtlasFormat format) {
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

    FramesParser::Result FramesParser::loadFramesCommon(stl::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesSpine(stl::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        auto readLine = [](std::string_view data, size_t & begin) noexcept -> std::string_view {
            if (begin >= data.size()) {
                return {};
            }

            auto end = data.find('\n', begin);
            if (end == std::string_view::npos) {
                end = data.size();
            }

            auto result = data.substr(begin, end - begin);
            begin = end + 1u;

            // strip CR of CRLF line endings (windows checkouts)
            if (!result.empty() && result.back() == '\r') {
                result.remove_suffix(1u);
            }

            return result;
        };

        auto readEntry = [](std::string_view data, stl::array<std::string_view, 5u> & result) noexcept -> uint8_t {
            size_t begin = 0u;
            while (begin < data.size() && (data[begin] == '\t' || data[begin] == ' ')) {
                ++begin;
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
        stl::array<std::string_view, 5u> entry = {};

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

            auto & frame = result.emplace_back(std::string{prefix}.append(line), Frame2D{}).second;

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

    FramesParser::Result FramesParser::loadFramesGodot(stl::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesPixiJs(stl::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::loadFramesMapBox(stl::span<const std::byte> rawBytes, vec2u size, std::string_view prefix) {
        return {};
    }

    FramesParser::Result FramesParser::generateFrames(vec2u size, std::string_view prefix) {
        vec2f step = vec2f{1.0f / size.x, 1.0f / size.y};

        FramesParser::Result result;
        result.reserve(size.x * size.y);

        for (uint32_t j = 0; j < size.y; ++j) {
            for (uint32_t i = 0; i < size.x; ++i) {
                auto & frame = result.emplace_back(fmt_string("%s%d", prefix.data(), j * size.x + i), Frame2D{}).second;
                frame.uvMin = {i * step.x, j * step.y};
                frame.uvMax = {(i + 1) * step.x, (j + 1) * step.y};
            }
        }

        return result;
    }

}