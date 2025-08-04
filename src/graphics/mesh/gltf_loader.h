#pragma once

// gltf 2.0 format specification
// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.pdf

#include "types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::graphics::models {
    
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

    // structs:
    struct Scene {
		std::string name;
		std::vector<uint16_t> nodes;
	};

    struct Node {
		std::string name;
		uint16_t mesh = kInvalidValue;
		uint16_t skin = kInvalidValue;
		vec3 scale = {1.0f, 1.0f, 1.0f};
		vec3 translation = {0.0f, 0.0f, 0.0f};
		vec4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
		std::vector<float> weights;
		std::vector<uint16_t> children;
	};

    struct Primitives {
		// std::map<AttributesSemantic, uint16_t> attributes;
		std::vector<std::pair<AttributesSemantic, uint16_t>> attributes;
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
			vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
			TextureInfo baseColorTexture;
			TextureInfo metallicRoughnessTexture;
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
		} pbrMetallicRoughness;

		TextureInfo normalTexture;
		TextureInfo occlusionTexture;
		TextureInfo emissiveTexture;
		vec3 emissiveFactor = {0.0f, 0.0f, 0.0f};
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