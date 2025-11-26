#include "command_list.h"

namespace engine::rhi::runtime {

    void CommandList::clear() noexcept {
        auto commandsCount = _commandsCount;
        std::byte * memory = _commands.memory().data();
        while ((commandsCount--) > 0u) {
            Command * cmd = reinterpret_cast<Command*>(memory);
            memory += cmd->size;
            cmd->destroy();
        }
        
        _commands->clear();
        _commandsCount = 0u;

        _resources.clear();
        _resources.reserve(1024u);
    }

    void CommandList::begin(CommandBufferUsageFlags flags) noexcept {
        clear();
        _commands->create<BeginCommand>(flags);
        _state.reset();
        ++_commandsCount;
    }

    void CommandList::end() noexcept {
        _commands->create<EndCommand>();
        ++_commandsCount;
    }

    void CommandList::reset(CommandBufferResetFlags flags) noexcept {
        clear();
        _commands->create<ResetCommand>(flags);
        _state.reset();
        ++_commandsCount;
    }

    void CommandList::beginRenderPass(RenderPass renderPass, FrameBufferPtr & framebuffer, 
        std::span<const ClearValue> clearValues, const Rect2D & area, 
        std::span<ImageTransitionInfo> imageTransitionInfo,
        std::span<BufferTransitionInfo> bufferTransitionInfo) noexcept {
        _resources.push_back(framebuffer);
        _commands->create<BeginRenderPassCommand>(renderPass, framebuffer, clearValues, area, 
            imageTransitionInfo, bufferTransitionInfo);
        ++_commandsCount;
    }

    void CommandList::endRenderPass() noexcept {
        _commands->create<EndRenderPassCommand>();
        ++_commandsCount;
    }

    void CommandList::nextSubpass(SubpassContents contents,
        std::span<ImageTransitionInfo> imageTransitionInfo,
        std::span<BufferTransitionInfo> bufferTransitionInfo) noexcept {
        _commands->create<NextSubpassCommand>(contents, imageTransitionInfo, bufferTransitionInfo);
        ++_commandsCount;
    }

    void CommandList::executeCommands(std::span<const CommandList*> secondaryCommandLists) noexcept {
        _commands->create<ExecuteCommandsCommand>(secondaryCommandLists);
        ++_commandsCount;
    }

    void CommandList::setViewport(float x, float y, float w, float h, float minDepth, float maxDepth, uint32_t viewportIdx) noexcept {
        if (_state.setViewPort(
            {
                .screenRect = {
                    .offset = {static_cast<int32_t>(x), static_cast<int32_t>(y)},
                    .extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)}
                },
                .minDepth = minDepth,
                .maxDepth = maxDepth
            }, 
        viewportIdx)) {
            _commands->create<SetViewportCommand>(x, y, w, h, minDepth, maxDepth, viewportIdx);
            ++_commandsCount;
        }
    }

    void CommandList::setScissor(const rhi::Rect2D & area, uint32_t scissorIdx) noexcept {
        if (_state.setScissor(area, scissorIdx)) {
            _commands->create<SetScissorCommand>(area, scissorIdx);
            ++_commandsCount;
        }
    }

    void CommandList::bindPipeline(const PipelinePtr & pipeline) noexcept {
        if (_state.pipeline != pipeline.get()) {
            _commands->create<BindPipelineCommand>(pipeline);
            _state.pipeline = pipeline.get();
            ++_commandsCount;
        }
    }

    // void CommandList::bindVertexBuffer(uint32_t binding, Buffer * buffer, uint32_t offset) {
    //     _commands->create<BindVertexBufferCommand>(binding, buffer, offset);
    //     ++_commandsCount;
    // }

    // void CommandList::bindIndexBuffer(const Buffer * buffer, uint32_t offset, IndexType type) {
    //     if (_state.bindIndexBuffer(buffer, offset)) {
    //         _commands->create<BindIndexBufferCommand>(buffer, offset, type);
    //         ++_commandsCount;
    //     }
    // }

    // void CommandList::bindBuffer(uint16_t binding, const Buffer * buffer, uint8_t setId, uint32_t offset, uint32_t range) {
    //     // no check it with state, correct check into record function!
    //     _commands->create<BindBufferCommand>(binding, buffer, setId, offset, range);
    //     ++_commandsCount;
    // }

/////////////////////// shared versions
    void CommandList::bindVertexBuffer(uint32_t binding, const BufferPtr & buffer, uint32_t offset) noexcept {
        // todo: use _state to change only right buffers!
        _resources.push_back(buffer);
        _commands->create<BindVertexBufferCommand>(binding, buffer.get(), offset);
        ++_commandsCount;
    }

    void CommandList::bindIndexBuffer(const BufferPtr & buffer, uint32_t offset, IndexType type) noexcept {
        if (_state.bindIndexBuffer(buffer.get(), offset)) {
            _resources.push_back(buffer);
            _commands->create<BindIndexBufferCommand>(buffer.get(), offset, type);
            ++_commandsCount;
        }
    }

    void CommandList::bindBuffer(uint16_t binding, const BufferPtr & buffer, uint8_t setId, uint32_t offset, uint32_t range) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(buffer);
        _commands->create<BindBufferCommand>(binding, buffer.get(), setId, offset, range);
        ++_commandsCount;
    }

    void CommandList::bindImage(uint16_t binding, const ImagePtr & image, std::optional<rhi::SamplerValue> sampler, uint8_t setId) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(image);
        _commands->create<BindImageSamplerCommand>(binding, image.get(), sampler, setId);
        ++_commandsCount;
    }

    void CommandList::bindSampler(uint16_t binding, const rhi::SamplerValue & sampler, uint8_t setId) noexcept {
        // no check it with state, correct check into record function!
        _commands->create<BindImageSamplerCommand>(binding, nullptr, sampler, setId);
        ++_commandsCount;
    }
///////////////////////

    // draw commands
    void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) noexcept {
        _commands->create<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance);
        ++_commandsCount;
    }

    void CommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, 
        uint32_t firstInstance) noexcept {
        _commands->create<DrawIndexedCommand>(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        ++_commandsCount;
    }

    // compute
    void CommandList::dispatch(uint16_t groupCountX, uint16_t groupCountY, uint16_t groupCountZ) noexcept {
        _commands->create<DispatchCommand>(groupCountX, groupCountY, groupCountZ);
        ++_commandsCount;
    }

    // query pool
    void CommandList::resetQueryPool(QueryPool *pool, uint16_t firstQuery, uint16_t queryCount) noexcept {
        _commands->create<ResetQueryPoolCommand>(pool, firstQuery, queryCount);
        ++_commandsCount;
    }

    void CommandList::writeTimestamp(QueryPool *pool, PipelineStageFlags stage, uint16_t query) noexcept {
        _commands->create<WriteTimestampCommand>(pool, stage, query);
        ++_commandsCount;
    }

    // update data
    void CommandList::updateBuffer(BufferPtr & dst, uint32_t dstOffset, uint32_t dataSize, const void* pData) noexcept {
        _resources.push_back(dst);
        _commands->create<UpdateBufferCommand>(dst.get(), dstOffset, dataSize, pData);
        ++_commandsCount;
    }

    // copy commands
    void CommandList::copyBuffer2Buffer(const BufferPtr & src, BufferPtr & dst, const CopyBufferInfo & copyInfo, bool autoBarriers) noexcept {
        _resources.push_back(src);
        _resources.push_back(dst);
        _commands->create<CopyBuffer2BufferCommand>(src.get(), dst.get(), copyInfo, autoBarriers);
        ++_commandsCount;
    }

    void CommandList::copyBuffer2Image(const BufferPtr & src, ImagePtr & dst, const CopyBufferImageInfo & copyInfos, bool autoBarriers) noexcept {
        _resources.push_back(src);
        _resources.push_back(dst);
        _commands->create<CopyBuffer2ImageCommand>(src.get(), dst.get(), copyInfos, autoBarriers);
        ++_commandsCount;
    }

    // manual barriers
    void CommandList::insertBarriers() noexcept {
        // todo!
        _commands->create<PipelineBarriersCommand>();
        ++_commandsCount;
    }
}