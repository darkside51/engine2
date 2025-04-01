#pragma once

#include "graphics/render/rhi/common/types.h"
#include "descriptor_set.h"

#include <vulkan/vulkan.h>

#include <array>
#include <span>
#include <utility>

namespace engine::vulkan {

    class Device;

    class DescriptorPool {
    public:
        DescriptorPool(Device & device, std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources, 
            rhi::shaders::DescriptorPoolFlags f = rhi::shaders::DescriptorPoolFlags::eNone, uint32_t maxSets = 64u);

        ~DescriptorPool();

        inline VkDescriptorPool operator ()() const noexcept { return _pool; }
        inline explicit operator bool() const noexcept { return _pool != VK_NULL_HANDLE; }

        uint32_t maxCount(rhi::shaders::ResourceType type) const noexcept {
            return _resources[std::to_underlying(type)].max;
        }

        uint32_t count(rhi::shaders::ResourceType type) const noexcept {
            return _resources[std::to_underlying(type)].current;
        }

        bool hasResources(rhi::shaders::ResourceType type, uint32_t count) const noexcept {
            return _resources[std::to_underlying(type)].current >= count;
        }

        bool getResources(rhi::shaders::ResourceType type, uint32_t count) noexcept {
            if (_resources[std::to_underlying(type)].current >= count) {
                _resources[std::to_underlying(type)].current -= count;
                return true;
            }
            return false;
        }

        bool returnResources(rhi::shaders::ResourceType type, uint32_t count) noexcept {
            if (!std::to_underlying(_flags & rhi::shaders::DescriptorPoolFlags::eFree_Descriptor_Set)) {
                return false;
            }
            _resources[std::to_underlying(type)].current += count;
            return true;
        }

        uint32_t maxSets() const noexcept { return _maxSets; }

        // allocateDescriptorSets, freeDescriptorSets, and resetDescriptorPool 
        void reset() noexcept;
        bool freeDescriptorSets(std::span<const VkDescriptorSet> sets) noexcept;
        bool freeDescriptorSet(const DescriptorSet & set) noexcept;
        DescriptorSet allocateDescriptorSet(const rhi::shaders::BindingSetLayout & layout) noexcept;
        
    private:
        struct ResourceCount {
            uint32_t max = 0u;
            uint32_t current = 0u;
        };

        Device & _device;
        VkDescriptorPool _pool = VK_NULL_HANDLE;
        std::array<ResourceCount, std::to_underlying(rhi::shaders::ResourceType::eCount)> _resources{};
        uint32_t _allocatedSets = 0u;
        uint32_t _maxSets = 0u;
        rhi::shaders::DescriptorPoolFlags _flags = rhi::shaders::DescriptorPoolFlags::eNone;
    };
}