#pragma once

#include "commands.h"
#include "runtime.h"

#include "allocators/stack_allocator.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace engine::rhi::runtime {

    class CommandList {
    public:
        virtual ~CommandList() { clear(); };

        void begin(CommandBufferUsageFlags flags = CommandBufferUsageFlags::eOne_Time_Submit) noexcept;
        void end() noexcept;
        void reset(CommandBufferResetFlags flags) noexcept;

        void beginRenderPass(RenderPass renderPass, FrameBufferPtr & framebuffer, 
            std::span<const ClearValue> clearValues, const Rect2D & area, 
            std::span<ImageTransitionInfo> imageTransitionInfo = {},
            std::span<BufferTransitionInfo> bufferTransitionInfo = {}) noexcept;
        void endRenderPass() noexcept;
        void nextSubpass(SubpassContents contents = SubpassContents::ePrimaryBufferInline,
            std::span<ImageTransitionInfo> imageTransitionInfo = {},
            std::span<BufferTransitionInfo> bufferTransitionInfo = {}) noexcept;

        void executeCommands(std::span<const CommandList*> secondaryCommandLists) noexcept;

        void setViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f, uint32_t viewportIdx = 0u) noexcept;
        void setScissor(const rhi::Rect2D & area, uint32_t scissorIdx = 0u) noexcept;

        void bindPipeline(const PipelinePtr & pipeline) noexcept;
        // void bindVertexBuffer(uint32_t binding, Buffer * buffer, uint32_t offset = 0u);
        // void bindIndexBuffer(const Buffer * buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32);
        // void bindBuffer(uint16_t binding, const Buffer * buffer, uint8_t setId = 0u, uint32_t offset = 0u, uint32_t range = kWholeSize32);

        void bindVertexBuffer(uint32_t binding, const BufferPtr & buffer, uint32_t offset = 0u) noexcept;
        void bindIndexBuffer(const BufferPtr & buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32) noexcept;
        void bindBuffer(uint16_t binding, const BufferPtr & buffer, uint8_t setId = 0u, uint32_t offset = 0u, uint32_t range = kWholeSize32) noexcept;
        void bindImage(uint16_t binding, const ImagePtr & image, std::optional<rhi::SamplerValue> sampler, uint8_t setId = 0u) noexcept;
        void bindSampler(uint16_t binding, const rhi::SamplerValue & sampler, uint8_t setId = 0u) noexcept;

        void draw(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) noexcept;
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1u, uint32_t firstIndex = 0u, int32_t vertexOffset = 0u, 
            uint32_t firstInstance = 0u) noexcept;

        void dispatch(uint16_t groupCountX, uint16_t groupCountY, uint16_t groupCountZ) noexcept;

        void resetQueryPool(QueryPool *pool, uint16_t firstQuery, uint16_t queryCount) noexcept;
        void writeTimestamp(QueryPool *pool, PipelineStageFlags stage, uint16_t query) noexcept;

        void copyBuffer2Buffer(const BufferPtr & src, BufferPtr & dst, const CopyBufferInfo & copyInfos, bool autoBarriers = true) noexcept;
        void copyBuffer2Image(const BufferPtr & src, ImagePtr & dst, const CopyBufferImageInfo & copyInfos, bool autoBarriers = true) noexcept;
        void updateBuffer(BufferPtr & dst, uint32_t dstOffset, uint32_t dataSize, const void* pData, bool autoBarriers = true) noexcept;

        void insertBarriers() noexcept;

        virtual void record(uint8_t frameIdx) noexcept {}
        
    protected:
        inline static constexpr size_t kStackSize = 1024u * 1024u; // 1 Mb
        allocators::StackAllocator<kStackSize, false> _commands{};
        uint32_t _commandsCount = 0u;

    private:
        void clear() noexcept;

        struct Statistics {
            void reset() noexcept {
                drawCalls = 0u;
                dispatchCalls = 0u;
                binPipelines = 0u;
                renderPasses = 0u;
            }

            uint32_t drawCalls = 0u;
            uint32_t dispatchCalls = 0u;
            uint16_t binPipelines = 0u;
            uint16_t renderPasses = 0u;
        };

        struct State {
            inline constexpr static uint32_t kMaxViewports = 1u; // todo: maybe no 1u if the multiViewport feature is enabled and need to check PhysicalDeviceLimits::maxViewports

            struct BindedBuffer {
                const Buffer * buffer = nullptr;
                uint32_t offset = 0u;
            };

            void reset() noexcept {
                pipeline = nullptr;
                viewport = {};
                scissor = {};
                indexBuffer = {};
            }

            bool setViewPort(const Viewport & v, uint32_t idx) noexcept {
                if (viewport[idx] == v) {
                    return false;
                }
                viewport[idx] = v;
                return true;
            }

            bool setScissor(const Rect2D & s, uint32_t idx) noexcept {
                if (scissor[idx] == s) {
                    return false;
                }
                scissor[idx] = s;
                return true;
            }

            bool bindIndexBuffer(const Buffer * buffer, uint32_t offset) noexcept {
                if (buffer != indexBuffer.buffer || offset != indexBuffer.offset) {
                    indexBuffer.buffer = buffer;
                    indexBuffer.offset = offset;
                    return true;
                }
                return false;
            }

            const Pipeline * pipeline = nullptr;

            std::array<Viewport, kMaxViewports> viewport{};
            std::array<Rect2D, kMaxViewports> scissor{};
            BindedBuffer indexBuffer{};
        } _state;

        std::vector<sh_ptr<PipeLineResource>> _resources;
    };

}