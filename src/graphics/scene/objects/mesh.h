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

    enum class MeshSkinProcessType : uint8_t {
        eSkinVertexShader = 0u,
        eSkinComputeShader = 1u,
        eSkinCpu = 2u
    };

    class Skeleton : public AttachmentObject {
    public:
        explicit Skeleton(const graphics::models::MeshData & meshData);
        ~Skeleton() /*override*/ = default;

        uint32_t nodesCount() const noexcept {
            return _nodes.size();
        }

        uint16_t skinMatricesCount() const noexcept;

        Node * node(uint16_t idx) noexcept {
            assert(idx < _nodes.size());
            return _nodes[idx];
        }

        const Node * node(uint16_t idx) const noexcept {
            assert(idx < _nodes.size());
            return _nodes[idx];
        }

        Node * setNode(uint16_t idx, Node * node) noexcept {
            assert(idx < _nodes.size());
            std::swap(_nodes[idx], node);
            return node;
        }

        const graphics::models::MeshData & meshData() const noexcept {
            return _meshData;
        }

        bool cmp(const Skeleton &) const noexcept;

        void fillSkinMatrices2(stl::span<mat3x4f> target) const noexcept;

    private:
        const graphics::models::MeshData & _meshData;
        stl::vector<Node *> _nodes;
    };

    class Mesh : public AttachmentObject {
    public:
        Mesh(const graphics::models::Primitive & primitive, uint32_t dataIndex, uint16_t geometryDataIndex, 
            uint16_t skin, uint32_t pipeline);
        ~Mesh() /*override*/ = default;

        void setPipeline(uint32_t pipeline) noexcept;
        uint32_t getPipeline() const noexcept;

        bool cmp(const Mesh &) const noexcept;
        void attachToNode(Node * node) noexcept;

        void setIndices(mesh::Attribute attribute) noexcept {
            _indices = std::move(attribute);
        }

        void setUniqueIndices(mesh::Attribute attribute) noexcept {
            _uniqueIndices = std::move(attribute);
        }

        void setAttribute(mesh::Attribute attribute, mesh::Semantic semantic) noexcept {
            _attributes[std::to_underlying(semantic)] = std::move(attribute);
        }

        const mesh::Attribute & getIndices() const noexcept {
            return _indices;
        }

        const mesh::Attribute & getUniqueIndices() const noexcept {
            return _uniqueIndices;
        }

        const mesh::Attribute & getAttribute(mesh::Semantic semantic) const noexcept {
            return _attributes[std::to_underlying(semantic)];
        }

        const graphics::models::Primitive & geometry() const noexcept { return _geometry; }

        uint32_t dataIndex() const noexcept { return _dataIndex; }
        uint16_t geometryDataIndex() const noexcept { return _geometryDataIndex; }

        uint16_t materialIndex() const noexcept { return _material; }
        void setMaterialIndex(uint16_t idx) noexcept { _material = idx; }

        // skeleton:
        const Skeleton * getSkeleton() const noexcept { return _skeleton; }
        void setSkeleton(Skeleton * skeleton) noexcept { _skeleton = skeleton; }

        uint32_t getVertexCount() const noexcept {
            return getAttribute(mesh::Semantic::ePosition).buffer->sizeInBytes() / 8u;
        }

    private:
        const graphics::models::Primitive & _geometry;
        Skeleton * _skeleton = nullptr;

        mesh::Attribute _indices;
        mesh::Attribute _uniqueIndices;
        stl::array<mesh::Attribute, static_cast<uint8_t>(mesh::Semantic::eSemanticsCount)> _attributes;

        uint32_t _pipeline = 0u;
        uint32_t _dataIndex = 0xff'ff'ff'ffu;
        uint16_t _geometryDataIndex = 0u;
        uint16_t _skin = graphics::models::kInvalidValue;
        uint16_t _material = 0u;     
    };

}