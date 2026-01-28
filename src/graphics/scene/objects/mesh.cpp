#include "mesh.h"

#include "common/type_id.h"
#include "graphics/scene/register.h"
#include "graphics/scene/node.h"

#include <functional>
#include <limits>

namespace engine::scene {
    Mesh::Mesh(const graphics::models::Mesh_Data & meshData, uint16_t meshIdx, uint32_t pipeline) :
        Object(ctti::typeIndex<Object, Mesh>()), _meshData(meshData), _pipeline(pipeline), _meshIdx(meshIdx) {
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

        if (_meshIdx != other._meshIdx) {
            return _meshIdx < other._meshIdx;
        }

        // attributes?????

        return std::less<const graphics::models::Mesh_Data*>{}(&_meshData, &other._meshData);
    }

    void Mesh::attachToNode(Node * node) noexcept {
        if (node) {
            vec3f minCorner{std::numeric_limits<float>::max()};
            vec3f maxCorner{std::numeric_limits<float>::lowest()};

            for (const auto & p : _meshData.meshes[_meshIdx].primitives) {
                minCorner.x = std::min({minCorner.x, p.minCorner.x, p.maxCorner.x});
                minCorner.y = std::min({minCorner.y, p.minCorner.y, p.maxCorner.y});
                minCorner.z = std::min({minCorner.z, p.minCorner.z, p.maxCorner.z});

                maxCorner.x = std::max({maxCorner.x, p.minCorner.x, p.maxCorner.x});
                maxCorner.y = std::max({maxCorner.y, p.minCorner.y, p.maxCorner.y});
                maxCorner.z = std::max({maxCorner.z, p.minCorner.z, p.maxCorner.z});
            }

            node->setObject(this);
            node->setBounds(engine::scene::Bounds::createCube(minCorner, maxCorner));
        }
    }
}