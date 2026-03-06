#pragma once

#include "types.h"
#include "math/math.h"
#include "graphics/render/rhi/common/types.h"
#include "graphics/texture/image_frame.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace engine {
    class FileManager;

namespace graphics {
	class TextureManager;

namespace models {

	struct Layout;
	struct Node;

	struct Primitive {
		uint32_t firstIndex = 0u;
		uint32_t indexCount = 0u;
		uint32_t minIndex = 0xff'ff'ff'ffu;
		uint32_t maxIndex = 0u;
		vec3f minCorner{};
		vec3f maxCorner{};
		uint16_t material = kInvalidValue;
		PrimitiveMode primitiveMode = PrimitiveMode::eTriangles;
	};

    struct MeshGeometry {
		std::vector<Primitive> primitives;
		// uint16_t nodeIndex = kInvalidValue;
        // uint16_t skinIndex = kInvalidValue;
	};

    struct MeshSkin {
		uint16_t skeletonRoot = kInvalidValue;
		std::vector<mat4f> inverseBindMatrices;
		std::vector<uint16_t> joints;
	};

    struct MeshAnimation {
		std::string name;
		// maybe move all animation data to special storges in animation manager for cache locallity?
        std::vector<float> samplerInputs;
		std::vector<vec4f> samplerOutputs;

        struct AnimationSampler {
			std::span<const float> inputs; // animation times
			std::span<const vec4f> outputs; // values
			Interpolation interpolation = Interpolation::eLinear;
		};

        struct AnimationChannel {
			uint16_t sampler = kInvalidValue;
			uint16_t target_node = kInvalidValue;
			AnimationChannelPath path = AnimationChannelPath::eTranslation;
		};

		std::vector<AnimationChannel> channels;
		std::vector<AnimationSampler> samplers;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::lowest();
		float duration = 0.0f;
		uint16_t minTargetNodeId = kInvalidValue;
		uint16_t maxTargetNodeId = 0u;
    };

	// enum class TextureId : uint8_t {
	// 	eBaseColor = 0u,
	// 	eNormal = 1u,
	// 	eMetallicRoughness = 2u,
	// 	eOccolusion = 3u,
	// 	eEmissive = 4u
	// };

	// struct MeshMaterial {
	// 	std::array<ImageFrame2D, 5u> textures;
	// 	vec4f baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
	// 	vec3f emissiveFactor = {0.0f, 0.0f, 0.0f};
	// 	float metallicFactor = 1.0f;
	// 	float roughnessFactor = 1.0f;	
	// 	float alphaCutoff = 0.5f;
	// 	rhi::BlendMode blendMode = rhi::common_blend_modes::blend_none;
	// 	rhi::CullMode cullMode = rhi::CullMode::eBack;
	// };

	struct MeshBuffersData {
		std::vector<std::vector<std::byte>> vertexBuffersData;
		std::vector<uint32_t> indexBufferData;
	};

    struct MeshData {
		MeshData() = default;
		
		MeshData(const MeshData &) = delete;
		MeshData(MeshData &&) noexcept = default;

		MeshData & operator= (const MeshData &&) = delete;
		MeshData & operator= (MeshData &&) noexcept = default;

        std::vector<uint16_t> sceneNodes;
		std::vector<MeshSkin> skins;
		std::vector<MeshAnimation> animations;
		std::vector<MeshGeometry> meshes;
        std::vector<Node> nodes;
		// std::vector<MeshMaterial> materials;
		std::vector<uint16_t> primitiveBegins;

		uint32_t skinMatricesCount = 0u;

		void loadSkins(const Layout & layout);
		void loadAnimations(const Layout & layout);
    	MeshBuffersData loadMeshes(const Layout & layout, std::span<const AttributeSemantic> allowedAttributes);
		void loadNodes(const Layout & layout);
		// void loadMaterials(const Layout & layout, const FileManager & fileManager, TextureManager & textureManager, std::string_view name);

		void optimizeBuffers(std::span<std::vector<std::byte>> vertexBuffersData, std::vector<uint32_t> & indexBufferData, std::span<const AttributeSemantic> allowedAttributes);
	};
}

}
}