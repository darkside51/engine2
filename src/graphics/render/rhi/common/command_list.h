#pragma once

#include "commands.h"
#include "runtime.h"

#include "allocators/stack_allocator.h"
#include "utils/dynamic_array.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace engine::rhi::runtime {

    namespace command_limits {
        constexpr uint8_t kMaxDescriptorsSets = 4u;
        constexpr uint32_t kMaxMemoryBytesToCopyCommand = 65536u;
    }

    struct BufferUsage {
        Buffer* buffer = nullptr;
        BufferUsageFlags flags = BufferUsageFlags::eNone;
        PipelineBindPoint pipelineType = PipelineBindPoint::eCompute;
        uint64_t offset = 0u;
        uint64_t size = kWholeSize64;
    };

    struct ImageUsage {
        Image* image = nullptr;
        ImageUsageFlags flags = ImageUsageFlags::eNone;
        PipelineBindPoint pipelineType = PipelineBindPoint::eCompute;
    };

    struct Pass {
        static constexpr uint16_t kDefaultBuffersInArray = 1024u;
        static constexpr uint16_t kDefaultImagesInArray = 512u;
        
        enum class Type : uint8_t {
            eTransfer = 0u,
            eCompute = 1u,
            eRender = 2u
        };
        
        Pass() = default;
        Pass(Type t, uint32_t number) : type(t), commandNumber(number) {}
        ~Pass() = default;

        dynamic_array<BufferUsage, kDefaultBuffersInArray> buffers = {};
        dynamic_array<ImageUsage, kDefaultImagesInArray> images = {};
        Type type = Type::eTransfer;
        uint32_t commandNumber = 0u;
    };

    class CommandList {
        struct Statistics {
            void reset() noexcept {
                drawCalls = 0u;
                dispatchCalls = 0u;
                binPipelines = 0u;
                renderPasses = 0u;
                updateBuffers = 0u;
                copyCommands = 0u;
            }

            uint32_t drawCalls = 0u;
            uint32_t dispatchCalls = 0u;
            uint32_t dispatchGroups = 0u;
            uint16_t binPipelines = 0u;
            uint16_t renderPasses = 0u;
            uint16_t updateBuffers = 0u;
            uint16_t copyCommands = 0u;
        };

    public:
        virtual ~CommandList() { clear(); };

        void begin(CommandBufferUsageFlags flags = CommandBufferUsageFlags::eOne_Time_Submit) noexcept;
        void end() noexcept;
        void reset(CommandBufferResetFlags flags) noexcept;

        void beginRenderPass(RenderPass renderPass, FrameBufferPtr & framebuffer, 
            stl::span<const ClearValue> clearValues, const Rect2D & area) noexcept;
        void endRenderPass() noexcept;
        void nextSubpass(SubpassContents contents = SubpassContents::ePrimaryBufferInline,
            stl::span<ImageTransitionInfo> imageTransitionInfo = {},
            stl::span<BufferTransitionInfo> bufferTransitionInfo = {}) noexcept;

        void executeCommands(stl::span<const CommandList*> secondaryCommandLists) noexcept;

        void setViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f, uint32_t viewportIdx = 0u) noexcept;
        void setScissor(const rhi::Rect2D & area, uint32_t scissorIdx = 0u) noexcept;

        void bindPipeline(const PipelinePtr & pipeline) noexcept;
        // void bindVertexBuffer(uint32_t binding, Buffer * buffer, uint32_t offset = 0u);
        // void bindIndexBuffer(const Buffer * buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32);
        // void bindBuffer(uint16_t binding, const Buffer * buffer, uint8_t setId = 0u, uint64_t offset = 0u, uint64_t range = kWholeSize64);

        void bindVertexBuffer(uint32_t binding, const BufferPtr & buffer, uint32_t offset = 0u) noexcept;
        void bindIndexBuffer(const BufferPtr & buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32) noexcept;
        void bindBuffer(uint16_t binding, const BufferPtr & buffer, uint8_t setId = 0u, uint64_t offset = 0u, uint64_t range = kWholeSize64) noexcept;
        void bindBufferDynamic(uint16_t binding, const BufferPtr & buffer, uint8_t setId = 0u, uint64_t offset = 0u, uint64_t range = kWholeSize64) noexcept;
        void bindImage(uint16_t binding, const ImagePtr & image, std::optional<rhi::SamplerValue> sampler, uint8_t setId = 0u,
             ImageAspectFlags aspectFlags = ImageAspectFlags::eColor, ImageViewType viewType = ImageViewType::eType_2D) noexcept;
        void bindSampler(uint16_t binding, const rhi::SamplerValue & sampler, uint8_t setId = 0u) noexcept;
        void bindTexelBuffer(uint16_t binding, const BufferPtr & buffer, Format format, uint8_t setId = 0u, uint64_t offset = 0u, uint64_t range = kWholeSize64) noexcept;
        void pushConstants(shaders::Stages stages, uint32_t offset, uint32_t size, const void * bytes) noexcept;

        void draw(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u) noexcept;
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1u, uint32_t firstIndex = 0u, int32_t vertexOffset = 0u, 
            uint32_t firstInstance = 0u) noexcept;

        void dispatch(uint16_t groupCountX, uint16_t groupCountY, uint16_t groupCountZ) noexcept;

        void resetQueryPool(QueryPool *pool, uint16_t firstQuery, uint16_t queryCount) noexcept;
        void writeTimestamp(QueryPool *pool, PipelineStageFlags stage, uint16_t query) noexcept;

        void copyBuffer2Buffer(const BufferPtr & src, BufferPtr & dst, const CopyBufferInfo & copyInfos) noexcept;
        void updateBuffer(BufferPtr & dst, uint32_t dstOffset, uint32_t dataSize, const void* pData) noexcept;

        void copyBuffer2Image(const BufferPtr & src, ImagePtr & dst, const CopyBufferImageInfo & copyInfos) noexcept;
        void blitImage(const ImagePtr & src, ImagePtr & dst, const BlitImageInfo & info) noexcept;

        void insertBarriers() noexcept;

        virtual void record(uint8_t frameIdx) noexcept {}
        
        const Statistics & getStats() const noexcept {
            return _statistics;
        }

    protected:
        inline static constexpr size_t kStackSize = 1024u * 1024u; // 1 Mb
        allocators::StackAllocator<kStackSize, false> _commands{};
        uint32_t _commandsCount = 0u;
        
        // dynamic_array<Pass, 8u> _passes = {};
        stl::vector<Pass> _passes = {};
        // std::array<Pass, 8u> _passes = {};
        // uint8_t _passId = 0u;
        Pass * _currentPass = nullptr;
        PipelineBindPoint _pipelineType = PipelineBindPoint::eCompute;

    private:
        void clear() noexcept;

        Statistics _statistics;

        struct State {
            inline constexpr static uint32_t kMaxViewports = 1u; // todo: maybe no 1u if the multiViewport feature is enabled and need to check PhysicalDeviceLimits::maxViewports
            inline constexpr static uint32_t kMaxBindVertexBuffers = 16u; // vkGetPhysicalDeviceProperties limits.maxVertexInputBindings, 16u is guaranteed

            struct BindedBuffer {
                const Buffer * buffer = nullptr;
                uint32_t offset = 0u;
            };

            void reset() noexcept {
                pipeline = nullptr;
                viewport = {};
                scissor = {};
                indexBuffer = {};
                vertexBuffers = {};
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

            bool bindVertexBuffer(uint32_t binding, const Buffer * buffer, uint32_t offset) noexcept {
                assert(binding < kMaxBindVertexBuffers);
                auto & vertexBuffer = vertexBuffers[binding];
                if (buffer != vertexBuffer.buffer || offset != vertexBuffer.offset) {
                    vertexBuffer.buffer = buffer;
                    vertexBuffer.offset = offset;
                    return true;
                }
                return false;
            }

            const Pipeline * pipeline = nullptr;

            std::array<Viewport, kMaxViewports> viewport{};
            std::array<Rect2D, kMaxViewports> scissor{};
            BindedBuffer indexBuffer{};
            std::array<BindedBuffer, kMaxBindVertexBuffers> vertexBuffers;
        } _state;

        stl::vector<sh_ptr<PipeLineResource>> _resources;
    };

}