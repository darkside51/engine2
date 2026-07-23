#include "sampler_manager.h"

#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "vk_types.h"

#include "log/log.h"

#include <cassert>
#include <thread>

namespace engine::vulkan {

    VkSampler SamplerManager::getSampler(const rhi::SamplerInfo & samplerInfo) {
        if (auto it = _samplers.find(samplerInfo); it != _samplers.end()) {
            return it->second;
        }

        static auto threadId = std::this_thread::get_id();
        if (std::this_thread::get_id() != threadId) {
            // no sync here yet, but can to create all used samplers once, before start render threads
            assert(false);
            LOG_E("vulkan: SamplerManager try to access from multy hreads!");
        }

        const VkSamplerCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = utils::convert2vk(samplerInfo.magFilter),
            .minFilter = utils::convert2vk(samplerInfo.minFilter),
            .mipmapMode = utils::convert2vk2(samplerInfo.mipMapFilter),
            .addressModeU = utils::convert2vk(samplerInfo.addressModeU),
            .addressModeV = utils::convert2vk(samplerInfo.addressModeV),
            .addressModeW = utils::convert2vk(samplerInfo.addressModeW),
            .mipLodBias = 0.0f,
            .anisotropyEnable = std::abs(samplerInfo.maxAnisotropy) > 1e-2f,
            .maxAnisotropy = samplerInfo.maxAnisotropy,
            .compareEnable = samplerInfo.compareOp != rhi::CompareOp::eAlways, // eNever
            .compareOp = utils::convert2vk(samplerInfo.compareOp),
            .minLod = 0.0f,
            .maxLod = static_cast<float>(VK_REMAINING_MIP_LEVELS),
            .borderColor = utils::convert2vk(samplerInfo.borderColor),
            .unnormalizedCoordinates = false
        };

        VkSampler sampler;
        vkCreateSampler(_device(), &info, _device.gpu().getInstance().getAllocationCallbacks(), &sampler);
        _samplers.emplace(samplerInfo, sampler);
        return sampler;
    }

    void SamplerManager::destroy() noexcept {
        for (auto & [_, sampler] : _samplers) {
            vkDestroySampler(_device(), sampler, _device.gpu().getInstance().getAllocationCallbacks());
        }

        _samplers.clear();
    }
}