#pragma once

#include "common/hash.h"
#include "graphics/render/rhi/common/renderpass.h"

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>

namespace engine::vulkan {

class Device;

struct RenderPass {
    // limits maxColorAttachments!
    static constexpr size_t kMaxAttachemts = 4u; 
    struct AttachmentInfo {
        VkImageLayout finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    };

    VkRenderPass vkRenderPass = VK_NULL_HANDLE;
    std::array<AttachmentInfo, kMaxAttachemts> attachments;
};

class RenderPassManager {
    using RenderPassMap = std::unordered_map<rhi::RenderPassDescription, std::unique_ptr<RenderPass>, 
                                            hash::Hasher<rhi::RenderPassDescription>, std::equal_to<>>;
public:
    explicit RenderPassManager(const Device & device) : _device(device) {};
    ~RenderPassManager();

    RenderPass* getRenderPass(const rhi::RenderPassDescription & description);

    void destroy() noexcept;

private:
    // need synchronisations ?
    const Device & _device;
    RenderPassMap _renderPassCollection;
};

}