#include "renderpass.h"

#include "common/hash.h"

namespace engine::rhi {

    inline size_t attachmentRefHash(const AttachmentReference & ref) noexcept {
        if (ref.attachment == 0xff'ffu) return 0u;
        return hash::hash_combine(ref.pNext.second, ref.attachment, ref.layout, ref.aspectMask);
    }

    inline size_t SubpassDescription::hash() const noexcept {
        size_t hash = hash::hash_combine(pNext.second, viewMask, attachmentRefHash(depthStencilAttachment));

        for (size_t i = 0u; i < inputAttachments.capacity; ++i) {
            hash::hash_combine(hash, attachmentRefHash(inputAttachments[i]));
        }

        for (size_t i = 0u; i < colorAttachments.capacity; ++i) {
            hash::hash_combine(hash, attachmentRefHash(colorAttachments[i]));
        }

        for (size_t i = 0u; i < resolveAttachments.capacity; ++i) {
            hash::hash_combine(hash, attachmentRefHash(resolveAttachments[i]));
        }
        
        for (size_t i = 0u; i < preserveAttachments.capacity; ++i) {
            hash::hash_combine(hash, preserveAttachments[i]);
        }

        return hash;
    }

    void RenderPassDescription::calculateHash() noexcept {
        _hash = 0u;
        for (auto const & a : _attachments) {
            const auto h = hash::hash_combine(a.format, a.loadOp, a.storeOp,
                                            a.stencilLoadOp, a.stencilStoreOp, a.initialLayout, 
                                            a.finalLayout, a.samples, a.aliasFlag);
            hash::hash_combine(_hash, h);
        }

        for (auto const & s : _subpasses) {
            hash::hash_combine(_hash, s.hash());
        }

        for (auto const & d : _subpassDependencies) {
            const auto h = hash::hash_combine(d.srcStageMask, d.dstStageMask, 
                                            d.srcAccessMask, d.dstAccessMask, 
                                            d.srcSubpass, d.dstSubpass, d.dependencyFlags);
            hash::hash_combine(_hash, h);
        }
    }

}