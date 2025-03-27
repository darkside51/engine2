#pragma once

#include "graphics/render/rhi/common/types.h"

#include <vulkan/vulkan.h>

#include <span>
#include <utility>

namespace engine::vulkan {

    class Device;

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

    class DescriptorPoolManager {
    public:

    private:
    };

}
