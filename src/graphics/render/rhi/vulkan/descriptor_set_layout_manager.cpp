#include "descriptor_set_layout_manager.h"

#include "device.h"

namespace engine::vulkan {
    DescriptorSetLayoutManager::~DescriptorSetLayoutManager() {
        destroy();
    }

    void DescriptorSetLayoutManager::destroy() noexcept {
        for (const auto & [_, layout] : _layouts) {
            _device.destroyDescriptorSetLayout(layout);
        }
        _layouts.clear();
    }

    VkDescriptorSetLayout DescriptorSetLayoutManager::getSetlayout(const rhi::shaders::BindingSetLayout & key) {
        if (auto it = _layouts.find(key); it != _layouts.end()) {
            return it->second;
        }

        return _layouts.emplace(key, _device.createDescriptorSetLayout(key)).first->second;
    }

    void DescriptorSetLayoutManager::eraseSetLayout(const rhi::shaders::BindingSetLayout & key) noexcept {
        if (auto it = _layouts.find(key); it != _layouts.end()) {
            _device.destroyDescriptorSetLayout(it->second);
            _layouts.erase(it);
        }
    }
}