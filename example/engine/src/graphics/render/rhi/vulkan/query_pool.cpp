#include "query_pool.h"

#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "vk_types.h"

#include <cassert>

namespace engine::vulkan {

    QueryPool::QueryPool(const Device & device, const rhi::QueryPoolInfo & info) :
        _device(device) {

        VkQueryPoolCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0, // = VkQueryPoolCreateFlagBits::VK_QUERY_POOL_CREATE_RESET_BIT_KHR, can use it alltime, or it must be reseted from command
            .queryType = utils::convert2vk(info.queryType),
            .queryCount = info.queryCount,
            .pipelineStatistics = utils::convert2vk(info.pipelineStatistics)
        };

        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        auto result = vkCreateQueryPool(_device(), &createInfo,allocationCallbacks, &_pool);

        if (result != VK_SUCCESS) {
            _pool = VK_NULL_HANDLE;
            assert(false);
        }
    }

    QueryPool::~QueryPool() {
        if (_pool != VK_NULL_HANDLE) {
            const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
            vkDestroyQueryPool(_device(), _pool, allocationCallbacks);
            _pool = VK_NULL_HANDLE;
        }
    }

    bool QueryPool::getResults(uint16_t firstQuery, uint16_t queryCount, stl::span<std::byte> target, 
        size_t stride, rhi::QueryResultFlags flags) const noexcept {
        auto result = vkGetQueryPoolResults(_device(), _pool, firstQuery, queryCount, 
            target.size_bytes(), target.data(), static_cast<VkDeviceSize>(stride),
            utils::convert2vk(flags));
        return result == VK_SUCCESS;
    }
}