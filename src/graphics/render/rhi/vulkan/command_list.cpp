#include "command_list.h"

#include "device.h"
#include "framebuffer.h"
#include "gpu_program.h"
#include "pipeline.h"
#include "buffer.h"
#include "image.h"
#include "vk_types.h"
#include "descriptor_set.h"
#include "descriptor_set_updater.h"
#include "descriptor_pool_manager.h"
#include "renderpass.h"
#include "query_pool.h"

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

    void CommandList::applyBarriers(const rhi::runtime::Pass & pass, RenderPass* renderPass, stl::span<rhi::FrameBufferAttachment> attachments) {
        { // attachments barriers
            uint8_t attachmentIdx = 0u;
            for (const auto & attachment : attachments) {
                auto * image = static_cast<Image*>(attachment.image);
                const auto usage = attachment.image->usage();

                // need check layers and levels count?
                if ((usage & rhi::ImageUsageFlags::eDepth_Stencil_Attachment) != rhi::ImageUsageFlags::eNone) {
                    auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

                    const auto aspectFlags = attachment.aspect;

                    VkImageLayout dstLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
                    if ((aspectFlags & rhi::ImageAspectFlags::eDepthStencil) != rhi::ImageAspectFlags::eNone) {
                        dstLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    } else if ((aspectFlags & rhi::ImageAspectFlags::eDepth) != rhi::ImageAspectFlags::eNone) {
                        dstLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                    } else if ((aspectFlags & rhi::ImageAspectFlags::eStencil) != rhi::ImageAspectFlags::eNone) {
                        dstLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
                    }

                    image->transition(
                        _barriers, 
                        dstLayout, 
                        dstStage, 
                        VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
                        0u, 0u, 
                        0u, 0u
                    );  
                }

                if ((usage & rhi::ImageUsageFlags::eColor_Attachment) != rhi::ImageUsageFlags::eNone) {
                    auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                    image->transition(
                        _barriers, 
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                        dstStage, 
                        VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
                        0u, 0u, 
                        0u, 0u
                    );  
                }

                // need to get finalLayout for attachment from command->renderPass
                image->assignLayout(renderPass->attachments[attachmentIdx].finalLayout, 0u, 0u, 0u, 0u);
                ++attachmentIdx;
            }
        }

        { // pass barriers
            // buffers
            const auto & buffers = pass.buffers.view();
            for (const auto & buffer : buffers) {
                auto * vkBuffer = static_cast<vulkan::Buffer*>(buffer.buffer);
                if ((buffer.flags & (rhi::BufferUsageFlags::eTransfer_Src)) != rhi::BufferUsageFlags::eNone) {
                    vkBuffer->transition(_barriers, 
                        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT,
                        buffer.offset, buffer.size
                    );
                }
                
                if ((buffer.flags & (rhi::BufferUsageFlags::eTransfer_Dst)) != rhi::BufferUsageFlags::eNone) {
                    vkBuffer->transition(_barriers,
                        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT,
                        buffer.offset, buffer.size
                    );
                }

                if ((buffer.flags & rhi::BufferUsageFlags::eVertex_Buffer) != rhi::BufferUsageFlags::eNone) {
                    auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    vkBuffer->transition(_barriers,
                        dstStage,
                        VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                        buffer.offset, buffer.size
                    );
                }

                if ((buffer.flags & rhi::BufferUsageFlags::eIndex_Buffer) != rhi::BufferUsageFlags::eNone) {
                    auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    vkBuffer->transition(_barriers,
                        dstStage,
                        VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT,
                        buffer.offset, buffer.size
                    );
                }

                if ((buffer.flags & (rhi::BufferUsageFlags::eUniform_Buffer | 
                                rhi::BufferUsageFlags::eUniform_Texel_Buffer)) != rhi::BufferUsageFlags::eNone) {
                    auto const usage = vkBuffer->usage();
                    if ((usage & (rhi::BufferUsageFlags::eUniform_Buffer | 
                                rhi::BufferUsageFlags::eUniform_Texel_Buffer)) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = pass.type == rhi::runtime::Pass::Type::eRender ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 
                                            VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        vkBuffer->transition(_barriers,
                            dstStage,
                            VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT, // correct?
                            buffer.offset, buffer.size
                        );
                    }    

                    if ((usage & (rhi::BufferUsageFlags::eStorage_Buffer | 
                                rhi::BufferUsageFlags::eStorage_Texel_Buffer)) != rhi::BufferUsageFlags::eNone) {
                        auto const dstStage = pass.type == rhi::runtime::Pass::Type::eRender ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 
                                            VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        vkBuffer->transition(_barriers,
                            dstStage,
                            VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT | VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT, // need to specify read or write?
                            buffer.offset, buffer.size
                        );
                    }

                    // rhi::BufferUsageFlags::eIndirect_Buffer
                }
            }

            // images
            const auto & images = pass.images.view();
            // size_t imn = 0u;
            for (const auto & image : images) {
                auto * vkImage = static_cast<vulkan::Image*>(image.image);
                auto const usage = vkImage->usage();
                // ++imn;
                if ((image.flags & rhi::ImageUsageFlags::eTransfer_Src) != rhi::ImageUsageFlags::eNone) {
                    vkImage->transition(
                        _barriers,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        0u, 0u, 
                        0u, 0u
                        // command->info.imageSubresource.baseArrayLayer,
                        // command->info.imageSubresource.mipLevel,
                        // command->info.imageSubresource.layerCount,
                        // 1u
                    );
                } else if ((image.flags & rhi::ImageUsageFlags::eTransfer_Dst) != rhi::ImageUsageFlags::eNone) {
                    vkImage->transition(
                        _barriers,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        0u, 0u, 
                        0u, 0u
                        // command->info.imageSubresource.baseArrayLayer,
                        // command->info.imageSubresource.mipLevel,
                        // command->info.imageSubresource.layerCount,
                        // 1u
                    );
                } else {
                    if ((usage & rhi::ImageUsageFlags::eSampled) != rhi::ImageUsageFlags::eNone) {
                        auto const dstStage = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                            VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                        vkImage->transition(
                            _barriers, 
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                            dstStage, 
                            VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT, 
                            0u, 0u, 
                            0u, 0u
                        );  
                    }
                }

                if ((usage & rhi::ImageUsageFlags::eStorage) != rhi::ImageUsageFlags::eNone) {
                    auto const dstStage = pass.type == rhi::runtime::Pass::Type::eRender ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                                                                        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT :
                                                                                        VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                    vkImage->transition(
                        _barriers, 
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                        dstStage, 
                        VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, 
                        0u, 0u, 
                        0u, 0u
                    );  
                }
            }
        }

        _barriers.flush(_commandbuffer());
    }

    void CommandList::record(uint8_t frameIdx) noexcept {
        if (_passes.empty()) {
            return;
        }

        using namespace rhi::runtime;

        std::byte * memory = _commands.memory().data();
        size_t totalMem = 0u;

        auto & device = const_cast<Device&>(_pool->getDevice()); // todo remove const_cast
        const Pipeline * currentPipeline = nullptr;
        stl::array<DescriptorSetUpdater, command_limits::kMaxDescriptorsSets> descriptorSetUpdaters;

        uint8_t changedSets = 0u;
        uint8_t changeDynamicBufferSets = 0u;
        stl::array<VkDescriptorSet, command_limits::kMaxDescriptorsSets> descriptorSets;

        const auto applyDescriptorSets = [&]() noexcept {
            uint8_t firstSet = 0xffu;
            dynamic_array<uint32_t, command_limits::kMaxDescriptorsSets * DescriptorSetUpdater::kDescriptorsOnStackLimit> dynamicOffsets;

            for (uint8_t set = 0u; set < command_limits::kMaxDescriptorsSets; ++set) {
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
                    assert(descriptorSet.set != VK_NULL_HANDLE);
                    
                    descriptorSetUpdaters[set].flush(descriptorSet);

                    descriptorSets[set] = descriptorSet();
                    if (firstSet == 0xffu) {
                        firstSet = set;
                    }
                } 
                
                if (firstSet != 0xffu && !changeDynamicOffset && !changeSet) {
                    _commandbuffer.bindDescriptorSets(stl::span{descriptorSets.begin() + firstSet, descriptorSets.begin() + set},
                        dynamicOffsets.view(), firstSet);
                    firstSet = 0xffu;
                    dynamicOffsets.clear();
                }
            }

            if (firstSet != 0xffu) {
                _commandbuffer.bindDescriptorSets(stl::span{descriptorSets.begin() + firstSet, descriptorSets.end()}, 
                    dynamicOffsets.view(), firstSet);
            }

            changedSets = 0u;
            changeDynamicBufferSets = 0u;
        };

        uint32_t passId = 0u;
       
        for (uint32_t commandsNumber = 0u; commandsNumber < _commandsCount; ++commandsNumber) {
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

                auto const & pass = _passes[passId];
                if (pass.commandNumber == commandsNumber && pass.type == Pass::Type::eTransfer) {
                    applyBarriers(pass, nullptr, {});
                    ++passId;
                }
            }
                break;
            case CommandType::eEnd:
            {
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
                const auto * command = static_cast<BeginRenderPassCommand*>(cmd);

                auto & framebuffer = static_cast<vulkan::FrameBuffer &>(*command->framebuffer);
                auto * renderPass = static_cast<RenderPass*>(command->renderPass);

                // no check _passes[passId].commandNumber == commandsNumber, result true alltimes for CommandType::eBeginRenderPass
                auto const & pass = _passes[passId];
                applyBarriers(pass, renderPass, framebuffer.attachments()); 
                ++passId;

                dynamic_array<VkClearValue, 8u> clearValues;
                clearValues.resize(command->clearValues.size());
                auto clearValuesView = clearValues.view();
                auto commandClearValuesView = command->clearValues.view();

                uint8_t i = 0u;
                for (auto & value : clearValuesView) {
                    value = utils::convert2vk(commandClearValuesView[i++]);
                }

                _commandbuffer.beginRenderPass(
                    renderPass->vkRenderPass, 
                    framebuffer,
                    clearValuesView,
                    command->area
                );
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

                // no check _passes[passId].commandNumber == commandsNumber, result true alltimes for CommandType::eBeginRenderPass
                auto const & pass = _passes[passId];
                applyBarriers(pass, nullptr, {});
                ++passId;

                _commandbuffer.nextSubpass(command->contents);
            }
                break;
            case CommandType::eExecuteCommands:
            {
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

                if (passId < _passes.size()) {
                    const auto & pass = _passes[passId];
                    if (pass.commandNumber == commandsNumber && 
                        pipeline->bindPoint == rhi::PipelineBindPoint::eCompute && 
                        pass.type == Pass::Type::eCompute) {
                        applyBarriers(pass, nullptr, {});
                        ++passId;
                    }
                }

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
            case CommandType::eBindBuffer:
            {
                const auto * command = static_cast<BindBufferCommand*>(cmd);
                const auto setId = command->setId;
                assert(setId < command_limits::kMaxDescriptorsSets);

                if (!descriptorSetUpdaters[setId]) {
                    // if (const auto * setLayout = currentPipeline->getLayout(setId)) {
                    //     descriptorSetUpdaters[setId] = DescriptorSetUpdater{device, *setLayout};
                    // } else {
                    //     assert(false); // we must use all sets in shader for correct working
                    // }
                    const auto * setLayout = currentPipeline->getLayout(setId);
                    assert(setLayout != nullptr); // we must use all sets in shader for correct working
                    descriptorSetUpdaters[setId] = DescriptorSetUpdater{device, *setLayout};
                }

                if (descriptorSetUpdaters[setId]) {
                    const auto bufferUpdateResult = descriptorSetUpdaters[setId].updateBuffer(static_cast<const Buffer*>(command->buffer), 
                        command->binding, command->offset, command->range, command->dynamic);
                    
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
            }
                break;
            case CommandType::eBindImageSampler: 
            {
                const auto * command = static_cast<BindImageSamplerCommand*>(cmd);
                const auto setId = command->setId;
                assert(setId < command_limits::kMaxDescriptorsSets);

                if (!descriptorSetUpdaters[setId]) {
                    if (const auto * setLayout = currentPipeline->getLayout(setId)) {
                        descriptorSetUpdaters[setId] = DescriptorSetUpdater{device, *setLayout};
                    }
                }

                if (descriptorSetUpdaters[setId]) {
                    auto * image = static_cast<Image*>(command->image);

                    auto view = image ? image->getImageView(
                        command->viewType, 
                        command->baseLayer, command->layersCount, 
                        command->baseLevel, command->levelsCount, 
                        command->aspectFlags
                    ) : nullptr;

                    VkSampler sampler = nullptr;
                    if (command->samplerValue) {
                        if (auto vkSampler = command->samplerValue->evaluatedSampler) {
                            sampler = static_cast<VkSampler>(vkSampler) ;
                        } else {
                            sampler = device.getSampler(command->samplerValue->info);
                        }
                    }
                    
                    VkImageLayout layout = image ? image->getLayout(command->baseLayer, command->baseLevel) : VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;

                    const auto changed = descriptorSetUpdaters[setId].updateImage(
                        {sampler, view, layout}, command->binding
                    );

                    if (changed) {
                        changedSets |= 1u << setId;
                    }
                }
            }
                break;
            case CommandType::eBindTexelBuffer: 
            {
                const auto * command = static_cast<BindTexelBufferCommand*>(cmd);
                const auto setId = command->setId;
                assert(setId < command_limits::kMaxDescriptorsSets);

                if (!descriptorSetUpdaters[setId]) {
                    const auto * setLayout = currentPipeline->getLayout(setId);
                    assert(setLayout != nullptr); // we must use all sets in shader for correct working
                    descriptorSetUpdaters[setId] = DescriptorSetUpdater{device, *setLayout};
                }

                if (descriptorSetUpdaters[setId]) {
                    const auto changed = descriptorSetUpdaters[setId].updateTexelBuffer(static_cast<Buffer*>(command->buffer), command->format, 
                        command->binding, command->offset, command->range);

                    if (changed) {
                        changedSets |= 1u << setId;
                    }
                }
            }
                break;
            case CommandType::ePushConstants:
            {
                const auto * command = static_cast<PushConstantsCommand*>(cmd);
                vkCmdPushConstants(_commandbuffer(), currentPipeline->layout, utils::convert2vk(command->stages), 
                    command->offset, command->size, command->data.data());
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
            case CommandType::eDispatch:
            {
                applyDescriptorSets();
                const auto * command = static_cast<DispatchCommand*>(cmd);
                vkCmdDispatch(_commandbuffer(), command->groupCountX, command->groupCountY, command->groupCountZ);
                // need to add DispatchBase command too?
                // vkCmdDispatchBase(_commandbuffer(), 0, 0, 0, command->groupCountX, command->groupCountY, command->groupCountZ);
            }   
                break;
            case CommandType::eResetQueryPool:
            {
                const auto * command = static_cast<ResetQueryPoolCommand*>(cmd);
                QueryPool * pool = static_cast<QueryPool*>(command->pool);
                vkCmdResetQueryPool(_commandbuffer(), pool->operator()(), command->firstQuery, command->queryCount);
            }   
                break;
            case CommandType::eWriteTimestamp:
            {
                const auto * command = static_cast<WriteTimestampCommand*>(cmd);
                QueryPool * pool = static_cast<QueryPool*>(command->pool);
                const auto stage = static_cast<VkPipelineStageFlagBits>(utils::convert2vk(command->stage));

                vkCmdWriteTimestamp(_commandbuffer(), 
                    stage, 
                    pool->operator()(), 
                    command->query);
            } 
                break;
            // case CommandType::eBeginQuery {

            // }
            //     break;
            case CommandType::eUpdateBuffer: {

                if (passId < _passes.size()) {
                    const auto & pass = _passes[passId];
                    if (pass.commandNumber == commandsNumber && pass.type == Pass::Type::eTransfer) {
                        applyBarriers(pass, nullptr, {});
                        ++passId;
                    }
                }

                const auto * command = static_cast<UpdateBufferCommand*>(cmd);
                assert(command->dataSize < command_limits::kMaxMemoryBytesToCopyCommand);
                
                const auto & vkDstBuffer = *static_cast<const Buffer*>(command->dst);
                _commandbuffer.updateBuffer(vkDstBuffer, command->dstOffset, command->dataSize, command->pData);
            }
                break;
            case CommandType::eCopyBuffer2Buffer:
            {
                if (passId < _passes.size()) {
                    const auto & pass = _passes[passId];
                    if (pass.commandNumber == commandsNumber && pass.type == Pass::Type::eTransfer) {
                        applyBarriers(pass, nullptr, {});
                        ++passId;
                    }
                }

                const auto * command = static_cast<CopyBuffer2BufferCommand*>(cmd);
                stl::array copyInfos{utils::convert2vk(command->info)};

                const auto & vkSrcBuffer = *static_cast<const Buffer*>(command->src);
                const auto & vkDstBuffer = *static_cast<const Buffer*>(command->dst);
                _commandbuffer.copyBuffer2Buffer(vkSrcBuffer, vkDstBuffer, copyInfos);
            }
                break;

            case CommandType::eCopyBuffer2Image:
            {
                if (passId < _passes.size()) {
                    const auto & pass = _passes[passId];
                    if (pass.commandNumber == commandsNumber && pass.type == Pass::Type::eTransfer) {
                        applyBarriers(pass, nullptr, {});
                        ++passId;
                    }
                }

                const auto * command = static_cast<CopyBuffer2ImageCommand*>(cmd);
                stl::array copyInfos{utils::convert2vk(command->info)};

                const auto & vkSrcBuffer = *static_cast<const Buffer*>(command->src);
                const auto & vkDstImage = *static_cast<const Image*>(command->dst);
                _commandbuffer.copyBuffer2Image(vkSrcBuffer, vkDstImage, copyInfos);
            }   
                break; 

            case CommandType::eBlitImage:
            {
                if (passId < _passes.size()) {
                    const auto & pass = _passes[passId];
                    if (pass.commandNumber == commandsNumber && pass.type == Pass::Type::eTransfer) {
                        applyBarriers(pass, nullptr, {});
                        ++passId;
                    }
                }

                const auto * command = static_cast<BlitImageCommand*>(cmd);
                const auto & commandBlitInfo = command->info;
   
                auto & vkSrcImage = *static_cast<Image*>(command->src);
                auto & vkDstImage = *static_cast<Image*>(command->dst);

                const auto srcLayout = utils::convert2vk(commandBlitInfo.srcImageLayout);
                const auto dstLayout = utils::convert2vk(commandBlitInfo.dstImageLayout);

                stl::array<VkImageBlit, 8u> vkBlitInfoArray;
                for (uint8_t i = 0u; i < commandBlitInfo.regionsCount; ++i) {
                    auto & vkBlitInfo = vkBlitInfoArray[i];
                    auto const & blitInfo = commandBlitInfo.regions[i];

                    vkSrcImage.transition(
                        _barriers, 
                        srcLayout, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_ACCESS_TRANSFER_READ_BIT, 
                        0u, blitInfo.srcSubresource.mipLevel, 
                        0u, 1u
                    );

                    vkDstImage.transition(
                        _barriers, 
                        dstLayout, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_ACCESS_TRANSFER_WRITE_BIT, 
                        0u, blitInfo.dstSubresource.mipLevel, 
                        0u, 1u
                    );

                    vkBlitInfo.srcOffsets[0u] = utils::convert2vk(blitInfo.srcOffsets[0u]);
                    vkBlitInfo.srcOffsets[1u] = utils::convert2vk(blitInfo.srcOffsets[1u]);
                    vkBlitInfo.srcSubresource = utils::convert2vk(blitInfo.srcSubresource);

                    vkBlitInfo.dstOffsets[0u] = utils::convert2vk(blitInfo.dstOffsets[0u]);
                    vkBlitInfo.dstOffsets[1u] = utils::convert2vk(blitInfo.dstOffsets[1u]);
                    vkBlitInfo.dstSubresource = utils::convert2vk(blitInfo.dstSubresource);
                }

                _barriers.flush(_commandbuffer());  

                _commandbuffer.blitImage(vkSrcImage, vkDstImage, srcLayout, dstLayout, 
                    {vkBlitInfoArray.begin(), vkBlitInfoArray.begin() + commandBlitInfo.regionsCount}, 
                    utils::convert2vk(commandBlitInfo.filter));
            } 
                break;
            case CommandType::eCopyImage2Image:
            {
                if (passId < _passes.size()) {
                    const auto & pass = _passes[passId];
                    if (pass.commandNumber == commandsNumber && pass.type == Pass::Type::eTransfer) {
                        applyBarriers(pass, nullptr, {});
                        ++passId;
                    }
                }

                const auto * command = static_cast<CopyImage2ImageCommand*>(cmd);
                stl::array copyInfos{utils::convert2vk(command->info)};

                const auto & vkSrcImage = *static_cast<const Image*>(command->src);
                const auto & vkDstImage = *static_cast<const Image*>(command->dst);
                _commandbuffer.copyImage2Image(vkSrcImage, vkDstImage, copyInfos);
            }   
                break;
            case CommandType::ePipelineBarriers:
                // todo!
                //_barriers.flush(_commandbuffer());
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