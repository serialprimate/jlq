#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace jlq
{

    enum class PathSegmentKind
    {
        Key,
        Index,
    };

    struct PathSegment
    {
        PathSegmentKind kind{PathSegmentKind::Key};
        std::string_view key{};
        std::size_t index{0};

        [[nodiscard]] static constexpr PathSegment keySegment(std::string_view k) noexcept
        {
            return PathSegment{PathSegmentKind::Key, k, 0};
        }

        [[nodiscard]] static constexpr PathSegment indexSegment(std::size_t i) noexcept
        {
            PathSegment s;
            s.kind = PathSegmentKind::Index;
            s.index = i;
            return s;
        }
    };

    // Parses dot-notation paths.
    // Rules:
    // - No leading/trailing '.'
    // - No empty segments ("a..b" invalid)
    // - Segments consisting only of digits are parsed as array indices.
    // Returns key segments as string_views into `path`.
    // Throws std::invalid_argument on invalid input.
    [[nodiscard]] std::vector<PathSegment> parseDotPath(std::string_view path);

} // namespace jlq
