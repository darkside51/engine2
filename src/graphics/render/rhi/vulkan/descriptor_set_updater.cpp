#include "descriptor_set_updater.h"

#include "device.h"
#include "physical_device.h"
#include "descriptor_set.h"
#include "buffer.h"
#include "vk_types.h"

#include <algorithm>
#include <cassert>

namespace engine::vulkan {

    DescriptorSetUpdater::DescriptorSetUpdater(const Device & device, const engine::rhi::shaders::BindingSetLayout & layout) : 
        _device(&device), 
        _layout(&layout),
        _minUniformBufferOffsetAlignment(_device->gpu().deviceProperties().limits.minUniformBufferOffsetAlignment),
        _minTexelBufferOffsetAlignment(_device->gpu().deviceProperties().limits.minTexelBufferOffsetAlignment),
        _minStorageBufferOffsetAlignment(_device->gpu().deviceProperties().limits.minStorageBufferOffsetAlignment) {

        uint32_t buffersCount = 0u;
        uint32_t imagesCount = 0u;
        uint32_t textelBuffersCount = 0u;
        for (const auto & layout : _layout->layouts.view()) {
            switch (layout.type) {
            case rhi::shaders::DescriptorType::eUniform_Buffer : [[fallthrough]];
            case rhi::shaders::DescriptorType::eUniform_Buffer_Dynamic : [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Buffer : [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Buffer_Dynamic :
                buffersCount += layout.count;
                break;
            case rhi::shaders::DescriptorType::eInput_Attachment: [[fallthrough]]; // https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorImageInfo.html
            case rhi::shaders::DescriptorType::eSampler: [[fallthrough]];
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

        _buffersInfo.resize(buffersCount);
        _imagesInfo.resize(imagesCount);
        _texelBuffersInfo.resize(textelBuffersCount);

        uint8_t updateBufferIndex = 0u;
        uint8_t updateImageIndex = 0u;
        uint8_t updateTexelBufferIndex = 0u;

        for (const auto & layout : _layout->layouts.view()) {
            switch (layout.type) {
            case rhi::shaders::DescriptorType::eUniform_Buffer : [[fallthrough]];
            case rhi::shaders::DescriptorType::eUniform_Buffer_Dynamic : [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Buffer : [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Buffer_Dynamic :
            {
                _buffersInfo.view()[updateBufferIndex++] = {
                    {
                        {
                        .buffer = VK_NULL_HANDLE,
                        .offset = 0u,
                        .range = 0u
                        },
                        0xff'ff'ff'ffu
                    }, 
                    layout.binding
                };
            }
                break;
            case rhi::shaders::DescriptorType::eInput_Attachment: [[fallthrough]]; // https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorImageInfo.html
            case rhi::shaders::DescriptorType::eSampler: [[fallthrough]];
            case rhi::shaders::DescriptorType::eSampled_Image: [[fallthrough]];
            case rhi::shaders::DescriptorType::eCombined_Image_Sampler:
            {
                _imagesInfo.view()[updateImageIndex++] = {
                    {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = VK_NULL_HANDLE, 
                    .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
                    },
                    layout.binding
                };
            }
                break;
            case rhi::shaders::DescriptorType::eUniform_Texel_Buffer: [[fallthrough]];
            case rhi::shaders::DescriptorType::eStorage_Texel_Buffer:
            {
                _texelBuffersInfo.view()[updateTexelBufferIndex++] = {
                    VK_NULL_HANDLE,
                    layout.binding
                };
            }
                break;    
            default:
                assert(false);
                break;
            }
        }
    }

    void DescriptorSetUpdater::updateBuffers(std::span<const std::pair<Buffer*, uint32_t>> bufferBindings) noexcept {
        if (!isValid()) { 
            return; 
        }

        const auto view = _buffersInfo.view();
        for (const auto & [buffer, binding] : bufferBindings) {
            const auto it = std::find_if(view.begin(), view.end(), [binding](const DescriptorInfo<DescriptorBufferInfo> & info) {
                return info.binding == binding;
            });

            if (it != view.end()) {
                const auto vkBuffer = buffer->operator()();
                auto & info = it->info;
                if (info.vkInfo.buffer != vkBuffer) {
                    info.vkInfo.buffer = vkBuffer;
                    info.vkInfo.offset = 0u;
                    info.vkInfo.range = static_cast<VkDeviceSize>(buffer->sizeInBytes());
                    _buffersDirty = true;
                }
            }
        }
    }

    void DescriptorSetUpdater::updateImages(std::span<const std::tuple<VkSampler, VkImageView, VkImageLayout, uint32_t>> imageBindings) noexcept {
        if (!isValid()) { 
            return; 
        }

        const auto view = _imagesInfo.view();
        for (const auto & [sampler, imageView, layout, binding] : imageBindings) {
            const auto it = std::find_if(view.begin(), view.end(), [binding](const DescriptorInfo<VkDescriptorImageInfo> & info) {
                return info.binding == binding;
            });

            if (it != view.end()) {
                auto & info = it->info;
                if (info.imageView != imageView || info.sampler != sampler || info.imageLayout != layout) {
                    info.sampler = sampler;
                    info.imageView = imageView;
                    info.imageLayout = layout; // must be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, or can be VK_IMAGE_LAYOUT_GENERAL sometimes (DescriptorType::eStorage_Image)?
                    _imagesDirty = true;
                }
            }
        }
    }

    void DescriptorSetUpdater::updatetexelBuffers(std::span<const std::tuple<Buffer*, rhi::Format, uint32_t>> texelBufferBindings) noexcept {
        if (!isValid()) { 
            return; 
        }

        const auto view = _texelBuffersInfo.view();
        for (const auto & [buffer, format, binding] : texelBufferBindings) {
            const auto it = std::find_if(view.begin(), view.end(), [binding](const DescriptorInfo<VkBufferView> & info) {
                return info.binding == binding;
            });

            if (it != view.end()) {
                auto const vkBufferView = buffer->recieveView(format);
                auto & bufferView = it->info;
                if (bufferView != vkBufferView) {
                    bufferView = vkBufferView;
                    _texelBufferDirty = true;
                }
            }
        }
    }

    DescriptorSetUpdater::BufferUpdateResult DescriptorSetUpdater::updateBuffer(const Buffer* buffer, 
        uint32_t binding, uint32_t offset, uint32_t range, bool dynamic) noexcept {
        if (!isValid()) { 
            return BufferUpdateResult::eNone; 
        }

        const auto view = _buffersInfo.view();
        const auto it = std::find_if(view.begin(), view.end(), [binding](const DescriptorInfo<DescriptorBufferInfo> & info) {
            return info.binding == binding;
        });

        if (it != view.end()) {
            const auto vkBuffer = buffer->operator()();
            auto & info = it->info;

            // // offset must be aligned :
            // uint32_t offset2 = 0u;
            // const auto type = _layout->findLayout(binding)->type;
            // switch (type) {
            // case rhi::shaders::DescriptorType::eUniform_Buffer:
            // case rhi::shaders::DescriptorType::eUniform_Buffer_Dynamic:
            //     offset2 = align_value(offset, _minUniformBufferOffsetAlignment);
            //     break;
            // case rhi::shaders::DescriptorType::eStorage_Buffer:
            // case rhi::shaders::DescriptorType::eStorage_Buffer_Dynamic:
            //     offset2 = align_value(offset, _minStorageBufferOffsetAlignment);
            //     break;
            // default:
            //     break;
            // }

            // if (offset != offset2) {
            //     offset = offset2;
            // }

            #ifdef DEBUG
            const auto type = _layout->findLayout(binding)->type;
            const bool isDynamic = type == rhi::shaders::DescriptorType::eUniform_Buffer_Dynamic || 
                                   type == rhi::shaders::DescriptorType::eStorage_Buffer_Dynamic;
            assert(dynamic == isDynamic);
            #endif

            bool dynamicOffsetChanged = false;
            if (dynamic) {
                // store offset
                if (info.dynamicOffset != offset) {
                    info.dynamicOffset = offset;
                    dynamicOffsetChanged = true;
                }
                // reset input data offset
                offset = 0u;
            }

            const auto finalRange = (range == rhi::kWholeSize32) ? static_cast<VkDeviceSize>(buffer->sizeInBytes()) - offset : static_cast<VkDeviceSize>(range) - offset;
            if (info.vkInfo.buffer != vkBuffer || info.vkInfo.offset != offset || info.vkInfo.range != finalRange) {
                info.vkInfo.buffer = vkBuffer;
                info.vkInfo.offset = offset;
                info.vkInfo.range = finalRange;
                _buffersDirty = true;
                return dynamic ? BufferUpdateResult::eChangeBufferAndDynamicOffset : BufferUpdateResult::eChangeBufferOrOffset;
            }

            return dynamicOffsetChanged ? BufferUpdateResult::eChangeDynamicOffset : BufferUpdateResult::eNone;
        }

        return BufferUpdateResult::eNone;
    }

    bool DescriptorSetUpdater::updateImage(std::tuple<VkSampler, VkImageView, VkImageLayout> image, uint32_t binding) noexcept {
        if (!isValid()) { 
            return false; 
        }

        const auto view = _imagesInfo.view();
        const auto & [sampler, imageView, layout] = image;
        const auto it = std::find_if(view.begin(), view.end(), [binding](const DescriptorInfo<VkDescriptorImageInfo> & info) {
            return info.binding == binding;
        });

        if (it != view.end()) {
            auto & info = it->info;
            if (info.imageView != imageView || info.sampler != sampler || info.imageLayout != layout) {
                info.sampler = sampler;
                info.imageView = imageView;
                info.imageLayout = layout; // must be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, or can be VK_IMAGE_LAYOUT_GENERAL sometimes (DescriptorType::eStorage_Image)?
                _imagesDirty = true;
                return true;
            }
        }
        return false;
    }

    bool DescriptorSetUpdater::updateTexelBuffer(Buffer* buffer, rhi::Format format, uint32_t binding, uint32_t offset, uint32_t range) noexcept {
        if (!isValid()) { 
            return false; 
        }

        const auto view = _texelBuffersInfo.view();
        const auto it = std::find_if(view.begin(), view.end(), [binding](const DescriptorInfo<VkBufferView> & info) {
            return info.binding == binding;
        });

        if (it != view.end()) {

            // // offset must be aligned :
            // uint32_t offset2 = 0u;
            // const auto type = _layout->findLayout(binding)->type;
            // switch (type) {
            // case rhi::shaders::DescriptorType::eUniform_Texel_Buffer:
            //     offset2 = align_value(offset, _minTexelBufferOffsetAlignment);
            //     break;
            // case rhi::shaders::DescriptorType::eStorage_Texel_Buffer:
            //     offset2 = align_value(offset, _minTexelBufferOffsetAlignment);
            //     break;
            // default:
            //     break;
            // }

            // if (offset != offset2) {
            //     offset = offset2;
            // }

            auto const vkBufferView = buffer->recieveView(format, offset, range);
            auto & bufferView = it->info;

            if (bufferView != vkBufferView) {
                bufferView = vkBufferView;
                _texelBufferDirty = true;
                return true;
            }
        }
        return false;
    }

    void DescriptorSetUpdater::fillDynamicOffsets(dynamic_array<uint32_t, 4u * kDescriptorsOnStackLimit> & dynamicOffsets) noexcept {
        if (!isValid()) { 
            return; 
        }

        for (const auto & bufferView : _buffersInfo.view()) {
            if (bufferView.info.dynamicOffset != 0xff'ff'ff'ffu) {
                dynamicOffsets.emplace_back(bufferView.info.dynamicOffset);
            }
        }
    }

    void DescriptorSetUpdater::flush(const DescriptorSet & set) noexcept {
        if (!isValid()) { 
            return; 
        }

        const auto targetSet = set();
        const bool setChanged = _set != targetSet;
        if (!_buffersDirty && !_imagesDirty && !_texelBufferDirty && !setChanged) {
            return;
        }

        _set = targetSet;

        engine::dynamic_array<VkWriteDescriptorSet, kWritesOnStackLimit> writes;
        writes.resize(_buffersInfo.size() + _imagesInfo.size() + _texelBuffersInfo.size());
        const auto writeViews = writes.view();
        
        // structures VkWriteDescriptorSetInlineUniformBlock, VkWriteDescriptorSetAccelerationStructureKHR 
        // can be available as pNext param for VkWriteDescriptorSet
        // no support it yet!

        uint32_t wi = 0u;
        if (_buffersDirty || setChanged) {
            for (const auto & bufferView : _buffersInfo.view()) {
                writeViews[wi++] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = _set,
                    .dstBinding = bufferView.binding,
                    .dstArrayElement = 0u,
                    .descriptorCount = 1u,
                    .descriptorType = engine::vulkan::utils::convert2vk(_layout->findLayout(bufferView.binding)->type, 
                        bufferView.info.dynamicOffset != 0xff'ff'ff'ffu),
                    .pImageInfo = nullptr,
                    .pBufferInfo = &bufferView.info.vkInfo,
                    .pTexelBufferView = nullptr
                };
            }
            _buffersDirty = false;
        }

        if (_imagesDirty || setChanged) {
            for (const auto & imageView : _imagesInfo.view()) {
                writeViews[wi++] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = _set,
                    .dstBinding = imageView.binding,
                    .dstArrayElement = 0u,
                    .descriptorCount = 1u,
                    .descriptorType = engine::vulkan::utils::convert2vk(_layout->findLayout(imageView.binding)->type),
                    .pImageInfo = &imageView.info,
                    .pBufferInfo = nullptr,
                    .pTexelBufferView = nullptr
                };
            }
            _imagesDirty = false;
        }

        if (_texelBufferDirty || setChanged) {
            for (const auto & texelBufferView : _texelBuffersInfo.view()) {
                writeViews[wi++] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = _set,
                    .dstBinding = texelBufferView.binding,
                    .dstArrayElement = 0u,
                    .descriptorCount = 1u,
                    .descriptorType = engine::vulkan::utils::convert2vk(_layout->findLayout(texelBufferView.binding)->type),
                    .pImageInfo = nullptr,
                    .pBufferInfo = nullptr,
                    .pTexelBufferView = &texelBufferView.info
                };
            }
            _texelBufferDirty = false;
        }

        vkUpdateDescriptorSets((*_device)(), writeViews.size(), writeViews.data(), 0u, nullptr); // need copy sets?
    }

}