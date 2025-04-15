#pragma once

#include "types.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace engine::rhi {

struct PhysicalDeviceFeatures {
    bool robustBufferAccess = false;
    bool fullDrawIndexUint32 = false;
    bool imageCubeArray = false;
    bool independentBlend = false;
    bool geometryShader = false;
    bool tessellationShader = false;
    bool sampleRateShading = false;
    bool dualSrcBlend = false;
    bool logicOp = false;
    bool multiDrawIndirect = false;
    bool drawIndirectFirstInstance = false;
    bool depthClamp = false;
    bool depthBiasClamp = false;
    bool fillModeNonSolid = false;
    bool depthBounds = false;
    bool wideLines = false;
    bool largePoints = false;
    bool alphaToOne = false;
    bool multiViewport = false;
    bool samplerAnisotropy = false;
    bool textureCompressionETC2 = false;
    bool textureCompressionASTC_LDR = false;
    bool textureCompressionBC = false;
    bool occlusionQueryPrecise = false;
    bool pipelineStatisticsQuery = false;
    bool vertexPipelineStoresAndAtomics = false;
    bool fragmentStoresAndAtomics = false;
    bool shaderTessellationAndGeometryPointSize = false;
    bool shaderImageGatherExtended = false;
    bool shaderStorageImageExtendedFormats = false;
    bool shaderStorageImageMultisample = false;
    bool shaderStorageImageReadWithoutFormat = false;
    bool shaderStorageImageWriteWithoutFormat = false;
    bool shaderUniformBufferArrayDynamicIndexing = false;
    bool shaderSampledImageArrayDynamicIndexing = false;
    bool shaderStorageBufferArrayDynamicIndexing = false;
    bool shaderStorageImageArrayDynamicIndexing = false;
    bool shaderClipDistance = false;
    bool shaderCullDistance = false;
    bool shaderFloat64 = false;
    bool shaderInt64 = false;
    bool shaderInt16 = false;
    bool shaderResourceResidency = false;
    bool shaderResourceMinLod = false;
    bool sparseBinding = false;
    bool sparseResidencyBuffer = false;
    bool sparseResidencyImage2D = false;
    bool sparseResidencyImage3D = false;
    bool sparseResidency2Samples = false;
    bool sparseResidency4Samples = false;
    bool sparseResidency8Samples = false;
    bool sparseResidency16Samples = false;
    bool sparseResidencyAliased = false;
    bool variableMultisampleRate = false;
    bool inheritedQueries = false;
};

namespace queue_flags {
    using flags = uint8_t;
    constexpr uint8_t graphics = 1u;
    constexpr uint8_t compute = 2u;
    constexpr uint8_t transfrer = 4u;
    constexpr uint8_t sparse_binding = 8u;
    constexpr uint8_t present = 16u;
}

enum class DeviceType : uint8_t {
    eOther = 0u,
    eIntegratedGPU = 1u,
    eDiscreteGPU = 2u,
    eVirtualGPU = 3u,
    eCPU = 4u
};

struct QueueFamily {
    uint8_t flags = 0u;
    uint32_t count = 0u;
    uint32_t timestampValidBits = 0u;
    Extent3D minImageTransferGranularity;

    inline bool supportFlags(uint8_t f) const noexcept {
        return (flags & f) == f;
    }
};

struct PhysicalDeviceExtFeatures {
    bool descriptorIndexingFeatures = false;
    bool transformFeedbackFeatures = false;
    bool shaderFloat16Int8Features = false;
    bool synchronization2Features = false;
}; 

struct PhysicalDeviceFeatureRequest {
    PhysicalDeviceFeatures features;
    PhysicalDeviceExtFeatures extFeatures;
};

struct PhysicalDeviceRequirements {
    const PhysicalDeviceFeatureRequest & featureRequest;
    std::vector<const char*> requiredExtensions;
    std::optional<DeviceType> requiredDeviceType;
    uint8_t requiredQueueFlags = queue_flags::graphics | queue_flags::present;
};

struct DeviceCreateInfo {
    const PhysicalDeviceFeatureRequest & featureRequest;
    struct QueueFamilyInfo {
        uint32_t index = 0u;
        uint32_t queueCount = 0u;
        std::vector<float> priorities = {};
        uint8_t flags = 0u;
    };
    std::vector<QueueFamilyInfo> queueFamilies;
    uint8_t commandPoolThreadsCount = 1u;
};

}