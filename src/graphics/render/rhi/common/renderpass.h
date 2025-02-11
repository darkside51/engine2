#pragma once

#include "types.h"
#include "utils/dynamic_array.h"

#include <span>
#include <utility>
#include <vector>

namespace engine::rhi {

constexpr size_t kMaxAttachemts = 16u; 

struct SubpassDescription {
    std::pair<const void*, size_t> pNext = {nullptr, 0u}; // value + hash(value) 
    AttachmentReference depthStencilAttachment = {}; // undefined as default
    dynamic_array<AttachmentReference, kMaxAttachemts> inputAttachments;
    dynamic_array<AttachmentReference, kMaxAttachemts> colorAttachments; // size = colorAttachmentCount
    dynamic_array<AttachmentReference, kMaxAttachemts> resolveAttachments; // size = colorAttachmentCount or empty
    dynamic_array<uint32_t, kMaxAttachemts> preserveAttachments;
    uint32_t viewMask = 0u;
    
    void setNext(void * next, size_t hash) noexcept {
        pNext = {next, hash};
    }
    inline size_t hash() const noexcept;
};

inline constexpr uint32_t kSubpassExternal = (~0u); // ~0u == 0xff'ff'ff'ffu == VK_SUBPASS_EXTERNAL

struct SubpassDependency {
    PipelineStageFlags srcStageMask;
    PipelineStageFlags dstStageMask;
    AccessFlags srcAccessMask;
    AccessFlags dstAccessMask;
    uint16_t srcSubpass = 0u;
    uint16_t dstSubpass = kSubpassExternal;
    DependencyFlags dependencyFlags = DependencyFlags::eBy_Region;
};

class RenderPassDescription {
public:
    RenderPassDescription(std::vector<AttachmentDescription> && attachments, 
            std::vector<SubpassDescription> && subpasses,
            std::vector<SubpassDependency> && subpassDependencies) :
            _attachments(std::move(attachments)), 
            _subpasses(std::move(subpasses)), 
            _subpassDependencies(std::move(subpassDependencies)) {
                recalculateHash();
            }

    RenderPassDescription(std::span<AttachmentDescription> attachments, 
            std::span<SubpassDescription> subpasses,
            std::span<SubpassDependency> subpassDependencies) :
            _attachments{attachments.begin(), attachments.end()}, 
            _subpasses{subpasses.begin(), subpasses.end()}, 
            _subpassDependencies{subpassDependencies.begin(), subpassDependencies.end()} {
                recalculateHash();
            }

    inline size_t hash() const noexcept { // for use as unordered containers key
        return _hash;
    }

private:
    void recalculateHash() noexcept;

    std::vector<AttachmentDescription> _attachments;
    std::vector<SubpassDescription> _subpasses;
    std::vector<SubpassDependency> _subpassDependencies;
    //std::pair<ResolveModeFlags, ResolveModeFlags> _depthStencilResolve;
    size_t _hash = 0u;
};

}