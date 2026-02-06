#pragma once

#include "graphics/mesh/mesh_data.h"
#include "graphics/mesh/types.h"
#include "graphics/render/rhi/common/runtime.h"
#include "graphics/scene/object.h"

#include "math/math.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <span>
#include <utility>

namespace engine::scene {

namespace mesh {
    using Semantic = graphics::models::AttributeSemantic;

    struct Attribute {
        rhi::runtime::BufferPtr buffer = nullptr;
        // uint32_t offset = 0u;
        // uint32_t size = 0u;
    };
}

    class Skeleton : public AttachmentObject {
    public:
        Skeleton(const graphics::models::Mesh_Data & meshData);
        ~Skeleton() /*override*/ = default;

        uint32_t nodesCount() const noexcept {
            return _nodes.size();
        }

        uint16_t skinMatricesCount() const noexcept;

        Node *& node(uint16_t idx) noexcept {
            assert(idx < _nodes.size());
            return _nodes[idx];
        }

        const graphics::models::Mesh_Data & meshData() const noexcept {
            return _meshData;
        }

        bool cmp(const Skeleton &) const noexcept;

        void fillSkinMatrices2(std::span<mat3x4f> target) const noexcept;

    private:
        const graphics::models::Mesh_Data & _meshData;
        std::vector<Node *> _nodes;
    };

    class Mesh : public AttachmentObject {
    public:
        Mesh(const graphics::models::Mesh_Data & meshData, uint16_t meshIdx, uint16_t skin, uint32_t pipeline);
        ~Mesh() /*override*/ = default;

        void setPipeline(uint32_t pipeline) noexcept;
        uint32_t getPipeline() const noexcept;

        bool cmp(const Mesh &) const noexcept;
        void attachToNode(Node * node) noexcept;

        void setIndices(mesh::Attribute attribute) noexcept {
            _indices = std::move(attribute);
        }

        void setAttribute(mesh::Attribute attribute, mesh::Semantic semantic) noexcept {
            _attributes[std::to_underlying(semantic)] = std::move(attribute);
        }

        const mesh::Attribute & getIndices() const noexcept {
            return _indices;
        }

        const mesh::Attribute & getAttribute(mesh::Semantic semantic) const noexcept {
            return _attributes[std::to_underlying(semantic)];
        }

        const graphics::models::Mesh_Data & meshData() const noexcept {
            return _meshData;
        }

        const graphics::models::Mesh_Geometry geometry() const noexcept {
            return _meshData.meshes[_meshIdx];
        }

        uint16_t meshIndex() const noexcept {
            return _meshIdx;
        }

        // skeleton:
        const Skeleton * getSkeleton() const noexcept { return _skeleton; }
        void setSkeleton(Skeleton * skeleton) noexcept { _skeleton = skeleton; }

    private:
        const graphics::models::Mesh_Data & _meshData;

        mesh::Attribute _indices;
        std::array<mesh::Attribute, static_cast<uint8_t>(mesh::Semantic::eSemanticsCount)> _attributes;
        Skeleton * _skeleton = nullptr;
        
        uint32_t _pipeline = 0u;
        uint16_t _meshIdx = 0u;
        uint16_t _skin = graphics::models::kInvalidValue;
    };

}