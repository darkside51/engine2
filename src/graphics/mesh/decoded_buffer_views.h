#pragma once

#include "stl/vector.h"

#include <cstdint>
#include <cstddef>

namespace engine::graphics::models {

    struct Layout;
    struct BufferView;

    // holds meshopt-decoded copies of compressed buffer views. each compressed view is
    // decoded lazily, at most once, so indices, vertex attributes, generated tangents,
    // skinned bounds, skin matrices and animation samplers all read the same data
    // through a single code path.
    class DecodedBufferViews {
    public:
        explicit DecodedBufferViews(const Layout & layout);

        // returns the view data (decoded for compressed views) and the byte offset of the
        // view start inside it. returns false when the view cannot be read. compressed views
        // are decoded lazily, at most once per instance.
        bool get(uint16_t bufferViewIndex, const std::byte *& data, uint32_t & offset) noexcept;

        const Layout & layout() noexcept { return _layout; }

    private:
        stl::vector<std::byte> decode(const BufferView & view) const;

        const Layout & _layout;
        stl::vector<stl::vector<std::byte>> _decoded;
        stl::vector<uint8_t> _ready;
    };

}