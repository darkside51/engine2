#include "command_list.h"

namespace engine::rhi::runtime {

    constexpr bool kStatisticsEnable = true;

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

        _pipelineType = PipelineBindPoint::eCompute;
        // for (auto & pass : _passes) {
        for (auto & pass : _passes.view()) {
            pass.buffers.clear();
            pass.images.clear();
        }
        _passes.clear();
        // _passes.reserve(8u);

        // _passId = 0u;
        _currentPass = nullptr;

        _resources.clear();
        _resources.reserve(1024u);

        _tmpBytes.clear();
        _tmpBytes.reserve(16u);

        if constexpr (kStatisticsEnable) {
            _statistics.reset();
        }
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
        stl::span<const ClearValue> clearValues, const Rect2D & area) noexcept {
        _resources.push_back(framebuffer);
        _commands->create<BeginRenderPassCommand>(renderPass, framebuffer, clearValues, area);
        
        if constexpr (kStatisticsEnable) {
            ++_statistics.renderPasses;
        }

        _currentPass = &_passes.emplace_back(Pass::Type::eRender, _commandsCount);
        ++_commandsCount;
    }

    void CommandList::endRenderPass() noexcept {
        _commands->create<EndRenderPassCommand>();
        _currentPass = nullptr;
        ++_commandsCount;
    }

    void CommandList::nextSubpass(SubpassContents contents,
        stl::span<ImageTransitionInfo> imageTransitionInfo,
        stl::span<BufferTransitionInfo> bufferTransitionInfo) noexcept {
        _commands->create<NextSubpassCommand>(contents, imageTransitionInfo, bufferTransitionInfo);

        if constexpr (kStatisticsEnable) {
            ++_statistics.renderPasses;
        }

        _currentPass = &_passes.emplace_back(Pass::Type::eRender, _commandsCount);
        ++_commandsCount;
    }

    void CommandList::executeCommands(stl::span<const CommandList*> secondaryCommandLists) noexcept {
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

    bool CommandList::bindPipeline(const PipelinePtr & pipeline) noexcept {
        if (_state.pipeline != pipeline.get()) {
            _commands->create<BindPipelineCommand>(pipeline);
            _state.pipeline = pipeline.get();

            if constexpr (kStatisticsEnable) {
            ++_statistics.binPipelines;
            }

            _pipelineType = pipeline->bindPoint;

            if (_currentPass == nullptr || pipeline->bindPoint == PipelineBindPoint::eCompute && _currentPass->type == Pass::Type::eTransfer) {
                _currentPass = &_passes.emplace_back(Pass::Type::eCompute, _commandsCount);
            }
            ++_commandsCount;
            return true;
        }
        return false;
    }

    bool CommandList::bindPipeline(const Pipeline & pipeline) noexcept {
        if (_state.pipeline != &pipeline) {
            _commands->create<BindPipelineCommand>(pipeline);
            _state.pipeline = &pipeline;

            if constexpr (kStatisticsEnable) {
            ++_statistics.binPipelines;
            }

            _pipelineType = pipeline.bindPoint;

            if (_currentPass == nullptr || pipeline.bindPoint == PipelineBindPoint::eCompute && _currentPass->type == Pass::Type::eTransfer) {
                _currentPass = &_passes.emplace_back(Pass::Type::eCompute, _commandsCount);
            }
            ++_commandsCount;
            return true;
        }
        return false;
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
        if (_state.bindVertexBuffer(binding, buffer.get(), offset)) {
            _resources.push_back(buffer);
            _commands->create<BindVertexBufferCommand>(binding, buffer.get(), offset);

            _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eVertex_Buffer, _pipelineType, offset);
            ++_commandsCount;
        }
    }

    void CommandList::bindIndexBuffer(const BufferPtr & buffer, uint32_t offset, IndexType type) noexcept {
        if (_state.bindIndexBuffer(buffer.get(), offset)) {
            _resources.push_back(buffer);
            _commands->create<BindIndexBufferCommand>(buffer.get(), offset, type);

            _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eIndex_Buffer, _pipelineType, offset);
            ++_commandsCount;
        }
    }

    void CommandList::bindBuffer(uint16_t binding, const BufferPtr & buffer, uint8_t setId, uint64_t offset, uint64_t range) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(buffer);
        _commands->create<BindBufferCommand>(binding, buffer.get(), setId, offset, range, false);

        _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eUniform_Buffer, _pipelineType, offset, range); // not only uniform, correct usage check into implementation of commandlist
        ++_commandsCount;
    }

    void CommandList::bindStorageBuffer(uint16_t binding, const BufferPtr & buffer, uint8_t setId, uint64_t offset, uint64_t range) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(buffer);
        _commands->create<BindBufferCommand>(binding, buffer.get(), setId, offset, range, false);

        _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eStorage_Buffer, _pipelineType, offset, range); // not only uniform, correct usage check into implementation of commandlist
        ++_commandsCount;
    }

    void CommandList::bindBufferDynamic(uint16_t binding, const BufferPtr & buffer, uint8_t setId, uint64_t offset, uint64_t range) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(buffer);
        _commands->create<BindBufferCommand>(binding, buffer.get(), setId, offset, range, true);

        _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eUniform_Buffer, _pipelineType, offset, range); // not only uniform, correct usage check into implementation of commandlist
        ++_commandsCount;
    }

    void CommandList::bindStorageBufferDynamic(uint16_t binding, const BufferPtr & buffer, uint8_t setId, uint64_t offset, uint64_t range) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(buffer);
        _commands->create<BindBufferCommand>(binding, buffer.get(), setId, offset, range, true);

        _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eStorage_Buffer, _pipelineType, offset, range); // not only uniform, correct usage check into implementation of commandlist
        ++_commandsCount;
    }

    void CommandList::bindTexelBuffer(uint16_t binding, const BufferPtr & buffer, Format format, uint8_t setId, uint64_t offset, uint64_t range) noexcept {
        _resources.push_back(buffer);
        _commands->create<BindTexelBufferCommand>(binding, buffer.get(), format, setId, offset, range);

        _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eUniform_Texel_Buffer, _pipelineType, offset, range); // ...
        ++_commandsCount;
    }

    void CommandList::bindStorageTexelBuffer(uint16_t binding, const BufferPtr & buffer, Format format, uint8_t setId, uint64_t offset, uint64_t range) noexcept {
        _resources.push_back(buffer);
        _commands->create<BindTexelBufferCommand>(binding, buffer.get(), format, setId, offset, range);

        _currentPass->buffers.emplace_back(buffer.get(), BufferUsageFlags::eStorage_Texel_Buffer, _pipelineType, offset, range); // ...
        ++_commandsCount;
    }

    void CommandList::bindImage(uint16_t binding, const ImagePtr & image, std::optional<rhi::SamplerValue> sampler, uint8_t setId, 
        ImageAspectFlags aspectFlags, ImageViewType viewType) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(image);
        _commands->create<BindImageSamplerCommand>(viewType, aspectFlags, binding, image.get(), sampler, setId);

        _currentPass->images.emplace_back(image.get(), ImageUsageFlags::eSampled, _pipelineType);  // ...
        ++_commandsCount;
    }

    void CommandList::bindStorageImage(uint16_t binding, const ImagePtr & image, std::optional<rhi::SamplerValue> sampler, uint8_t setId, 
        ImageAspectFlags aspectFlags, ImageViewType viewType) noexcept {
        // no check it with state, correct check into record function!
        _resources.push_back(image);
        _commands->create<BindImageSamplerCommand>(viewType, aspectFlags, binding, image.get(), sampler, setId);

        _currentPass->images.emplace_back(image.get(), ImageUsageFlags::eStorage, _pipelineType);  // ...
        ++_commandsCount;
    }

    void CommandList::bindSampler(uint16_t binding, const rhi::SamplerValue & sampler, uint8_t setId) noexcept {
        // no check it with state, correct check into record function!
        _commands->create<BindImageSamplerCommand>(ImageViewType::eType_Undefined, binding, nullptr, sampler, setId);
        ++_commandsCount;
    }

    void CommandList::pushConstants(shaders::Stages stages, uint32_t offset, uint32_t size, const void * bytes) noexcept {
        _commands->create<PushConstantsCommand>(stages, offset, size, bytes);
        ++_commandsCount;
    }
///////////////////////

    // draw commands
    void CommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) noexcept {
        _commands->create<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance);

        if constexpr (kStatisticsEnable) {
            ++_statistics.drawCalls;
        }
        ++_commandsCount;
    }

    void CommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, 
        uint32_t firstInstance) noexcept {
        _commands->create<DrawIndexedCommand>(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

        if constexpr (kStatisticsEnable) {
            ++_statistics.drawCalls;
        }
        ++_commandsCount;
    }

    // compute
    void CommandList::dispatch(uint16_t groupCountX, uint16_t groupCountY, uint16_t groupCountZ) noexcept {
        _commands->create<DispatchCommand>(groupCountX, groupCountY, groupCountZ);

        if constexpr (kStatisticsEnable) {
            ++_statistics.dispatchCalls;
            _statistics.dispatchGroups += groupCountX * groupCountY * groupCountZ;
        }
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
  
        if constexpr (kStatisticsEnable) {
            ++_statistics.updateBuffers;
        }

        if (_currentPass == nullptr) {
            _currentPass = &_passes.emplace_back(Pass::Type::eTransfer, _commandsCount);
        }

        _currentPass->buffers.emplace_back(dst.get(), BufferUsageFlags::eTransfer_Dst, _pipelineType); // ...
        ++_commandsCount;
    }

    void CommandList::updateBuffer(BufferPtr & dst, uint32_t dstOffset, stl::vector<std::byte> && data) noexcept {
        const auto & tmpData = _tmpBytes.emplace_back(std::move(data));
        updateBuffer(dst, dstOffset, tmpData.size(), tmpData.data());
    }

    // copy commands
    void CommandList::copyBuffer2Buffer(const BufferPtr & src, BufferPtr & dst, const CopyBufferInfo & copyInfo) noexcept {
        _resources.push_back(src);
        _resources.push_back(dst);
        _commands->create<CopyBuffer2BufferCommand>(src.get(), dst.get(), copyInfo);

        if constexpr (kStatisticsEnable) {
            ++_statistics.copyCommands;
        }

        if (_currentPass == nullptr) {
            _currentPass = &_passes.emplace_back(Pass::Type::eTransfer, _commandsCount);
        }

        _currentPass->buffers.emplace_back(src.get(), BufferUsageFlags::eTransfer_Src, _pipelineType); // ...
        _currentPass->buffers.emplace_back(dst.get(), BufferUsageFlags::eTransfer_Dst, _pipelineType); // ...
        ++_commandsCount;
    }

    void CommandList::copyBuffer2Image(const BufferPtr & src, ImagePtr & dst, const CopyBufferImageInfo & copyInfos) noexcept {
        _resources.push_back(src);
        _resources.push_back(dst);
        _commands->create<CopyBuffer2ImageCommand>(src.get(), dst.get(), copyInfos);

        if constexpr (kStatisticsEnable) {
            ++_statistics.copyCommands;
        }

        if (_currentPass == nullptr) {
            _currentPass = &_passes.emplace_back(Pass::Type::eTransfer, _commandsCount);
        }

        _currentPass->buffers.emplace_back(src.get(), BufferUsageFlags::eTransfer_Src, _pipelineType); // ...
        _currentPass->images.emplace_back(dst.get(), ImageUsageFlags::eTransfer_Dst, _pipelineType); // ...
        ++_commandsCount;
    }

    void CommandList::blitImage(const ImagePtr & src, ImagePtr & dst, const BlitImageInfo & info) noexcept {
        _resources.push_back(src);
        _resources.push_back(dst);
        _commands->create<BlitImageCommand>(src.get(), dst.get(), info);

        if constexpr (kStatisticsEnable) {
            ++_statistics.copyCommands;
        }

        if (_currentPass == nullptr) {
            _currentPass = &_passes.emplace_back(Pass::Type::eTransfer, _commandsCount);
            _currentPass->images.emplace_back(src.get(), ImageUsageFlags::eTransfer_Src, _pipelineType); // ...
            if (src.get() != dst.get()) {
                _currentPass->images.emplace_back(dst.get(), ImageUsageFlags::eTransfer_Dst, _pipelineType); // ...
            }
        }

        // _currentPass->images.emplace_back(dst.get(), ImageUsageFlags::eTransfer_Dst, _pipelineType); // ...
        ++_commandsCount;
    }

    void CommandList::copyImage2Image(ImagePtr & src, ImagePtr & dst, const CopyImage2ImageInfo & info) noexcept {
        _resources.push_back(src);
        _resources.push_back(dst);
        _commands->create<CopyImage2ImageCommand>(src.get(), dst.get(), info);

        if constexpr (kStatisticsEnable) {
            ++_statistics.copyCommands;
        }

        if (_currentPass == nullptr) {
            _currentPass = &_passes.emplace_back(Pass::Type::eTransfer, _commandsCount);
        }

        _currentPass->images.emplace_back(src.get(), ImageUsageFlags::eTransfer_Src, _pipelineType); // ...
        _currentPass->images.emplace_back(dst.get(), ImageUsageFlags::eTransfer_Dst, _pipelineType); // ...
        ++_commandsCount;
    }

    void CommandList::prepareImage2HostRead(ImagePtr & img) noexcept {
        _resources.push_back(img);
        _commands->create<PrepareImage2HostRead>(img.get());

        if constexpr (kStatisticsEnable) {
            ++_statistics.otherCommands;
        }

        if (_currentPass == nullptr) {
            _currentPass = &_passes.emplace_back(Pass::Type::eTransfer, _commandsCount);
        }
        ++_commandsCount;
    }

    // manual barriers
    void CommandList::insertBarriers() noexcept {
        // todo!
        _commands->create<PipelineBarriersCommand>();
        ++_commandsCount;
    }
}