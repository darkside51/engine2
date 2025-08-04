#include "mesh_data.h"
#include "gltf_loader.h"

namespace engine::graphics::models {
    
    void Mesh_Data::loadMeshes(const Layout & layout, std::span<const AttributesSemantic> allowedAttributes) {
        meshes.reserve(layout.meshes.size());

        for (const auto & srcMesh : layout.meshes) {

            for (const auto & srcPrimitive : srcMesh.primitives) {

            }
            
        }
    }

	void Mesh_Data::loadNodes(const Layout & layout) {
		sceneNodes = layout.scenes[layout.scene].nodes;
		nodes = layout.nodes; // copy data

        auto loadMeshNode = [&](const auto & self, uint16_t nodeId) -> void {
            const auto & node = nodes[nodeId];

            if (node.mesh != kInvalidValue) {
                meshes[node.mesh].nodeIndex = nodeId;
                meshes[node.mesh].skinIndex = node.skin;
            }

            for (uint16_t cNodeId : node.children) {
			    self(self, cNodeId);
		    }
        };

		for (uint16_t nodeId : sceneNodes) {
			loadMeshNode(loadMeshNode, nodeId);
		}
	}

    void Mesh_Data::loadSkins(const Layout & layout) {
        skins.reserve(layout.skins.size());

        for (const auto & srcSkin : layout.skins) {
            auto & skin = skins.emplace_back(srcSkin.skeleton);
            skin.joints = srcSkin.joints; // copy

            if (srcSkin.inverseBindMatrices != kInvalidValue) {
                const auto & accessor = layout.accessors[srcSkin.inverseBindMatrices];
				const auto & bufferView = layout.bufferViews[accessor.bufferView];
				const auto & buffer = layout.buffers[bufferView.buffer];
				skin.inverseBindMatrices.resize(accessor.count);
                memcpy(&skin.inverseBindMatrices[0u], &buffer.data[accessor.offset + bufferView.offset], accessor.count * sizeof(mat4f));
            }
        }
    }

    void Mesh_Data::loadAnimations(const Layout & layout) {
        animations.reserve(layout.animations.size());

        size_t samplerInputsCount = 0u;
        size_t samplerOutputsCount = 0u;

        for (const auto & srcAnimation : layout.animations) {
            auto & anim = animations.emplace_back(srcAnimation.name);
            anim.samplers.reserve(srcAnimation.samplers.size());

            for (const auto & sampler : srcAnimation.samplers) {
                {
                    const auto & accessor = layout.accessors[sampler.input];
                    samplerInputsCount += accessor.count;
                }
                {
                    const auto & accessor = layout.accessors[sampler.output];
                    samplerOutputsCount += accessor.count;
                }
            }

            anim.samplerInputs.resize(samplerInputsCount);
            anim.samplerOutputs.resize(samplerOutputsCount);

            anim.channels.reserve(srcAnimation.channels.size());
        }

        size_t i = 0u;
        size_t samplerInputsOffset = 0u;
        size_t samplerOutputsOffset = 0u;
        for (const auto & srcAnimation : layout.animations) {
            auto & anim = animations[i];

            // samplers
            for (const auto & srcSampler : srcAnimation.samplers) {
                auto & sampler = anim.samplers.emplace_back();
                sampler.interpolation = srcSampler.interpolation;
                // read keyframes input times
                {
                    const auto & accessor = layout.accessors[srcSampler.input];
                    const auto & bufferView = layout.bufferViews[accessor.bufferView];
                    const auto & buffer = layout.buffers[bufferView.buffer];
                    const void * data = &buffer.data[accessor.offset + bufferView.offset];

                    memcpy(&anim.samplerInputs[samplerInputsOffset], data, accessor.count * sizeof(float));
                    sampler.inputs = {anim.samplerInputs.begin() + samplerInputsOffset, accessor.count};
                    samplerInputsOffset += accessor.count;

                    // adjust animation's start and end times
					for (auto input : sampler.inputs) {
						if (input < anim.start) {
							anim.start = input;
						}

						if (input > anim.end) {
							anim.end = input;
						}
					}
                }

                // read sampler keyframe output translate/rotate/scale values
				{
					const auto & accessor = layout.accessors[srcSampler.output];
					const auto & bufferView = layout.bufferViews[accessor.bufferView];
					const auto & buffer = layout.buffers[bufferView.buffer];
					const void * data = &buffer.data[accessor.offset + bufferView.offset];

					switch (accessor.type) {
						case AccessorType::eVec3: {
							const vec3 * buf = static_cast<const vec3*>(data);
							for (size_t index = 0u; index < accessor.count; ++index) {
                                anim.samplerOutputs[samplerOutputsOffset + index] = vec4f{buf[index].x, buf[index].y, buf[index].z, 0.0f};
							}
							break;
						}
						case AccessorType::eVec4: {
                            memcpy(&anim.samplerOutputs[samplerOutputsOffset], data, accessor.count * sizeof(vec4f));
							break;
						}
						default:
							break;
					}

                    sampler.outputs = {anim.samplerOutputs.begin() + samplerOutputsOffset, accessor.count};
                    samplerOutputsOffset += accessor.count;
				}
            }

            anim.duration = anim.end - anim.start;

            // channels
			anim.minTargetNodeId = kInvalidValue;
			anim.maxTargetNodeId = 0u;
			for (const auto & srcChannel : srcAnimation.channels) {
		        auto & channel = anim.channels.emplace_back(srcChannel.sampler, srcChannel.target_node, srcChannel.path);
				if (channel.target_node < anim.minTargetNodeId) { anim.minTargetNodeId = channel.target_node; }
				if (channel.target_node > anim.maxTargetNodeId) { anim.maxTargetNodeId = channel.target_node; }
            }
        }
	}

}