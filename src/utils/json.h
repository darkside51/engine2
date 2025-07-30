#pragma once

#include <glaze/glaze.hpp>

#include <string_view>
#include <vector>

namespace engine::json {
    using namespace glz;
    using Json = glz::json_t;
    using Array = glz::json_t::array_t;
    using Object = glz::json_t::object_t;
    using Error = glz::error_ctx;

    inline const Json * findChild(const Json & js, std::string_view name) noexcept {
        if (!js.contains(name)) {
            return nullptr;
        }
        return &js[name];
    }

    template <typename T>
    inline T getValue(const Json & js, std::string_view name, T defaultValue) noexcept {
        if (!js.contains(name)) {
            return defaultValue;
        }
        return js[name].as<T>();
    }

    template <>
    inline bool getValue<bool>(const Json & js, std::string_view name, bool defaultValue) noexcept {
        if (!js.contains(name)) {
            return defaultValue;
        }
        return js[name].get_boolean();
    }

    template <typename T>
    inline std::vector<T> getArray(const Json & js, std::string_view name) noexcept {
        if (auto * array = findChild(js, name)) {
            std::vector<T> result;
            result.reserve(array->size());
            for (const auto & node : array->get_array()) {
                result.emplace_back(node.as<T>());
            }
            return result;
        }
        return {};
    }
}