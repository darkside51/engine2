#include "renderpass.h"

#include "common/hash.h"

namespace engine::rhi {

    inline size_t attachmentRefHash(const AttachmentReference & ref) noexcept {
        if (ref.attachment == 0xff'ffu) return 0u;
        return hash::hash_combine(ref.pNext.second, ref.attachment, ref.layout, ref.aspectMask);
    }

    inline size_t SubpassDescription::hash() const noexcept {
        size_t hash = hash::hash_combine(pNext.second, viewMask, attachmentRefHash(depthStencilAttachment));

        const auto attachmentsView = inputAttachments.view();
        for (const auto & attachment : attachmentsView) {
            hash::hash_combine(hash, attachmentRefHash(attachment));
        }

        const auto colorAttachmentsView = colorAttachments.view();
        for (const auto & attachment : colorAttachmentsView) {
            hash::hash_combine(hash, attachmentRefHash(attachment));
        }

        const auto resolveAttachmentsView = resolveAttachments.view();
        for (const auto & attachment : resolveAttachmentsView) {
            hash::hash_combine(hash, attachmentRefHash(attachment));
        }
        
        const auto reserveAttachmentsView = reserveAttachments.view();
        for (auto attachment : reserveAttachmentsView) {
            hash::hash_combine(hash, attachment);
        }

        return hash;
    }

    void RenderPassDescription::calculateHash() noexcept {
        _hash = 0u;
        for (auto const & a : _attachments) {
            const auto h = hash::hash_combine(a.format, a.loadStoreOp.mask,
                                            a.stencilLoadStoreOp.mask, a.initialLayout, 
                                            a.finalLayout, a.samples, a.aliasFlag);
            hash::hash_combine(_hash, h);
        }

        for (auto const & s : _subpasses) {
            hash::hash_combine(_hash, s.hash());
        }

        for (auto const & d : _subpassDependencies) {
            const auto h = hash::hash_combine(d.stages.src.stage, d.stages.dst.stage,
                                            d.stages.src.accessMask, d.stages.dst.accessMask,
                                            d.srcSubpass, d.dstSubpass, d.dependencyFlags);
            hash::hash_combine(_hash, h);
        }
    }

}