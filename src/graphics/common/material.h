#pragma once

#include "allocators/pool_allocator.h"
#include "math/math.h"
#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/texture/texture_data_loader.h"

#include "graphics/mesh/gltf_loader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace engine {
class FileManager;
}

namespace engine::graphics {
class TextureManager;
struct ImageHandler;

enum class PbrTextureId : uint8_t {
    eBaseColor = 0u,
    eNormal = 1u,
    eMetallicRoughness = 2u,
    eOccolusion = 3u,
    eEmissive = 4u
};

struct Material {
    // std::array<rhi::runtime::ImagePtr, 5u> textures;
    std::array<const ImageHandler*, 5u> textures;
    vec4f baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
    vec3f emissiveFactor = {0.0f, 0.0f, 0.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;	
    float alphaCutoff = 0.5f;
    rhi::BlendMode blendMode = rhi::common_blend_modes::blend_none;
    rhi::CullMode cullMode = rhi::CullMode::eBack;
};

class MaterialManager {
    using Allocator = allocators::PoolAllocator<sizeof(Material), alignof(Material), false>;
    using Materials = std::vector<Material*>;

public:
    void destroy();
    std::vector<uint32_t> prepareMaterials(std::string_view name, const models::Layout & layout, TextureManager & textureManager);
    std::vector<uint32_t> loadMaterials(std::string_view name, const models::Layout & layout, const FileManager & fileManager, TextureManager & textureManager);
    const Material & getMaterial(uint32_t idx) const noexcept { return *_materials[idx];/*return reinterpret_cast<const Material*>(_allocator.memory())[idx];*/ }

    void removeMaterial(uint32_t idx) noexcept;

private:
    Allocator _allocator{16'384u};
    Materials _materials{_allocator.capacity()};
};

}