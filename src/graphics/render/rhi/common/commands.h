#pragma once

#include "types.h"
#include "runtime.h"

#include "utils/dynamic_array.h"
#include "log/log.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
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
        eBindBuffer = 13u,
        eBindImageSampler = 14u,
        eDraw = 15u,
        eDrawIndexed = 16u,
        eCopyBuffer2Buffer = 17u,
        eCopyBuffer2Image = 18u,
        ePipelineBarriers = 19u,
        eSetEvent = 20u,
        eWaitEvents = 21u,
        eResetQueryPool = 22u,
        eWriteTimestamp = 23u,
        eCopyQueryPoolResults = 24u,
        eDispatch = 25u,
        eUpdateBuffer = 26u,
        ePushConstants = 27u,
    };

    class CommandList;

    struct Command {
        uint32_t size = 0u;
        CommandType cmdType = CommandType::eUndefined;
        Command(CommandType type, uint32_t size) : cmdType(type), size(size) {}

        inline constexpr void destroy() const noexcept;
    };

    struct BeginCommand : Command {
        CommandBufferUsageFlags flags;
        BeginCommand(CommandBufferUsageFlags flags = CommandBufferUsageFlags::eOne_Time_Submit) : 
            Command(CommandType::eBegin, sizeof(BeginCommand)), flags(flags) {}
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

    struct ImageTransitionInfo {
        Image * img = nullptr;
        ImageLayout layout = ImageLayout::eShader_Read_Only_Optimal;
        PipelineStageFlags stage = PipelineStageFlags::eFragment_Shader;
        AccessFlags access = AccessFlags::eShader_Read;
        uint8_t baseLayer = 0u; 
        uint8_t baseLevel = 0u; 
        uint8_t layersCount = 0u; 
        uint8_t levelsCount = 0u;
    };

    struct BufferTransitionInfo {
        Buffer * buff = nullptr;
        PipelineStageFlags stage = PipelineStageFlags::eVertex_Shader;
        AccessFlags access = AccessFlags::eShader_Read;
        size_t offset = 0u;
        size_t size = kWholeSize;
    };

    struct BeginRenderPassCommand : Command {
        RenderPass renderPass{};
        FrameBuffer * framebuffer = nullptr; 
        dynamic_array<ClearValue, 4u> clearValues;
        rhi::Rect2D area{};
        std::span<ImageTransitionInfo> imageTransitionInfo;
        std::span<BufferTransitionInfo> bufferTransitionInfo;
        BeginRenderPassCommand(
            RenderPass renderPass, FrameBufferPtr & framebuffer, 
            std::span<const ClearValue> clearValues, const Rect2D & area, 
            std::span<ImageTransitionInfo> imageTransitionInfo = {},
            std::span<BufferTransitionInfo> bufferTransitionInfo = {}
        ) : 
            Command(CommandType::eBeginRenderPass, sizeof(BeginRenderPassCommand)),
            renderPass(renderPass),
            framebuffer(framebuffer.get()),
            clearValues(clearValues),
            area(area),
            imageTransitionInfo(imageTransitionInfo),
            bufferTransitionInfo(bufferTransitionInfo) {}
    };

    struct EndRenderPassCommand : Command {
        EndRenderPassCommand() : Command(CommandType::eEndRenderPass, sizeof(EndRenderPassCommand)) {}
    };

    struct NextSubpassCommand : Command {
        SubpassContents contents;
        std::span<ImageTransitionInfo> imageTransitionInfo;
        std::span<BufferTransitionInfo> bufferTransitionInfo;
        NextSubpassCommand(SubpassContents contents,
            std::span<ImageTransitionInfo> imageTransitionInfo = {},
            std::span<BufferTransitionInfo> bufferTransitionInfo = {}) : 
            Command(CommandType::eNextSubpass, sizeof(NextSubpassCommand)),
            contents(contents),
            imageTransitionInfo(imageTransitionInfo),
            bufferTransitionInfo(bufferTransitionInfo) {}
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
        Buffer * buffer = nullptr;
        uint32_t binding = 0u;
        uint32_t offset = 0u;
        BindVertexBufferCommand(uint32_t binding, Buffer * buffer, uint32_t offset = 0u) : 
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

    struct BindBufferCommand : Command {
        const Buffer * buffer = nullptr;
        uint32_t offset = 0u;
        uint32_t range = kWholeSize32;
        uint16_t binding = 0u;
        uint8_t setId = 0u;
        bool dynamic = false;
        
        BindBufferCommand(uint16_t binding, const Buffer * buffer, uint8_t setId = 0u, uint32_t offset = 0u, uint32_t range = kWholeSize32, bool dynamic = false) : 
            Command(CommandType::eBindBuffer, sizeof(BindBufferCommand)), 
            buffer(buffer),
            binding(binding),
            setId(setId), 
            offset(offset),
            range(range),
            dynamic(dynamic) {}
    };

    struct BindImageSamplerCommand : Command { // todo!
        Image * image = nullptr;
        rhi::ImageViewType viewType = rhi::ImageViewType::eType_2D;
        uint16_t binding = 0u;
        uint8_t setId = 0u;

        uint32_t baseLayer = 0u;
        uint32_t layersCount = 1u;
        uint32_t baseLevel = 0u;
        uint32_t levelsCount = 1u;

        std::optional<rhi::SamplerValue> samplerValue= {};

        BindImageSamplerCommand(uint16_t binding, Image * image, std::optional<rhi::SamplerValue> sampler, uint8_t setId = 0u) : 
            Command(CommandType::eBindImageSampler, sizeof(BindImageSamplerCommand)),
            image(image),
            binding(binding),
            setId(setId),
            layersCount(image->getInfo().arrayLayers),
            levelsCount(image->getInfo().mipLevels),
            samplerValue(sampler) {
            }
    };

    struct PushConstantsCommand : Command {
        uint32_t offset = 0u;
        uint32_t size = 0u;
        shaders::Stages stages = shaders::Stages::eNone;
        std::array<std::byte, 128u> data;
        PushConstantsCommand(shaders::Stages stages, uint32_t offset, uint32_t size, const void * bytes) : 
            Command(CommandType::ePushConstants, sizeof(PushConstantsCommand)),
            stages(stages),
            offset(offset),
            size(size) {
                memcpy(data.data(), bytes, size);
            }
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

    struct UpdateBufferCommand : Command {
        Buffer * dst = nullptr;
        uint32_t dstOffset;
        uint32_t dataSize;
        const void* pData;
        bool autoBarriers = true;

        UpdateBufferCommand(Buffer * dst, uint32_t dstOffset, uint32_t dataSize, const void* pData, bool autoBarriers) : 
            Command(CommandType::eUpdateBuffer, sizeof(UpdateBufferCommand)),
            dst(dst), dstOffset(dstOffset), dataSize(dataSize), pData(pData),
            autoBarriers(autoBarriers) {}
    };

    struct CopyBuffer2BufferCommand : Command {
        CopyBufferInfo info{}; // todo: support array of CopyBufferInfo
        Buffer * src = nullptr;
        Buffer * dst = nullptr;
        bool autoBarriers = true;
        CopyBuffer2BufferCommand(Buffer * src, Buffer * dst, const CopyBufferInfo & info, bool autoBarriers) : 
            Command(CommandType::eCopyBuffer2Buffer, sizeof(CopyBuffer2BufferCommand)),
            info(info),
            src(src), 
            dst(dst),
            autoBarriers(autoBarriers) {}
    };

    struct CopyBuffer2ImageCommand : Command {
        CopyBufferImageInfo info{}; // todo: support array of CopyBufferImageInfo
        Buffer * src = nullptr;
        Image * dst = nullptr;
        bool autoBarriers = true;
        CopyBuffer2ImageCommand(Buffer * src, Image * dst, const CopyBufferImageInfo & info, bool autoBarriers = true) : 
            Command(CommandType::eCopyBuffer2Image, sizeof(CopyBuffer2ImageCommand)),
            info(info),
            src(src), 
            dst(dst),
            autoBarriers(autoBarriers) {}
    };

    struct PipelineBarriersCommand : Command { // todo!
        PipelineBarriersCommand() : Command(CommandType::ePipelineBarriers, sizeof(PipelineBarriersCommand)) {}
    };

    struct SetEventCommand : Command { // todo!
        SetEventCommand() : Command(CommandType::eSetEvent, sizeof(SetEventCommand)) {}
    };

    struct WaitEventsCommand : Command { // todo!
        WaitEventsCommand() : Command(CommandType::eWaitEvents, sizeof(WaitEventsCommand)) {}
    };

    struct ResetQueryPoolCommand : Command {
        QueryPool *pool;
        uint16_t firstQuery;
        uint16_t queryCount;
        ResetQueryPoolCommand(QueryPool *pool, uint16_t firstQuery, uint16_t queryCount) : 
            Command(CommandType::eResetQueryPool, sizeof(ResetQueryPoolCommand)), 
            pool(pool), 
            firstQuery(firstQuery),
            queryCount(queryCount) {}
    };

    struct WriteTimestampCommand : Command {
        QueryPool *pool;
        PipelineStageFlags stage;
        uint16_t query;
        WriteTimestampCommand(QueryPool *pool, PipelineStageFlags stage, uint16_t query) :
            Command(CommandType::eWriteTimestamp, sizeof(WriteTimestampCommand)), 
            pool(pool), 
            stage(stage),
            query(query) {}
    };

    struct CopyQueryPoolResults : Command { // todo!
        CopyQueryPoolResults() : Command(CommandType::eCopyQueryPoolResults, sizeof(CopyQueryPoolResults)) {}
    };

    struct DispatchCommand : Command {
        uint16_t groupCountX;
        uint16_t groupCountY;
        uint16_t groupCountZ;
        DispatchCommand(uint16_t groupCountX, uint16_t groupCountY, uint16_t groupCountZ) :
            Command(CommandType::eDispatch, sizeof(DispatchCommand)),
            groupCountX(groupCountX),
            groupCountY(groupCountY),
            groupCountZ(groupCountZ) {}
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
            case CommandType::eBindBuffer: [[fallthrough]];
            case CommandType::eBindImageSampler: [[fallthrough]];
            case CommandType::eDraw: [[fallthrough]];
            case CommandType::eDrawIndexed: [[fallthrough]];
            case CommandType::eCopyBuffer2Buffer: [[fallthrough]];
            case CommandType::eCopyBuffer2Image: [[fallthrough]];
            case CommandType::ePipelineBarriers: [[fallthrough]];
            case CommandType::eSetEvent: [[fallthrough]];
            case CommandType::eWaitEvents: [[fallthrough]];
            case CommandType::eResetQueryPool: [[fallthrough]];
            case CommandType::eWriteTimestamp: [[fallthrough]];
            case CommandType::eCopyQueryPoolResults: [[fallthrough]];
            case CommandType::eDispatch: [[fallthrough]];
            case CommandType::eUpdateBuffer: [[fallthrough]];
            case CommandType::ePushConstants:
                return;
            case CommandType::eBeginRenderPass: {
                static_cast<const BeginRenderPassCommand*>(this)->~BeginRenderPassCommand();
                return;
            }
        }

        LOG_E("destroy Command with type %d not handled", std::to_underlying(cmdType));
        assert(false);
    }
}