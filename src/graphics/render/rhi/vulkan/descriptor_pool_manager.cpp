#include "descriptor_pool_manager.h"

#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "vk_types.h"

#include <cassert>

namespace engine::vulkan {
    
    DescriptorPool::DescriptorPool(Device & device, std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources,
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
        if (vkCreateDescriptorPool(_device(), &poolInfo, allocationCallbacks, &_pool) != VK_SUCCESS) {
            for (auto & r : _resources) {
                r = {0u, 0u};
            }
            return;
        }
    }

    DescriptorPool::~DescriptorPool() {
        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        vkDestroyDescriptorPool(_device(), _pool, allocationCallbacks);
    }

    void DescriptorPool::reset() noexcept {
        if (vkResetDescriptorPool(_device(), _pool, 0) == VK_SUCCESS) {
            _allocatedSets = 0u;
            for (auto & r : _resources) {
                r.current = r.max;
            }
        }
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

    bool DescriptorPool::freeDescriptorSet(const DescriptorSet & set) noexcept {
        if (!std::to_underlying(_flags & rhi::shaders::DescriptorPoolFlags::eFree_Descriptor_Set)) {
            return false;
        }

        for (size_t i = 0u; i < std::to_underlying(rhi::shaders::ResourceType::eCount); ++i) {
            if (auto const count = set.resources[i]; count > 0u) {
                returnResources(static_cast<rhi::shaders::ResourceType>(i), count);
            }
        }

        freeDescriptorSets(std::array{set()});
        --_allocatedSets;

        return true;
    }

    DescriptorSet DescriptorPool::allocateDescriptorSet(const rhi::shaders::BindingSetLayout & layout) noexcept {
        if (_allocatedSets >= _maxSets) {
            return {layout, VK_NULL_HANDLE, *this};
        }

        ++_allocatedSets;

        std::array<uint32_t, std::to_underlying(rhi::shaders::ResourceType::eCount)> resources{};
        for (const rhi::shaders::BindingLayout & l : layout.layouts.view()) {
            resources[std::to_underlying(l.type)] += l.count;
        }

        for (size_t i = 0u; i < std::to_underlying(rhi::shaders::ResourceType::eCount); ++i) {
            if (!hasResources(static_cast<rhi::shaders::ResourceType>(i), resources[i])) {
                return {layout, VK_NULL_HANDLE, *this};
            }
        }

        for (size_t i = 0u; i < std::to_underlying(rhi::shaders::ResourceType::eCount); ++i) {
            if (auto const count = resources[i]; count > 0u) {
                getResources(static_cast<rhi::shaders::ResourceType>(i), count);
            }
        }

        auto setLayout = _device.getDescriptorSetLayoutManager().getSetlayout(layout);
        VkDescriptorSet set = VK_NULL_HANDLE;

        const VkDescriptorSetAllocateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = _pool,
            .descriptorSetCount = 1u,
            .pSetLayouts = &setLayout
        };

        if (const auto result = vkAllocateDescriptorSets(_device(), &info, &set); result != VK_SUCCESS) {
            // todo!
            assert(false);
            return {layout, VK_NULL_HANDLE, *this};
        }

        return {layout, set, *this};
    }


    ////////////
    DescriptorPool & DescriptorPoolManager::createPool(std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources, 
        rhi::shaders::DescriptorPoolFlags f, uint32_t maxSets) {
        engine::vulkan::DescriptorPoolMask mask;
        for (const auto & [type, _] : resources) {
            mask.addResources(type);
        }

        return _pools[mask()].emplace_back(_device, resources, f, maxSets);
    }

    DescriptorPool & DescriptorPoolManager::getCompatiblePool(const rhi::shaders::BindingSetLayout & layout) {
        engine::vulkan::DescriptorPoolMask mask;
        std::array<uint32_t, std::to_underlying(rhi::shaders::ResourceType::eCount)> resources{};
        for (const auto & setLayout : layout.layouts.view()) {
            mask.addResources(setLayout.type);
            resources[std::to_underlying(setLayout.type)] += setLayout.count;
        }

        auto findSuitablePool = [&](std::list<DescriptorPool> & pools)-> DescriptorPool* {
            for (auto & pool : pools) {
                bool suitable = true;
                for (size_t i = 0u; i < std::to_underlying(rhi::shaders::ResourceType::eCount); ++i) {
                    if (!pool.hasResources(static_cast<rhi::shaders::ResourceType>(i), resources[i])) {
                        suitable = false;
                        break;
                    }
                }

                if (suitable) {
                    return &pool;
                }
            }
            return nullptr;
        };
        
        if (auto it = _pools.find(mask()); it != _pools.end()) { // strike!!!
            if (auto * pool = findSuitablePool(it->second)) {
                return *pool;
            }
            
            //auto & pool = it->second.emplace_back(_device, resources, f, maxSets);
            //return pool.allocateDescriptorSet(layout);
        } else {
            // find compatible pools
            for (auto & [m, pools] : _pools) {
                if (mask.compatibleWith(DescriptorPoolMask{m})) {
                    if (auto * pool = findSuitablePool(pools)) {
                        return *pool;
                    }
                }
            }

            // if no compatible - create strick mask pools
            // auto [it2, success] = _pools.try_emplace(mask(), _device, resources, f, maxSets);
            // if (success) {
            //     return it2->second.begin().allocateDescriptorSet(layout);
            // }
        }
    }
}
