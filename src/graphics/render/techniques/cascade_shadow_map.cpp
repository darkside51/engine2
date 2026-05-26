#include "cascade_shadow_map.h"
#include "graphics/render/rhi/common/command_list.h"

namespace engine::graphics {
    constexpr bool kUseInstanceDrawShadows = true;

    CascadeShadowMap::CascadeShadowMap(rhi::runtime::Rhi & api, rhi::runtime::Device & device, 
            uint16_t size, uint8_t cascades, uint8_t bits, float near, float far) : 
            _api(api),
            _device(device),
            _cascadeMatrices(cascades),
            _splitDepths(cascades),
            _cascadeSplits(cascades),
            _clipRange(std::abs(far - near)),
            _reversedDepth(near > far),
            _near(_reversedDepth ? far : near),
            _far(_reversedDepth ? near : far),
            _size(size),
            _cascadesCount(cascades),
            _bits(bits) {
        initCascadeSplits();
    }

    void CascadeShadowMap::createRenderData() {
        using namespace rhi;
        const auto format = _device.getSupportedDepthFormat(_bits, false, true);

        // renderpass
        std::array attachment = {
            AttachmentDescription {
                .format = format,
                .loadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eClear, AttachmentStoreOp::eStore},
                .stencilLoadStoreOp = AttachmentLoadStoreOp{AttachmentLoadOp::eDont_Care, AttachmentStoreOp::eDont_Care},
                .initialLayout = ImageLayout::eUndefined,
                .finalLayout = ImageLayout::eDepth_Stencil_Read_Only_Optimal,
                .samples = 1u
            }
        };

        std::array subpass  = {
            SubpassDescription {
                .pNext = {nullptr, 0u},
                .depthStencilAttachment = {
                    .pNext = {nullptr, 0u},
                    .attachment = 0u,
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal, //eDepth_Stencil_Attachment_Optimal, // eDepth_Attachment_Optimal - maybe?
                    .aspectMask = ImageAspectFlags::eDepthStencil
                }
            }
        };

        std::array<SubpassDependency, 2u> dependency;
        dependency[0u] = {
            .stages = {
                .src = {
                    .stage = PipelineStageFlags::eFragment_Shader,
                    .accessMask = AccessFlags::eShader_Read
                },
                .dst = {
                    .stage = PipelineStageFlags::eEarly_Fragment_Tests,
                    .accessMask = AccessFlags::eDepth_Stencil_Attachment_Write
                }
            }
        };

        dependency[1u] = {
            .stages = {
                .src = {
                    .stage = PipelineStageFlags::eLate_Fragment_Tests,
                    .accessMask = AccessFlags::eDepth_Stencil_Attachment_Write
                },
                .dst = {
                    .stage = PipelineStageFlags::eFragment_Shader,
                    .accessMask = AccessFlags::eShader_Read
                }
            }
        };

        const RenderPassDescription description{attachment, subpass, dependency};
        _renderPass = _api.getRenderPass(_device, description);

        // image
        const auto imageInfo = ImageInfo {
            ImageUsageFlags::eSampled | ImageUsageFlags::eDepth_Stencil_Attachment, format, {_size, _size, 1u},
            ImageAspectFlags::eDepthStencil,
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
                .image = _api.createImage(_device, imageInfo),
                .frameBuffers = std::vector<runtime::FrameBufferPtr>(kUseInstanceDrawShadows ? 1u : _cascadesCount)
            };

            if constexpr (kUseInstanceDrawShadows) {
                //create only 1 framebuffer if draw with instancing
                std::vector fbAttachemnts = {
                    FrameBufferAttachment{
                        .image = framebuffer.image.get(),
                        .imageSize = {_size, _size},
                        .aspect = ImageAspectFlags::eDepth,
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
                    std::vector fbAttachemnts = {
                        FrameBufferAttachment{
                            .image = framebuffer.image.get(),
                            .imageSize = {_size, _size},
                            .aspect = ImageAspectFlags::eDepth,
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
            .mipMapMode = FilterMode::eLinear,
            .addressModeU = SamplerAddressMode::eClamp_To_Border,
            .addressModeV = SamplerAddressMode::eClamp_To_Border,
            .addressModeW = SamplerAddressMode::eClamp_To_Border,
            .compareOp = _reversedDepth ? CompareOp::eGequal : CompareOp::eLess,
            .borderColor = BorderColor::eFloat_Opaque_Black,
            .maxAnisotropy = 0.0f
        };
    }

    void CascadeShadowMap::prepare() {
        _device.evaluateSampler(_sampler);
    }

    void CascadeShadowMap::initCascadeSplits() noexcept {
        const float minZ = _near + 1.0f;
		const float maxZ = _far - 20.0f;

		const float range = maxZ - minZ;
		const float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0u; i < _cascadesCount; i++) {
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

            auto frustumCorners = _reversedDepth ? std::array{
				vec3f{-1.0f,  1.0f, 1.0f},
				vec3f{ 1.0f,  1.0f, 1.0f},
				vec3f{ 1.0f, -1.0f, 1.0f},
				vec3f{-1.0f, -1.0f,	1.0f},
				vec3f{-1.0f,  1.0f, 0.0f},
				vec3f{ 1.0f,  1.0f, 0.0f},
				vec3f{ 1.0f, -1.0f, 0.0f},
				vec3f{-1.0f, -1.0f, 0.0f}
			} :
            std::array{
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

            float radius = 0.0f;
			for (uint8_t j = 0u; j < 8u; ++j) {
				const float distance = glm::length2(frustumCorners[j] - frustumCenter);
				radius = std::max(radius, distance);
			}
            // float radius = glm::length(frustumCorners[7u] - frustumCenter);
            radius = std::ceil(std::sqrt(radius) * 16.0f) / 16.0f;

            const mat4f lightViewMatrix = glm::lookAtRH(frustumCenter - _direction * radius, frustumCenter, vec3f{0.0f, 1.0f, 0.0f});
            const mat4f lightOrthoMatrix = _reversedDepth ? 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 2.0f * radius, 0.0f) : 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
			
            // store split distance and matrix in cascade
            //const float sign = _reversedDepth ? -1.0f : 1.0f;
            _splitDepths[i] = (_near + /*sign **/ splitDist * _clipRange) * -1.0f;
			_cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = _cascadeSplits[i];
        }
    }

    void CascadeShadowMap::beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept {
        const auto clearValues = std::array{
            rhi::ClearValue{_reversedDepth ? 0.0f : 1.0f, 0u},
        };
        commandList.setViewport(0.0f, 0.0f, _size, _size);
        commandList.setScissor({{0, 0}, {_size, _size}});
        commandList.beginRenderPass(_renderPass, _framebuffers[frameId].frameBuffers[0u], clearValues, {{0, 0}, {_size, _size}});
    }

    void CascadeShadowMap::endRenderPass(rhi::runtime::CommandList & commandList) const noexcept {
        commandList.endRenderPass();
    }

    const rhi::runtime::ImagePtr & CascadeShadowMap::getImage(uint8_t frameId) const noexcept {
        return _framebuffers[frameId].image;
    }

    bool CascadeShadowMap::setDirection(const vec3f & direction, float eps) noexcept {
        if (compare(_direction, direction, eps)) {
            _direction = direction;
            return true;
        }
        return false;
    }
}