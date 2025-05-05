#include "command_list.h"

#include "device.h"
#include "framebuffer.h"
#include "gpu_program.h"
#include "pipeline.h"
#include "buffer.h"
#include "vk_types.h"
#include "descriptor_set.h"
#include "descriptor_set_updater.h"
#include "descriptor_pool_manager.h"

#include "graphics/render/rhi/common/commands.h"

#include "utils/dynamic_array.h"

#include <array>
#include <cassert>
#include <utility>

namespace engine::vulkan::runtime {
    constexpr uint8_t kBufferBarriersReserveCapacity = 8u;

    CommandList::CommandList(rhi::runtime::CommandPool * commandPool, rhi::CommandBufferLevel level) :
        _pool(static_cast<vulkan::CommandPool*>(commandPool)),
        _commandbuffer{*_pool, level} {
    }

    void CommandList::addBufferBarrier(
        VkPipelineStageFlags srcStageMask, 
        VkPipelineStageFlags dstStageMask, 
        VkDependencyFlags dependencyFlags, 
        VkBufferMemoryBarrier barrier) {

        if (_srcStageMask != srcStageMask || _dstStageMask != dstStageMask || _dependencyFlags != dependencyFlags) {
            flushBarriers();

            _srcStageMask = srcStageMask;
            _dstStageMask = dstStageMask;
            _dependencyFlags = dependencyFlags;
        }

        _bufferBarriers.push_back(barrier);
    }

    void CommandList::flushBarriers() noexcept {
        if (_bufferBarriers.empty()) {
            return;
        }

        vkCmdPipelineBarrier(
            _commandbuffer(), 
            _srcStageMask, _dstStageMask, _dependencyFlags, 
            0u, nullptr, 
            _bufferBarriers.size(), _bufferBarriers.data(), 
            0u, nullptr
        );

        _bufferBarriers.clear();
        _bufferBarriers.reserve(kBufferBarriersReserveCapacity);
    }

    void CommandList::resetState() noexcept {
        _srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        _dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        _dependencyFlags = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;
        _bufferBarriers.clear();
        _bufferBarriers.reserve(kBufferBarriersReserveCapacity);
    }

    void CommandList::flush(uint8_t frameIdx) noexcept {
        using namespace rhi::runtime;

        constexpr uint8_t kMaxDescriptorsSets = 4u;

        std::byte * memory = _commands.memory().data();
        size_t totalMem = 0u;

        auto & device = const_cast<Device&>(_pool->getDevice()); // todo remove const_cast
        const Pipeline * currentPipeline = nullptr;
        std::array<DescriptorSetUpdater, kMaxDescriptorsSets> descriptorSetUpdaters;
        std::array<DescriptorSet, kMaxDescriptorsSets> descriptorSets;
        uint8_t changedSets = 0u;

        const auto applyDescriptorSets = [&]() noexcept {
            std::array<VkDescriptorSet, kMaxDescriptorsSets> descriptorSets;
            uint8_t firstSet = 0xffu;

            for (uint8_t set = 0u; set < kMaxDescriptorsSets; ++set) {
                if (((changedSets >> set) & 1u) == 1u) {
                    const auto * setLayout = currentPipeline->getProgram()->getLayout(set);
                    auto & pool = device.getDescriptorPoolManager().getCompatiblePool(*setLayout, frameIdx);
                    const auto descriptorSet = pool.allocateDescriptorSet(*setLayout);
                    descriptorSetUpdaters[set].flush(descriptorSet);

                    descriptorSets[set] = descriptorSet();
                    if (firstSet == 0xffu) {
                        firstSet = set;
                    }
                } else if (firstSet != 0xffu) {
                    _commandbuffer.bindDescriptorSets(std::span{descriptorSets.begin() + firstSet, descriptorSets.begin() + set});
                    firstSet = 0xffu;
                }
            }

            if (firstSet != 0xffu) {
                _commandbuffer.bindDescriptorSets(std::span{descriptorSets.begin() + firstSet, descriptorSets.end()});
            }
        };

        while ((_commandsCount--) > 0u) {
            Command * cmd = reinterpret_cast<Command*>(memory);
            switch (cmd->cmdType) {
            case CommandType::eBegin:
            {
                _commandbuffer.begin();
                currentPipeline = nullptr;
                changedSets = 0u;
                resetState();
            }
                break;
            case CommandType::eEnd:
            {
                flushBarriers();
                _commandbuffer.end();
            }
                break;
            case CommandType::eReset: 
            {
                const auto * command = static_cast<ResetCommand*>(cmd);
                _commandbuffer.reset(command->flags);
                currentPipeline = nullptr;
                changedSets = 0u;
                resetState();
            }
                break;
            case CommandType::eBeginRenderPass:
            {
                flushBarriers();
                const auto * command = static_cast<BeginRenderPassCommand*>(cmd);

                dynamic_array<VkClearValue, 4u> clearValues;
                clearValues.reserve(command->clearValues.size());
                auto clearValuesView = clearValues.view();
                auto comandClearValuesView = command->clearValues.view();

                uint8_t i = 0u;
                for (auto & value : clearValuesView) {
                    value = utils::convert2vk(comandClearValuesView[i++]);
                }

                _commandbuffer.beginRenderPass(
                    static_cast<VkRenderPass>(command->renderPass), 
                    static_cast<const vulkan::FrameBuffer &>(*command->framebuffer),
                    clearValuesView,
                    command->area
                );

                command->destroy();
            }
                break;
            case CommandType::eEndRenderPass:
            {
                _commandbuffer.endRenderPass();
            }
                break;
            case CommandType::eNextSubpass:
            {
                const auto * command = static_cast<NextSubpassCommand*>(cmd);
                _commandbuffer.nextSubpass(command->contents);
            }
                break;
            case CommandType::eExecuteCommands:
            {
                flushBarriers();
                const auto * command = static_cast<ExecuteCommandsCommand*>(cmd);
                // _commandbuffer.executeCommands();// todo!
            }
                break;
            case CommandType::eSetViewport:
            {
                const auto * command = static_cast<SetViewportCommand*>(cmd);
                _commandbuffer.setViewport(command->x, command->y, command->w, command->h, 
                    command->minDepth, command->maxDepth, command->viewportIdx);
            }
                break;
            case CommandType::eSetScissor:
            {
                const auto * command = static_cast<SetScissorCommand*>(cmd);
                _commandbuffer.setScissorRect(command->area, command->scissorIdx);
            }
                break;
            case CommandType::eBindPipeline:
            {
                const auto * command = static_cast<BindPipelineCommand*>(cmd);
                const auto * pipeline = static_cast<const Pipeline *>(command->pipeline);
                _commandbuffer.bindPipeline(*pipeline);
                
                if (currentPipeline == nullptr || currentPipeline->getProgram() != pipeline->getProgram()) {
                    for (auto & descriptorSetUpdater : descriptorSetUpdaters) {
                        descriptorSetUpdater.invalidate();
                    }
                    changedSets = 0u;
                }

                currentPipeline = static_cast<const Pipeline *>(command->pipeline);
            }
                break;
            case CommandType::eBindVertexBuffer:
            {
                const auto * command = static_cast<BindVertexBufferCommand*>(cmd);
                // todo fill tmp structs and bindVertexBuffers before draw command!
                _commandbuffer.bindVertexBuffer(command->binding, static_cast<const Buffer&>(*command->buffer)(), command->offset);
            }
                break;
            case CommandType::eBindIndexBuffer:
            {
                const auto * command = static_cast<BindIndexBufferCommand*>(cmd);
                _commandbuffer.bindIndexBuffer(static_cast<const Buffer&>(*command->buffer)(), command->offset, command->type);
            }
                break;
            case CommandType::eBindResource:
            {
                const auto * command = static_cast<BindResourceCommand*>(cmd);
                const auto setId = command->setId;
                assert(setId < kMaxDescriptorsSets);

                if (!descriptorSetUpdaters[setId]) {
                    if (const auto * setLayout = currentPipeline->getProgram()->getLayout(setId)) {
                        descriptorSetUpdaters[setId] = DescriptorSetUpdater{device, *setLayout};
                    }
                }

                if (descriptorSetUpdaters[setId]) {
                    changedSets |= 1u << setId;

                    if (command->buffer) {
                        descriptorSetUpdaters[setId].updateBuffer(static_cast<const Buffer*>(command->buffer), command->binding);
                    }

                    //...
                    // if (command->image) {

                    // }
                }
            }
                break;
            case CommandType::eDraw:
            {
                applyDescriptorSets();

                const auto * command = static_cast<DrawCommand*>(cmd);
                _commandbuffer.draw(command->vertexCount, command->instanceCount, command->firstVertex, command->firstInstance);
            }
                break;
            case CommandType::eDrawIndexed:
            {
                applyDescriptorSets();

                const auto * command = static_cast<DrawIndexedCommand*>(cmd);
                _commandbuffer.drawIndexed(command->indexCount, command->instanceCount, command->firstIndex, 
                    command->vertexOffset, command->firstInstance);
            }
                break;
            case CommandType::eCopyBuffer2Buffer:
            {
                const auto * command = static_cast<CopyBuffer2BufferCommand*>(cmd);
                std::array copyInfos{utils::convert2vk(command->info)};

                auto & vkSrcBuffer = *static_cast<Buffer*>(command->src);
                auto & vkDstBuffer = *static_cast<Buffer*>(command->dst);

                auto const usage = vkSrcBuffer.usage();
                if (vkSrcBuffer.isShadersWriteAllowed() && ((usage & (rhi::BufferUsageFlags::eStorage_Buffer | 
                    rhi::BufferUsageFlags::eStorage_Texel_Buffer)) != rhi::BufferUsageFlags::eNone)) {
                    auto const srcStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | 
                                          VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // use other shader stages?
                    auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                    auto const dependency = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;

                    addBufferBarrier(srcStage, dstStage, dependency, vkDstBuffer.transition(
                        VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT, 
                        VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT, 
                        command->info.dstOffset, command->info.size));
                }

                _commandbuffer.copyBuffer2Buffer(vkSrcBuffer, vkDstBuffer, copyInfos);

                {
                    auto const srcStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT;
                    auto const dependency = VkDependencyFlagBits::VK_DEPENDENCY_BY_REGION_BIT;

                    auto const usage = vkDstBuffer.usage();
                    if ((usage & rhi::BufferUsageFlags::eVertex_Buffer) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                        addBufferBarrier(srcStage, dstStage, dependency, vkDstBuffer.transition(
                            VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT, 
                            VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, 
                            command->info.dstOffset, command->info.size));
                    }

                    if ((usage & rhi::BufferUsageFlags::eIndex_Buffer) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                        addBufferBarrier(srcStage, dstStage, dependency, vkDstBuffer.transition(
                            VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT, 
                            VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT, 
                            command->info.dstOffset, command->info.size));
                    }

                    if ((usage & rhi::BufferUsageFlags::eUniform_Buffer) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                              VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        addBufferBarrier(srcStage, dstStage, dependency, vkDstBuffer.transition(
                            VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT, 
                            VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT, 
                            command->info.dstOffset, command->info.size));
                    }    

                    if ((usage & (rhi::BufferUsageFlags::eStorage_Buffer | 
                                  rhi::BufferUsageFlags::eUniform_Texel_Buffer | 
                                  rhi::BufferUsageFlags::eStorage_Texel_Buffer)) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                              VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        addBufferBarrier(srcStage, dstStage, dependency, vkDstBuffer.transition(
                            VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT, 
                            VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT, 
                            command->info.dstOffset, command->info.size));
                    }    
                }    
            }
                break;
            case CommandType::eUndefined:
                break;    
            default:
                break;
            }
            
            totalMem += cmd->size;
            if (totalMem >= kStackSize) {
                assert(false);
                break;
            }
    
            memory += cmd->size;
        }
    }
}