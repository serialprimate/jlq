#pragma once

#include <string_view>
#include <vector>

namespace jlq
{

    // Parses dot-notation object key paths.
    // Rules:
    // - No leading/trailing '.'
    // - No empty segments ("a..b" invalid)
    // Returns segments as string_views into `path`.
    // Throws std::invalid_argument on invalid input.
    [[nodiscard]] std::vector<std::string_view> parseDotPath(std::string_view path);

} // namespace jlq
