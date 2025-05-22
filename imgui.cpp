#include "imgui.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include "math/math.h"

#include <imgui.h>

#include <cstddef>

namespace engine {

    ImGuiRenderer::ImGuiRenderer() {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        io.IniFilename = nullptr; // disable creation "imgui.ini"
    }

    ImGuiRenderer::~ImGuiRenderer() {
        ImGui::DestroyContext();
    }

    void ImGuiRenderer::createPipeline(const std::unique_ptr<rhi::runtime::Rhi> & api,
        std::unique_ptr<rhi::runtime::Device> & device, 
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass) {
        rhi::GraphicsPipelineStatesInfo info;
        info.viewportState = {1u, 1u};
        info.blendModes = {rhi::common_blend_modes::blend_alpha};
        info.depthStencilState = {
            {
                .depthTestEnable = false,
                .depthWriteEnable = false,
                .depthCompareOp = rhi::CompareOp::eNever
            }, 
            {}
        };
        info.dynamicStates = rhi::DynamicState::eViewport | rhi::DynamicState::eScissor;
        info.rasterisationState = {rhi::PolygonMode::eFill, rhi::CullMode::eNone};
        info.inputAssemblyState = {rhi::PrimitiveTopology::eTriangles, false};

        info.vertexInputState = {
            std::vector{rhi::VertexInputBinding::create<ImDrawVert>(0u)},
            std::vector{
                rhi::VertexInputAttribute{ // pos
                    .location = 0u,
                    .binding = 0u,
                    .format = rhi::Format::eR32G32_Sfloat,
                    .offset = 0u
                },
                rhi::VertexInputAttribute{ // uv
                    .location = 1u,
                    .binding = 0u,
                    .format = rhi::Format::eR32G32_Sfloat,
                    .offset = sizeof(ImVec2)
                },
                rhi::VertexInputAttribute{ // col
                    .location = 2u,
                    .binding = 0u,
                    .format = rhi::Format::eR8G8B8A8_Unorm,
                    .offset = 2u * sizeof(ImVec2)
                }
            }
        };

        _pipeline = api->createGraphicsPipeline(device, renderPass, program, info);
    }

    void ImGuiRenderer::createFontTexture(const std::unique_ptr<rhi::runtime::Rhi> & api,
        std::unique_ptr<rhi::runtime::Device> & device) {
        ImGuiIO &io = ImGui::GetIO();

        unsigned char *pixels = nullptr;
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

        const rhi::BufferInfo stageInfo {
            rhi::BufferUsageFlags::eTransfer_Src,
            rhi::MemoryPropertyFlags::eHostVisible
        };

        _stagefontBuffer = api->createBuffer(device, stageInfo, sizeof(unsigned char) * width * height);
        {
            auto * map = _stagefontBuffer->map2<unsigned char>();
            memcpy(map, pixels, _stagefontBuffer->sizeInBytes());
            _stagefontBuffer->flush();
            _stagefontBuffer->unmap();
        }

        const auto imageInfo = rhi::ImageInfo{
            rhi::ImageUsageFlags::eSampled | rhi::ImageUsageFlags::eTransfer_Dst,
            rhi::Format::eR8_Unorm, {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1u}
        };

        _fontImage = api->createImage(device, imageInfo);

        _sampler.magFilter = rhi::FilterMode::eLinear;
        _sampler.minFilter = rhi::FilterMode::eLinear;
        _sampler.addressModeU = rhi::SamplerAddressMode::eClamp_To_Edge;
        _sampler.addressModeV = rhi::SamplerAddressMode::eClamp_To_Edge;
        _sampler.addressModeW = rhi::SamplerAddressMode::eClamp_To_Edge;

        io.Fonts->SetTexID(&_fontImage);
    }

    void ImGuiRenderer::createBuffers(const std::unique_ptr<rhi::runtime::Rhi> & api,
        std::unique_ptr<rhi::runtime::Device> & device) {
        // uniforms
        rhi::BufferInfo unifromsInfo {
            rhi::BufferUsageFlags::eUniform_Buffer,
            rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached // use it for imgui?
        };

        _uniform = std::make_unique<graphics::InflightBuffer>(
            api, device, unifromsInfo, sizeof(vec4f)
        );

        // vertices
        const rhi::BufferInfo verticesInfo {
            rhi::BufferUsageFlags::eVertex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        constexpr uint32_t kVerticesCountMax = 8192u;
        _vertices =  std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, sizeof(ImDrawVert) * kVerticesCountMax
        );

        // indices
        const rhi::BufferInfo indicesInfo {
            rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        constexpr uint32_t kIndicesCountMax = 8192u;
        _indices =  std::make_unique<graphics::InflightBuffer>(
            api, device, indicesInfo, sizeof(ImDrawIdx) * kIndicesCountMax
        );
    }

    void ImGuiRenderer::beginFrame(float delta, uint16_t w, uint16_t h) noexcept {
        ImGuiIO &io = ImGui::GetIO();
        io.DeltaTime = (delta > 0.0f) ? delta : (1.0f / 60.0f);
        io.DisplaySize = ImVec2(w, h);
        ImGui::NewFrame();
    }

    void ImGuiRenderer::endFrame() {
        ImGui::Render();
    }

    void ImGuiRenderer::prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame) {
        if (_stagefontBuffer) {
            commandList->copyBuffer2Image(_stagefontBuffer, _fontImage, {
                .bufferOffset = 0u,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _fontImage->getExtent()
            });
            _stagefontBuffer = nullptr;
        }

        // todo: need resize buffers if haven't memory!
        auto * verticesMem = _vertices->map2<std::byte>();
        auto * indicesMem = _indices->map2<std::byte>();

        uint32_t vOffset = 0u;
        uint32_t iOffset = 0u;

        const ImDrawData *drawData = ImGui::GetDrawData();
        for (uint32_t i = 0u; i < drawData->CmdListsCount; ++i) {
            const ImDrawList *drawList = drawData->CmdLists[i];
            const uint32_t vertexBufferSize = drawList->VtxBuffer.size() * sizeof(ImDrawVert);
            const uint32_t indexBufferSize = drawList->IdxBuffer.size() * sizeof(ImDrawIdx);

            memcpy(verticesMem + vOffset, drawList->VtxBuffer.Data, vertexBufferSize);
            memcpy(indicesMem + iOffset, drawList->IdxBuffer.Data, indexBufferSize);
            vOffset += vertexBufferSize;
            iOffset += indexBufferSize;
        }

        _vertices->unmap();
        _vertices->commit(commandList, frame);

        _indices->unmap();
        _indices->commit(commandList, frame);

        const vec2f scale{2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y};
        const vec2f translate{(drawData->DisplayPos.x - drawData->DisplaySize.x * 0.5f) * scale.x,
                              (drawData->DisplayPos.y - drawData->DisplaySize.y * 0.5f) * scale.y};

        auto * uniformMem = _uniform->map2<vec4f>();
        uniformMem->x = scale.x; uniformMem->y = scale.y;
        uniformMem->z = translate.x; uniformMem->w = translate.y;
        _uniform->unmap();
        _uniform->commit(commandList, frame);
    }

    void ImGuiRenderer::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept {
        const ImDrawData *drawData = ImGui::GetDrawData();
        
        commandList->bindPipeline(_pipeline);
        commandList->bindBuffer(0u, (*_uniform)(frame)); // bind uniform

        uint32_t vOffset = 0u;
        uint32_t iOffset = 0u;
        for (uint32_t i = 0u; i < drawData->CmdListsCount; ++i) {
            const ImDrawList *drawList = drawData->CmdLists[i];
            const uint32_t vertexBufferSize = drawList->VtxBuffer.size() * sizeof(ImDrawVert);
            const uint32_t indexBufferSize = drawList->IdxBuffer.size() * sizeof(ImDrawIdx);

            uint32_t indexBufferOffset = 0u;
            const uint32_t firstIndex = iOffset / sizeof(ImDrawIdx);

            for (uint32_t j = 0u; j < drawList->CmdBuffer.Size; ++j) {
                ImDrawCmd const &cmd = drawList->CmdBuffer[j];

                commandList->setScissor({
                    {static_cast<int32_t>(cmd.ClipRect.x), static_cast<int32_t>(cmd.ClipRect.y)}, 
                    {static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x), static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y)}
                    }
                );

                auto & texture = *static_cast<rhi::runtime::ImagePtr *>(cmd.TextureId);
                commandList->bindImage(1u, texture, _sampler);

                commandList->bindVertexBuffer(0u, (*_vertices)(frame), vOffset);
                commandList->bindIndexBuffer((*_indices)(frame), indexBufferOffset, rhi::IndexType::eU16);

                commandList->drawIndexed(static_cast<uint32_t>(cmd.ElemCount), 1u, firstIndex);

                indexBufferOffset += cmd.ElemCount * sizeof(ImDrawIdx);
            }

            vOffset += vertexBufferSize;
            iOffset += indexBufferSize;
        }
    }

}