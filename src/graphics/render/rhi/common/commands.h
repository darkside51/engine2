#pragma once

#include "types.h"
#include "runtime.h"

#include "log/log.h"

#include "stl/array.h"
#include "stl/span.h"
#include "stl/vector.h"
#include "common/dynamic_array.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

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
        eBindTexelBuffer = 28u,
        eBlitImage = 29u,
        eCopyImage2Image = 30u,
        eCopyImage2Buffer = 31u,
        ePrepareImage2HostRead = 32u,
    };

    class CommandList;

    struct alignas(8) Command {
        uint16_t size = 0u;
        uint16_t align = 0u;
        uint16_t offset = 0u;
        CommandType cmdType = CommandType::eUndefined;
        Command(CommandType type, uint16_t size, uint16_t align) : cmdType(type), size(size), align(align) {}

        inline constexpr void destroy() const noexcept;

        inline uint32_t alignedSize() const noexcept { // for no use alignas(8) - may be need to impl it!
            return size + align - 1u; // see stackAllocator allocAligned
        }
    };

    struct BeginCommand : Command {
        CommandBufferUsageFlags flags;
        BeginCommand(CommandBufferUsageFlags flags = CommandBufferUsageFlags::eOne_Time_Submit) : 
            Command(CommandType::eBegin, sizeof(BeginCommand), alignof(BeginCommand)), flags(flags) {}
    };

    struct EndCommand : Command {
        EndCommand() : Command(CommandType::eEnd, sizeof(EndCommand), alignof(EndCommand)) {}
    };

    struct ResetCommand : Command {
        CommandBufferResetFlags flags;
        ResetCommand(CommandBufferResetFlags flags) : 
            Command(CommandType::eReset, sizeof(ResetCommand), alignof(ResetCommand)),
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
        dynamic_array<ClearValue, 8u> clearValues;
        rhi::Rect2D area{};
        BeginRenderPassCommand(
            RenderPass renderPass, FrameBufferPtr & framebuffer, 
            stl::span<const ClearValue> clearValues, const Rect2D & area
        ) : 
            Command(CommandType::eBeginRenderPass, sizeof(BeginRenderPassCommand), alignof(BeginRenderPassCommand)),
            renderPass(renderPass),
            framebuffer(framebuffer.get()),
            clearValues(clearValues),
            area(area) {}
    };

    struct EndRenderPassCommand : Command {
        EndRenderPassCommand() : Command(CommandType::eEndRenderPass, sizeof(EndRenderPassCommand), alignof(EndRenderPassCommand)) {}
    };

    struct NextSubpassCommand : Command {
        SubpassContents contents;
        stl::span<ImageTransitionInfo> imageTransitionInfo;
        stl::span<BufferTransitionInfo> bufferTransitionInfo;
        NextSubpassCommand(SubpassContents contents,
            stl::span<ImageTransitionInfo> imageTransitionInfo = {},
            stl::span<BufferTransitionInfo> bufferTransitionInfo = {}) : 
            Command(CommandType::eNextSubpass, sizeof(NextSubpassCommand), alignof(NextSubpassCommand)),
            contents(contents),
            imageTransitionInfo(imageTransitionInfo),
            bufferTransitionInfo(bufferTransitionInfo) {}
    };

    struct ExecuteCommandsCommand : Command { // todo!
        ExecuteCommandsCommand(stl::span<const CommandList*> secondaryCommandLists) : 
            Command(CommandType::eExecuteCommands, sizeof(ExecuteCommandsCommand), alignof(ExecuteCommandsCommand)) {}
    };

    struct SetViewportCommand : Command {
        float x, y, w, h, minDepth = 0.0f, maxDepth = 1.0f;
        uint32_t viewportIdx = 0u;
        SetViewportCommand(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f, uint32_t viewportIdx = 0u) : 
            Command(CommandType::eSetViewport, sizeof(SetViewportCommand), alignof(SetViewportCommand)),
            x(x), y(y), w(w), h(h), minDepth(minDepth), maxDepth(maxDepth), viewportIdx(viewportIdx) {}
    };

    struct SetScissorCommand : Command {
        Rect2D area{};
        uint32_t scissorIdx = 0u;
        SetScissorCommand(const rhi::Rect2D & area, uint32_t scissorIdx = 0u) : 
            Command(CommandType::eSetScissor, sizeof(SetScissorCommand), alignof(SetScissorCommand)),
            area(area), scissorIdx(scissorIdx) {}
    };

    struct BindPipelineCommand : Command {
        const Pipeline * pipeline = nullptr;
        BindPipelineCommand(const PipelinePtr & pipeline) : 
            Command(CommandType::eBindPipeline, sizeof(BindPipelineCommand), alignof(BindPipelineCommand)), 
            pipeline(pipeline.get()) {}
            
        BindPipelineCommand(const Pipeline & pipeline) : 
            Command(CommandType::eBindPipeline, sizeof(BindPipelineCommand), alignof(BindPipelineCommand)), 
            pipeline(&pipeline) {}
    };

    struct BindVertexBufferCommand : Command {
        Buffer * buffer = nullptr;
        uint32_t binding = 0u;
        uint32_t offset = 0u;
        BindVertexBufferCommand(uint32_t binding, Buffer * buffer, uint32_t offset = 0u) : 
            Command(CommandType::eBindVertexBuffer, sizeof(BindVertexBufferCommand), alignof(BindVertexBufferCommand)), 
            binding(binding), buffer(buffer), offset(offset) {}
    };

    struct BindIndexBufferCommand : Command {
        const Buffer * buffer = nullptr;
        uint32_t offset = 0u;
        IndexType type = rhi::IndexType::eU32;
        BindIndexBufferCommand(const Buffer * buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32) : 
            Command(CommandType::eBindIndexBuffer, sizeof(BindIndexBufferCommand), alignof(BindIndexBufferCommand)), 
            buffer(buffer), offset(offset), type(type) {}
    };

    struct BindBufferCommand : Command {
        const Buffer * buffer = nullptr;
        uint64_t offset = 0u;
        uint64_t range = kWholeSize64;
        uint16_t binding = 0u;
        uint8_t setId = 0u;
        bool dynamic = false;
        
        BindBufferCommand(uint16_t binding, const Buffer * buffer, uint8_t setId = 0u, uint64_t offset = 0u, uint64_t range = kWholeSize64, bool dynamic = false) : 
            Command(CommandType::eBindBuffer, sizeof(BindBufferCommand), alignof(BindBufferCommand)), 
            buffer(buffer),
            binding(binding),
            setId(setId), 
            offset(offset),
            range(range),
            dynamic(dynamic) {}
    };

    struct BindImageSamplerCommand : Command { // todo!
        Image * image = nullptr;
        
        uint32_t baseLayer = 0u;
        uint32_t layersCount = 1u;
        uint32_t baseLevel = 0u;
        uint32_t levelsCount = 1u;

        uint16_t binding = 0u;
        uint8_t setId = 0u;

        rhi::ImageViewType viewType = rhi::ImageViewType::eType_2D;
        rhi::ImageAspectFlags aspectFlags = rhi::ImageAspectFlags::eNone;
        
        std::optional<rhi::SamplerValue> samplerValue = {};

        BindImageSamplerCommand(rhi::ImageViewType viewType, uint16_t binding, Image * image, std::optional<rhi::SamplerValue> sampler, uint8_t setId = 0u) : 
            Command(CommandType::eBindImageSampler, sizeof(BindImageSamplerCommand), alignof(BindImageSamplerCommand)),
            image(image),
            layersCount(image->getInfo().arrayLayers),
            levelsCount(image->getInfo().mipLevels),
            binding(binding),
            setId(setId),
            viewType(viewType),
            aspectFlags(image->getInfo().aspectFlags),
            samplerValue(sampler) {}

        BindImageSamplerCommand(
            rhi::ImageViewType viewType, rhi::ImageAspectFlags aspectFlags, uint16_t binding, 
            Image * image, std::optional<rhi::SamplerValue> sampler, uint8_t setId = 0u) : 
            Command(CommandType::eBindImageSampler, sizeof(BindImageSamplerCommand), alignof(BindImageSamplerCommand)),
            image(image),
            layersCount(image->getInfo().arrayLayers),
            levelsCount(image->getInfo().mipLevels),
            binding(binding),
            setId(setId),
            viewType(viewType),
            aspectFlags(aspectFlags),
            samplerValue(sampler) {}
    };

    struct BindTexelBufferCommand : Command {
        Buffer * buffer = nullptr;
        uint64_t offset = 0u;
        uint64_t range = kWholeSize64;
        uint16_t binding = 0u;
        uint8_t setId = 0u;
        Format format = Format::eUndefined;
        
        BindTexelBufferCommand(uint16_t binding, Buffer * buffer, Format format, uint8_t setId = 0u, uint64_t offset = 0u, uint64_t range = kWholeSize64) : 
            Command(CommandType::eBindTexelBuffer, sizeof(BindTexelBufferCommand), alignof(BindTexelBufferCommand)), 
            buffer(buffer),
            offset(offset),
            range(range),
            binding(binding),
            setId(setId), 
            format(format) {}
    };

    struct PushConstantsCommand : Command {
        uint32_t dataOffset = 0u;
        uint32_t dataSize = 0u;
        shaders::Stages stages = shaders::Stages::eNone;
        stl::array<std::byte, 128u> data;
        PushConstantsCommand(shaders::Stages stages, uint32_t offset, uint32_t size, const void * bytes) : 
            Command(CommandType::ePushConstants, sizeof(PushConstantsCommand), alignof(PushConstantsCommand)),
            stages(stages),
            dataOffset(offset),
            dataSize(size) {
                memcpy(data.data(), bytes, dataSize);
            }
    };

    struct DrawCommand : Command {
        uint32_t vertexCount = 0u;
        uint32_t instanceCount = 1u;
        uint32_t firstVertex = 0u;
        uint32_t firstInstance = 0u;
        DrawCommand(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) : 
            Command(CommandType::eDraw, sizeof(DrawCommand), alignof(DrawCommand)),
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
            Command(CommandType::eDrawIndexed, sizeof(DrawIndexedCommand), alignof(DrawIndexedCommand)),
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

        UpdateBufferCommand(Buffer * dst, uint32_t dstOffset, uint32_t dataSize, const void* pData) : 
            Command(CommandType::eUpdateBuffer, sizeof(UpdateBufferCommand), alignof(UpdateBufferCommand)),
            dst(dst), dstOffset(dstOffset), dataSize(dataSize), pData(pData) {
            }
    };

    struct CopyBuffer2BufferCommand : Command {
        CopyBufferInfo info{}; // todo: support array of CopyBufferInfo
        Buffer * src = nullptr;
        Buffer * dst = nullptr;
        CopyBuffer2BufferCommand(Buffer * src, Buffer * dst, const CopyBufferInfo & info) : 
            Command(CommandType::eCopyBuffer2Buffer, sizeof(CopyBuffer2BufferCommand), alignof(CopyBuffer2BufferCommand)),
            info(info),
            src(src), 
            dst(dst) {}
    };

    struct CopyBuffer2ImageCommand : Command {
        CopyBufferImageInfo info{}; // todo: support array of CopyBufferImageInfo
        Buffer * src = nullptr;
        Image * dst = nullptr;
        CopyBuffer2ImageCommand(Buffer * src, Image * dst, const CopyBufferImageInfo & info) : 
            Command(CommandType::eCopyBuffer2Image, sizeof(CopyBuffer2ImageCommand), alignof(CopyBuffer2ImageCommand)),
            info(info),
            src(src), 
            dst(dst) {}
    };

    struct CopyImage2ImageCommand : Command {
        CopyImage2ImageInfo info{}; // todo: support array of CopyImage2ImageInfo
        Image * src = nullptr;
        Image * dst = nullptr;
        CopyImage2ImageCommand(Image * src, Image * dst, const CopyImage2ImageInfo & info) : 
            Command(CommandType::eCopyImage2Image, sizeof(CopyImage2ImageCommand), alignof(CopyImage2ImageCommand)),
            info(info),
            src(src), 
            dst(dst) {}
    };

    struct CopyImage2BufferCommand : Command {
        CopyBufferImageInfo info{}; // todo: support array of CopyImage2ImageInfo
        Image * src = nullptr;
        Buffer * dst = nullptr;
        CopyImage2BufferCommand(Image * src, Buffer * dst, const CopyBufferImageInfo & info) : 
            Command(CommandType::eCopyImage2Buffer, sizeof(CopyImage2BufferCommand), alignof(CopyImage2BufferCommand)),
            info(info),
            src(src), 
            dst(dst) {}
    };

    struct PrepareImage2HostRead : Command {
        Image * img = nullptr;
        PrepareImage2HostRead(Image * img) : 
            Command(CommandType::ePrepareImage2HostRead, sizeof(PrepareImage2HostRead), alignof(PrepareImage2HostRead)),
            img(img) {}
    };

    struct BlitImageCommand : Command {
        BlitImageInfo info{};
        Image * src = nullptr;
        Image * dst = nullptr;
        BlitImageCommand(Image * src, Image * dst, const BlitImageInfo & info) : 
            Command(CommandType::eBlitImage, sizeof(BlitImageCommand), alignof(BlitImageCommand)),
            info(info),
            src(src), 
            dst(dst) {}
    };

    struct PipelineBarriersCommand : Command { // todo!
        PipelineBarriersCommand() : Command(CommandType::ePipelineBarriers, sizeof(PipelineBarriersCommand), alignof(PipelineBarriersCommand)) {}
    };

    struct SetEventCommand : Command { // todo!
        SetEventCommand() : Command(CommandType::eSetEvent, sizeof(SetEventCommand), alignof(SetEventCommand)) {}
    };

    struct WaitEventsCommand : Command { // todo!
        WaitEventsCommand() : Command(CommandType::eWaitEvents, sizeof(WaitEventsCommand), alignof(WaitEventsCommand)) {}
    };

    struct ResetQueryPoolCommand : Command {
        QueryPool *pool;
        uint16_t firstQuery;
        uint16_t queryCount;
        ResetQueryPoolCommand(QueryPool *pool, uint16_t firstQuery, uint16_t queryCount) : 
            Command(CommandType::eResetQueryPool, sizeof(ResetQueryPoolCommand), alignof(ResetQueryPoolCommand)), 
            pool(pool), 
            firstQuery(firstQuery),
            queryCount(queryCount) {}
    };

    struct WriteTimestampCommand : Command {
        QueryPool *pool;
        PipelineStageFlags stage;
        uint16_t query;
        WriteTimestampCommand(QueryPool *pool, PipelineStageFlags stage, uint16_t query) :
            Command(CommandType::eWriteTimestamp, sizeof(WriteTimestampCommand), alignof(WriteTimestampCommand)), 
            pool(pool), 
            stage(stage),
            query(query) {}
    };

    struct CopyQueryPoolResults : Command { // todo!
        CopyQueryPoolResults() : Command(CommandType::eCopyQueryPoolResults, sizeof(CopyQueryPoolResults), alignof(CopyQueryPoolResults)) {}
    };

    struct DispatchCommand : Command {
        uint16_t groupCountX;
        uint16_t groupCountY;
        uint16_t groupCountZ;
        DispatchCommand(uint16_t groupCountX, uint16_t groupCountY, uint16_t groupCountZ) :
            Command(CommandType::eDispatch, sizeof(DispatchCommand), alignof(DispatchCommand)),
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
            case CommandType::ePushConstants: [[fallthrough]];
            case CommandType::eBindTexelBuffer: [[fallthrough]];
            case CommandType::eBlitImage: [[fallthrough]];
            case CommandType::eCopyImage2Image: [[fallthrough]];
            case CommandType::eCopyImage2Buffer: [[fallthrough]];
            case CommandType::ePrepareImage2HostRead:
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