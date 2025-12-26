#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace jlq
{

    enum class ValueType
    {
        String,
        Number,
        Bool,
        Null,
    };

    struct QueryValue
    {
        ValueType type{ValueType::String};

        // For ValueType::String
        std::string_view string_value{};

        // For ValueType::Number
        double number_value{0.0};

        // For ValueType::Bool
        bool bool_value{false};
    };

    struct QueryConfig
    {
        std::vector<std::string_view> path_segments;
        QueryValue value{};
        bool strict{false};
        std::size_t threads{1};
    };

} // namespace jlq
