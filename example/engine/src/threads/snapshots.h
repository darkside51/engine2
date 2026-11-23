#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <utility>

namespace threads {

    template <typename Snapshot>
    class SnapshotManager {
        static constexpr uint8_t kDirtyBit  = 0x80u; // 1000 0000
        static constexpr uint8_t kIndexMask = 0x03u; // 0000 0011

    public:
        SnapshotManager() noexcept {
            // _writeSlot = 0 (producer)
            // _readSlot  = 1 (consumer)
            // shared     = 2 (free buffer, flag isNew = 0)
            _sharedState.store(2, std::memory_order_relaxed);
        }

        template <typename SnapshotFiller>
        void produce(SnapshotFiller && fill) noexcept { // on fill data thread
            fill(_snapshots[_writeSlot]);

            // pack index and set dirty bit (0x80)
            const uint8_t newShared = _writeSlot | kDirtyBit;

            // atomically swap our buffer with the one previously held in _sharedState. 
            // 'release' semantics ensure that the write to _snapshots is complete 
            const uint8_t prevShared = _sharedState.exchange(newShared, std::memory_order_acq_rel);

            // claim the newly freed buffer (while clearing the 'freshness' bit).
            _writeSlot = prevShared & kIndexMask;
        }

        std::pair<const Snapshot &, bool> consume() noexcept { // on consume data thread (return true, if is new frame)
            const uint8_t currentShared = _sharedState.load(std::memory_order_acquire);
            
            if ((currentShared & kDirtyBit) != 0) {
                // if there is a new frame, we take it for ourselves, and we pass our old _readSlot to _sharedState (without the dirty bit).
                uint8_t prevShared = _sharedState.exchange(_readSlot, std::memory_order_acq_rel);
                _readSlot = prevShared & kIndexMask;
                
                return {_snapshots[_readSlot], true};
            }

            // no new data from the logic yet - read from the old buffer (or skip consume)
            return {_snapshots[_readSlot], false};
        }

    private:
        std::array<Snapshot, 3u> _snapshots{};
        
        // alignment to a cache line (64 bytes) protects against false sharing between threads
        alignas(64) uint8_t _writeSlot = 0u;
        alignas(64) uint8_t _readSlot  = 1u;
        alignas(64) std::atomic<uint8_t> _sharedState{2u};
    };

    template <typename Snapshot>
    class InterpolatedSnapshotManager {
        static constexpr uint8_t kDirtyBit  = 0x80u; // 1000 0000
        static constexpr uint8_t kIndexMask = 0x03u; // 0000 0011

    public:
        struct Frame {
            Snapshot data{};
            double timestamp = 0.0;
        };

        InterpolatedSnapshotManager() noexcept {
            // _writeSlot = 0 (producer)
            // _prevSlot  = 1 (consumer: frame N-1)
            // _currSlot  = 2 (consumer: frame N)
            // shared     = 3 (free buffer)
            _sharedState.store(3, std::memory_order_relaxed);
        }

        template <typename SnapshotFiller>
        void produce(double time, SnapshotFiller && fill) { // on fill data thread, time — the current accumulated simulation time in seconds
            Frame & target = _frames[_writeSlot];
            target.timestamp = time;
            fill(target.data);

            // mark the slot as fresh and replace it with the one stored in _sharedState
            const uint8_t newShared = _writeSlot | kDirtyBit;
            const uint8_t prevShared = _sharedState.exchange(newShared, std::memory_order_acq_rel);

            _writeSlot = prevShared & kIndexMask;
        }

        template <typename Interpolator>
        void consume(double time, Interpolator && read) { // on consume data thread, time — the current consume time (typically currentTime - fixedDeltaTime)
            const uint8_t currentShared = _sharedState.load(std::memory_order_acquire);
            
            if ((currentShared & kDirtyBit) != 0) { // if the logic has prepared a new frame:
                // pass the oldest buffer (_prevSlot) to the shared state, as it is no longer needed.
                const uint8_t prevShared = _sharedState.exchange(_prevSlot, std::memory_order_acq_rel);
                
                // shift the interpolation window: the previous "current" slot becomes the "past" slot, and the fresh slot becomes the "current" one.
                _prevSlot = _currSlot;
                _currSlot = prevShared & kIndexMask;
            }

            const Frame & prev = _frames[_prevSlot];
            const Frame & curr = _frames[_currSlot];

            // calculate the interpolation coefficient, alpha, within the range [0.0, 1.0]
            double duration = curr.timestamp - prev.timestamp;
            float alpha = 1.0f;
            
            if (duration > 1e-6) {
                double rawAlpha = (time - prev.timestamp) / duration;
                alpha = static_cast<float>(std::clamp(rawAlpha, 0.0, 1.0));
            }

            // pass both snapshots and the factor to the function read
            read(prev.data, curr.data, alpha);
        }

    private:
        std::array<Frame, 4u> _frames{};

        alignas(64) uint8_t _writeSlot = 0u;
        alignas(64) uint8_t _prevSlot  = 1u;
        alignas(64) uint8_t _currSlot  = 2u;
        alignas(64) std::atomic<uint8_t> _sharedState{3u};
    };

}