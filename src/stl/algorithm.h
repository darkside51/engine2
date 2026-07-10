#pragma once

#include "EASTL/algorithm.h"
#include "EASTL/iterator.h"
#include "vector.h"

// #include <algorithm>

namespace engine::stl {

template<typename Iterator>
using move_iterator = eastl::move_iterator<Iterator>;

template <class T, class Allocator, class Predicate>
typename vector<T, Allocator>::size_type erase_if(vector<T, Allocator>& c, Predicate predicate) {
    return eastl::erase_if(c, predicate);
}

}