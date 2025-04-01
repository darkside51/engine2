#include "descriptor_pool_manager.h"

#include <cassert>
#include <vector>

namespace engine::vulkan {
    
    DescriptorPool & DescriptorPoolManager::createPool(std::span<const std::pair<rhi::shaders::ResourceType, uint32_t>> resources, 
        rhi::shaders::DescriptorPoolFlags f, uint32_t maxSets) {
        engine::vulkan::DescriptorPoolMask mask;
        for (const auto & [type, _] : resources) {
            mask.addResources(type);
        }

        return _pools[mask()].emplace_back(_device, resources, f, maxSets);
    }

    DescriptorPool & DescriptorPoolManager::getCompatiblePool(const rhi::shaders::BindingSetLayout & layout, rhi::shaders::DescriptorPoolFlags flags) {
        engine::vulkan::DescriptorPoolMask mask{flags};
        std::array<uint32_t, std::to_underlying(rhi::shaders::ResourceType::eCount)> resources{};
        for (const auto & setLayout : layout.layouts.view()) {
            mask.addResources(setLayout.type);
            resources[std::to_underlying(setLayout.type)] += setLayout.count;
        }

        auto findSuitablePool = [&](std::list<DescriptorPool> & pools) noexcept -> DescriptorPool* {
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

        auto createNewPoolResources = [&]() {
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
        
        if (auto it = _pools.find(mask()); it != _pools.end()) { // strike!!!
            if (auto * pool = findSuitablePool(it->second)) {
                return *pool;
            }
            
            auto const maxSets = it->second.back().maxSets();
            return it->second.emplace_back(_device, createNewPoolResources(), flags, maxSets);
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
            auto & list = _pools[mask()];
            return list.emplace_back(_device, createNewPoolResources(), flags);
        }
    }
}
