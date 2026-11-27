#include "mesh_data.h"
#include "gltf_loader.h"
#include "decoded_buffer_views.h"
#include "graphics/texture/texture_data_loader.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/utils/vertex_processing.h"
#include "file/file_manager.h"
#include "stl/array.h"

#include "meshoptimizer.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace engine::graphics::models {
    
    constexpr static auto kDefaultSemanticsSizeBytes = stl::array{
        12u, // ePosition: vec3 (3 floats of 4 bytes)
        12u, // eNormal
        12u, // eTangent
        4u, // eColor0: as uint32_t
        4u, // eColor1
        4u, // eColor2
        4u, // eColor3
        8u, // eJoints: 4 uint16_t
        16u, // eWeights: 4 floats
        8u, // eUv0: 2 floats
        8u, // eUv1
        8u, // eUv2
        8u, // eUv3
        8u, // eUv4
    };

    constexpr static auto kOptimalSemanticsSizeBytes = stl::array {
        8u, // ePosition: R16G16B16A16
        4u, // eNormal: R8G8B8A8
        4u, // eTangent: R8G8B8A8
        4u, // eColor0: R8G8B8A8
        4u, // eColor1
        4u, // eColor2
        4u, // eColor3
        4u, // eJoints: R8G8B8A8
        8u, // eWeights: R16G16B16A16
        4u, // eUv0: R16G16
        4u, // eUv1
        4u, // eUv2
        4u, // eUv3
        4u, // eUv4
    };

    constexpr static auto kOptimalComponentCount = stl::array {
        4u, // ePosition: R16G16B16A16
        4u, // eNormal: R8G8B8A8
        4u, // eTangent: R8G8B8A8
        4u, // eColor0: R8G8B8A8
        4u, // eColor1
        4u, // eColor2
        4u, // eColor3
        4u, // eJoints: R8G8B8A8
        4u, // eWeights: R16G16B16A16
        2u, // eUv0: R16G16
        2u, // eUv1
        2u, // eUv2
        2u, // eUv3
        2u, // eUv4
    };

    namespace {
       inline float readAccessorComponent(const std::byte * data, AccessorComponentType type, bool normalized) noexcept {
           switch (type) {
           case AccessorComponentType::eByte: {
               const auto value = static_cast<int8_t>(*data);
               return normalized ? std::max(static_cast<float>(value) / 127.0f, -1.0f) : static_cast<float>(value);
           }
           case AccessorComponentType::eUnsigned_Byte: {
               const auto value = static_cast<uint8_t>(*data);
               return normalized ? static_cast<float>(value) / 255.0f : static_cast<float>(value);
           }
           case AccessorComponentType::eShort: {
               int16_t value = 0;
               std::memcpy(&value, data, sizeof(value));
               return normalized ? std::max(static_cast<float>(value) / 32767.0f, -1.0f) : static_cast<float>(value);
           }
           case AccessorComponentType::eUnsigned_Short: {
               uint16_t value = 0u;
               std::memcpy(&value, data, sizeof(value));
               return normalized ? static_cast<float>(value) / 65535.0f : static_cast<float>(value);
           }
           case AccessorComponentType::eUnsigned_int: {
               uint32_t value = 0u;
               std::memcpy(&value, data, sizeof(value));
               return static_cast<float>(value);
           }
           case AccessorComponentType::eFloat: {
               float value = 0.0f;
               std::memcpy(&value, data, sizeof(value));
               return value;
           }
           default:
               return 0.0f;
           }
       }

       inline uint32_t readAccessorIndex(const std::byte * data, AccessorComponentType type) noexcept {
           switch (type) {
           case AccessorComponentType::eUnsigned_Byte:
               return static_cast<uint32_t>(static_cast<uint8_t>(*data));
           case AccessorComponentType::eUnsigned_Short: {
               uint16_t value = 0u;
               std::memcpy(&value, data, sizeof(value));
               return value;
           }
           case AccessorComponentType::eUnsigned_int: {
               uint32_t value = 0u;
               std::memcpy(&value, data, sizeof(value));
               return value;
           }
           default:
               return 0u;
           }
       }

       inline bool isAccessorReadable(DecodedBufferViews & views, const Accessor & accessor) noexcept {
           const std::byte * data = nullptr;
           uint32_t offset = 0u;
           return views.get(accessor.bufferView, data, offset) && data != nullptr;
       }

       inline const std::byte * getAccessorElement(DecodedBufferViews & views, const Accessor & accessor, uint32_t index) noexcept {
           const std::byte * data = nullptr;
           uint32_t viewOffset = 0u;
           if (!views.get(accessor.bufferView, data, viewOffset)) {
               return nullptr;
           }

           const auto & view = views.layout().bufferViews[accessor.bufferView];
           const size_t stride = view.stride != kInvalidValue ? view.stride : accessor.getElementSize();
           return data + viewOffset + accessor.offset + static_cast<size_t>(index) * stride;
       }

       // reads an accessor element as a full precision vec3 (any source component type)
       inline vec3f readAccessorVec3(DecodedBufferViews & views, const Accessor & accessor, uint32_t index) noexcept {
           const std::byte * data = getAccessorElement(views, accessor, index);
           if (!data) {
               return {};
           }

           const auto typeSize = getTypeSize(accessor.componentType);
           const auto components = std::min(accessor.componentsCount(), uint8_t{3u});

           vec3f result{0.0f};
           for (uint8_t i = 0u; i < components; ++i) {
               result[i] = readAccessorComponent(data + i * typeSize, accessor.componentType, accessor.normalized);
           }
           return result;
       }

       // reads an accessor element as a full precision vec2 (any source component type)
       inline vec2f readAccessorVec2(DecodedBufferViews & views, const Accessor & accessor, uint32_t index) noexcept {
           const std::byte * data = getAccessorElement(views, accessor, index);
           if (!data) {
               return {};
           }

           const auto typeSize = getTypeSize(accessor.componentType);
           const auto components = std::min(accessor.componentsCount(), uint8_t{2u});

           vec2f result{0.0f};
           for (uint8_t i = 0u; i < components; ++i) {
               result[i] = readAccessorComponent(data + i * typeSize, accessor.componentType, accessor.normalized);
           }
           return result;
       }

       // reads an accessor element as a full precision vec4 (any source component type).
       // Animation data may be quantized (e.g. rotations as 4x int16 via KHR_mesh_quantization
       // and EXT_meshopt_compression), so it must never be copied as raw floats.
       inline vec4f readAccessorVec4(DecodedBufferViews & views, const Accessor & accessor, uint32_t index) noexcept {
           const std::byte * data = getAccessorElement(views, accessor, index);
           if (!data) {
               return {};
           }

           const auto typeSize = getTypeSize(accessor.componentType);
           const auto components = std::min(accessor.componentsCount(), uint8_t{4u});

           vec4f result{0.0f};
           for (uint8_t i = 0u; i < components; ++i) {
               result[i] = readAccessorComponent(data + i * typeSize, accessor.componentType, accessor.normalized);
           }
           return result;
       }

       // Generates 4D tangents packed as R8G8B8A8_Snorm (4 bytes per vertex) for the primitive.
       // Generation is done on the original accessor data, i.e. before quantization: snorm8
       // normals and half float positions/uvs are not precise enough to build a good TBN basis.
       // The result is appended to the tangent vertex stream before the mesh optimizer remaps it,
       // so the generated tangents are remapped together with all the other streams.
       stl::vector<std::byte> generatePrimitiveTangents(DecodedBufferViews & views, const Primitives & srcPrimitive) {
           const auto & layout = views.layout();
           uint16_t positionAccessorId = kInvalidValue;
           uint16_t normalAccessorId = kInvalidValue;
           uint16_t uvAccessorId = kInvalidValue;

           for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
               switch (semantic) {
               case AttributeSemantic::ePosition: positionAccessorId = accessorId; break;
               case AttributeSemantic::eNormal: normalAccessorId = accessorId; break;
               case AttributeSemantic::eUv0: uvAccessorId = accessorId; break;
               default: break;
               }
           }

           if (positionAccessorId == kInvalidValue || positionAccessorId >= layout.accessors.size()
               || srcPrimitive.indices == kInvalidValue || srcPrimitive.indices >= layout.accessors.size()) {
               return {};
           }

           const auto & positionsAccessor = layout.accessors[positionAccessorId];
           if (!isAccessorReadable(views, positionsAccessor)) {
               return {};
           }

           const auto vertexCount = positionsAccessor.count;

           const bool hasNormal = normalAccessorId != kInvalidValue && normalAccessorId < layout.accessors.size()
               && isAccessorReadable(views, layout.accessors[normalAccessorId]);
           const bool hasUv = uvAccessorId != kInvalidValue && uvAccessorId < layout.accessors.size()
               && isAccessorReadable(views, layout.accessors[uvAccessorId]);

           if (!hasNormal || !hasUv) {
               // there is no source data to build a meaningful tangent basis - emit defaults
               // so the tangent stream stays aligned with the other vertex streams
               stl::vector<std::byte> fallback(vertexCount * 4u);
               for (size_t i = 0u; i < vertexCount; ++i) {
                   fallback[i * 4u + 0u] = static_cast<std::byte>(float_to_snorm(1.0f, 8u));
                   fallback[i * 4u + 1u] = std::byte{0u};
                   fallback[i * 4u + 2u] = std::byte{0u};
                   fallback[i * 4u + 3u] = static_cast<std::byte>(float_to_snorm(1.0f, 8u));
               }
               return fallback;
           }

           const auto & normalsAccessor = layout.accessors[normalAccessorId];
           const auto & uvsAccessor = layout.accessors[uvAccessorId];

           const auto & indicesAccessor = layout.accessors[srcPrimitive.indices];
           if (!isAccessorReadable(views, indicesAccessor)) {
               return {};
           }

           stl::vector<vec3f> positions(vertexCount);
           stl::vector<vec3f> normals(vertexCount);
           stl::vector<vec2f> uvs(vertexCount);

           for (uint32_t i = 0u; i < vertexCount; ++i) {
               positions[i] = readAccessorVec3(views, positionsAccessor, i);
               normals[i] = readAccessorVec3(views, normalsAccessor, i);
               uvs[i] = readAccessorVec2(views, uvsAccessor, i);
           }

           // triangle lists only (the rest of the loader makes the same assumption)
           const auto indexCount = indicesAccessor.count - (indicesAccessor.count % 3u);

           stl::vector<uint32_t> indices(indexCount);
           for (uint32_t i = 0u; i < indexCount; ++i) {
               const auto * element = getAccessorElement(views, indicesAccessor, i);
               const auto index = element ? readAccessorIndex(element, indicesAccessor.componentType) : 0u;
               indices[i] = index < vertexCount ? index : 0u;
           }

           return generateTangentsCompressed(positions, normals, uvs, indices);
       }

       // Bounds of the primitive after skinning: vertices are transformed exactly as the
       // skinning shader does (weighted sum of jointMatrix * position). This is required
       // because the default pose of a skeleton may differ from its bind pose, so neither
       // the raw accessor AABB nor its transform by the node describes the rendered mesh.
       void computePrimitiveSkinnedBounds(DecodedBufferViews & views, const Primitives & srcPrimitive,
           stl::span<const mat4f> jointMatrices, Primitive & primitive) noexcept {
           const auto & layout = views.layout();

           uint16_t positionAccessorId = kInvalidValue;
           uint16_t jointsAccessorId = kInvalidValue;
           uint16_t weightsAccessorId = kInvalidValue;

           for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
               switch (semantic) {
               case AttributeSemantic::ePosition: positionAccessorId = accessorId; break;
               case AttributeSemantic::eJoints: jointsAccessorId = accessorId; break;
               case AttributeSemantic::eWeights: weightsAccessorId = accessorId; break;
               default: break;
               }
           }

           if (positionAccessorId == kInvalidValue || jointsAccessorId == kInvalidValue || weightsAccessorId == kInvalidValue
               || positionAccessorId >= layout.accessors.size() || jointsAccessorId >= layout.accessors.size()
               || weightsAccessorId >= layout.accessors.size()) {
               return;
           }

           const auto & positions = layout.accessors[positionAccessorId];
           const auto & joints = layout.accessors[jointsAccessorId];
           const auto & weights = layout.accessors[weightsAccessorId];

           if (!isAccessorReadable(views, positions) || !isAccessorReadable(views, joints) || !isAccessorReadable(views, weights)) {
               return;
           }

           const auto count = std::min({positions.count, joints.count, weights.count});
           if (count == 0u) {
               return;
           }

           const uint8_t positionComponents = std::min(positions.componentsCount(), uint8_t{3u});
           const uint8_t influenceCount = std::min({joints.componentsCount(), weights.componentsCount(), uint8_t{4u}});

           const auto positionTypeSize = getTypeSize(positions.componentType);
           const auto jointsTypeSize = getTypeSize(joints.componentType);
           const auto weightsTypeSize = getTypeSize(weights.componentType);

           vec3f minCorner{std::numeric_limits<float>::max()};
           vec3f maxCorner{std::numeric_limits<float>::lowest()};

           for (uint32_t vertexIndex = 0u; vertexIndex < count; ++vertexIndex) {
               const auto * positionData = getAccessorElement(views, positions, vertexIndex);
               const auto * jointsData = getAccessorElement(views, joints, vertexIndex);
               const auto * weightsData = getAccessorElement(views, weights, vertexIndex);
               if (!positionData || !jointsData || !weightsData) {
                   return;
               }

               vec3f position{0.0f};
               for (uint8_t i = 0u; i < positionComponents; ++i) {
                   position[i] = readAccessorComponent(positionData + i * positionTypeSize, positions.componentType, positions.normalized);
               }

               vec3f skinnedPosition{0.0f};
               for (uint8_t i = 0u; i < influenceCount; ++i) {
                   const auto weight = readAccessorComponent(weightsData + i * weightsTypeSize, weights.componentType, weights.normalized);
                   if (weight == 0.0f) {
                       continue;
                   }

                   const auto jointIndex = readAccessorIndex(jointsData + i * jointsTypeSize, joints.componentType);
                   if (jointIndex >= jointMatrices.size()) {
                       continue;
                   }

                   skinnedPosition += vec3f(jointMatrices[jointIndex] * vec4f{position, 1.0f}) * weight;
               }

               minCorner = min(minCorner, skinnedPosition);
               maxCorner = max(maxCorner, skinnedPosition);
           }

           primitive.minCorner = minCorner;
           primitive.maxCorner = maxCorner;
           primitive.hasSkinnedBounds = true;
       }
   }

    MeshBuffersData MeshData::loadMeshes(const Layout & layout, stl::span<const AttributeSemantic> allowedAttributes) {
        meshes.reserve(layout.meshes.size());

        MeshBuffersData result;
        auto & vertexBuffersData = result.vertexBuffersData;
        auto & indexBufferData = result.indexBufferData;

        vertexBuffersData.resize(allowedAttributes.size());

        primitiveBegins.reserve(layout.meshes.size());
        uint16_t primitiveBegin = 0u;

        // default-pose skin matrices, needed to compute bounds of skinned meshes
        stl::vector<mat4f> nodeGlobals;
        stl::vector<uint16_t> meshSkins;

        if (!skins.empty() && !nodes.empty()) {
            nodeGlobals.resize(nodes.size(), mat4f{1.0f});

            const auto computeNodeGlobals = [&](const auto & self, uint16_t nodeId, const mat4f & parentTransform) noexcept -> void {
                const auto & node = nodes[nodeId];
                mat4f local = mat4_from_quat(quatf{node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z});
                scale_matrix(local, vec3f{node.scale.x, node.scale.y, node.scale.z});
                translate_matrix_to(local, vec3f{node.translation.x, node.translation.y, node.translation.z});

                nodeGlobals[nodeId] = parentTransform * local;

                for (const auto childId : node.children) {
                    if (childId < nodes.size()) {
                        self(self, childId, nodeGlobals[nodeId]);
                    }
                }
            };

            for (const auto rootId : sceneNodes) {
                if (rootId < nodes.size()) {
                    computeNodeGlobals(computeNodeGlobals, rootId, mat4f{1.0f});
                }
            }

            meshSkins.resize(layout.meshes.size(), kInvalidValue);
            for (const auto & node : nodes) {
                if (node.mesh != kInvalidValue && node.skin != kInvalidValue && node.mesh < meshSkins.size()) {
                    meshSkins[node.mesh] = node.skin;
                }
            }
        }

        // compressed buffer views are decoded once and shared by every accessor that uses them
        DecodedBufferViews decodedViews(layout);

        for (size_t meshIndex = 0u; meshIndex < layout.meshes.size(); ++meshIndex) {
            const auto & srcMesh = layout.meshes[meshIndex];

            stl::vector<mat4f> jointMatrices;
            const auto skinIndex = meshSkins.empty() ? kInvalidValue : meshSkins[meshIndex];
            if (skinIndex != kInvalidValue && skinIndex < skins.size()) {
                const auto & skin = skins[skinIndex];
                const auto jointsCount = std::min(skin.joints.size(), skin.inverseBindMatrices.size());
                jointMatrices.reserve(jointsCount);
                for (size_t i = 0u; i < jointsCount; ++i) {
                    const auto jointNodeId = skin.joints[i];
                    if (jointNodeId < nodeGlobals.size()) {
                        jointMatrices.emplace_back(nodeGlobals[jointNodeId] * skin.inverseBindMatrices[i]);
                    } else {
                        jointMatrices.emplace_back(mat4f{1.0f});
                    }
                }
            }

            primitiveBegins.emplace_back(primitiveBegin);
            primitiveBegin += srcMesh.primitives.size();

            auto & mesh = meshes.emplace_back();
            mesh.primitives.reserve(srcMesh.primitives.size());
            //const auto firstIndex = static_cast<uint32_t>(indexBufferData.size());
            for (const auto & srcPrimitive : srcMesh.primitives) {
                auto & primitive = mesh.primitives.emplace_back();
                primitive.material = srcPrimitive.material;
                primitive.primitiveMode = srcPrimitive.mode;

                // position of the tangent stream in the vertex buffers layout
                size_t tangentAttributeIndex = allowedAttributes.size();
                for (size_t i = 0u; i < allowedAttributes.size(); ++i) {
                    if (allowedAttributes[i] == AttributeSemantic::eTangent) {
                        tangentAttributeIndex = i;
                        break;
                    }
                }

                bool hasSourceTangents = false;
                for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
                    if (semantic == AttributeSemantic::eTangent) {
                        hasSourceTangents = true;
                        break;
                    }
                }

                { // indices
                    if (srcPrimitive.indices == kInvalidValue || srcPrimitive.indices >= layout.accessors.size()) {
                        continue; // the loader supports indexed primitives only
                    }

                    const Accessor & accessor = layout.accessors[srcPrimitive.indices];

                    const std::byte * bufferData = nullptr;
                    uint32_t viewOffset = 0u;
                    if (!decodedViews.get(accessor.bufferView, bufferData, viewOffset)) {
                        continue; // the index buffer view cannot be read
                    }

                    primitive.indexCount = accessor.count;
                    primitive.firstIndex = static_cast<uint32_t>(indexBufferData.size());

                    indexBufferData.resize(primitive.firstIndex + primitive.indexCount);
                    const auto vertexCount = vertexBuffersData[0u].size() / kOptimalSemanticsSizeBytes[0u];

                    switch (accessor.componentType) {
						case AccessorComponentType::eUnsigned_int:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								const size_t offset = accessor.offset + viewOffset + index * sizeof(uint32_t);
								const auto v1 = static_cast<uint8_t>(bufferData[offset + 0]);
								const auto v2 = static_cast<uint8_t>(bufferData[offset + 1]);
								const auto v3 = static_cast<uint8_t>(bufferData[offset + 2]);
								const auto v4 = static_cast<uint8_t>(bufferData[offset + 3]);
								const auto v = static_cast<uint32_t>((static_cast<uint32_t>(v1) << 0) | 
									(static_cast<uint32_t>(v2) << 8) | (static_cast<uint32_t>(v3) << 16) | 
									(static_cast<uint32_t>(v4) << 24));
								indexBufferData[primitive.firstIndex + index] = v + vertexCount;
							}
								break;
						case AccessorComponentType::eUnsigned_Short:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								const size_t offset = accessor.offset + viewOffset + index * sizeof(uint16_t);
								const auto v1 = static_cast<uint8_t>(bufferData[offset + 0]);
								const auto v2 = static_cast<uint8_t>(bufferData[offset + 1]);
								const auto v = static_cast<uint16_t>((static_cast<uint16_t>(v1) << 0) | 
									(static_cast<uint16_t>(v2) << 8));
								indexBufferData[primitive.firstIndex + index] = v + vertexCount;
							}
								break;
						case AccessorComponentType::eUnsigned_Byte:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								indexBufferData[primitive.firstIndex + index] = static_cast<uint32_t>(bufferData[accessor.offset + viewOffset + index]) + vertexCount;
							}
							break;
						default:
							break;
					}
                }
                
                uint32_t attributeId = 0u;
                for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
                    size_t i = 0u;
                    for (auto attributeSemantic : allowedAttributes) {
                        if (attributeSemantic == semantic) {
                            break;
                        }
                        ++i;
                    }

                    if (i < allowedAttributes.size() && accessorId < layout.accessors.size()) {
                        const Accessor & accessor = layout.accessors[accessorId];

                        if (semantic == AttributeSemantic::ePosition) { // position is vec3, alltimes is true?
                            primitive.minCorner = vec3f(accessor.min[0u], accessor.min[1u], accessor.min[2u]);
                            primitive.maxCorner = vec3f(accessor.max[0u], accessor.max[1u], accessor.max[2u]);
                        }

                        const auto count = accessor.count;
                        const auto semanticId = std::to_underlying(semantic);
                        const auto semanticDimension = kOptimalSemanticsSizeBytes[semanticId];
                        const auto targetDataSize = count * semanticDimension;

                        auto & vbData = vertexBuffersData[i];
                        const auto currentBufferOffset = vbData.size();
                        vbData.resize(vbData.size() + targetDataSize);

                        const auto elementSize = accessor.getElementSize();
                        const auto componentType = accessor.componentType;

                        const std::byte * bufferData = nullptr;
                        uint32_t viewOffset = 0u;
                        if (!decodedViews.get(accessor.bufferView, bufferData, viewOffset)) {
                            continue; // keep the stream aligned with zeros
                        }

                        // glTF allows interleaved vertex attributes: several accessors may share one
                        // bufferView with a byteStride larger than the element size. Always honour it.
                        const BufferView & view = layout.bufferViews[accessor.bufferView];
                        const auto sourceStride = view.stride != kInvalidValue ? view.stride : elementSize;

                        const auto * srcBegin = bufferData + viewOffset + accessor.offset;

                        // no fast memcpy path: the target encoding (half float / snorm8 / uint8)
                        // never matches a non-float glTF source bit-for-bit, so every component
                        // is converted by value below
                        {
                            const auto componentCount = accessor.componentsCount();
                            const auto targetComponentCount = static_cast<uint8_t>(kOptimalComponentCount[semanticId]);
                            const auto targetComponentDimension = semanticDimension / targetComponentCount;

                            auto * dst8 = reinterpret_cast<uint8_t*>(&vbData[currentBufferOffset]);
                            auto * dst16 = reinterpret_cast<uint16_t*>(&vbData[currentBufferOffset]);
                            auto * dst32 = reinterpret_cast<uint32_t*>(&vbData[currentBufferOffset]);

                            // the target component is either a half float (2 bytes) or an 8-bit
                            // normalized/unsigned value, so every source component is converted by
                            // value: a raw byte copy is only valid for 8-bit targets, while integer
                            // sources (KHR_mesh_quantization) must not be reinterpreted as halves
                            const auto typeSize = getTypeSize(componentType);
                            const auto normalized = accessor.normalized;
                            constexpr float kMaxHalf = 65504.0f;

                            for (uint32_t i = 0u; i < accessor.count; ++i) {
                                const auto * srcElement = srcBegin + static_cast<size_t>(i) * sourceStride;

                                uint32_t targetIndex = i * targetComponentCount;

                                for (uint32_t j = 0u; j < std::min(componentCount, targetComponentCount); ++j) {
                                    const auto value = readAccessorComponent(srcElement + j * typeSize, componentType, normalized);

                                    switch (targetComponentDimension) {
                                    case 2u:
                                        // half float (position/uv/weights); clamp to the finite half range
                                        dst16[targetIndex + j] = float_to_uint16(std::clamp(value, -kMaxHalf, kMaxHalf));
                                        break;
                                    case 1u:
                                        if (semantic == AttributeSemantic::eJoints) {
                                            dst8[targetIndex + j] = static_cast<uint8_t>(value);
                                        } else {
                                            dst8[targetIndex + j] = static_cast<uint8_t>(float_to_snorm(value, 8u));
                                        }
                                        break;
                                    case 4u:
                                        dst32[targetIndex + j] = static_cast<uint32_t>(value);
                                        break;
                                    default:
                                        assert(false);
                                        break;
                                    }
                                }

                                for (uint32_t j = componentCount; j < targetComponentCount; ++j) {
                                    if (targetComponentDimension == 4u) {
                                        dst32[targetIndex + j] = 0u;
                                    } else if (targetComponentDimension == 2u) {
                                        dst16[targetIndex + j] = 0u;
                                    } else {
                                        dst8[targetIndex + j] = 0u;
                                    }
                                }
                            }
                        }
                    }
                    ++attributeId;
                }

                // tangents are generated from the original full precision accessor data
                // (instead of the quantized vertex buffers) and before the mesh optimizer
                // remaps the vertex streams, so the optimizer remaps the generated tangents
                // together with all the other attributes
                if (tangentAttributeIndex < allowedAttributes.size() && !hasSourceTangents) {
                    const auto tangents = generatePrimitiveTangents(decodedViews, srcPrimitive);
                    auto & tangentBuffer = vertexBuffersData[tangentAttributeIndex];
                    const auto offset = tangentBuffer.size();
                    tangentBuffer.resize(offset + tangents.size());
                    if (!tangents.empty()) {
                        std::memcpy(&tangentBuffer[offset], tangents.data(), tangents.size());
                    }
                }

                if (!jointMatrices.empty()) {
                    computePrimitiveSkinnedBounds(decodedViews, srcPrimitive, jointMatrices, primitive);
                }
            }

            stl::sort(mesh.primitives.begin(), mesh.primitives.end(), [](const auto & a, const auto & b) noexcept {
                return a.material < b.material;
            });
        }

        verticesCount = vertexBuffersData[0u].size() / kOptimalSemanticsSizeBytes[0u];

        return result;
    }

    void MeshData::optimizeBuffers(stl::span<stl::vector<std::byte>> vertexBuffersData, stl::vector<uint32_t> & indexBufferData, stl::span<const AttributeSemantic> allowedAttributes) {
        // mesh optimizer
        stl::vector<meshopt_Stream> streams;
        streams.reserve(vertexBuffersData.size());

        size_t j = 0U;
        for (const auto & vb : vertexBuffersData) {
            if (vb.empty()) {
                ++j;
                continue;
            }
            const auto semanticId = std::to_underlying(allowedAttributes[j]);
            // const auto semanticDimension = kDefaultSemanticsSizeBytes[semanticId];
            const auto semanticDimension = kOptimalSemanticsSizeBytes[semanticId];
            streams.emplace_back(vb.data(), semanticDimension, semanticDimension);
            ++j;
        }

        const auto indexCount = indexBufferData.size();
        const auto baseVertexCount = vertexBuffersData[0u].size() / streams[0u].size;
        
        stl::vector<uint32_t> remap;
        remap.resize(std::max(indexCount, baseVertexCount));
        const auto vertexCount = meshopt_generateVertexRemapMulti(remap.data(), indexBufferData.data(), indexCount, baseVertexCount, streams.data(), streams.size());
        
        meshopt_remapIndexBuffer(indexBufferData.data(), indexBufferData.data(), indexCount, remap.data());

        j = 0U;
        for (auto & vertexBuffer : vertexBuffersData) {
            if (vertexBuffer.empty()) {
                continue;
            }
            stl::vector<std::byte> vertices(vertexCount * streams[j].size);
            meshopt_remapVertexBuffer(vertices.data(), vertexBuffer.data(), baseVertexCount, streams[j].size, remap.data());
            std::swap(vertices, vertexBuffer);
            ++j;
        }
        
        stl::vector<uint32_t> tmp(indexCount);
        meshopt_optimizeVertexCache(tmp.data(), indexBufferData.data(), indexCount, vertexCount);
        meshopt_optimizeVertexFetchRemap(remap.data(), tmp.data(), indexCount, vertexCount);
        
        meshopt_remapIndexBuffer(indexBufferData.data(), indexBufferData.data(), indexCount, remap.data());
        
        j = 0U;
        for (auto & vertexBuffer : vertexBuffersData) {
            if (vertexBuffer.empty()) {
                continue;
            }
            meshopt_remapVertexBuffer(vertexBuffer.data(), vertexBuffer.data(), vertexCount, streams[j].size, remap.data());
            ++j;
        }
      
        ////
        for (auto & vertexBuffer : vertexBuffersData) {
            vertexBuffer.shrink_to_fit();
        }
        indexBufferData.shrink_to_fit();

        verticesCount = vertexBuffersData[0u].size() / kOptimalSemanticsSizeBytes[0u]; // modify after recalculate!
    }

	void MeshData::loadNodes(const Layout & layout) {
		sceneNodes = layout.scenes[layout.scene].nodes;
		nodes = layout.nodes; // copy data
	}

    void MeshData::loadSkins(const Layout & layout) {
        // inverse bind matrices may live in a meshopt-compressed buffer view as well
        DecodedBufferViews decodedViews(layout);

        skins.reserve(layout.skins.size());
        skinMatricesCount = 0u;
        for (const auto & srcSkin : layout.skins) {
            auto & skin = skins.emplace_back(srcSkin.skeleton);
            skin.joints = srcSkin.joints; // copy

            if (srcSkin.inverseBindMatrices != kInvalidValue && srcSkin.inverseBindMatrices < layout.accessors.size()) {
                const auto & accessor = layout.accessors[srcSkin.inverseBindMatrices];

                const std::byte * data = nullptr;
                uint32_t offset = 0u;
                if (!decodedViews.get(accessor.bufferView, data, offset)) {
                    continue;
                }

                skin.inverseBindMatrices.resize(accessor.count);
                memcpy(&skin.inverseBindMatrices[0u], data + offset + accessor.offset, accessor.count * sizeof(mat4f));
                skinMatricesCount += accessor.count;
            }
        }
        assert(skinMatricesCount <= 0xffu); // support 256u bones max if enabled uint8_t joints!
    }

    void MeshData::loadAnimations(const Layout & layout) {
        // animation sampler inputs/outputs may live in meshopt-compressed buffer views as well
        DecodedBufferViews decodedViews(layout);

        animations.reserve(layout.animations.size());

        for (const auto & srcAnimation : layout.animations) {
            auto & anim = animations.emplace_back(srcAnimation.name);
            anim.samplers.reserve(srcAnimation.samplers.size());
            anim.channels.reserve(srcAnimation.channels.size());

            // size the keyframe storage by this animation's own samplers only
            size_t inputsCount = 0u;
            size_t outputsCount = 0u;
            for (const auto & srcSampler : srcAnimation.samplers) {
                if (srcSampler.input < layout.accessors.size()) {
                    inputsCount += layout.accessors[srcSampler.input].count;
                }
                if (srcSampler.output < layout.accessors.size()) {
                    outputsCount += layout.accessors[srcSampler.output].count;
                }
            }

            anim.samplerInputs.resize(inputsCount);
            anim.samplerOutputs.resize(outputsCount);

            size_t inputsOffset = 0u;
            size_t outputsOffset = 0u;

            // samplers
            for (const auto & srcSampler : srcAnimation.samplers) {
                auto & sampler = anim.samplers.emplace_back();
                sampler.interpolation = srcSampler.interpolation;

                // read keyframes input times
                if (srcSampler.input < layout.accessors.size()) {
                    const auto & accessor = layout.accessors[srcSampler.input];
                    const auto available = std::min<size_t>(accessor.count, inputsCount - inputsOffset);

                    for (size_t index = 0u; index < available; ++index) {
                        const std::byte * element = getAccessorElement(decodedViews, accessor, static_cast<uint32_t>(index));
                        anim.samplerInputs[inputsOffset + index] = element
                            ? readAccessorComponent(element, accessor.componentType, accessor.normalized)
                            : 0.0f;
                    }

                    sampler.inputs = {anim.samplerInputs.begin() + inputsOffset, available};
                    inputsOffset += available;

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
                if (srcSampler.output < layout.accessors.size()) {
                    const auto & accessor = layout.accessors[srcSampler.output];
                    const auto available = std::min<size_t>(accessor.count, outputsCount - outputsOffset);

                    // the source data may be quantized (KHR_mesh_quantization / EXT_meshopt_compression),
                    // so every element is converted through the accessor component type, never memcpy'd
                    switch (accessor.type) {
                    case AccessorType::eVec4:
                        for (size_t index = 0u; index < available; ++index) {
                            anim.samplerOutputs[outputsOffset + index] = readAccessorVec4(decodedViews, accessor, static_cast<uint32_t>(index));
                        }
                        break;
                    case AccessorType::eVec3:
                        for (size_t index = 0u; index < available; ++index) {
                            const vec3f value = readAccessorVec3(decodedViews, accessor, static_cast<uint32_t>(index));
                            anim.samplerOutputs[outputsOffset + index] = vec4f{value.x, value.y, value.z, 0.0f};
                        }
                        break;
                    default:
                        for (size_t index = 0u; index < available; ++index) {
                            const std::byte * element = getAccessorElement(decodedViews, accessor, static_cast<uint32_t>(index));
                            const auto value = element ? readAccessorComponent(element, accessor.componentType, accessor.normalized) : 0.0f;
                            anim.samplerOutputs[outputsOffset + index] = vec4f{value, 0.0f, 0.0f, 0.0f};
                        }
                        break;
                    }

                    sampler.outputs = {anim.samplerOutputs.begin() + outputsOffset, available};
                    outputsOffset += available;
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