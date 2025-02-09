#pragma once

#include "../utils/evaluteable_property.h"
#include "../log/log.h"

#include <functional>

namespace engine::tests {

    struct Prop0 : DynamicProperty<int, std::function<int(float)>> {
        template <typename E>
        Prop0(E&& f) : DynamicProperty(std::move(f)) {}
    };
    struct Prop1 : DynamicProperty<int, std::function<int(float)>> {
        template <typename E>
        Prop1(E&& f) : DynamicProperty(std::move(f)) {}
    };
    struct Prop2 : DynamicProperty<float, std::function<float(float)>> {
        template <typename E>
        Prop2(E&& f) : DynamicProperty(std::move(f)) {}
    };

    struct PropertyTest {
        void main() {
            using P = Properties<Prop0, Prop1, Prop2>;
            P props{
                [](float v){return 1 * v;}, 
                [](float v){return 2 + v;}, 
                [](float v){return 10.0f + 10 * v;}
            };

            const auto ev2 = props.evaluate(10.0f);
            const auto v0 = ev2.get<Prop0>();
            const auto v1 = ev2.get<Prop1>();
            const auto v2 = ev2.get<Prop2>();
            const auto v3 = props.evaluate_one<Prop0>(20.0f);
            const auto v4 = props.evaluate_one<Prop1>(20.0f);
            const auto v5 = props.evaluate_one<Prop2>(20.0f);
        
            std::tuple<int, int, float> t = {1, 2, 123.0f};
            auto & [i1, i2, f1] = t;
        }
    };

}