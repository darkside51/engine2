#include "renderpass.h"
#include "device.h"
#include "instance.h"
#include "physical_device.h"
#include "vk_types.h"

#include "log/log.h"
#include "utils/dynamic_array.h"

#include <cassert>

namespace engine::vulkan {

    VkAttachmentDescription convert2vk(const rhi::AttachmentDescription & description) noexcept {
        return {
            description.aliasFlag ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT : VkAttachmentDescriptionFlags{0},
            utils::convert2vk(description.format),
            utils::convert2vk(description.samples),
            utils::convert2vk(description.loadStoreOp.loadOp()),
            utils::convert2vk(description.loadStoreOp.storeOp()),
            utils::convert2vk(description.stencilLoadStoreOp.loadOp()),
            utils::convert2vk(description.stencilLoadStoreOp.storeOp()),
            utils::convert2vk(description.initialLayout),
            utils::convert2vk(description.finalLayout)
        };
    }

    VkSubpassDependency convert2vk(const rhi::SubpassDependency & dependency) noexcept {
        return {
            dependency.srcSubpass,
            dependency.dstSubpass,
            utils::convert2vk(dependency.stages.src.stage),
            utils::convert2vk(dependency.stages.dst.stage),
            utils::convert2vk(dependency.stages.src.accessMask),
            utils::convert2vk(dependency.stages.dst.accessMask),
            utils::convert2vk(dependency.dependencyFlags)
        };
    }

    VkAttachmentReference convert2vk(const rhi::AttachmentReference & ref) noexcept {
        return {
            ref.attachment, 
            utils::convert2vk(ref.layout)
        };
    }

    VkAttachmentReference2 convert2vk_2(const rhi::AttachmentReference & ref) noexcept {
        return {
            VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
            ref.pNext.first, // need be converted?
            ref.attachment, 
            utils::convert2vk(ref.layout),
            utils::convert2vk(ref.aspectMask)
        };
    }

    RenderPassManager::~RenderPassManager() {
        destroy();
    }

    void RenderPassManager::destroy() noexcept {
        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        for (auto & [key, pass] : _renderPassCollection) {
            vkDestroyRenderPass(_device(), pass->vkRenderPass, allocationCallbacks);
        }
        _renderPassCollection.clear();
    }

    RenderPass* RenderPassManager::getRenderPass(const rhi::RenderPassDescription & description) {
        if (auto it = _renderPassCollection.find(description); it != _renderPassCollection.end()) {
            return it->second;
        }

        const auto attachmentsSrc = description.getAttachments();
        stl::vector<VkAttachmentDescription> attachments;
        attachments.reserve(attachmentsSrc.size());
        for (auto const & description : attachmentsSrc) {
            attachments.emplace_back(convert2vk(description));
        }

        const auto dependenciesSrc = description.getDependencies();
        stl::vector<VkSubpassDependency> dependencies;
        dependencies.reserve(dependenciesSrc.size());
        for (auto const & dependency : dependenciesSrc) {
            dependencies.emplace_back(convert2vk(dependency));
        }

        ////
        const auto subpassesSrc = description.getSubpasses();
        stl::vector<VkSubpassDescription> subpasses;
        subpasses.reserve(subpassesSrc.size());

        using VkAttachmentsCollection = dynamic_array<VkAttachmentReference, rhi::SubpassDescription::kMaxAttachemts>;

        stl::vector<VkAttachmentsCollection> inputAttachmentReferencesVec;
        inputAttachmentReferencesVec.reserve(subpassesSrc.size());

        stl::vector<VkAttachmentsCollection> colorAttachmentReferencesVec;
        colorAttachmentReferencesVec.reserve(subpassesSrc.size());

        stl::vector<VkAttachmentsCollection> resolveAttachmentReferencesVec;
        resolveAttachmentReferencesVec.reserve(subpassesSrc.size());

        stl::vector<VkAttachmentReference> depthStencilAttachmentsVec;
        depthStencilAttachmentsVec.reserve(subpassesSrc.size());

        for (auto const & subpass : subpassesSrc) {
            assert ( // resolveAttachments are empty or it size equal colorAttachments size
                subpass.resolveAttachments.empty() ||
                subpass.resolveAttachments.capacity == subpass.colorAttachments.capacity
            );

            auto & inputAttachmentReferences = inputAttachmentReferencesVec.emplace_back(subpass.inputAttachments.capacity);
            const auto inputAttachmentReferencesView = inputAttachmentReferences.view();
            const auto attachmentsView = subpass.inputAttachments.view();
            size_t i = 0u;
            for (const auto & attachment : attachmentsView) {
                inputAttachmentReferencesView[i] = convert2vk(attachment);
                ++i; 
            }

            auto & colorAttachmentReferences = colorAttachmentReferencesVec.emplace_back(subpass.colorAttachments.capacity);
            const auto colorAttachmentReferencesView = colorAttachmentReferences.view();
            const auto colorAttachmentsView = subpass.colorAttachments.view();
            i = 0u;
            for (const auto & attachment : colorAttachmentsView) {
                colorAttachmentReferencesView[i] = convert2vk(attachment);
                ++i;
            }

            auto & resolveAttachmentReferences = resolveAttachmentReferencesVec.emplace_back(subpass.resolveAttachments.capacity);
            const auto resolveAttachmentReferencesView = resolveAttachmentReferences.view();
            const auto resolveAttachmentsView = subpass.resolveAttachments.view();
            i = 0u;
            for (const auto & attachment : resolveAttachmentsView) {
                resolveAttachmentReferencesView[i] = convert2vk(attachment); 
                ++i;
            }

            VkAttachmentReference * depthStencilRef = nullptr;
            if (subpass.hasDepthStencil()) {
                depthStencilRef = & depthStencilAttachmentsVec.emplace_back(convert2vk(subpass.depthStencilAttachment));
            }

            subpasses.emplace_back(
                0,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                inputAttachmentReferencesView.size(),
                inputAttachmentReferencesView.data(),
                colorAttachmentReferencesView.size(),
                colorAttachmentReferencesView.data(),
                resolveAttachmentReferencesView.empty() ? nullptr : resolveAttachmentReferencesView.data(),
                depthStencilRef,
                subpass.reserveAttachments.capacity,
                subpass.reserveAttachments.view().data()
            );
        }
        
        VkRenderPassCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.attachmentCount = attachments.size();
        createInfo.pAttachments = attachments.data();
        createInfo.subpassCount = subpasses.size();
        createInfo.pSubpasses = subpasses.data();
        createInfo.dependencyCount = dependencies.size();
        createInfo.pDependencies = dependencies.data();

        const auto * allocationCallbacks = _device.gpu().getInstance().getAllocationCallbacks();
        VkRenderPass renderPass = VK_NULL_HANDLE;
        if (const auto result = vkCreateRenderPass(_device(), &createInfo, allocationCallbacks, &renderPass);
            result != VK_SUCCESS) {
            LOG_E("vulkan: vkCreateRenderPass result: %d", result);
            return nullptr;
        }

        assert(attachments.size() <= RenderPass::kMaxAttachemts);
        stl::array<RenderPass::AttachmentInfo, RenderPass::kMaxAttachemts> attachmentInfos;
        for (size_t i = 0u; i < attachments.size(); ++i) {
            attachmentInfos[i].finalLayout = attachments[i].finalLayout;
        }

        if (auto * rp = _renderPassPool.create<RenderPass>(renderPass, attachmentInfos)) {
            const auto [it, success] = _renderPassCollection.emplace(description, rp);
            if (success) {
                return it->second;
            }
        }

        LOG_E("vulkan: RenderPassManager::getRenderPass map emplace error!");
        return nullptr;
    }

}