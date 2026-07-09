#pragma once

#include "graphics/render/rhi/common/framebuffer.h"
#include "graphics/render/rhi/common/types.h"
#include "graphics/render/rhi/common/runtime.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <span>
#include <vector>

namespace engine::vulkan {

class Device;

class FrameBuffer final : public rhi::runtime::FrameBuffer {
public:
    FrameBuffer(const Device & device, stl::vector<rhi::FrameBufferAttachment> && attachemnts, VkRenderPass renderPass);
    ~FrameBuffer();

    inline VkFramebuffer operator ()() const noexcept { return _framebuffer; }
    inline explicit operator bool() const noexcept { return _framebuffer != VK_NULL_HANDLE; }

    VkRenderPass getRenderPass() const noexcept { return _renderPass; }
    rhi::Extent2D getSize() const noexcept { return _size; }

    stl::span<rhi::FrameBufferAttachment> attachments() noexcept {
        return _attachments;
    }

private:
    const Device & _device;
    rhi::Extent2D _size = {0u, 0u};
    stl::vector<rhi::FrameBufferAttachment> _attachments;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
};

}