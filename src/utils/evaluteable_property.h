#pragma once

#include "traits.h"

namespace engine {

template <typename... Ts>
class Properties {
    using Evaluated_Types = traits::Types<typename Ts::Evaluated_Type...>;
    using Self_Types = std::tuple<Ts...>;

public:
    struct Evaluated : public traits::Tuple<Evaluated_Types> {
        using Base = traits::Tuple<Evaluated_Types>;
        
        template<typename...Us>
        Evaluated(Us&&...us) : Base(std::forward<Us>(us)...) {}

        template<typename T>
        auto& get() noexcept { 
            constexpr auto index = traits::TypeIndex<T, Self_Types>::value;
            return std::get<index>(*this);
        }

        template<typename T>
        const auto& get() const noexcept { 
            constexpr auto index = traits::TypeIndex<T, Self_Types>::value;
            return std::get<index>(*this);
        }

        template<size_t index>
        auto& get() noexcept { 
            return std::get<index>(*this);
        }

        template<size_t index>
        const auto& get() const noexcept { 
            return std::get<index>(*this);
        }
    };

    Properties() = default;

    Properties(Ts&&...ts) : _values(std::forward<Ts>(ts)...) {}

    template <typename...Args>
    Evaluated evaluate(Args&&...args) const noexcept {
        return Evaluated(std::get<Ts>(_values)(std::forward<Args>(args)...)...);
    }

    template <typename...Args>
    Evaluated evaluate2(Args&&...args) const noexcept {
        return evaluateByNums(std::make_index_sequence<std::tuple_size_v<decltype(_values)>>{}, std::forward<Args>(args)...);
    }

    template <typename T, typename...Args>
    auto evaluate_one(Args&&...args) const noexcept {
        constexpr auto index = traits::TypeIndex<T, Self_Types>::value;
        return std::get<index>(_values)(std::forward<Args>(args)...);
    }

    template <size_t index, typename...Args>
    auto evaluate_one2(Args&&...args) const noexcept {
        return std::get<index>(_values)(std::forward<Args>(args)...);
    }

private:
    template <size_t... Is, typename...Args>
    Evaluated evaluateByNums(std::index_sequence<Is...>, Args&&...args) const noexcept {
        return Evaluated(std::get<Is>(_values)(std::forward<Args>(args)...)...);
    }

    Self_Types _values = {};
};

template <typename T, typename E>
class DynamicProperty {
public:
    using Evaluated_Type = T;
    
    DynamicProperty(E&& f) : _exec(std::move(f)) {}

    template <typename...Args>
    inline Evaluated_Type operator()(Args&&...args) const {
        return _exec(std::forward<Args>(args)...);
    }
private:
    E _exec;
};

}