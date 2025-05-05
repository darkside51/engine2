#pragma once

#include "types.h"
#include "runtime.h"

#include "utils/dynamic_array.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace engine::rhi::runtime {

    enum class CommandType : uint8_t {
        eUndefined = 0u,
        eBegin = 1u,
        eEnd = 2u,
        eReset = 3u,
        eBeginRenderPass = 4u,
        eEndRenderPass = 5u,
        eNextSubpass = 6u,
        eExecuteCommands = 7u,
        eSetViewport = 8u,
        eSetScissor = 9u,
        eBindPipeline = 10u,
        eBindVertexBuffer = 11u,
        eBindIndexBuffer = 12u,
        eBindResource = 13u,
        eDraw = 14u,
        eDrawIndexed = 15u,
        eCopyBuffer2Buffer = 16u,
        eCopyBuffer2Image = 17u,
        ePipelineBarrier = 18u,
    };

    class CommandList;

    struct Command {
        uint32_t size = 0u;
        CommandType cmdType = CommandType::eUndefined;
        Command(CommandType type, uint32_t size) : cmdType(type), size(size) {}

        inline constexpr void destroy() const noexcept;
    };

    struct BeginCommand : Command {
        BeginCommand() : Command(CommandType::eBegin, sizeof(BeginCommand)) {}
    };

    struct EndCommand : Command {
        EndCommand() : Command(CommandType::eEnd, sizeof(EndCommand)) {}
    };

    struct ResetCommand : Command {
        CommandBufferResetFlags flags;
        ResetCommand(CommandBufferResetFlags flags) : 
            Command(CommandType::eReset, sizeof(ResetCommand)),
            flags(flags) {}
    };

    struct BeginRenderPassCommand : Command {
        RenderPass renderPass{};
        const FrameBuffer * framebuffer = nullptr; 
        dynamic_array<ClearValue, 4u> clearValues;
        rhi::Rect2D area{};
        BeginRenderPassCommand(RenderPass renderPass, const FrameBufferPtr & framebuffer, 
            std::span<const ClearValue> clearValues, const Rect2D & area) : 
            Command(CommandType::eBeginRenderPass, sizeof(BeginRenderPassCommand)),
            renderPass(renderPass),
            framebuffer(framebuffer.get()),
            clearValues(clearValues),
            area(area) {}
    };

    struct EndRenderPassCommand : Command {
        EndRenderPassCommand() : Command(CommandType::eEndRenderPass, sizeof(EndRenderPassCommand)) {}
    };

    struct NextSubpassCommand : Command {
        SubpassContents contents;
        NextSubpassCommand(SubpassContents contents) : 
            Command(CommandType::eNextSubpass, sizeof(NextSubpassCommand)),
            contents(contents) {}
    };

    struct ExecuteCommandsCommand : Command { // todo!
        ExecuteCommandsCommand(std::span<const CommandList*> secondaryCommandLists) : 
            Command(CommandType::eExecuteCommands, sizeof(ExecuteCommandsCommand)) {}
    };

    struct SetViewportCommand : Command {
        float x, y, w, h, minDepth = 0.0f, maxDepth = 1.0f;
        uint32_t viewportIdx = 0u;
        SetViewportCommand(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f, uint32_t viewportIdx = 0u) : 
            Command(CommandType::eSetViewport, sizeof(SetViewportCommand)),
            x(x), y(y), w(w), h(h), minDepth(minDepth), maxDepth(maxDepth), viewportIdx(viewportIdx) {}
    };

    struct SetScissorCommand : Command {
        Rect2D area{};
        uint32_t scissorIdx = 0u;
        SetScissorCommand(const rhi::Rect2D & area, uint32_t scissorIdx = 0u) : 
            Command(CommandType::eSetScissor, sizeof(SetScissorCommand)),
            area(area), scissorIdx(scissorIdx) {}
    };

    struct BindPipelineCommand : Command {
        const Pipeline * pipeline = nullptr;
        BindPipelineCommand(const PipelinePtr & pipeline) : 
            Command(CommandType::eBindPipeline, sizeof(BindPipelineCommand)), 
            pipeline(pipeline.get()) {}
    };

    struct BindVertexBufferCommand : Command {
        const Buffer * buffer = nullptr;
        uint32_t binding = 0u;
        uint32_t offset = 0u;
        BindVertexBufferCommand(uint32_t binding, const Buffer * buffer, uint32_t offset = 0u) : 
            Command(CommandType::eBindVertexBuffer, sizeof(BindVertexBufferCommand)), 
            binding(binding), buffer(buffer), offset(offset) {}
    };

    struct BindIndexBufferCommand : Command {
        const Buffer * buffer = nullptr;
        uint32_t offset = 0u;
        IndexType type = rhi::IndexType::eU32;
        BindIndexBufferCommand(const Buffer * buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32) : 
            Command(CommandType::eBindIndexBuffer, sizeof(BindIndexBufferCommand)), 
            buffer(buffer), offset(offset), type(type) {}
    };

    struct BindResourceCommand : Command { // todo!
        const Buffer * buffer = nullptr;
        uint16_t binding = 0u;
        uint8_t setId = 0u;
        
        BindResourceCommand(uint16_t binding, const Buffer * buffer, uint8_t setId = 0u) : 
            Command(CommandType::eBindResource, sizeof(BindResourceCommand)), 
            buffer(buffer),
            binding(binding),
            setId(setId) {}
    };

    struct DrawCommand : Command {
        uint32_t vertexCount = 0u;
        uint32_t instanceCount = 1u;
        uint32_t firstVertex = 0u;
        uint32_t firstInstance = 0u;
        DrawCommand(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) : 
            Command(CommandType::eDraw, sizeof(DrawCommand)),
            vertexCount(vertexCount),
            instanceCount(instanceCount),
            firstVertex(firstVertex),
            firstInstance(firstInstance) {}
    };

    struct DrawIndexedCommand : Command {
        uint32_t indexCount = 0u;
        uint32_t instanceCount = 1u;
        uint32_t firstIndex = 0u;
        int32_t vertexOffset = 0u;
        uint32_t firstInstance = 0u;
        DrawIndexedCommand(uint32_t indexCount, uint32_t instanceCount = 1u, uint32_t firstIndex = 0u, int32_t vertexOffset = 0u, 
            uint32_t firstInstance = 0u) : 
            Command(CommandType::eDrawIndexed, sizeof(DrawIndexedCommand)),
            indexCount(indexCount),
            instanceCount(instanceCount),
            firstIndex(firstIndex),
            vertexOffset(vertexOffset),
            firstInstance(firstInstance) {}
    };

    struct CopyBuffer2BufferCommand : Command {
        CopyInfo info{};
        Buffer * src = nullptr;
        Buffer * dst = nullptr;
        CopyBuffer2BufferCommand(Buffer * src, Buffer * dst, const CopyInfo & info) : 
            Command(CommandType::eCopyBuffer2Buffer, sizeof(CopyBuffer2BufferCommand)),
            src(src), 
            dst(dst),
            info(info) {}
    };

    struct PipelineBarrierCommand : Command { // todo!
        PipelineBarrierCommand() : Command(CommandType::ePipelineBarrier, sizeof(PipelineBarrierCommand)) {}
    };

    ///////////////////////////////////////////////
    inline constexpr void Command::destroy() const noexcept {
        switch (cmdType) {
            case CommandType::eUndefined: [[fallthrough]];
            case CommandType::eBegin: [[fallthrough]];
            case CommandType::eEnd: [[fallthrough]];
            case CommandType::eReset: [[fallthrough]];
            case CommandType::eEndRenderPass: [[fallthrough]];
            case CommandType::eNextSubpass: [[fallthrough]];
            case CommandType::eExecuteCommands: [[fallthrough]];
            case CommandType::eSetViewport: [[fallthrough]];
            case CommandType::eSetScissor: [[fallthrough]];
            case CommandType::eBindPipeline: [[fallthrough]];
            case CommandType::eBindVertexBuffer: [[fallthrough]];
            case CommandType::eBindIndexBuffer: [[fallthrough]];
            case CommandType::eBindResource: [[fallthrough]];
            case CommandType::eDraw: [[fallthrough]];
            case CommandType::eDrawIndexed: [[fallthrough]];
            case CommandType::eCopyBuffer2Buffer: [[fallthrough]];
            case CommandType::eCopyBuffer2Image: [[fallthrough]];
            case CommandType::ePipelineBarrier:
                return;
            case CommandType::eBeginRenderPass: 
                static_cast<const BeginRenderPassCommand*>(this)->~BeginRenderPassCommand();
        }
    }
}