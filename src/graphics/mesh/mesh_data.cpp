#include "mesh_data.h"
#include "gltf_loader.h"

namespace engine::graphics::models {
    
    void Mesh_Data::loadMeshes(const Layout & layout, std::span<const AttributesSemantic> allowedAttributes) {
        meshes.reserve(layout.meshes.size());
        vertexBuffersData.resize(allowedAttributes.size());

        for (const auto & srcMesh : layout.meshes) {
            auto & mesh = meshes.emplace_back();
            mesh.primitives.reserve(srcMesh.primitives.size());
            //const auto firstIndex = static_cast<uint32_t>(indexBufferData.size());

            for (const auto & srcPrimitive : srcMesh.primitives) {
                auto & primitive = mesh.primitives.emplace_back();
                primitive.material = srcPrimitive.material;
                primitive.primitiveMode = srcPrimitive.mode;
                primitive.vbOffsets.resize(allowedAttributes.size());

                { // indices
                    const Accessor & accessor = layout.accessors[srcPrimitive.indices];
                    const BufferView & view = layout.bufferViews[accessor.bufferView];
                    const Buffer& buffer = layout.buffers[view.buffer];

                    primitive.indexCount = accessor.count;
                    primitive.firstIndex = static_cast<uint32_t>(indexBufferData.size());
                    indexBufferData.resize(primitive.firstIndex + primitive.indexCount);

                    switch (accessor.componentType) {
						case AccessorComponentType::eUnsigned_int:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								const size_t offset = accessor.offset + view.offset + index * sizeof(uint32_t);
								const auto v1 = static_cast<uint8_t>(buffer.data[offset + 0]);
								const auto v2 = static_cast<uint8_t>(buffer.data[offset + 1]);
								const auto v3 = static_cast<uint8_t>(buffer.data[offset + 2]);
								const auto v4 = static_cast<uint8_t>(buffer.data[offset + 3]);
								const auto v = static_cast<uint32_t>((static_cast<uint32_t>(v1) << 0) | 
									(static_cast<uint32_t>(v2) << 8) | (static_cast<uint32_t>(v3) << 16) | 
									(static_cast<uint32_t>(v4) << 24));
								indexBufferData[primitive.firstIndex + index] = v;
							}
								break;
						case AccessorComponentType::eUnsigned_Short:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								const size_t offset = accessor.offset + view.offset + index * sizeof(uint16_t);
								const auto v1 = static_cast<uint8_t>(buffer.data[offset + 0]);
								const auto v2 = static_cast<uint8_t>(buffer.data[offset + 1]);
								const auto v = static_cast<uint16_t>((static_cast<uint16_t>(v1) << 0) | 
									(static_cast<uint16_t>(v2) << 8));
								indexBufferData[primitive.firstIndex + index] = v;
							}
								break;
						case AccessorComponentType::eUnsigned_Byte:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								indexBufferData[primitive.firstIndex + index] = static_cast<uint32_t>(buffer.data[accessor.offset + view.offset + index]);
							}
							break;
						default:
							break;
					}
                }

                std::array<std::pair<size_t, uint32_t>, std::to_underlying(AttributesSemantic::eSemanticsCount)> counts{}; 

                uint32_t attributeId = 0u;
                for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
                    size_t i = 0u;
                    for (auto attributeSemantic : allowedAttributes) {
                        if ((attributeSemantic & semantic) == semantic) {
                            break;
                        }
                        ++i;
                    }

                    if (i < allowedAttributes.size()) {
                        const Accessor & accessor = layout.accessors[accessorId];
                        // counts[i].first += accessor.getElementSize() * accessor.count; // need to get element size from in params (use predefined size for attributes (12 bytes for position, 4 bytes for color...))
                        // need to convert data after
                        counts[i].first += kDefaultSemanticsSizeBytes[std::log2(std::to_underlying(semantic))] * accessor.count;
                        counts[i].second |= 1u << attributeId;

                        // auto & vbOffset = primitive.vbOffsets[i];
                        // auto & vbData = vertexBuffersData[i];
                        // vbOffset.offset = vbData.size();

                        // const Accessor & accessor = layout.accessors[accessorId];
						// const BufferView & view = layout.bufferViews[accessor.bufferView];

                        // const auto count = accessor.count;
                        // const auto componentTtype = accessor.componentType;
                        // const auto type = accessor.type;

                        // vbData.resize(vbData.size() + accessor.count * sizeof(float) * 3);
                    }
                    ++attributeId;
                }

                for (size_t j = 0u; j < allowedAttributes.size(); ++j) {
                    auto & vbData = vertexBuffersData[j];
                    auto & vbOffset = primitive.vbOffsets[j];
                    vbOffset.offset = vbData.size();
                    vbData.resize(vbData.size() + counts[j].first);

                    uint32_t attributeId = 0u;
                    for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
                        if (counts[j].second & (1u << attributeId)) {
                            const Accessor & accessor = layout.accessors[accessorId];
                            const BufferView & view = layout.bufferViews[accessor.bufferView];

                            const auto dataSize = accessor.getElementSize() * accessor.count; // 

                            // copy per elements data
                        }
                        ++attributeId;
                    }
                }
            }

            int a = 0;
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