#include "imgui.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include <imgui.h>

#include <cstddef>

namespace engine {

    ImGuiSystem::ImGuiSystem() {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        io.IniFilename = nullptr; // disable creation "imgui.ini"
        io.LogFilename = nullptr;

        setupKeyMap();
    }

    ImGuiSystem::~ImGuiSystem() {
        destroy();
        ImGui::DestroyContext();
    }

    void ImGuiSystem::destroy() noexcept {
        _pipeline = nullptr;
        _stagefontBuffer = nullptr;
        _fontImage = nullptr;
        _vertices = nullptr;
        _indices = nullptr;
        _uniform = nullptr;
        _staging = nullptr;
    }

    void ImGuiSystem::createPipeline(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device, 
        const rhi::runtime::GpuProgramPtr & program, rhi::runtime::RenderPass renderPass,
        uint32_t subpass) {
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

        _pipeline = api->createGraphicsPipeline(device, renderPass, program, info, subpass);
    }

    void ImGuiSystem::createFontTexture(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device) {
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

    void ImGuiSystem::createBuffers(const std::unique_ptr<rhi::runtime::Rhi> & api,
        rhi::runtime::DevicePtr & device) {
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

        constexpr uint32_t kInitialVerticesCountMax = 1024u;
        _vertices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, sizeof(ImDrawVert) * kInitialVerticesCountMax
        );

        // indices
        const rhi::BufferInfo indicesInfo {
            rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
            rhi::MemoryPropertyFlags::eDeviceLocal
        };

        constexpr uint32_t kInitialIndicesCountMax = 1024u;
        _indices = std::make_unique<graphics::InflightBuffer>(
            api, device, indicesInfo, sizeof(ImDrawIdx) * kInitialIndicesCountMax
        );
    }

    void ImGuiSystem::beginFrame(float delta, uint16_t w, uint16_t h) noexcept {
        ImGuiIO &io = ImGui::GetIO();
        io.DeltaTime = delta;
        io.DisplaySize = ImVec2(w, h);
        ImGui::NewFrame();
    }

    void ImGuiSystem::endFrame() {
        ImGui::Render();
    }

    void ImGuiSystem::prepare(rhi::runtime::CommandListPtr & commandList, uint8_t frame,
        const std::unique_ptr<rhi::runtime::Rhi> & api, rhi::runtime::DevicePtr & device) {
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

        const ImDrawData *drawData = ImGui::GetDrawData();

        // resize buffers if haven't memory
        const auto vtxDataSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
        if (_vertices == nullptr ||  vtxDataSize >= _vertices->sizeInBytes()) {
            const rhi::BufferInfo verticesInfo {
                rhi::BufferUsageFlags::eVertex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
                rhi::MemoryPropertyFlags::eDeviceLocal
            };

            _vertices = std::make_unique<graphics::InflightBuffer>(
                api, device, verticesInfo, vtxDataSize * 2u
            );
        }

        const auto idxDataSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
        if (_indices == nullptr || idxDataSize >= _indices->sizeInBytes()) {
            const rhi::BufferInfo indicesInfo {
                rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
                rhi::MemoryPropertyFlags::eDeviceLocal
            };

            _indices = std::make_unique<graphics::InflightBuffer>(
                api, device, indicesInfo, idxDataSize * 2u
            );
        }

        if (!_vertices->isHostVisible() || !_indices->isHostVisible()) { // if we need to use a staging buffer
            if (_staging == nullptr || idxDataSize + vtxDataSize >= _staging->sizeInBytes()) {
                constexpr size_t kMinStagingSize = 100'000u /*100 Kb*/;
                _staging = std::make_unique<graphics::InflightStagingBuffer>(api, device, std::max((idxDataSize + vtxDataSize) * 2u, kMinStagingSize), 
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
            }
        }

        _vertices->setStagingBuffer(_staging.get());
        _indices->setStagingBuffer(_staging.get());

        _staging->map(frame);

        auto * verticesMem = _vertices->map2<std::byte>();
        auto * indicesMem = _indices->map2<std::byte>();

        uint32_t vOffset = 0u;
        uint32_t iOffset = 0u;

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

        constexpr float kEps = 1e-7f;
        const vec4f transform{scale.x, scale.y, translate.x, translate.y};
        if (compare(_transform, transform, kEps)) {
            _transform = transform;
            auto & uniformMem = *_uniform->map2<vec4f>();
            uniformMem = _transform;
            _uniform->unmap();
        }
        _uniform->commit(commandList, frame);

        _staging->unmap();
    }

    void ImGuiSystem::draw(rhi::runtime::CommandListPtr & commandList, uint8_t frame) noexcept {
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

    bool ImGuiSystem::onInputPointerEvent(const PointerEvent & event) {
        ImGuiIO &io = ImGui::GetIO();

        switch (event.state) {
            case InputEventState::ePress:
                io.AddMouseButtonEvent(static_cast<int>(event.button), true);
                break;
            case InputEventState::eRelease:
                io.AddMouseButtonEvent(static_cast<int>(event.button), false);
                break;
            default:
                break;
        }

        io.MousePos = ImVec2(event.x, io.DisplaySize.y - event.y);
        return io.WantCaptureMouse;
    }

    bool ImGuiSystem::onInputWheelEvent(float dx, float dy) {
        ImGuiIO &io = ImGui::GetIO();
        io.AddMouseWheelEvent(dx, dy);
        return io.WantCaptureMouse;
    }

    bool ImGuiSystem::onInpuKeyEvent(const KeyEvent & event, const InputKeyMod & mod) {
        ImGuiIO &io = ImGui::GetIO();
        io.KeyShift = mod.isShiftPressed();
        io.KeyAlt = mod.isAltPressed();
        io.KeySuper = mod.isSuperPressed();
        io.KeyCtrl = mod.isCtrlPressed();
        io.KeysDown[static_cast<uint8_t>(event.key)] = event.state == InputEventState::ePress;

        if (event.state == InputEventState::ePress || event.state == InputEventState::eRepeate) {
            const auto &[key, keyWithShift] = _keyMap[static_cast<uint8_t>(event.key)];
            io.AddInputCharacter(io.KeyShift ? keyWithShift : key);
        }

        return io.WantCaptureKeyboard;
    }

    void ImGuiSystem::setupKeyMap() {
        ImGuiIO &io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_LeftArrow] = static_cast<uint8_t>(KeyboardKey::eLeft);
        io.KeyMap[ImGuiKey_RightArrow] = static_cast<uint8_t>(KeyboardKey::eRight);
        io.KeyMap[ImGuiKey_UpArrow] = static_cast<uint8_t>(KeyboardKey::eUp);
        io.KeyMap[ImGuiKey_DownArrow] = static_cast<uint8_t>(KeyboardKey::eDown);
        io.KeyMap[ImGuiKey_Delete] = static_cast<uint8_t>(KeyboardKey::eDelete);
        io.KeyMap[ImGuiKey_Backspace] = static_cast<uint8_t>(KeyboardKey::eBackSpace);
        io.KeyMap[ImGuiKey_Space] = static_cast<uint8_t>(KeyboardKey::eSpace);
        io.KeyMap[ImGuiKey_Enter] = static_cast<uint8_t>(KeyboardKey::eEnter);
        io.KeyMap[ImGuiKey_Tab] = static_cast<uint8_t>(KeyboardKey::eTab);

        _keyMap[static_cast<uint8_t>(KeyboardKey::eBackSpace)] = {8u, 8u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eTab)] = {9u, 9u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eSpace)] = {32u, 32u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eLeft)] = {17u, 17u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eRight)] = {18u, 18u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eUp)] = {19u, 19u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eDown)] = {20u, 20u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eMinus)] = {45u, 95u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eSlash)] = {47u, 47u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e0)] = {48u, 41u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e1)] = {49u, 33u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e2)] = {50u, 64u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e3)] = {51u, 35u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e4)] = {52u, 36u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e5)] = {53u, 37u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e6)] = {54u, 94u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e7)] = {55u, 38u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e8)] = {56u, 42u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::e9)] = {57u, 40u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eEqual)] = {61u, 43u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eDelete)] = {127u, 127u};

        for (uint8_t k = static_cast<uint8_t>(KeyboardKey::eA); k <= static_cast<uint8_t>(KeyboardKey::eZ); ++k) {
            uint8_t i = k - static_cast<uint8_t>(KeyboardKey::eA);
            _keyMap[k] = {97u + i, 65u + i};
        }
    }

}