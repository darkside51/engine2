#pragma once

#include "EASTL/algorithm.h"
#include "EASTL/iterator.h"
#include <EASTL/sort.h>
#include "vector.h"

// #include <algorithm>

namespace engine::stl {

template<typename Iterator>
using move_iterator = eastl::move_iterator<Iterator>;

template <class T, class Allocator, class Predicate>
typename vector<T, Allocator>::size_type erase_if(vector<T, Allocator>& c, Predicate predicate) {
    return eastl::erase_if(c, predicate);
}

inline constexpr struct {
    template <typename... Args>
    constexpr decltype(auto) operator()(Args &&... args) const {
        return eastl::sort(std::forward<Args>(args)...);
    }
} sort;

inline constexpr struct {
    template <typename... Args>
    constexpr decltype(auto) operator()(Args &&... args) const {
        return eastl::stable_sort(std::forward<Args>(args)...);
    }
} stable_sort;

template <typename Container>
using back_insert_iterator = eastl::back_insert_iterator<Container>;

template <typename Container>
inline back_insert_iterator<Container> back_inserter(Container & x) { return eastl::back_inserter<Container>(x); }

// template <typename... Args>
// inline constexpr auto sort = [](Args &&... args) {
//     eastl::sort(std::forward<Args>(args)...);
// };

// template <typename RandomAccessIterator>
// constexpr auto&& sort = eastl::sort<RandomAccessIterator>;

// template <typename RandomAccessIterator, typename Compare>
// constexpr auto&& sort2 = eastl::sort<RandomAccessIterator, Compare>;
}