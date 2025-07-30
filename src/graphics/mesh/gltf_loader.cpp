#include "gltf_loader.h"

#include "common/hash.h"
#include "math/math.h"
#include "utils/base64.h"
#include "utils/json.h"
#include "utils/system.h"

#include <cassert>
#include <charconv>
#include <cstring>
#include <unordered_map>

namespace engine::graphics::model {
    
    template <typename K, typename T>
	using map_type = std::unordered_map<K, T>;

    template <typename T>
    using string_map = std::unordered_map<std::string_view, T, hash::String_hash, std::equal_to<>>;

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

    const static string_map<AttributesSemantic> semantics = {
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

    const static string_map<AccessorType> accesorTypes = {
        {"SCALAR", AccessorType::eScalar},
        {"VEC2", AccessorType::eVec2},
        {"VEC3", AccessorType::eVec3},
        {"VEC4", AccessorType::eVec4},
        {"MAT2", AccessorType::eMat2},
        {"MAT3", AccessorType::eMat3},
        {"MAT4", AccessorType::eMat4}
    };

    const static string_map<AnimationChannelPath> animChannelTypes = {
        {"translation", AnimationChannelPath::eTranslation},
        {"rotation", AnimationChannelPath::eRotation},
        {"scale", AnimationChannelPath::eScale},
        {"weights", AnimationChannelPath::eWeights}
    };

    const static string_map<Interpolation> interpolationTypes = {
        {"LINEAR", Interpolation::eLinear},
        {"STEP", Interpolation::eStep},
        {"CUBICSPLINE", Interpolation::eCubicSpline}
    };

    const static string_map<MimeType> mimeTypes = {
        {"image/jpeg", MimeType::eJpeg},
        {"image/png", MimeType::ePng}
    };

    const static string_map<AlphaMode> alphaModes = {
        {"OPAQUE", AlphaMode::eOpaque},
        {"BLEND", AlphaMode::eBlend},
        {"MASK", AlphaMode::eMask}
    };

    template <typename T, typename F, typename... Args>
	void parseArray(std::vector<T>& arr, F && f, std::string_view name, const json::Json & js, Args &&... args) {
		if (auto * target = json::findChild(js, name)) {
            const auto & array = target->get_array();
			const size_t count = array.size();
			arr.resize(count);
			for (size_t i = 0u; i < count; ++i) {
				f(arr[i], array[i], std::forward<Args>(args)...);
			}
		}
	}

    void parseScene(Scene & scene, const json::Json & object) noexcept {
		scene.name = json::getValue(object, "name", std::string_view(""));
        scene.nodes = json::getArray<uint16_t>(object, "nodes");
	}

    void parseNode(Node & node, const json::Json & object) noexcept {
        node.name = json::getValue(object, "name", std::string_view(""));
        node.mesh = json::getValue<uint16_t>(object, "mesh", kInvalidValue);
        node.skin = json::getValue<uint16_t>(object, "skin", kInvalidValue);

        node.weights = json::getArray<float>(object, "weights"); // ?
        node.children = json::getArray<uint16_t>(object, "children");

        const std::vector<float>  matrixArray = json::getArray<float>(object, "matrix");
        if (!matrixArray.empty()) {
            mat4f matrix = glm::make_mat4(matrixArray.data());
            vec3f scale;
            vec3f translation;
            glm::quat rotation;
            vec3f skew;
            vec4f perspective;
            glm::decompose(matrix, scale, rotation, translation, skew, perspective);

            // set values to node! (glm vectorisation make vec3 size == vec4 size)
            node.scale = {scale.x, scale.y, scale.z};
            node.rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
            node.translation = {translation.x, translation.y, translation.z};
            return;
        }

        if (auto * scale = json::findChild(object, "scale")) {
            const auto & array = scale->get_array();
            node.scale = {array[0].as<float>(), array[1].as<float>(), array[2].as<float>()};
        }

        if (auto * rotation = json::findChild(object, "rotation")) {
            const auto & array = rotation->get_array();
            node.rotation = {array[0].as<float>(), array[1].as<float>(), array[2].as<float>(), array[3].as<float>()};
        }

        if (auto * translation = json::findChild(object, "translation")) {
            const auto & array = translation->get_array();
            node.translation = {array[0].as<float>(), array[1].as<float>(), array[2].as<float>()};
        }
    }

    void parseMesh(Mesh & mesh, const json::Json & object) {
		mesh.name = json::getValue(object, "name", std::string_view(""));
        mesh.weights = json::getArray<float>(object, "weights"); // ??

        auto const & primitivesArray = object["primitives"].get_array();
        mesh.primitives.reserve(primitivesArray.size());
        for (const auto & primitive : primitivesArray) {
            const uint8_t indices = json::getValue(primitive, "indices", kInvalidValue);
            const uint8_t material = json::getValue(primitive, "material", kInvalidValue);
            const uint8_t mode = json::getValue(primitive, "mode", 4u);

            auto & meshPrimitive = mesh.primitives.emplace_back(
                std::map<AttributesSemantic, uint16_t>{}, 
                indices, 
                material, 
                static_cast<PrimitiveMode>(mode)
            );

            auto const & attributesArray = primitive["attributes"].get_object();
            for (const auto & [key, attribute] : attributesArray) {
                if (auto it = semantics.find(key); it != semantics.end()) {
                    meshPrimitive.attributes.emplace(it->second, attribute.as<uint16_t>());
                }
            }
        }
	}

    void parseBuffer(Buffer & buffer, const json::Json & object, std::string_view path, const std::byte * binData) {
        buffer.name = json::getValue(object, "name", std::string_view(""));
		buffer.byteLength = object["byteLength"].as<uint32_t>();

        std::string_view uri = json::getValue(object, "uri", std::string_view(""));

		if (!uri.empty()) {
			constexpr std::string_view octetStreamHeader = "data:application/octet-stream;base64,";
			constexpr std::string_view bufferHeader = "data:application/gltf-buffer;base64,";

            if (uri.contains(octetStreamHeader)) {
                const std::string data = base64_decode(uri.substr(octetStreamHeader.size()));
                buffer.data.resize(buffer.byteLength);
                std::memcpy(buffer.data.data(), data.data(), buffer.byteLength);
            } else if (uri.contains(bufferHeader)) {
                const std::string data = base64_decode(uri.substr(bufferHeader.size()));
                buffer.data.resize(buffer.byteLength);
                std::memmove(buffer.data.data(), data.data(), buffer.byteLength);
            } else {
                assert(false);
                // auto & fm = FileManager;
                // buffer.data = fm.readFile((folder + uri));
            }
		} else if (binData) {
            buffer.data.resize(buffer.byteLength);
            std::memcpy(buffer.data.data(), binData, buffer.byteLength);
		}
	}

    void parseBufferView(BufferView & bufferView, const json::Json & object) {
		bufferView.name = json::getValue(object, "name", std::string_view(""));
		bufferView.buffer = json::getValue(object, "buffer", kInvalidValue);
		bufferView.length = json::getValue(object, "byteLength", 0u);
		bufferView.stride = json::getValue(object, "byteStride", kInvalidValue);
		bufferView.offset = json::getValue(object, "byteOffset", 0u);

		const auto target = json::getValue<uint16_t>(object, "target", 0u);
		switch (target) {
		case 34962:
			bufferView.target = BufferTarget::eArray_Buffer;
			break;
		case 34963:
			bufferView.target = BufferTarget::eElement_Array_Buffer;
			break;
		default:
			break;
		}
	}

    void parseAccessor(Accessor & accessor, const json::Json & object) {
        accessor.name = json::getValue(object, "name", std::string_view(""));
        accessor.bufferView = json::getValue(object, "bufferView", kInvalidValue);
        accessor.count = json::getValue(object, "count", 0u);
        accessor.offset = json::getValue(object, "byteOffset", 0u);
        accessor.normalized = json::getValue(object, "normalized", false);

		switch (object["componentType"].as<uint16_t>()) {
		case 5120u: accessor.componentType = AccessorComponentType::eByte; break;
		case 5121u: accessor.componentType = AccessorComponentType::eUnsigned_Byte; break;
		case 5122u: accessor.componentType = AccessorComponentType::eShort; break;
		case 5123u: accessor.componentType = AccessorComponentType::eUnsigned_Short; break;
		case 5125u: accessor.componentType = AccessorComponentType::eUnsigned_int; break;
		case 5126u: accessor.componentType = AccessorComponentType::eFloat; break;
		default: break;
		}

        accessor.type = accesorTypes.find(object["type"].as<std::string_view>())->second;
        accessor.min = json::getArray<float>(object, "min");
        accessor.max = json::getArray<float>(object, "max");

        if (object.contains("sparce")) {
            auto const & sparseJson = object["sparse"];
            auto & sparse = accessor.sparse.emplace();
            sparse.count = json::getValue(sparseJson, "count", 0u);

            auto const & indicesJson = sparseJson["indices"];
            sparse.indices.bufferView = json::getValue(indicesJson, "bufferView", kInvalidValue);
            sparse.indices.offset = json::getValue(indicesJson, "byteOffset", 0u);
            const auto componentType = indicesJson["componentType"].as<uint16_t>();
            switch (componentType) {
			case 5121: sparse.indices.componentType = AccessorComponentType::eUnsigned_Byte; break;
			case 5123: sparse.indices.componentType = AccessorComponentType::eUnsigned_Short; break;
			case 5125: sparse.indices.componentType = AccessorComponentType::eUnsigned_int; break;
			default: break;
			}

            const auto & values = sparseJson["values"];
            sparse.values.bufferView = json::getValue(values, "bufferView", kInvalidValue);
            sparse.values.offset = json::getValue(values, "byteOffset", 0u);
        }
    }

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

            // load json from binary data
            std::string_view input(reinterpret_cast<const char*>(&bytes[headerLength]), chunk0_length);
            const auto err = json::read_json(js, input);
        } else {
            // load json
            std::string_view input(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            const auto err = json::read_json(js, input);
        }

        Layout layout;
        const auto version = js["asset"]["version"].as<std::string_view>();
        std::from_chars(version.data(), version.data() + version.size(), layout.asset.version);
        layout.scene = js["scene"].as<uint16_t>();

        parseArray(layout.scenes, parseScene, "scenes",	js);
        parseArray(layout.nodes, parseNode,	"nodes", js);
        parseArray(layout.meshes, parseMesh, "meshes", js);
        parseArray(layout.buffers, parseBuffer, "buffers", js, path, binData);
        parseArray(layout.bufferViews, parseBufferView, "bufferViews", js);
        parseArray(layout.accessors, parseAccessor, "accessors", js);

        return layout;
    }
}