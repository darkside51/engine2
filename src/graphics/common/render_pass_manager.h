#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/runtime.h"
#include "stl/vector.h"
#include "stl/unordered_map.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace engine::graphics {

    class RenderPassManager final {
        using PassesMap = stl::unordered_map<std::string, uint32_t, hash::String_hash, std::equal_to<>>;
        
    public:
        RenderPassManager(
            rhi::runtime::Rhi & api,
            rhi::runtime::Device & device
        ) : _api(api), _device(device) {}

        uint32_t registerRenderPass(rhi::runtime::RenderPass pass);
        uint32_t registerRenderPass(std::string_view name, rhi::runtime::RenderPass pass);

        uint32_t registerRenderPass(const rhi::RenderPassDescription & description);
        uint32_t registerRenderPass(std::string_view name, const rhi::RenderPassDescription & description);

        rhi::runtime::RenderPass getRenderPass(uint32_t idx) const noexcept;
        rhi::runtime::RenderPass getRenderPass(std::string_view name) const noexcept;

    void destroy() noexcept {
        _passes.clear();
        _passesMap.clear();
    }

    private:
        rhi::runtime::Rhi & _api;
        rhi::runtime::Device & _device;
        stl::vector<rhi::runtime::RenderPass> _passes;
        PassesMap _passesMap;
    };
}