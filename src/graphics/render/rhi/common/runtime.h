#pragma once

#include "instance.h"
#include "device.h"
#include "framebuffer.h"
#include "renderpass.h"
#include "surface.h"
#include "swapchain.h"
#include "types.h"
#include "shared_ref.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::rhi {
struct GraphicsPipelineStatesInfo;
}

namespace engine::rhi::runtime {

using RenderPass = void*;
using Fence = void*;
using Semaphore = void*;
using Event = void*;
class CommandPool;
class CommandList;

template <typename T>
class sh_ptr;

class Instance {
public:
    virtual ~Instance() = default;
};

class Surface {
public:
    virtual ~Surface() = default;
};

class PhysicalDevice {
public:
    virtual ~PhysicalDevice() = default;
    virtual bool checkSurfaceSupport(const Surface & surface) noexcept = 0;
    virtual stl::pair<uint32_t, const QueueFamily*> getQueueFamily(queue_flags::flags flags, uint32_t start = 0u) const noexcept = 0;
    virtual float timestampToMilliseconds(double time) const noexcept = 0;
};

class Device {
public:
    virtual ~Device() = default;
    virtual void waitIdle() const noexcept = 0;
    virtual uint8_t inFlightFramesCount() const noexcept = 0;
    virtual Format getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport = false) const noexcept = 0;
    virtual void resetFrameTmpData(uint8_t frameId) noexcept = 0;
    virtual CommandPool * acquireCommandPool(queue_flags::flags queueFlags, 
        CommandPoolCreateFlags poolFlags = CommandPoolCreateFlags::eReset_Command_Buffer) noexcept = 0;

    virtual size_t getUniformBufferOffsetAlignment() const noexcept = 0;
    virtual size_t getStorageBufferOffsetAlignment() const noexcept = 0;
    virtual size_t getTexelBufferOffsetAlignment() const noexcept = 0;
    
    virtual FormatFeatureAccess isFormatSupported(Format format, FormatFeatureFlags featureFlags) const noexcept = 0;
    virtual bool isExtensionSupported(std::string_view extension) const noexcept = 0;

    virtual void evaluateSampler(rhi::SamplerValue & samplerValue) = 0;
};

struct PipeLineResource {
    virtual ~PipeLineResource() = default;
};

class Buffer : public PipeLineResource {
public:
    virtual ~Buffer() override = default;
    Buffer() = default;
    Buffer(uint32_t sizeInBytes) : _size(sizeInBytes) {}

    uint32_t sizeInBytes() const noexcept { return _size; }

    virtual void * map() noexcept = 0;
    virtual void unmap() noexcept = 0;
    virtual void flush(size_t offset = 0u, size_t size = kWholeSize) noexcept = 0;
    virtual void invalidate(size_t offset = 0u, size_t size = kWholeSize) noexcept = 0;
    virtual bool isHostVisible() const noexcept = 0;

    template <typename T>
    inline T * map2() noexcept { return static_cast<T*>(map()); }

protected:
    uint32_t _size = 0u;
};

class Image : public PipeLineResource {
public:
    Image() = default;
    Image(const rhi::ImageInfo & imageInfo) : _info(imageInfo) {}
    virtual ~Image() = default;

    const rhi::ImageInfo & getInfo() const noexcept { return _info; }
    const rhi::Extent3D & getExtent() const noexcept { return _info.extent; }

    rhi::ImageUsageFlags usage() const noexcept {
        return _info.usage;
    }

    virtual void * map() noexcept = 0;
    virtual void unmap() noexcept = 0;
    virtual void flush(size_t offset = 0u, size_t size = kWholeSize) noexcept = 0;
    virtual void invalidate(size_t offset = 0u, size_t size = kWholeSize) noexcept = 0;
    virtual bool isHostVisible() const noexcept = 0;

protected:
    rhi::ImageInfo _info = {};
};

class Swapchain {
public:
    virtual ~Swapchain() = default;
    virtual void resize(uint32_t w, uint32_t h, bool vsync) = 0;
    virtual stl::pair<uint32_t, bool> acquireNextImage(uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const noexcept = 0;
    virtual uint32_t getImagesCount() const noexcept = 0;
    virtual const Image & getImage(uint8_t index) const noexcept = 0;
    virtual Image & getImage(uint8_t index) noexcept = 0;
    virtual Semaphore imageAvailableSemaphore(uint32_t frameIndex) const noexcept = 0;

    Format getFormat() const noexcept { return _format; }
    ColorSpace getColorSpace() const noexcept { return _colorSpace; }
    rhi::Extent2D getSize() const noexcept {
        return {_width, _height};
    }

protected:
    Format _format = Format::eUndefined;
    ColorSpace _colorSpace = ColorSpace::eSRGB_Nonlinear;
    uint32_t _width = 0u;
    uint32_t _height = 0u;
};

class FrameBuffer : public PipeLineResource {
public:
    virtual ~FrameBuffer() = default;
};

class Queue {
public:
    struct SubmitInfo {
        stl::span<const Semaphore> w8Semaphores;
        stl::span<const rhi::PipelineStageFlags> w8DstStages;
        stl::span<const Semaphore> signalSemaphores;
        stl::span<CommandList *> lists;
    };

    virtual ~Queue() = default;
    virtual bool waitSubmit(uint8_t frameIdx, uint64_t timeout = UINT64_MAX, Fence f = nullptr) noexcept = 0;
    virtual bool submit(uint8_t frameIdx, stl::span<const SubmitInfo> submitInfos, Fence f = nullptr) noexcept = 0;
    virtual bool present(const Swapchain * swapchain, 
        uint32_t imageIdx, stl::span<const rhi::runtime::Semaphore> waitSemaphores) noexcept = 0;
    virtual bool waitIdle() noexcept = 0;
};

struct ShaderModuleView {
    void * shaderModule;
    const void * reflection;  
};

class ShaderModule {
public:
    virtual ~ShaderModule() = default;
    virtual ShaderModuleView getView() const noexcept = 0;
};

class ShaderModulesManager {
public:
    virtual ~ShaderModulesManager() = default;
    virtual const ShaderModule & registerModule(std::string key, const Device & device, 
        rhi::shaders::Stages stage, stl::span<const std::byte> code) = 0;
    virtual const ShaderModule * getModule(std::string_view key) const noexcept = 0;
};

class GpuProgram {
public:
    virtual ~GpuProgram() = default;
};

class Pipeline {
public:
    virtual ~Pipeline() = default;
    explicit Pipeline(PipelineBindPoint p) : bindPoint(p) {}
    
    PipelineBindPoint bindPoint = PipelineBindPoint::eCompute;
};

class PipelinesManager {
public:
    virtual ~PipelinesManager() = default;
    virtual const Pipeline & getGraphicsPipeline(Device & device, const GpuProgram & program, 
        RenderPass renderpass, const rhi::GraphicsPipelineStatesInfo & info) = 0;
    virtual const Pipeline & getComputePipeline(Device & device, const GpuProgram & program) = 0;
};

struct DescriptorSet {
    virtual ~DescriptorSet() = default;
};

class CommandPool {
public:
    virtual ~CommandPool() = default;
};

class QueryPool {
public:
    virtual ~QueryPool() = default;
    virtual bool getResults(uint16_t firstQuery, uint16_t queryCount, stl::span<std::byte> target, 
        size_t stride, QueryResultFlags flags) const noexcept = 0;
};

using SurfacePtr = std::unique_ptr<Surface>;
using PysicalDevicePtr = std::unique_ptr<PhysicalDevice>;
using DevicePtr = std::unique_ptr<Device>;
using ImagePtr = sh_ptr<Image>;//std::unique_ptr<Image>;
using SwapchainPtr = std::unique_ptr<Swapchain>;
using FrameBufferPtr = sh_ptr<FrameBuffer>;//std::unique_ptr<FrameBuffer>;
using QueuePtr = std::unique_ptr<Queue>;
using ShaderModulesManagerPtr = std::unique_ptr<ShaderModulesManager>;
using GpuProgramPtr = std::unique_ptr<GpuProgram>;
using PipelinePtr = std::unique_ptr<Pipeline>;
using PipelinesManagerPtr = std::unique_ptr<PipelinesManager>;
using BufferPtr = sh_ptr<Buffer>;//std::unique_ptr<Buffer>;
using CommandListPtr = std::unique_ptr<CommandList>;
using QueryPoolPtr = std::unique_ptr<QueryPool>;

class Rhi {
public:
    virtual ~Rhi() = default;

    virtual Instance & getInstance() noexcept = 0;

    virtual SurfacePtr createSurface(const SurfaceCreateInfo & info) const = 0;

    virtual stl::vector<PysicalDevicePtr> enumerateDevices(const PhysicalDeviceRequirements & info) const = 0;
    virtual DevicePtr createDevice(PysicalDevicePtr & gpu, const DeviceCreateInfo & info) const = 0;

    virtual SwapchainPtr createSwapchain(const Device & device, const Surface & surface, const SwapchainCreateInfo & info) const = 0;

    virtual ImagePtr createImage(Device & device, const ImageInfo & imageInfo) const noexcept = 0;
            
    virtual RenderPass getRenderPass(Device & device, const RenderPassDescription & description) const noexcept = 0;

    virtual FrameBufferPtr createFrameBuffer(const Device & device,  stl::vector<FrameBufferAttachment> && attachemnts, 
        const RenderPass & renderPass) const noexcept = 0;

    virtual QueuePtr getQueue(const Device & device, queue_flags::flags familyFlags, uint8_t queueIdx, uint8_t framesCount) const = 0;

    virtual Fence createFence(const Device & device, FenceCreateFlags flags) const noexcept = 0;
    virtual Semaphore createSemaphore(const Device & device) const noexcept = 0;
    virtual Event createEvent(const Device & device, EventCreateFlags flags) const noexcept = 0;

    virtual void waitFence(const rhi::runtime::Device & device, 
            const rhi::runtime::Fence & fence, uint64_t timeout) const noexcept = 0;

    virtual void resetFence(const Device & device, const Fence & fence) const noexcept = 0;
    virtual void destroyFence(const Device & device, const Fence & fence) const noexcept = 0;
    virtual void destroySemaphore(const Device & device, const Semaphore & semaphore) const noexcept = 0;
    virtual void destroyEvent(const Device & device, const Event & event) const noexcept = 0;

    virtual void resetEvent(const Device & device, const Event & event) const noexcept = 0;

    virtual ShaderModulesManagerPtr createShaderModulesManager() const = 0;

    virtual GpuProgramPtr createProgram(Device & device, stl::span<const ShaderModuleView> moduleViews, 
        shaders::SpecializationInfo && specializationInfo = {}) const = 0;

    virtual PipelinePtr createComputePipeline(Device & device, const GpuProgramPtr & program) const = 0;

    virtual PipelinePtr createGraphicsPipeline(Device & device, RenderPass renderPass,
        const GpuProgramPtr & program, const GraphicsPipelineStatesInfo & info, uint32_t subpass = 0u) const = 0;

    virtual BufferPtr createBuffer(Device & device, const rhi::BufferInfo & info, size_t size) = 0;

    virtual CommandListPtr createCommandList(CommandPool * commandPool, rhi::CommandBufferLevel level = rhi::CommandBufferLevel::ePrimary) const = 0;
    virtual QueryPoolPtr createQueryPool(Device & device, const rhi::QueryPoolInfo & info) const = 0;
};
}