#include "scene_helper.h"

#include "graphics/mesh/model_manager.h"
#include "math/math.h"
#include "node.h"
#include "objects/mesh.h"
#include "scene.h"

#include <limits>

namespace engine::scene {

void loadNodes(Scene & scene, const graphics::ModelManager::ModelBuffers & gpuBuffers, 
    const graphics::models::MeshData & meshData, uint32_t dataIndex, engine::scene::Node * parent, 
    uint16_t nodeId, engine::scene::Skeleton * skeleton, MeshPipelines pipelines, 
    vec3f & minCorner, vec3f & maxCorner, int16_t order, uint32_t nodeColorId) {

    auto * current = scene.createNode();
    current->setId(nodeColorId);
    if (parent) {
        parent->addChild(current);
    }

    if (skeleton) {
        skeleton->setNode(nodeId, current);
    }

    const auto & node = meshData.nodes[nodeId];
    current->setScale(vec3f{node.scale.x, node.scale.y, node.scale.z});
    current->setRotation(quatf{node.rotation.w, node.rotation.x, node.rotation.y, node.rotation.z});
    current->setPosition(vec3f{node.translation.x, node.translation.y, node.translation.z});

    current->applyTransformations();
    current->calculateWorldTransform(true);

    if (node.mesh != engine::graphics::models::kInvalidValue) {
        bool const createSkeletalMesh = skeleton != nullptr && node.skin != engine::graphics::models::kInvalidValue;
        const auto pipelineId = createSkeletalMesh ? pipelines.skeletalPipeline : pipelines.commonPipeline;

        auto primitiveId = meshData.primitiveBegins[node.mesh];
        for (const auto & p : meshData.meshes[node.mesh].primitives) {
            auto * mesh = scene.createObject<engine::scene::Mesh>(p, /*meshData.materials[p.material], */dataIndex, primitiveId++, node.skin, pipelineId);
                
            mesh->setUniqueIndices({gpuBuffers.uniqueIndicesBuffer});
            mesh->setIndices({gpuBuffers.indexBuffer});
            uint8_t bufferId = 0u;
            mesh->setAttribute({gpuBuffers.vertexBuffers[bufferId++]}, engine::scene::mesh::Semantic::ePosition);
            mesh->setAttribute({gpuBuffers.vertexBuffers[bufferId++]}, engine::scene::mesh::Semantic::eNormal);
            mesh->setAttribute({gpuBuffers.vertexBuffers[bufferId++]}, engine::scene::mesh::Semantic::eTangent);
            mesh->setAttribute({gpuBuffers.vertexBuffers[bufferId++]}, engine::scene::mesh::Semantic::eUv0);

            if (createSkeletalMesh) {
                mesh->setSkeleton(skeleton);
                mesh->setAttribute({gpuBuffers.vertexBuffers[bufferId++]}, engine::scene::mesh::Semantic::eJoints);
                mesh->setAttribute({gpuBuffers.vertexBuffers[bufferId++]}, engine::scene::mesh::Semantic::eWeights);
            }

            mesh->setOrder(order);

            if (nodeColorId != 0xff'ff'ff'ffu) {
                mesh->setEnableColorId(true);
            }

            engine::scene::attachObjectToNode(*mesh, *current);

            // for calculate rootNode size
            //
            // Skinned vertices are transformed by the skin matrices computed from the
            // skeleton pose, which may differ from the bind pose, so for skinned meshes
            // the AABB precomputed at load time from the actually skinned vertices is used.
            // It is already in the mesh root space, so the node transform is not applied.
            //
            // For regular meshes all 8 corners must be transformed: transforming only
            // min/max gives a wrong (too small and shifted) AABB as soon as the node
            // transform contains a rotation or a non-uniform scale.
            if (createSkeletalMesh && p.hasSkinnedBounds) {
                minCorner = engine::math::min(minCorner, p.minCorner);
                maxCorner = engine::math::max(maxCorner, p.maxCorner);
            } else {
                const mat4f & worldTransform = current->getWorldTransform();

                const vec3f corners[8u] = {
                    {p.minCorner.x, p.minCorner.y, p.minCorner.z},
                    {p.maxCorner.x, p.minCorner.y, p.minCorner.z},
                    {p.minCorner.x, p.maxCorner.y, p.minCorner.z},
                    {p.maxCorner.x, p.maxCorner.y, p.minCorner.z},
                    {p.minCorner.x, p.minCorner.y, p.maxCorner.z},
                    {p.maxCorner.x, p.minCorner.y, p.maxCorner.z},
                    {p.minCorner.x, p.maxCorner.y, p.maxCorner.z},
                    {p.maxCorner.x, p.maxCorner.y, p.maxCorner.z}
                };

                for (uint8_t i = 0u; i < 8u; ++i) {
                    const vec3f corner = toWorld(corners[i], worldTransform);
                    minCorner = engine::math::min(minCorner, corner);
                    maxCorner = engine::math::max(maxCorner, corner);
                }
            }
        }
    }

    for (const uint16_t childNodeId : node.children) {
        loadNodes(
            scene, gpuBuffers, meshData, dataIndex, current, childNodeId, skeleton, 
            pipelines, minCorner, maxCorner, order, nodeColorId
        );
    }
};

Node * createNodeWithSkeletalMesh(Scene & scene, const graphics::ModelManager & modelManager, 
    uint32_t modelIndex, Bounds::Type boundsType, MeshPipelines pipelines, ObjectSizeLayout layout, int16_t order, uint32_t nodeColorId) {
    auto * meshRoot = scene.createNode();
    const auto & meshData = modelManager.getModelData(modelIndex);
    engine::scene::Skeleton * skeleton = scene.createObject<engine::scene::Skeleton>(meshData);
    meshRoot->setId(nodeColorId);

    if (skeleton) {
        engine::scene::attachObjectToNode(*skeleton, *meshRoot);
    }

    vec3f minCorner{std::numeric_limits<float>::max()};
    vec3f maxCorner{std::numeric_limits<float>::lowest()};

    for (const auto nodeId : meshData.sceneNodes) {
        loadNodes(scene, modelManager.getModelBuffers(modelIndex), meshData, 
            modelIndex, meshRoot, nodeId, skeleton, pipelines, minCorner, maxCorner, order, nodeColorId);
    }

    // no meshes have been found for the model: bounds cannot be built
    const bool hasBounds = minCorner.x <= maxCorner.x && minCorner.y <= maxCorner.y && minCorner.z <= maxCorner.z;

    if (hasBounds) {
        switch (boundsType) {
        case Bounds::Type::eSphere:
        {
            auto const center = (minCorner + maxCorner) * 0.5f;
            // radius of the sphere enclosing the whole AABB (distance to the farthest corner)
            auto const radius = glm::length(maxCorner - center);
            meshRoot->setBounds(engine::scene::Bounds::createSphere(center, radius));
        }
            break;
        case Bounds::Type::eCube:
        {
            meshRoot->setBounds(engine::scene::Bounds::createCube(minCorner, maxCorner));
        }
            break;
        default:
            break;
        }
    }

    if (hasBounds && layout.type != ObjectSizeLayout::Type::eNone) {
        // todo: check types!
        float scale = 1.0f;
        const auto & meshBounds = meshRoot->getBounds();
        if (meshBounds.type == engine::scene::Bounds::Type::eSphere) {
            const auto radius = meshBounds.volume.sphere.radius;
            if (radius > 0.0f) {
                scale = 0.5f * std::min({layout.size[0u], layout.size[1u], layout.size[2u]}) / radius;
            }
        } else if (meshBounds.type == engine::scene::Bounds::Type::eCube) {
            const auto & cube = meshBounds.volume.cube;
            const auto size = cube.corner1 - cube.corner0;
            // skip degenerate axes, otherwise the scale becomes infinite
            float minScale = std::numeric_limits<float>::max();
            for (uint8_t i = 0u; i < 3u; ++i) {
                const auto extent = std::abs(size[i]);
                if (extent > std::numeric_limits<float>::epsilon()) {
                    minScale = std::min(minScale, layout.size[i] / extent);
                }
            }

            if (minScale < std::numeric_limits<float>::max()) {
                scale = minScale;
            }
        }

        meshRoot->setScale(scale);
    }

    return meshRoot;
}

}