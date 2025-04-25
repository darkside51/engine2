#pragma once

#include "commands.h"
#include "runtime.h"

#include "allocators/stack_allocator.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace engine::rhi::runtime {

    class CommandList {
    public:
        virtual ~CommandList() = default;

        void begin();
        void end();
        void reset(CommandBufferResetFlags flags);

        void beginRenderPass(RenderPass renderPass, const FrameBufferPtr & framebuffer, 
            std::span<const ClearValue> clearValues, const Rect2D & area);
        void endRenderPass();
        void nextSubpass(SubpassContents contents);

        void executeCommands(std::span<const CommandList*> secondaryCommandLists);

        void setViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f, uint32_t viewportIdx = 0u);
        void setScissor(const rhi::Rect2D & area, uint32_t scissorIdx = 0u);

        void bindPipeline(const PipelinePtr & pipeline);
        void bindVertexBuffer(uint32_t binding, const BufferPtr & buffer, uint32_t offset = 0u);
        void bindIndexBuffer(const BufferPtr & buffer, uint32_t offset = 0u, IndexType type = rhi::IndexType::eU32);
        void bindBuffer(uint16_t binding, Buffer * buffer, uint8_t setId = 0u);

        void draw(uint32_t vertexCount, uint32_t instanceCount = 1u, uint32_t firstVertex = 0u, uint32_t firstInstance = 0u);
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1u, uint32_t firstIndex = 0u, int32_t vertexOffset = 0u, 
            uint32_t firstInstance = 0u);


        virtual void flush(uint8_t frameIdx) noexcept {}
    protected:
        inline static constexpr size_t kStackSize = 1'000'000u; // 1 Mb
        allocators::StackAllocator<kStackSize, false> _commands{};

    private:
        const Pipeline * _pipeline = nullptr;
    };

}