#pragma once

#include <glaze/glaze.hpp>

namespace engine::json {
    using namespace glz;
    using Json = glz::json_t;
    using Array = glz::json_t::array_t;
    using Object = glz::json_t::object_t;
    using Error = glz::error_ctx;
}