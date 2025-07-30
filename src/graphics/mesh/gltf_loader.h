#pragma once

// gltf 2.0 format specification
// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.pdf

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace engine::graphics::model {
    
    // constants:
    inline constexpr uint16_t kInvalidValue = 0xff'ffu;

    struct vec3 {
        union {
            struct {
                float x, y, z;
            };
            std::array<float, 3> data;
        };

        vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        vec3(float a, float b, float c) : x(a), y(b), z(c) {}

        inline float operator[](uint8_t i) const noexcept { return data[i]; }
        inline float& operator[](uint8_t i) noexcept { return data[i]; }
    };

    struct vec4 {
        union {
            struct {
                float x, y, z, w;
            };
            std::array<float, 4> data;
        };

        vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
        vec4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {}

        inline float operator[](uint8_t i) const noexcept { return data[i]; }
        inline float& operator[](uint8_t i) noexcept { return data[i]; }
    };

    struct mat4 {
        std::array<vec4, 4> data;

        mat4() = default;
        mat4(float f) : 
            data {
                vec4(f, 0.0f, 0.0f, 0.0f),
                vec4(0.0f, f, 0.0f, 0.0f),
                vec4(0.0f, 0.0f, f, 0.0f),
                vec4(0.0f, 0.0f, 0.0f, f)
            } {}

        inline vec4 operator[](uint8_t i) const noexcept { return data[i]; }
        inline vec4& operator[](uint8_t i) noexcept { return data[i]; }
    };

    // enums:
    enum class AccessorComponentType : uint8_t {
		eByte = 0u,
		eUnsigned_Byte = 1u,
		eShort = 2u,
		eUnsigned_Short = 3u,
		eUnsigned_int = 4u,
		eFloat = 5u
	};

    enum class AccessorType : uint8_t {
		eScalar = 0u,
		eVec2 = 1u,
		eVec3 = 2u,
		eVec4 = 3u,
		eMat2 = 4u,
		eMat3 = 5u,
		eMat4 = 6u
	};

    enum class AttributesSemantic : uint8_t {
        ePosition = 0u,
        eNormal = 1u,
        eTangent = 2u,
        eColor0 = 3u,
        eColor1 = 4u,
        eColor2 = 5u,
        eColor3 = 6u,
        eJoints = 7u,
        eWeight = 8u,
        eUv0 = 9u,
        eUv1 = 10u,
        eUv2 = 11u,
        eUv3 = 12u,
        eUv4 = 13u,
        eSemanticsCount = 14u
    };

    enum class BufferTarget : uint8_t {
		eArray_Buffer = 0u,
		eElement_Array_Buffer = 1u,
		eAny_Data = 3u
	};

    enum class PrimitiveMode : uint8_t {
        ePoints = 0u,
        eLines = 1u,
        eLine_Loop = 2u,
        eLine_Strip = 3u,
        eTriangles = 4u,
        eTriangle_Strip = 5u,
        eTriangle_Fan = 6u
    };

    enum class AnimationChannelPath : uint8_t {
        eTranslation = 0u,
        eRotation = 1u,
        eScale = 2u,
        eWeights = 3u
    };

    enum class Interpolation : uint8_t { 
        eLinear = 0u,
        eStep = 1u,
        eCubicSpline = 2u
    };

    enum class MimeType : uint8_t {
		eJpeg = 0u,
		ePng = 1u,
        eOther = 2u
	};

	enum class AlphaMode : uint8_t {
		eOpaque = 0u,
		eMask = 1u,
		eBlend = 2u
	};

	enum class SamplerFilter : uint8_t {
		eNearest = 0u,
		eLinear = 1u,
		eNearest_Mipmap_Nearest = 2u,
		eLinear_Mipmap_Nearest = 3u,
		eNearest_Mipmap_Linear = 4u,
		eLinear_Mipmap_Linear = 5u
	};

    enum class SamplerWrap : uint8_t {
		eClamp_To_Edge = 0u,
		eMirrored_Repeat = 1u,
		eRepeat = 2u
	};

    // structs:
    struct Scene {
		std::string name;
		std::vector<uint16_t> nodes;
	};

    struct Node {
		std::string name;
		uint16_t mesh = kInvalidValue;
		uint16_t skin = kInvalidValue;
		model::vec3 scale = {1.0f, 1.0f, 1.0f};
		model::vec3 translation = {0.0f, 0.0f, 0.0f};
		model::vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
		std::vector<float> weights;
		std::vector<uint16_t> children;
	};

    struct Primitives {
		std::map<AttributesSemantic, uint16_t> attributes;
		uint16_t indices = kInvalidValue;
		uint16_t material = kInvalidValue;
		PrimitiveMode mode = PrimitiveMode::eTriangles;
	};

    struct Mesh {
		std::string name;
		std::vector<Primitives> primitives;
		std::vector<float> weights;
	};

    struct Buffer {
		std::string name;
		std::vector<std::byte> data;
		uint32_t byteLength = 0u;

		Buffer() = default;
		Buffer(Buffer && b) noexcept = default;
        Buffer& operator= (Buffer && b) noexcept = default;

		Buffer(const Buffer&) = delete;
        Buffer& operator= (const Buffer&) = delete;
	};

    struct BufferView {
		std::string name;
		uint16_t buffer = kInvalidValue;
		uint16_t stride = kInvalidValue;
		uint32_t offset = 0u;
		uint32_t length = 0u;
		BufferTarget target = BufferTarget::eAny_Data;
	};

    struct AccessorSparse {
		uint32_t count = 0u;
		struct {
			uint16_t bufferView = kInvalidValue;
			uint32_t offset = 0u;
			AccessorComponentType componentType; // may be eUnsigned_Byte or eShort or eUnsigned_int
		} indices;

		struct {
			uint16_t bufferView = kInvalidValue;
			uint32_t offset = 0u;
		} values;
	};

    struct Accessor {
		std::string name;
		uint16_t bufferView = 0xffffu;
		uint32_t count = 0u;
		uint32_t offset = 0u;
		AccessorComponentType componentType;
		bool normalized = false;
		AccessorType type;
		std::vector<float> min; // [1 - 16] it can be 1, 2, 3, 4, 9, or 16
		std::vector<float> max; // [1 - 16] it can be 1, 2, 3, 4, 9, or 16
		std::optional<AccessorSparse> sparse = std::nullopt; // no required

		~Accessor() = default;

		Accessor() = default;
		Accessor(Accessor&& a) noexcept :
			name(std::move(a.name)), sparse(std::move(a.sparse)), bufferView(a.bufferView),
			count(a.count), offset(a.offset), componentType(a.componentType),
			normalized(a.normalized), type(a.type), min(a.min), max(a.max) {

		}

        Accessor& operator= (Accessor&& a) noexcept = default;
		Accessor(const Accessor&) = delete;
        Accessor& operator= (const Accessor&) = delete;
	};

    struct Asset {
		float version = 0.0f;
	};

	struct AnimationSampler {
		uint16_t input = 0u;
		uint16_t output = 0u;
		Interpolation interpolation = Interpolation::eLinear;
	};

	struct AnimationChannel {
		uint16_t sampler = 0u;
		uint16_t target_node = 0u;
		AnimationChannelPath path = AnimationChannelPath::eTranslation;
	};

    struct Animation {
		std::string name;
		std::vector<AnimationChannel> channels;
		std::vector<AnimationSampler> samplers;
	};

    struct Image {
		std::string name;
		std::string uri;
		MimeType mimeType = MimeType::eOther;
		uint16_t bufferView = kInvalidValue;
	};

    struct TextureInfo {
		uint16_t index = kInvalidValue;
		uint16_t texCoord = kInvalidValue;
		float scale = 1.0f; // for normalTexture
		float strength = 1.0f; // for occolusionTexture
	};

    struct Material {
		std::string name;
		struct {
			model::vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
			TextureInfo baseColorTexture;
			TextureInfo metallicRoughnessTexture;
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
		} pbrMetallicRoughness;

		TextureInfo normalTexture;
		TextureInfo occlusionTexture;
		TextureInfo emissiveTexture;
		model::vec3 emissiveFactor = {0.0f, 0.0f, 0.0f};
		AlphaMode alphaMode = AlphaMode::eOpaque;
		float alphaCutoff = 0.5f;
		bool doubleSided = false;
	};

    struct Sampler {
		SamplerFilter magFilter = SamplerFilter::eLinear;
		SamplerFilter minFilter = SamplerFilter::eLinear;
		SamplerWrap wrapS = SamplerWrap::eRepeat;
		SamplerWrap wrapT = SamplerWrap::eRepeat;
	};

    struct Skin {
		std::string name;
		uint16_t inverseBindMatrices = kInvalidValue; // The index of the accessor containing the floating-point 4x4 inverse-bind matrices.
		uint16_t skeleton = kInvalidValue; // The index of the node used as a skeleton root.
		std::vector<uint16_t> joints; // Indices of skeleton nodes, used as joints in this skin.
	};

    struct Texture {
		std::string name;
		uint16_t sampler = kInvalidValue;
		uint16_t source = kInvalidValue;
	};

    struct Layout {
		std::vector<Accessor> accessors;
		std::vector<Animation> animations;
		std::vector<Buffer> buffers;
		std::vector<BufferView> bufferViews;
		std::vector<Image> images;
		std::vector<Material> materials;
		std::vector<Mesh> meshes;
		std::vector<Node> nodes;
		std::vector<Sampler> samplers;
		std::vector<Scene> scenes;
		std::vector<Skin> skins;
		std::vector<Texture> textures;
		uint16_t scene = kInvalidValue;
		Asset asset;

		Layout() = default;
		Layout(Layout &&) noexcept = default;
        Layout& operator= (Layout && d) noexcept = default;

		Layout(const Layout&) = delete;
        Layout& operator= (const Layout&) = delete;
	};

    Layout loadModelLayout(std::span<std::byte const> bytes, std::string_view path = {});
}