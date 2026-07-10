#pragma once

#include "types.h"
#include "utils/dynamic_array.h"

#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace engine::rhi {

struct SubpassDescription {
    static constexpr size_t kMaxAttachemts = 8u; 
    stl::pair<const void*, size_t> pNext = {nullptr, 0u}; // value + hash(value) 
    AttachmentReference depthStencilAttachment = {}; // undefined as default
    dynamic_array<AttachmentReference, kMaxAttachemts> inputAttachments;
    dynamic_array<AttachmentReference, kMaxAttachemts> colorAttachments; // size = colorAttachmentCount
    dynamic_array<AttachmentReference, kMaxAttachemts> resolveAttachments; // size = colorAttachmentCount or empty
    dynamic_array<uint32_t, kMaxAttachemts> reserveAttachments;
    uint32_t viewMask = 0u;
    
    inline void setNext(void * next, size_t hash) noexcept {
        pNext = {next, hash};
    }

    inline size_t hash() const noexcept;

    inline bool hasDepthStencil() const noexcept {
        return depthStencilAttachment.attachment != 0xff'ffu;
    }

    inline bool operator == (const SubpassDescription & other) const noexcept {
            return viewMask == other.viewMask && depthStencilAttachment == other.depthStencilAttachment && 
                   inputAttachments == other.inputAttachments && colorAttachments == other.colorAttachments &&
                   resolveAttachments == other.resolveAttachments && reserveAttachments == other.reserveAttachments &&
                   pNext.second == other.pNext.second;
        };
};

inline constexpr auto kSubpassExternal = uint32_t(~0u); // ~0u == 0xff'ff'ff'ffu == VK_SUBPASS_EXTERNAL

struct SubpassDependency {
    struct StageAndAccessMask {
        PipelineStageFlags stage;
        AccessFlags accessMask;
    };

    struct Stages {
        StageAndAccessMask src;
        StageAndAccessMask dst;
    };

    Stages stages;

    uint32_t srcSubpass = 0u;
    uint32_t dstSubpass = kSubpassExternal;
    DependencyFlags dependencyFlags = DependencyFlags::eBy_Region;
};

class RenderPassDescription {
public:
    RenderPassDescription(stl::vector<AttachmentDescription> && attachments, 
            stl::vector<SubpassDescription> && subpasses,
            stl::vector<SubpassDependency> && subpassDependencies) :
            _attachments(std::move(attachments)), 
            _subpasses(std::move(subpasses)), 
            _subpassDependencies(std::move(subpassDependencies)) {
                calculateHash();
            }

    RenderPassDescription(stl::span<AttachmentDescription> attachments, 
            stl::span<SubpassDescription> subpasses,
            stl::span<SubpassDependency> subpassDependencies) :
            _attachments{attachments.begin(), attachments.end()}, 
            _subpasses{subpasses.begin(), subpasses.end()}, 
            _subpassDependencies{subpassDependencies.begin(), subpassDependencies.end()} {
                calculateHash();
            }

    inline bool operator == (const RenderPassDescription & other) const noexcept {
        if (_hash != other._hash) {
            return false;
        }

        if (_subpassDependencies.size() != other._subpassDependencies.size()) {
            return false;
        }

        if (_subpasses.size() != other._subpasses.size()) {
            return false;
        }

        if (_attachments.size() != other._attachments.size()) {
            return false;
        }

        if (std::memcmp(_subpassDependencies.data(), other._subpassDependencies.data(), 
                sizeof(SubpassDependency) * _subpassDependencies.size()) != 0) {
            return false; // _subpassDependencies no equals
        }

        if (std::memcmp(_attachments.data(), other._attachments.data(), 
                sizeof(AttachmentDescription) * _attachments.size()) != 0) {
            return false; // _attachments no equals
        }
        
        return _subpasses == other._subpasses; // _subpasses
    }

    inline size_t hash() const noexcept { // for use as unordered containers key
        return _hash;
    }

    stl::span<const AttachmentDescription> getAttachments() const noexcept { return _attachments; }
    stl::span<const SubpassDescription> getSubpasses() const noexcept { return _subpasses; }
    stl::span<const SubpassDependency> getDependencies() const noexcept { return _subpassDependencies; }

private:
    void calculateHash() noexcept;

    stl::vector<AttachmentDescription> _attachments;
    stl::vector<SubpassDescription> _subpasses;
    stl::vector<SubpassDependency> _subpassDependencies;
    //stl::pair<ResolveModeFlags, ResolveModeFlags> _depthStencilResolve;
    size_t _hash = 0u;
};

}