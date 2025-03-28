#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <array>
#include <list>
#include <span>
#include <unordered_map>
#include <utility>

namespace engine::vulkan {

    class Device;
    class DescriptorPool;

    struct DescriptorSet {
        DescriptorSet(const DescriptorSet &) = delete;
        DescriptorSet & operator= (const DescriptorSet &) = delete;

        DescriptorSet(DescriptorSet && other) noexcept : pool(other.pool), set(other.set) {
            other.pool = nullptr;
            other.set = VK_NULL_HANDLE;
        }

        DescriptorSet & operator= (DescriptorSet && other) noexcept {
            pool = other.pool;
            set = other.set;
            other.pool = nullptr;
            other.set = VK_NULL_HANDLE;
            return *this;
        }

        ~DescriptorSet();

        inline VkDescriptorSet operator ()() const noexcept { return set; }
        inline explicit operator bool() const noexcept { return set != VK_NULL_HANDLE; }

        VkDescriptorSet set = VK_NULL_HANDLE;
        DescriptorPool * pool = nullptr;
    };

    class DescriptorPoolMask {
    public:
        template<typename... Ts>
        explicit DescriptorPoolMask(Ts... resources) {
            addResources(resources...);
        }

        template<typename... Ts>
        void addResources(Ts... resources) noexcept {
            _(resources...);
        }

        template<typename... Ts>
        void removeResources(Ts... resources) noexcept {
            __(resources...);
        }

        inline uint32_t operator ()() const noexcept { return _mask; }

        inline bool operator [](rhi::shaders::ResourceType v) const noexcept {
            return _mask & (1u << std::to_underlying(v));
        }

        inline bool operator [](uint8_t bit) const noexcept {
            return _mask & (1u << bit);
        }

        bool compatibleWith(const DescriptorPoolMask & other) const noexcept {
            for (uint8_t i = 0u; i < std::to_underlying(rhi::shaders::ResourceType::eCount); ++i) {
                if (operator[](i) && !other[i]) { return false; }
            }

            return true;
        }

    private:
        void _(rhi::shaders::ResourceType v) {
            _mask |= 1u << std::to_underlying(v);
        }

        void __(rhi::shaders::ResourceType v) {
            _mask &= ~(1u << std::to_underlying(v));
        }

        template<typename... Ts>
        void _(rhi::shaders::ResourceType v, Ts... args) noexcept {
            _(v);
            _(args...);
        }

        template<typename... Ts>
        void __(rhi::shaders::ResourceType v, Ts... args) noexcept {
            __(v);
            __(args...);
        }

        uint32_t _mask = 0u;
    };

    class DescriptorPool {
    public:
        DescriptorPool(const Device & device, std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources, 
            rhi::shaders::DescriptorPoolFlags f = rhi::shaders::DescriptorPoolFlags::eNone, uint32_t maxSets = 64u);

        inline VkDescriptorPool operator ()() const noexcept { return _pool; }
        inline explicit operator bool() const noexcept { return _pool != VK_NULL_HANDLE; }

        uint32_t maxCount(rhi::shaders::ResourceType type) const noexcept {
            return _resources[std::to_underlying(type)].max;
        }

        uint32_t count(rhi::shaders::ResourceType type) const noexcept {
            return _resources[std::to_underlying(type)].current;
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

        // vkAllocateDescriptorSets, vkFreeDescriptorSets, and vkResetDescriptorPool 
        void reset() noexcept;
        bool freeDescriptorSets(std::span<const VkDescriptorSet> sets) noexcept;
        
    private:
        struct ResourceCount {
            uint32_t max = 0u;
            uint32_t current = 0u;
        };

        const Device & _device;
        VkDescriptorPool _pool = VK_NULL_HANDLE;
        std::array<ResourceCount, std::to_underlying(rhi::shaders::ResourceType::eCount)> _resources;
        uint32_t _allocatedSets = 0u;
        uint32_t _maxSets;
        rhi::shaders::DescriptorPoolFlags _flags;
    };

    class DescriptorPoolManager {
    public:

    private:
        std::unordered_map<uint32_t, std::list<DescriptorPool>> _pools;
    };

}
