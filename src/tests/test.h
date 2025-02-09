#pragma once

#include "../log/log.h"

#include <string_view>
#include <utility>

namespace engine::tests {

    template <typename T, typename...Args>
    void execute(std::string_view name, T && test, Args&&...args) {
        LOG_I("+++++++ start test(%s) +++++++", name.data());
        test.main(std::forward<Args>(args)...);
        LOG_I("+++++++ finish test(%s) +++++++", name.data());
    }

}