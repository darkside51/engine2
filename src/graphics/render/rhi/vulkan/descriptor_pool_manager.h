#pragma once

#include "graphics/render/rhi/common/types.h"
#include "descriptor_pool.h"

#include <vulkan/vulkan.h>

#include <list>
#include <span>
#include <unordered_map>
#include <utility>

namespace engine::vulkan {

    class Device;

    class DescriptorPoolMask {
    public:
        DescriptorPoolMask() = default;
        explicit DescriptorPoolMask(uint32_t m) : _mask(m) {}
        explicit DescriptorPoolMask(rhi::shaders::DescriptorPoolFlags flags) : _mask(static_cast<uint32_t>(flags) << 24u) {}
        
        template<typename... Ts>
        explicit DescriptorPoolMask(Ts... resources) {
            addResources(resources...);
        }

        void setFlags(rhi::shaders::DescriptorPoolFlags flags) noexcept {
            _mask &= 0x00'ff'ff'ffu; 
            _mask |= static_cast<uint32_t>(flags) << 24u;
        }

        template<typename... Ts>
        void addResources(Ts... resources) noexcept {
            _(resources...);
        }

        template<typename... Ts>
        void removeResources(Ts... resources) noexcept {
            __(resources...);
        }

        inline uint8_t flags() const noexcept {
            return static_cast<uint8_t>(_mask >> 24u);
        }
        inline uint32_t operator ()() const noexcept { return _mask; }

        inline bool operator [](rhi::shaders::ResourceType v) const noexcept {
            return _mask & (1u << std::to_underlying(v));
        }

        inline bool operator [](uint8_t bit) const noexcept {
            return _mask & (1u << bit);
        }

        bool compatibleWith(const DescriptorPoolMask & other) const noexcept {
            if (flags() != other.flags()) {
                return false;
            }

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

    class DescriptorPoolManager {
        inline static constexpr uint32_t kDefaultMaxSets = 64u;
        inline static constexpr uint32_t kDefaultResourcesInSet = 64u;
    public:
        DescriptorPoolManager(Device & device) : _device(device) {}

        void destroy() noexcept {
            for (auto & [_, pools] : _pools) {
                for (auto & pool : pools) {
                    pool.reset();
                }
            }

            _pools.clear();
        }

        DescriptorPool & createPool(std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources, 
            rhi::shaders::DescriptorPoolFlags f = rhi::shaders::DescriptorPoolFlags::eNone, uint32_t maxSets = kDefaultMaxSets);

        DescriptorPool & getCompatiblePool(const rhi::shaders::BindingSetLayout & layout, 
            rhi::shaders::DescriptorPoolFlags flags = rhi::shaders::DescriptorPoolFlags::eNone);
    private:
        Device & _device;
        std::unordered_map<uint32_t, std::list<DescriptorPool>> _pools;
    };

}
