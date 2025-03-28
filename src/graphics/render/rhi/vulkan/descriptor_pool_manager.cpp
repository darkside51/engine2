#include "descriptor_pool_manager.h"

#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "vk_types.h"

#include <cassert>

namespace engine::vulkan {
    
    DescriptorSet::~DescriptorSet() {
        if (pool && set) {
            pool->freeDescriptorSets(std::array{set});
        }
        pool = nullptr;
        set = VK_NULL_HANDLE;
    }

    DescriptorPool::DescriptorPool(const Device & device, std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources,
        rhi::shaders::DescriptorPoolFlags f, uint32_t maxSets) : _device(device), _flags(f) {

        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(resources.size());
        uint32_t sets = 0u;
        for (const auto [type, count] : resources) {
            _resources[std::to_underlying(type)] = {count, count};
            poolSizes.emplace_back(utils::convert2vk(type), count);
            sets += count;
        }

        _maxSets = std::min(sets, maxSets);

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = utils::convert2vk(_flags),
            .maxSets = _maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };

        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        if (!vkCreateDescriptorPool(_device(), &poolInfo, allocationCallbacks, &_pool) == VK_SUCCESS) {
            for (auto & r : _resources) {
                r = {0u, 0u};
            }
            return;
        }
    }

    void DescriptorPool::reset() noexcept {
        if (vkResetDescriptorPool(_device(), _pool, 0) == VK_SUCCESS) {
            _allocatedSets = 0u;
            for (auto & r : _resources) {
                r.current = r.max;
            }
        }
        assert(false);
    }

    bool DescriptorPool::freeDescriptorSets(std::span<const VkDescriptorSet> sets) noexcept {
        if (!std::to_underlying(_flags & rhi::shaders::DescriptorPoolFlags::eFree_Descriptor_Set)) {
            return false;
        }
        if (vkFreeDescriptorSets(_device(), _pool, sets.size(), sets.data()) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

}
