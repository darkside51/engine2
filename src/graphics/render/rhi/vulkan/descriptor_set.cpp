#include "descriptor_set.h"

#include "descriptor_pool_manager.h"

#include <utility>

namespace engine::vulkan {
    
    DescriptorSet::DescriptorSet(const rhi::shaders::BindingSetLayout & layout, VkDescriptorSet set, DescriptorPool & pool) : 
        set(set), pool(&pool) {
            if (set != VK_NULL_HANDLE) {
                for (const rhi::shaders::BindingLayout & l : layout.layouts.view()) {
                    resources[std::to_underlying(l.type)] += l.count;
                }
            }
    }

    DescriptorSet::~DescriptorSet() {
        if (pool && set) {
            pool->freeDescriptorSet(*this);
        }
        pool = nullptr;
        set = VK_NULL_HANDLE;
    }
}