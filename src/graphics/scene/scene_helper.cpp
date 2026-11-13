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
            const vec3f corner1 = current->getWorldTransform() * vec4f(p.minCorner, 1.0f);
            const vec3f corner2 = current->getWorldTransform() * vec4f(p.maxCorner, 1.0f);

            minCorner.x = std::min({minCorner.x, corner1.x, corner2.x});
            minCorner.y = std::min({minCorner.y, corner1.y, corner2.y});
            minCorner.z = std::min({minCorner.z, corner1.z, corner2.z});

            maxCorner.x = std::max({maxCorner.x, corner1.x, corner2.x});
            maxCorner.y = std::max({maxCorner.y, corner1.y, corner2.y});
            maxCorner.z = std::max({maxCorner.z, corner1.z, corner2.z});
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

    switch (boundsType) {
    case Bounds::Type::eSphere:
    {
        auto const center = (minCorner + maxCorner) * 0.5f;
        auto const radius = std::max({
            std::abs(minCorner.x - center.x), std::abs(minCorner.y - center.y), std::abs(minCorner.z - center.z),
            std::abs(maxCorner.x - center.x), std::abs(maxCorner.y - center.y), std::abs(maxCorner.z - center.z)
        });
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

    if (layout.type != ObjectSizeLayout::Type::eNone) {
        // todo: check types!
        float scale = 1.0f;
        const auto & meshBounds = meshRoot->getBounds();
        if (meshBounds.type == engine::scene::Bounds::Type::eSphere) {
            scale = 0.5f * std::min({layout.size[0u], layout.size[1u], layout.size[2u]}) / meshBounds.volume.sphere.radius;
        } else if (meshBounds.type == engine::scene::Bounds::Type::eCube) {
            const auto & cube = meshBounds.volume.cube;
            const auto size = cube.corner1 - cube.corner0;
            scale = std::min({layout.size[0u] / std::abs(size.x), layout.size[1u] / std::abs(size.y), layout.size[2u] / std::abs(size.z)});
        }

        meshRoot->setScale(scale);
    }

    return meshRoot;
}

}