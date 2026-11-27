#include "decoded_buffer_views.h"
#include "gltf_loader.h"

#include "meshoptimizer.h"

namespace engine::graphics::models {

    DecodedBufferViews::DecodedBufferViews(const Layout & layout) : 
    _layout(layout), 
    _decoded(layout.bufferViews.size()), 
    _ready(layout.bufferViews.size(), uint8_t{0u}) {}

    bool DecodedBufferViews::get(uint16_t bufferViewIndex, const std::byte *& data, uint32_t & offset) noexcept {
        if (bufferViewIndex == kInvalidValue || bufferViewIndex >= _layout.bufferViews.size()) {
            return false;
        }

        const auto & view = _layout.bufferViews[bufferViewIndex];
        if (view.mode == MeshoptMode::eUndefined) {
            if (view.buffer >= _layout.buffers.size()) {
                return false;
            }

            data = _layout.buffers[view.buffer].data.data();
            offset = view.offset;
            return data != nullptr;
        }

        if (_ready[bufferViewIndex] == 0u) {
            _decoded[bufferViewIndex] = decode(view);
            _ready[bufferViewIndex] = 1u;
        }

        const auto & decoded = _decoded[bufferViewIndex];
        if (decoded.empty()) {
            return false;
        }

        data = decoded.data();
        offset = 0u;
        return true;
    }

    stl::vector<std::byte> DecodedBufferViews::decode(const BufferView & view) const {
        // every field of the meshopt extension is required
        if (view.buffer == kInvalidValue || view.buffer >= _layout.buffers.size()
            || view.stride == kInvalidValue || view.stride == 0u || view.count == 0u) {
            return {};
        }

        const auto & buffer = _layout.buffers[view.buffer];
        if (view.offset > buffer.data.size() || view.length > buffer.data.size() - view.offset) {
            return {};
        }

        const size_t elementCount = view.count;
        const size_t elementSize = view.stride;
        if (elementCount > std::numeric_limits<size_t>::max() / elementSize) {
            return {};
        }

        stl::vector<std::byte> destination(elementCount * elementSize);
        const auto * compressedData = reinterpret_cast<const uint8_t *>(buffer.data.data() + view.offset);

        int result = -1;
        switch (view.mode) {
        case MeshoptMode::eAttributes:
            result = meshopt_decodeVertexBuffer(destination.data(), elementCount, elementSize, compressedData, view.length);
            break;
        case MeshoptMode::eTriangles:
            result = meshopt_decodeIndexBuffer(destination.data(), elementCount, elementSize, compressedData, view.length);
            break;
        case MeshoptMode::eIndices:
            result = meshopt_decodeIndexSequence(destination.data(), elementCount, elementSize, compressedData, view.length);
            break;
        default:
            return {};
        }

        if (result != 0) {
            assert(false && "failed to decode a meshopt compressed buffer view");
            return {};
        }

        // filters are defined for vertex attribute streams only
        if (view.mode == MeshoptMode::eAttributes) {
            switch (view.filter) {
            case MeshoptFilter::eOctahedral:
                if (elementSize == 4u || elementSize == 8u) {
                    meshopt_decodeFilterOct(destination.data(), elementCount, elementSize);
                }
                break;
            case MeshoptFilter::eQuaternion:
                if (elementSize == 8u) {
                    meshopt_decodeFilterQuat(destination.data(), elementCount, elementSize);
                }
                break;
            case MeshoptFilter::eExponential:
                if ((elementSize & (4u - 1u)) == 0u) { // elementSize % 4u == 0u
                    meshopt_decodeFilterExp(destination.data(), elementCount, elementSize);
                }
                break;
            default:
                break;
            }
        }

        return destination;
    }

}