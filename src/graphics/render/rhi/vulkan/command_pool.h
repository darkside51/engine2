#pragma once

#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/runtime.h"
#include "threads/utils.h"

#include <vulkan/vulkan.hpp>

#include <array>
#include <optional>
#include <vector>

namespace engine::vulkan {

class Device;

class CommandPool : public rhi::runtime::CommandPool {
public:
    CommandPool(const Device & device, uint8_t queueFlags, rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer);
    ~CommandPool();

    VkCommandBuffer allocCommandBuffer(rhi::CommandBufferLevel level);
    void deallocCommandBuffer(rhi::CommandBufferLevel level, VkCommandBuffer commandBuffer);

    inline VkCommandPool operator ()() const noexcept { return _pool; }
    inline explicit operator bool() const noexcept { return _pool != VK_NULL_HANDLE; }

    inline const Device & getDevice() const noexcept { return _device; }

private:
    VkCommandPool _pool = VK_NULL_HANDLE;
    const Device & _device;
    rhi::CommandPoolCreateFlags _flags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer;
    std::array<std::vector<VkCommandBuffer>, 2u> _freeBuffers; // for primary ans secondary buffers
};

class CommandPoolManager {
public:
    CommandPoolManager(const Device & device, uint8_t threadsCount) : _device(device), _pools{threadsCount} {}
    
    [[nodiscard]] inline CommandPool * getPool() noexcept {
        return getPool(threads::unique_t<CommandPoolManager>::id());
    }

    [[nodiscard]] inline CommandPool * getPool(uint32_t idx) noexcept {
        return (idx < _pools.size() ? (_pools[idx].has_value() ? &(*_pools[idx]) : nullptr) : nullptr);
    }

    CommandPool * acquirePool(uint8_t queueFlags, rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer) noexcept {
        return acquirePool(threads::unique_t<CommandPoolManager>::id(), queueFlags, poolFlags);
    }

    CommandPool * acquirePool(uint32_t idx, uint8_t queueFlags, rhi::CommandPoolCreateFlags poolFlags = rhi::CommandPoolCreateFlags::eReset_Command_Buffer) noexcept {
        if (idx >= _pools.size()) {
            return nullptr;
        }

        auto & pool = _pools[idx];
        if (pool.has_value()) {
            return &(*pool);
        }

        return &(pool.emplace(_device, queueFlags, poolFlags));
    }

    bool registerThread() noexcept {
        return threads::unique_t<CommandPoolManager>::id() < _pools.size();
    }

    void destroy() noexcept {
        _pools.clear();
    }

private:
    const Device & _device;
    std::vector<std::optional<CommandPool>> _pools;
};

}