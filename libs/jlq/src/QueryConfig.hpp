#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace jlq
{

    using QueryValue = std::variant<std::monostate, std::string_view, double, bool>;

    struct QueryConfig
    {
        std::vector<std::string_view> path_segments;
        QueryValue value{std::monostate{}};
        bool strict{false};
        std::size_t threads{1};
    };

} // namespace jlq
