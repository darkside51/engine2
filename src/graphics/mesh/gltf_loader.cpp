#include "gltf_loader.h"

#include "utils/json.h"
#include "utils/system.h"

#include <cstring>
#include <unordered_map>

namespace engine::graphics {
    
    template <typename K, typename T>
	using map_type = std::unordered_map<K, T>;

    auto constexpr swap4IfBigEndian = [](uint32_t & val) noexcept {
        if constexpr (isLittleEndian) {
            return;
        }
        //for bigendian
        uint32_t tmp = val;
        unsigned char* dst = reinterpret_cast<unsigned char*>(&val);
        unsigned char* src = reinterpret_cast<unsigned char*>(&tmp);

        dst[0u] = src[3u];
        dst[1u] = src[2u];
        dst[2u] = src[1u];
        dst[3u] = src[0u];
    };

    const static map_type<std::string, AttributesSemantic> semantics = {
        {"POSITION", AttributesSemantic::ePosition},
        {"NORMAL", AttributesSemantic::eNormal},
        {"TANGENT", AttributesSemantic::eTangent},
        {"COLOR_0", AttributesSemantic::eColor0},
        {"COLOR_1", AttributesSemantic::eColor1},
        {"COLOR_2", AttributesSemantic::eColor2},
        {"COLOR_3", AttributesSemantic::eColor3},
        {"JOINTS_0", AttributesSemantic::eJoints},
        {"WEIGHTS_0", AttributesSemantic::eWeight},
        {"TEXCOORD_0", AttributesSemantic::eUv0},
        {"TEXCOORD_1", AttributesSemantic::eUv1},
        {"TEXCOORD_2", AttributesSemantic::eUv2},
        {"TEXCOORD_3", AttributesSemantic::eUv3},
        {"TEXCOORD_4", AttributesSemantic::eUv4}
    };

    const static map_type<std::string, AccessorType> accesorTypes = {
        {"SCALAR", AccessorType::eScalar},
        {"VEC2", AccessorType::eVec2},
        {"VEC3", AccessorType::eVec3},
        {"VEC4", AccessorType::eVec4},
        {"MAT2", AccessorType::eMat2},
        {"MAT3", AccessorType::eMat3},
        {"MAT4", AccessorType::eMat4}
    };

    const static map_type<std::string, AnimationChannelPath> animChannelTypes = {
        {"translation", AnimationChannelPath::eTranslation},
        {"rotation", AnimationChannelPath::eRotation},
        {"scale", AnimationChannelPath::eScale},
        {"weights", AnimationChannelPath::eWeights}
    };

    const static map_type<std::string, Interpolation> interpolationTypes = {
        {"LINEAR", Interpolation::eLinear},
        {"STEP", Interpolation::eStep},
        {"CUBICSPLINE", Interpolation::eCubicSpline}
    };

    const static map_type<std::string, MimeType> mimeTypes = {
        {"image/jpeg", MimeType::eJpeg},
        {"image/png", MimeType::ePng}
    };

    const static map_type<std::string, AlphaMode> alphaModes = {
        {"OPAQUE", AlphaMode::eOpaque},
        {"BLEND", AlphaMode::eBlend},
        {"MASK", AlphaMode::eMask}
    };

    Layout loadModelLayout(std::span<std::byte const> bytes, std::string_view path) {
        const std::byte * binData = nullptr;
        size_t binSize = 0u;

        json::Json js{};

        if (static_cast<char>(bytes[0u]) == 'g' && static_cast<char>(bytes[1u]) == 'l'
            && static_cast<char>(bytes[2u]) == 'T' && static_cast<char>(bytes[3u]) == 'F') {
            // binary format
            uint32_t version;        // 4 bytes
			uint32_t length;         // 4 bytes
			uint32_t chunk0_length;  // 4 bytes
			uint32_t chunk0_format;  // 4 bytes

            std::memcpy(&version, &bytes[4u], 4u);
			std::memcpy(&length, &bytes[8u], 4u); // Total glb size, including header and all chunks
			std::memcpy(&chunk0_length, &bytes[12u], 4u); // JSON data length
			std::memcpy(&chunk0_format, &bytes[16u], 4u);

            swap4IfBigEndian(version);
            swap4IfBigEndian(length);
            swap4IfBigEndian(chunk0_length);
			swap4IfBigEndian(chunk0_format);

            constexpr uint8_t headerLength = 20u;
			const uint64_t header_and_json_size = headerLength + static_cast<uint64_t>(chunk0_length);

            // get bin data
			uint32_t chunk1_length = 0u; // 4 bytes
			uint32_t chunk1_format = 0u; // 4 bytes;
			std::memcpy(&chunk1_length, bytes.data() + header_and_json_size, 4u); // bin data length
			std::memcpy(&chunk1_format, bytes.data() + header_and_json_size + 4u, 4u);

            swap4IfBigEndian(chunk1_length);
			swap4IfBigEndian(chunk1_format);

			binData = bytes.data() + header_and_json_size + 8u; // 4 bytes (bin_buffer_length) + 4 bytes(bin_buffer_format)
			binSize = chunk1_length;

            // load json
            std::string_view input(reinterpret_cast<const char*>(bytes[headerLength]), chunk0_length);
            const auto err = json::read_json(js, input);
        } else {
            // load json
            std::string_view input(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            const auto err = json::read_json(js, input);
        }

        return {};
    }
}