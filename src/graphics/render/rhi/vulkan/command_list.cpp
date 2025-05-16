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
    
    CommandList::CommandList(rhi::runtime::CommandPool * commandPool, rhi::CommandBufferLevel level) :
        _pool(static_cast<vulkan::CommandPool*>(commandPool)),
        _commandbuffer{*_pool, level} {
    }

    void CommandList::resetState() noexcept {
        _barriers.reset();
    }

    void CommandList::record(uint8_t frameIdx) noexcept {
        using namespace rhi::runtime;

        constexpr uint8_t kMaxDescriptorsSets = 4u;

        std::byte * memory = _commands.memory().data();
        size_t totalMem = 0u;

        auto & device = const_cast<Device&>(_pool->getDevice()); // todo remove const_cast
        const Pipeline * currentPipeline = nullptr;
        std::array<DescriptorSetUpdater, kMaxDescriptorsSets> descriptorSetUpdaters;

        uint8_t changedSets = 0u;
        uint8_t changeDynamicBufferSets = 0u;
        std::array<VkDescriptorSet, kMaxDescriptorsSets> descriptorSets;

        const auto applyDescriptorSets = [&]() noexcept {
            uint8_t firstSet = 0xffu;
            dynamic_array<uint32_t, kMaxDescriptorsSets * DescriptorSetUpdater::kDescriptorsOnStackLimit> dynamicOffsets;

            for (uint8_t set = 0u; set < kMaxDescriptorsSets; ++set) {
                const bool changeDynamicOffset = ((changeDynamicBufferSets >> set) & 1u) == 1u;
                const bool changeSet = ((changedSets >> set) & 1u) == 1u;

                if (changeDynamicOffset) {
                    // fill dynamicOffsets
                    descriptorSetUpdaters[set].fillDynamicOffsets(dynamicOffsets);

                    if (firstSet == 0xffu) {
                        firstSet = set;
                    }
                }

                if (changeSet) {
                    const auto * setLayout = currentPipeline->getProgram()->getLayout(set);
                    auto & pool = device.getDescriptorPoolManager().getCompatiblePool(*setLayout, frameIdx);
                    const auto descriptorSet = pool.allocateDescriptorSet(*setLayout);
                    descriptorSetUpdaters[set].flush(descriptorSet);

                    descriptorSets[set] = descriptorSet();
                    if (firstSet == 0xffu) {
                        firstSet = set;
                    }
                } 
                
                if (firstSet != 0xffu && !changeDynamicOffset && !changeSet) {
                    _commandbuffer.bindDescriptorSets(std::span{descriptorSets.begin() + firstSet, descriptorSets.begin() + set},
                        dynamicOffsets.view(), firstSet);
                    firstSet = 0xffu;
                    dynamicOffsets.clear();
                }
            }

            if (firstSet != 0xffu) {
                _commandbuffer.bindDescriptorSets(std::span{descriptorSets.begin() + firstSet, descriptorSets.end()}, 
                    dynamicOffsets.view(), firstSet);
            }

            changedSets = 0u;
            changeDynamicBufferSets = 0u;
        };

        while ((_commandsCount--) > 0u) {
            Command * cmd = reinterpret_cast<Command*>(memory);
            switch (cmd->cmdType) {
            case CommandType::eBegin:
            {
                const auto * command = static_cast<BeginCommand*>(cmd);
                _commandbuffer.begin(command->flags);
                currentPipeline = nullptr;
                changedSets = 0u;
                changeDynamicBufferSets = 0u;
                resetState();
            }
                break;
            case CommandType::eEnd:
            {
                _barriers.flush(_commandbuffer());
                _commandbuffer.end();
            }
                break;
            case CommandType::eReset: 
            {
                const auto * command = static_cast<ResetCommand*>(cmd);
                _commandbuffer.reset(command->flags);
                currentPipeline = nullptr;
                changedSets = 0u;
                changeDynamicBufferSets = 0u;
                resetState();
            }
                break;
            case CommandType::eBeginRenderPass:
            {
                _barriers.flush(_commandbuffer());
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
                _barriers.flush(_commandbuffer());
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
                    changeDynamicBufferSets = 0u;
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
                    if (const auto * setLayout = currentPipeline->getLayout(setId)) {
                        descriptorSetUpdaters[setId] = DescriptorSetUpdater{device, *setLayout};
                    }
                }

                if (descriptorSetUpdaters[setId]) {
                    
                    if (command->buffer) {
                        const auto bufferUpdateResult = descriptorSetUpdaters[setId].updateBuffer(static_cast<const Buffer*>(command->buffer), 
                            command->binding, command->offset, command->range);
                        
                        switch (bufferUpdateResult) {
                            case DescriptorSetUpdater::BufferUpdateResult::eChangeBufferOrOffset:
                                changedSets |= 1u << setId;
                                break;
                            case DescriptorSetUpdater::BufferUpdateResult::eChangeDynamicOffset:
                                changeDynamicBufferSets |= 1u << setId;
                                break;
                            case DescriptorSetUpdater::BufferUpdateResult::eChangeBufferAndDynamicOffset:
                                changedSets |= 1u << setId;
                                changeDynamicBufferSets |= 1u << setId;
                                break;
                            default: break;
                        }
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

                if (command->autoBarriers) {
                    vkSrcBuffer.transition(_barriers, 
                        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT, 
                        command->info.dstOffset, command->info.size);

                    vkDstBuffer.transition(_barriers,
                        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT, 
                        command->info.dstOffset, command->info.size
                    );

                    _barriers.flush(_commandbuffer());
                }

                _commandbuffer.copyBuffer2Buffer(vkSrcBuffer, vkDstBuffer, copyInfos);

                if (command->autoBarriers) {
                    auto const usage = vkSrcBuffer.usage();
                    if ((usage & rhi::BufferUsageFlags::eVertex_Buffer) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                        vkDstBuffer.transition(_barriers,
                            dstStage,
                            VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, 
                            command->info.dstOffset, command->info.size
                        );
                    }

                    if ((usage & rhi::BufferUsageFlags::eIndex_Buffer) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                        vkDstBuffer.transition(_barriers,
                            dstStage,
                            VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT, 
                            command->info.dstOffset, command->info.size
                        );
                    }

                    if ((usage & rhi::BufferUsageFlags::eUniform_Buffer) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                                VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        vkDstBuffer.transition(_barriers,
                            dstStage,
                            VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT, 
                            command->info.dstOffset, command->info.size
                        );
                    }    

                    if ((usage & (rhi::BufferUsageFlags::eStorage_Buffer | 
                                    rhi::BufferUsageFlags::eUniform_Texel_Buffer | 
                                    rhi::BufferUsageFlags::eStorage_Texel_Buffer)) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                            VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        vkDstBuffer.transition(_barriers,
                            dstStage,
                            VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT, 
                            command->info.dstOffset, command->info.size
                        );
                    }

                    _barriers.flush(_commandbuffer()); 
                }
            }
                break;
            case CommandType::ePipelineBarriers:
                // todo!
                _barriers.flush(_commandbuffer());
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