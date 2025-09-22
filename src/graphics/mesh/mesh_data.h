#pragma once

#include "types.h"
#include "math/math.h"

#include <cstdint>
#include <cstddef>

#include <limits>
#include <span>
#include <string>
#include <vector>

namespace engine::graphics::models {

	struct Layout;
	struct Node;

	struct Primitive {
		struct VertexBufferOffsets {
			size_t offset = 0u;
			//uint8_t bufferId = 0u;
		};
		
		size_t ibOffset = 0u;
		std::vector<VertexBufferOffsets> vbOffsets;
		uint32_t firstIndex = 0u;
		uint32_t indexCount = 0u;
		vec3f minCorner{};
		vec3f maxCorner{};
		uint16_t material = kInvalidValue;
		PrimitiveMode primitiveMode = PrimitiveMode::eTriangles;
	};

    struct Mesh_Geometry {
		std::vector<Primitive> primitives;
		uint16_t nodeIndex = kInvalidValue;
        uint16_t skinIndex = kInvalidValue;
	};

    struct Mesh_Skin {
		uint16_t skeletonRoot = kInvalidValue;
		std::vector<mat4f> inverseBindMatrices;
		std::vector<uint16_t> joints;
	};

    struct Mesh_Animation {
		std::string name;
		// maybe move all animation data to special storges in inimation manager for cache locallity?
        std::vector<float> samplerInputs;
		std::vector<vec4f> samplerOutputs;

        struct AnimationSampler {
			std::span<const float> inputs; // animation times
			std::span<const vec4f> outputs; // values
			Interpolation interpolation = Interpolation::eLinear;
		};

        struct AnimationChannel {
			uint16_t sampler = 0u;
			uint16_t target_node = 0u;
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

    struct Mesh_Data {
        std::vector<uint16_t> sceneNodes;
		std::vector<Mesh_Skin> skins;
		std::vector<Mesh_Animation> animations;
		std::vector<Mesh_Geometry> meshes;
        std::vector<Node> nodes;

		std::vector<std::vector<std::byte>> vertexBuffersData;
		std::vector<uint32_t> indexBufferData;
        
		void loadSkins(const Layout & layout);
		void loadAnimations(const Layout & layout);
    	void loadMeshes(const Layout & layout, std::span<const AttributesSemantic> allowedAttributes);
		void loadNodes(const Layout & layout);
	};
}