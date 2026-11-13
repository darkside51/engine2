#include "mesh.h"

#include "common/type_id.h"
#include "graphics/mesh/gltf_loader.h"
#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

#include <functional>
#include <limits>

namespace engine::scene {

    // skeleton:
    Skeleton::Skeleton(const graphics::models::MeshData & meshData) :
        AttachmentObject(ctti::typeIndex<AttachmentObject, Skeleton>()), _meshData(meshData), _nodes(meshData.nodes.size()) {
    }

    uint16_t Skeleton::skinMatricesCount() const noexcept {
        return _meshData.skinMatricesCount;
    }

    bool Skeleton::cmp(const Skeleton & other) const noexcept {
        return std::less<const graphics::models::MeshData*>{}(&_meshData, &other._meshData);
    }

    void Skeleton::fillSkinMatrices2(stl::span<mat3x4f> target) const noexcept {
        assert(target.size() >= _meshData.skinMatricesCount);

        uint32_t n = 0u;
        for (const auto & skin : _meshData.skins) {
			const auto numJoints = skin.joints.size();
            for (uint32_t i = 0u; i < numJoints; ++i) {
                target[n] = glm::transpose(
                    _nodes[skin.joints[i]]->getWorldTransform() * skin.inverseBindMatrices[i]
                );
                ++n;
            }
		}
    }

    // mesh:
    Mesh::Mesh(const graphics::models::Primitive & primitive,
        uint32_t dataIndex, uint16_t geometryDataIndex, uint16_t skin, uint32_t pipeline) :
        AttachmentObject(ctti::typeIndex<AttachmentObject, Mesh>()), 
        _geometry(primitive),
        _pipeline(pipeline), 
        _dataIndex(dataIndex), 
        _geometryDataIndex(geometryDataIndex), 
        _skin(skin),
        _material(primitive.material) {
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

        if (_material != other._material) {
            return _material < other._material;
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

        // sizeof(Mesh);
        // sizeof(stl::span<graphics::models::MeshMaterial>);

        if (node) {
            vec3f minCorner{std::numeric_limits<float>::max()};
            vec3f maxCorner{std::numeric_limits<float>::lowest()};

            const auto & nodeBounds = node->getBounds();

            switch(nodeBounds.type) {
            case Bounds::Type::eNone:
            break;
            case Bounds::Type::eCube:
            {
                const auto & cube = nodeBounds.volume.cube;
                minCorner = cube.corner0;
                maxCorner = cube.corner1;
            }
            break;
            case Bounds::Type::eSphere:
            {
                const auto & sphere = nodeBounds.volume.sphere;
                minCorner = sphere.center - vec3f{sphere.radius};
                maxCorner = sphere.center + vec3f{sphere.radius};
            }
            break;
            }

            minCorner.x = std::min({minCorner.x, _geometry.minCorner.x, _geometry.maxCorner.x});
            minCorner.y = std::min({minCorner.y, _geometry.minCorner.y, _geometry.maxCorner.y});
            minCorner.z = std::min({minCorner.z, _geometry.minCorner.z, _geometry.maxCorner.z});

            maxCorner.x = std::max({maxCorner.x, _geometry.minCorner.x, _geometry.maxCorner.x});
            maxCorner.y = std::max({maxCorner.y, _geometry.minCorner.y, _geometry.maxCorner.y});
            maxCorner.z = std::max({maxCorner.z, _geometry.minCorner.z, _geometry.maxCorner.z});

            node->setAttachment(this);
            node->setBounds(engine::scene::Bounds::createCube(minCorner, maxCorner));
        }
    }
}