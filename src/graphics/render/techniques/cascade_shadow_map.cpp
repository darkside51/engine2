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
            _texelSizes(cascades),
            _clipRange(std::abs(far - near)),
            _reversedDepth(near > far),
            _near(_reversedDepth ? far : near),
            _far(_reversedDepth ? near : far),
            _size(size),
            _cascadesCount(cascades),
            _bits(bits) {
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
                    .layout = ImageLayout::eDepth_Stencil_Attachment_Optimal, //eDepth_Stencil_Attachment_Optimal, // eDepth_Attachment_Optimal - maybe?
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
                .image = _api.createImage(_device, imageInfo),
                .frameBuffers = stl::vector<runtime::FrameBufferPtr>(kUseInstanceDrawShadows ? 1u : _cascadesCount)
            };

            if constexpr (kUseInstanceDrawShadows) {
                //create only 1 framebuffer if draw with instancing
                stl::vector fbAttachemnts = {
                    FrameBufferAttachment{
                        .image = framebuffer.image.get(),
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
                            .image = framebuffer.image.get(),
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
            .mipMapMode = FilterMode::eNearest,
            .addressModeU = SamplerAddressMode::eClamp_To_Border,
            .addressModeV = SamplerAddressMode::eClamp_To_Border,
            .addressModeW = SamplerAddressMode::eClamp_To_Border,
            .compareOp = _reversedDepth ? CompareOp::eGequal : CompareOp::eLess,
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
		for (uint32_t i = 0u; i < _cascadesCount; i++) {
			const float p = (i + 1u) / static_cast<float>(_cascadesCount);
			const float log = minZ * std::pow(ratio, p);
			const float uniform = minZ + range * p;
			const float d = _cascadeSplitLambda * (log - uniform) + uniform;
			_cascadeSplits[i] = (d - _near) / _clipRange;
		}
    }

    // void CascadeShadowMap::updateCascades(const mat4f & cameraInverse) {
    // cameraInverse — это матрица из инвертированная полная матрица камеры glm::inverse(camera.getTransform()); 
    //     // calculate orthographic projection matrix for each cascade
	// 	float lastSplitDist = 0.0f;
	// 	for (uint8_t i = 0u; i < _cascadesCount; ++i) {
    //         const float splitDist = _cascadeSplits[i];

    //         auto frustumCorners = _reversedDepth ? stl::array{
	// 			vec3f{-1.0f,  1.0f, 1.0f},
	// 			vec3f{ 1.0f,  1.0f, 1.0f},
	// 			vec3f{ 1.0f, -1.0f, 1.0f},
	// 			vec3f{-1.0f, -1.0f,	1.0f},
	// 			vec3f{-1.0f,  1.0f, 0.0f},
	// 			vec3f{ 1.0f,  1.0f, 0.0f},
	// 			vec3f{ 1.0f, -1.0f, 0.0f},
	// 			vec3f{-1.0f, -1.0f, 0.0f}
	// 		} :
    //         stl::array{
	// 			vec3f{-1.0f,  1.0f, 0.0f},
	// 			vec3f{ 1.0f,  1.0f, 0.0f},
	// 			vec3f{ 1.0f, -1.0f, 0.0f},
	// 			vec3f{-1.0f, -1.0f,	0.0f},
	// 			vec3f{-1.0f,  1.0f, 1.0f},
	// 			vec3f{ 1.0f,  1.0f, 1.0f},
	// 			vec3f{ 1.0f, -1.0f, 1.0f},
	// 			vec3f{-1.0f, -1.0f, 1.0f}
	// 		};

    //         // project frustum corners into world space
	// 		for (auto & corner : frustumCorners) {
	// 			const auto invCorner = cameraInverse * vec4f{corner, 1.0f};
	// 			corner = invCorner / invCorner.w;
	// 		}

    //         for (uint8_t j = 0u; j < 4u; ++j) {
	// 			const auto dist = frustumCorners[j + 4u] - frustumCorners[j];
	// 			frustumCorners[j + 4u] = frustumCorners[j] + (dist * splitDist);
	// 			frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
	// 		}

    //         // get frustum center
	// 		vec3f frustumCenter{0.0f};
	// 		for (uint8_t j = 0u; j < 8u; ++j) { frustumCenter += frustumCorners[j]; }
	// 		frustumCenter /= 8.0f;

    //         const float radius = glm::length(frustumCorners[7u] - frustumCenter);
    //         const mat4f lightViewMatrix = glm::lookAtRH(frustumCenter - _direction * radius, frustumCenter, vec3f{0.0f, 1.0f, 0.0f});
    //         mat4f lightOrthoMatrix = _reversedDepth ? 
    //             glm::orthoRH_ZO(-radius, radius, -radius, radius, 2.0f * radius, 0.0f) : 
    //             glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
    //         lightOrthoMatrix[1u][1u] *= -1.0f;
			
    //         { // shimmering / aliasing fix
    //             const auto shadowMatrix = lightOrthoMatrix * lightViewMatrix;
    //             const auto shadowOrigin = vec2f(shadowMatrix[3u]) * static_cast<float>(_size) * 0.5f;

    //             // floor (pixel snapping)
    //             const vec2f roundedOrigin = glm::round(shadowOrigin);
    //             const vec2f roundOffset = (roundedOrigin - shadowOrigin) * (2.0f / static_cast<float>(_size));

    //             // adjust the projection matrix by the texel offset value
    //             lightOrthoMatrix[3u] += vec4f{roundOffset, 0.0f, 0.0f};
    //         }

    //         // store split distance and matrix in cascade
    //         _splitDepths[i] = (_near + splitDist * _clipRange) * -1.0f;
    //         _cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;

    //         lastSplitDist = _cascadeSplits[i];
    //     }
    // }

    void CascadeShadowMap::updateCascades(const mat4f & cameraInverse) {
        // cameraInverse — это матрица из View Space в World Space (обычно glm::inverse(viewMatrix))
        
        float cameraFovY = 3.1415926f / 3.0f;
        float cameraAspect = 16.0f / 9.0f;
        float lastSplitDist = _near;
        const float halfTanFov = glm::tan(cameraFovY * 0.5f);

        for (uint8_t i = 0u; i < _cascadesCount; ++i) {
            // Если _cascadeSplits[i] это коэффициент от 0 до 1, переводим в метры.
            // Если это уже метры, то просто: float splitDist = _cascadeSplits[i];
            const float splitDist = _near + (_cascadeSplits[i] * _clipRange);

            // 1. Считаем размеры ближней и дальней плоскостей текущего каскада во View Space
            const float nearHeight = lastSplitDist * halfTanFov;
            const float nearWidth  = nearHeight * cameraAspect;
            const float farHeight  = splitDist * halfTanFov;
            const float farWidth   = farHeight * cameraAspect;

            // 2. Строим 8 вершин каскада во View Space (камера смотрит вдоль -Z)
            // Для Reverse-Z и обычного Z во View Space координаты одинаковы, меняется только NDC.
            stl::array<vec3f, 8> frustumCorners = {
                // Ближняя плоскость каскада (Z = -lastSplitDist)
                vec3f{-nearWidth,  nearHeight, -lastSplitDist},
                vec3f{ nearWidth,  nearHeight, -lastSplitDist},
                vec3f{ nearWidth, -nearHeight, -lastSplitDist},
                vec3f{-nearWidth, -nearHeight, -lastSplitDist},
                // Дальняя плоскость каскада (Z = -splitDist)
                vec3f{-farWidth,   farHeight,  -splitDist},
                vec3f{ farWidth,   farHeight,  -splitDist},
                vec3f{ farWidth,  -farHeight,  -splitDist},
                vec3f{-farWidth,  -farHeight,  -splitDist}
            };

            // 3. Переводим вершины каскада из View Space в World Space
            for (auto & corner : frustumCorners) {
                const auto worldCorner = cameraInverse * vec4f{corner, 1.0f};
                corner = vec3f{worldCorner} / worldCorner.w;
            }

            // 4. Находим центр усеченного конуса (Frustum Center) в мировом пространстве
            vec3f frustumCenter{0.0f};
            for (uint8_t j = 0u; j < 8u; ++j) { 
                frustumCenter += frustumCorners[j]; 
            }
            frustumCenter /= 8.0f;

            // 5. Считаем радиус ограничивающей сферы (Bounding Sphere)
            // Идеальная сфера гарантирует, что при вращении камеры размер каскада не изменится
            float radius = 0.0f;
            for (uint8_t j = 0u; j < 8u; ++j) {
                radius = glm::max(radius, glm::length(frustumCorners[j] - frustumCenter));
            }

            // 6. Строим матрицу вида для направленного света (Light View Matrix)
            const vec3f lightPos = frustumCenter - _direction * radius;
            const mat4f lightViewMatrix = glm::lookAtRH(lightPos, frustumCenter, vec3f{0.0f, 1.0f, 0.0f});

            // 2. Строим исходную ортографическую проекцию
            mat4f lightOrthoMatrix = _reversedDepth ? 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 2.0f * radius, 0.0f) : 
                glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);

            lightOrthoMatrix[1u][1u] *= -1.0f; // Инверсия Y для Vulkan/DirectX (если применимо)

            { // shimmering / aliasing fix
                const auto shadowMatrix = lightOrthoMatrix * lightViewMatrix;
                const auto shadowOrigin = vec2f(shadowMatrix[3u]) * static_cast<float>(_size) * 0.5f;

                // floor (pixel snapping)
                const vec2f roundedOrigin = glm::round(shadowOrigin);
                const vec2f roundOffset = (roundedOrigin - shadowOrigin) * (2.0f / static_cast<float>(_size));

                // adjust the projection matrix by the texel offset value
                lightOrthoMatrix[3u] += vec4f{roundOffset, 0.0f, 0.0f};
            }

            // Сохраняем дистанцию сплита для шейдера (в честных метрах или в NDC, зависит от вашего шейдера)
            // Обычно для шейдера удобнее хранить её в View Space (отрицательной, если камера смотрит в -Z)
            _splitDepths[i] = -splitDist; 
            //_splitDepths[i] = (_near + _cascadeSplits[i] * _clipRange) * -1.0f;
            _cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;
            _texelSizes[i] = (2.0f * radius) / _size;

            lastSplitDist = splitDist;
        }
    }

// void CascadeShadowMap::updateCascades(const mat4f & cameraInverse) {
//     float cameraFovY = 3.1415926f / 3.0f;
//     float cameraAspect = 16.0f / 9.0f;
//     float lastSplitDist = _near;
//     const float halfTanFov = glm::tan(cameraFovY * 0.5f);

//     for (uint8_t i = 0u; i < _cascadesCount; ++i) {
//         const float splitDist = _near + (_cascadeSplits[i] * _clipRange);

//         // 1. АНАЛИТИЧЕСКИЙ РАСЧЕТ ЦЕНТРА СФЕРЫ ВО VIEW SPACE
//         // Центр идеальной ограничивающей сферы усеченного конуса лежит на оси Z
//         float k = halfTanFov * cameraAspect;
//         float num = (splitDist * splitDist - lastSplitDist * lastSplitDist) * (1.0f + halfTanFov * halfTanFov + k * k);
//         float den = 2.0f * (splitDist - lastSplitDist);
//         // Дистанция до центра сферы от глаза камеры вдоль оси -Z
//         float centerZ = lastSplitDist + num / den; 
        
//         vec3f viewSpaceCenter{0.0f, 0.0f, -centerZ};

//         // 2. АНАЛИТИЧЕСКИЙ РАСЧЕТ ИДЕАЛЬНОГО РАДИУСА СФЕРЫ
//         // Считаем расстояние от центра до дальнего угла каскада во View Space
//         float farHeight = splitDist * halfTanFov;
//         float farWidth  = farHeight * cameraAspect;
//         vec3f farCornerView{farWidth, farHeight, -splitDist};
        
//         // Этот радиус ТЕПЕРЬ АБСОЛЮТНО СТАТИЧЕН при движении и вращении камеры!
//         float radius = glm::length(farCornerView - viewSpaceCenter);

//         // 3. ПЕРЕВОД ЦЕНТРА СФЕРЫ В WORLD SPACE
//         // cameraInverse — это чистая viewMatrix камеры в мировое пространство (без проекции)
//         vec4f worldSpaceCenter = cameraInverse * vec4f{viewSpaceCenter, 1.0f};
//         vec3f frustumCenter = vec3f{worldSpaceCenter} / worldSpaceCenter.w;

//         // 4. СТРОИМ МАТРИЦУ СВЕТА (LIGHT VIEW)
//         const vec3f lightPos = frustumCenter - _direction * radius;
//         mat4f lightViewMatrix = glm::lookAtRH(lightPos, frustumCenter, vec3f{0.0f, 1.0f, 0.0f});

//         // 5. СТРОИМ ОРТОГРАФИЧЕСКУЮ ПРОЕКЦИЮ СВЕТА
//         mat4f lightOrthoMatrix = _reversedDepth ? 
//             glm::orthoRH_ZO(-radius, radius, -radius, radius, 2.0f * radius, 0.0f) : 
//             glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);

//         lightOrthoMatrix[1u][1u] *= -1.0f; // Инверсия Y для Vulkan/DirectX (если нужно)

//         { // ЖЕЛЕЗОБЕТОННЫЙ TEXEL SNAPPING (БЕЗ ИСКАЖЕНИЯ Z)
//             const mat4f shadowMatrix = lightOrthoMatrix * lightViewMatrix;
            
//             // Проецируем центр мира (0,0,0) в NDC тени и переводим в масштаб текселей
//             vec4 shadowOrigin = shadowMatrix * vec4f{0.0f, 0.0f, 0.0f, 1.0f};
//             shadowOrigin *= static_cast<float>(_size) * 0.5f;

//             // Округляем до пиксельной сетки карты теней
//             const vec2f roundedOrigin = glm::round(vec2f(shadowOrigin));
//             const vec2f roundOffset = (roundedOrigin - vec2f(shadowOrigin)) * (2.0f / static_cast<float>(_size));

//             // Сдвигаем строго XY сетку NDC, не трогая строки Z
//             mat4f texelOffsetMat = mat4f(1.0f);
//             texelOffsetMat[3u][0u] = roundOffset.x;
//             texelOffsetMat[3u][1u] = roundOffset.y;

//             // Итоговая матрица каскада
//             _cascadeMatrices[i] = texelOffsetMat * shadowMatrix;
//         }

//         _splitDepths[i] = -splitDist;
//         _texelSizes[i] = (2.0f * radius) / _size;
//         lastSplitDist = splitDist;
//     }
// }


    void CascadeShadowMap::beginRenderPass(rhi::runtime::CommandList & commandList, uint8_t frameId) noexcept {
        const auto clearValues = stl::array{
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