#pragma once

#include "stl/algorithm.h"
#include "stl/array.h"
#include "stl/vector.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>

namespace engine::stl {

    template<typename Iterator>
    inline stl::move_iterator<Iterator> make_move_iterator(Iterator i) { 
        return eastl::make_move_iterator<Iterator>(std::forward<Iterator>(i)); 
    }

    template<class T, std::size_t N>
    auto make_vector(std::array<T, N> && a)-> stl::vector<T> {
    return {stl::make_move_iterator(a.begin()),
            stl::make_move_iterator(a.end())};
    }

    template <typename InputIterator, typename Predicate>
	inline InputIterator
	find_if(InputIterator first, InputIterator last, Predicate predicate) {
		return eastl::find_if(std::forward<InputIterator>(first), std::forward<InputIterator>(last), std::forward<Predicate>(predicate));
	}

    template<class... T>
    auto make_vector(T && ... t) {
        return make_vector(std::to_array({std::forward<T>(t)...}));
    }
    
}

namespace variant {
    template<typename... Args>
    struct Overload : Args ... {
        using Args::operator()...;
    };

    template<typename... Args>
    Overload(Args &&...) -> Overload<Args...>;  // line not needed since C++20...

    template<typename T, typename... Args>
    inline auto visit(T && v, Args &&...args) {
        return std::visit(Overload<Args...>{std::forward<Args>(args)...}, std::forward<T>(v));
    }
}