#pragma once

#include <span>
#include <string_view>

namespace jlq
{

    int run(std::span<const std::string_view> args);

} // namespace jlq
