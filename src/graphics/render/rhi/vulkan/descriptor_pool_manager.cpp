#include "descriptor_pool_manager.h"
#include "device.h"

#include <cassert>
#include <vector>

namespace engine::vulkan {
    
    DescriptorPoolManager::DescriptorPoolManager(Device & device, bool useGlobalPool) : 
        _device(device), 
        _pools(useGlobalPool ? device.inFlightFramesCount() + 1u : device.inFlightFramesCount()),
        _useGlobalPool(useGlobalPool) {}

    DescriptorPoolManager::DescriptorPoolManager(Device & device, uint8_t poolsCount) :
        _device(device), 
        _pools(poolsCount),
        _useGlobalPool(false) {}

    DescriptorPoolManager::DescriptorPools & DescriptorPoolManager::poolsForFrame(uint8_t frameIdx) noexcept {
        // frame 0 - use _pools[1], frame 1 - use _pools[2], ..., frame kGlobalPoolsIndex - use _pools[0] - if _useGlobalPool is true
        return _useGlobalPool ? _pools[frameIdx + 1u] : _pools[frameIdx];
    }

    DescriptorPool & DescriptorPoolManager::createPool(std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources, 
        uint8_t frameIdx, rhi::shaders::DescriptorPoolFlags f, uint32_t maxSets) {
        engine::vulkan::DescriptorPoolMask mask;
        for (const auto & [type, _] : resources) {
            mask.addResources(type);
        }

        return poolsForFrame(frameIdx)[mask()].list.emplace_back(_device, resources, f, maxSets);
    }

    void DescriptorPoolManager::resetPools(uint8_t frameIdx) noexcept {
        constexpr uint16_t kUnusedFramesToRemovePool = 256u;

        auto & framePools = poolsForFrame(frameIdx);
        for (auto & [mask, pools] : framePools) {
            if (pools.list.empty()) { // has been unused on last frame
                if (++pools.count == kUnusedFramesToRemovePool) {
                    framePools.erase(mask);
                }
            } else {
                for (auto it = pools.list.begin(); it != pools.list.end();) {
                    auto & pool = *it;
                    if (pool.allocatedSets() == 0u) {
                        it = pools.list.erase(it);
                    } else {
                        pool.reset();
                        ++it;
                    }
                }
                pools.count = 0u;
            }
        }
    }

    DescriptorPool & DescriptorPoolManager::getCompatiblePool(const rhi::shaders::BindingSetLayout & layout, uint8_t frameIdx,
        rhi::shaders::DescriptorPoolFlags flags) {
        engine::vulkan::DescriptorPoolMask mask{flags};
        std::array<uint32_t, std::to_underlying(rhi::shaders::ResourceType::eCount)> resources{};
        for (const auto & setLayout : layout.layouts.view()) {
            mask.addResources(setLayout.type);
            resources[std::to_underlying(setLayout.type)] += setLayout.count;
        }

        const auto findSuitablePool = [&](std::list<DescriptorPool> & pools) noexcept -> DescriptorPool* {
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

        const auto createNewPoolResources = [&]() {
            std::vector<std::pair<rhi::shaders::ResourceType, uint32_t>> setResources;
            setResources.reserve(std::to_underlying(rhi::shaders::ResourceType::eCount));
            for (size_t i = 0u; i < std::to_underlying(rhi::shaders::ResourceType::eCount); ++i) {
                if (resources[i] > 0u) {
                    setResources.emplace_back(static_cast<rhi::shaders::ResourceType>(i), resources[i] * kDefaultMaxSets);
                }
            }
            setResources.shrink_to_fit();
            return setResources;
        };
        
        auto & framePools = poolsForFrame(frameIdx);
        if (auto it = framePools.find(mask()); it != framePools.end()) { // strike!!!
            if (auto * pool = findSuitablePool(it->second.list)) {
                return *pool;
            }
            
            auto const maxSets = it->second.list.back().maxSets();
            return it->second.list.emplace_back(_device, createNewPoolResources(), flags, maxSets);
        } else {
            // find compatible pools
            for (auto & [m, pools] : framePools) {
                if (mask.compatibleWith(DescriptorPoolMask{m})) {
                    if (auto * pool = findSuitablePool(pools.list)) {
                        return *pool;
                    }
                }
            }

            // if no compatible - create strick mask pools
            auto & list = framePools[mask()].list;
            return list.emplace_back(_device, createNewPoolResources(), flags);
        }
    }
}
