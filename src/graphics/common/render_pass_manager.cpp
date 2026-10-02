#include "render_pass_manager.h"
#include "log/log.h"

namespace engine::graphics {
 
    uint32_t RenderPassManager::registerRenderPass(rhi::runtime::RenderPass pass) {
        _passes.emplace_back(pass);
        return _passes.size() - 1u;
    }

    uint32_t RenderPassManager::registerRenderPass(std::string_view name, rhi::runtime::RenderPass pass) {
        if (auto it = _passesMap.find(name); it != _passesMap.end()) {
            LOG_D("RenderPassManager::registerRenderPass: pass with name %s, already exists", name.data());
            return it->second;
        }

        const auto result = registerRenderPass(pass);
        _passesMap.emplace(name, result);
        return result;
    }

    uint32_t RenderPassManager::registerRenderPass(const rhi::RenderPassDescription & description) {
        return registerRenderPass(_api.getRenderPass(_device, description));
    }

    uint32_t RenderPassManager::registerRenderPass(std::string_view name, const rhi::RenderPassDescription & description) {
        return registerRenderPass(name, _api.getRenderPass(_device, description));
    }

    rhi::runtime::RenderPass RenderPassManager::getRenderPass(uint32_t idx) const noexcept {
        if (idx >= _passes.size()) { [[unlikely]]
            LOG_E("RenderPassManager::getRenderPass: no pass with id %s", idx);
            return nullptr;
        }
        return _passes[idx];
    }

    rhi::runtime::RenderPass RenderPassManager::getRenderPass(std::string_view name) const noexcept {
        if (auto it = _passesMap.find(name); it != _passesMap.end()) {
            return _passes[it->second];
        }

        LOG_E("RenderPassManager::getRenderPass: no pass with name %s", name.data());
        return nullptr;
    }
}