#include "descriptor_set_updater.h"

#include "device.h"
#include "descriptor_set.h"
#include "buffer.h"
#include "vk_types.h"

#include <cassert>

namespace engine::vulkan {

    DescriptorSetUpdater::DescriptorSetUpdater(const Device & device, const engine::rhi::shaders::BindingSetLayout & layout) : 
        _device(device), _layout(layout) {

        uint32_t buffersCount = 0u;
        uint32_t imagesCount = 0u;
        uint32_t textelBuffersCount = 0u;
        for (const auto & layout : _layout.layouts.view()) {
            switch (layout.type) {
            case rhi::shaders::DescriptorType::eUniform_Buffer : [[fallthrough]];
            case rhi::shaders::DescriptorType::eUniform_Buffer_Dynamic : [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Buffer : [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Buffer_Dynamic :
                buffersCount += layout.count;
                break;
            case rhi::shaders::DescriptorType::eSampled_Image: [[fallthrough]];
            case rhi::shaders::DescriptorType::eCombined_Image_Sampler:
                imagesCount += layout.count;
                break;
            case rhi::shaders::DescriptorType::eUniform_Texel_Buffer: [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Texel_Buffer:
                textelBuffersCount += layout.count;
                break;    
            default:
                assert(false);
                break;
            }
        }

        _buffersInfo.reserve(buffersCount);
        _imagesInfo.reserve(imagesCount);
        _texelBuffersInfo.reserve(textelBuffersCount);
    }

    void DescriptorSetUpdater::updateBuffers(std::span<const std::pair<Buffer*, uint32_t>> bufferBindings) noexcept {
        for (const auto & [buffer, binding] : bufferBindings) {
            _buffersInfo.view()[_updateBufferIndex++] = {
                {
                .buffer = buffer->operator()(),
                .offset = 0u,
                .range = buffer->size()
                },
                binding
            };
        }
    }

    void DescriptorSetUpdater::updateImages(std::span<const std::pair<void*, uint32_t>> imageBindings) noexcept {
        // todo!
    }

    void DescriptorSetUpdater::updatetexelBuffers(std::span<const std::tuple<Buffer*, rhi::Format, uint32_t>> texelBufferBindings) noexcept {
        for (const auto & [buffer, format, binding] : texelBufferBindings) {
            _texelBuffersInfo.view()[_updateTexelBufferIndex++] = {
                buffer->recieveView(format),
                binding
            };
        }
    }

    void DescriptorSetUpdater::flush(const DescriptorSet & set) noexcept {
        engine::dynamic_array<VkWriteDescriptorSet, kWritesOnStackLimit> writes;
        writes.reserve(_buffersInfo.size() + _imagesInfo.size() + _texelBuffersInfo.size());
        const auto & writeViews = writes.view();
        
        uint32_t wi = 0u;
        for (const auto & bufferView : _buffersInfo.view()) {
            writeViews[wi++] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = set(),
                .dstBinding = bufferView.binding,
                .dstArrayElement = 0u,
                .descriptorCount = 1u,
                .descriptorType = engine::vulkan::utils::convert2vk(_layout.findLayout(bufferView.binding)->type),
                .pImageInfo = nullptr,
                .pBufferInfo = &bufferView.info,
                .pTexelBufferView = nullptr
            };
        }

        for (const auto & imageView : _imagesInfo.view()) {
            writeViews[wi++] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = set(),
                .dstBinding = imageView.binding,
                .dstArrayElement = 0u,
                .descriptorCount = 1u,
                .descriptorType = engine::vulkan::utils::convert2vk(_layout.findLayout(imageView.binding)->type),
                .pImageInfo = &imageView.info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            };
        }

        for (const auto & texelBufferView : _texelBuffersInfo.view()) {
            writeViews[wi++] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = set(),
                .dstBinding = texelBufferView.binding,
                .dstArrayElement = 0u,
                .descriptorCount = 1u,
                .descriptorType = engine::vulkan::utils::convert2vk(_layout.findLayout(texelBufferView.binding)->type),
                .pImageInfo = nullptr,
                .pBufferInfo = nullptr,
                .pTexelBufferView = &texelBufferView.info
            };
        }

        vkUpdateDescriptorSets(_device(), writeViews.size(), writeViews.data(), 0u, nullptr); // need copy stets
    }

}