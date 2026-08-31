#include "cascade_shadow_map.h"
#include "graphics/render/rhi/common/command_list.h"

namespace engine::graphics {
    constexpr bool kUseInstanceDrawShadows = true;
    constexpr bool kUseVarianceShadowMap = false;

    CascadeShadowMap::CascadeShadowMap(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
            uint16_t size, uint8_t cascades, uint8_t bits, float near, float far) : 
            _api(api),
            _device(device),
            _cascadeMatrices(cascades),
            _splitDepths(cascades),
            _cascadeSplits(cascades),
            _texelSizes(cascades),
            _clipRange(std::abs(far - near)),
            _reversedDepth(near > far),
            _near(_reversedDepth ? far : near),
            _far(_reversedDepth ? near : far),
            _size(size),
            _cascadesCount(cascades),
            _bits(bits) {
    }

    void CascadeShadowMap::createRenderDataVSM() {
        using namespace rhi;

        const auto colorFormat = rhi::Format::eR16G16_Sfloat;
        auto access = _device.isFormatSupported(colorFormat, rhi::FormatFeatureFlags::eFormatFeature_Color_Attachment);
        const ImageAspectFlags colorAspectFlags = ImageAspectFlags::eColor;

        const auto format = _device.getSupportedDepthFormat(_bits, false, false);

        const ImageAspectFlags aspectFlags = isDepthStencilFormat(format) ? 
                                                ImageAspectFlags::eDepthStencil : 
                                                ImageAspectFlags::eDepth;

        // renderpass
        stl::array attachments = {
            AttachmentDescription {
                .format = format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eShader_Read_Only_Optimal,
                .samples = 1u
            },
            AttachmentDescription {
                .format = format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eDont_Care},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                .samples = 1u
            }
        };

        stl::array subpass  = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .depthStencilAttachment = {
                    .pNext = {nullptr, 0u},
                    .attachment = 1u,
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal, //eDepth_Stencil_Attachment_Optimal, // eDepth_Attachment_Optimal - maybe?
                    .aspectMask = aspectFlags
                },
                .colorAttachments = {
                    {
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eColor_Attachment_Optimal,
                    .aspectMask = colorAspectFlags
                    }
                }
            }
        };

        const RenderPassDescription description{attachments, subpass, {}};
        _renderPass = _api.getRenderPass(_device, description);

        // image
        const auto colorImageInfo = ImageInfo {
            ImageUsageFlags::eSampled | ImageUsageFlags::eColor_Attachment, colorFormat, {_size, _size, 1u},
            colorAspectFlags,
            ImageType::eType_2D,
            1u,
            _cascadesCount,
            1u,
            ImageTiling::eOptimal
        };

        // depthImage
        const auto imageInfo = ImageInfo {
            ImageUsageFlags::eDepth_Stencil_Attachment, format, {_size, _size, 1u},
            aspectFlags,
            ImageType::eType_2D,
            1u,
            _cascadesCount,
            1u,
            ImageTiling::eOptimal
        };

        const auto frames = _device.inFlightFramesCount();
        _framebuffers.reserve(frames);

        for (uint8_t i = 0u; i < frames; ++i) {
            CascadeFrameBuffer framebuffer = {
                .image = _api.createImage(_device, colorImageInfo),
                .depthImage = _api.createImage(_device, imageInfo),
                .frameBuffers = stl::vector<runtime::FrameBufferPtr>(kUseInstanceDrawShadows ? 1u : _cascadesCount)
            };

            if constexpr (kUseInstanceDrawShadows) {
                //create only 1 framebuffer if draw with instancing
                stl::vector fbAttachemnts = {
                    FrameBufferAttachment{
                        .image = framebuffer.image.get(),
                        .imageSize = {_size, _size},
                        .aspect = ImageAspectFlags::eColor,
                        .layer = 0u,
                        .level = 0u,
                        .layersCount = _cascadesCount,
                        .levelsCount = 1u,
                        .viewType = ImageViewType::eType_2D_Array
                    },
                    FrameBufferAttachment{
                        .image = framebuffer.depthImage.get(),
                        .imageSize = {_size, _size},
                        .aspect = ImageAspectFlags::eDepth, // eDepth - is true here
                        .layer = 0u,
                        .level = 0u,
                        .layersCount = _cascadesCount,
                        .levelsCount = 1u,
                        .viewType = ImageViewType::eType_2D_Array
                    }, 
                };

                framebuffer.frameBuffers[0] = _api.createFrameBuffer(_device, std::move(fbAttachemnts), _renderPass);
            } else {
                //....
            }
            
            _framebuffers.emplace_back(framebuffer);
        }

        // sampler
        _sampler.info = {
            .magFilter = FilterMode::eLinear,
            .minFilter = FilterMode::eLinear,
            .mipMapFilter = FilterMode::eNearest,
            .addressModeU = SamplerAddressMode::eClamp_To_Border,
            .addressModeV = SamplerAddressMode::eClamp_To_Border,
            .addressModeW = SamplerAddressMode::eClamp_To_Border,
            .compareOp = _reversedDepth ? CompareOp::eGreater : CompareOp::eLess,
            .borderColor = _reversedDepth ? BorderColor::eFloat_Opaque_Black : BorderColor::eFloat_Opaque_White,
            .maxAnisotropy = 0.0f
        };
    }

    void CascadeShadowMap::createRenderData() {
        using namespace rhi;

        const auto format = _device.getSupportedDepthFormat(_bits, false, true);
        const ImageAspectFlags aspectFlags = isDepthStencilFormat(format) ? 
                                                ImageAspectFlags::eDepthStencil : 
                                                ImageAspectFlags::eDepth;

        // renderpass
        stl::array attachment = {
            AttachmentDescription {
                .format = format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eDepth_Stencil_Read_Only_Optimal,
                .samples = 1u
            }
        };

        stl::array subpass  = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .depthStencilAttachment = {
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal,
                    .aspectMask = aspectFlags
                }
            }
        };

        // stl::array<SubpassDependency, 2u> dependency;
        // dependency[0u] = {
        //     .stages = {
        //         .src = {
        //             .stage = PipelineStageFlags::eFragment_Shader,
        //             .accessMask = AccessFlags::eShader_Read
        //         },
        //         .dst = {
        //             .stage = PipelineStageFlags::eEarly_Fragment_Tests,
        //             .accessMask = AccessFlags::eDepth_Stencil_Attachment_Write
        //         }
        //     }
        // };

        // dependency[1u] = {
        //     .stages = {
        //         .src = {
        //             .stage = PipelineStageFlags::eLate_Fragment_Tests,
        //             .accessMask = AccessFlags::eDepth_Stencil_Attachment_Write
        //         },
        //         .dst = {
        //             .stage = PipelineStageFlags::eFragment_Shader,
        //             .accessMask = AccessFlags::eShader_Read
        //         }
        //     }
        // };

        // const RenderPassDescription description{attachment, subpass, dependency};
        const RenderPassDescription description{attachment, subpass, {}};
        _renderPass = _api.getRenderPass(_device, description);

        // image
        const auto imageInfo = ImageInfo {
            ImageUsageFlags::eSampled | ImageUsageFlags::eDepth_Stencil_Attachment, format, {_size, _size, 1u},
            aspectFlags,
            ImageType::eType_2D,
            1u,
            _cascadesCount,
            1u,
            ImageTiling::eOptimal
        };

        const auto frames = _device.inFlightFramesCount();
        _framebuffers.reserve(frames);

        for (uint8_t i = 0u; i < frames; ++i) {
            CascadeFrameBuffer framebuffer = {
                .image = nullptr,
                .depthImage = _api.createImage(_device, imageInfo),
                .frameBuffers = stl::vector<runtime::FrameBufferPtr>(kUseInstanceDrawShadows ? 1u : _cascadesCount)
            };

            if constexpr (kUseInstanceDrawShadows) {
                //create only 1 framebuffer if draw with instancing
                stl::vector fbAttachemnts = {
                    FrameBufferAttachment{
                        .image = framebuffer.depthImage.get(),
                        .imageSize = {_size, _size},
                        .aspect = ImageAspectFlags::eDepth, // eDepth - is true here
                        .layer = 0u,
                        .level = 0u,
                        .layersCount = _cascadesCount,
                        .levelsCount = 1u,
                        .viewType = ImageViewType::eType_2D_Array
                    }
                };

                framebuffer.frameBuffers[0] = _api.createFrameBuffer(_device, std::move(fbAttachemnts), _renderPass);
            } else {
                for (uint8_t j = 0u; j < _cascadesCount; ++j) {
                    stl::vector fbAttachemnts = {
                        FrameBufferAttachment{
                            .image = framebuffer.depthImage.get(),
                            .imageSize = {_size, _size},
                            .aspect = ImageAspectFlags::eDepth, // eDepth - is true here
                            .layer = j,
                            .level = 0u,
                            .layersCount = 1u,
                            .levelsCount = 1u,
                            .viewType = ImageViewType::eType_2D
                        }
                    };

                    framebuffer.frameBuffers[j] = _api.createFrameBuffer(_device, std::move(fbAttachemnts), _renderPass);
                }
            }
            
            _framebuffers.emplace_back(framebuffer);
        }

        // sampler
        _sampler.info = {
            .magFilter = FilterMode::eLinear,
            .minFilter = FilterMode::eLinear,
            .mipMapFilter = FilterMode::eNearest,
            .addressModeU = SamplerAddressMode::eClamp_To_Border,
            .addressModeV = SamplerAddressMode::eClamp_To_Border,
            .addressModeW = SamplerAddressMode::eClamp_To_Border,
            .compareOp = _reversedDepth ? CompareOp::eGreater : CompareOp::eLess,
            .borderColor = _reversedDepth ? BorderColor::eFloat_Opaque_Black : BorderColor::eFloat_Opaque_White,
            .maxAnisotropy = 0.0f
        };
    }

    void CascadeShadowMap::prepare() {
        _device.evaluateSampler(_sampler);
    }

    void CascadeShadowMap::initCascadeSplits(vec3f splitNearFarOffsets) noexcept {
        _cascadeSplitLambda = splitNearFarOffsets.x;
        const float minZ = _near + splitNearFarOffsets.y;
		const float maxZ = _far + splitNearFarOffsets.z;

		const float range = maxZ - minZ;
		const float ratio = maxZ / minZ;

		// calculate split depths based on view camera frustum
		// based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0u; i < _cascadesCount; ++i) {
			const float p = (i + 1u) / static_cast<float>(_cascadesCount);
			const float log = minZ * std::pow(ratio, p);
			const float uniform = minZ + range * p;
			const float d = _cascadeSplitLambda * (log - uniform) + uniform;
			_cascadeSplits[i] = (d - _near) / _clipRange;
		}
    }

    void CascadeShadowMap::updateCascades(const mat4f & cameraInverse) {
        // calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0f;
		for (uint8_t i = 0u; i < _cascadesCount; ++i) {
            const float splitDist = _cascadeSplits[i];

            auto frustumCorners = _reversedDepth ? stl::array<vec3f, 8u>{
				vec3f{-1.0f,  1.0f, 1.0f},
				vec3f{ 1.0f,  1.0f, 1.0f},
				vec3f{ 1.0f, -1.0f, 1.0f},
				vec3f{-1.0f, -1.0f,	1.0f},
				vec3f{-1.0f,  1.0f, 0.0f},
				vec3f{ 1.0f,  1.0f, 0.0f},
				vec3f{ 1.0f, -1.0f, 0.0f},
				vec3f{-1.0f, -1.0f, 0.0f}
			} :
            stl::array<vec3f, 8u>{
				vec3f{-1.0f,  1.0f, 0.0f},
				vec3f{ 1.0f,  1.0f, 0.0f},
				vec3f{ 1.0f, -1.0f, 0.0f},
				vec3f{-1.0f, -1.0f,	0.0f},
				vec3f{-1.0f,  1.0f, 1.0f},
				vec3f{ 1.0f,  1.0f, 1.0f},
				vec3f{ 1.0f, -1.0f, 1.0f},
				vec3f{-1.0f, -1.0f, 1.0f}
			};

            // project frustum corners into world space
			for (auto & corner : frustumCorners) {
				const auto invCorner = cameraInverse * vec4f{corner, 1.0f};
				corner = invCorner / invCorner.w;
			}

            for (uint8_t j = 0u; j < 4u; ++j) {
				const auto dist = frustumCorners[j + 4u] - frustumCorners[j];
				frustumCorners[j + 4u] = frustumCorners[j] + (dist * splitDist);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
			}

            // get frustum center
			vec3f frustumCenter{0.0f};
			for (uint8_t j = 0u; j < 8u; ++j) { frustumCenter += frustumCorners[j]; }
			frustumCenter /= 8.0f;

            const float radius = glm::length(frustumCorners[7u] - frustumCenter);
            const mat4f lightViewMatrix = glm::lookAtRH(frustumCenter - _direction * radius, frustumCenter, vec3f{0.0f, 0.0f, 1.0f});
            mat4f lightOrthoMatrix = _reversedDepth ? 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 2.0f * radius, 0.0f) : 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
            lightOrthoMatrix[1u][1u] *= -1.0f;
			
            { // shimmering / aliasing fix
                const auto shadowMatrix = lightOrthoMatrix * lightViewMatrix;
                const auto shadowOrigin = vec2f(shadowMatrix[3u]) * static_cast<float>(_size) * 0.5f;

                // floor (pixel snapping)
                const vec2f roundedOrigin = glm::round(shadowOrigin);
                const vec2f roundOffset = (roundedOrigin - shadowOrigin) * (2.0f / static_cast<float>(_size));

                // adjust the projection matrix by the texel offset value
                lightOrthoMatrix[3u] += vec4f{roundOffset, 0.0f, 0.0f};
            }

            // store split distance and matrix in cascade
            _splitDepths[i] = (_near + splitDist * _clipRange) * -1.0f;
            _cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = _cascadeSplits[i];
        }
    }

    void CascadeShadowMap::updateCascades(const mat4f & viewInverse, float cameraFovY, float cameraAspect) {
        float lastSplitDist = _near;
        const float halfTanFov = glm::tan(cameraFovY * 0.5f);

        for (uint8_t i = 0u; i < _cascadesCount; ++i) {
            const float splitDist = _near + (_cascadeSplits[i] * _clipRange);

            // считаем размеры ближней и дальней плоскостей текущего каскада во View Space
            const float nearHeight = lastSplitDist * halfTanFov;
            const float nearWidth  = nearHeight * cameraAspect;
            const float farHeight  = splitDist * halfTanFov;
            const float farWidth   = farHeight * cameraAspect;

            // строим 8 вершин каскада во View Space (камера смотрит вдоль -Z)
            // для Reverse-Z и обычного Z во View Space координаты одинаковы, меняется только NDC.
            stl::array<vec3f, 8> frustumCorners = {
                // ближняя плоскость каскада (Z = -lastSplitDist)
                vec3f{-nearWidth,  nearHeight, -lastSplitDist},
                vec3f{ nearWidth,  nearHeight, -lastSplitDist},
                vec3f{ nearWidth, -nearHeight, -lastSplitDist},
                vec3f{-nearWidth, -nearHeight, -lastSplitDist},
                // дальняя плоскость каскада (Z = -splitDist)
                vec3f{-farWidth,   farHeight,  -splitDist},
                vec3f{ farWidth,   farHeight,  -splitDist},
                vec3f{ farWidth,  -farHeight,  -splitDist},
                vec3f{-farWidth,  -farHeight,  -splitDist}
            };

            // переводим вершины каскада из View Space в World Space
            for (auto & corner : frustumCorners) {
                const auto worldCorner = viewInverse * vec4f{corner, 1.0f};
                corner = vec3f{worldCorner} / worldCorner.w;
            }

            // находим центр усеченного конуса (Frustum Center) в мировом пространстве
            vec3f frustumCenter{0.0f};
            for (uint8_t j = 0u; j < 8u; ++j) { 
                frustumCenter += frustumCorners[j]; 
            }
            frustumCenter /= 8.0f;

            const float radius = glm::length(frustumCorners[7u] - frustumCenter);

            // строим матрицу вида для направленного света (Light View Matrix)
            const vec3f lightPos = frustumCenter - _direction * radius;
            const mat4f lightViewMatrix = glm::lookAtRH(lightPos, frustumCenter, vec3f{0.0f, 0.0f, 1.0f});

            mat4f lightOrthoMatrix = _reversedDepth ? 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 2.0f * radius, 0.0f) : 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
            lightOrthoMatrix[1u][1u] *= -1.0f; // инверсия Y для Vulkan/DirectX (если применимо)

            { // shimmering / aliasing fix
                const auto shadowMatrix = lightOrthoMatrix * lightViewMatrix;
                const auto shadowOrigin = vec2f(shadowMatrix[3u]) * static_cast<float>(_size) * 0.5f;

                // floor (pixel snapping)
                const vec2f roundedOrigin = glm::round(shadowOrigin);
                const vec2f roundOffset = (roundedOrigin - shadowOrigin) * (2.0f / static_cast<float>(_size));

                // adjust the projection matrix by the texel offset value
                lightOrthoMatrix[3u] += vec4f{roundOffset, 0.0f, 0.0f};
            }

            _splitDepths[i] = -splitDist; 
            _cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;
            _texelSizes[i] = (2.0f * radius) / _size;

            lastSplitDist = splitDist;
        }
    }

    void CascadeShadowMap::beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept {
        commandList.setViewport(0.0f, 0.0f, _size, _size);
        commandList.setScissor({{0, 0}, {_size, _size}});

        if constexpr (kUseVarianceShadowMap) {
            const auto clearValues = stl::array{
                rhi::ClearValue{_reversedDepth ? 0u: 0xff'ff'ff'ffu},
                rhi::ClearValue{_reversedDepth ? 0.0f : 1.0f, 0u},
            };
        
            commandList.beginRenderPass(_renderPass, _framebuffers[frameId].frameBuffers[0u], clearValues, {{0, 0}, {_size, _size}});
        } else {
            const auto clearValues = stl::array{
                rhi::ClearValue{_reversedDepth ? 0.0f : 1.0f, 0u}
            };
        
            commandList.beginRenderPass(_renderPass, _framebuffers[frameId].frameBuffers[0u], clearValues, {{0, 0}, {_size, _size}});
        }
    }

    void CascadeShadowMap::endRenderPass(rhi::runtime::CommandList & commandList) const noexcept {
        commandList.endRenderPass();
    }

    const rhi::runtime::ImagePtr & CascadeShadowMap::getImage(uint8_t frameId) const noexcept {
        return _framebuffers[frameId].image;
    }

    const rhi::runtime::ImagePtr & CascadeShadowMap::getDepthImage(uint8_t frameId) const noexcept {
        return _framebuffers[frameId].depthImage;
    }

    bool CascadeShadowMap::setDirection(const vec3f & direction, float eps) noexcept {
        if (compare(_direction, direction, eps)) {
            _direction = direction;
            return true;
        }
        return false;
    }
}