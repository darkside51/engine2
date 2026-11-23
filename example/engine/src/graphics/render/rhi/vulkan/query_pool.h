#pragma once

#include "graphics/render/rhi/common/runtime.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <span>

namespace engine::vulkan {

class Device;

class QueryPool final : public rhi::runtime::QueryPool {
public:
    QueryPool(const Device & device, const rhi::QueryPoolInfo & info);
    ~QueryPool();

    inline VkQueryPool operator ()() const noexcept { return _pool; }
    inline explicit operator bool() const noexcept { return _pool != VK_NULL_HANDLE; }

    bool getResults(uint16_t firstQuery, uint16_t queryCount, stl::span<std::byte> target, 
        size_t stride, rhi::QueryResultFlags flags) const noexcept override;

private:
    VkQueryPool _pool = VK_NULL_HANDLE;
    const Device & _device;
};

}