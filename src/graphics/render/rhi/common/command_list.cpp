#include "command_list.h"

namespace engine::rhi::runtime {

    void CommandList::clear() {
        _commands->clear();

        _resources.clear();
        _resources.reserve(1024u);
    }

    void CommandList::begin() {
        clear();
        _commands->create<BeginCommand>();
        _pipeline = nullptr;
    }

    void CommandList::end() {
        _commands->create<EndCommand>();
    }

    void CommandList::reset(CommandBufferResetFlags flags) {
        clear();
        _commands->create<ResetCommand>(flags);
        _pipeline = nullptr;
    }

    void CommandList::beginRenderPass(RenderPass renderPass, const FrameBufferPtr & framebuffer, 
        std::span<const ClearValue> clearValues, const Rect2D & area) {
        _commands->create<BeginRenderPassCommand>(renderPass, framebuffer, clearValues, area);
    }

    void CommandList::endRenderPass() {
        _commands->create<EndRenderPassCommand>();
    }

    void CommandList::nextSubpass(SubpassContents contents) {
        _commands->create<NextSubpassCommand>(contents);
    }

    void CommandList::executeCommands(std::span<const CommandList*> secondaryCommandLists) {
        _commands->create<ExecuteCommandsCommand>(secondaryCommandLists);
    }

    void CommandList::setViewport(float x, float y, float w, float h, float minDepth, float maxDepth, uint32_t viewportIdx) {
        _commands->create<SetViewportCommand>(x, y, w, h, minDepth, maxDepth, viewportIdx);
    }

    void CommandList::setScissor(const rhi::Rect2D & area, uint32_t scissorIdx) {
        _commands->create<SetScissorCommand>(area, scissorIdx);
    }

    void CommandList::bindPipeline(const PipelinePtr & pipeline) {
        if (_pipeline != pipeline.get()) {
            _commands->create<BindPipelineCommand>(pipeline);
            _pipeline = pipeline.get();
        }
    }

    void CommandList::bindVertexBuffer(uint32_t binding, const Buffer * buffer, uint32_t offset) {
        _commands->create<BindVertexBufferCommand>(binding, buffer, offset);
    }

    void CommandList::bindIndexBuffer(const Buffer * buffer, uint32_t offset, IndexType type) {
        _commands->create<BindIndexBufferCommand>(buffer, offset, type);
    }

    void CommandList::bindBuffer(uint16_t binding, const Buffer * buffer, uint8_t setId) {
        _commands->create<BindResourceCommand>(binding, buffer, setId);
    }

/////////////////////// shared versions
    void CommandList::bindVertexBuffer(uint32_t binding, BufferPtr & buffer, uint32_t offset) {
        _resources.push_back(buffer);
        _commands->create<BindVertexBufferCommand>(binding, buffer.get(), offset);
    }

    void CommandList::bindIndexBuffer(BufferPtr & buffer, uint32_t offset, IndexType type) {
        _resources.push_back(buffer);
        _commands->create<BindIndexBufferCommand>(buffer.get(), offset, type);
    }

    void CommandList::bindBuffer(uint16_t binding, BufferPtr & buffer, uint8_t setId) {
        _resources.push_back(buffer);
        _commands->create<BindResourceCommand>(binding, buffer.get(), setId);
    }
///////////////////////

    void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        _commands->create<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, 
        uint32_t firstInstance) {
        _commands->create<DrawIndexedCommand>(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

}