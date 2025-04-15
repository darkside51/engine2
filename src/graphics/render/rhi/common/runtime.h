#pragma once

#include "device.h"
#include "types.h"

#include <cstdint>
#include <utility>

namespace engine::rhi::runtime {

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
    ~PhysicalDevice() = default;
    virtual bool checkSurfaceSupport(const Surface & surface) noexcept = 0;
    virtual std::pair<uint32_t, const QueueFamily*> getQueueFamily(queue_flags::flags flags, uint32_t start = 0u) const noexcept = 0;
};

class Device {
public:
    virtual ~Device() = default;
    virtual Format getSupportedDepthFormat(uint8_t desiredBpp, bool withStencil, bool samplingSupport = false) const noexcept = 0;
};

class Swapchain {
public:
    virtual ~Swapchain() = default;
    virtual void resize(uint32_t w, uint32_t h, bool vsync) = 0;
    virtual std::pair<uint32_t, bool> acquireNextImage(uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const noexcept = 0;
    virtual uint32_t getImagesCount() const noexcept = 0;

    Format getFormat() const noexcept { return _format; }
    ColorSpace getColorSpace() const noexcept { return _colorSpace; }

protected:
    Format _format = Format::eUndefined;
    ColorSpace _colorSpace = ColorSpace::eSRGB_Nonlinear;
};

class Image {
public:
    virtual ~Image() = default;
};

}