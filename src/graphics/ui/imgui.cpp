#include "imgui.h"

#include "graphics/render/rhi/common/command_list.h"
#include "graphics/render/rhi/common/pipeline.h"

#include <imgui.h>

#include <cstddef>

namespace engine {

    const rhi::BufferInfo verticesInfo {
        rhi::BufferUsageFlags::eVertex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
        rhi::MemoryPropertyFlags::eDeviceLocal
    };

    const rhi::BufferInfo indicesInfo {
        rhi::BufferUsageFlags::eIndex_Buffer | rhi::BufferUsageFlags::eTransfer_Dst,
        rhi::MemoryPropertyFlags::eDeviceLocal
    };

    ImGuiSystem::ImGuiSystem() {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        io.IniFilename = nullptr; // disable creation "imgui.ini"
        io.LogFilename = nullptr;

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        setupKeyMap();
    }

    ImGuiSystem::~ImGuiSystem() {
        destroy();
        ImGui::DestroyContext();
    }

    void ImGuiSystem::destroy() noexcept {
        _pipeline = nullptr;
        _fontStageBuffer = nullptr;
        _fontImage = nullptr;
        _vertices = nullptr;
        _indices = nullptr;
        _uniform = nullptr;
        _staging = nullptr;
    }

    void ImGuiSystem::createPipeline(rhi::runtime::Rhi & api,
        rhi::runtime::Device & device, 
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
            stl::vector{rhi::VertexInputBinding::create<ImDrawVert>(0u)},
            stl::vector{
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

        _pipeline = api.createGraphicsPipeline(device, renderPass, program, info, subpass);
    }

    void ImGuiSystem::createFontTexture(rhi::runtime::Rhi & api,
        rhi::runtime::Device & device) {
        ImGuiIO &io = ImGui::GetIO();

        unsigned char *pixels = nullptr;
        int width = 0;
        int height = 0;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

        const rhi::BufferInfo stageInfo {
            rhi::BufferUsageFlags::eTransfer_Src,
            rhi::MemoryPropertyFlags::eHostVisible
        };

        _fontStageBuffer = api.createBuffer(device, stageInfo, sizeof(unsigned char) * width * height);
        {
            auto * memory = _fontStageBuffer->map2<unsigned char>();
            memcpy(memory, pixels, _fontStageBuffer->sizeInBytes());
            _fontStageBuffer->flush();
            _fontStageBuffer->unmap();
        }

        const auto imageInfo = rhi::ImageInfo{
            rhi::ImageUsageFlags::eSampled | rhi::ImageUsageFlags::eTransfer_Dst,
            rhi::Format::eR8_Unorm, {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1u}
        };

        _fontImage = api.createImage(device, imageInfo);

        _sampler.info.magFilter = rhi::FilterMode::eLinear;
        _sampler.info.minFilter = rhi::FilterMode::eLinear;
        _sampler.info.addressModeU = rhi::SamplerAddressMode::eClamp_To_Edge;
        _sampler.info.addressModeV = rhi::SamplerAddressMode::eClamp_To_Edge;
        _sampler.info.addressModeW = rhi::SamplerAddressMode::eClamp_To_Edge;

        device.evaluateSampler(_sampler);

        io.Fonts->SetTexID(&_fontImage);
    }

    void ImGuiSystem::createBuffers(rhi::runtime::Rhi & api,
        rhi::runtime::Device & device) {
        // uniforms
        rhi::BufferInfo unifromsInfo {
            rhi::BufferUsageFlags::eUniform_Buffer,
            rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached // use it for imgui?
        };

        _uniform = std::make_unique<graphics::InflightBuffer>(
            api, device, unifromsInfo, sizeof(vec4f)
        );

        // vertices
        constexpr uint32_t kInitialVerticesCountMax = 1024u;
        _vertices = std::make_unique<graphics::InflightBuffer>(
            api, device, verticesInfo, sizeof(ImDrawVert) * kInitialVerticesCountMax
        );

        // indices
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

        // for docking
        const ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpaceOverViewport(0u, ImGui::GetMainViewport(), dockFlags);
    }

    void ImGuiSystem::endFrame() {
        ImGui::Render();
    }

    void ImGuiSystem::prepare(rhi::runtime::CommandList & commandList, uint8_t frame,
        rhi::runtime::Rhi & api, rhi::runtime::Device & device) {
        if (_fontStageBuffer) {
            commandList.copyBuffer2Image(_fontStageBuffer, _fontImage, {
                .bufferOffset = 0u,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {.aspectMask = engine::rhi::ImageAspectFlags::eColor},
                .imageOffset = {0u, 0u, 0u},
                .imageExtent = _fontImage->getExtent()
            });
            _fontStageBuffer = nullptr;
        }

        const ImDrawData *drawData = ImGui::GetDrawData();

        // resize buffers if haven't memory
        const auto vtxDataSize = sizeof(ImDrawVert) * drawData->TotalVtxCount;
        if (vtxDataSize >= _vertices->sizeInBytes()) {
            _vertices->resize(vtxDataSize * 2u);
        }

        const auto idxDataSize = sizeof(ImDrawIdx) * drawData->TotalIdxCount;
        if (idxDataSize >= _indices->sizeInBytes()) {
            _indices->resize(idxDataSize * 2u);
        }

        const bool useStagingBuffer = !_vertices->isHostVisible() || !_indices->isHostVisible(); // we need to use a staging buffer

        if (useStagingBuffer) { 
            if (_staging == nullptr || idxDataSize + vtxDataSize >= _staging->sizeInBytes()) {
                constexpr size_t kMinStagingSize = 100'000u /*100 Kb*/;
                _staging = std::make_unique<graphics::InflightStagingBuffer>(api, device, std::max((idxDataSize + vtxDataSize) * 2u, kMinStagingSize), 
                    rhi::MemoryPropertyFlags::eHostVisible | rhi::MemoryPropertyFlags::eHostCached);
            }

            _vertices->setStagingBuffer(_staging.get());
            _indices->setStagingBuffer(_staging.get());

            _staging->map(frame);
        }

        auto * verticesMem = _vertices->memory2<std::byte>();
        auto * indicesMem = _indices->memory2<std::byte>();

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

        _vertices->modify();
        _vertices->commit(commandList, frame);

        _indices->modify();
        _indices->commit(commandList, frame);

        const vec2f scale{2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y};
        const vec2f translate{(drawData->DisplayPos.x - drawData->DisplaySize.x * 0.5f) * scale.x,
                              (drawData->DisplayPos.y - drawData->DisplaySize.y * 0.5f) * scale.y};

        constexpr float kEps = 1e-7f;
        const vec4f transform{scale.x, scale.y, translate.x, translate.y};
        if (compare(_transform, transform, kEps)) {
            _transform = transform;
            auto & uniformMem = *_uniform->memory2<vec4f>();
            uniformMem = _transform;
            _uniform->modify();
        }
        _uniform->commit(commandList, frame);

        // if (useStagingBuffer) { // do not unmap
        //     _staging->unmap();
        // }
    }

    void ImGuiSystem::draw(rhi::runtime::CommandList & commandList, uint8_t frame) noexcept {
        const ImDrawData *drawData = ImGui::GetDrawData();
        
        commandList.bindPipeline(_pipeline);
        commandList.bindBuffer(0u, (*_uniform)(frame)); // bind uniform

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

                commandList.setScissor({
                    {static_cast<int32_t>(cmd.ClipRect.x), static_cast<int32_t>(cmd.ClipRect.y)}, 
                    {static_cast<uint32_t>(cmd.ClipRect.z - cmd.ClipRect.x), static_cast<uint32_t>(cmd.ClipRect.w - cmd.ClipRect.y)}
                    }
                );

                auto & texture = *reinterpret_cast<rhi::runtime::ImagePtr *>(cmd.GetTexID());
                commandList.bindImage(1u, texture, _sampler);

                commandList.bindVertexBuffer(0u, (*_vertices)(frame), vOffset);
                commandList.bindIndexBuffer((*_indices)(frame), indexBufferOffset, rhi::IndexType::eU16);

                // ??
                const uint32_t oneChannelTexture = texture->getInfo().format == rhi::Format::eR8_Unorm;
                commandList.pushConstants(rhi::shaders::Stages::eFragment, 0u, sizeof(uint32_t), &oneChannelTexture);

                commandList.drawIndexed(static_cast<uint32_t>(cmd.ElemCount), 1u, firstIndex);

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

    constexpr ImGuiKey translateKey(KeyboardKey key) noexcept {
        switch (key) {
            // system
            case KeyboardKey::eEscape:       return ImGuiKey_Escape;
            case KeyboardKey::eEnter:        return ImGuiKey_Enter;
            case KeyboardKey::eSpace:        return ImGuiKey_Space;
            case KeyboardKey::eBackSpace:    return ImGuiKey_Backspace;
            case KeyboardKey::eTab:          return ImGuiKey_Tab;
            case KeyboardKey::eInsert:       return ImGuiKey_Insert;
            case KeyboardKey::eDelete:       return ImGuiKey_Delete;
            case KeyboardKey::ePageUp:       return ImGuiKey_PageUp;
            case KeyboardKey::ePageDown:     return ImGuiKey_PageDown;
            case KeyboardKey::eHome:         return ImGuiKey_Home;
            case KeyboardKey::eEnd:          return ImGuiKey_End;
            case KeyboardKey::ePrintScreen:  return ImGuiKey_PrintScreen;
            case KeyboardKey::eScrollLock:   return ImGuiKey_ScrollLock;
            case KeyboardKey::ePause:        return ImGuiKey_Pause;
            case KeyboardKey::eCapsLock:     return ImGuiKey_CapsLock;

            // arrows
            case KeyboardKey::eLeft:         return ImGuiKey_LeftArrow;
            case KeyboardKey::eRight:        return ImGuiKey_RightArrow;
            case KeyboardKey::eUp:           return ImGuiKey_UpArrow;
            case KeyboardKey::eDown:         return ImGuiKey_DownArrow;

            // modifiers
            case KeyboardKey::eLeftShift:    return ImGuiKey_LeftShift;
            case KeyboardKey::eRightShift:   return ImGuiKey_RightShift;
            case KeyboardKey::eLeftControl:  return ImGuiKey_LeftCtrl;
            case KeyboardKey::eRightControl: return ImGuiKey_RightCtrl;
            case KeyboardKey::eLeftAlt:      return ImGuiKey_LeftAlt;
            case KeyboardKey::eRightAlt:     return ImGuiKey_RightAlt;
            case KeyboardKey::eLeftSuper:    return ImGuiKey_LeftSuper;
            case KeyboardKey::eRightSuper:   return ImGuiKey_RightSuper;

            // letters
            case KeyboardKey::eA:            return ImGuiKey_A;
            case KeyboardKey::eB:            return ImGuiKey_B;
            case KeyboardKey::eC:            return ImGuiKey_C;
            case KeyboardKey::eD:            return ImGuiKey_D;
            case KeyboardKey::eE:            return ImGuiKey_E;
            case KeyboardKey::eF:            return ImGuiKey_F;
            case KeyboardKey::eG:            return ImGuiKey_G;
            case KeyboardKey::eH:            return ImGuiKey_H;
            case KeyboardKey::eI:            return ImGuiKey_I;
            case KeyboardKey::eJ:            return ImGuiKey_J;
            case KeyboardKey::eK:            return ImGuiKey_K;
            case KeyboardKey::eL:            return ImGuiKey_L;
            case KeyboardKey::eM:            return ImGuiKey_M;
            case KeyboardKey::eN:            return ImGuiKey_N;
            case KeyboardKey::eO:            return ImGuiKey_O;
            case KeyboardKey::eP:            return ImGuiKey_P;
            case KeyboardKey::eQ:            return ImGuiKey_Q;
            case KeyboardKey::eR:            return ImGuiKey_R;
            case KeyboardKey::eS:            return ImGuiKey_S;
            case KeyboardKey::eT:            return ImGuiKey_T;
            case KeyboardKey::eU:            return ImGuiKey_U;
            case KeyboardKey::eV:            return ImGuiKey_V;
            case KeyboardKey::eW:            return ImGuiKey_W;
            case KeyboardKey::eX:            return ImGuiKey_X;
            case KeyboardKey::eY:            return ImGuiKey_Y;
            case KeyboardKey::eZ:            return ImGuiKey_Z;

            // numbers
            case KeyboardKey::e0:            return ImGuiKey_0;
            case KeyboardKey::e1:            return ImGuiKey_1;
            case KeyboardKey::e2:            return ImGuiKey_2;
            case KeyboardKey::e3:            return ImGuiKey_3;
            case KeyboardKey::e4:            return ImGuiKey_4;
            case KeyboardKey::e5:            return ImGuiKey_5;
            case KeyboardKey::e6:            return ImGuiKey_6;
            case KeyboardKey::e7:            return ImGuiKey_7;
            case KeyboardKey::e8:            return ImGuiKey_8;
            case KeyboardKey::e9:            return ImGuiKey_9;

            // F1-F12
            case KeyboardKey::eF1:           return ImGuiKey_F1;
            case KeyboardKey::eF2:           return ImGuiKey_F2;
            case KeyboardKey::eF3:           return ImGuiKey_F3;
            case KeyboardKey::eF4:           return ImGuiKey_F4;
            case KeyboardKey::eF5:           return ImGuiKey_F5;
            case KeyboardKey::eF6:           return ImGuiKey_F6;
            case KeyboardKey::eF7:           return ImGuiKey_F7;
            case KeyboardKey::eF8:           return ImGuiKey_F8;
            case KeyboardKey::eF9:           return ImGuiKey_F9;
            case KeyboardKey::eF10:          return ImGuiKey_F10;
            case KeyboardKey::eF11:          return ImGuiKey_F11;
            case KeyboardKey::eF12:          return ImGuiKey_F12;

            // numpad
            case KeyboardKey::eKP0:          return ImGuiKey_Keypad0;
            case KeyboardKey::eKP1:          return ImGuiKey_Keypad1;
            case KeyboardKey::eKP2:          return ImGuiKey_Keypad2;
            case KeyboardKey::eKP3:          return ImGuiKey_Keypad3;
            case KeyboardKey::eKP4:          return ImGuiKey_Keypad4;
            case KeyboardKey::eKP5:          return ImGuiKey_Keypad5;
            case KeyboardKey::eKP6:          return ImGuiKey_Keypad6;
            case KeyboardKey::eKP7:          return ImGuiKey_Keypad7;
            case KeyboardKey::eKP8:          return ImGuiKey_Keypad8;
            case KeyboardKey::eKP9:          return ImGuiKey_Keypad9;
            case KeyboardKey::eKPDecimal:    return ImGuiKey_KeypadDecimal;
            case KeyboardKey::eKPDivide:     return ImGuiKey_KeypadDivide;
            case KeyboardKey::eKPMultiply:   return ImGuiKey_KeypadMultiply;
            case KeyboardKey::eKPSubtract:   return ImGuiKey_KeypadSubtract;
            case KeyboardKey::eKPAdd:        return ImGuiKey_KeypadAdd;
            case KeyboardKey::eKPEnter:      return ImGuiKey_KeypadEnter;
            case KeyboardKey::eNumLock:      return ImGuiKey_NumLock;

            // other
            case KeyboardKey::eApostrophe:   return ImGuiKey_Apostrophe;   // '
            case KeyboardKey::eComma:        return ImGuiKey_Comma;        // ,
            case KeyboardKey::eMinus:        return ImGuiKey_Minus;        // -
            case KeyboardKey::ePeriod:       return ImGuiKey_Period;       // .
            case KeyboardKey::eSlash:        return ImGuiKey_Slash;        // /
            case KeyboardKey::eSemicolon:    return ImGuiKey_Semicolon;    // ;
            case KeyboardKey::eEqual:        return ImGuiKey_Equal;        // =
            case KeyboardKey::eLeftBracket:  return ImGuiKey_LeftBracket;  // [
            case KeyboardKey::eBackSlash:    return ImGuiKey_Backslash;    // \
            case KeyboardKey::eRightBracket: return ImGuiKey_RightBracket; // ]
            case KeyboardKey::eGraveAccent:  return ImGuiKey_GraveAccent;  // ` ~

            default:                         return ImGuiKey_None;
        }
    }

    bool ImGuiSystem::onInpuKeyEvent(const KeyEvent & event, const InputKeyMod & mod) {
        ImGuiIO &io = ImGui::GetIO();
        io.KeyShift = mod.isShiftPressed();
        io.KeyAlt = mod.isAltPressed();
        io.KeySuper = mod.isSuperPressed();
        io.KeyCtrl = mod.isCtrlPressed();
        const auto key = translateKey(event.key);
        io.AddKeyEvent(key, event.state == InputEventState::ePress);

        if (event.state == InputEventState::ePress || event.state == InputEventState::eRepeate) {
            const auto &[key, keyWithShift] = _keyMap[static_cast<uint8_t>(event.key)];
            io.AddInputCharacter(io.KeyShift ? keyWithShift : key);
        }

        return io.WantCaptureKeyboard;
    }

    void ImGuiSystem::setupKeyMap() {
        _keyMap[static_cast<uint8_t>(KeyboardKey::eBackSpace)] = {8u, 8u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eTab)] = {9u, 9u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eSpace)] = {32u, 32u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eLeft)] = {17u, 17u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eRight)] = {18u, 18u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eUp)] = {19u, 19u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eDown)] = {20u, 20u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eMinus)] = {45u, 95u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::eSlash)] = {47u, 47u};
        _keyMap[static_cast<uint8_t>(KeyboardKey::ePeriod)] = {46u, 46u};
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