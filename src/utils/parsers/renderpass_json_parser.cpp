#include "renderpass_json_parser.h"

#include <string_view>

namespace engine {

    constexpr auto parsePipelineStateFlags(std::string_view flags) noexcept {
        auto result = engine::rhi::PipelineStageFlags::eNone;
        if (flags.contains("eTop_Of_Pipe")) {
            result = result | engine::rhi::PipelineStageFlags::eTop_Of_Pipe;
        } 
        if (flags.contains("eDraw_Indirect")) {
            result = result | engine::rhi::PipelineStageFlags::eDraw_Indirect;
        } 
        if (flags.contains("eVertex_Input")) {
            result = result | engine::rhi::PipelineStageFlags::eVertex_Input;
        } 
        if (flags.contains("eVertex_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eVertex_Shader;
        } 
        if (flags.contains("eTesselation_Control_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eTesselation_Control_Shader;
        } 
        if (flags.contains("eTesselation_Evaluation_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eTesselation_Evaluation_Shader;
        } 
        if (flags.contains("eGeometry_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eGeometry_Shader;
        } 
        if (flags.contains("eFragment_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eFragment_Shader;
        } 
        if (flags.contains("eEarly_Fragment_Tests")) {
            result = result | engine::rhi::PipelineStageFlags::eEarly_Fragment_Tests;
        } 
        if (flags.contains("eLate_Fragment_Tests")) {
            result = result | engine::rhi::PipelineStageFlags::eLate_Fragment_Tests;
        } 
        if (flags.contains("eColor_Attachment_Output")) {
            result = result | engine::rhi::PipelineStageFlags::eColor_Attachment_Output;
        } 
        if (flags.contains("eCompute_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eCompute_Shader;
        } 
        if (flags.contains("eTransfer")) {
            result = result | engine::rhi::PipelineStageFlags::eTransfer;
        } 
        if (flags.contains("eBottom_Of_Pipe")) {
            result = result | engine::rhi::PipelineStageFlags::eBottom_Of_Pipe;
        } 
        if (flags.contains("eHost")) {
            result = result | engine::rhi::PipelineStageFlags::eHost;
        } 
        if (flags.contains("eAll_Graphics")) {
            result = result | engine::rhi::PipelineStageFlags::eAll_Graphics;
        } 
        if (flags.contains("eAll_Commands")) {
            result = result | engine::rhi::PipelineStageFlags::eAll_Commands;
        } 
        if (flags.contains("eTransform_Feedback_Ext")) {
            result = result | engine::rhi::PipelineStageFlags::eTransform_Feedback_Ext;
        } 
        if (flags.contains("eConditional_Rendering_Ext")) {
            result = result | engine::rhi::PipelineStageFlags::eConditional_Rendering_Ext;
        } 
        if (flags.contains("eAcceleration_Structure_Build")) {
            result = result | engine::rhi::PipelineStageFlags::eAcceleration_Structure_Build;
        } 
        if (flags.contains("eRay_Tracing_Shader")) {
            result = result | engine::rhi::PipelineStageFlags::eRay_Tracing_Shader;
        } 
        if (flags.contains("eFragment_Density_Process_Ext")) {
            result = result | engine::rhi::PipelineStageFlags::eFragment_Density_Process_Ext;
        } 
        if (flags.contains("eFragment_Shading_Rate_Attachment")) {
            result = result | engine::rhi::PipelineStageFlags::eFragment_Shading_Rate_Attachment;
        } 
        if (flags.contains("eCommand_Preprocess_Ext")) {
            result = result | engine::rhi::PipelineStageFlags::eCommand_Preprocess_Ext;
        } 
        if (flags.contains("eTast_Shader_Ext")) {
            result = result | engine::rhi::PipelineStageFlags::eTast_Shader_Ext;
        } 
        if (flags.contains("eMesh_Shader_Ext")) {
            result = result | engine::rhi::PipelineStageFlags::eMesh_Shader_Ext;
        } 
        return result;
    }

    constexpr auto parseAccessFlags(std::string_view flags) noexcept {
        auto result = engine::rhi::AccessFlags::eNone;
        if (flags.contains("eIndirect_Command_Read")) {
            result = result | engine::rhi::AccessFlags::eIndirect_Command_Read;
        } 
        if (flags.contains("eIndex_Read")) {
            result = result | engine::rhi::AccessFlags::eIndex_Read;
        } 
        if (flags.contains("eVertex_Attribute_Read")) {
            result = result | engine::rhi::AccessFlags::eVertex_Attribute_Read;
        } 
        if (flags.contains("eUniform_Read")) {
            result = result | engine::rhi::AccessFlags::eUniform_Read;
        } 
        if (flags.contains("eInput_Attachment_Read")) {
            result = result | engine::rhi::AccessFlags::eInput_Attachment_Read;
        } 
        if (flags.contains("eShader_Read")) {
            result = result | engine::rhi::AccessFlags::eShader_Read;
        } 
        if (flags.contains("eShader_Write")) {
            result = result | engine::rhi::AccessFlags::eShader_Write;
        } 
        if (flags.contains("eColor_Attachment_Read")) {
            result = result | engine::rhi::AccessFlags::eColor_Attachment_Read;
        } 
        if (flags.contains("eColor_Attachment_Write")) {
            result = result | engine::rhi::AccessFlags::eColor_Attachment_Write;
        } 
        if (flags.contains("eDepth_Stencil_Attachment_Read")) {
            result = result | engine::rhi::AccessFlags::eDepth_Stencil_Attachment_Read;
        } 
        if (flags.contains("eDepth_Stencil_Attachment_Write")) {
            result = result | engine::rhi::AccessFlags::eDepth_Stencil_Attachment_Write;
        } 
        if (flags.contains("eTransfer_Read")) {
            result = result | engine::rhi::AccessFlags::eTransfer_Read;
        } 
        if (flags.contains("eTransfer_Write")) {
            result = result | engine::rhi::AccessFlags::eTransfer_Write;
        } 
        if (flags.contains("eHost_Read")) {
            result = result | engine::rhi::AccessFlags::eHost_Read;
        } 
        if (flags.contains("eHost_Write")) {
            result = result | engine::rhi::AccessFlags::eHost_Write;
        } 
        if (flags.contains("eMemory_Read")) {
            result = result | engine::rhi::AccessFlags::eMemory_Read;
        } 
        if (flags.contains("eMemory_Write")) {
            result = result | engine::rhi::AccessFlags::eMemory_Write;
        } 
        if (flags.contains("eTransfrom_Feedback_Write_Ext")) {
            result = result | engine::rhi::AccessFlags::eTransfrom_Feedback_Write_Ext;
        } 
        if (flags.contains("eTransfrom_Feedback_Counter_Read_Ext")) {
            result = result | engine::rhi::AccessFlags::eTransfrom_Feedback_Counter_Read_Ext;
        } 
        if (flags.contains("eTransfrom_Feedback_Counter_Write_Ext")) {
            result = result | engine::rhi::AccessFlags::eTransfrom_Feedback_Counter_Write_Ext;
        } 
        if (flags.contains("eConditional_Rendering_Read_Ext")) {
            result = result | engine::rhi::AccessFlags::eConditional_Rendering_Read_Ext;
        } 
        if (flags.contains("eColor_Attachment_Read_Noncoherent_Ext")) {
            result = result | engine::rhi::AccessFlags::eColor_Attachment_Read_Noncoherent_Ext;
        } 
        if (flags.contains("eAcceleration_Structure_Read")) {
            result = result | engine::rhi::AccessFlags::eAcceleration_Structure_Read;
        } 
        if (flags.contains("eAcceleration_Structure_Write")) {
            result = result | engine::rhi::AccessFlags::eAcceleration_Structure_Write;
        } 
        if (flags.contains("eFragment_Density_Map_Read_Ext")) {
            result = result | engine::rhi::AccessFlags::eHost_Write;
        } 
        if (flags.contains("eFragment_Shading_Rate_Attachment_Read")) {
            result = result | engine::rhi::AccessFlags::eHost_Write;
        } 
        if (flags.contains("eCommand_Preprocess_Read_Ext")) {
            result = result | engine::rhi::AccessFlags::eHost_Write;
        } 
        if (flags.contains("eCommand_Preprocess_Write_Ext")) {
            result = result | engine::rhi::AccessFlags::eHost_Write;
        } 
        return result;
    }

    constexpr auto parseImageLayout(std::string_view layout) noexcept {
        if (layout == "eUndefined") {
            return engine::rhi::ImageLayout::eUndefined;
        } else if (layout == "eGeneral") {
            return engine::rhi::ImageLayout::eGeneral;
        } else if (layout == "eColor_Attachment_Optimal") {
            return engine::rhi::ImageLayout::eColor_Attachment_Optimal;
        } else if (layout == "eDepth_Stencil_Attachment_Optimal") {
            return engine::rhi::ImageLayout::eDepth_Stencil_Attachment_Optimal;
        } else if (layout == "eDepth_Stencil_Read_Only_Optimal") {
            return engine::rhi::ImageLayout::eDepth_Stencil_Read_Only_Optimal;
        } else if (layout == "eShader_Read_Only_Optimal") {
            return engine::rhi::ImageLayout::eShader_Read_Only_Optimal;
        } else if (layout == "eTransfer_Src_Optimal") {
            return engine::rhi::ImageLayout::eTransfer_Src_Optimal;
        } else if (layout == "eTransfer_Dst_Optimal") {
            return engine::rhi::ImageLayout::eTransfer_Dst_Optimal;
        } else if (layout == "ePreinitialized") {
            return engine::rhi::ImageLayout::ePreinitialized;
        } else if (layout == "eDepth_Read_Only_Stencil_Attachment_Optimal") {
            return engine::rhi::ImageLayout::eDepth_Read_Only_Stencil_Attachment_Optimal;
        } else if (layout == "eDepth_Attachment_Stencil_Read_Only_Optimal") {
            return engine::rhi::ImageLayout::eDepth_Attachment_Stencil_Read_Only_Optimal;
        } else if (layout == "eDepth_Attachment_Optimal") {
            return engine::rhi::ImageLayout::eDepth_Attachment_Optimal;
        } else if (layout == "eDepth_Read_Only_Optimal") {
            return engine::rhi::ImageLayout::eDepth_Read_Only_Optimal;
        } else if (layout == "eStencil_Attachment_Optimal") {
            return engine::rhi::ImageLayout::eStencil_Attachment_Optimal;
        } else if (layout == "eStencil_Read_Only_Optimal") {
            return engine::rhi::ImageLayout::eStencil_Read_Only_Optimal;
        } else if (layout == "eRead_Only_Optimal") {
            return engine::rhi::ImageLayout::eRead_Only_Optimal;
        } else if (layout == "eAttachment_Optimal") {
            return engine::rhi::ImageLayout::eAttachment_Optimal;
        } else if (layout == "ePresent_Src") {
            return engine::rhi::ImageLayout::ePresent_Src;
        } else if (layout == "eShared_Present") {
            return engine::rhi::ImageLayout::eShared_Present;
        } else if (layout == "eFragment_Density_Map_Optimal_Ext") {
            return engine::rhi::ImageLayout::eFragment_Density_Map_Optimal_Ext;
        } else if (layout == "eFragment_Shading_Rate_Attachment_Optimal") {
            return engine::rhi::ImageLayout::eFragment_Shading_Rate_Attachment_Optimal;
        } else if (layout == "eAttachment_Feedback_Loop_Optimal_Ext") {
            return engine::rhi::ImageLayout::eAttachment_Feedback_Loop_Optimal_Ext;
        }
        return engine::rhi::ImageLayout::eUndefined;
    }

    constexpr auto parseLoadStoreOp(const stl::pair<std::string_view, std::string_view> & loadStoreOp) noexcept {
        auto loadOp = engine::rhi::AttachmentLoadOp::eNone;
        if (loadStoreOp.first == "eLoad") {
            loadOp = engine::rhi::AttachmentLoadOp::eLoad;
        } else if (loadStoreOp.first == "eClear") {
            loadOp = engine::rhi::AttachmentLoadOp::eClear;
        } else {
            loadOp = engine::rhi::AttachmentLoadOp::eDont_Care;
        }

        auto storeOp = engine::rhi::AttachmentStoreOp::eNone;
        if (loadStoreOp.second == "eStore") {
            storeOp = engine::rhi::AttachmentStoreOp::eStore;
        } else  {
            storeOp = engine::rhi::AttachmentStoreOp::eDont_Care;
        }
        
        return engine::rhi::AttachmentLoadStoreOp{
            loadOp, 
            storeOp
        };
    }

    constexpr auto parseSubpassAttachments(
        const engine::json::Array & json, 
        stl::span<engine::rhi::AttachmentReference> view
    ) {
        size_t i = 0u;
        for (const auto & js: json) {
            view[i].attachment = js["attachment"].as<uint16_t>();
            view[i].layout = parseImageLayout(js["layout"].as<std::string_view>());
            ++i;
        }
    }

    rhi::RenderPassDescription parseRenderPass(const engine::json::Json & json, stl::span<const engine::rhi::Format> formats) noexcept {
        size_t i = 0u;
        const auto & attachmentsJson = json["attachments"].get_array();
        stl::vector<engine::rhi::AttachmentDescription> attachments;
        attachments.resize(attachmentsJson.size());
        for (const auto & js : attachmentsJson) {
            const auto & jsLoadStoreOp = js["loadStoreOp"].get_array();
            const auto loadStoreOp = stl::pair<std::string_view, std::string_view>{
                jsLoadStoreOp[0].as<std::string_view>(),
                jsLoadStoreOp[1].as<std::string_view>()
            };

            const auto & jsStencilLoadStoreOp = js["stencilLoadStoreOp"].get_array();
            const auto stencilLoadStoreOp = stl::pair<std::string_view, std::string_view>{
                jsStencilLoadStoreOp[0].as<std::string_view>(),
                jsStencilLoadStoreOp[1].as<std::string_view>()
            };

            attachments[i].format = formats[i];

            attachments[i].initialLayout = parseImageLayout(json::getValue(js, "initialLayout", std::string_view("eUndefined")));
            attachments[i].finalLayout = parseImageLayout(js["finalLayout"].as<std::string_view>());
            attachments[i].loadStoreOp = parseLoadStoreOp(loadStoreOp);
            attachments[i].stencilLoadStoreOp = parseLoadStoreOp(stencilLoadStoreOp);
            ++i;
        }

        i = 0u;
        const auto & subpassesJson = json["subpasses"].get_array();
        stl::vector<engine::rhi::SubpassDescription> subpasses;
        subpasses.resize(subpassesJson.size());
        for (const auto & js : subpassesJson) {
            auto & subpass = subpasses[i];

            if (js.contains("reserveAttachments")) {
                const auto & reserveAttachmentsArray = js["reserveAttachments"].get_array();
                subpass.reserveAttachments.resize(reserveAttachmentsArray.size());
                for (const auto & ra : reserveAttachmentsArray) {
                    subpass.reserveAttachments.emplace_back(ra.as<uint32_t>());
                }
            }
            
            if (js.contains("inputAttachments")) {
                const auto & inputAttachmentsArray = js["inputAttachments"].get_array();
                subpass.inputAttachments.resize(inputAttachmentsArray.size());
                parseSubpassAttachments(inputAttachmentsArray, subpass.inputAttachments.view());
            }

            if (js.contains("resolveAttachments")) {
                const auto & resolveAttachmentsArray = js["resolveAttachments"].get_array();
                subpass.resolveAttachments.resize(resolveAttachmentsArray.size());
                parseSubpassAttachments(resolveAttachmentsArray, subpass.resolveAttachments.view());
            }

            if (js.contains("colorAttachments")) {
                const auto & colorAttachmentsArray = js["colorAttachments"].get_array();
                subpass.colorAttachments.resize(colorAttachmentsArray.size());
                parseSubpassAttachments(colorAttachmentsArray, subpass.colorAttachments.view());
            }

            if (js.contains("depthStencilAttachment")) {
                const auto & depthStencil = js["depthStencilAttachment"];
                subpass.depthStencilAttachment.attachment = depthStencil["attachment"].as<uint16_t>();
                subpass.depthStencilAttachment.layout = parseImageLayout(depthStencil["layout"].as<std::string_view>());
            }
            
            ++i;
        }

        stl::vector<engine::rhi::SubpassDependency> dependensies;
        if (json.contains("subpassDependensies")) {
            i = 0;
            const auto & depsJson = json["subpassDependensies"].get_array();
            dependensies.resize(depsJson.size());
            for (const auto & js : depsJson) {
                auto & dep = dependensies[i];
                dep.srcSubpass = js["srcSubpass"].as<uint32_t>();
                dep.dstSubpass = js["dstSubpass"].as<uint32_t>();
                auto dependencyFlags = js["dependencyFlags"].as<std::string_view>();
                if (dependencyFlags == "eBy_Region") {
                    dep.dependencyFlags = engine::rhi::DependencyFlags::eBy_Region;
                } else if (dependencyFlags == "eDevice_Group") {
                    dep.dependencyFlags = engine::rhi::DependencyFlags::eDevice_Group;
                } else if (dependencyFlags == "eView_Local") {
                    dep.dependencyFlags = engine::rhi::DependencyFlags::eView_Local;
                } else if (dependencyFlags == "eFeedback_Loop_Ext") {
                    dep.dependencyFlags = engine::rhi::DependencyFlags::eFeedback_Loop_Ext;
                } else if (dependencyFlags == "eQueue_Family_Ownership_Transfer_Use_All_Stages") {
                    dep.dependencyFlags = engine::rhi::DependencyFlags::eQueue_Family_Ownership_Transfer_Use_All_Stages;
                }

                dep.stages = {
                    .src = {
                        .stage = parsePipelineStateFlags(js["srcStage"].as<std::string_view>()),
                        .accessMask = parseAccessFlags(js["srcAccessMask"].as<std::string_view>())
                    },
                    .dst = {
                        .stage = parsePipelineStateFlags(js["dstStage"].as<std::string_view>()),
                        .accessMask = parseAccessFlags(js["dstAccessMask"].as<std::string_view>()),
                    }
                };
                ++i;
            }
        }

        return engine::rhi::RenderPassDescription {
            std::move(attachments), 
            std::move(subpasses), 
            std::move(dependensies)
        };
    }
}