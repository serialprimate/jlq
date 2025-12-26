#pragma once

#include <span>
#include <ostream>
#include <string_view>

namespace jlq
{

    int run(std::span<const std::string_view> args, std::ostream &out, std::ostream &err);
    int run(std::span<const std::string_view> args);

} // namespace jlq
