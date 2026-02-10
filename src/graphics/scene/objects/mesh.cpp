#include "mesh.h"

#include "common/type_id.h"
#include "graphics/mesh/gltf_loader.h"
#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

#include <functional>
#include <limits>

namespace engine::scene {

    // skeleton:
    Skeleton::Skeleton(const graphics::models::Mesh_Data & meshData) :
        AttachmentObject(ctti::typeIndex<AttachmentObject, Skeleton>()), _meshData(meshData), _nodes(meshData.nodes.size()) {
    }

    uint16_t Skeleton::skinMatricesCount() const noexcept {
        return _meshData.skinMatricesCount;
    }

    bool Skeleton::cmp(const Skeleton & other) const noexcept {
        return std::less<const graphics::models::Mesh_Data*>{}(&_meshData, &other._meshData);
    }

    void Skeleton::fillSkinMatrices2(std::span<mat3x4f> target) const noexcept {
        assert(target.size() >= _meshData.skinMatricesCount);

        uint32_t n = 0u;
        for (const auto & skin : _meshData.skins) {
			const auto numJoints = skin.joints.size();
            for (uint32_t i = 0u; i < numJoints; ++i) {
                const auto & node = _nodes[skin.joints[i]];
                const auto tr = node->getWorldTransform() * skin.inverseBindMatrices[i];
                target[n] = glm::transpose(tr);
                ++n;
            }
		}
    }

    // mesh:
    Mesh::Mesh(const graphics::models::Mesh_Geometry & geometry, uint32_t dataIndex, uint16_t geometryDataIndex, uint16_t skin, uint32_t pipeline) :
        AttachmentObject(ctti::typeIndex<AttachmentObject, Mesh>()), _geometry(geometry), _pipeline(pipeline), _dataIndex(dataIndex), _geometryDataIndex(geometryDataIndex), _skin(skin) {
    }

    void Mesh::setPipeline(uint32_t pipeline) noexcept {
        _pipeline = pipeline;
    }

    uint32_t Mesh::getPipeline() const noexcept {
        return _pipeline;
    }

    bool Mesh::cmp(const Mesh & other) const noexcept {
        if (_pipeline != other._pipeline) {
            return _pipeline < other._pipeline;
        }

        if (_geometryDataIndex != other._geometryDataIndex) {
            return _geometryDataIndex < other._geometryDataIndex;
        }

        if (_skin != other._skin) {
            return _skin < other._skin;
        }

        if (_skeleton != other._skeleton) {
            return std::less<const Skeleton *>{}(_skeleton, other._skeleton);
        }

        // attributes?????
        return _dataIndex < other._dataIndex;
    }

    void Mesh::attachToNode(Node * node) noexcept {
        if (node) {
            vec3f minCorner{std::numeric_limits<float>::max()};
            vec3f maxCorner{std::numeric_limits<float>::lowest()};

            for (const auto & p : _geometry.primitives) {
                minCorner.x = std::min({minCorner.x, p.minCorner.x, p.maxCorner.x});
                minCorner.y = std::min({minCorner.y, p.minCorner.y, p.maxCorner.y});
                minCorner.z = std::min({minCorner.z, p.minCorner.z, p.maxCorner.z});

                maxCorner.x = std::max({maxCorner.x, p.minCorner.x, p.maxCorner.x});
                maxCorner.y = std::max({maxCorner.y, p.minCorner.y, p.maxCorner.y});
                maxCorner.z = std::max({maxCorner.z, p.minCorner.z, p.maxCorner.z});
            }

            node->setAttachment(this);
            node->setBounds(engine::scene::Bounds::createCube(minCorner, maxCorner));
        }
    }
}