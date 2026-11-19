#pragma once

#include "stl/span.h"
#include "meta.h"
#include "types.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <utility>

namespace engine::ecs {

class Column {
    using Row = uint32_t;
    
public:
    Column() = delete;

    explicit Column(const ComponentMeta & meta) noexcept : _meta(&meta) {}

    Column(Column && other) noexcept
        : _meta(std::exchange(other._meta, nullptr))
        , _data(std::exchange(other._data, nullptr))
        , _size(std::exchange(other._size, 0u))
        , _capacity(std::exchange(other._capacity, 0u)) {}

    Column & operator= (Column && other) noexcept {
        if (this != &other) {
            release();
            _meta = std::exchange(other._meta, nullptr);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0u);
            _capacity = std::exchange(other._capacity, 0u);
        }
        return *this;
    }

    Column(const Column &) = delete;
    Column & operator= (const Column&) = delete;

    ~Column() noexcept { 
        release(); 
    }

    const ComponentMeta & meta() const noexcept { 
        assert(_meta != nullptr);
        return *_meta; 
    }
    size_t size() const noexcept { return _size; }
    size_t capacity() const noexcept { return _capacity; }
    bool empty() const noexcept { return _size == 0u; }
    bool has_meta() const noexcept { return _meta != nullptr; }

    void * at(Row row) noexcept {
        assert(_meta != nullptr && row < _size);
        return _data + static_cast<size_t>(row) * _meta->size;
    }
    const void * at(Row row) const noexcept { 
        assert(_meta != nullptr && row < _size);
        return _data + static_cast<size_t>(row) * _meta->size; 
    }

    // typed access (debug id test).
    template <typename T>
    T * as(Row row) noexcept {
        assert(_meta != nullptr);
        assert(_meta->type == typeId<T>());
        return static_cast<T*>(at(row));
    }

    // all alive elements
    stl::span<std::byte> raw() noexcept {
        return (_data == nullptr || _size == 0u) ? 
            stl::span<std::byte>{} : 
            stl::span<std::byte>{_data, _size * _meta->size}; 
    }

    void reserve(size_t rows) {
        if (rows <= _capacity) {
            return;
        }

        std::byte * newData = allocate(rows); 
        relocate_all(newData); // noexcept
        deallocate(_data);
        _data = newData;
        _capacity = rows;
    }

    template <typename T, typename... Args>
    T & emplace_back(Args&&... args) {
        assert(_meta != nullptr && _meta->type == typeId<T>());
        if (_size == _capacity) {
            reserve(grow_capacity());
        }

        void * slot = _data + _size * _meta->size;
        T * obj = ::new (slot) T(std::forward<Args>(args)...);
        ++_size;
        return *obj;
    }

    // size++ without creation object
    void * push_back_uninitialized() {
        assert(_meta != nullptr);
        if (_size == _capacity) {
            reserve(grow_capacity());
        }
        return _data + _size++ * _meta->size;
    }

    // the slot is raw; destroy is not needed
    void rollback_last() noexcept { 
        assert(_size > 0u); 
        --_size; 
    }  

    void pop_back() noexcept {
        if (_size == 0u) {
            return;
        }
        --_size;
        if (!_meta->trivially_destructible) {
            _meta->destroy(_data + _size * _meta->size);
        }
    }

    // move srcRow into the raw slot at dstRow in another (or the same) column.
    // the source remains in a "moved-from" state — it will eventually be cleaned up by popBack/removeRow.
    void move_to(Column& dst, Row srcRow, Row dstRow) noexcept {
        assert(_meta != nullptr && dst._meta != nullptr);
        assert(_meta->type == dst._meta->type);
        _meta->move_construct(dst.at(dstRow), at(srcRow));
    }

    // move_replace within a column: used in the swap-and-pop idiom.
    void move_assign_row(Row dstRow, Row srcRow) noexcept {
        assert(_meta != nullptr);
        _meta->move_replace(at(dstRow), at(srcRow));
    }

    // explicit destruction without modifying _size (required for non-swap removal).
    void destroy_row(Row row) noexcept {
        assert(_meta != nullptr && row < _size);
        if (!_meta->trivially_destructible) {
            _meta->destroy(at(row));
        }
    }

    // swapping two live elements. not needed for the archetype (which uses swap&pop),
    // but useful for manual sorting/maintaining a stable order..
    void swap_rows(Row a, Row b) /*noexcept*/ {
        assert(_meta != nullptr && a < _size && b < _size);
        if (a == b) {
            return;
        }
        constexpr size_t kStackBytes = 64u;
        alignas(64) std::byte stackBuf[kStackBytes];

        const size_t bytes = _meta->size;
        const bool useStack = bytes <= kStackBytes && _meta->align <= 64u;

        // allocate(1) can throw std::bad_alloc: todo!
        std::byte* tmp = useStack ? stackBuf : allocate(1);

        
        if (_meta->trivially_relocatable) {
            std::memcpy(tmp,   at(a), bytes);
            std::memcpy(at(a), at(b), bytes);
            std::memcpy(at(b), tmp,   bytes);
        } else {
            _meta->move_construct(tmp, at(a));
            _meta->move_replace(at(a), at(b));
            _meta->move_replace(at(b), tmp);
            _meta->destroy(tmp);
        }

        if (!useStack) {
            deallocate(tmp);
        }
    }

    void clear() noexcept {
        destroy_all();
    }

    void shrink_to_fit() {
        if (_size == _capacity) {
            return;
        }

        if (_size == 0u) { 
            deallocate(_data); 
            _data = nullptr; 
            _capacity = 0u; 
            return; 
        }

        std::byte* newData = allocate(_size);
        relocate_all(newData);
        deallocate(_data);
        _data = newData;
        _capacity = _size;
    }

    void release() noexcept {
        destroy_all();
        deallocate(_data);
        _data = nullptr;
        _capacity = 0u;
        _size = 0u;
    }

private:
    std::byte * allocate(size_t rows) const {
        assert(_meta != nullptr);
        assert(rows <= SIZE_MAX / _meta->size);
        return static_cast<std::byte *>(
            ::operator new(rows * _meta->size, std::align_val_t{_meta->align}));
    }

    void deallocate(std::byte * ptr) const noexcept {
        if (ptr != nullptr) {
            ::operator delete(ptr, std::align_val_t{_meta->align});
        }
    }

    size_t grow_capacity() const noexcept {
        const size_t grown = _capacity + _capacity / 2u + 1u; // x1.5 + 1
        assert(grown > _capacity); // overflow check
        return _capacity ? grown : 8u;
    }

    // move all live elements to a new buffer `dst` (`dst` is aligned, capacity >= `_size`). 
    // mor `trivially_relocatable` types, use a single `memcpy`; otherwise, perform `move_construct` followed by `destroy` for each element individually.
    void relocate_all(std::byte * dst) noexcept {
        if (_size == 0u) return;

        if (_meta->trivially_relocatable) {
            std::memcpy(dst, _data, _size * _meta->size);
            return; // do not destroy the old objects: their dtor is trivial
        }

        for (size_t i = 0u; i < _size; ++i) {
            std::byte * src = _data + i * _meta->size;
            std::byte * out = dst  + i * _meta->size;
            _meta->move_construct(out, src);
            _meta->destroy(src);
        }
    }

    // destroy all live elements without touching the memory.
    void destroy_all() noexcept {
        if (_meta == nullptr) { 
            _size = 0u; return;
        }

        if (!_meta->trivially_destructible) {
            for (size_t i = 0u; i < _size; ++i) {
                _meta->destroy(_data + i * _meta->size);
            }
        }
        _size = 0u;
    }

    const ComponentMeta * _meta = nullptr;
    std::byte * _data = nullptr;
    size_t _size = 0u;
    size_t _capacity = 0u;
};

}