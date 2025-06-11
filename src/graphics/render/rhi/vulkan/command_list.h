#pragma once

#include "graphics/render/rhi/common/runtime.h"
#include "graphics/render/rhi/common/command_list.h"
#include "barriers.h"
#include "command_buffer.h"

#include <span>
#include <vector>

namespace engine::vulkan::runtime {

    class CommandList : public rhi::runtime::CommandList {
        template <typename T>
        struct ReusedVector {
            std::vector<T> vec{};
            size_t current = 0u;

            constexpr void reserve(size_t n) {
                vec.reserve(n);
            }

            std::vector<T>* operator->() noexcept { return &vec; }
            void clear() noexcept {
                current = 0u;
            }

            template <typename...Args>
            void emplace_back(Args&&...args) {
                if (current < vec.size()) {
                    vec[current] = T(std::forward<Args>(args)...);
                } else {
                    vec.emplace_back(std::forward<Args>(args)...);
                }
                ++current;
            }

            std::span<const T> view() const noexcept {
                return {vec.begin(), vec.begin() + current};
            }
        };

        struct PostBufferBarrierInfo {
            Buffer * buffer;
            size_t offset;
            size_t size;
        };

        struct PostImageBarrierInfo {
            Image * image;
        };

    public:
        CommandList(rhi::runtime::CommandPool * commandPool, 
            rhi::CommandBufferLevel level = rhi::CommandBufferLevel::ePrimary);
        ~CommandList() = default;

        void record(uint8_t frameIdx) noexcept override;

        const CommandBuffer & commandBuffer() const noexcept { return _commandbuffer; }

    private:
        void resetState() noexcept;

        CommandPool * _pool = nullptr;
        CommandBuffer _commandbuffer;

        PipelineBarriers _barriers;
        PipelineBarriers _postBarriers;

        ReusedVector<PostBufferBarrierInfo> _postBarriersBuffers;
        ReusedVector<PostImageBarrierInfo> _postBarriersImages;
    };
}