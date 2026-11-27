#include "gltf_loader.h"

#include "common/hash.h"
#include "core/system.h"
#include "math/math.h"
#include "file/file_manager.h"
#include "utils/base64.h"
#include "utils/json.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstring>
#include <unordered_map>

namespace engine::graphics::models {
    
    namespace extensions {
        constexpr auto kKtxExtension = "KHR_texture_basisu";
        constexpr auto kMeshoptCompressionExtension = "EXT_meshopt_compression";
        constexpr auto kMeshQuantizationExtension = "KHR_mesh_quantization";
    }

    template <typename K, typename T>
    using map_type = stl::unordered_map<K, T>;

    template <typename T, size_t N>
    constexpr const T * findByName(const stl::array<std::pair<std::string_view, T>, N> & table, std::string_view name) noexcept {
        for (size_t i = 0u; i < N; ++i) {
            if (table[i].first == name) {
                return &table[i].second;
            }
        }
        return nullptr;
    }

    template <typename T, size_t N>
    constexpr T getByName(const stl::array<std::pair<std::string_view, T>, N> & table, std::string_view name, T defaultValue) noexcept {
        if (const T * value = findByName(table, name)) {
            return *value;
        }
        return defaultValue;
    }

    constexpr SamplerFilter getFilter(uint16_t f) noexcept {
		switch (f) {
		case 9728: return SamplerFilter::eNearest;
		case 9729: return SamplerFilter::eLinear;
		case 9984: return SamplerFilter::eNearest_Mipmap_Nearest;
		case 9985: return SamplerFilter::eLinear_Mipmap_Nearest;
		case 9986: return SamplerFilter::eNearest_Mipmap_Linear;
		case 9987: return SamplerFilter::eLinear_Mipmap_Linear;
		default: return SamplerFilter::eLinear;
		}
	};

	constexpr SamplerWrap getWrap(uint16_t w) noexcept {
		switch (w) {
		case 33071: return SamplerWrap::eClamp_To_Edge;
		case 33648: return SamplerWrap::eMirrored_Repeat;
		default: return SamplerWrap::eRepeat;
		}
	};

    constexpr auto swap4IfBigEndian = [](uint32_t & val) noexcept {
        if constexpr (isLittleEndian) {
            return;
        }
        // for bigendian
        uint32_t tmp = val;
        unsigned char* dst = reinterpret_cast<unsigned char*>(&val);
        unsigned char* src = reinterpret_cast<unsigned char*>(&tmp);

        dst[0u] = src[3u];
        dst[1u] = src[2u];
        dst[2u] = src[1u];
        dst[3u] = src[0u];
    };

    constexpr stl::array<std::pair<std::string_view, AttributeSemantic>, 14u> semantics = {
        std::pair{"POSITION", AttributeSemantic::ePosition},
        std::pair{"NORMAL", AttributeSemantic::eNormal},
        std::pair{"TANGENT", AttributeSemantic::eTangent},
        std::pair{"COLOR_0", AttributeSemantic::eColor0},
        std::pair{"COLOR_1", AttributeSemantic::eColor1},
        std::pair{"COLOR_2", AttributeSemantic::eColor2},
        std::pair{"COLOR_3", AttributeSemantic::eColor3},
        std::pair{"JOINTS_0", AttributeSemantic::eJoints},
        std::pair{"WEIGHTS_0", AttributeSemantic::eWeights},
        std::pair{"TEXCOORD_0", AttributeSemantic::eUv0},
        std::pair{"TEXCOORD_1", AttributeSemantic::eUv1},
        std::pair{"TEXCOORD_2", AttributeSemantic::eUv2},
        std::pair{"TEXCOORD_3", AttributeSemantic::eUv3},
        std::pair{"TEXCOORD_4", AttributeSemantic::eUv4}
    };

    constexpr stl::array<std::pair<std::string_view, AccessorType>, 7u> accesorTypes = {
        std::pair{"SCALAR", AccessorType::eScalar},
        std::pair{"VEC2", AccessorType::eVec2},
        std::pair{"VEC3", AccessorType::eVec3},
        std::pair{"VEC4", AccessorType::eVec4},
        std::pair{"MAT2", AccessorType::eMat2},
        std::pair{"MAT3", AccessorType::eMat3},
        std::pair{"MAT4", AccessorType::eMat4}
    };

    constexpr stl::array<std::pair<std::string_view, AnimationChannelPath>, 4u> animChannelTypes = {
        std::pair{"translation", AnimationChannelPath::eTranslation},
        std::pair{"rotation", AnimationChannelPath::eRotation},
        std::pair{"scale", AnimationChannelPath::eScale},
        std::pair{"weights", AnimationChannelPath::eWeights}
    };

    constexpr stl::array<std::pair<std::string_view, Interpolation>, 3u> interpolationTypes = {
        std::pair{"LINEAR", Interpolation::eLinear},
        std::pair{"STEP", Interpolation::eStep},
        std::pair{"CUBICSPLINE", Interpolation::eCubicSpline}
    };

    constexpr stl::array<std::pair<std::string_view, MimeType>, 4u> mimeTypes = {
        std::pair{"image/jpeg", MimeType::eJpeg},
        std::pair{"image/png", MimeType::ePng},
        std::pair{"image/ktx2", MimeType::eKtx2},
        std::pair{"undefined", MimeType::eOther}
    };

    constexpr stl::array<std::pair<std::string_view, AlphaMode>, 3u> alphaModes = {
        std::pair{"OPAQUE", AlphaMode::eOpaque},
        std::pair{"BLEND", AlphaMode::eBlend},
        std::pair{"MASK", AlphaMode::eMask}
    };

    constexpr stl::array<std::pair<std::string_view, MeshoptMode>, 4u> meshoptModes = {
        std::pair{"", MeshoptMode::eUndefined},
        std::pair{"ATTRIBUTES", MeshoptMode::eAttributes}, 
        std::pair{"INDICES", MeshoptMode::eIndices}, 
        std::pair{"TRIANGLES", MeshoptMode::eTriangles}
    };

    constexpr stl::array<std::pair<std::string_view, MeshoptFilter>, 4u> meshoptFilters = {
        std::pair{"NONE", MeshoptFilter::eNone},
        std::pair{"OCTAHEDRAL", MeshoptFilter::eOctahedral}, 
        std::pair{"QUATERNION", MeshoptFilter::eQuaternion}, 
        std::pair{"EXPONENTIAL", MeshoptFilter::eExponential}
    };

    template <typename T, typename F, typename... Args>
	void parseArray(stl::vector<T>& arr, F && f, std::string_view name, const json::Json & js, Args &&... args) {
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

        const stl::vector<float>  matrixArray = json::getArray<float>(object, "matrix");
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
            const uint16_t indices = json::getValue(primitive, "indices", kInvalidValue);
            const uint16_t material = json::getValue(primitive, "material", kInvalidValue);
            const uint8_t mode = json::getValue(primitive, "mode", 4u);

            auto & meshPrimitive = mesh.primitives.emplace_back(
                stl::vector<stl::pair<AttributeSemantic, uint16_t>>{},
                indices, 
                material, 
                static_cast<PrimitiveMode>(mode)
            );

            auto const & attributesArray = primitive["attributes"].get_object();
            for (const auto & [key, attribute] : attributesArray) {
                if (const auto * semantic = findByName(semantics, key)) {
                    meshPrimitive.attributes.emplace_back(*semantic, attribute.as<uint16_t>());
                }
            }

            // using Iterator = decltype(meshPrimitive.attributes)::iterator;
            // stl::sort<Iterator>(meshPrimitive.attributes.begin(), meshPrimitive.attributes.end());
            stl::sort(meshPrimitive.attributes.begin(), meshPrimitive.attributes.end());
        }
	}

    void parseBuffer(Buffer & buffer, const json::Json & object, const FileManager & fileManager, std::string_view path, const std::byte * binData) {
        buffer.name = json::getValue(object, "name", std::string_view(""));
		buffer.byteLength = object["byteLength"].as<uint32_t>();

        std::string_view uri = json::getValue(object, "uri", std::string_view(""));

		if (!uri.empty()) {
			constexpr std::string_view octetStreamHeader = "data:application/octet-stream;base64,";
			constexpr std::string_view bufferHeader = "data:application/gltf-buffer;base64,";

            if (uri.contains(octetStreamHeader)) {
                const std::string data = base64_decode(std::string{ uri.substr(octetStreamHeader.size()) });
                buffer.data.resize(buffer.byteLength);
                std::memcpy(buffer.data.data(), data.data(), buffer.byteLength);
            } else if (uri.contains(bufferHeader)) {
                const std::string data = base64_decode(std::string{ uri.substr(bufferHeader.size()) });
                buffer.data.resize(buffer.byteLength);
                std::memmove(buffer.data.data(), data.data(), buffer.byteLength);
            } else {
                buffer.data = fileManager.readFile(std::string{path}.append("/", 1u).append(uri.data(), uri.size()));
            }
		} else if (binData) {
            buffer.data.resize(buffer.byteLength);
            std::memcpy(buffer.data.data(), binData, buffer.byteLength);
		}
	}

    void parseBufferView(BufferView & bufferView, const json::Json & object) {

        bufferView.name = json::getValue(object, "name", std::string_view(""));

        if (auto * ext = json::findChild(object, "extensions")) {
            if (auto * mExt = json::findChild(*ext, extensions::kMeshoptCompressionExtension)) {
                bufferView.buffer = json::getValue(*mExt, "buffer", kInvalidValue);
                bufferView.length = json::getValue(*mExt, "byteLength", 0u);
                bufferView.stride = json::getValue(*mExt, "byteStride", kInvalidValue);
                bufferView.offset = json::getValue(*mExt, "byteOffset", 0u);

                const auto target = json::getValue<uint16_t>(*mExt, "target", 0u);
                switch (target) {
                case 34962: bufferView.target = BufferTarget::eArray_Buffer; break;
                case 34963: bufferView.target = BufferTarget::eElement_Array_Buffer; break;
                default: break;
                }

                const auto modeName = json::getValue(*mExt, "mode", std::string_view(""));
                bufferView.mode = getByName(meshoptModes, modeName, MeshoptMode::eUndefined);

                const auto filterName = json::getValue(*mExt, "filter", std::string_view("NONE"));
                bufferView.filter = getByName(meshoptFilters, filterName, MeshoptFilter::eNone);
            }
        } else {
            bufferView.buffer = json::getValue(object, "buffer", kInvalidValue);
            bufferView.length = json::getValue(object, "byteLength", 0u);
            bufferView.stride = json::getValue(object, "byteStride", kInvalidValue);
            bufferView.offset = json::getValue(object, "byteOffset", 0u);

            const auto target = json::getValue<uint16_t>(object, "target", 0u);
            switch (target) {
            case 34962: bufferView.target = BufferTarget::eArray_Buffer; break;
            case 34963: bufferView.target = BufferTarget::eElement_Array_Buffer; break;
            default: break;
            }
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

        accessor.type = getByName(accesorTypes, object["type"].as<std::string_view>(), AccessorType::eScalar);
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

    void parseAnimation(Animation & animation, const json::Json & object) {
		animation.name = json::getValue(object, "name", std::string_view(""));

        // channels
        auto const & channelsArray = object["channels"].get_array();
        animation.channels.reserve(channelsArray.size());
        for (const auto & channel : channelsArray) {
            auto const & target = channel["target"];
            animation.channels.emplace_back(
                json::getValue(channel, "sampler", 0u),
                json::getValue(target, "node", kInvalidValue),
                getByName(animChannelTypes, target["path"].as<std::string_view>(), AnimationChannelPath::eTranslation)
            );
        }

        // samplers
        auto const & samplersArray = object["samplers"].get_array();
        animation.samplers.reserve(samplersArray.size());
        for (const auto & sampler : samplersArray) {
            animation.samplers.emplace_back(
                json::getValue(sampler, "input", kInvalidValue),
                json::getValue(sampler, "output", kInvalidValue),
                getByName(interpolationTypes, json::getValue(sampler, "interpolation", std::string_view("LINEAR")), Interpolation::eLinear)
            );
        }
	}

    void parseSkin(Skin & skin, const json::Json & object) {
        skin.name = json::getValue(object, "name", std::string_view(""));
        skin.inverseBindMatrices = json::getValue(object, "inverseBindMatrices", kInvalidValue);
		skin.skeleton = json::getValue(object, "skeleton", kInvalidValue);
		skin.joints = json::getArray<uint16_t>(object, "joints");
	}

	void parseSampler(Sampler & sampler, const json::Json & object) {
		sampler.magFilter = getFilter(json::getValue(object, "magFilter", kInvalidValue));
		sampler.minFilter = getFilter(json::getValue(object, "minFilter", kInvalidValue));
        constexpr uint16_t kDefaultWrap = 10497u;
		sampler.wrapS = getWrap(json::getValue(object, "wrapS", kDefaultWrap));
		sampler.wrapT = getWrap(json::getValue(object, "wrapT", kDefaultWrap));
	}

    void parseTexture(Texture & texture, const json::Json & object) {
        if (auto * ext = json::findChild(object, "extensions")) {
            if (auto * ktxExt = json::findChild(*ext, extensions::kKtxExtension)) {
                texture.source = json::getValue(*ktxExt, "source", kInvalidValue);
            }
        }

        texture.name = json::getValue(object, "name", std::string_view(""));
		texture.sampler = json::getValue(object, "sampler", kInvalidValue);
        
        if (texture.source == kInvalidValue) {
		    texture.source = json::getValue(object, "source", kInvalidValue);
        }
	}

    void parseTextureInfo(TextureInfo & info, const json::Json & object) {
		info.index = json::getValue(object, "index", kInvalidValue);
		info.texCoord = json::getValue(object, "texCoord", uint16_t(0u));
		info.scale = json::getValue(object, "scale", 1.0f);
		info.strength = json::getValue(object, "strength", 1.0f);
	}

    void parseImage(Image & image, const json::Json & object, std::string_view path) {
		image.name = json::getValue(object, "name", std::string_view(""));
        if (object.contains("uri")) {
            image.uri = std::string(path) + "/" + object["uri"].as<std::string>();
        }
		
        image.mimeType = getByName(mimeTypes, json::getValue(object, "mimeType", std::string_view("undefined")), MimeType::eOther);
		image.bufferView = json::getValue(object, "bufferView", kInvalidValue);
	}

    void parseMaterial(Material & material, const json::Json & object) {
		material.name = json::getValue(object, "name", std::string_view(""));
		material.doubleSided = json::getValue(object, "doubleSided", false);
		material.alphaCutoff = json::getValue(object, "alphaCutoff", 0.5f);
		material.alphaMode = getByName(alphaModes, json::getValue(object, "alphaMode", std::string_view("OPAQUE")), AlphaMode::eOpaque);

        if (const auto * pbrMetallicData = json::findChild(object, "pbrMetallicRoughness")) {
            if (const auto * baseColorData = json::findChild(*pbrMetallicData, "baseColorFactor")) {
                auto const & array = baseColorData->get_array();
                material.pbrMetallicRoughness.baseColorFactor = {
                    array[0].as<float>(),
                    array[1].as<float>(),
                    array[2].as<float>(),
                    array[3].as<float>()
                };
            }

            material.pbrMetallicRoughness.metallicFactor = json::getValue(*pbrMetallicData, "metallicFactor", 1.0f);
            material.pbrMetallicRoughness.roughnessFactor = json::getValue(*pbrMetallicData, "roughnessFactor", 1.0f);

            if (const auto * texture = json::findChild(*pbrMetallicData, "baseColorTexture")) {
                parseTextureInfo(material.pbrMetallicRoughness.baseColorTexture, *texture);
            }
            if (const auto * texture = json::findChild(*pbrMetallicData, "metallicRoughnessTexture")) {
                parseTextureInfo(material.pbrMetallicRoughness.metallicRoughnessTexture, *texture);
            }
        }

        if (const auto * texture = json::findChild(object, "normalTexture")) {
			parseTextureInfo(material.normalTexture, *texture);
		}
        if (const auto * texture = json::findChild(object, "occlusionTexture")) {
			parseTextureInfo(material.occlusionTexture, *texture);
		}
        if (const auto * texture = json::findChild(object, "emissiveTexture")) {
			parseTextureInfo(material.emissiveTexture, *texture);
		}

        if (const auto * emissiveFactorData = json::findChild(object, "emissiveFactor")) {
            auto const & array = emissiveFactorData->get_array();
            material.emissiveFactor = {
                array[0].as<float>(),
                array[1].as<float>(),
                array[2].as<float>()
            };
        }
	}

    Layout loadModelLayout(stl::span<std::byte const> bytes, const FileManager & fileManager, std::string_view path) {
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
        parseArray(layout.buffers, parseBuffer, "buffers", js, fileManager, path, binData);
        parseArray(layout.bufferViews, parseBufferView, "bufferViews", js);
        parseArray(layout.accessors, parseAccessor, "accessors", js);
        parseArray(layout.animations, parseAnimation, "animations", js);
        parseArray(layout.skins, parseSkin, "skins", js);
		parseArray(layout.samplers, parseSampler, "samplers", js);
        parseArray(layout.textures, parseTexture, "textures", js);
        parseArray(layout.images, parseImage, "images", js, path);
        parseArray(layout.materials, parseMaterial, "materials", js);

        return layout;
    }
}