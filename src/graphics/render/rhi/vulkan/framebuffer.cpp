#include "framebuffer.h"

#include "device.h"
#include "physical_device.h"
#include "instance.h"
#include "image.h"

#include "graphics/render/rhi/common/utils.h"

#include "log/log.h"

namespace engine::vulkan {

FrameBuffer::FrameBuffer(const Device & device, std::vector<rhi::FrameBufferAttachment> && fbAttachemnts, VkRenderPass renderPass) :
    _device(device),
    _attachments(std::move(fbAttachemnts)) {

    std::vector<VkImageView> attachments;
    attachments.reserve(_attachments.size());
    for (auto & attachment : _attachments) {
        attachments.emplace_back(static_cast<Image*>(attachment.image)->getImageView(rhi::ImageViewType::eType_2D, 
                            attachment.layer, 1u, attachment.level, 1u, attachment.aspect));
        _size = rhi::getMipLevelSize(attachment.imageSize, attachment.level);
    }

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0u;
    info.renderPass = renderPass;
    info.attachmentCount = attachments.size();
    info.pAttachments = attachments.data();
    info.width = _size.width;
    info.height = _size.height;
    info.layers = 1u;

    const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
    if (auto result = vkCreateFramebuffer(_device(), &info, allocationCallbacks, &_framebuffer); result != VK_SUCCESS) {
        LOG_E("vulkan: vkCreateFramebuffer result: %d", result);
        _framebuffer = VK_NULL_HANDLE;
        return;
    }

    LOG_I("vulkan: framebuffer created");
}

FrameBuffer::~FrameBuffer() {
    const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
    vkDestroyFramebuffer(_device(), _framebuffer, allocationCallbacks);
    _framebuffer = VK_NULL_HANDLE;
    _size = {0u, 0u};
    LOG_I("vulkan: framebuffer destroyed");
}

}