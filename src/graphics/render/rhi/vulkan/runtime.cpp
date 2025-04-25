#include "runtime.h"

#include "surface.h"
#include "physical_device.h"
#include "device.h"
#include "swapchain.h"
#include "framebuffer.h"
#include "queue.h"
#include "gpu_program.h"
#include "pipeline.h"
#include "buffer.h"
#include "vk_types.h"
#include "descriptor_set.h"
#include "descriptor_set_updater.h"
#include "descriptor_pool_manager.h"

#include "graphics/render/rhi/common/shader_module.h"
#include "graphics/render/rhi/common/commands.h"

#include "utils/dynamic_array.h"

#include <array>
#include <cassert>
#include <utility>

namespace engine::vulkan::runtime {
    //////////////////////////////////////////////////////////
    CommandList::CommandList(rhi::runtime::CommandPool * commandPool, rhi::CommandBufferLevel level) :
        _pool(static_cast<vulkan::CommandPool*>(commandPool)),
        _commandbuffer{*_pool, level} {
        
    }

    void CommandList::flush(uint8_t frameIdx) noexcept {
        using namespace rhi::runtime;

        constexpr uint8_t kMaxDescriptorsSets = 4u;

        std::byte * memory = _commands.memory().data();
        bool iterate = true;
        size_t totalMem = 0u;

        auto & device = const_cast<Device&>(_pool->getDevice()); // todo remove const_cast
        const Pipeline * currentPipeline = nullptr;
        std::array<DescriptorSetUpdater, kMaxDescriptorsSets> descriptorSetUpdaters;
        std::array<DescriptorSet, kMaxDescriptorsSets> descriptorSets;
        uint8_t changedSets = 0u;

        const auto applyDescriptorSets = [&]() noexcept {
            // apply tmp structs:
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

        while (iterate) {
            Command * cmd = reinterpret_cast<Command*>(memory);
            switch (cmd->cmdType) {
            case CommandType::eBegin:
            {
                _commandbuffer.begin();
                currentPipeline = nullptr;
            }
                break;
            case CommandType::eEnd:
            {
                _commandbuffer.end();
            }
                break;
            case CommandType::eReset: 
            {
                auto * command = reinterpret_cast<ResetCommand*>(memory);
                _commandbuffer.reset(command->flags);
                currentPipeline = nullptr;
                changedSets = 0u;
            }
                break;
            case CommandType::eBeginRenderPass:
            {
                auto * command = reinterpret_cast<BeginRenderPassCommand*>(memory);

                dynamic_array<VkClearValue, 4u> clearValues;
                clearValues.reserve(command->clearValues.size());
                auto view = clearValues.view();

                for (size_t i = 0u; i < command->clearValues.size(); ++i) {
                    view[i] = utils::convert2vk(command->clearValues[i]);
                }

                _commandbuffer.beginRenderPass(
                    static_cast<VkRenderPass>(command->renderPass), 
                    static_cast<const vulkan::FrameBuffer &>(*command->framebuffer),
                    view,
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
                auto * command = reinterpret_cast<NextSubpassCommand*>(memory);
                _commandbuffer.nextSubpass(command->contents);
            }
                break;
            case CommandType::eExecuteCommands:
            {
                auto * command = reinterpret_cast<ExecuteCommandsCommand*>(memory);
                // _commandbuffer.executeCommands();// todo!
            }
                break;
            case CommandType::eSetViewport:
            {
                auto * command = reinterpret_cast<SetViewportCommand*>(memory);
                _commandbuffer.setViewport(command->x, command->y, command->w, command->h, 
                    command->minDepth, command->maxDepth, command->viewportIdx);
            }
                break;
            case CommandType::eSetScissor:
            {
                auto * command = reinterpret_cast<SetScissorCommand*>(memory);
                _commandbuffer.setScissorRect(command->area, command->scissorIdx);
            }
                break;
            case CommandType::eBindPipeline:
            {
                auto * command = reinterpret_cast<BindPipelineCommand*>(memory);
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
                auto * command = reinterpret_cast<BindVertexBufferCommand*>(memory);
                // todo fill tmp structs and bindVertexBuffers before draw command!
                _commandbuffer.bindVertexBuffer(command->binding, static_cast<const Buffer&>(*command->buffer)(), command->offset);
            }
                break;
            case CommandType::eBindIndexBuffer:
            {
                auto * command = reinterpret_cast<BindIndexBufferCommand*>(memory);
                _commandbuffer.bindIndexBuffer(static_cast<const Buffer&>(*command->buffer)(), command->offset, command->type);
            }
                break;
            case CommandType::eBindResource:
            {
                auto * command = reinterpret_cast<BindResourceCommand*>(memory);
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
                        descriptorSetUpdaters[setId].updateBuffer(static_cast<Buffer*>(command->buffer), command->binding);
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

                auto * command = reinterpret_cast<DrawCommand*>(memory);
                _commandbuffer.draw(command->vertexCount, command->instanceCount, command->firstVertex, command->firstInstance);
            }
                break;
            case CommandType::eDrawIndexed:
            {
                applyDescriptorSets();

                auto * command = reinterpret_cast<DrawIndexedCommand*>(memory);
                _commandbuffer.drawIndexed(command->indexCount, command->instanceCount, command->firstIndex, 
                    command->vertexOffset, command->firstInstance);
            }
                break;
            case CommandType::eUndefined:
                iterate = false;
            break;    
            default:
                break;
            }
            
            totalMem += cmd->size;
            if (totalMem >= kStackSize) {
                break;
            }
    
            memory += cmd->size;
        }
    }

    //////////////////////////////////////////////////////////
    rhi::runtime::SurfacePtr Rhi::createSurface(const rhi::SurfaceCreateInfo & info) const {
        return std::make_unique<Api::Surface>(_instance, info);
    }

    std::vector<rhi::runtime::PysicalDevicePtr> Rhi::enumerateDevices(const rhi::PhysicalDeviceRequirements & info) const {
        auto apiPhysicalDevices = Api::PhysicalDevice::getSuitableDevices(_instance, info);
        
        std::vector<rhi::runtime::PysicalDevicePtr> physicalDevices;
        physicalDevices.reserve(physicalDevices.size());
        for (auto & d : apiPhysicalDevices) {
            physicalDevices.emplace_back(std::make_unique<Api::PhysicalDevice>(std::move(d)));
        }
        return physicalDevices;
    }

    rhi::runtime::DevicePtr Rhi::createDevice(rhi::runtime::PysicalDevicePtr & gpu, const rhi::DeviceCreateInfo & info) const {
        return std::make_unique<Api::Device>(static_cast<Api::PhysicalDevice &>(*gpu), info);
    }

    rhi::runtime::SwapchainPtr Rhi::createSwapchain(const rhi::runtime::DevicePtr & device, 
        const rhi::runtime::SurfacePtr & surface, const rhi::SwapchainCreateInfo & info) const {

        return std::make_unique<Api::Swapchain>(static_cast<const Api::Device &>(*device), 
            static_cast<const Api::Surface &>(*surface), info);
    }

    rhi::runtime::ImagePtr Rhi::createImage(const rhi::runtime::DevicePtr & device, const rhi::ImageInfo & info) const noexcept {
        return std::make_unique<Api::Image>(static_cast<Api::Device &>(*device), info);
    }

    rhi::runtime::RenderPass Rhi::getRenderPass(rhi::runtime::DevicePtr & device, 
        const rhi::RenderPassDescription & description) const noexcept {
        return static_cast<Api::Device &>(*device).getRenderPass(description);
    }

    rhi::runtime::FrameBufferPtr Rhi::createFrameBuffer(
        const rhi::runtime::DevicePtr & device, 
        std::vector<rhi::FrameBufferAttachment> && attachemnts, 
        const rhi::runtime::RenderPass & renderPass) const noexcept {

        return std::make_unique<Api::FrameBuffer>(static_cast<const Api::Device &>(*device), 
        std::move(attachemnts), static_cast<VkRenderPass>(renderPass));
    }

    rhi::runtime::QueuePtr Rhi::getQueue(const rhi::runtime::DevicePtr & device, 
        rhi::queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) const {
        return std::make_unique<Api::Queue>(static_cast<const Api::Device &>(*device), familyFlags, queueIdx, framesCount);
    }

    rhi::runtime::Fence Rhi::createFence(const rhi::runtime::DevicePtr & device, 
        rhi::FenceCreateFlags flags) const noexcept {
        return static_cast<const Api::Device &>(*device).createFence(flags);
    }
    rhi::runtime::Semaphore Rhi::createSemaphore(const rhi::runtime::DevicePtr & device) const noexcept {
        return static_cast<const Api::Device &>(*device).createSemaphore();
    }

    void Rhi::destroyFence(const rhi::runtime::DevicePtr & device, 
        const rhi::runtime::Fence & fence) const noexcept {
        static_cast<const Api::Device &>(*device).destroyFence(static_cast<VkFence>(fence));
    }
    void Rhi::destroySemaphore(const rhi::runtime::DevicePtr & device, 
        const rhi::runtime::Semaphore & semaphore) const noexcept {
        static_cast<const Api::Device &>(*device).destroySemaphore(static_cast<VkSemaphore>(semaphore));
    }

    rhi::runtime::ShaderModulesManagerPtr Rhi::createShaderModulesManager() const {
        return std::make_unique<engine::rhi::ShaderModulesManager<Api>>();
    }

    rhi::runtime::GpuProgramPtr Rhi::createProgram(
        rhi::runtime::DevicePtr & device, 
        std::span<const rhi::runtime::ShaderModuleView> moduleViews
    ) const {
        std::array<Api::ShaderModule::View, rhi::shaders::kStagesCount> apiModules;
        size_t i = 0u;
        for (const auto & view : moduleViews) {
            apiModules[i++] = {
                static_cast<VkShaderModule>(view.shaderModule), 
                static_cast<const engine::vulkan::ModuleReflection*>(view.reflection)
            };
        }

        return std::make_unique<Api::GpuProgram>(static_cast<Api::Device &>(*device), 
            std::span{apiModules.begin(), apiModules.begin() + moduleViews.size()});
    }

    std::unique_ptr<engine::rhi::runtime::Pipeline> Rhi::createComputePipeline(
        rhi::runtime::DevicePtr & device,
        const rhi::runtime::GpuProgramPtr & program) const {

        const auto & vkProgram = static_cast<const Api::GpuProgram&>(*program);

        return std::make_unique<Api::Pipeline>(static_cast<Api::Device &>(*device), vkProgram,
            computePipelineCreateInfo(vkProgram));
    }

    std::unique_ptr<engine::rhi::runtime::Pipeline> Rhi::createGraphicsPipeline(
        rhi::runtime::DevicePtr & device,
        rhi::runtime::RenderPass renderPass,
        const rhi::runtime::GpuProgramPtr & program,
        const rhi::GraphicsPipelineStatesInfo & info) const {

        const auto & vkProgram = static_cast<const Api::GpuProgram&>(*program);
        const Api::GraphicsPipelineInfo pipelineInfo{info};
        return std::make_unique<Api::Pipeline>(static_cast<Api::Device &>(*device), vkProgram,
            pipelineInfo.create(vkProgram, static_cast<VkRenderPass>(renderPass)));
    }

    rhi::runtime::BufferPtr Rhi::createBuffer(rhi::runtime::DevicePtr & device, 
        const rhi::BufferInfo & info, size_t size) const {
        return std::make_unique<Api::Buffer>(static_cast<Api::Device &>(*device), info, size);
    }

    rhi::runtime::CommandListPtr Rhi::createCommandList(rhi::runtime::CommandPool * commandPool, 
        rhi::CommandBufferLevel level) const {
        return std::make_unique<CommandList>(commandPool, level); 
    }
}