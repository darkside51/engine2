#include "mesh_data.h"
#include "gltf_loader.h"
#include "graphics/texture/texture_data_loader.h"
#include "graphics/texture/texture_manager.h"
#include "graphics/utils/vertex_processing.h"
#include "file/file_manager.h"

#include "meshoptimizer.h"

#include <cassert>
#include <string>
#include <string_view>

namespace engine::graphics::models {
    
    constexpr static auto kDefaultSemanticsSizeBytes = std::array{
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

    constexpr static auto kOptimalSemanticsSizeBytes = std::array {
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

    constexpr static auto kOptimalComponentCount = std::array {
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

    MeshBuffersData MeshData::loadMeshes(const Layout & layout, std::span<const AttributeSemantic> allowedAttributes) {
        meshes.reserve(layout.meshes.size());

        MeshBuffersData result;
        auto & vertexBuffersData = result.vertexBuffersData;
        auto & indexBufferData = result.indexBufferData;

        vertexBuffersData.resize(allowedAttributes.size());

        primitiveBegins.reserve(layout.meshes.size());
        uint16_t primitiveBegin = 0u;

        for (const auto & srcMesh : layout.meshes) {
            primitiveBegins.emplace_back(primitiveBegin);
            primitiveBegin += srcMesh.primitives.size();

            auto & mesh = meshes.emplace_back();
            mesh.primitives.reserve(srcMesh.primitives.size());
            //const auto firstIndex = static_cast<uint32_t>(indexBufferData.size());
            for (const auto & srcPrimitive : srcMesh.primitives) {
                auto & primitive = mesh.primitives.emplace_back();
                primitive.material = srcPrimitive.material;
                primitive.primitiveMode = srcPrimitive.mode;

                // std::vector<uint32_t> indices;
                // std::vector<std::vector<std::byte>> vertices(allowedAttributes.size());

                { // indices
                    const Accessor & accessor = layout.accessors[srcPrimitive.indices];
                    const BufferView & view = layout.bufferViews[accessor.bufferView];
                    const Buffer& buffer = layout.buffers[view.buffer];

                    primitive.indexCount = accessor.count;
                    primitive.firstIndex = static_cast<uint32_t>(indexBufferData.size());

                    indexBufferData.resize(primitive.firstIndex + primitive.indexCount);
                    // indices.resize(primitive.indexCount);
                    // const auto vertexCount = vertexBuffersData[0u].size() / kDefaultSemanticsSizeBytes[0u];
                    const auto vertexCount = vertexBuffersData[0u].size() / kOptimalSemanticsSizeBytes[0u];

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
								indexBufferData[primitive.firstIndex + index] = v + vertexCount;
                                // indices[index] = v;
							}
								break;
						case AccessorComponentType::eUnsigned_Short:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								const size_t offset = accessor.offset + view.offset + index * sizeof(uint16_t);
								const auto v1 = static_cast<uint8_t>(buffer.data[offset + 0]);
								const auto v2 = static_cast<uint8_t>(buffer.data[offset + 1]);
								const auto v = static_cast<uint16_t>((static_cast<uint16_t>(v1) << 0) | 
									(static_cast<uint16_t>(v2) << 8));
								indexBufferData[primitive.firstIndex + index] = v + vertexCount;
                                // indices[index] = v;
							}
								break;
						case AccessorComponentType::eUnsigned_Byte:
							for (uint32_t index = 0u; index < accessor.count; ++index) {
								indexBufferData[primitive.firstIndex + index] = static_cast<uint32_t>(buffer.data[accessor.offset + view.offset + index]) + vertexCount;
                                // indices[index] = static_cast<uint32_t>(buffer.data[accessor.offset + view.offset + index]);
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

                    if (i < allowedAttributes.size()) {
                        const Accessor & accessor = layout.accessors[accessorId];

                        if (semantic == AttributeSemantic::ePosition) { // position is vec3, alltimes is true?
                            primitive.minCorner = vec3f(accessor.min[0u], accessor.min[1u], accessor.min[2u]);
                            primitive.maxCorner = vec3f(accessor.max[0u], accessor.max[1u], accessor.max[2u]);
                        }

                        const auto count = accessor.count;
                        const auto semanticId = std::to_underlying(semantic);
                        // const auto semanticDimension = kDefaultSemanticsSizeBytes[semanticId];
                        const auto semanticDimension = kOptimalSemanticsSizeBytes[semanticId];
                        const auto targetDataSize = count * semanticDimension;

                        auto & vbData = vertexBuffersData[i];
                        const auto currentBufferOffset = vbData.size();
                        vbData.resize(vbData.size() + targetDataSize);

                        // auto & vbData = vertices[i];
                        // vbData.resize(targetDataSize);

                        const BufferView & view = layout.bufferViews[accessor.bufferView];
                        std::span<const std::byte> bufferViewData = {&layout.buffers[view.buffer].data[accessor.offset + view.offset], count}; // const vector<byte> &

                        const auto componentType = accessor.componentType;
                        const auto acessorType = accessor.type;

                        if (accessor.getElementSize() == semanticDimension) {
                            memcpy(&vbData[currentBufferOffset], &bufferViewData[0u], targetDataSize);
                        } else {
                            const auto componentCount = accessor.componentsCount();
                            const auto targetComponentCount = static_cast<uint8_t>(kOptimalComponentCount[semanticId]);
                            const auto targetComponentDimension = semanticDimension / targetComponentCount;

                            const auto * srcf = reinterpret_cast<const float*>(&bufferViewData[0u]);
                            const auto * src8 = reinterpret_cast<const uint8_t*>(&bufferViewData[0u]);
                            const auto * src16 = reinterpret_cast<const uint16_t*>(&bufferViewData[0u]);
                            const auto * src32 = reinterpret_cast<const uint32_t*>(&bufferViewData[0u]);

                            auto * dst8 = reinterpret_cast<uint8_t*>(&vbData[currentBufferOffset]);
                            auto * dst16 = reinterpret_cast<uint16_t*>(&vbData[currentBufferOffset]);
                            auto * dst32 = reinterpret_cast<uint32_t*>(&vbData[currentBufferOffset]);
                            auto * dstf = reinterpret_cast<float*>(&vbData[currentBufferOffset]);

                            for (uint32_t i = 0u; i < accessor.count; ++i) {
                                uint32_t sourceIndex = i * componentCount;
                                uint32_t targetIndex = i * targetComponentCount;

                                for (uint32_t j = 0u; j < std::min(componentCount, targetComponentCount); ++j) {
                                    switch (targetComponentDimension) {
                                    case 2u:
                                    {
                                        if (accessor.componentType == AccessorComponentType::eByte || accessor.componentType == AccessorComponentType::eUnsigned_Byte) {
                                            dst16[targetIndex + j] = src8[sourceIndex + j];
                                        } else if (accessor.componentType == AccessorComponentType::eShort || accessor.componentType == AccessorComponentType::eUnsigned_Short) {
                                            dst16[targetIndex + j] = src16[sourceIndex + j];
                                        } else if (accessor.componentType == AccessorComponentType::eUnsigned_int) {
                                            dst16[targetIndex + j] = static_cast<uint16_t>(src32[sourceIndex + j]);
                                        }  else if (accessor.componentType == AccessorComponentType::eFloat) {
                                            dst16[targetIndex + j] = float_to_uint16(srcf[sourceIndex + j]);
                                        }
                                    }
                                        break;
                                    case 1u:
                                    {
                                        if (accessor.componentType == AccessorComponentType::eByte || accessor.componentType == AccessorComponentType::eUnsigned_Byte) {
                                            dst8[targetIndex + j] = src8[sourceIndex + j];
                                        } else if (accessor.componentType == AccessorComponentType::eShort || accessor.componentType == AccessorComponentType::eUnsigned_Short) {
                                            dst8[targetIndex + j] = static_cast<uint8_t>(src16[sourceIndex + j]);
                                        } else if (accessor.componentType == AccessorComponentType::eUnsigned_int) {
                                            dst8[targetIndex + j] = static_cast<uint8_t>(src32[sourceIndex + j]);
                                        }  else if (accessor.componentType == AccessorComponentType::eFloat) {
                                            dst8[targetIndex + j] = float_to_snorm(srcf[sourceIndex + j], 8u);
                                        }
                                    }
                                        break;
                                    case 4u:
                                    {
                                        if (accessor.componentType == AccessorComponentType::eByte || accessor.componentType == AccessorComponentType::eUnsigned_Byte) {
                                            dst32[targetIndex + j] = src8[sourceIndex + j];
                                        } else if (accessor.componentType == AccessorComponentType::eShort || accessor.componentType == AccessorComponentType::eUnsigned_Short) {
                                            dst32[targetIndex + j] = src16[sourceIndex + j];
                                        } else if (accessor.componentType == AccessorComponentType::eUnsigned_int) {
                                            dst32[targetIndex + j] = src32[sourceIndex + j];
                                        }  else if (accessor.componentType == AccessorComponentType::eFloat) {
                                            dstf[targetIndex + j] = srcf[sourceIndex + j];
                                        }
                                    }
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



                        // if (accessor.getElementSize() == semanticDimension) {
                        //     memcpy(&vbData[currentBufferOffset], &bufferViewData[0u], targetDataSize);
                        //     // memcpy(vbData.data(), bufferViewData.data(), targetDataSize);
                        // } else { // attribute convertion
                        //     // assert(false); // need impl
                        //     switch (acessorType) {
                        //     case AccessorType::eScalar: {
                        //         assert(false); // need impl
                        //     }
                        //         break;
                        //     case AccessorType::eVec2: {
                        //         assert(false); // need impl
                        //     }
                        //         break;
                        //     case AccessorType::eVec3: {
                        //         assert(false); // need impl
                        //     }
                        //         break;
                        //     case AccessorType::eVec4: {
                        //         // for unsigned byte
                        //         uint16_t * target = reinterpret_cast<uint16_t*>(&vbData[currentBufferOffset]);
                        //         for (uint32_t i = 0u; i < accessor.count; ++i) {
                        //             const size_t offset = accessor.offset + view.offset + i * 4u;
                        //             target[i * 4u + 0u] = static_cast<uint16_t>(bufferViewData[i * 4u + 0u]);
                        //             target[i * 4u + 1u] = static_cast<uint16_t>(bufferViewData[i * 4u + 1u]);
                        //             target[i * 4u + 2u] = static_cast<uint16_t>(bufferViewData[i * 4u + 2u]);
                        //             target[i * 4u + 3u] = static_cast<uint16_t>(bufferViewData[i * 4u + 3u]);
                        //         }
                        //     }
                        //         break;
                            
                        //     default:
                        //         break;
                        //     }
                        }
                    }
                    ++attributeId;
                }

                // optimizeBuffers(vertices, indices, allowedAttributes);

                // indexBufferData.reserve(indexBufferData.size() + indices.size());
                // const auto vertexCount = vertexBuffersData[0u].size() / kDefaultSemanticsSizeBytes[0u];
                // for (size_t i = 0; i < indices.size(); ++i) {
                //     indexBufferData.push_back(indices[i] + vertexCount);
                // }

                // size_t j = 0;
                // for (auto & vertexBuffer : vertexBuffersData) {
                //     vertexBuffer.reserve(vertexBuffer.size() + vertices[j].size());
                //     for (size_t i = 0; i < vertices[j].size(); ++i) {
                //         vertexBuffer.push_back(vertices[j][i]);
                //     }
                //     ++j;
                // }

////

                // std::array<std::pair<size_t, uint32_t>, std::to_underlying(AttributeSemantic::eSemanticsCount)> counts{}; 
                // for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
                //     size_t i = 0u;
                //     for (auto attributeSemantic : allowedAttributes) {
                //         if ((attributeSemantic & semantic) == semantic) {
                //             break;
                //         }
                //         ++i;
                //     }

                //     if (i < allowedAttributes.size()) {
                //         const Accessor & accessor = layout.accessors[accessorId];
                //         // counts[i].first += accessor.getElementSize() * accessor.count; // need to get element size from in params (use predefined size for attributes (12 bytes for position, 4 bytes for color...))
                //         // need to convert data after
                //         const auto count = accessor.count;
                //         const auto semanticDimension = kDefaultSemanticsSizeBytes[std::log2(std::to_underlying(semantic))];
                //         counts[i].first += semanticDimension * count;
                //         counts[i].second |= 1u << attributeId;

                //         //auto & vbOffset = primitive.vbOffsets[i];
                //         //auto & vbData = vertexBuffersData[i];
                //         //vbOffset.offset = vbData.size();
                //         //vbData.resize(vbData.size() + count * semanticDimension);

                //         // const BufferView & view = layout.bufferViews[accessor.bufferView];
                //         // const auto componentType = accessor.componentType;
                //         // const auto type = accessor.type;
                //     }
                //     ++attributeId;
                // }

                // for (size_t j = 0u; j < allowedAttributes.size(); ++j) {
                //     auto & vbData = vertexBuffersData[j];
                //     auto & vbOffset = primitive.vbOffsets[j];
                //     vbOffset.offset = vbData.size();
                //     vbData.resize(vbData.size() + counts[j].first);

                //     uint32_t attributeId = 0u;
                //     for (const auto & [semantic, accessorId] : srcPrimitive.attributes) {
                //         if (counts[j].second & (1u << attributeId)) {
                //             const Accessor & accessor = layout.accessors[accessorId];
                //             const BufferView & view = layout.bufferViews[accessor.bufferView];

                //             const auto dataSize = accessor.getElementSize() * accessor.count; // 

                //             // copy per elements data
                //         }
                //         ++attributeId;
                //     }
                // }
                // break;
            }

            std::sort(mesh.primitives.begin(), mesh.primitives.end(), [](const auto & a, const auto & b) noexcept {
                return a.material < b.material;
            });
        }

        return result;
    }

    void MeshData::optimizeBuffers(std::span<std::vector<std::byte>> vertexBuffersData, std::vector<uint32_t> & indexBufferData, std::span<const AttributeSemantic> allowedAttributes) {
        // mesh optimizer
        std::vector<meshopt_Stream> streams;
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
        
        std::vector<uint32_t> remap;
        remap.resize(std::max(indexCount, baseVertexCount));
        const auto vertexCount = meshopt_generateVertexRemapMulti(remap.data(), indexBufferData.data(), indexCount, baseVertexCount, streams.data(), streams.size());
        
        meshopt_remapIndexBuffer(indexBufferData.data(), indexBufferData.data(), indexCount, remap.data());

        j = 0U;
        for (auto & vertexBuffer : vertexBuffersData) {
            if (vertexBuffer.empty()) {
                continue;
            }
            std::vector<std::byte> vertices(vertexCount * streams[j].size);
            meshopt_remapVertexBuffer(vertices.data(), vertexBuffer.data(), baseVertexCount, streams[j].size, remap.data());
            std::swap(vertices, vertexBuffer);
            ++j;
        }
        
        std::vector<uint32_t> tmp(indexCount);
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
      
        // for (auto & mesh : meshes) {
        //     for (auto & primitive : mesh.primitives) {
                //primitive.firstIndex = remap[primitive.firstIndex];
        //     }
        // }

        // // quantize attributes
        // for (size_t i = 0u; i < allowedAttributes.size(); ++i) {
        //     if (allowedAttributes[i] == AttributeSemantic::ePosition) {
        //         constexpr auto quantizePositionSemanticSize = 8u; // eR16G16B16A16_Sfloat // for 4 bytes align
        //         constexpr auto defaultPositionSemanticSize = kDefaultSemanticsSizeBytes[0u];
        //         std::vector<std::byte> vertices(vertexCount * quantizePositionSemanticSize);
        //         auto & vb = vertexBuffersData[i];
        //         if (vb.empty()) {
        //             continue;
        //         }

        //         for (size_t n = 0u; n < vertexCount; ++n) {
        //             const float * position = reinterpret_cast<const float*>(&vb[n * defaultPositionSemanticSize]);
        //             uint16_t * targetPosition = reinterpret_cast<uint16_t*>(&vertices[n * quantizePositionSemanticSize]);

        //             targetPosition[0u] = float_to_uint16(position[0u]);
        //             targetPosition[1u] = float_to_uint16(position[1u]);
        //             targetPosition[2u] = float_to_uint16(position[2u]);
        //             targetPosition[3u] = 0u;
        //         }
        //         std::swap(vertices, vb);
        //     }

        //     if (allowedAttributes[i] == AttributeSemantic::eNormal) {
        //         constexpr auto quantizeNormalSemanticSize = 4u; // eR8G8B8A8_Snorm // for 4 bytes align
        //         constexpr auto defaultNormalSemanticSize = kDefaultSemanticsSizeBytes[1u];
        //         std::vector<std::byte> vertices(vertexCount * quantizeNormalSemanticSize);
        //         auto & vb = vertexBuffersData[i];
        //         if (vb.empty()) {
        //             continue;
        //         }

        //         for (size_t n = 0u; n < vertexCount; ++n) {
        //             const float * normal = reinterpret_cast<const float*>(&vb[n * defaultNormalSemanticSize]);

        //             vertices[n * quantizeNormalSemanticSize + 0u] = static_cast<std::byte>(float_to_snorm(normal[0u], 8u));
        //             vertices[n * quantizeNormalSemanticSize + 1u] = static_cast<std::byte>(float_to_snorm(normal[1u], 8u));
        //             vertices[n * quantizeNormalSemanticSize + 2u] = static_cast<std::byte>(float_to_snorm(normal[2u], 8u));
        //             vertices[n * quantizeNormalSemanticSize + 3u] = std::byte{0u};
        //         }
        //         std::swap(vertices, vb);
        //     }

        //     if (allowedAttributes[i] == AttributeSemantic::eUv0) {
        //         constexpr auto quantizeUvSemanticSize = 4u; // eR16G16_Sfloat
        //         constexpr auto defaultUvSemanticSize = kDefaultSemanticsSizeBytes[9u];
        //         std::vector<std::byte> vertices(vertexCount * quantizeUvSemanticSize);
        //         auto & vb = vertexBuffersData[i];
        //         if (vb.empty()) {
        //             continue;
        //         }

        //         for (size_t n = 0u; n < vertexCount; ++n) {
        //             const float * uv = reinterpret_cast<const float*>(&vb[n * defaultUvSemanticSize]);
        //             uint16_t * targetUv = reinterpret_cast<uint16_t*>(&vertices[n * quantizeUvSemanticSize]);
        //             targetUv[0u] = float_to_uint16(uv[0u]);
        //             targetUv[1u] = float_to_uint16(uv[1u]);
        //             // const uint16_t u = float_to_unorm(uv[0u], 16u);
        //             // const uint16_t v = float_to_unorm(uv[1u], 16u);

        //             // vertices[n * quantizeUvSemanticSize + 0u] = static_cast<std::byte>(u);
        //             // vertices[n * quantizeUvSemanticSize + 1u] = static_cast<std::byte>(u >> 8u);
        //             // vertices[n * quantizeUvSemanticSize + 2u] = static_cast<std::byte>(v);
        //             // vertices[n * quantizeUvSemanticSize + 3u] = static_cast<std::byte>(v >> 8u);
        //         }
        //         std::swap(vertices, vb);
        //     }

        //     if (allowedAttributes[i] == AttributeSemantic::eJoints) { // support 256 bones max if it enabled!
        //         constexpr auto quantizeSemanticSize = 4u; // eR8G8B8A8_Uint
        //         constexpr auto defaultSemanticSize = kDefaultSemanticsSizeBytes[7u];
        //          std::vector<std::byte> vertices(vertexCount * quantizeSemanticSize);
        //         auto & vb = vertexBuffersData[i];
        //         if (vb.empty()) {
        //             continue;
        //         }

        //         for (size_t n = 0u; n < vertexCount; ++n) {
        //             const auto * value = reinterpret_cast<const uint16_t*>(&vb[n * defaultSemanticSize]);
        //             auto * targetValue = reinterpret_cast<uint8_t*>(&vertices[n * quantizeSemanticSize]);

        //             targetValue[0u] = static_cast<uint8_t>(value[0u]);
        //             targetValue[1u] = static_cast<uint8_t>(value[1u]);
        //             targetValue[2u] = static_cast<uint8_t>(value[2u]);
        //             targetValue[3u] = static_cast<uint8_t>(value[3u]);
        //         }
        //         std::swap(vertices, vb);
        //     }

        //     if (allowedAttributes[i] == AttributeSemantic::eWeights) {
        //         constexpr auto quantizeSemanticSize = 8u; // eR16G16B16A16_Sfloat
        //         constexpr auto defaultSemanticSize = kDefaultSemanticsSizeBytes[8u];
        //          std::vector<std::byte> vertices(vertexCount * quantizeSemanticSize);
        //         auto & vb = vertexBuffersData[i];
        //         if (vb.empty()) {
        //             continue;
        //         }

        //         for (size_t n = 0u; n < vertexCount; ++n) {
        //             const float * value = reinterpret_cast<const float*>(&vb[n * defaultSemanticSize]);
        //             uint16_t * targetValue = reinterpret_cast<uint16_t*>(&vertices[n * quantizeSemanticSize]);

        //             targetValue[0u] = float_to_uint16(value[0u]);
        //             targetValue[1u] = float_to_uint16(value[1u]);
        //             targetValue[2u] = float_to_uint16(value[2u]);
        //             targetValue[3u] = float_to_uint16(value[3u]);
        //         }
        //         std::swap(vertices, vb);
        //     }
        // }

        const bool useTangents = std::find(allowedAttributes.begin(), allowedAttributes.end(), AttributeSemantic::eTangent) != allowedAttributes.end();

        if (useTangents && vertexBuffersData[2u].empty()) { // generate tangents
            auto src16p = std::span(reinterpret_cast<const uint16_t*>(vertexBuffersData[0u].data()), vertexBuffersData[0u].size() / sizeof(uint16_t));
            auto src8n = std::span(reinterpret_cast<const int8_t*>(vertexBuffersData[1u].data()), vertexBuffersData[1u].size() / sizeof(int8_t));
            auto src16uv = std::span(reinterpret_cast<const uint16_t*>(vertexBuffersData[3u].data()), vertexBuffersData[3u].size() / sizeof(uint16_t));
            vertexBuffersData[2u] = generateTangentsCompressed(
                src16p,
                src8n,
                src16uv,
                indexBufferData
            );
        }

        ////
        for (auto & vertexBuffer : vertexBuffersData) {
            vertexBuffer.shrink_to_fit();
        }
        indexBufferData.shrink_to_fit();
    }

	void MeshData::loadNodes(const Layout & layout) {
		sceneNodes = layout.scenes[layout.scene].nodes;
		nodes = layout.nodes; // copy data

        // auto loadMeshNode = [&](const auto & self, uint16_t nodeId) noexcept -> void {
        //     const auto & node = nodes[nodeId];

        //     // if (node.mesh != kInvalidValue) {
        //     //     meshes[node.mesh].nodeIndex = nodeId;
        //     //     meshes[node.mesh].skinIndex = node.skin;
        //     // }

        //     for (uint16_t cNodeId : node.children) {
		// 	    self(self, cNodeId);
		//     }
        // };

		// for (uint16_t nodeId : sceneNodes) {
		// 	loadMeshNode(loadMeshNode, nodeId);
		// }
	}

    void MeshData::loadSkins(const Layout & layout) {
        skins.reserve(layout.skins.size());
        skinMatricesCount = 0u;
        for (const auto & srcSkin : layout.skins) {
            auto & skin = skins.emplace_back(srcSkin.skeleton);
            skin.joints = srcSkin.joints; // copy

            if (srcSkin.inverseBindMatrices != kInvalidValue) {
                const auto & accessor = layout.accessors[srcSkin.inverseBindMatrices];
				const auto & bufferView = layout.bufferViews[accessor.bufferView];
				const auto & buffer = layout.buffers[bufferView.buffer];
				skin.inverseBindMatrices.resize(accessor.count);
                memcpy(&skin.inverseBindMatrices[0u], &buffer.data[accessor.offset + bufferView.offset], accessor.count * sizeof(mat4f));
                skinMatricesCount += accessor.count;
            }
        }
        assert(skinMatricesCount <= 0xffu); // support 256 bones max if enabled uint8_t joints!
    }

    void MeshData::loadAnimations(const Layout & layout) {
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
            ++i;
        }
	}

    // void MeshData::loadMaterials(const Layout & layout, const FileManager & fileManager, TextureManager & textureManager, std::string_view name) {
    //     auto const loadTexture = [&layout, &fileManager, &textureManager](uint16_t index, std::string_view name) -> ImageFrame2D {
    //         if (index == kInvalidValue) {
    //             return {};
    //         }

    //         const auto & texture = layout.textures[index];
    //         const auto & image = layout.images[texture.source];

    //         if (image.bufferView != kInvalidValue) {
    //             const auto & bufferView = layout.bufferViews[image.bufferView];
    //             const auto & buffer = layout.buffers[bufferView.buffer];
    //             const std::byte * begin = &buffer.data[bufferView.offset];
    //             auto textureData = engine::graphics::parseTextureData({begin, begin + bufferView.length});
    //             engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Unorm, textureData); // ??
    //             if (!image.name.empty()) {
    //                 return textureManager.addFrame(image.name, std::move(textureData));
    //             } else {
    //                 return textureManager.addFrame(std::string{name} + std::to_string(index), std::move(textureData));
    //             }
    //         }
    //         if (!image.uri.empty()) {
    //             auto const imageBytes = fileManager.readFile(image.uri);
    //             auto textureData = engine::graphics::parseTextureData(imageBytes);
    //             engine::graphics::convertTextureDataFormat(engine::rhi::Format::eR8G8B8A8_Unorm, textureData); // ??
    //             if (!image.name.empty()) {
    //                 return textureManager.addFrame(image.name, std::move(textureData));
    //             } else {
    //                 return textureManager.addFrame(std::string{name} + std::to_string(index), std::move(textureData));
    //             }
    //         }

    //         return {};
    //     };

    //     materials.reserve(layout.materials.size());

    //     for (const auto & srcMaterial : layout.materials) {
    //         auto & material = materials.emplace_back();
    //         material.baseColorFactor = {
    //             srcMaterial.pbrMetallicRoughness.baseColorFactor.x,
    //             srcMaterial.pbrMetallicRoughness.baseColorFactor.y,
    //             srcMaterial.pbrMetallicRoughness.baseColorFactor.z,
    //             srcMaterial.pbrMetallicRoughness.baseColorFactor.w
    //         };
    //         material.emissiveFactor = {
    //             srcMaterial.emissiveFactor.x,
    //             srcMaterial.emissiveFactor.y,
    //             srcMaterial.emissiveFactor.z,
    //         };
    //         material.metallicFactor = srcMaterial.pbrMetallicRoughness.metallicFactor;
    //         material.roughnessFactor = srcMaterial.pbrMetallicRoughness.roughnessFactor;
    //         material.alphaCutoff = srcMaterial.alphaCutoff;
    //         switch (srcMaterial.alphaMode) {
    //         case AlphaMode::eOpaque:
    //             material.blendMode = rhi::common_blend_modes::blend_none;
    //             break;
    //         case AlphaMode::eMask:
    //             material.blendMode = rhi::common_blend_modes::blend_multiply; // ?
    //             break;
    //         case AlphaMode::eBlend:
    //             material.blendMode = rhi::common_blend_modes::blend_alpha_pma; // ?
    //             break;
    //         }

    //         material.cullMode = srcMaterial.doubleSided ? rhi::CullMode::eNone : rhi::CullMode::eBack;

    //         material.textures[static_cast<uint8_t>(TextureId::eBaseColor)] = loadTexture(srcMaterial.pbrMetallicRoughness.baseColorTexture.index, name);
    //         material.textures[static_cast<uint8_t>(TextureId::eNormal)] = loadTexture(srcMaterial.normalTexture.index, name);
    //         material.textures[static_cast<uint8_t>(TextureId::eMetallicRoughness)] = loadTexture(srcMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index, name);
    //         material.textures[static_cast<uint8_t>(TextureId::eOccolusion)] = loadTexture(srcMaterial.occlusionTexture.index, name);
    //         material.textures[static_cast<uint8_t>(TextureId::eEmissive)] = loadTexture(srcMaterial.emissiveTexture.index, name);
    //     }
    // }

}